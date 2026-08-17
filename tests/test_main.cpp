#include "proteus/components/Digital.hpp"
#include "proteus/components/Interactive.hpp"
#include "proteus/components/Passive.hpp"
#include "proteus/components/Sources.hpp"
#include "proteus/core/Circuit.hpp"
#include "proteus/simulation/SimulationEngine.hpp"
#include "proteus/wiring/OrthogonalRouter.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

class TestComponent final : public proteus::Component {
public:
    TestComponent(proteus::ComponentId id,
                  std::string label,
                  proteus::Point position = {})
        : Component(id, std::move(label), position) {
        addPin("A", proteus::PinDirection::Input, {-10.0, 0.0});
        addPin("Y", proteus::PinDirection::Output, {10.0, 0.0});
    }

    std::string typeName() const override { return "TestComponent"; }
};

int failures = 0;

void expect(bool condition, const std::string& description) {
    if (!condition) {
        ++failures;
        std::cerr << "[FAIL] " << description << '\n';
    } else {
        std::cout << "[PASS] " << description << '\n';
    }
}

void testComponentTransform() {
    proteus::Circuit circuit;
    auto& component = circuit.emplaceComponent<TestComponent>(
        "U1", proteus::Point{100.0, 50.0});
    expect(component.pinPosition(1).approximatelyEquals({110.0, 50.0}),
           "pin position uses component origin");
    component.rotateClockwise();
    expect(component.pinPosition(1).approximatelyEquals({100.0, 60.0}),
           "rotation updates pin coordinates");
    component.mirrorHorizontal();
    expect(component.pinPosition(1).approximatelyEquals({100.0, 60.0}),
           "horizontal mirror preserves an x-axis pin");
}

void testOrthogonalRouting() {
    const auto route = proteus::OrthogonalRouter::route({10.0, 10.0},
                                                        {50.0, 40.0});
    expect(route.size() == 3, "router inserts one elbow");
    expect(route[1].approximatelyEquals({50.0, 10.0}),
           "router chooses horizontal-first elbow");
    expect(proteus::OrthogonalRouter::isOrthogonal(route),
           "route contains only right-angle segments");
}

void testCircuitConnectivityAndDragging() {
    proteus::Circuit circuit;
    auto& first = circuit.emplaceComponent<TestComponent>(
        "U1", proteus::Point{0.0, 0.0});
    auto& second = circuit.emplaceComponent<TestComponent>(
        "U2", proteus::Point{100.0, 40.0});
    auto& wire = circuit.connect({first.id(), 1}, {second.id(), 0});
    expect(first.pin(1).netId() == second.pin(0).netId(),
           "connected pins share a net");
    const proteus::WireId wireId = wire.id();
    circuit.moveComponent(second.id(), {120.0, 60.0});
    const auto& updatedRoute = circuit.wire(wireId).route();
    expect(updatedRoute.back().approximatelyEquals({110.0, 60.0}),
           "wire endpoint follows a moved component pin");
    expect(proteus::OrthogonalRouter::isOrthogonal(updatedRoute),
           "dragging preserves orthogonal wire geometry");
}

void testPinDetectionAndExplicitJunction() {
    proteus::Circuit circuit;
    auto& left = circuit.emplaceComponent<TestComponent>(
        "LEFT", proteus::Point{0.0, 0.0});
    auto& right = circuit.emplaceComponent<TestComponent>(
        "RIGHT", proteus::Point{100.0, 0.0});
    auto& top = circuit.emplaceComponent<TestComponent>(
        "TOP", proteus::Point{55.0, -50.0});
    auto& bottom = circuit.emplaceComponent<TestComponent>(
        "BOTTOM", proteus::Point{85.0, 50.0});

    const auto& horizontal = circuit.connect({left.id(), 1}, {right.id(), 0});
    const auto& vertical = circuit.connect({top.id(), 1}, {bottom.id(), 0});
    expect(left.pin(1).isMouseOver({10.0, 0.0}, left.pinPosition(1)),
           "pin hit testing highlights a nearby mouse position");
    expect(left.pin(1).netId() != top.pin(1).netId(),
           "crossing wires remain electrically separate without a junction");
    const auto& junction = circuit.addJunction(
        {75.0, 0.0}, {horizontal.id(), vertical.id()});
    expect(left.pin(1).netId() == top.pin(1).netId(),
           "explicit junction joins crossing wire nets");
    circuit.removeJunction(junction.id);
    expect(left.pin(1).netId() != top.pin(1).netId(),
           "removing a junction separates the nets again");
}

