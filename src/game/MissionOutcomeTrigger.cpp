#include "MissionOutcomeTrigger.h"

namespace bs3d {

namespace {

std::string outcomeTriggerId(const std::string& outcomeId) {
    return "outcome:" + outcomeId;
}

bool outcomeTriggerMatches(const std::string& authoredTrigger, const std::string& outcomeId) {
    const std::string concreteTrigger = outcomeTriggerId(outcomeId);
    if (authoredTrigger == concreteTrigger) {
        return true;
    }
    if (authoredTrigger.size() <= std::string("outcome:*").size() ||
        authoredTrigger.rfind("outcome:", 0) != 0 ||
        authoredTrigger.back() != '*') {
        return false;
    }
    const std::string prefix = authoredTrigger.substr(0, authoredTrigger.size() - 1);
    return concreteTrigger.rfind(prefix, 0) == 0;
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

    for (const MissionPhaseData& phase : missionData.phases) {
        if (missionPhaseFromDataName(phase.phase) != currentPhase ||
            !outcomeTriggerMatches(phase.trigger, outcomeId)) {
            continue;
        }

        return {true, phase.trigger, action, outcomePosition, outcomeRadius};
    }

    return {};
}

} // namespace bs3d
