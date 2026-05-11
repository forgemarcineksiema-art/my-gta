#include "WorldDataLoader.h"

#include "EditorOverlayApply.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace bs3d {

namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::filesystem::path resolveDataRoot(const std::string& dataRoot) {
    std::filesystem::path candidate(dataRoot);
    if (std::filesystem::exists(candidate)) {
        return candidate;
    }
    candidate = std::filesystem::path("..") / dataRoot;
    if (std::filesystem::exists(candidate)) {
        return candidate;
    }
    candidate = std::filesystem::path("..") / ".." / dataRoot;
    return candidate;
}

enum class JsonType {
    Null,
    Number,
    String,
    Array,
    Object
};

struct JsonValue {
    JsonType type = JsonType::Null;
    double number = 0.0;
    std::string text;
    std::vector<JsonValue> array;
    std::unordered_map<std::string, JsonValue> object;
};

class JsonParser {
public:
    explicit JsonParser(const std::string& source) : source_(source) {}

    JsonValue parse() {
        skipWhitespace();
        return parseValue();
    }

private:
    const std::string& source_;
    std::size_t cursor_ = 0;

    void skipWhitespace() {
        while (cursor_ < source_.size() &&
               std::isspace(static_cast<unsigned char>(source_[cursor_]))) {
            ++cursor_;
        }
    }

    bool consume(char expected) {
        skipWhitespace();
        if (cursor_ >= source_.size() || source_[cursor_] != expected) {
            return false;
        }
        ++cursor_;
        return true;
    }

    JsonValue parseValue() {
        skipWhitespace();
        if (cursor_ >= source_.size()) {
            return {};
        }
        const char ch = source_[cursor_];
        if (ch == '"') {
            JsonValue value;
            value.type = JsonType::String;
            value.text = parseString();
            return value;
        }
        if (ch == '[') {
            return parseArray();
        }
        if (ch == '{') {
            return parseObject();
        }
        return parseNumberOrLiteral();
    }

    std::string parseString() {
        std::string result;
        if (!consume('"')) {
            return result;
        }
        while (cursor_ < source_.size()) {
            const char ch = source_[cursor_++];
            if (ch == '"') {
                break;
            }
            if (ch == '\\' && cursor_ < source_.size()) {
                const char escaped = source_[cursor_++];
                switch (escaped) {
                case '"':
                case '\\':
                case '/':
                    result.push_back(escaped);
                    break;
                case 'n':
                    result.push_back('\n');
                    break;
                case 't':
                    result.push_back('\t');
                    break;
                default:
                    result.push_back(escaped);
                    break;
                }
            } else {
                result.push_back(ch);
            }
        }
        return result;
    }

    JsonValue parseArray() {
        JsonValue value;
        value.type = JsonType::Array;
        consume('[');
        skipWhitespace();
        if (consume(']')) {
            return value;
        }
        while (cursor_ < source_.size()) {
            value.array.push_back(parseValue());
            skipWhitespace();
            if (consume(']')) {
                break;
            }
            if (!consume(',')) {
                break;
            }
        }
        return value;
    }

    JsonValue parseObject() {
        JsonValue value;
        value.type = JsonType::Object;
        consume('{');
        skipWhitespace();
        if (consume('}')) {
            return value;
        }
        while (cursor_ < source_.size()) {
            skipWhitespace();
            if (cursor_ >= source_.size() || source_[cursor_] != '"') {
                break;
            }
            const std::string key = parseString();
            if (!consume(':')) {
                break;
            }
            value.object[key] = parseValue();
            skipWhitespace();
            if (consume('}')) {
                break;
            }
            if (!consume(',')) {
                break;
            }
        }
        return value;
    }

    JsonValue parseNumberOrLiteral() {
        const std::size_t start = cursor_;
        while (cursor_ < source_.size() &&
               (std::isalnum(static_cast<unsigned char>(source_[cursor_])) ||
                source_[cursor_] == '-' ||
                source_[cursor_] == '+' ||
                source_[cursor_] == '.')) {
            ++cursor_;
        }
        JsonValue value;
        const std::string token = source_.substr(start, cursor_ - start);
        char* end = nullptr;
        const double parsed = std::strtod(token.c_str(), &end);
        if (end != nullptr && *end == '\0') {
            value.type = JsonType::Number;
            value.number = parsed;
        }
        return value;
    }
};

