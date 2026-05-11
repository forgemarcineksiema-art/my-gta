#include "MissionOutcomeTrigger.h"

namespace bs3d {

namespace {

std::string outcomeTriggerId(const std::string& outcomeId) {
    return "outcome:" + outcomeId;
}

MissionTriggerAction actionForOutcomePhase(MissionPhase phase) {
    switch (phase) {
    case MissionPhase::WalkToShop:
        return MissionTriggerAction::ShopVisitedOnFoot;
    case MissionPhase::DriveToShop:
        return MissionTriggerAction::ShopReachedByVehicle;
    case MissionPhase::ReturnToLot:
        return MissionTriggerAction::DropoffReached;
    case MissionPhase::WaitingForStart:
    case MissionPhase::ReturnToBench:
    case MissionPhase::ReachVehicle:
    case MissionPhase::ChaseActive:
    case MissionPhase::Completed:
    case MissionPhase::Failed:
        break;
    }
    return MissionTriggerAction::None;
}

} // namespace

MissionTriggerResult missionOutcomeTriggerForCurrentPhase(const MissionData& missionData,
                                                          MissionPhase currentPhase,
                                                          const std::string& outcomeId,
                                                          Vec3 outcomePosition,
                                                          float outcomeRadius) {
    if (!missionData.loaded || outcomeId.empty()) {
        return {};
    }

    const MissionTriggerAction action = actionForOutcomePhase(currentPhase);
    if (action == MissionTriggerAction::None) {
        return {};
    }

    const std::string triggerId = outcomeTriggerId(outcomeId);
    for (const MissionPhaseData& phase : missionData.phases) {
        if (missionPhaseFromDataName(phase.phase) != currentPhase || phase.trigger != triggerId) {
            continue;
        }

        return {true, triggerId, action, outcomePosition, outcomeRadius};
    }

    return {};
}

} // namespace bs3d
