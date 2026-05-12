#include "bs3d/core/SaveGame.h"

#include "bs3d/core/WorldServiceState.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace bs3d {

namespace {

constexpr int MaxSavedWorldEvents = 64;
constexpr int MaxSavedPrzypalContributors = 64;
constexpr int MaxSavedEventCooldowns = 64;
constexpr int MaxSavedCompletedLocalRewirFavors = 64;

std::string vecToText(Vec3 value) {
    std::ostringstream out;
    out << value.x << ',' << value.y << ',' << value.z;
    return out.str();
}

Vec3 parseVec3(const std::string& text) {
    Vec3 value{};
    char commaA = 0;
    char commaB = 0;
    std::istringstream in(text);
    in >> value.x >> commaA >> value.y >> commaB >> value.z;
    if (!in || commaA != ',' || commaB != ',') {
        const float nan = std::numeric_limits<float>::quiet_NaN();
        return {nan, nan, nan};
    }
    return value;
}

std::unordered_map<std::string, std::string> parseLines(const std::string& text) {
    std::unordered_map<std::string, std::string> values;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        values[line.substr(0, equals)] = line.substr(equals + 1);
    }
    return values;
}

int asInt(const std::unordered_map<std::string, std::string>& values, const std::string& key, int fallback) {
    const auto found = values.find(key);
    return found == values.end() ? fallback : std::atoi(found->second.c_str());
}

float asFloat(const std::unordered_map<std::string, std::string>& values, const std::string& key, float fallback) {
    const auto found = values.find(key);
    return found == values.end() ? fallback : std::strtof(found->second.c_str(), nullptr);
}

bool asBool(const std::unordered_map<std::string, std::string>& values, const std::string& key, bool fallback) {
    return asInt(values, key, fallback ? 1 : 0) != 0;
}

std::string asText(const std::unordered_map<std::string, std::string>& values,
                   const std::string& key,
                   const std::string& fallback = {}) {
    const auto found = values.find(key);
    return found == values.end() ? fallback : found->second;
}

bool validMissionPhase(MissionPhase phase) {
    switch (phase) {
    case MissionPhase::WaitingForStart:
    case MissionPhase::WalkToShop:
    case MissionPhase::ReturnToBench:
    case MissionPhase::ReachVehicle:
    case MissionPhase::DriveToShop:
    case MissionPhase::ChaseActive:
    case MissionPhase::ReturnToLot:
    case MissionPhase::Completed:
    case MissionPhase::Failed:
        return true;
    }
    return false;
}

bool validParagonPhase(ParagonMissionPhase phase) {
    switch (phase) {
    case ParagonMissionPhase::Locked:
    case ParagonMissionPhase::TalkToZenon:
    case ParagonMissionPhase::FindReceiptHolder:
    case ParagonMissionPhase::ResolveReceipt:
    case ParagonMissionPhase::ReturnToZenon:
    case ParagonMissionPhase::Completed:
    case ParagonMissionPhase::Failed:
        return true;
    }
    return false;
}

bool validParagonSolution(ParagonSolution solution) {
    switch (solution) {
    case ParagonSolution::None:
    case ParagonSolution::Peaceful:
    case ParagonSolution::Trick:
    case ParagonSolution::Chaos:
        return true;
    }
    return false;
}

bool validPrzypalBand(PrzypalBand band) {
    switch (band) {
    case PrzypalBand::Calm:
    case PrzypalBand::Noticed:
    case PrzypalBand::Hot:
    case PrzypalBand::ChaseRisk:
    case PrzypalBand::Meltdown:
        return true;
    }
    return false;
}

bool validWorldEventType(WorldEventType type) {
    switch (type) {
    case WorldEventType::PublicNuisance:
    case WorldEventType::PropertyDamage:
    case WorldEventType::ChaseSeen:
    case WorldEventType::ShopTrouble:
        return true;
    }
    return false;
}

bool validWorldLocationTag(WorldLocationTag location) {
    switch (location) {
    case WorldLocationTag::Unknown:
    case WorldLocationTag::Block:
    case WorldLocationTag::Shop:
    case WorldLocationTag::Parking:
    case WorldLocationTag::Garage:
    case WorldLocationTag::Trash:
    case WorldLocationTag::RoadLoop:
        return true;
    }
    return false;
}

