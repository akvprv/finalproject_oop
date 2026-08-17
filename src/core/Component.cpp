#include "proteus/core/Component.hpp"

#include <stdexcept>
#include <utility>

namespace proteus {

Component::Component(ComponentId id, std::string label, Point position)
    : id_(id), label_(std::move(label)), position_(position) {}

ComponentId Component::id() const noexcept { return id_; }
const std::string& Component::label() const noexcept { return label_; }
void Component::setLabel(std::string label) { label_ = std::move(label); }
const Point& Component::position() const noexcept { return position_; }
Rotation Component::rotation() const noexcept { return rotation_; }
bool Component::mirroredHorizontally() const noexcept {
    return mirrorHorizontal_;
}
bool Component::mirroredVertically() const noexcept { return mirrorVertical_; }

void Component::moveTo(Point position) noexcept { position_ = position; }

void Component::rotateClockwise() noexcept {
    switch (rotation_) {
        case Rotation::Deg0: rotation_ = Rotation::Deg90; break;
        case Rotation::Deg90: rotation_ = Rotation::Deg180; break;
        case Rotation::Deg180: rotation_ = Rotation::Deg270; break;
        case Rotation::Deg270: rotation_ = Rotation::Deg0; break;
    }
}

void Component::setRotation(Rotation rotation) noexcept { rotation_ = rotation; }
void Component::mirrorHorizontal() noexcept {
    mirrorHorizontal_ = !mirrorHorizontal_;
}
void Component::mirrorVertical() noexcept {
    mirrorVertical_ = !mirrorVertical_;
}

std::size_t Component::pinCount() const noexcept { return pins_.size(); }

Pin& Component::pin(std::size_t index) {
    if (index >= pins_.size()) {
        throw std::out_of_range("Pin index is outside the component pin list");
    }
    return pins_[index];
}

const Pin& Component::pin(std::size_t index) const {
    if (index >= pins_.size()) {
        throw std::out_of_range("Pin index is outside the component pin list");
    }
    return pins_[index];
}

Point Component::pinPosition(std::size_t index) const {
    const Point transformed = transformLocalPoint(pin(index).localPosition());
    return {position_.x + transformed.x, position_.y + transformed.y};
}

void Component::tick(double) {}

void Component::resetSimulation() {
    for (auto& currentPin : pins_) {
        currentPin.clearSimulationState();
    }
}

std::optional<double> Component::nextEventTime() const {
    return std::nullopt;
}

std::vector<std::pair<std::size_t, std::size_t>>
Component::conductiveBridges() const {
    return {};
}

std::size_t Component::addPin(std::string name,
                              PinDirection direction,
                              Point localPosition,
                              double sensitivityRadius) {
    const std::size_t index = pins_.size();
    pins_.emplace_back(PinRef{id_, index},
                       std::move(name),
                       direction,
                       localPosition,
                       sensitivityRadius);
    return index;
}

Point Component::transformLocalPoint(Point localPoint) const noexcept {
    if (mirrorHorizontal_) {
        localPoint.y = -localPoint.y;
    }
    if (mirrorVertical_) {
        localPoint.x = -localPoint.x;
    }

    switch (rotation_) {
        case Rotation::Deg0: return localPoint;
        case Rotation::Deg90: return {-localPoint.y, localPoint.x};
        case Rotation::Deg180: return {-localPoint.x, -localPoint.y};
        case Rotation::Deg270: return {localPoint.y, -localPoint.x};
    }
    return localPoint;
}

}
