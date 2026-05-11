#pragma once

#include "WorldDataLoader.h"

#include "bs3d/core/MissionTriggerSystem.h"

#include <string>

namespace bs3d {

MissionTriggerResult missionOutcomeTriggerForCurrentPhase(const MissionData& missionData,
                                                          MissionPhase currentPhase,
                                                          const std::string& outcomeId,
                                                          Vec3 outcomePosition,
                                                          float outcomeRadius);

} // namespace bs3d
