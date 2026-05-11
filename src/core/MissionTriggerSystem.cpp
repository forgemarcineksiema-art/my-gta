#include "bs3d/core/MissionTriggerSystem.h"

#include <algorithm>

namespace bs3d {

void MissionTriggerSystem::setTriggers(std::vector<MissionTriggerDefinition> triggers) {
    triggers_ = std::move(triggers);
    resetConsumed();
}

void MissionTriggerSystem::resetConsumed() {
    consumedIds_.clear();
}

void MissionTriggerSystem::clear() {
    triggers_.clear();
    consumedIds_.clear();
}

MissionTriggerResult MissionTriggerSystem::update(MissionPhase phase,
                                                  Vec3 playerPosition,
                                                  Vec3 vehiclePosition,
                                                  bool playerInVehicle) {
    for (const MissionTriggerDefinition& trigger : triggers_) {
        if (trigger.phase != phase || trigger.action == MissionTriggerAction::None || isConsumed(trigger.id)) {
            continue;
        }

        const bool actorAvailable =
            trigger.actor == MissionTriggerActor::Vehicle ? playerInVehicle : !playerInVehicle;
        if (!actorAvailable) {
            continue;
        }

        const Vec3 actorPosition = trigger.actor == MissionTriggerActor::Vehicle ? vehiclePosition : playerPosition;
        if (distanceXZ(actorPosition, trigger.position) > trigger.radius) {
            continue;
        }

        markConsumed(trigger.id);
        return {true, trigger.id, trigger.action, trigger.position, trigger.radius};
    }

    return {};
}

const std::vector<MissionTriggerDefinition>& MissionTriggerSystem::triggers() const {
    return triggers_;
}

bool MissionTriggerSystem::isConsumed(const std::string& id) const {
    return std::find(consumedIds_.begin(), consumedIds_.end(), id) != consumedIds_.end();
}

void MissionTriggerSystem::markConsumed(const std::string& id) {
    consumedIds_.push_back(id);
}

} // namespace bs3d