const JsonValue* objectMember(const JsonValue& value, const std::string& key) {
    if (value.type != JsonType::Object) {
        return nullptr;
    }
    const auto found = value.object.find(key);
    return found == value.object.end() ? nullptr : &found->second;
}

std::string stringMember(const JsonValue& value, const std::string& key) {
    const JsonValue* member = objectMember(value, key);
    return member != nullptr && member->type == JsonType::String ? member->text : std::string{};
}

float numberMember(const JsonValue& value, const std::string& key, float fallback = 0.0f) {
    const JsonValue* member = objectMember(value, key);
    return member != nullptr && member->type == JsonType::Number ? static_cast<float>(member->number) : fallback;
}

std::string resolveLocalizedText(const JsonValue& value,
                                const std::unordered_map<std::string, std::string>& localization,
                                const std::string& fallbackLineField = "line") {
    const std::string text = stringMember(value, "text");
    if (!text.empty()) {
        return text;
    }
    const std::string legacyLine = stringMember(value, fallbackLineField);
    if (!legacyLine.empty()) {
        return legacyLine;
    }
    const std::string line = stringMember(value, "lineKey");
    if (line.empty()) {
        return {};
    }
    const auto found = localization.find(line);
    if (found == localization.end() || found->second.empty()) {
        return {};
    }
    return found->second;
}

Vec3 vec3FromArray(const JsonValue* value) {
    if (value == nullptr || value->type != JsonType::Array || value->array.size() < 3) {
        return {};
    }
    if (value->array[0].type != JsonType::Number ||
        value->array[1].type != JsonType::Number ||
        value->array[2].type != JsonType::Number) {
        return {};
    }
    return {static_cast<float>(value->array[0].number),
            static_cast<float>(value->array[1].number),
            static_cast<float>(value->array[2].number)};
}

const JsonValue* nestedObjectMember(const JsonValue& value,
                                    const std::string& primary,
                                    const std::string& fallback) {
    if (const JsonValue* member = objectMember(value, primary)) {
        return member;
    }
    return objectMember(value, fallback);
}

WorldEventType worldEventTypeFromDataName(const std::string& type) {
    if (type == "PropertyDamage") {
        return WorldEventType::PropertyDamage;
    }
    if (type == "ChaseSeen") {
        return WorldEventType::ChaseSeen;
    }
    if (type == "ShopTrouble") {
        return WorldEventType::ShopTrouble;
    }
    return WorldEventType::PublicNuisance;
}

ObjectOutcomeCatalogData parseObjectOutcomeCatalog(const std::string& text) {
    ObjectOutcomeCatalogData result;
    if (text.empty()) {
        return result;
    }

    const JsonValue catalog = JsonParser(text).parse();
    const JsonValue* outcomes = objectMember(catalog, "outcomes");
    result.loaded = catalog.type == JsonType::Object &&
                    numberMember(catalog, "schemaVersion", 0.0f) == 1.0f &&
                    outcomes != nullptr &&
                    outcomes->type == JsonType::Array;
    if (!result.loaded) {
        return result;
    }

    for (const JsonValue& outcomeValue : outcomes->array) {
        if (outcomeValue.type != JsonType::Object) {
            continue;
        }
        ObjectOutcomeData outcome;
        outcome.id = stringMember(outcomeValue, "id");
        outcome.idPattern = stringMember(outcomeValue, "idPattern");
        outcome.label = stringMember(outcomeValue, "label");
        outcome.location = stringMember(outcomeValue, "location");
        outcome.speaker = stringMember(outcomeValue, "speaker");
        outcome.line = stringMember(outcomeValue, "line");
        if (outcome.id.empty() && outcome.idPattern.empty()) {
            continue;
        }

        if (const JsonValue* worldEvent = objectMember(outcomeValue, "worldEvent");
            worldEvent != nullptr && worldEvent->type == JsonType::Object) {
            ObjectOutcomeWorldEventData event;
            event.type = worldEventTypeFromDataName(stringMember(*worldEvent, "type"));
            event.intensity = numberMember(*worldEvent, "intensity", 0.0f);
            event.cooldownSeconds = numberMember(*worldEvent, "cooldownSeconds", 0.0f);
            if (event.intensity > 0.0f && event.cooldownSeconds >= 1.0f) {
                outcome.worldEvent = event;
            }
        }
        result.outcomes.push_back(std::move(outcome));
    }
    return result;
}

