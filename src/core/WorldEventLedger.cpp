#include "bs3d/core/WorldEventLedger.h"

#include <algorithm>
#include <array>
#include <utility>

namespace bs3d {

namespace {

constexpr float MergeRadius = 6.0f;
constexpr int MaxStackCount = 5;

float clampedIntensity(float intensity) {
    return std::clamp(intensity, 0.0f, 3.0f);
}

float heatMultiplierForMergedStack(int stackCount) {
    return stackCount >= 3 ? 0.25f : 0.45f;
}

float hotspotScore(const WorldEvent& event) {
    return event.intensity * static_cast<float>(std::max(1, event.stackCount));
}

WorldEventHotspot makeHotspot(const WorldEvent& event) {
    WorldEventHotspot hotspot;
    hotspot.eventId = event.id;
    hotspot.type = event.type;
    hotspot.location = event.location;
    hotspot.position = event.position;
    hotspot.intensity = event.intensity;
    hotspot.stackCount = event.stackCount;
    hotspot.ageSeconds = event.ageSeconds;
    hotspot.remainingSeconds = event.remainingSeconds;
    hotspot.score = hotspotScore(event);
    hotspot.source = event.source;
    return hotspot;
}

} // namespace

WorldEventAddResult WorldEventLedger::add(WorldEvent event) {
    event.intensity = clampedIntensity(event.intensity);
    event.stackCount = std::max(1, event.stackCount);
    if (event.remainingSeconds <= 0.0f) {
        event.remainingSeconds = defaultDuration(event.type);
    }

    for (WorldEvent& existing : events_) {
        if (!canMerge(existing, event)) {
            continue;
        }

        const int previousStack = existing.stackCount;
        existing.stackCount = std::min(MaxStackCount, existing.stackCount + 1);
        existing.remainingSeconds = std::max(existing.remainingSeconds, event.remainingSeconds);
        existing.intensity = std::max(existing.intensity, event.intensity);
        existing.position = event.position;

        WorldEventAddResult result;
        result.addedToLedger = true;
        result.heatPulseAccepted = previousStack < MaxStackCount;
        result.accepted = result.heatPulseAccepted;
        result.merged = true;
        result.eventId = existing.id;
        result.type = existing.type;
        result.location = existing.location;
        result.intensity = event.intensity;
        result.newStackCount = existing.stackCount;
        result.heatMultiplier = result.heatPulseAccepted ? heatMultiplierForMergedStack(existing.stackCount) : 0.0f;
        return result;
    }

    event.id = nextEventId_++;
    event.stackCount = 1;
    event.ageSeconds = 0.0f;
    events_.push_back(event);

    WorldEventAddResult result;
    result.addedToLedger = true;
    result.heatPulseAccepted = true;
    result.accepted = true;
    result.merged = false;
    result.eventId = event.id;
    result.type = event.type;
    result.location = event.location;
    result.intensity = event.intensity;
    result.newStackCount = 1;
    result.heatMultiplier = 1.0f;
    return result;
}

void WorldEventLedger::update(float deltaSeconds) {
    const float dt = std::max(0.0f, deltaSeconds);
    for (WorldEvent& event : events_) {
        event.remainingSeconds = std::max(0.0f, event.remainingSeconds - dt);
        event.ageSeconds += dt;
    }

    events_.erase(std::remove_if(events_.begin(),
                                 events_.end(),
                                 [](const WorldEvent& event) { return event.remainingSeconds <= 0.0f; }),
                  events_.end());
}

void WorldEventLedger::clear() {
    events_.clear();
    nextEventId_ = 1;
}

void WorldEventLedger::restoreRecentEvents(std::vector<WorldEvent> events) {
    events_ = std::move(events);
    nextEventId_ = 1;
    for (const WorldEvent& event : events_) {
        nextEventId_ = std::max(nextEventId_, event.id + 1);
    }
}

std::vector<WorldEvent> WorldEventLedger::query(WorldEventType type,
                                                WorldLocationTag location,
                                                Vec3 position,
                                                float radius) const {
    std::vector<WorldEvent> matches;
    const float radiusSq = radius * radius;
    for (const WorldEvent& event : events_) {
        if (event.type != type || event.location != location) {
            continue;
        }
        if (distanceSquaredXZ(event.position, position) > radiusSq) {
            continue;
        }
        matches.push_back(event);
    }
    return matches;
}

std::vector<WorldEvent> WorldEventLedger::queryByLocationAndSource(WorldLocationTag location,
                                                                   const std::string& source) const {
    std::vector<WorldEvent> matches;
    for (const WorldEvent& event : events_) {
        if (event.location != location || event.source != source) {
            continue;
        }
        matches.push_back(event);
    }
    return matches;
}

WorldEventLocationCounts WorldEventLedger::locationCounts() const {
    WorldEventLocationCounts counts;
    for (const WorldEvent& event : events_) {
        switch (event.location) {
        case WorldLocationTag::Unknown:
            ++counts.unknown;
            break;
        case WorldLocationTag::Block:
            ++counts.block;
            break;
        case WorldLocationTag::Shop:
            ++counts.shop;
            break;
        case WorldLocationTag::Parking:
            ++counts.parking;
            break;
        case WorldLocationTag::Garage:
            ++counts.garage;
            break;
        case WorldLocationTag::Trash:
            ++counts.trash;
            break;
        case WorldLocationTag::RoadLoop:
            ++counts.roadLoop;
            break;
        }
    }
    return counts;
}

std::vector<WorldEventHotspot> WorldEventLedger::memoryHotspots() const {
    static constexpr std::array<WorldLocationTag, 6> RewirOrder{
        WorldLocationTag::Block,
        WorldLocationTag::Shop,
        WorldLocationTag::Parking,
        WorldLocationTag::Garage,
        WorldLocationTag::Trash,
        WorldLocationTag::RoadLoop,
    };

    std::vector<WorldEventHotspot> hotspots;
    for (WorldLocationTag location : RewirOrder) {
        const WorldEvent* strongest = nullptr;
        float strongestScore = -1.0f;
        for (const WorldEvent& event : events_) {
            if (event.location != location) {
                continue;
            }

            const float score = hotspotScore(event);
            if (strongest == nullptr || score > strongestScore) {
                strongest = &event;
                strongestScore = score;
            }
        }

        if (strongest != nullptr) {
            hotspots.push_back(makeHotspot(*strongest));
        }
    }

    return hotspots;
}

const std::vector<WorldEvent>& WorldEventLedger::recentEvents() const {
    return events_;
}

float WorldEventLedger::defaultDuration(WorldEventType type) {
    switch (type) {
    case WorldEventType::PublicNuisance:
    case WorldEventType::ChaseSeen:
        return 14.0f;
    case WorldEventType::PropertyDamage:
    case WorldEventType::ShopTrouble:
        return 18.0f;
    }

    return 14.0f;
}

bool WorldEventLedger::canMerge(const WorldEvent& existing, const WorldEvent& incoming) {
    return existing.type == incoming.type &&
           existing.location == incoming.location &&
           existing.source == incoming.source &&
           distanceSquaredXZ(existing.position, incoming.position) <= MergeRadius * MergeRadius;
}

bool WorldEventEmitterCooldowns::consume(WorldEventType type, const std::string& source, float cooldownSeconds) {
    const float cooldown = std::max(0.0f, cooldownSeconds);
    for (Entry& entry : cooldowns_) {
        if (entry.type == type && entry.source == source) {
            if (entry.remainingSeconds > 0.0f) {
                return false;
            }
            entry.remainingSeconds = cooldown;
            return true;
        }
    }

    cooldowns_.push_back({type, source, cooldown});
    return true;
}

void WorldEventEmitterCooldowns::update(float deltaSeconds) {
    const float dt = std::max(0.0f, deltaSeconds);
    for (Entry& entry : cooldowns_) {
        entry.remainingSeconds = std::max(0.0f, entry.remainingSeconds - dt);
    }
}

void WorldEventEmitterCooldowns::clear() {
    cooldowns_.clear();
}

std::vector<WorldEventEmitterCooldownSnapshot> WorldEventEmitterCooldowns::snapshot() const {
    std::vector<WorldEventEmitterCooldownSnapshot> result;
    result.reserve(cooldowns_.size());
    for (const Entry& entry : cooldowns_) {
        result.push_back({entry.type, entry.source, entry.remainingSeconds});
    }
    return result;
}

void WorldEventEmitterCooldowns::restore(std::vector<WorldEventEmitterCooldownSnapshot> cooldowns) {
    cooldowns_.clear();
    cooldowns_.reserve(cooldowns.size());
    for (WorldEventEmitterCooldownSnapshot cooldown : cooldowns) {
        cooldown.remainingSeconds = std::max(0.0f, cooldown.remainingSeconds);
        cooldowns_.push_back({cooldown.type, std::move(cooldown.source), cooldown.remainingSeconds});
    }
}

bool shouldEmitPropertyDamage(float impactSpeed, float threshold) {
    return impactSpeed >= threshold;
}

} // namespace bs3d