void testJunctionLifecycle() {
    proteus::Circuit circuit;
    auto& left = circuit.emplaceComponent<TestComponent>(
        "LEFT", proteus::Point{0.0, 0.0});
    auto& right = circuit.emplaceComponent<TestComponent>(
        "RIGHT", proteus::Point{100.0, 0.0});
    auto& top = circuit.emplaceComponent<TestComponent>(
        "TOP", proteus::Point{55.0, -50.0});
    auto& bottom = circuit.emplaceComponent<TestComponent>(
        "BOTTOM", proteus::Point{85.0, 50.0});
    const auto& horizontal = circuit.connect({left.id(), 1}, {right.id(), 0});
    const auto& vertical = circuit.connect({top.id(), 1}, {bottom.id(), 0});
    circuit.addJunction({75.0, 0.0}, {horizontal.id(), vertical.id()});

    bool duplicateRejected = false;
    try {
        circuit.addJunction({75.0, 0.0}, {horizontal.id(), vertical.id()});
    } catch (const std::invalid_argument&) {
        duplicateRejected = true;
    }
    expect(duplicateRejected, "duplicate wire junction is rejected");

    circuit.moveComponent(bottom.id(), {125.0, 50.0});
    expect(circuit.junctionIds().empty(),
           "moving a wire away removes the stale junction");
    expect(left.pin(1).netId() != top.pin(1).netId(),
           "a removed stale junction no longer joins unrelated nets");
}

void testLogicGateTruthAndUndefinedState() {
    proteus::Circuit circuit;
    auto& gate = circuit.emplaceComponent<proteus::LogicGate>(
        "U1", proteus::GateKind::And, 2, 0.0);
    gate.pin(0).setResolvedVoltage(5.0);
    gate.pin(1).setResolvedVoltage(5.0);
    gate.tick(0.0);
    expect(gate.pin(gate.outputPinIndex()).drivenVoltage() ==
               std::optional<double>{5.0},
           "AND gate produces HIGH for two HIGH inputs");
    gate.pin(1).setResolvedVoltage(0.0);
    gate.tick(0.0);
    expect(gate.pin(gate.outputPinIndex()).drivenVoltage() ==
               std::optional<double>{0.0},
           "AND gate produces LOW when one input is LOW");
    gate.pin(1).setResolvedVoltage(std::nullopt);
    gate.tick(0.0);
    expect(!gate.pin(gate.outputPinIndex()).drivenVoltage().has_value(),
           "undefined input propagates to gate output");
}

void testDFlipFlopRisingEdge() {
    proteus::Circuit circuit;
    auto& flipFlop = circuit.emplaceComponent<proteus::DFlipFlop>("FF1", 0.0);
    flipFlop.pin(0).setResolvedVoltage(5.0);
    flipFlop.pin(1).setResolvedVoltage(0.0);
    flipFlop.tick(0.0);
    expect(!flipFlop.pin(2).drivenVoltage().has_value(),
           "D flip-flop does not sample without a rising edge");
    flipFlop.pin(1).setResolvedVoltage(5.0);
    flipFlop.tick(0.1);
    expect(flipFlop.pin(2).drivenVoltage() == std::optional<double>{5.0},
           "D flip-flop samples D on a rising clock edge");
    flipFlop.pin(0).setResolvedVoltage(0.0);
    flipFlop.tick(0.2);
    expect(flipFlop.pin(2).drivenVoltage() == std::optional<double>{5.0},
           "D flip-flop holds Q while the clock remains HIGH");
}

void testSimulationControlsAndLiveInteraction() {
    proteus::Circuit circuit;
    circuit.emplaceComponent<proteus::Ground>("GND1");
    auto& source = circuit.emplaceComponent<proteus::DcVoltageSource>(
        "V1", 5.0, proteus::Point{-80.0, 0.0});
    auto& switchComponent = circuit.emplaceComponent<proteus::ToggleSwitch>(
        "SW1", true, proteus::Point{-30.0, 0.0});
    auto& gate = circuit.emplaceComponent<proteus::LogicGate>(
        "U1", proteus::GateKind::Not, 1, 0.0,
        proteus::LogicLevels{}, proteus::Point{30.0, 0.0});
    auto& led = circuit.emplaceComponent<proteus::Led>(
        "LED1", "green", 1.8, proteus::Point{90.0, 0.0});

    circuit.connect({source.id(), 0}, {switchComponent.id(), 0});
    circuit.connect({switchComponent.id(), 1}, {gate.id(), 0});
    const auto& outputWire = circuit.connect(
        {gate.id(), gate.outputPinIndex()}, {led.id(), 0});
    circuit.connect({source.id(), 1}, {led.id(), 1});

    proteus::SimulationEngine engine(circuit);
    expect(engine.run(), "valid digital circuit enters Run state");
    expect(engine.state() == proteus::SimulationState::Running,
           "Run control updates engine state");
    expect(!led.isIlluminated(),
           "NOT output is LOW while the closed switch carries HIGH");
    expect(engine.wireColor(outputWire.id()) == "#1976d2",
           "LOW wire is rendered blue during simulation");

    switchComponent.setClosed(false);
    engine.advance(1e-3);
    expect(engine.pinLogicState({gate.id(), 0}) ==
               proteus::LogicState::Undefined,
           "opening a switch updates the circuit live");
    expect(engine.wireColor(outputWire.id()) == "#f9a825",
           "undefined wire is rendered with a distinct color");

    engine.pause();
    expect(engine.state() == proteus::SimulationState::Paused,
           "Pause preserves the circuit in a paused state");
    const double pausedTime = engine.currentTimeSeconds();
    engine.advance(1.0);
    expect(engine.currentTimeSeconds() == pausedTime,
           "ordinary advance does not move paused simulation time");
    engine.stepToNextEvent(0.01);
    expect(engine.currentTimeSeconds() > pausedTime &&
               engine.state() == proteus::SimulationState::Paused,
           "Step advances time and remains paused");
    engine.stop();
    expect(engine.currentTimeSeconds() == 0.0 &&
               engine.state() == proteus::SimulationState::Stopped,
           "Stop resets time and returns to edit mode");
    expect(engine.wireColor(outputWire.id()) == "#404040",
           "stopped simulation restores the default wire color");
}

