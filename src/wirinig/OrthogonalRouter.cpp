#include "proteus/wiring/OrthogonalRouter.hpp"

#include <cmath>

namespace proteus {

std::vector<Point> OrthogonalRouter::route(
    Point start,
    Point end,
    FirstSegmentDirection firstSegment) {
    if (start.approximatelyEquals(end)) {
        return {start};
    }
    if (std::abs(start.x - end.x) <= 1e-9 ||
        std::abs(start.y - end.y) <= 1e-9) {
        return {start, end};
    }
    const Point elbow = firstSegment == FirstSegmentDirection::Horizontal
                            ? Point{end.x, start.y}
                            : Point{start.x, end.y};
    return {start, elbow, end};
}

bool OrthogonalRouter::isOrthogonal(const std::vector<Point>& route,
                                    double epsilon) noexcept {
    for (std::size_t index = 1; index < route.size(); ++index) {
        const bool horizontal =
            std::abs(route[index].y - route[index - 1].y) <= epsilon;
        const bool vertical =
            std::abs(route[index].x - route[index - 1].x) <= epsilon;
        if (!horizontal && !vertical) {
            return false;
        }
    }
    return true;
}

}
