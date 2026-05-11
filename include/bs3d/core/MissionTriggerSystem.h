#pragma once

#include "bs3d/core/MissionController.h"
#include "bs3d/core/Types.h"

#include <string>
#include <vector>

namespace bs3d {

enum class MissionTriggerActor {
    Player,
    Vehicle
};

enum class MissionTriggerAction {
    None,
    ShopVisitedOnFoot,
    ShopReachedByVehicle,
    DropoffReached
};

struct MissionTriggerDefinition {
    std::string id;
    MissionPhase phase = MissionPhase::WaitingForStart;
    MissionTriggerActor actor = MissionTriggerActor::Player;
    Vec3 position{};
    float radius = 1.0f;
    MissionTriggerAction action = MissionTriggerAction::None;
};

struct MissionTriggerResult {
    bool triggered = false;
    std::string id;
    MissionTriggerAction action = MissionTriggerAction::None;
    Vec3 position{};
    float radius = 0.0f;
};

class MissionTriggerSystem {
public:
    void setTriggers(std::vector<MissionTriggerDefinition> triggers);
    void resetConsumed();
    void clear();

    MissionTriggerResult update(MissionPhase phase, Vec3 playerPosition, Vec3 vehiclePosition, bool playerInVehicle);
    const std::vector<MissionTriggerDefinition>& triggers() const;

private:
    std::vector<MissionTriggerDefinition> triggers_;
    std::vector<std::string> consumedIds_;

    bool isConsumed(const std::string& id) const;
    void markConsumed(const std::string& id);
};

} // namespace bs3d
