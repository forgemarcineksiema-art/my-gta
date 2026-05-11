#pragma once

#include "bs3d/core/GameFeedback.h"
#include "bs3d/core/MissionController.h"
#include "bs3d/core/MissionTriggerSystem.h"
#include "bs3d/core/StoryState.h"
#include "bs3d/core/WorldEventLedger.h"

#include <functional>
#include <string>

namespace bs3d {

struct MissionRuntimeBridgeResult {
    bool handled = false;
    bool completed = false;
};

struct MissionRuntimeBridgeContext {
    MissionController& mission;
    StoryState& story;
    GameFeedback& feedback;
    WorldEventEmitterCooldowns& eventCooldowns;
    Vec3 vehiclePosition{};
    std::function<void(WorldEventType, Vec3, float, const std::string&)> emitWorldEvent;
};

class MissionRuntimeBridge {
public:
    MissionRuntimeBridgeResult handleTrigger(const MissionTriggerResult& trigger,
                                             MissionRuntimeBridgeContext& context) const;
};

} // namespace bs3d
