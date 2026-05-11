#include "bs3d/core/ParagonMissionSpec.h"

namespace bs3d {

const ParagonActorSpec* ParagonMissionSpec::actor(const std::string& id) const {
    for (const ParagonActorSpec& actorSpec : actors) {
        if (actorSpec.id == id) {
            return &actorSpec;
        }
    }
    return nullptr;
}

const ParagonChoiceSpec* ParagonMissionSpec::choice(ParagonSolution solution) const {
    for (const ParagonChoiceSpec& choiceSpec : choices) {
        if (choiceSpec.solution == solution) {
            return &choiceSpec;
        }
    }
    return nullptr;
}

const ParagonMissionSpec& defaultParagonMissionSpec() {
    static const ParagonMissionSpec spec{
        {
            {"bogus", "Boguś", "E: Paragon Grozy"},
            {"zenon", "Zenon", "E: pogadaj z Zenonem"},
            {"lolek", "Lolek", "E: spytaj Lolka"},
            {"receipt_holder", "Typ z paragonem", "E/F/LMB: paragon"},
            {"halina", "Halina", "Reakcja z okna"},
        },
        {
            {ParagonSolution::Peaceful,
             "E: spokojnie wypros paragon",
             "Gadaj bez przypału",
             WorldEventType::PublicNuisance,
             0.18f,
             "paragon_peaceful",
             false},
            {ParagonSolution::Trick,
             "F: zakombinuj paragon",
             "Kombinuj, ale bez bojki",
             WorldEventType::ShopTrouble,
             0.48f,
             "paragon_trick",
             false},
            {ParagonSolution::Chaos,
             "LMB/X: zrob chaos o paragon",
             "Szturchnij albo popchnij typa",
             WorldEventType::ShopTrouble,
             0.88f,
             "paragon_chaos",
             true},
        }};
    return spec;
}

} // namespace bs3d
