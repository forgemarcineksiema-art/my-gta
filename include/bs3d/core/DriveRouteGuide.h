#pragma once

#include "bs3d/core/Types.h"

#include <string>
#include <vector>

namespace bs3d {

struct RoutePoint {
    Vec3 position{};
    float radius = 2.0f;
    std::string label;
};

class DriveRouteGuide {
public:
    void reset(std::vector<RoutePoint> points);
    void clear();
    void update(Vec3 position);

    const RoutePoint* activePoint() const;
    int activeIndex() const;
    bool isComplete() const;
    bool empty() const;
    float yawFrom(Vec3 position) const;

private:
    std::vector<RoutePoint> points_;
    int activeIndex_ = 0;
};

} // namespace bs3d
