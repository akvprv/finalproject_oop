#include "proteus/simulation/SimulationEngine.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace proteus {
namespace {

class DisjointSet {
public:
    explicit DisjointSet(std::size_t size) : parent_(size), rank_(size, 0) {
        std::iota(parent_.begin(), parent_.end(), 0);
    }

    std::size_t find(std::size_t value) {
        if (parent_[value] != value) {
            parent_[value] = find(parent_[value]);
        }
        return parent_[value];
    }

    void unite(std::size_t first, std::size_t second) {
        first = find(first);
        second = find(second);
        if (first == second) {
            return;
        }
        if (rank_[first] < rank_[second]) {
            std::swap(first, second);
        }
        parent_[second] = first;
        if (rank_[first] == rank_[second]) {
            ++rank_[first];
        }
    }

private:
    std::vector<std::size_t> parent_;
    std::vector<unsigned int> rank_;
};

bool equalVoltage(const std::optional<double>& first,
                  const std::optional<double>& second) {
    if (first.has_value() != second.has_value()) {
        return false;
    }
    return !first.has_value() || std::abs(*first - *second) <= 1e-9;
}

}

SimulationEngine::SimulationEngine(Circuit& circuit, LogicLevels levels)
    : circuit_(circuit), levels_(levels) {}

SimulationState SimulationEngine::state() const noexcept { return state_; }
double SimulationEngine::currentTimeSeconds() const noexcept {
    return currentTimeSeconds_;
}
const std::vector<SimulationMessage>& SimulationEngine::messages() const noexcept {
    return messages_;
}
void SimulationEngine::clearMessages() { messages_.clear(); }

bool SimulationEngine::run() {
    if (state_ == SimulationState::Running) {
        return true;
    }
    if (state_ == SimulationState::Stopped) {
        currentTimeSeconds_ = 0.0;
        clearMessages();
        for (const ComponentId id : circuit_.componentIds()) {
            circuit_.component(id).resetSimulation();
        }
        settleAtCurrentTime();
        if (!validateBeforeRun()) {
            state_ = SimulationState::Stopped;
            return false;
        }
    }
    state_ = SimulationState::Running;
    log(MessageSeverity::Info,
        "SIM_RUNNING",
        "Simulation is running.",
        false);
    return true;
}

void SimulationEngine::pause() {
    if (state_ == SimulationState::Running) {
        state_ = SimulationState::Paused;
        log(MessageSeverity::Info,
            "SIM_PAUSED",
            "Simulation is paused; state and pending events are preserved.",
            false);
    }
}

void SimulationEngine::stop() {
    for (const ComponentId id : circuit_.componentIds()) {
        circuit_.component(id).resetSimulation();
    }
    currentTimeSeconds_ = 0.0;
    state_ = SimulationState::Stopped;
    log(MessageSeverity::Info,
        "SIM_STOPPED",
        "Simulation stopped and time was reset.",
        false);
}

void SimulationEngine::advance(double deltaSeconds) {
    if (deltaSeconds < 0.0) {
        throw std::invalid_argument("Simulation delta cannot be negative");
    }
    if (state_ != SimulationState::Running) {
        return;
    }
    currentTimeSeconds_ += deltaSeconds;
    settleAtCurrentTime();
}

void SimulationEngine::runFor(double durationSeconds, double stepSeconds) {
    if (durationSeconds < 0.0 || stepSeconds <= 0.0) {
        throw std::invalid_argument(
            "Duration must be non-negative and the step must be positive");
    }
    if (!run()) {
        return;
    }
    const double target = currentTimeSeconds_ + durationSeconds;
    while (state_ == SimulationState::Running &&
           currentTimeSeconds_ < target - 1e-12) {
        advance(std::min(stepSeconds, target - currentTimeSeconds_));
    }
}

void SimulationEngine::stepToNextEvent(double fallbackStepSeconds) {
    if (fallbackStepSeconds <= 0.0) {
        throw std::invalid_argument("Fallback step must be positive");
    }
    if (state_ == SimulationState::Stopped && !run()) {
        return;
    }
    state_ = SimulationState::Paused;

    double nextTime = currentTimeSeconds_ + fallbackStepSeconds;
    for (const ComponentId id : circuit_.componentIds()) {
        const auto candidate = circuit_.component(id).nextEventTime();
        if (candidate.has_value() &&
            *candidate > currentTimeSeconds_ + 1e-12) {
            nextTime = std::min(nextTime, *candidate);
        }
    }
    currentTimeSeconds_ = nextTime;
    settleAtCurrentTime();
    log(MessageSeverity::Info,
        "SIM_STEP",
        "Simulation advanced to the next event while remaining paused.",
        false);
}

LogicState SimulationEngine::pinLogicState(PinRef reference) const {
    return circuit_.pin(reference).logicState(levels_);
}

LogicState SimulationEngine::wireLogicState(WireId wireId) const {
    return pinLogicState(circuit_.wire(wireId).first());
}

std::string SimulationEngine::wireColor(WireId wireId) const {
    if (state_ == SimulationState::Stopped) {
        return "#404040";
    }
    switch (wireLogicState(wireId)) {
        case LogicState::High: return "#d32f2f";
        case LogicState::Low: return "#1976d2";
        case LogicState::Undefined: return "#f9a825";
    }
    return "#757575";
}

