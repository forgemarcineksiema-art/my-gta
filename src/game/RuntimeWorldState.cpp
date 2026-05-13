#include "RuntimeWorldState.h"

namespace bs3d {

void rebuildRuntimeWorldDerivedState(const IntroLevelData& level,
                                     Scene& scene,
                                     WorldCollision& collision,
                                     PropSimulationSystem& propSimulation,
                                     WorldInteraction& interactions,
                                     MissionTriggerSystem& missionTriggers) {
    IntroLevelBuilder::populateWorld(level, scene, collision);
    propSimulation.addPropsFromWorld(level.objects);
    IntroLevelBuilder::populateInteractions(level, interactions);
    missionTriggers.setTriggers(level.missionTriggers);
}

} // namespace bs3d