void testSimulationDiagnostics() {
    proteus::Circuit floatingCircuit;
    floatingCircuit.emplaceComponent<proteus::Ground>("GND1");
    floatingCircuit.emplaceComponent<proteus::LogicGate>(
        "U1", proteus::GateKind::And, 2);
    proteus::SimulationEngine floatingEngine(floatingCircuit);
    expect(!floatingEngine.run(), "floating inputs prevent simulation Run");
    const bool hasFloatingMessage = std::any_of(
        floatingEngine.messages().begin(),
        floatingEngine.messages().end(),
        [](const proteus::SimulationMessage& message) {
            return message.code == "FLOATING_INPUT";
        });
    expect(hasFloatingMessage, "floating input is reported in the log");

    proteus::Circuit shortCircuit;
    shortCircuit.emplaceComponent<proteus::Ground>("GND1");
    auto& highSource = shortCircuit.emplaceComponent<proteus::DcVoltageSource>(
        "VHIGH", 5.0);
    auto& lowSource = shortCircuit.emplaceComponent<proteus::DcVoltageSource>(
        "VLOW", 0.0);
    auto& inverter = shortCircuit.emplaceComponent<proteus::LogicGate>(
        "U1", proteus::GateKind::Not, 1);
    shortCircuit.connect({highSource.id(), 0}, {inverter.id(), 0});
    shortCircuit.connect({lowSource.id(), 0}, {inverter.id(), 0});
    proteus::SimulationEngine shortEngine(shortCircuit);
    expect(!shortEngine.run(), "conflicting output sources prevent Run");
    const bool hasShortMessage = std::any_of(
        shortEngine.messages().begin(),
        shortEngine.messages().end(),
        [](const proteus::SimulationMessage& message) {
            return message.code == "SHORT_CIRCUIT";
        });
    expect(hasShortMessage, "short circuit is reported in the log");
}

void testInvalidNumericValues() {
    proteus::Circuit circuit;

    bool sourceRejected = false;
    try {
        circuit.emplaceComponent<proteus::DcVoltageSource>(
            "V_BAD", std::numeric_limits<double>::quiet_NaN());
    } catch (const std::invalid_argument&) {
        sourceRejected = true;
    }
    expect(sourceRejected, "source rejects a non-finite voltage");

    bool passiveRejected = false;
    try {
        circuit.emplaceComponent<proteus::Resistor>(
            "R_BAD", std::numeric_limits<double>::infinity());
    } catch (const std::invalid_argument&) {
        passiveRejected = true;
    }
    expect(passiveRejected, "passive component rejects a non-finite value");

    bool ledRejected = false;
    try {
        circuit.emplaceComponent<proteus::Led>(
            "LED_BAD", "red", std::numeric_limits<double>::quiet_NaN());
    } catch (const std::invalid_argument&) {
        ledRejected = true;
    }
    expect(ledRejected, "LED rejects a non-finite threshold");

    bool delayRejected = false;
    try {
        circuit.emplaceComponent<proteus::DFlipFlop>(
            "FF_BAD", std::numeric_limits<double>::infinity());
    } catch (const std::invalid_argument&) {
        delayRejected = true;
    }
    expect(delayRejected, "digital component rejects a non-finite delay");
}

}

int main() {
    testComponentTransform();
    testOrthogonalRouting();
    testCircuitConnectivityAndDragging();
    testPinDetectionAndExplicitJunction();
    testJunctionLifecycle();
    testLogicGateTruthAndUndefinedState();
    testDFlipFlopRisingEdge();
    testSimulationControlsAndLiveInteraction();
    testSimulationDiagnostics();
    testInvalidNumericValues();
    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All tests passed\n";
    return EXIT_SUCCESS;
}
