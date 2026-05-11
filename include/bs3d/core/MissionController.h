#pragma once

#include "bs3d/core/DialogueSystem.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace bs3d {

enum class MissionPhase {
    WaitingForStart,
    WalkToShop,
    ReturnToBench,
    ReachVehicle,
    DriveToShop,
    ChaseActive,
    ReturnToLot,
    Completed,
    Failed
};

class MissionController {
public:
    explicit MissionController(DialogueSystem& dialogue);

    void start();
    void onShopVisitedOnFoot();
    void onReturnedToBogus();
    void onPlayerEnteredVehicle();
    void onShopReachedByVehicle();
    void onChaseEscaped();
    void onDropoffReached();
    void fail(std::string reason);
    void retry();
    void retryToCheckpoint(MissionPhase checkpointPhase);
    void restoreForSave(MissionPhase phase, float phaseSeconds);
    void setObjectiveOverride(MissionPhase phase, std::string objective);

    MissionPhase phase() const;
    float phaseSeconds() const;
    bool chaseWanted() const;
    bool consumeChaseWanted();
    const std::string& failureReason() const;
    std::string objectiveText() const;
    void setMissionDialogueLinesForPhase(MissionPhase phase, std::vector<DialogueLine> lines);
    void setNpcReactionLinesForPhase(MissionPhase phase, std::vector<DialogueLine> lines);
    void setCutsceneLinesForPhase(MissionPhase phase, std::vector<DialogueLine> lines);

private:
    DialogueSystem& dialogue_;
    MissionPhase phase_ = MissionPhase::WaitingForStart;
    float phaseSeconds_ = 0.0f;
    bool chaseWanted_ = false;
    std::string failureReason_;
    std::unordered_map<int, std::string> objectiveOverrides_;
    std::unordered_map<int, std::vector<DialogueLine>> missionPhaseDialogues_;
    std::unordered_map<int, std::vector<DialogueLine>> missionPhaseNpcReactions_;
    std::unordered_map<int, std::vector<DialogueLine>> missionPhaseCutscenes_;

    void pushObjectiveLine(const std::string& text);
    void pushObjectiveForPhase(MissionPhase phase);
    void setLinesForPhase(std::unordered_map<int, std::vector<DialogueLine>>& bucket,
                         MissionPhase phase,
                         std::vector<DialogueLine> lines);
    void queuePhaseLines(const std::unordered_map<int, std::vector<DialogueLine>>& linesByPhase,
                        MissionPhase phase);
    void queueMissionPhaseLines(MissionPhase phase);
};

} // namespace bs3d