bool SimulationEngine::validateBeforeRun() {
    bool valid = true;
    const auto componentIds = circuit_.componentIds();
    const bool hasGround = std::any_of(
        componentIds.begin(), componentIds.end(), [this](ComponentId id) {
            return circuit_.component(id).typeName() == "Ground";
        });
    if (!hasGround) {
        log(MessageSeverity::Error,
            "MISSING_GROUND",
            "At least one GND component is required before Run.");
        valid = false;
    }

    for (const PinRef reference : circuit_.allPins()) {
        const Pin& currentPin = circuit_.pin(reference);
        if (currentPin.direction() == PinDirection::Input &&
            !currentPin.resolvedVoltage().has_value()) {
            std::ostringstream message;
            message << "Floating input detected: component "
                    << circuit_.component(reference.componentId).label()
                    << ", pin " << currentPin.name() << '.';
            log(MessageSeverity::Error, "FLOATING_INPUT", message.str(), false);
            valid = false;
        }
    }

    const bool hasBlockingDiagnostic = std::any_of(
        messages_.begin(), messages_.end(), [](const SimulationMessage& message) {
            return message.severity == MessageSeverity::Error;
        });
    return valid && !hasBlockingDiagnostic;
}

void SimulationEngine::settleAtCurrentTime() {
    static constexpr std::size_t kMaximumIterations = 64;
    for (std::size_t iteration = 0; iteration < kMaximumIterations; ++iteration) {
        const auto previous = captureResolvedVoltages();
        for (const ComponentId id : circuit_.componentIds()) {
            circuit_.component(id).tick(currentTimeSeconds_);
        }
        resolveNets();
        if (!resolvedVoltagesChanged(previous)) {
            return;
        }
    }
    log(MessageSeverity::Error,
        "NO_CONVERGENCE",
        "Circuit outputs did not converge within 64 iterations.");
}

void SimulationEngine::resolveNets() {
    circuit_.rebuildNets();
    if (circuit_.netCount() == 0) {
        return;
    }
    DisjointSet effectiveNets(circuit_.netCount());
    for (const ComponentId id : circuit_.componentIds()) {
        const auto& currentComponent = circuit_.component(id);
        for (const auto& [firstIndex, secondIndex] :
             currentComponent.conductiveBridges()) {
            if (firstIndex >= currentComponent.pinCount() ||
                secondIndex >= currentComponent.pinCount()) {
                log(MessageSeverity::Error,
                    "INVALID_BRIDGE",
                    "A component exposed an invalid conductive bridge.");
                continue;
            }
            effectiveNets.unite(
                static_cast<std::size_t>(currentComponent.pin(firstIndex).netId()),
                static_cast<std::size_t>(currentComponent.pin(secondIndex).netId()));
        }
    }

    std::unordered_map<std::size_t, std::vector<PinRef>> groups;
    for (const PinRef reference : circuit_.allPins()) {
        const NetId net = circuit_.pin(reference).netId();
        groups[effectiveNets.find(static_cast<std::size_t>(net))].push_back(reference);
    }

    for (const auto& [root, pins] : groups) {
        std::optional<double> resolved;
        bool hasDriver = false;
        bool conflict = false;
        for (const PinRef reference : pins) {
            const Pin& currentPin = circuit_.pin(reference);
            const bool canDrive =
                currentPin.direction() == PinDirection::Output ||
                currentPin.direction() == PinDirection::Bidirectional;
            if (!canDrive || !currentPin.drivenVoltage().has_value()) {
                continue;
            }
            if (!hasDriver) {
                resolved = currentPin.drivenVoltage();
                hasDriver = true;
            } else if (std::abs(*resolved - *currentPin.drivenVoltage()) > 1e-6) {
                conflict = true;
                resolved.reset();
            }
        }
        if (conflict) {
            std::ostringstream message;
            message << "Conflicting output voltages detected on effective net "
                    << root << '.';
            log(MessageSeverity::Error,
                "SHORT_CIRCUIT",
                message.str());
        }
        if (!hasDriver || conflict) {
            resolved.reset();
        }
        for (const PinRef reference : pins) {
            circuit_.pin(reference).setResolvedVoltage(resolved);
        }
    }
}

bool SimulationEngine::resolvedVoltagesChanged(
    const std::vector<std::optional<double>>& previous) const {
    const auto current = captureResolvedVoltages();
    if (previous.size() != current.size()) {
        return true;
    }
    for (std::size_t index = 0; index < previous.size(); ++index) {
        if (!equalVoltage(previous[index], current[index])) {
            return true;
        }
    }
    return false;
}

std::vector<std::optional<double>>
SimulationEngine::captureResolvedVoltages() const {
    std::vector<std::optional<double>> result;
    for (const PinRef reference : circuit_.allPins()) {
        result.push_back(circuit_.pin(reference).resolvedVoltage());
    }
    return result;
}

void SimulationEngine::log(MessageSeverity severity,
                           std::string code,
                           std::string text,
                           bool deduplicate) {
    if (deduplicate) {
        const bool exists = std::any_of(
            messages_.begin(), messages_.end(),
            [&code, &text](const SimulationMessage& message) {
                return message.code == code && message.text == text;
            });
        if (exists) {
            return;
        }
    }
    messages_.push_back(
        {severity, currentTimeSeconds_, std::move(code), std::move(text)});
}

}
