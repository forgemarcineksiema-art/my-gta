#pragma once

#include "EditorOverlayCodec.h"
#include "IntroLevelBuilder.h"

#include "bs3d/core/MissionController.h"
#include "bs3d/core/WorldEventLedger.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace bs3d {

class WorldAssetRegistry;

struct WorldDataBounds {
    Vec3 center{};
    Vec3 size{};
};

struct WorldMapData {
    bool loaded = false;
    std::string id;
    Vec3 playerSpawn{};
    Vec3 vehicleSpawn{};
    Vec3 npcSpawn{};
    Vec3 shopPosition{};
    Vec3 dropoffPosition{};
    WorldDataBounds districtBounds{};
};

struct MissionPhaseData {
    std::string phase;
    std::string objective;
    std::string trigger;
};

struct MissionDialogueData {
    std::string phase;
    std::string speaker;
    std::string lineKey;
    std::string text;
    float durationSeconds = 3.0f;
};

struct MissionNpcReactionData {
    std::string phase;
    std::string speaker;
    std::string lineKey;
    std::string text;
    float durationSeconds = 3.0f;
};

struct MissionCutsceneLineData {
    std::string cutscene;
    std::string phase;
    std::string speaker;
    std::string lineKey;
    std::string text;
    float durationSeconds = 3.0f;
};

struct MissionData {
    bool loaded = false;
    std::string id;
    std::string title;
    std::vector<MissionPhaseData> phases;
    std::vector<MissionDialogueData> dialogue;
    std::vector<MissionNpcReactionData> npcReactions;
    std::vector<MissionCutsceneLineData> cutscenes;
};

struct WorldEditorOverlayData {
    bool loaded = false;
    EditorOverlayDocument document{};
    std::vector<std::string> warnings;
};

struct ObjectOutcomeWorldEventData {
    WorldEventType type = WorldEventType::PublicNuisance;
    float intensity = 0.0f;
    float cooldownSeconds = 0.0f;
};

struct ObjectOutcomeData {
    std::string id;
    std::string idPattern;
    std::string label;
    std::string location;
    std::string speaker;
    std::string line;
    std::optional<ObjectOutcomeWorldEventData> worldEvent;

    std::string key() const {
        return id.empty() ? idPattern : id;
    }
};

struct ObjectOutcomeCatalogData {
    bool loaded = false;
    std::vector<ObjectOutcomeData> outcomes;
};

struct WorldDataCatalog {
    bool loaded = false;
    WorldMapData world{};
    MissionData mission{};
    WorldEditorOverlayData editorOverlay{};
    ObjectOutcomeCatalogData objectOutcomes{};
    std::unordered_map<std::string, std::string> missionLocalization{};
    std::vector<std::string> warnings;
};

struct WorldDataApplyResult {
    bool applied = false;
    int appliedMissionPhases = 0;
    int appliedEditorOverlayOverrides = 0;
    int appliedEditorOverlayInstances = 0;
    std::vector<std::string> warnings;
};

WorldDataCatalog loadWorldDataCatalog(const std::string& dataRoot);
WorldDataApplyResult applyWorldDataCatalog(IntroLevelData& level, const WorldDataCatalog& catalog);
WorldDataApplyResult applyWorldDataCatalog(IntroLevelData& level,
                                           const WorldDataCatalog& catalog,
                                           const WorldAssetRegistry& registry);
int applyMissionDataToController(MissionController& mission, const MissionData& data);
MissionPhase missionPhaseFromDataName(const std::string& phase);
const ObjectOutcomeData* findObjectOutcomeData(const ObjectOutcomeCatalogData& catalog, const std::string& outcomeId);

} // namespace bs3d