std::unordered_map<std::string, std::string> parseMissionLocalization(const std::string& text) {
    std::unordered_map<std::string, std::string> lines;
    if (text.empty()) {
        return lines;
    }

    const JsonValue localization = JsonParser(text).parse();
    if (localization.type != JsonType::Object ||
        numberMember(localization, "schemaVersion", 0.0f) != 1.0f) {
        return lines;
    }
    const JsonValue* entries = objectMember(localization, "lines");
    if (entries == nullptr || entries->type != JsonType::Object) {
        return lines;
    }
    for (const auto& pair : entries->object) {
        if (pair.second.type == JsonType::String && !pair.first.empty()) {
            lines[pair.first] = pair.second.text;
        }
    }
    return lines;
}

} // namespace

MissionPhase missionPhaseFromDataName(const std::string& phase) {
    if (phase == "WalkToShop") {
        return MissionPhase::WalkToShop;
    }
    if (phase == "ReturnToBench") {
        return MissionPhase::ReturnToBench;
    }
    if (phase == "ReachVehicle") {
        return MissionPhase::ReachVehicle;
    }
    if (phase == "DriveToShop") {
        return MissionPhase::DriveToShop;
    }
    if (phase == "ChaseActive") {
        return MissionPhase::ChaseActive;
    }
    if (phase == "ReturnToLot") {
        return MissionPhase::ReturnToLot;
    }
    if (phase == "Completed") {
        return MissionPhase::Completed;
    }
    return MissionPhase::WaitingForStart;
}

namespace {

WorldDataBounds boundsFromSpawns(const std::vector<Vec3>& points) {
    Vec3 min = points.front();
    Vec3 max = points.front();
    for (Vec3 point : points) {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }
    const Vec3 padding{8.0f, 4.0f, 8.0f};
    return {(min + max) * 0.5f, (max - min) + padding};
}

} // namespace

