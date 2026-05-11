#include "bs3d/core/NpcReactionSystem.h"
#include "bs3d/core/WorldRewirPressure.h"

#include <algorithm>

namespace bs3d {

namespace {

constexpr float GlobalCooldownSeconds = 3.0f;
constexpr float SourceCooldownSeconds = 8.0f;
constexpr float AgedRewirHotspotReactionMinScore = 1.0f;
constexpr int RewirPressurePatrolPriority = 42;

bool isAlarmEvent(const WorldEvent& event) {
    return event.type == WorldEventType::PublicNuisance || event.type == WorldEventType::PropertyDamage;
}

ReactionSource rewirWitnessSourceFor(WorldLocationTag location) {
    switch (location) {
    case WorldLocationTag::Garage:
        return ReactionSource::GarageCrew;
    case WorldLocationTag::Parking:
        return ReactionSource::ParkingWitness;
    case WorldLocationTag::Trash:
        return ReactionSource::TrashWitness;
    case WorldLocationTag::RoadLoop:
        return ReactionSource::RoadLoopWitness;
    case WorldLocationTag::Unknown:
    case WorldLocationTag::Block:
    case WorldLocationTag::Shop:
        break;
    }

    return ReactionSource::None;
}

int freshRewirWitnessPriority(ReactionSource source) {
    switch (source) {
    case ReactionSource::GarageCrew:
        return 84;
    case ReactionSource::ParkingWitness:
        return 83;
    case ReactionSource::TrashWitness:
        return 82;
    case ReactionSource::RoadLoopWitness:
        return 81;
    case ReactionSource::None:
    case ReactionSource::BabkaWindow:
    case ReactionSource::Shopkeeper:
    case ReactionSource::ZulGossip:
    case ReactionSource::PatrolHint:
        break;
    }

    return 0;
}

} // namespace

ReactionIntent NpcReactionSystem::update(const WorldEventLedger& ledger,
                                         const PrzypalSystem& przypal,
                                         Vec3 playerPosition,
                                         bool missionUiBusy,
                                         float deltaSeconds) {
    tickCooldowns(deltaSeconds);

    if (globalCooldownSeconds_ > 0.0f) {
        return {};
    }

    Candidate best;
    const auto betterCandidate = [](const Candidate& candidate, const Candidate& currentBest) {
        if (candidate.priority != currentBest.priority) {
            return candidate.priority > currentBest.priority;
        }
        return candidate.score > currentBest.score;
    };
    const auto consider = [&](Candidate candidate) {
        if (candidate.source == ReactionSource::None || !sourceReady(candidate.source)) {
            return;
        }
        if (!sourceCanReactToEvent(candidate.source, candidate.eventId)) {
            return;
        }
        if (missionUiBusy) {
            if (candidate.priority < 80) {
                return;
            }
            candidate.hudOnly = true;
        }
        if (betterCandidate(candidate, best)) {
            best = candidate;
        }
    };

    if (przypal.band() == PrzypalBand::ChaseRisk || przypal.band() == PrzypalBand::Meltdown) {
        consider({ReactionSource::PatrolHint, 88, przypal.value(), false, 0});
    }

    const RewirPressureSnapshot rewirPressure = resolveRewirPressure(ledger, playerPosition);
    if (rewirPressure.patrolInterest) {
        const float distanceScore = std::max(0.0f, 20.0f - rewirPressure.distanceToPlayer);
        consider({ReactionSource::PatrolHint,
                  RewirPressurePatrolPriority,
                  rewirPressure.score * 10.0f + distanceScore,
                  false,
                  rewirPressure.eventId});
    }

    for (const WorldEventHotspot& hotspot : ledger.memoryHotspots()) {
        if (hotspot.ageSeconds < 3.0f || hotspot.score < AgedRewirHotspotReactionMinScore) {
            continue;
        }

        const ReactionSource source = rewirWitnessSourceFor(hotspot.location);
        if (source == ReactionSource::None) {
            continue;
        }

        const float distanceScore = std::max(0.0f, 20.0f - distanceXZ(playerPosition, hotspot.position));
        consider({source, 45, hotspot.score * 10.0f + distanceScore, false, hotspot.eventId});
    }

    for (const WorldEvent& event : ledger.recentEvents()) {
        const float distanceScore = std::max(0.0f, 20.0f - distanceXZ(playerPosition, event.position));
        const float score = event.intensity * 10.0f + static_cast<float>(event.stackCount) + distanceScore;

        if (event.type == WorldEventType::ChaseSeen) {
            consider({ReactionSource::PatrolHint, 95, score, false, event.id});
            continue;
        }

        if (event.type == WorldEventType::ShopTrouble ||
            (event.location == WorldLocationTag::Shop && event.type == WorldEventType::PropertyDamage)) {
            consider({ReactionSource::Shopkeeper, 90, score, false, event.id});
            continue;
        }

        const ReactionSource rewirWitness = rewirWitnessSourceFor(event.location);
        if (isAlarmEvent(event) && rewirWitness != ReactionSource::None && event.ageSeconds < 4.0f) {
            consider({rewirWitness, freshRewirWitnessPriority(rewirWitness), score, false, event.id});
            continue;
        }

        if (isAlarmEvent(event) && event.ageSeconds < 4.0f) {
            consider({ReactionSource::BabkaWindow, 80, score, false, event.id});
            continue;
        }

        if (event.ageSeconds >= 3.0f) {
            consider({ReactionSource::ZulGossip, 35, score, false, event.id});
        }
    }

    if (best.source == ReactionSource::None) {
        return {};
    }

    ReactionIntent intent = makeIntent(best.source, best.priority, best.hudOnly);
    startCooldowns(best.source, best.eventId);
    return intent;
}

void NpcReactionSystem::clear() {
    globalCooldownSeconds_ = 0.0f;
    sourceCooldowns_.fill(0.0f);
    lastVariantBySource_.fill(-1);
    lastReactedEventIdBySource_.fill(0);
}

void NpcReactionSystem::tickCooldowns(float deltaSeconds) {
    const float dt = std::max(0.0f, deltaSeconds);
    globalCooldownSeconds_ = std::max(0.0f, globalCooldownSeconds_ - dt);
    for (float& cooldown : sourceCooldowns_) {
        cooldown = std::max(0.0f, cooldown - dt);
    }
}

bool NpcReactionSystem::sourceReady(ReactionSource source) const {
    const int index = sourceIndex(source);
    return index > 0 && sourceCooldowns_[static_cast<std::size_t>(index)] <= 0.0f;
}

bool NpcReactionSystem::sourceCanReactToEvent(ReactionSource source, int eventId) const {
    if (eventId <= 0) {
        return true;
    }

    const int index = sourceIndex(source);
    return index > 0 && lastReactedEventIdBySource_[static_cast<std::size_t>(index)] != eventId;
}

void NpcReactionSystem::startCooldowns(ReactionSource source, int eventId) {
    const int index = sourceIndex(source);
    if (index > 0) {
        sourceCooldowns_[static_cast<std::size_t>(index)] = SourceCooldownSeconds;
        if (eventId > 0) {
            lastReactedEventIdBySource_[static_cast<std::size_t>(index)] = eventId;
        }
    }
    globalCooldownSeconds_ = GlobalCooldownSeconds;
}

ReactionIntent NpcReactionSystem::makeIntent(ReactionSource source, int priority, bool hudOnly) {
    const std::vector<std::string>& lines = linesFor(source);
    const int index = sourceIndex(source);
    int nextVariant = 0;
    if (!lines.empty() && index >= 0) {
        nextVariant = (lastVariantBySource_[static_cast<std::size_t>(index)] + 1) %
                      static_cast<int>(lines.size());
        lastVariantBySource_[static_cast<std::size_t>(index)] = nextVariant;
    }

    ReactionIntent intent;
    intent.available = true;
    intent.source = source;
    intent.speaker = speakerFor(source);
    intent.text = lines.empty() ? "" : lines[static_cast<std::size_t>(nextVariant)];
    intent.priority = priority;
    intent.heatDelta = 0.0f;
    intent.hudOnly = hudOnly;
    intent.feedbackEvent = FeedbackEvent::PrzypalNotice;
    return intent;
}

int NpcReactionSystem::sourceIndex(ReactionSource source) {
    switch (source) {
    case ReactionSource::None:
        return 0;
    case ReactionSource::BabkaWindow:
        return 1;
    case ReactionSource::Shopkeeper:
        return 2;
    case ReactionSource::ZulGossip:
        return 3;
    case ReactionSource::PatrolHint:
        return 4;
    case ReactionSource::GarageCrew:
        return 5;
    case ReactionSource::TrashWitness:
        return 6;
    case ReactionSource::ParkingWitness:
        return 7;
    case ReactionSource::RoadLoopWitness:
        return 8;
    }

    return 0;
}

const char* NpcReactionSystem::speakerFor(ReactionSource source) {
    switch (source) {
    case ReactionSource::BabkaWindow:
        return "Babka z okna";
    case ReactionSource::Shopkeeper:
        return "Sklepikarz";
    case ReactionSource::ZulGossip:
        return "Zul";
    case ReactionSource::PatrolHint:
        return "Osiedle";
    case ReactionSource::GarageCrew:
        return "Garazowy";
    case ReactionSource::TrashWitness:
        return "Dozorca";
    case ReactionSource::ParkingWitness:
        return "Parkingowy";
    case ReactionSource::RoadLoopWitness:
        return "Kierowca";
    case ReactionSource::None:
        break;
    }

    return "";
}

const std::vector<std::string>& NpcReactionSystem::linesFor(ReactionSource source) {
    static const std::vector<std::string> empty;
    static const std::vector<std::string> babka{
        "Ej! Ja wszystko widze z okna!",
        "Zaraz zadzwonie gdzie trzeba!",
        "Znowu ten sam, ja go znam!"
    };
    static const std::vector<std::string> shopkeeper{
        "Pod sklepem nie rob cyrku!",
        "Jeszcze raz i masz zakaz wejscia!",
        "Kamerka wszystko nagrala, kolego."
    };
    static const std::vector<std::string> zul{
        "Slyszalem, ze niezly przypal zrobiles.",
        "Cale osiedle juz gada.",
        "Ty to masz talent do problemow."
    };
    static const std::vector<std::string> patrol{
        "Patrol kreci sie blizej osiedla.",
        "Ktos chyba zglosil zadyme."
    };
    static const std::vector<std::string> garage{
        "Ej, przy garazach delikatniej!",
        "To nie tor testowy, tylko garaze.",
        "Lakier znowu bedzie placil kto?"
    };
    static const std::vector<std::string> trash{
        "Przy smietniku tez ludzie patrza.",
        "Nie rozsypuj mi rewiru pod altana.",
        "Smietnik pamieta, kolego."
    };
    static const std::vector<std::string> parking{
        "Na parkingu sie patrzy w lusterka!",
        "To miejsce bylo zajete zanim wjechales.",
        "Jeszcze rysa i bedzie rozmowa."
    };
    static const std::vector<std::string> roadLoop{
        "Na petli trzymaj pas, mistrzu.",
        "Tu sie jezdzi, nie robi zawijasy.",
        "Asfalt slyszal ten numer juz rano."
    };

    switch (source) {
    case ReactionSource::BabkaWindow:
        return babka;
    case ReactionSource::Shopkeeper:
        return shopkeeper;
    case ReactionSource::ZulGossip:
        return zul;
    case ReactionSource::PatrolHint:
        return patrol;
    case ReactionSource::GarageCrew:
        return garage;
    case ReactionSource::TrashWitness:
        return trash;
    case ReactionSource::ParkingWitness:
        return parking;
    case ReactionSource::RoadLoopWitness:
        return roadLoop;
    case ReactionSource::None:
        break;
    }

    return empty;
}

} // namespace bs3d