bool finite(Vec3 value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

void addError(SaveGameValidation& validation, std::string error) {
    validation.ok = false;
    validation.errors.push_back(std::move(error));
}

} // namespace

std::string serializeSaveGame(const SaveGame& save) {
    std::ostringstream out;
    out << "version=" << save.version << '\n';
    out << "story.visitedShopOnFoot=" << (save.story.visitedShopOnFoot ? 1 : 0) << '\n';
    out << "story.gruzUnlocked=" << (save.story.gruzUnlocked ? 1 : 0) << '\n';
    out << "story.introCompleted=" << (save.story.introCompleted ? 1 : 0) << '\n';
    out << "story.shopTroubleSeen=" << (save.story.shopTroubleSeen ? 1 : 0) << '\n';
    out << "story.paragonCompleted=" << (save.story.paragonCompleted ? 1 : 0) << '\n';
    out << "story.shopBanActive=" << (save.story.shopBanActive ? 1 : 0) << '\n';
    out << "story.paragonSolution=" << static_cast<int>(save.story.paragonSolution) << '\n';
    out << "mission.phase=" << static_cast<int>(save.mission.phase) << '\n';
    out << "mission.phaseSeconds=" << save.mission.phaseSeconds << '\n';
    out << "checkpoint=" << save.mission.checkpointId << '\n';
    out << "paragon.phase=" << static_cast<int>(save.paragonPhase) << '\n';
    out << "player.position=" << vecToText(save.playerPosition) << '\n';
    out << "player.yaw=" << save.playerYawRadians << '\n';
    out << "player.inVehicle=" << (save.playerInVehicle ? 1 : 0) << '\n';
    out << "vehicle.position=" << vecToText(save.vehiclePosition) << '\n';
    out << "vehicle.yaw=" << save.vehicleYawRadians << '\n';
    out << "vehicle.condition=" << save.vehicleCondition << '\n';
    out << "przypal.value=" << save.przypalValue << '\n';
    out << "przypal.decayDelayRemaining=" << save.przypalDecayDelayRemaining << '\n';
    out << "przypal.band=" << static_cast<int>(save.przypalBand) << '\n';
    out << "przypal.bandPulseAvailable=" << (save.przypalBandPulseAvailable ? 1 : 0) << '\n';
    out << "przypal.contributors.count=" << save.przypalContributors.size() << '\n';
    for (std::size_t index = 0; index < save.przypalContributors.size(); ++index) {
        const PrzypalContributor& contributor = save.przypalContributors[index];
        const std::string prefix = "przypal.contributor." + std::to_string(index) + ".";
        out << prefix << "type=" << static_cast<int>(contributor.type) << '\n';
        out << prefix << "location=" << static_cast<int>(contributor.location) << '\n';
        out << prefix << "heat=" << contributor.heat << '\n';
        out << prefix << "ageSeconds=" << contributor.ageSeconds << '\n';
    }
    out << "world.events.count=" << save.worldEvents.size() << '\n';
    for (std::size_t index = 0; index < save.worldEvents.size(); ++index) {
        const WorldEvent& event = save.worldEvents[index];
        const std::string prefix = "world.event." + std::to_string(index) + ".";
        out << prefix << "id=" << event.id << '\n';
        out << prefix << "type=" << static_cast<int>(event.type) << '\n';
        out << prefix << "location=" << static_cast<int>(event.location) << '\n';
        out << prefix << "position=" << vecToText(event.position) << '\n';
        out << prefix << "intensity=" << event.intensity << '\n';
        out << prefix << "remainingSeconds=" << event.remainingSeconds << '\n';
        out << prefix << "source=" << event.source << '\n';
        out << prefix << "stackCount=" << event.stackCount << '\n';
        out << prefix << "ageSeconds=" << event.ageSeconds << '\n';
    }
    out << "world.cooldowns.count=" << save.eventCooldowns.size() << '\n';
    for (std::size_t index = 0; index < save.eventCooldowns.size(); ++index) {
        const WorldEventEmitterCooldownSnapshot& cooldown = save.eventCooldowns[index];
        const std::string prefix = "world.cooldown." + std::to_string(index) + ".";
        out << prefix << "type=" << static_cast<int>(cooldown.type) << '\n';
        out << prefix << "source=" << cooldown.source << '\n';
        out << prefix << "remainingSeconds=" << cooldown.remainingSeconds << '\n';
    }
    out << "local.favors.completed.count=" << save.completedLocalRewirFavorIds.size() << '\n';
    for (std::size_t index = 0; index < save.completedLocalRewirFavorIds.size(); ++index) {
        out << "local.favor.completed." << index << ".id=" << save.completedLocalRewirFavorIds[index] << '\n';
    }
    return out.str();
}

