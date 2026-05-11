#pragma once

#include "bs3d/core/ParagonMission.h"
#include "bs3d/core/WorldEventLedger.h"

namespace bs3d {

struct StoryState {
    bool visitedShopOnFoot = false;
    bool gruzUnlocked = false;
    bool introCompleted = false;
    bool shopTroubleSeen = false;
    bool paragonCompleted = false;
    bool shopBanActive = false;
    ParagonSolution paragonSolution = ParagonSolution::None;

    void clear() {
        visitedShopOnFoot = false;
        gruzUnlocked = false;
        introCompleted = false;
        shopTroubleSeen = false;
        paragonCompleted = false;
        shopBanActive = false;
        paragonSolution = ParagonSolution::None;
    }

    void onWorldEvent(const WorldEventAddResult&) {
        // Story flags advance from explicit mission beats, not passive world memory.
    }

    void onShopVisitedOnFoot() {
        visitedShopOnFoot = true;
        shopTroubleSeen = true;
    }

    void onReturnedToBogus() {
        gruzUnlocked = true;
    }

    void onIntroCompleted() {
        introCompleted = true;
    }

    void onParagonResolved(ParagonSolution solution) {
        paragonCompleted = true;
        paragonSolution = solution;
        if (solution == ParagonSolution::Chaos) {
            shopBanActive = true;
        }
    }
};

} // namespace bs3d
