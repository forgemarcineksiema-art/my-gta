#include "bs3d/core/WorldRewirPressure.h"

#include <algorithm>

namespace bs3d {

namespace {

constexpr float MinPressureScore = 1.0f;
constexpr float WatchfulScore = 1.0f;
constexpr float AlertScore = 2.5f;
constexpr float BaseInfluenceRadius = 10.0f;
constexpr float ScoreRadiusScale = 2.0f;
constexpr float MaxInfluenceRadius = 18.0f;

bool locationCanCreateRewirPressure(WorldLocationTag location) {
    const std::vector<WorldLocationTag>& locations = rewirPressureLocations();
    return std::find(locations.begin(), locations.end(), location) != locations.end();
}

float influenceRadiusForScore(float score) {
    return std::min(MaxInfluenceRadius, BaseInfluenceRadius + std::max(0.0f, score) * ScoreRadiusScale);
}

RewirPressureLevel levelForScore(float score) {
    if (score >= AlertScore) {
        return RewirPressureLevel::Alert;
    }
    if (score >= WatchfulScore) {
        return RewirPressureLevel::Watchful;
    }
    return RewirPressureLevel::Quiet;
}

} // namespace

const std::vector<WorldLocationTag>& rewirPressureLocations() {
    static const std::vector<WorldLocationTag> locations{
        WorldLocationTag::Parking,
        WorldLocationTag::Garage,
        WorldLocationTag::Trash,
        WorldLocationTag::RoadLoop
    };
    return locations;
}

RewirPressureSnapshot resolveRewirPressure(const WorldEventLedger& ledger, Vec3 playerPosition) {
    RewirPressureSnapshot best;
    float bestScore = -1.0f;

    for (const WorldEventHotspot& hotspot : ledger.memoryHotspots()) {
        if (!locationCanCreateRewirPressure(hotspot.location) || hotspot.score < MinPressureScore) {
            continue;
        }

        const float distance = distanceXZ(playerPosition, hotspot.position);
        if (distance > influenceRadiusForScore(hotspot.score)) {
            continue;
        }

        const float proximity = std::max(0.0f, influenceRadiusForScore(hotspot.score) - distance);
        const float pressureScore = hotspot.score * 10.0f + proximity;
        if (pressureScore <= bestScore) {
            continue;
        }

        bestScore = pressureScore;
        best.eventId = hotspot.eventId;
        best.active = true;
        best.patrolInterest = true;
        best.level = levelForScore(hotspot.score);
        best.location = hotspot.location;
        best.position = hotspot.position;
        best.score = hotspot.score;
        best.distanceToPlayer = distance;
        best.source = hotspot.source;
    }

    return best;
}

const char* rewirPressureLevelLabel(RewirPressureLevel level) {
    switch (level) {
    case RewirPressureLevel::Quiet:
        return "quiet";
    case RewirPressureLevel::Watchful:
        return "watch";
    case RewirPressureLevel::Alert:
        return "alert";
    }

    return "unknown";
}

const char* rewirPressureLocationLabel(WorldLocationTag location) {
    switch (location) {
    case WorldLocationTag::Parking:
        return "parking";
    case WorldLocationTag::Garage:
        return "garage";
    case WorldLocationTag::Trash:
        return "trash";
    case WorldLocationTag::RoadLoop:
        return "road";
    case WorldLocationTag::Block:
        return "block";
    case WorldLocationTag::Shop:
        return "shop";
    case WorldLocationTag::Unknown:
        break;
    }

    return "none";
}

} // namespace bs3d
