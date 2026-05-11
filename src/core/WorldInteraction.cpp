#include "bs3d/core/WorldInteraction.h"

#include <limits>
#include <utility>

namespace bs3d {

void WorldInteraction::clear() {
    points_.clear();
}

void WorldInteraction::addPoint(InteractionPoint point) {
    points_.push_back(std::move(point));
}

InteractionPrompt WorldInteraction::query(Vec3 actorPosition) const {
    InteractionPrompt best;
    float bestDistance = std::numeric_limits<float>::max();

    for (const InteractionPoint& point : points_) {
        const float distance = distanceXZ(actorPosition, point.position);
        if (distance <= point.radius && distance < bestDistance) {
            best.available = true;
            best.id = point.id;
            best.prompt = point.prompt;
            best.distance = distance;
            bestDistance = distance;
        }
    }

    return best;
}

const std::vector<InteractionPoint>& WorldInteraction::points() const {
    return points_;
}

} // namespace bs3d