WorldDataCatalog loadWorldDataCatalog(const std::string& dataRoot) {
    WorldDataCatalog catalog;
    const std::filesystem::path root = resolveDataRoot(dataRoot);
    const std::string mapText = readFile(root / "map_block_loop.json");
    const std::string missionText = readFile(root / "mission_driving_errand.json");
    const std::string outcomeText = readFile(root / "world" / "object_outcome_catalog.json");
    const std::string missionLocalizationText = readFile(root / "world" / "mission_localization_pl.json");
    catalog.missionLocalization = parseMissionLocalization(missionLocalizationText);

    if (!mapText.empty()) {
        const JsonValue map = JsonParser(mapText).parse();
        const JsonValue* spawn = nestedObjectMember(map, "spawn", "spawns");
        const JsonValue* points = objectMember(map, "points");
        catalog.world.loaded = map.type == JsonType::Object;
        catalog.world.id = stringMember(map, "id");
        catalog.world.playerSpawn = spawn != nullptr ? vec3FromArray(objectMember(*spawn, "player")) : Vec3{};
        catalog.world.vehicleSpawn = spawn != nullptr ? vec3FromArray(objectMember(*spawn, "vehicle")) : Vec3{};
        catalog.world.npcSpawn = spawn != nullptr ? vec3FromArray(objectMember(*spawn, "npc")) : Vec3{};
        catalog.world.shopPosition = points != nullptr
                                          ? vec3FromArray(objectMember(*points, "shop"))
                                          : (spawn != nullptr ? vec3FromArray(objectMember(*spawn, "shop")) : Vec3{});
        catalog.world.dropoffPosition = points != nullptr
                                             ? vec3FromArray(objectMember(*points, "dropoff"))
                                             : (spawn != nullptr ? vec3FromArray(objectMember(*spawn, "dropoff")) : Vec3{});
        catalog.world.districtBounds = boundsFromSpawns({catalog.world.playerSpawn,
                                                         catalog.world.vehicleSpawn,
                                                         catalog.world.npcSpawn,
                                                         catalog.world.shopPosition,
                                                         catalog.world.dropoffPosition});
        if (!catalog.world.loaded || spawn == nullptr) {
            catalog.world.loaded = false;
            catalog.warnings.push_back("invalid map_block_loop.json schema");
        }
    } else {
        catalog.warnings.push_back("missing map_block_loop.json");
    }

    if (!missionText.empty()) {
        const JsonValue mission = JsonParser(missionText).parse();
        catalog.mission.loaded = mission.type == JsonType::Object;
        catalog.mission.id = stringMember(mission, "id");
        catalog.mission.title = stringMember(mission, "title");

        const JsonValue* steps = nestedObjectMember(mission, "steps", "phases");
        if (steps != nullptr && steps->type == JsonType::Array) {
            for (const JsonValue& step : steps->array) {
                if (step.type != JsonType::Object) {
                    continue;
                }
                MissionPhaseData phase;
                phase.phase = stringMember(step, "phase");
                phase.objective = stringMember(step, "objective");
                phase.trigger = stringMember(step, "trigger");
                if (!phase.phase.empty() && !phase.objective.empty()) {
                    catalog.mission.phases.push_back(std::move(phase));
                }
            }
        }

        if (const JsonValue* dialogue = objectMember(mission, "dialogue");
            dialogue != nullptr && dialogue->type == JsonType::Array) {
            for (const JsonValue& line : dialogue->array) {
                if (line.type != JsonType::Object) {
                    continue;
                }
                MissionDialogueData dialogueLine;
                dialogueLine.phase = stringMember(line, "phase");
                dialogueLine.speaker = stringMember(line, "speaker");
                dialogueLine.lineKey = stringMember(line, "lineKey");
                dialogueLine.text = resolveLocalizedText(line, catalog.missionLocalization);
                if (dialogueLine.text.empty() && !dialogueLine.lineKey.empty()) {
                    const auto found = catalog.missionLocalization.find(dialogueLine.lineKey);
                    if (found != catalog.missionLocalization.end() && !found->second.empty()) {
                        dialogueLine.text = found->second;
                    } else {
                        catalog.warnings.push_back("mission localization key missing: " + dialogueLine.lineKey);
                    }
                }
                dialogueLine.durationSeconds = numberMember(line, "durationSeconds", 3.0f);
                if (!dialogueLine.speaker.empty() && !dialogueLine.text.empty()) {
                    catalog.mission.dialogue.push_back(std::move(dialogueLine));
                }
            }
        }

        if (const JsonValue* npcReactions = objectMember(mission, "npcReactions");
            npcReactions != nullptr && npcReactions->type == JsonType::Array) {
            for (const JsonValue& reaction : npcReactions->array) {
                if (reaction.type != JsonType::Object) {
                    continue;
                }
                MissionNpcReactionData lineData;
                lineData.phase = stringMember(reaction, "phase");
                lineData.speaker = stringMember(reaction, "speaker");
                lineData.lineKey = stringMember(reaction, "lineKey");
                lineData.text = resolveLocalizedText(reaction, catalog.missionLocalization);
                if (lineData.text.empty() && !lineData.lineKey.empty()) {
                    const auto found = catalog.missionLocalization.find(lineData.lineKey);
                    if (found != catalog.missionLocalization.end() && !found->second.empty()) {
                        lineData.text = found->second;
                    } else {
                        catalog.warnings.push_back("mission localization key missing: " + lineData.lineKey);
                    }
                }
                lineData.durationSeconds = numberMember(reaction, "durationSeconds", 3.0f);
                if (!lineData.phase.empty() && !lineData.speaker.empty() && !lineData.text.empty()) {
                    catalog.mission.npcReactions.push_back(std::move(lineData));
                }
            }
        }

        if (const JsonValue* cutscenes = objectMember(mission, "cutscenes");
            cutscenes != nullptr && cutscenes->type == JsonType::Array) {
            for (const JsonValue& cut : cutscenes->array) {
                if (cut.type != JsonType::Object) {
                    continue;
                }
                MissionCutsceneLineData lineData;
                lineData.cutscene = stringMember(cut, "cutscene");
                lineData.phase = stringMember(cut, "phase");
                lineData.speaker = stringMember(cut, "speaker");
                lineData.lineKey = stringMember(cut, "lineKey");
                lineData.text = resolveLocalizedText(cut, catalog.missionLocalization);
                if (lineData.text.empty() && !lineData.lineKey.empty()) {
                    const auto found = catalog.missionLocalization.find(lineData.lineKey);
                    if (found != catalog.missionLocalization.end() && !found->second.empty()) {
                        lineData.text = found->second;
                    } else {
                        catalog.warnings.push_back("mission localization key missing: " + lineData.lineKey);
                    }
                }
                lineData.durationSeconds = numberMember(cut, "durationSeconds", 3.0f);
                if (!lineData.cutscene.empty() && !lineData.phase.empty() &&
                    !lineData.speaker.empty() && !lineData.text.empty()) {
                    catalog.mission.cutscenes.push_back(std::move(lineData));
                }
            }
        }

        if (!catalog.mission.loaded || catalog.mission.phases.empty()) {
            catalog.mission.loaded = false;
            catalog.warnings.push_back("invalid mission_driving_errand.json schema");
        }
    } else {
        catalog.warnings.push_back("missing mission_driving_errand.json");
    }

    const std::filesystem::path overlayPath = root / "world" / "block13_editor_overlay.json";
    if (std::filesystem::exists(overlayPath)) {
        const EditorOverlayLoadResult overlay = loadEditorOverlayFile(overlayPath.string());
        catalog.editorOverlay.loaded = overlay.success;
        catalog.editorOverlay.document = overlay.document;
        catalog.editorOverlay.warnings = overlay.warnings;
        for (const std::string& warning : overlay.warnings) {
            catalog.warnings.push_back("editor overlay: " + warning);
        }
    }

    catalog.objectOutcomes = parseObjectOutcomeCatalog(outcomeText);
    if (!catalog.objectOutcomes.loaded) {
        catalog.warnings.push_back("missing or invalid world/object_outcome_catalog.json");
    }

    catalog.loaded = catalog.world.loaded && catalog.mission.loaded;
    return catalog;
}

