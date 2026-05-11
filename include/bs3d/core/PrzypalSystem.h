#pragma once

#include "bs3d/core/WorldEventLedger.h"

#include <vector>

namespace bs3d {

enum class PrzypalBand {
    Calm,
    Noticed,
    Hot,
    ChaseRisk,
    Meltdown
};

struct PrzypalContributor {
    WorldEventType type = WorldEventType::PublicNuisance;
    WorldLocationTag location = WorldLocationTag::Unknown;
    float heat = 0.0f;
    float ageSeconds = 0.0f;
};

struct PrzypalSnapshot {
    float value = 0.0f;
    float decayDelayRemaining = 0.0f;
    PrzypalBand band = PrzypalBand::Calm;
    bool bandPulseAvailable = false;
    std::vector<PrzypalContributor> contributors;
};

struct WorldReactionSnapshot {
    PrzypalSnapshot przypal;
    std::vector<WorldEvent> events;
    std::vector<WorldEventEmitterCooldownSnapshot> cooldowns;
};

class PrzypalSystem {
public:
    void onWorldEvent(const WorldEventAddResult& result);
    void update(float deltaSeconds);
    void clear();
    void setValue(float value);
    PrzypalSnapshot snapshot() const;
    void restore(const PrzypalSnapshot& snapshot);

    float value() const;
    float normalizedIntensity() const;
    PrzypalBand band() const;
    bool consumeBandPulse();
    const std::vector<PrzypalContributor>& contributors() const;

    static PrzypalBand bandForValue(float value);
    static const char* hudLabel(PrzypalBand band);
    static float baseHeat(WorldEventType type);

private:
    float value_ = 0.0f;
    float decayDelayRemaining_ = 0.0f;
    PrzypalBand band_ = PrzypalBand::Calm;
    bool bandPulseAvailable_ = false;
    std::vector<PrzypalContributor> contributors_;

    void refreshBand();
};

WorldReactionSnapshot captureWorldReactionSnapshot(const PrzypalSystem& przypal,
                                                   const WorldEventLedger& ledger,
                                                   const WorldEventEmitterCooldowns& cooldowns);
void restoreWorldReactionSnapshot(const WorldReactionSnapshot& snapshot,
                                  PrzypalSystem& przypal,
                                  WorldEventLedger& ledger,
                                  WorldEventEmitterCooldowns& cooldowns);

} // namespace bs3d
