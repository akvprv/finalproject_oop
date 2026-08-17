#include "proteus/core/Pin.hpp"

#include <utility>

namespace proteus {

Pin::Pin(PinRef reference,
         std::string name,
         PinDirection direction,
         Point localPosition,
         double sensitivityRadius)
    : reference_(reference),
      name_(std::move(name)),
      direction_(direction),
      localPosition_(localPosition),
      sensitivityRadius_(sensitivityRadius) {}

const PinRef& Pin::reference() const noexcept { return reference_; }
const std::string& Pin::name() const noexcept { return name_; }
PinDirection Pin::direction() const noexcept { return direction_; }
const Point& Pin::localPosition() const noexcept { return localPosition_; }
double Pin::sensitivityRadius() const noexcept { return sensitivityRadius_; }
NetId Pin::netId() const noexcept { return netId_; }
void Pin::setNetId(NetId netId) noexcept { netId_ = netId; }

void Pin::setResolvedVoltage(std::optional<double> voltage) noexcept {
    resolvedVoltage_ = voltage;
}

const std::optional<double>& Pin::resolvedVoltage() const noexcept {
    return resolvedVoltage_;
}

void Pin::drive(std::optional<double> voltage) noexcept {
    drivenVoltage_ = voltage;
}

const std::optional<double>& Pin::drivenVoltage() const noexcept {
    return drivenVoltage_;
}

void Pin::clearSimulationState() noexcept {
    resolvedVoltage_.reset();
    drivenVoltage_.reset();
}

bool Pin::isMouseOver(const Point& mousePosition,
                      const Point& absolutePosition) const noexcept {
    return distance(mousePosition, absolutePosition) <= sensitivityRadius_;
}

LogicState Pin::logicState(const LogicLevels& levels) const {
    return levels.classify(resolvedVoltage_);
}

}
