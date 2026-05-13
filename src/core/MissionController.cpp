#include "bs3d/core/MissionController.h"

#include <utility>

namespace bs3d {

MissionController::MissionController(DialogueSystem& dialogue) : dialogue_(dialogue) {}

void MissionController::start() {
    // Only start from WaitingForStart — prevents double-start.
    if (phase_ != MissionPhase::WaitingForStart) {
        return;
    }

    phase_ = MissionPhase::WalkToShop;
    phaseSeconds_ = 0.0f;
    failureReason_.clear();
    const int phaseKey = static_cast<int>(phase_);
    const bool hasPhaseDialogue =
        missionPhaseDialogues_.find(phaseKey) != missionPhaseDialogues_.end() ||
        missionPhaseNpcReactions_.find(phaseKey) != missionPhaseNpcReactions_.end() ||
        missionPhaseCutscenes_.find(phaseKey) != missionPhaseCutscenes_.end();

    if (!hasPhaseDialogue) {
        dialogue_.push(
            {"Bogu\u015b", "Wr\u00f3ci\u0142e\u015b. Id\u017a do Zenona i sprawd\u017a, czy jeszcze liczy d\u0142ugi.",
             4.4f, DialogueChannel::MissionCritical});
    }
    pushObjectiveForPhase(phase_);
    queueMissionPhaseLines(phase_);
}

void MissionController::onShopVisitedOnFoot() {
    if (phase_ != MissionPhase::WalkToShop) {
        return;
    }

    phase_ = MissionPhase::ReturnToBench;
    phaseSeconds_ = 0.0f;
    const int phaseKey = static_cast<int>(phase_);
    const bool hasPhaseDialogue =
        missionPhaseDialogues_.find(phaseKey) != missionPhaseDialogues_.end() ||
        missionPhaseNpcReactions_.find(phaseKey) != missionPhaseNpcReactions_.end() ||
        missionPhaseCutscenes_.find(phaseKey) != missionPhaseCutscenes_.end();

    if (!hasPhaseDialogue) {
        dialogue_.push({"Zenon", "Bogu\u015b? Ja go lubi\u0119. Najbardziej z daleka od zeszytu d\u0142ug\u00f3w.",
                        4.2f, DialogueChannel::MissionCritical});
    }
    pushObjectiveLine("Wr\u00f3\u0107 do Bogusia");
    queueMissionPhaseLines(phase_);
}

void MissionController::onReturnedToBogus() {
    if (phase_ != MissionPhase::ReturnToBench) {
        return;
    }

    phase_ = MissionPhase::ReachVehicle;
    phaseSeconds_ = 0.0f;
    {
        const int phaseKey = static_cast<int>(phase_);
        const bool hasPhaseDialogue =
            missionPhaseDialogues_.find(phaseKey) != missionPhaseDialogues_.end() ||
            missionPhaseNpcReactions_.find(phaseKey) != missionPhaseNpcReactions_.end() ||
            missionPhaseCutscenes_.find(phaseKey) != missionPhaseCutscenes_.end();

        if (!hasPhaseDialogue) {
            dialogue_.push(
                {"Bogu\u015b", "Sklep \u017cyje, czyli plan dzia\u0142a. Teraz id\u017a do Ry\u015bka po gruza.",
                 4.2f, DialogueChannel::MissionCritical});
        }
    }
    pushObjectiveLine("Wsi\u0105d\u017a do gruza");
    queueMissionPhaseLines(phase_);
}

void MissionController::onPlayerEnteredVehicle() {
    if (phase_ != MissionPhase::ReachVehicle) {
        return;
    }

    phase_ = MissionPhase::DriveToShop;
    phaseSeconds_ = 0.0f;
    const int phaseKey = static_cast<int>(phase_);
    const bool hasPhaseDialogue =
        missionPhaseDialogues_.find(phaseKey) != missionPhaseDialogues_.end() ||
        missionPhaseNpcReactions_.find(phaseKey) != missionPhaseNpcReactions_.end() ||
        missionPhaseCutscenes_.find(phaseKey) != missionPhaseCutscenes_.end();

    if (!hasPhaseDialogue) {
        dialogue_.push(
            {"Rysiek", "To nie z\u0142om. To Borex Kombi po przej\u015bciach. Podjed\u017a nim pod sklep.",
             4.4f, DialogueChannel::MissionCritical});
    }
    pushObjectiveLine("Podjed\u017a pod sklep");
    queueMissionPhaseLines(phase_);
}

void MissionController::onShopReachedByVehicle() {
    if (phase_ != MissionPhase::DriveToShop) {
        return;
    }

    phase_ = MissionPhase::ChaseActive;
    phaseSeconds_ = 0.0f;
    chaseWanted_ = true;
    const int phaseKey = static_cast<int>(phase_);
    const bool hasPhaseDialogue =
        missionPhaseDialogues_.find(phaseKey) != missionPhaseDialogues_.end() ||
        missionPhaseNpcReactions_.find(phaseKey) != missionPhaseNpcReactions_.end() ||
        missionPhaseCutscenes_.find(phaseKey) != missionPhaseCutscenes_.end();
    if (!hasPhaseDialogue) {
        dialogue_.push({"Zenon", "Gruz pod sklepem? Kamerka wszystko nagrala, kolego.", 4.2f, DialogueChannel::MissionCritical});
        dialogue_.push({"Halina", "Ja to widze z okna! Zaraz bedzie telefon!", 4.0f, DialogueChannel::MissionCritical});
    }
    pushObjectiveLine("Zgub przypa\u0142.");
    queueMissionPhaseLines(phase_);
}

void MissionController::onChaseEscaped() {
    if (phase_ != MissionPhase::ChaseActive) {
        return;
    }

    phase_ = MissionPhase::ReturnToLot;
    phaseSeconds_ = 0.0f;
    {
        const int phaseKey = static_cast<int>(phase_);
        const bool hasPhaseDialogue =
            missionPhaseDialogues_.find(phaseKey) != missionPhaseDialogues_.end() ||
            missionPhaseNpcReactions_.find(phaseKey) != missionPhaseNpcReactions_.end() ||
            missionPhaseCutscenes_.find(phaseKey) != missionPhaseCutscenes_.end();
        if (!hasPhaseDialogue) {
            dialogue_.push(
                {"Rysiek", "Cisza. Czyli jeszcze jedzie. Wr\u00f3\u0107 na parking, zanim si\u0119 rozmy\u015bli.",
                 4.2f, DialogueChannel::MissionCritical});
        }
    }
    pushObjectiveLine("Wr\u00f3\u0107 na parking");
    queueMissionPhaseLines(phase_);
}

void MissionController::onDropoffReached() {
    if (phase_ != MissionPhase::ReturnToLot) {
        return;
    }

    phase_ = MissionPhase::Completed;
    phaseSeconds_ = 0.0f;
    dialogue_.push({"Misja", "Odcinek zaliczony. Rewir juz cie kojarzy.", 3.0f, DialogueChannel::MissionCritical});
    pushObjectiveLine("Pogadaj z Bogusiem o Paragon Grozy.");
    queueMissionPhaseLines(phase_);
}

void MissionController::fail(std::string reason) {
    // Cannot fail before start or after completion — prevents ghost failures.
    if (phase_ == MissionPhase::Completed || phase_ == MissionPhase::WaitingForStart) {
        return;
    }

    phase_ = MissionPhase::Failed;
    phaseSeconds_ = 0.0f;
    chaseWanted_ = false;
    failureReason_ = std::move(reason);
    dialogue_.clear();
    dialogue_.push({"Misja", "Nie wyszlo: " + failureReason_, 3.0f, DialogueChannel::FailChase});
}

void MissionController::retry() {
    retryToCheckpoint(MissionPhase::ReachVehicle);
}

void MissionController::retryToCheckpoint(MissionPhase checkpointPhase) {
    if (phase_ != MissionPhase::Failed) {
        return;
    }

    phase_ = checkpointPhase;
    if (phase_ == MissionPhase::Failed || phase_ == MissionPhase::Completed || phase_ == MissionPhase::WaitingForStart) {
        phase_ = MissionPhase::ReachVehicle;
    }
    chaseWanted_ = false;
    failureReason_.clear();
    phaseSeconds_ = 0.0f;
    dialogue_.clear();
    pushObjectiveForPhase(phase_);
}

void MissionController::restoreForSave(MissionPhase phase, float phaseSeconds) {
    phase_ = phase;
    // Repair: if saved in Failed, restart from ReachVehicle checkpoint.
    if (phase_ == MissionPhase::Failed) {
        phase_ = MissionPhase::ReachVehicle;
    }
    phaseSeconds_ = std::max(0.0f, phaseSeconds);
    chaseWanted_ = phase_ == MissionPhase::ChaseActive;
    failureReason_.clear();
    dialogue_.clear();
    if (phase_ != MissionPhase::WaitingForStart && phase_ != MissionPhase::Completed) {
        pushObjectiveLine(objectiveText());
    }
}

void MissionController::setObjectiveOverride(MissionPhase phase, std::string objective) {
    if (objective.empty()) {
        objectiveOverrides_.erase(static_cast<int>(phase));
        return;
    }
    objectiveOverrides_[static_cast<int>(phase)] = std::move(objective);
}

void MissionController::setMissionDialogueLinesForPhase(MissionPhase phase, std::vector<DialogueLine> lines) {
    for (DialogueLine& line : lines) {
        line.channel = DialogueChannel::MissionCritical;
    }
    setLinesForPhase(missionPhaseDialogues_, phase, std::move(lines));
}

void MissionController::setNpcReactionLinesForPhase(MissionPhase phase, std::vector<DialogueLine> lines) {
    for (DialogueLine& line : lines) {
        line.channel = DialogueChannel::Reaction;
    }
    setLinesForPhase(missionPhaseNpcReactions_, phase, std::move(lines));
}

void MissionController::setCutsceneLinesForPhase(MissionPhase phase, std::vector<DialogueLine> lines) {
    for (DialogueLine& line : lines) {
        line.channel = DialogueChannel::SystemHint;
    }
    setLinesForPhase(missionPhaseCutscenes_, phase, std::move(lines));
}

MissionPhase MissionController::phase() const {
    return phase_;
}

float MissionController::phaseSeconds() const {
    return phaseSeconds_;
}

bool MissionController::chaseWanted() const {
    return chaseWanted_;
}

bool MissionController::consumeChaseWanted() {
    const bool wasWanted = chaseWanted_;
    chaseWanted_ = false;
    return wasWanted;
}

const std::string& MissionController::failureReason() const {
    return failureReason_;
}

std::string MissionController::objectiveText() const {
    const auto override = objectiveOverrides_.find(static_cast<int>(phase_));
    if (override != objectiveOverrides_.end()) {
        return override->second;
    }

    if (phase_ == MissionPhase::WaitingForStart) {
        const auto startOverride = objectiveOverrides_.find(static_cast<int>(MissionPhase::WalkToShop));
        if (startOverride != objectiveOverrides_.end()) {
            return startOverride->second;
        }
    }

    switch (phase_) {
    case MissionPhase::WaitingForStart:
        return "Spytaj Bogusia o zeszyt dlugow";
    case MissionPhase::WalkToShop:
        return "Sprawdz u Zenona, czy dlug nadal zyje";
    case MissionPhase::ReturnToBench:
        return "Wroc do Bogusia z wiesciami";
    case MissionPhase::ReachVehicle:
        return "Wsiadz do gruza";
    case MissionPhase::DriveToShop:
        return "Podjedz gruzem pod Zenona";
    case MissionPhase::ChaseActive:
        return "Zgub przypal";
    case MissionPhase::ReturnToLot:
        return "Wroc na parking";
    case MissionPhase::Completed:
        return "Misja zakonczona.";
    case MissionPhase::Failed:
        return "Misja nieudana. Nacisnij R, zeby powtorzyc.";
    }

    return {};
}

void MissionController::pushObjectiveLine(const std::string& text) {
    dialogue_.push({"Misja", text, 2.4f, DialogueChannel::SystemHint});
}

void MissionController::pushObjectiveForPhase(MissionPhase phase) {
    pushObjectiveLine(objectiveText());
    if (phase == MissionPhase::ChaseActive) {
        chaseWanted_ = true;
    }
}

void MissionController::setLinesForPhase(std::unordered_map<int, std::vector<DialogueLine>>& bucket,
                                        MissionPhase phase,
                                        std::vector<DialogueLine> lines) {
    if (lines.empty()) {
        bucket.erase(static_cast<int>(phase));
        return;
    }
    bucket[static_cast<int>(phase)] = std::move(lines);
}

void MissionController::queuePhaseLines(const std::unordered_map<int, std::vector<DialogueLine>>& linesByPhase,
                                      MissionPhase phase) {
    const auto it = linesByPhase.find(static_cast<int>(phase));
    if (it == linesByPhase.end()) {
        return;
    }

    for (const DialogueLine& line : it->second) {
        dialogue_.push(line);
    }
}

void MissionController::queueMissionPhaseLines(MissionPhase phase) {
    queuePhaseLines(missionPhaseDialogues_, phase);
    queuePhaseLines(missionPhaseNpcReactions_, phase);
    queuePhaseLines(missionPhaseCutscenes_, phase);
}

} // namespace bs3d