SaveGame deserializeSaveGame(const std::string& text) {
    const std::unordered_map<std::string, std::string> values = parseLines(text);
    SaveGame save;
    save.version = asInt(values, "version", 1);
    save.story.visitedShopOnFoot = asBool(values, "story.visitedShopOnFoot", false);
    save.story.gruzUnlocked = asBool(values, "story.gruzUnlocked", false);
    save.story.introCompleted = asBool(values, "story.introCompleted", false);
    save.story.shopTroubleSeen = asBool(values, "story.shopTroubleSeen", false);
    save.story.paragonCompleted = asBool(values, "story.paragonCompleted", false);
    save.story.shopBanActive = asBool(values, "story.shopBanActive", false);
    save.story.paragonSolution = static_cast<ParagonSolution>(asInt(values, "story.paragonSolution", 0));
    save.mission.phase = static_cast<MissionPhase>(asInt(values, "mission.phase", 0));
    save.mission.phaseSeconds = asFloat(values, "mission.phaseSeconds", 0.0f);
    save.mission.checkpointId = asText(values, "checkpoint");
    save.paragonPhase = static_cast<ParagonMissionPhase>(asInt(values, "paragon.phase", 0));
    save.playerPosition = parseVec3(asText(values, "player.position", "0,0,0"));
    save.playerYawRadians = asFloat(values, "player.yaw", 0.0f);
    save.playerInVehicle = asBool(values, "player.inVehicle", false);
    save.vehiclePosition = parseVec3(asText(values, "vehicle.position", "0,0,0"));
    save.vehicleYawRadians = asFloat(values, "vehicle.yaw", 0.0f);
    save.vehicleCondition = asFloat(values, "vehicle.condition", 100.0f);
    save.przypalValue = asFloat(values, "przypal.value", 0.0f);
    save.przypalDecayDelayRemaining = asFloat(values, "przypal.decayDelayRemaining", 0.0f);
    save.przypalBand = static_cast<PrzypalBand>(
        asInt(values, "przypal.band", static_cast<int>(PrzypalSystem::bandForValue(save.przypalValue))));
    save.przypalBandPulseAvailable = asBool(values, "przypal.bandPulseAvailable", false);
    const int contributorCount =
        std::clamp(asInt(values, "przypal.contributors.count", 0), 0, MaxSavedPrzypalContributors);
    // Cap parsed count to prevent excessive allocation from tampered/corrupt files.
    save.przypalContributors.reserve(static_cast<std::size_t>(contributorCount));
    for (int index = 0; index < contributorCount; ++index) {
        const std::string prefix = "przypal.contributor." + std::to_string(index) + ".";
        PrzypalContributor contributor;
        contributor.type = static_cast<WorldEventType>(asInt(values, prefix + "type", 0));
        contributor.location = static_cast<WorldLocationTag>(asInt(values, prefix + "location", 0));
        contributor.heat = asFloat(values, prefix + "heat", 0.0f);
        contributor.ageSeconds = asFloat(values, prefix + "ageSeconds", 0.0f);
        save.przypalContributors.push_back(contributor);
    }
    const int eventCount = std::clamp(asInt(values, "world.events.count", 0), 0, MaxSavedWorldEvents);
    save.worldEvents.reserve(static_cast<std::size_t>(eventCount));
    for (int index = 0; index < eventCount; ++index) {
        const std::string prefix = "world.event." + std::to_string(index) + ".";
        WorldEvent event;
        event.id = asInt(values, prefix + "id", 0);
        event.type = static_cast<WorldEventType>(asInt(values, prefix + "type", 0));
        event.location = static_cast<WorldLocationTag>(asInt(values, prefix + "location", 0));
        event.position = parseVec3(asText(values, prefix + "position", "0,0,0"));
        event.intensity = asFloat(values, prefix + "intensity", 0.0f);
        event.remainingSeconds = asFloat(values, prefix + "remainingSeconds", 0.0f);
        event.source = asText(values, prefix + "source");
        event.stackCount = asInt(values, prefix + "stackCount", 1);
        event.ageSeconds = asFloat(values, prefix + "ageSeconds", 0.0f);
        save.worldEvents.push_back(event);
    }
    const int cooldownCount =
        std::clamp(asInt(values, "world.cooldowns.count", 0), 0, MaxSavedEventCooldowns);
    save.eventCooldowns.reserve(static_cast<std::size_t>(cooldownCount));
    for (int index = 0; index < cooldownCount; ++index) {
        const std::string prefix = "world.cooldown." + std::to_string(index) + ".";
        WorldEventEmitterCooldownSnapshot cooldown;
        cooldown.type = static_cast<WorldEventType>(asInt(values, prefix + "type", 0));
        cooldown.source = asText(values, prefix + "source");
        cooldown.remainingSeconds = asFloat(values, prefix + "remainingSeconds", 0.0f);
        save.eventCooldowns.push_back(cooldown);
    }
    const int completedFavorCount =
        std::clamp(asInt(values, "local.favors.completed.count", 0), 0, MaxSavedCompletedLocalRewirFavors);
    save.completedLocalRewirFavorIds.reserve(static_cast<std::size_t>(completedFavorCount));
    for (int index = 0; index < completedFavorCount; ++index) {
        save.completedLocalRewirFavorIds.push_back(
            asText(values, "local.favor.completed." + std::to_string(index) + ".id"));
    }
    return save;
}

