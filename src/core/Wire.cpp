#include "proteus/core/Wire.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace proteus {
namespace {

double distanceToSegment(Point point, Point first, Point second) {
    const double dx = second.x - first.x;
    const double dy = second.y - first.y;
    const double lengthSquared = dx * dx + dy * dy;
    if (lengthSquared == 0.0) {
        return distance(point, first);
    }
    const double projection = std::clamp(
        ((point.x - first.x) * dx + (point.y - first.y) * dy) /
            lengthSquared,
        0.0,
        1.0);
    return distance(point, {first.x + projection * dx,
                            first.y + projection * dy});
}

}

Wire::Wire(WireId id, PinRef first, PinRef second, std::vector<Point> route)
    : id_(id), first_(first), second_(second), route_(std::move(route)) {}

WireId Wire::id() const noexcept { return id_; }
const PinRef& Wire::first() const noexcept { return first_; }
const PinRef& Wire::second() const noexcept { return second_; }
const std::vector<Point>& Wire::route() const noexcept { return route_; }
void Wire::setRoute(std::vector<Point> route) { route_ = std::move(route); }

bool Wire::containsPoint(Point point, double tolerance) const noexcept {
    if (route_.size() < 2) {
        return false;
    }
    for (std::size_t index = 1; index < route_.size(); ++index) {
        if (distanceToSegment(point, route_[index - 1], route_[index]) <=
            tolerance) {
            return true;
        }
    }
    return false;
}

}
