#include "bs3d/core/PrzypalSystem.h"

#include <algorithm>

namespace bs3d {

namespace {

constexpr float MaxPrzypal = 100.0f;
constexpr float DecayDelaySeconds = 1.5f;
constexpr float DecayPerSecond = 3.0f;
constexpr float ContributorLifetime = 12.0f;

} // namespace

void PrzypalSystem::onWorldEvent(const WorldEventAddResult& result) {
    if (!result.heatPulseAccepted) {
        return;
    }

    const float heat = baseHeat(result.type) * std::max(0.0f, result.intensity) *
                       std::max(0.0f, result.heatMultiplier);
    if (heat <= 0.0f) {
        return;
    }

    value_ = std::clamp(value_ + heat, 0.0f, MaxPrzypal);
    decayDelayRemaining_ = DecayDelaySeconds;
    contributors_.push_back({result.type, result.location, heat, 0.0f});
    refreshBand();
}

void PrzypalSystem::update(float deltaSeconds) {
    float dt = std::max(0.0f, deltaSeconds);
    for (PrzypalContributor& contributor : contributors_) {
        contributor.ageSeconds += dt;
    }
    contributors_.erase(std::remove_if(contributors_.begin(),
                                       contributors_.end(),
                                       [](const PrzypalContributor& contributor) {
                                           return contributor.ageSeconds > ContributorLifetime;
                                       }),
                        contributors_.end());

    if (dt <= 0.0f || value_ <= 0.0f) {
        return;
    }

    if (decayDelayRemaining_ > 0.0f) {
        const float consumed = std::min(decayDelayRemaining_, dt);
        decayDelayRemaining_ -= consumed;
        dt -= consumed;
    }

    if (dt > 0.0f) {
        value_ = std::max(0.0f, value_ - dt * DecayPerSecond);
        refreshBand();
    }
}

void PrzypalSystem::clear() {
    value_ = 0.0f;
    decayDelayRemaining_ = 0.0f;
    band_ = PrzypalBand::Calm;
    bandPulseAvailable_ = false;
    contributors_.clear();
}

void PrzypalSystem::setValue(float value) {
    value_ = std::clamp(value, 0.0f, MaxPrzypal);
    decayDelayRemaining_ = 0.0f;
    contributors_.clear();
    refreshBand();
}

PrzypalSnapshot PrzypalSystem::snapshot() const {
    return {value_, decayDelayRemaining_, band_, bandPulseAvailable_, contributors_};
}

void PrzypalSystem::restore(const PrzypalSnapshot& snapshot) {
    value_ = std::clamp(snapshot.value, 0.0f, MaxPrzypal);
    decayDelayRemaining_ = std::max(0.0f, snapshot.decayDelayRemaining);
    band_ = snapshot.band;
    bandPulseAvailable_ = snapshot.bandPulseAvailable;
    contributors_ = snapshot.contributors;
}

float PrzypalSystem::value() const {
    return value_;
}

float PrzypalSystem::normalizedIntensity() const {
    return std::clamp(value_ / MaxPrzypal, 0.0f, 1.0f);
}

PrzypalBand PrzypalSystem::band() const {
    return band_;
}

bool PrzypalSystem::consumeBandPulse() {
    const bool result = bandPulseAvailable_;
    bandPulseAvailable_ = false;
    return result;
}

const std::vector<PrzypalContributor>& PrzypalSystem::contributors() const {
    return contributors_;
}

PrzypalBand PrzypalSystem::bandForValue(float value) {
    if (value >= 90.0f) {
        return PrzypalBand::Meltdown;
    }
    if (value >= 70.0f) {
        return PrzypalBand::ChaseRisk;
    }
    if (value >= 45.0f) {
        return PrzypalBand::Hot;
    }
    if (value >= 20.0f) {
        return PrzypalBand::Noticed;
    }
    return PrzypalBand::Calm;
}

const char* PrzypalSystem::hudLabel(PrzypalBand band) {
    switch (band) {
    case PrzypalBand::Calm:
        return "Spokój";
    case PrzypalBand::Noticed:
        return "Ktos patrzy";
    case PrzypalBand::Hot:
        return "Goraco";
    case PrzypalBand::ChaseRisk:
        return "Zaraz psy";
    case PrzypalBand::Meltdown:
        return "Totalny przypal";
    }

    return "Nie wiadomo";
}

float PrzypalSystem::baseHeat(WorldEventType type) {
    switch (type) {
    case WorldEventType::PublicNuisance:
        return 8.0f;
    case WorldEventType::PropertyDamage:
        return 16.0f;
    case WorldEventType::ChaseSeen:
        return 12.0f;
    case WorldEventType::ShopTrouble:
        return 18.0f;
    }

    return 0.0f;
}

void PrzypalSystem::refreshBand() {
    const PrzypalBand next = bandForValue(value_);
    if (next != band_) {
        const bool rising = static_cast<int>(next) > static_cast<int>(band_);
        band_ = next;
        if (rising) {
            bandPulseAvailable_ = true;
        }
    }
}

WorldReactionSnapshot captureWorldReactionSnapshot(const PrzypalSystem& przypal,
                                                   const WorldEventLedger& ledger,
                                                   const WorldEventEmitterCooldowns& cooldowns) {
    return {przypal.snapshot(), ledger.recentEvents(), cooldowns.snapshot()};
}

void restoreWorldReactionSnapshot(const WorldReactionSnapshot& snapshot,
                                  PrzypalSystem& przypal,
                                  WorldEventLedger& ledger,
                                  WorldEventEmitterCooldowns& cooldowns) {
    przypal.restore(snapshot.przypal);
    ledger.restoreRecentEvents(snapshot.events);
    cooldowns.restore(snapshot.cooldowns);
}

} // namespace bs3d
