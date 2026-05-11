#include "bs3d/core/DriveRouteGuide.h"

#include <utility>

namespace bs3d {

void DriveRouteGuide::reset(std::vector<RoutePoint> points) {
    points_ = std::move(points);
    activeIndex_ = 0;
}

void DriveRouteGuide::clear() {
    points_.clear();
    activeIndex_ = 0;
}

void DriveRouteGuide::update(Vec3 position) {
    while (activeIndex_ < static_cast<int>(points_.size())) {
        const RoutePoint& point = points_[static_cast<std::size_t>(activeIndex_)];
        if (distanceXZ(position, point.position) > point.radius) {
            break;
        }
        ++activeIndex_;
    }
}

const RoutePoint* DriveRouteGuide::activePoint() const {
    if (activeIndex_ < 0 || activeIndex_ >= static_cast<int>(points_.size())) {
        return nullptr;
    }
    return &points_[static_cast<std::size_t>(activeIndex_)];
}

int DriveRouteGuide::activeIndex() const {
    return activeIndex_;
}

bool DriveRouteGuide::isComplete() const {
    return !points_.empty() && activeIndex_ >= static_cast<int>(points_.size());
}

bool DriveRouteGuide::empty() const {
    return points_.empty();
}

float DriveRouteGuide::yawFrom(Vec3 position) const {
    const RoutePoint* point = activePoint();
    if (point == nullptr) {
        return 0.0f;
    }
    return yawFromDirection(point->position - position);
}

} // namespace bs3d
