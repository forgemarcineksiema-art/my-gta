#pragma once

#include "bs3d/core/DialogueSystem.h"
#include "bs3d/core/WorldEventLedger.h"

#include <string>

namespace bs3d {

enum class ParagonMissionPhase {
    Locked,
    TalkToZenon,
    FindReceiptHolder,
    ResolveReceipt,
    ReturnToZenon,
    Completed,
    Failed
};

enum class ParagonSolution {
    None,
    Peaceful,
    Trick,
    Chaos
};

struct ParagonMissionPulse {
    bool available = false;
    WorldEventType type = WorldEventType::ShopTrouble;
    WorldLocationTag location = WorldLocationTag::Shop;
    float intensity = 0.0f;
    std::string source;
};

class ParagonMission {
public:
    explicit ParagonMission(DialogueSystem& dialogue);

    bool start(bool introCompleted);
    ParagonMissionPulse onZenonAccusesBogus();
    void onReceiptHolderFound();
    ParagonMissionPulse resolve(ParagonSolution solution);
    void onReturnedToZenon();
    void restoreForSave(ParagonMissionPhase phase, ParagonSolution solution, bool shopBanActive);

    ParagonMissionPhase phase() const;
    ParagonSolution solution() const;
    bool shopBanActive() const;
    std::string objectiveText() const;
    std::string zenonShopLineAfterCompletion() const;

private:
    DialogueSystem& dialogue_;
    ParagonMissionPhase phase_ = ParagonMissionPhase::Locked;
    ParagonSolution solution_ = ParagonSolution::None;
    bool shopBanActive_ = false;

    void pushObjectiveLine(const std::string& text);
};

} // namespace bs3d
