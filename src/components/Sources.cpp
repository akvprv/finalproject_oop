#include "proteus/components/Sources.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace proteus {

Ground::Ground(ComponentId id, std::string label, Point position)
    : Component(id, std::move(label), position) {
    addPin("GND", PinDirection::Output, {0.0, 12.0});
}

std::string Ground::typeName() const { return "Ground"; }
void Ground::tick(double) { pin(0).drive(0.0); }

DcVoltageSource::DcVoltageSource(ComponentId id,
                                 std::string label,
                                 double voltage,
                                 Point position)
    : Component(id, std::move(label), position), voltage_(0.0) {
    setVoltage(voltage);
    addPin("POS", PinDirection::Output, {0.0, -16.0});
    addPin("NEG", PinDirection::Output, {0.0, 16.0});
}

std::string DcVoltageSource::typeName() const { return "DcVoltageSource"; }
double DcVoltageSource::voltage() const noexcept { return voltage_; }
void DcVoltageSource::setVoltage(double voltage) {
    if (!std::isfinite(voltage)) {
        throw std::invalid_argument("Source voltage must be finite");
    }
    voltage_ = voltage;
}
void DcVoltageSource::tick(double) {
    pin(0).drive(voltage_);
    pin(1).drive(0.0);
}

Battery::Battery(ComponentId id,
                 std::string label,
                 double voltage,
                 double internalResistanceOhms,
                 Point position)
    : Component(id, std::move(label), position),
      voltage_(0.0),
      internalResistanceOhms_(0.0) {
    setVoltage(voltage);
    setInternalResistanceOhms(internalResistanceOhms);
    addPin("POS", PinDirection::Output, {0.0, -16.0});
    addPin("NEG", PinDirection::Output, {0.0, 16.0});
}

std::string Battery::typeName() const { return "Battery"; }
double Battery::voltage() const noexcept { return voltage_; }
double Battery::internalResistanceOhms() const noexcept {
    return internalResistanceOhms_;
}
void Battery::setVoltage(double voltage) {
    if (!std::isfinite(voltage)) {
        throw std::invalid_argument("Battery voltage must be finite");
    }
    voltage_ = voltage;
}
void Battery::setInternalResistanceOhms(double resistance) {
    if (!std::isfinite(resistance) || resistance < 0.0) {
        throw std::invalid_argument(
            "Battery resistance must be finite and non-negative");
    }
    internalResistanceOhms_ = resistance;
}
void Battery::tick(double) {
    pin(0).drive(voltage_);
    pin(1).drive(0.0);
}

ClockGenerator::ClockGenerator(ComponentId id,
                               std::string label,
                               double periodSeconds,
                               double dutyCycle,
                               LogicLevels levels,
                               Point position)
    : Component(id, std::move(label), position),
      periodSeconds_(periodSeconds),
      dutyCycle_(dutyCycle),
      levels_(levels) {
    setPeriodSeconds(periodSeconds);
    setDutyCycle(dutyCycle);
    addPin("OUT", PinDirection::Output, {16.0, 0.0});
    addPin("GND", PinDirection::Output, {-16.0, 0.0});
}

std::string ClockGenerator::typeName() const { return "ClockGenerator"; }
double ClockGenerator::periodSeconds() const noexcept { return periodSeconds_; }
double ClockGenerator::dutyCycle() const noexcept { return dutyCycle_; }
void ClockGenerator::setPeriodSeconds(double periodSeconds) {
    if (!std::isfinite(periodSeconds) || periodSeconds <= 0.0) {
        throw std::invalid_argument("Clock period must be finite and positive");
    }
    periodSeconds_ = periodSeconds;
}
void ClockGenerator::setDutyCycle(double dutyCycle) {
    if (!std::isfinite(dutyCycle) || dutyCycle <= 0.0 || dutyCycle >= 1.0) {
        throw std::invalid_argument("Clock duty cycle must be between 0 and 1");
    }
    dutyCycle_ = dutyCycle;
}
void ClockGenerator::tick(double simulationTimeSeconds) {
    currentTimeSeconds_ = std::max(0.0, simulationTimeSeconds);
    const double phase = std::fmod(currentTimeSeconds_, periodSeconds_);
    const bool high = phase < periodSeconds_ * dutyCycle_;
    pin(0).drive(high ? levels_.highVoltage : levels_.lowVoltage);
    pin(1).drive(levels_.lowVoltage);
}
void ClockGenerator::resetSimulation() {
    Component::resetSimulation();
    currentTimeSeconds_ = 0.0;
}
std::optional<double> ClockGenerator::nextEventTime() const {
    const double phase = std::fmod(currentTimeSeconds_, periodSeconds_);
    const double highEnd = periodSeconds_ * dutyCycle_;
    double delta = phase < highEnd ? highEnd - phase : periodSeconds_ - phase;
    if (delta <= 1e-12) {
        delta = periodSeconds_ * std::min(dutyCycle_, 1.0 - dutyCycle_);
    }
    return currentTimeSeconds_ + delta;
}

}
