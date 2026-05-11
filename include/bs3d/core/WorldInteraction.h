#pragma once

#include "bs3d/core/Types.h"

#include <string>
#include <vector>

namespace bs3d {

struct InteractionPoint {
    std::string id;
    std::string prompt;
    Vec3 position{};
    float radius = 2.0f;
};

struct InteractionPrompt {
    bool available = false;
    std::string id;
    std::string prompt;
    float distance = 0.0f;
};

class WorldInteraction {
public:
    void clear();
    void addPoint(InteractionPoint point);
    InteractionPrompt query(Vec3 actorPosition) const;
    const std::vector<InteractionPoint>& points() const;

private:
    std::vector<InteractionPoint> points_;
};

} // namespace bs3d