SaveGameValidation validateSaveGame(const SaveGame& save) {
    SaveGameValidation validation;
    // Version lock: reject unknown format versions to prevent misparse.
    if (save.version != 1) {
        addError(validation, "unsupported save version: " + std::to_string(save.version));
    }
    if (!validMissionPhase(save.mission.phase)) {
        addError(validation, "invalid mission phase");
    }
    if (!std::isfinite(save.mission.phaseSeconds) || save.mission.phaseSeconds < 0.0f) {
        addError(validation, "invalid mission phase seconds");
    }
    if (!validParagonPhase(save.paragonPhase)) {
        addError(validation, "invalid paragon phase");
    }
    if (!validParagonSolution(save.story.paragonSolution)) {
        addError(validation, "invalid paragon solution");
    }
    if (!finite(save.playerPosition)) {
        addError(validation, "invalid player position");
    }
    if (!std::isfinite(save.playerYawRadians)) {
        addError(validation, "invalid player yaw");
    }
    if (!finite(save.vehiclePosition)) {
        addError(validation, "invalid vehicle position");
    }
    if (!std::isfinite(save.vehicleYawRadians)) {
        addError(validation, "invalid vehicle yaw");
    }
    if (!std::isfinite(save.vehicleCondition) || save.vehicleCondition < 0.0f || save.vehicleCondition > 100.0f) {
        addError(validation, "invalid vehicle condition");
    }
    if (!std::isfinite(save.przypalValue) || save.przypalValue < 0.0f || save.przypalValue > 100.0f) {
        addError(validation, "invalid przypal value");
    }
    if (!std::isfinite(save.przypalDecayDelayRemaining) || save.przypalDecayDelayRemaining < 0.0f) {
        addError(validation, "invalid przypal decay delay");
    }
    if (!validPrzypalBand(save.przypalBand)) {
        addError(validation, "invalid przypal band");
    }
    if (save.przypalContributors.size() > MaxSavedPrzypalContributors) {
        addError(validation, "too many przypal contributors");
    }
    for (const PrzypalContributor& contributor : save.przypalContributors) {
        if (!validWorldEventType(contributor.type)) {
            addError(validation, "invalid przypal contributor type");
        }
        if (!validWorldLocationTag(contributor.location)) {
            addError(validation, "invalid przypal contributor location");
        }
        if (!std::isfinite(contributor.heat) || contributor.heat < 0.0f) {
            addError(validation, "invalid przypal contributor heat");
        }
        if (!std::isfinite(contributor.ageSeconds) || contributor.ageSeconds < 0.0f) {
            addError(validation, "invalid przypal contributor age");
        }
    }
    if (save.worldEvents.size() > MaxSavedWorldEvents) {
        addError(validation, "too many world memory events");
    }
    for (const WorldEvent& event : save.worldEvents) {
        if (event.id <= 0) {
            addError(validation, "invalid world event id");
        }
        if (!validWorldEventType(event.type)) {
            addError(validation, "invalid world event type");
        }
        if (!validWorldLocationTag(event.location)) {
            addError(validation, "invalid world event location");
        }
        if (!finite(event.position)) {
            addError(validation, "invalid world event position");
        }
        if (!std::isfinite(event.intensity) || event.intensity < 0.0f || event.intensity > 3.0f) {
            addError(validation, "invalid world event intensity");
        }
        if (!std::isfinite(event.remainingSeconds) || event.remainingSeconds <= 0.0f) {
            addError(validation, "invalid world event remaining time");
        }
        if (event.stackCount < 1 || event.stackCount > 5) {
            addError(validation, "invalid world event stack count");
        }
        if (!std::isfinite(event.ageSeconds) || event.ageSeconds < 0.0f) {
            addError(validation, "invalid world event age");
        }
        if (event.source.find('\n') != std::string::npos || event.source.find('\r') != std::string::npos) {
            addError(validation, "invalid world event source");
        }
    }
    if (save.eventCooldowns.size() > MaxSavedEventCooldowns) {
        addError(validation, "too many world cooldowns");
    }
    for (const WorldEventEmitterCooldownSnapshot& cooldown : save.eventCooldowns) {
        if (!validWorldEventType(cooldown.type)) {
            addError(validation, "invalid world cooldown type");
        }
        if (!std::isfinite(cooldown.remainingSeconds) || cooldown.remainingSeconds < 0.0f) {
            addError(validation, "invalid world cooldown remaining time");
        }
        if (cooldown.source.find('\n') != std::string::npos || cooldown.source.find('\r') != std::string::npos) {
            addError(validation, "invalid world cooldown source");
        }
    }
    if (save.completedLocalRewirFavorIds.size() > MaxSavedCompletedLocalRewirFavors) {
        addError(validation, "too many completed local rewir favors");
    }
    for (const std::string& favorId : save.completedLocalRewirFavorIds) {
        if (favorId.empty() || favorId.size() > 96) {
            addError(validation, "invalid completed local rewir favor id");
        }
        if (favorId.find('\n') != std::string::npos || favorId.find('\r') != std::string::npos) {
            addError(validation, "invalid completed local rewir favor id");
        }
        if (!localRewirFavorById(favorId).has_value()) {
            addError(validation, "unknown completed local rewir favor id");
        }
    }
    return validation;
}

bool saveGameToFile(const SaveGame& save, const std::string& path) {
    // Validate before writing to prevent corrupt saves on disk.
    if (!validateSaveGame(save).ok) {
        return false;
    }

    const std::filesystem::path target(path);
    std::error_code ec;
    if (!target.parent_path().empty()) {
        std::filesystem::create_directories(target.parent_path(), ec);
        if (ec) {
            return false;
        }
    }

    const std::filesystem::path temp = target.string() + ".tmp";
    std::ofstream out(temp, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << serializeSaveGame(save);
    out.close();
    if (!out.good()) {
        std::filesystem::remove(temp, ec);
        return false;
    }

    std::filesystem::remove(target, ec);
    ec.clear();
    std::filesystem::rename(temp, target, ec);
    if (ec) {
        std::filesystem::remove(temp, ec);
        return false;
    }
    return true;
}

bool loadSaveGameFromFile(const std::string& path, SaveGame& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = deserializeSaveGame(buffer.str());
    // Re-validate after deserialization to catch corrupted or tampered files.
    return validateSaveGame(out).ok;
}

} // namespace bs3d
