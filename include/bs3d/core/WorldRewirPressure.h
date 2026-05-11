#pragma once

#include "bs3d/core/WorldEventLedger.h"

#include <string>
#include <vector>

namespace bs3d {

enum class RewirPressureLevel {
    Quiet,
    Watchful,
    Alert
};

struct RewirPressureSnapshot {
    int eventId = 0;
    bool active = false;
    bool patrolInterest = false;
    RewirPressureLevel level = RewirPressureLevel::Quiet;
    WorldLocationTag location = WorldLocationTag::Unknown;
    Vec3 position{};
    float score = 0.0f;
    float distanceToPlayer = 0.0f;
    std::string source;
};

const std::vector<WorldLocationTag>& rewirPressureLocations();
RewirPressureSnapshot resolveRewirPressure(const WorldEventLedger& ledger, Vec3 playerPosition);
const char* rewirPressureLevelLabel(RewirPressureLevel level);
const char* rewirPressureLocationLabel(WorldLocationTag location);

} // namespace bs3d
