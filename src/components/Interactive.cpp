#include "proteus/components/Interactive.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace proteus {

ToggleSwitch::ToggleSwitch(ComponentId id,
                           std::string label,
                           bool initiallyClosed,
                           Point position)
    : Component(id, std::move(label), position), closed_(initiallyClosed) {
    addPin("A", PinDirection::Passive, {-18.0, 0.0});
    addPin("B", PinDirection::Passive, {18.0, 0.0});
}
std::string ToggleSwitch::typeName() const { return "ToggleSwitch"; }
bool ToggleSwitch::isClosed() const noexcept { return closed_; }
void ToggleSwitch::setClosed(bool closed) noexcept { closed_ = closed; }
void ToggleSwitch::toggle() noexcept { closed_ = !closed_; }
std::vector<std::pair<std::size_t, std::size_t>>
ToggleSwitch::conductiveBridges() const {
    return closed_ ? std::vector<std::pair<std::size_t, std::size_t>>{{0, 1}}
                   : std::vector<std::pair<std::size_t, std::size_t>>{};
}

PushButton::PushButton(ComponentId id, std::string label, Point position)
    : Component(id, std::move(label), position) {
    addPin("A", PinDirection::Passive, {-18.0, 0.0});
    addPin("B", PinDirection::Passive, {18.0, 0.0});
}
std::string PushButton::typeName() const { return "PushButton"; }
bool PushButton::isPressed() const noexcept { return pressed_; }
void PushButton::press() noexcept { pressed_ = true; }
void PushButton::release() noexcept { pressed_ = false; }
std::vector<std::pair<std::size_t, std::size_t>>
PushButton::conductiveBridges() const {
    return pressed_ ? std::vector<std::pair<std::size_t, std::size_t>>{{0, 1}}
                    : std::vector<std::pair<std::size_t, std::size_t>>{};
}

Led::Led(ComponentId id,
         std::string label,
         std::string color,
         double thresholdVoltage,
         Point position)
    : Component(id, std::move(label), position),
      color_(std::move(color)),
      thresholdVoltage_(0.0) {
    setThresholdVoltage(thresholdVoltage);
    addPin("ANODE", PinDirection::Input, {-12.0, 0.0});
    addPin("CATHODE", PinDirection::Input, {12.0, 0.0});
}
std::string Led::typeName() const { return "Led"; }
const std::string& Led::color() const noexcept { return color_; }
bool Led::isIlluminated() const noexcept { return illuminated_; }
double Led::thresholdVoltage() const noexcept { return thresholdVoltage_; }
void Led::setColor(std::string color) { color_ = std::move(color); }
void Led::setThresholdVoltage(double voltage) {
    if (!std::isfinite(voltage) || voltage <= 0.0) {
        throw std::invalid_argument(
            "LED threshold voltage must be finite and positive");
    }
    thresholdVoltage_ = voltage;
}
void Led::tick(double) {
    const auto& anode = pin(0).resolvedVoltage();
    const auto& cathode = pin(1).resolvedVoltage();
    illuminated_ = anode.has_value() && cathode.has_value() &&
                   (*anode - *cathode) >= thresholdVoltage_;
}
void Led::resetSimulation() {
    Component::resetSimulation();
    illuminated_ = false;
}

SevenSegment::SevenSegment(ComponentId id,
                           std::string label,
                           bool includeDecimalPoint,
                           LogicLevels levels,
                           Point position)
    : Component(id, std::move(label), position),
      includeDecimalPoint_(includeDecimalPoint),
      levels_(levels) {
    static constexpr std::array<const char*, kSegmentCount> names{
        "A", "B", "C", "D", "E", "F", "G", "DP"};
    for (std::size_t index = 0; index < kSegmentCount; ++index) {
        addPin(names[index], PinDirection::Input,
               {-24.0 + static_cast<double>(index) * 7.0, -18.0});
    }
    addPin("COMMON_CATHODE", PinDirection::Input, {0.0, 24.0});
}
std::string SevenSegment::typeName() const { return "SevenSegment"; }
bool SevenSegment::includesDecimalPoint() const noexcept {
    return includeDecimalPoint_;
}
void SevenSegment::setIncludesDecimalPoint(bool enabled) noexcept {
    includeDecimalPoint_ = enabled;
}
const std::array<bool, SevenSegment::kSegmentCount>&
SevenSegment::illuminatedSegments() const noexcept {
    return illuminatedSegments_;
}
void SevenSegment::tick(double) {
    const auto common = pin(kSegmentCount).resolvedVoltage();
    for (std::size_t index = 0; index < kSegmentCount; ++index) {
        const bool enabledPin = index != kSegmentCount - 1 || includeDecimalPoint_;
        const auto segment = pin(index).resolvedVoltage();
        illuminatedSegments_[index] =
            enabledPin && common.has_value() && segment.has_value() &&
            levels_.classify(segment) == LogicState::High &&
            levels_.classify(common) == LogicState::Low;
    }
}
void SevenSegment::resetSimulation() {
    Component::resetSimulation();
    illuminatedSegments_.fill(false);
}

}
