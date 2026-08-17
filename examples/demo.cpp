#include "proteus/components/Digital.hpp"
#include "proteus/components/Interactive.hpp"
#include "proteus/components/Sources.hpp"
#include "proteus/core/Circuit.hpp"
#include "proteus/simulation/SimulationEngine.hpp"

#include <iostream>

int main() {
    proteus::Circuit circuit;
    circuit.emplaceComponent<proteus::Ground>("GND1");
    auto& supply = circuit.emplaceComponent<proteus::DcVoltageSource>(
        "V1", 5.0, proteus::Point{0.0, 50.0});
    auto& switchComponent = circuit.emplaceComponent<proteus::ToggleSwitch>(
        "SW1", true, proteus::Point{70.0, 20.0});
    auto& inverter = circuit.emplaceComponent<proteus::LogicGate>(
        "U1", proteus::GateKind::Not, 1, 0.0,
        proteus::LogicLevels{}, proteus::Point{150.0, 20.0});
    auto& led = circuit.emplaceComponent<proteus::Led>(
        "LED1", "green", 1.8, proteus::Point{230.0, 20.0});

    circuit.connect({supply.id(), 0}, {switchComponent.id(), 0});
    circuit.connect({switchComponent.id(), 1}, {inverter.id(), 0});
    const auto& outputWire = circuit.connect(
        {inverter.id(), inverter.outputPinIndex()}, {led.id(), 0});
    circuit.connect({supply.id(), 1}, {led.id(), 1});

    proteus::SimulationEngine engine(circuit);
    if (!engine.run()) {
        std::cerr << "Simulation could not start\n";
        return 1;
    }
    std::cout << "Switch closed -> LED: "
              << (led.isIlluminated() ? "ON" : "OFF") << '\n';
    std::cout << "Output wire color: " << engine.wireColor(outputWire.id())
              << '\n';

    switchComponent.setClosed(false);
    engine.advance(1e-3);
    std::cout << "Switch open -> output state: "
              << (engine.wireLogicState(outputWire.id()) ==
                          proteus::LogicState::Undefined
                      ? "UNDEFINED"
                      : "DEFINED")
              << '\n';
}
