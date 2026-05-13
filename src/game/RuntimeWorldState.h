#pragma once

#include "IntroLevelBuilder.h"
#include "PropSimulationSystem.h"

#include "bs3d/core/MissionTriggerSystem.h"
#include "bs3d/core/Scene.h"
#include "bs3d/core/WorldCollision.h"
#include "bs3d/core/WorldInteraction.h"

namespace bs3d {

void rebuildRuntimeWorldDerivedState(const IntroLevelData& level,
                                     Scene& scene,
                                     WorldCollision& collision,
                                     PropSimulationSystem& propSimulation,
                                     WorldInteraction& interactions,
                                     MissionTriggerSystem& missionTriggers);

} // namespace bs3d
