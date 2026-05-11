#include "bs3d/core/InteractionResolver.h"

#include <limits>
#include <utility>

namespace bs3d {

void InteractionResolver::clear() {
    candidates_.clear();
}

void InteractionResolver::addCandidate(InteractionCandidate candidate) {
    candidates_.push_back(std::move(candidate));
}

InteractionResolution InteractionResolver::resolve(Vec3 actorPosition, InteractionInput input) const {
    InteractionResolution best;
    float bestDistance = std::numeric_limits<float>::max();

    for (const InteractionCandidate& candidate : candidates_) {
        if (!candidate.enabled || candidate.input != input || candidate.action == InteractionAction::None) {
            continue;
        }

        const float distance = distanceXZ(actorPosition, candidate.position);
        if (distance > candidate.radius) {
            continue;
        }

        const bool higherPriority = !best.available || candidate.priority > best.priority;
        const bool samePriorityCloser = best.available && candidate.priority == best.priority && distance < bestDistance;
        if (higherPriority || samePriorityCloser) {
            best.available = true;
            best.id = candidate.id;
            best.action = candidate.action;
            best.input = candidate.input;
            best.prompt = candidate.prompt;
            best.distance = distance;
            best.priority = candidate.priority;
            bestDistance = distance;
        }
    }

    return best;
}

const std::vector<InteractionCandidate>& InteractionResolver::candidates() const {
    return candidates_;
}

} // namespace bs3d