WorldDataApplyResult applyWorldDataCatalog(IntroLevelData& level, const WorldDataCatalog& catalog) {
    WorldDataApplyResult result;
    if (!catalog.loaded) {
        return result;
    }
    IntroLevelBuildConfig config;
    const Vec3 shopOffset = catalog.world.shopPosition - config.shopPosition;

    config.playerStart = catalog.world.playerSpawn;
    config.vehicleStart = catalog.world.vehicleSpawn;
    config.npcPosition = catalog.world.npcSpawn;
    config.dropoffPosition = catalog.world.dropoffPosition;
    config.shopPosition = catalog.world.shopPosition;
    config.shopEntrancePosition = config.shopEntrancePosition + shopOffset;
    config.shopRoadPosition = config.shopRoadPosition + shopOffset;
    config.zenonPosition = config.zenonPosition + shopOffset;
    config.initialChaserPosition = config.initialChaserPosition + shopOffset;

    level = IntroLevelBuilder::build(config);
    if (catalog.editorOverlay.loaded) {
        const EditorOverlayApplyResult overlay = applyEditorOverlay(level, catalog.editorOverlay.document);
        result.appliedEditorOverlayOverrides = overlay.appliedOverrides;
        result.appliedEditorOverlayInstances = overlay.appliedInstances;
        result.warnings.insert(result.warnings.end(), overlay.warnings.begin(), overlay.warnings.end());
    }
    result.applied = true;
    result.appliedMissionPhases = static_cast<int>(catalog.mission.phases.size());
    return result;
}

