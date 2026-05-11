#pragma once

#include "bs3d/core/Types.h"

#include <string>
#include <vector>

namespace bs3d {

enum class WorldEventType {
    PublicNuisance,
    PropertyDamage,
    ChaseSeen,
    ShopTrouble
};

enum class WorldLocationTag {
    Unknown,
    Block,
    Shop,
    Parking,
    Garage,
    Trash,
    RoadLoop
};

struct WorldEvent {
    int id = 0;
    WorldEventType type = WorldEventType::PublicNuisance;
    WorldLocationTag location = WorldLocationTag::Unknown;
    Vec3 position{};
    float intensity = 1.0f;
    float remainingSeconds = 0.0f;
    std::string source;
    int stackCount = 1;
    float ageSeconds = 0.0f;
};

struct WorldEventAddResult {
    bool accepted = false;
    bool addedToLedger = false;
    bool heatPulseAccepted = false;
    bool merged = false;
    int eventId = 0;
    WorldEventType type = WorldEventType::PublicNuisance;
    WorldLocationTag location = WorldLocationTag::Unknown;
    float intensity = 0.0f;
    int newStackCount = 0;
    float heatMultiplier = 0.0f;
};

struct WorldEventLocationCounts {
    int unknown = 0;
    int block = 0;
    int shop = 0;
    int parking = 0;
    int garage = 0;
    int trash = 0;
    int roadLoop = 0;
};

struct WorldEventHotspot {
    int eventId = 0;
    WorldEventType type = WorldEventType::PublicNuisance;
    WorldLocationTag location = WorldLocationTag::Unknown;
    Vec3 position{};
    float intensity = 0.0f;
    int stackCount = 0;
    float ageSeconds = 0.0f;
    float remainingSeconds = 0.0f;
    float score = 0.0f;
    std::string source;
};

class WorldEventLedger {
public:
    WorldEventAddResult add(WorldEvent event);
    void update(float deltaSeconds);
    void clear();
    void restoreRecentEvents(std::vector<WorldEvent> events);

    std::vector<WorldEvent> query(WorldEventType type,
                                  WorldLocationTag location,
                                  Vec3 position,
                                  float radius) const;
    std::vector<WorldEvent> queryByLocationAndSource(WorldLocationTag location,
                                                     const std::string& source) const;
    WorldEventLocationCounts locationCounts() const;
    std::vector<WorldEventHotspot> memoryHotspots() const;

    const std::vector<WorldEvent>& recentEvents() const;

private:
    std::vector<WorldEvent> events_;
    int nextEventId_ = 1;

    static float defaultDuration(WorldEventType type);
    static bool canMerge(const WorldEvent& existing, const WorldEvent& incoming);
};

struct WorldEventEmitterCooldownSnapshot {
    WorldEventType type = WorldEventType::PublicNuisance;
    std::string source;
    float remainingSeconds = 0.0f;
};

class WorldEventEmitterCooldowns {
public:
    bool consume(WorldEventType type, const std::string& source, float cooldownSeconds);
    void update(float deltaSeconds);
    void clear();
    std::vector<WorldEventEmitterCooldownSnapshot> snapshot() const;
    void restore(std::vector<WorldEventEmitterCooldownSnapshot> cooldowns);

private:
    struct Entry {
        WorldEventType type = WorldEventType::PublicNuisance;
        std::string source;
        float remainingSeconds = 0.0f;
    };

    std::vector<Entry> cooldowns_;
};

bool shouldEmitPropertyDamage(float impactSpeed, float threshold = 4.0f);

} // namespace bs3d
