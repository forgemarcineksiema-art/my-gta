#pragma once

#include "bs3d/core/MissionController.h"
#include "bs3d/core/ParagonMission.h"
#include "bs3d/core/PrzypalSystem.h"
#include "bs3d/core/StoryState.h"
#include "bs3d/core/Types.h"

#include <string>
#include <vector>

namespace bs3d {

struct MissionSaveState {
    MissionPhase phase = MissionPhase::WaitingForStart;
    float phaseSeconds = 0.0f;
    std::string checkpointId;
};

struct SaveGame {
    int version = 1;
    StoryState story{};
    MissionSaveState mission{};
    ParagonMissionPhase paragonPhase = ParagonMissionPhase::Locked;
    Vec3 playerPosition{};
    float playerYawRadians = 0.0f;
    bool playerInVehicle = false;
    Vec3 vehiclePosition{};
    float vehicleYawRadians = 0.0f;
    float vehicleCondition = 100.0f;
    float przypalValue = 0.0f;
    float przypalDecayDelayRemaining = 0.0f;
    PrzypalBand przypalBand = PrzypalBand::Calm;
    bool przypalBandPulseAvailable = false;
    std::vector<PrzypalContributor> przypalContributors;
    std::vector<WorldEvent> worldEvents;
    std::vector<WorldEventEmitterCooldownSnapshot> eventCooldowns;
    std::vector<std::string> completedLocalRewirFavorIds;
};

struct SaveGameValidation {
    bool ok = true;
    std::vector<std::string> errors;
};

std::string serializeSaveGame(const SaveGame& save);
SaveGame deserializeSaveGame(const std::string& text);
SaveGameValidation validateSaveGame(const SaveGame& save);

bool saveGameToFile(const SaveGame& save, const std::string& path);
bool loadSaveGameFromFile(const std::string& path, SaveGame& out);

} // namespace bs3d
