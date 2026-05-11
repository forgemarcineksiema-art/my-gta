#include "bs3d/core/ParagonMission.h"

#include "bs3d/core/ParagonMissionSpec.h"

namespace bs3d {

namespace {

ParagonMissionPulse pulse(WorldEventType type, float intensity, std::string source) {
    ParagonMissionPulse result;
    result.available = true;
    result.type = type;
    result.location = WorldLocationTag::Shop;
    result.intensity = intensity;
    result.source = std::move(source);
    return result;
}

} // namespace

ParagonMission::ParagonMission(DialogueSystem& dialogue) : dialogue_(dialogue) {}

bool ParagonMission::start(bool introCompleted) {
    if (!introCompleted || phase_ != ParagonMissionPhase::Locked) {
        return false;
    }

    phase_ = ParagonMissionPhase::TalkToZenon;
    solution_ = ParagonSolution::None;
    shopBanActive_ = false;
    dialogue_.push({"Misja", "Paragon Grozy", 2.2f, DialogueChannel::SystemHint});
    dialogue_.push({"Boguś", "Zenon coś sapie o stary dług. Idź i zobacz, czy ma dowody.", 4.2f, DialogueChannel::MissionCritical});
    pushObjectiveLine("Idź do Zenona");
    return true;
}

ParagonMissionPulse ParagonMission::onZenonAccusesBogus() {
    if (phase_ != ParagonMissionPhase::TalkToZenon) {
        return {};
    }

    phase_ = ParagonMissionPhase::FindReceiptHolder;
    dialogue_.push({"Zenon", "Boguś ma dług. Paragon był, zeszyt był, honoru nie było.", 4.2f, DialogueChannel::MissionCritical});
    dialogue_.push({"Lolek", "Paragon widzialem u typa, co czyta gazete odwrotnie.", 4.0f, DialogueChannel::MissionCritical});
    pushObjectiveLine("Znajdź typa z paragonem");
    return pulse(WorldEventType::ShopTrouble, 0.28f, "paragon_accusation");
}

void ParagonMission::onReceiptHolderFound() {
    if (phase_ != ParagonMissionPhase::FindReceiptHolder) {
        return;
    }

    phase_ = ParagonMissionPhase::ResolveReceipt;
    dialogue_.push({"Misja", "Odzyskaj paragon: gadaj, kombinuj albo zrob chaos.", 3.2f, DialogueChannel::SystemHint});
    pushObjectiveLine("Odzyskaj paragon");
}

ParagonMissionPulse ParagonMission::resolve(ParagonSolution solution) {
    if (phase_ != ParagonMissionPhase::ResolveReceipt || solution == ParagonSolution::None) {
        return {};
    }

    solution_ = solution;
    phase_ = ParagonMissionPhase::ReturnToZenon;
    pushObjectiveLine("Wróć do Zenona");

    switch (solution) {
    case ParagonSolution::Peaceful:
        dialogue_.push({"Misja", "Paragon odzyskany bez wojny. Osiedle jest rozczarowane spokojem.", 3.4f, DialogueChannel::MissionCritical});
        return pulse(defaultParagonMissionSpec().choice(solution)->worldEventType,
                     defaultParagonMissionSpec().choice(solution)->intensity,
                     defaultParagonMissionSpec().choice(solution)->source);
    case ParagonSolution::Trick:
        dialogue_.push({"Lolek", "To nie klamstwo. To kreatywna ksiegowosc chodnikowa.", 3.7f, DialogueChannel::MissionCritical});
        return pulse(defaultParagonMissionSpec().choice(solution)->worldEventType,
                     defaultParagonMissionSpec().choice(solution)->intensity,
                     defaultParagonMissionSpec().choice(solution)->source);
    case ParagonSolution::Chaos:
        shopBanActive_ = true;
        dialogue_.push({"Halina", "O paragon sie bija! Fiskalizacja upadla na moich oczach!", 4.0f, DialogueChannel::MissionCritical});
        return pulse(defaultParagonMissionSpec().choice(solution)->worldEventType,
                     defaultParagonMissionSpec().choice(solution)->intensity,
                     defaultParagonMissionSpec().choice(solution)->source);
    case ParagonSolution::None:
        break;
    }

    return {};
}

void ParagonMission::onReturnedToZenon() {
    if (phase_ != ParagonMissionPhase::ReturnToZenon) {
        return;
    }

    phase_ = ParagonMissionPhase::Completed;
    switch (solution_) {
    case ParagonSolution::Peaceful:
        dialogue_.push({"Zenon", "Czyli mozna normalnie. Az mi kasa fiskalna przycichla z wrazenia.", 4.0f, DialogueChannel::MissionCritical});
        break;
    case ParagonSolution::Trick:
        dialogue_.push({"Zenon", "Paragon jest, ale krzywo pachnie. Jak promocja na mielonke.", 4.0f, DialogueChannel::MissionCritical});
        break;
    case ParagonSolution::Chaos:
        dialogue_.push({"Zenon", "Masz zakaz. Od dzis kupujesz przez okienko, ty fiskalny bandyto.", 4.2f, DialogueChannel::MissionCritical});
        break;
    case ParagonSolution::None:
        break;
    }
    pushObjectiveLine("Paragon Grozy zaliczony");
}

void ParagonMission::restoreForSave(ParagonMissionPhase phase,
                                    ParagonSolution solution,
                                    bool shopBanActive) {
    phase_ = phase;
    solution_ = solution;
    shopBanActive_ = shopBanActive || solution_ == ParagonSolution::Chaos;
    dialogue_.clear();
    if (phase_ != ParagonMissionPhase::Locked && phase_ != ParagonMissionPhase::Completed) {
        pushObjectiveLine(objectiveText());
    }
}

ParagonMissionPhase ParagonMission::phase() const {
    return phase_;
}

ParagonSolution ParagonMission::solution() const {
    return solution_;
}

bool ParagonMission::shopBanActive() const {
    return shopBanActive_;
}

std::string ParagonMission::objectiveText() const {
    switch (phase_) {
    case ParagonMissionPhase::Locked:
        return "Ukoncz pierwszy odcinek";
    case ParagonMissionPhase::TalkToZenon:
        return "Idź do sklepu Zenona";
    case ParagonMissionPhase::FindReceiptHolder:
        return "Zapytaj Lolka o paragon";
    case ParagonMissionPhase::ResolveReceipt:
        return "Odzyskaj paragon: E spokojnie, F kombinuj, LMB/X chaos";
    case ParagonMissionPhase::ReturnToZenon:
        if (shopBanActive_) {
            return "Wróć do Zenona przez okienko";
        }
        return "Wróć do Zenona";
    case ParagonMissionPhase::Completed:
        return "Paragon Grozy zaliczony";
    case ParagonMissionPhase::Failed:
        return "Paragon Grozy nieudany";
    }
    return {};
}

std::string ParagonMission::zenonShopLineAfterCompletion() const {
    if (phase_ != ParagonMissionPhase::Completed) {
        return {};
    }

    switch (solution_) {
    case ParagonSolution::Peaceful:
        return "Po tamtym paragonie masz u mnie spokoj. Ale nie za duzo.";
    case ParagonSolution::Trick:
        return "Patrze ci na rece. I na kieszenie. I na paragon.";
    case ParagonSolution::Chaos:
        return "Zakaz sklepu. okienko albo won spod lady.";
    case ParagonSolution::None:
        break;
    }
    return {};
}

void ParagonMission::pushObjectiveLine(const std::string& text) {
    dialogue_.push({"Misja", text, 2.4f, DialogueChannel::SystemHint});
}

} // namespace bs3d
