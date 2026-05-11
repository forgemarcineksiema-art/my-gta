#pragma once

#include "bs3d/core/ParagonMission.h"
#include "bs3d/core/WorldEventLedger.h"

#include <string>
#include <vector>

namespace bs3d {

struct ParagonActorSpec {
    std::string id;
    std::string speaker;
    std::string prompt;
};

struct ParagonChoiceSpec {
    ParagonSolution solution = ParagonSolution::None;
    std::string prompt;
    std::string actionHint;
    WorldEventType worldEventType = WorldEventType::ShopTrouble;
    float intensity = 0.0f;
    std::string source;
    bool shopBanConsequence = false;
};

struct ParagonMissionSpec {
    std::vector<ParagonActorSpec> actors;
    std::vector<ParagonChoiceSpec> choices;

    const ParagonActorSpec* actor(const std::string& id) const;
    const ParagonChoiceSpec* choice(ParagonSolution solution) const;
};

const ParagonMissionSpec& defaultParagonMissionSpec();

} // namespace bs3d