int applyMissionDataToController(MissionController& mission, const MissionData& data) {
    int applied = 0;
    std::unordered_map<int, std::vector<DialogueLine>> missionDialoguesByPhase;
    std::unordered_map<int, std::vector<DialogueLine>> npcReactionLinesByPhase;
    std::unordered_map<int, std::vector<DialogueLine>> cutsceneLinesByPhase;

    for (const MissionPhaseData& phase : data.phases) {
        if (phase.objective.empty()) {
            continue;
        }
        if (phase.phase.empty()) {
            continue;
        }
        const MissionPhase parsedPhase = missionPhaseFromDataName(phase.phase);
        if (parsedPhase == MissionPhase::WaitingForStart && phase.phase != "WaitingForStart") {
            continue;
        }
        mission.setObjectiveOverride(missionPhaseFromDataName(phase.phase), phase.objective);
        ++applied;
    }

    for (const MissionDialogueData& line : data.dialogue) {
        if (line.phase.empty() || line.speaker.empty() || line.text.empty()) {
            continue;
        }
        const MissionPhase parsedPhase = missionPhaseFromDataName(line.phase);
        if (parsedPhase == MissionPhase::WaitingForStart && line.phase != "WaitingForStart") {
            continue;
        }
        missionDialoguesByPhase[static_cast<int>(parsedPhase)].push_back(
            {line.speaker, line.text, line.durationSeconds, DialogueChannel::MissionCritical});
    }

    for (const MissionNpcReactionData& reaction : data.npcReactions) {
        if (reaction.phase.empty() || reaction.speaker.empty() || reaction.text.empty()) {
            continue;
        }
        const MissionPhase parsedPhase = missionPhaseFromDataName(reaction.phase);
        if (parsedPhase == MissionPhase::WaitingForStart && reaction.phase != "WaitingForStart") {
            continue;
        }
        npcReactionLinesByPhase[static_cast<int>(parsedPhase)].push_back(
            {reaction.speaker, reaction.text, reaction.durationSeconds, DialogueChannel::Reaction});
    }

    for (const MissionCutsceneLineData& cutscene : data.cutscenes) {
        if (cutscene.cutscene.empty() || cutscene.phase.empty() ||
            cutscene.speaker.empty() || cutscene.text.empty()) {
            continue;
        }
        const MissionPhase parsedPhase = missionPhaseFromDataName(cutscene.phase);
        if (parsedPhase == MissionPhase::WaitingForStart && cutscene.phase != "WaitingForStart") {
            continue;
        }
        cutsceneLinesByPhase[static_cast<int>(parsedPhase)].push_back(
            {cutscene.speaker, cutscene.text, cutscene.durationSeconds, DialogueChannel::SystemHint});
    }

    for (auto& pair : missionDialoguesByPhase) {
        mission.setMissionDialogueLinesForPhase(static_cast<MissionPhase>(pair.first), std::move(pair.second));
    }
    for (auto& pair : npcReactionLinesByPhase) {
        mission.setNpcReactionLinesForPhase(static_cast<MissionPhase>(pair.first), std::move(pair.second));
    }
    for (auto& pair : cutsceneLinesByPhase) {
        mission.setCutsceneLinesForPhase(static_cast<MissionPhase>(pair.first), std::move(pair.second));
    }
    return applied;
}

const ObjectOutcomeData* findObjectOutcomeData(const ObjectOutcomeCatalogData& catalog, const std::string& outcomeId) {
    for (const ObjectOutcomeData& outcome : catalog.outcomes) {
        if (!outcome.id.empty() && outcome.id == outcomeId) {
            return &outcome;
        }
        if (!outcome.idPattern.empty() && outcome.idPattern.back() == '*') {
            const std::string prefix = outcome.idPattern.substr(0, outcome.idPattern.size() - 1);
            if (outcomeId.find(prefix) == 0) {
                return &outcome;
            }
        }
    }
    return nullptr;
}

} // namespace bs3d
