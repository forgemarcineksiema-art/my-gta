#pragma once

#include "bs3d/core/GameFeedback.h"
#include "bs3d/core/PrzypalSystem.h"

#include <array>
#include <string>
#include <vector>

namespace bs3d {

enum class ReactionSource {
    None,
    BabkaWindow,
    Shopkeeper,
    ZulGossip,
    PatrolHint,
    GarageCrew,
    TrashWitness,
    ParkingWitness,
    RoadLoopWitness
};

struct ReactionIntent {
    bool available = false;
    ReactionSource source = ReactionSource::None;
    std::string speaker;
    std::string text;
    int priority = 0;
    float heatDelta = 0.0f;
    bool hudOnly = false;
    FeedbackEvent feedbackEvent = FeedbackEvent::PrzypalNotice;
};

class NpcReactionSystem {
public:
    ReactionIntent update(const WorldEventLedger& ledger,
                          const PrzypalSystem& przypal,
                          Vec3 playerPosition,
                          bool missionUiBusy,
                          float deltaSeconds);
    void clear();

private:
    struct Candidate {
        ReactionSource source = ReactionSource::None;
        int priority = 0;
        float score = 0.0f;
        bool hudOnly = false;
        int eventId = 0;
    };

    static constexpr int SourceSlotCount = 9;

    float globalCooldownSeconds_ = 0.0f;
    std::array<float, SourceSlotCount> sourceCooldowns_{};
    std::array<int, SourceSlotCount> lastVariantBySource_{{-1, -1, -1, -1, -1, -1, -1, -1, -1}};
    std::array<int, SourceSlotCount> lastReactedEventIdBySource_{{0, 0, 0, 0, 0, 0, 0, 0, 0}};

    void tickCooldowns(float deltaSeconds);
    bool sourceReady(ReactionSource source) const;
    bool sourceCanReactToEvent(ReactionSource source, int eventId) const;
    void startCooldowns(ReactionSource source, int eventId);
    ReactionIntent makeIntent(ReactionSource source, int priority, bool hudOnly);

    static int sourceIndex(ReactionSource source);
    static const char* speakerFor(ReactionSource source);
    static const std::vector<std::string>& linesFor(ReactionSource source);
};

} // namespace bs3d
