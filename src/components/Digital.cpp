#include "proteus/components/Digital.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace proteus {
namespace {

bool equalVoltage(const std::optional<double>& first,
                  const std::optional<double>& second) {
    if (first.has_value() != second.has_value()) {
        return false;
    }
    return !first.has_value() || std::abs(*first - *second) <= 1e-9;
}

}

LogicGate::LogicGate(ComponentId id,
                     std::string label,
                     GateKind kind,
                     std::size_t inputCount,
                     double propagationDelaySeconds,
                     LogicLevels levels,
                     Point position)
    : Component(id, std::move(label), position),
      kind_(kind),
      inputCount_(inputCount),
      propagationDelaySeconds_(propagationDelaySeconds),
      levels_(levels) {
    if (kind == GateKind::Not) {
        inputCount_ = 1;
    } else if (inputCount < 2) {
        throw std::invalid_argument("A non-NOT gate needs at least two inputs");
    }
    setPropagationDelaySeconds(propagationDelaySeconds);
    for (std::size_t index = 0; index < inputCount_; ++index) {
        const double offset =
            (static_cast<double>(index) -
             static_cast<double>(inputCount_ - 1) / 2.0) * 12.0;
        addPin("IN" + std::to_string(index + 1),
               PinDirection::Input,
               {-24.0, offset});
    }
    addPin("OUT", PinDirection::Output, {24.0, 0.0});
}

std::string LogicGate::typeName() const {
    switch (kind_) {
        case GateKind::And: return "AndGate";
        case GateKind::Or: return "OrGate";
        case GateKind::Not: return "NotGate";
        case GateKind::Xor: return "XorGate";
        case GateKind::Nand: return "NandGate";
    }
    return "LogicGate";
}

GateKind LogicGate::kind() const noexcept { return kind_; }
std::size_t LogicGate::inputCount() const noexcept { return inputCount_; }
std::size_t LogicGate::outputPinIndex() const noexcept { return inputCount_; }
double LogicGate::propagationDelaySeconds() const noexcept {
    return propagationDelaySeconds_;
}
void LogicGate::setPropagationDelaySeconds(double delaySeconds) {
    if (!std::isfinite(delaySeconds) || delaySeconds < 0.0) {
        throw std::invalid_argument(
            "Propagation delay must be finite and non-negative");
    }
    propagationDelaySeconds_ = delaySeconds;
}

void LogicGate::tick(double simulationTimeSeconds) {
    if (pendingOutput_.has_value() &&
        pendingOutput_->first <= simulationTimeSeconds + 1e-12) {
        pin(outputPinIndex()).drive(pendingOutput_->second);
        pendingOutput_.reset();
    }
    requestOutput(levels_.voltageFor(evaluateLogic()), simulationTimeSeconds);
}

void LogicGate::resetSimulation() {
    Component::resetSimulation();
    hasRequestedOutput_ = false;
    requestedOutput_.reset();
    pendingOutput_.reset();
}

std::optional<double> LogicGate::nextEventTime() const {
    return pendingOutput_.has_value()
               ? std::optional<double>{pendingOutput_->first}
               : std::nullopt;
}

LogicState LogicGate::evaluateLogic() const {
    std::vector<LogicState> inputs;
    inputs.reserve(inputCount_);
    for (std::size_t index = 0; index < inputCount_; ++index) {
        const LogicState state = pin(index).logicState(levels_);
        if (state == LogicState::Undefined) {
            return LogicState::Undefined;
        }
        inputs.push_back(state);
    }
    const auto isHigh = [](LogicState state) {
        return state == LogicState::High;
    };
    switch (kind_) {
        case GateKind::And:
            return std::all_of(inputs.begin(), inputs.end(), isHigh)
                       ? LogicState::High
                       : LogicState::Low;
        case GateKind::Or:
            return std::any_of(inputs.begin(), inputs.end(), isHigh)
                       ? LogicState::High
                       : LogicState::Low;
        case GateKind::Not:
            return isHigh(inputs.front()) ? LogicState::Low : LogicState::High;
        case GateKind::Xor: {
            const auto highCount = std::count_if(inputs.begin(), inputs.end(), isHigh);
            return highCount % 2 == 1 ? LogicState::High : LogicState::Low;
        }
        case GateKind::Nand:
            return std::all_of(inputs.begin(), inputs.end(), isHigh)
                       ? LogicState::Low
                       : LogicState::High;
    }
    return LogicState::Undefined;
}

void LogicGate::requestOutput(std::optional<double> voltage,
                              double simulationTimeSeconds) {
    if (hasRequestedOutput_ && equalVoltage(requestedOutput_, voltage)) {
        return;
    }
    hasRequestedOutput_ = true;
    requestedOutput_ = voltage;
    if (propagationDelaySeconds_ <= 1e-12) {
        pin(outputPinIndex()).drive(voltage);
        pendingOutput_.reset();
    } else {
        pendingOutput_ = std::make_pair(
            simulationTimeSeconds + propagationDelaySeconds_, voltage);
    }
}

DFlipFlop::DFlipFlop(ComponentId id,
                     std::string label,
                     double propagationDelaySeconds,
                     LogicLevels levels,
                     Point position)
    : Component(id, std::move(label), position),
      propagationDelaySeconds_(propagationDelaySeconds),
      levels_(levels) {
    setPropagationDelaySeconds(propagationDelaySeconds);
    addPin("D", PinDirection::Input, {-24.0, -10.0});
    addPin("CLK", PinDirection::Input, {-24.0, 10.0});
    addPin("Q", PinDirection::Output, {24.0, 0.0});
}

std::string DFlipFlop::typeName() const { return "DFlipFlop"; }
double DFlipFlop::propagationDelaySeconds() const noexcept {
    return propagationDelaySeconds_;
}
void DFlipFlop::setPropagationDelaySeconds(double delaySeconds) {
    if (!std::isfinite(delaySeconds) || delaySeconds < 0.0) {
        throw std::invalid_argument(
            "Propagation delay must be finite and non-negative");
    }
    propagationDelaySeconds_ = delaySeconds;
}

void DFlipFlop::tick(double simulationTimeSeconds) {
    if (pendingOutput_.has_value() &&
        pendingOutput_->first <= simulationTimeSeconds + 1e-12) {
        pin(2).drive(pendingOutput_->second);
        pendingOutput_.reset();
    }
    const LogicState clock = pin(1).logicState(levels_);
    if (previousClock_ == LogicState::Low && clock == LogicState::High) {
        const auto sampled = levels_.voltageFor(pin(0).logicState(levels_));
        if (propagationDelaySeconds_ <= 1e-12) {
            pin(2).drive(sampled);
        } else {
            pendingOutput_ = std::make_pair(
                simulationTimeSeconds + propagationDelaySeconds_, sampled);
        }
    }
    previousClock_ = clock;
}

void DFlipFlop::resetSimulation() {
    Component::resetSimulation();
    previousClock_ = LogicState::Undefined;
    pendingOutput_.reset();
}

std::optional<double> DFlipFlop::nextEventTime() const {
    return pendingOutput_.has_value()
               ? std::optional<double>{pendingOutput_->first}
               : std::nullopt;
}

}
