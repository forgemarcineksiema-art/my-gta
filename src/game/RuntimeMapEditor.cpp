#include "RuntimeMapEditor.h"

#include "EditorOverlayCodec.h"
#include "WorldAssetRegistry.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace bs3d {

namespace {

constexpr std::size_t RuntimeEditorHistoryLimit = 100;

bool hasEditorPrefix(const std::string& id) {
    return id.rfind("editor_", 0) == 0;
}

bool sameVec3(Vec3 lhs, Vec3 rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

EditorOverlayObject toOverlayObject(const WorldObject& object) {
    EditorOverlayObject overlay;
    overlay.id = object.id;
    overlay.assetId = object.assetId;
    overlay.position = object.position;
    overlay.scale = object.scale;
    overlay.yawRadians = object.yawRadians;
    overlay.zoneTag = object.zoneTag;
    overlay.gameplayTags = object.gameplayTags;
    return overlay;
}

std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::string searchableAssetText(const WorldAssetDefinition& asset) {
    std::string text = asset.id + " " + asset.modelPath + " " + asset.originPolicy + " " +
                       asset.renderBucket + " " + asset.defaultCollision;
    for (const std::string& tag : asset.defaultTags) {
        text += " ";
        text += tag;
    }
    return lowerCopy(text);
}

std::vector<std::string> filterTokens(const std::string& filter) {
    std::vector<std::string> tokens;
    std::istringstream stream(filter);
    std::string token;
    while (stream >> token) {
        tokens.push_back(lowerCopy(token));
    }
    return tokens;
}

bool isFiniteVec3(Vec3 value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool isNonGameplayAsset(const WorldAssetDefinition& definition) {
    return definition.assetType == "DebugOnly" ||
           definition.renderBucket == "DebugOnly" ||
           !definition.renderInGameplay;
}

bool validateOverlayObjectForSave(const EditorOverlayObject& object,
                                  const char* section,
                                  const WorldAssetRegistry& registry,
                                  std::vector<std::string>& warnings) {
    bool ok = true;
    const std::string label = std::string("editor overlay ") + section + " '" + object.id + "'";
    if (object.id.empty()) {
        warnings.push_back(std::string("editor overlay ") + section + " has an empty id");
        ok = false;
    }
    if (object.assetId.empty()) {
        warnings.push_back(label + " has an empty asset id");
        ok = false;
    }
    const WorldAssetDefinition* definition = object.assetId.empty() ? nullptr : registry.find(object.assetId);
    if (!object.assetId.empty() && definition == nullptr) {
        warnings.push_back(label + " references unknown asset '" + object.assetId + "'");
        ok = false;
    }
    if (definition != nullptr && isNonGameplayAsset(*definition)) {
        warnings.push_back(label + " references non-gameplay asset '" + object.assetId + "'");
        ok = false;
    }
    if (definition != nullptr && definition->assetType == "CollisionProxy") {
        warnings.push_back(label + " references collision proxy asset '" + object.assetId + "'");
        ok = false;
    }
    if (!isFiniteVec3(object.position)) {
        warnings.push_back(label + " has a non-finite position");
        ok = false;
    }
    if (!isFiniteVec3(object.scale) ||
        object.scale.x <= 0.0f ||
        object.scale.y <= 0.0f ||
        object.scale.z <= 0.0f) {
        warnings.push_back(label + " has an invalid scale");
        ok = false;
    }
    if (!std::isfinite(object.yawRadians)) {
        warnings.push_back(label + " has a non-finite yaw");
        ok = false;
    }

    std::unordered_set<std::string> tags;
    for (const std::string& tag : object.gameplayTags) {
        if (tag.empty()) {
            warnings.push_back(label + " has an empty gameplay tag");
            ok = false;
            continue;
        }
        if (!tags.insert(tag).second) {
            warnings.push_back(label + " has duplicate gameplay tag '" + tag + "'");
            ok = false;
        }
    }
    return ok;
}

bool validateOverlayDocumentForSave(const EditorOverlayDocument& document,
                                    const WorldAssetRegistry& registry,
                                    std::vector<std::string>& warnings) {
    bool ok = true;
    std::unordered_set<std::string> ids;
    const auto validateId = [&ids, &warnings, &ok](const EditorOverlayObject& object, const char* section) {
        if (!object.id.empty() && !ids.insert(object.id).second) {
            warnings.push_back(std::string("editor overlay ") + section + " duplicates id '" + object.id + "'");
            ok = false;
        }
    };

    for (const EditorOverlayObject& object : document.overrides) {
        validateId(object, "override");
        ok = validateOverlayObjectForSave(object, "override", registry, warnings) && ok;
    }
    for (const EditorOverlayObject& object : document.instances) {
        validateId(object, "instance");
        ok = validateOverlayObjectForSave(object, "instance", registry, warnings) && ok;
    }
    return ok;
}

} // namespace

Vec3 runtimeEditorPlacementPosition(Vec3 anchor, float yawRadians, float distanceMeters) {
    return anchor + forwardFromYaw(yawRadians) * distanceMeters;
}

std::string runtimeEditorOverlayPathForDataRoot(const std::string& dataRoot) {
    return (std::filesystem::path(dataRoot) / "world" / "block13_editor_overlay.json").string();
}

bool runtimeEditorAssetMatchesFilter(const WorldAssetDefinition& asset, const std::string& filter) {
    const std::vector<std::string> tokens = filterTokens(filter);
    if (tokens.empty()) {
        return true;
    }

    const std::string searchable = searchableAssetText(asset);
    for (const std::string& token : tokens) {
        if (searchable.find(token) == std::string::npos) {
            return false;
        }
    }
    return true;
}

void RuntimeMapEditor::attach(IntroLevelData& level) {
    level_ = &level;
    selectedObjectId_.clear();
    editedBaseObjectIds_.clear();
    undoStack_.clear();
    redoStack_.clear();
    dirty_ = false;
    generatedInstanceCounter_ = 0;
    revision_ = 0;
}

void RuntimeMapEditor::attach(IntroLevelData& level, const EditorOverlayDocument& loadedOverlay) {
    attach(level);
    editedBaseObjectIds_.reserve(loadedOverlay.overrides.size());
    for (const EditorOverlayObject& overrideObject : loadedOverlay.overrides) {
        if (overrideObject.id.empty() || hasEditorPrefix(overrideObject.id)) {
            continue;
        }
        if (std::find(editedBaseObjectIds_.begin(), editedBaseObjectIds_.end(), overrideObject.id) ==
            editedBaseObjectIds_.end()) {
            editedBaseObjectIds_.push_back(overrideObject.id);
        }
    }
}

bool RuntimeMapEditor::selectObject(const std::string& id) {
    if (level_ == nullptr) {
        return false;
    }
    for (const WorldObject& object : level_->objects) {
        if (object.id == id) {
            selectedObjectId_ = id;
            return true;
        }
    }
    return false;
}

const std::string& RuntimeMapEditor::selectedObjectId() const {
    return selectedObjectId_;
}

WorldObject* RuntimeMapEditor::selectedObject() {
    if (level_ == nullptr || selectedObjectId_.empty()) {
        return nullptr;
    }
    for (WorldObject& object : level_->objects) {
        if (object.id == selectedObjectId_) {
            return &object;
        }
    }
    return nullptr;
}

const WorldObject* RuntimeMapEditor::selectedObject() const {
    if (level_ == nullptr || selectedObjectId_.empty()) {
        return nullptr;
    }
    for (const WorldObject& object : level_->objects) {
        if (object.id == selectedObjectId_) {
            return &object;
        }
    }
    return nullptr;
}

bool RuntimeMapEditor::setSelectedPosition(Vec3 position) {
    WorldObject* object = selectedObject();
    if (object == nullptr) {
        return false;
    }
    if (sameVec3(object->position, position)) {
        return true;
    }
    HistoryState before = captureState();
    object->position = position;
    markSelectedBaseObjectEdited();
    pushHistory(std::move(before));
    return true;
}

bool RuntimeMapEditor::setSelectedPositionSilent(Vec3 position) {
    WorldObject* object = selectedObject();
    if (object == nullptr) {
        return false;
    }
    if (sameVec3(object->position, position)) {
        return true;
    }
    object->position = position;
    markSelectedBaseObjectEdited();
    ++revision_;
    return true;
}

bool RuntimeMapEditor::setSelectedScale(Vec3 scale) {
    WorldObject* object = selectedObject();
    if (object == nullptr) {
        return false;
    }
    if (sameVec3(object->scale, scale)) {
        return true;
    }
    HistoryState before = captureState();
    object->scale = scale;
    markSelectedBaseObjectEdited();
    pushHistory(std::move(before));
    return true;
}

bool RuntimeMapEditor::setSelectedYaw(float yawRadians) {
    WorldObject* object = selectedObject();
    if (object == nullptr) {
        return false;
    }
    if (object->yawRadians == yawRadians) {
        return true;
    }
    HistoryState before = captureState();
    object->yawRadians = yawRadians;
    markSelectedBaseObjectEdited();
    pushHistory(std::move(before));
    return true;
}

bool RuntimeMapEditor::setSelectedAssetId(const std::string& assetId) {
    if (assetId.empty()) {
        return false;
    }
    WorldObject* object = selectedObject();
    if (object == nullptr) {
        return false;
    }
    if (object->assetId == assetId) {
        return true;
    }
    HistoryState before = captureState();
    object->assetId = assetId;
    markSelectedBaseObjectEdited();
    pushHistory(std::move(before));
    return true;
}

bool RuntimeMapEditor::setSelectedZoneTag(WorldLocationTag zoneTag) {
    WorldObject* object = selectedObject();
    if (object == nullptr) {
        return false;
    }
    if (object->zoneTag == zoneTag) {
        return true;
    }
    HistoryState before = captureState();
    object->zoneTag = zoneTag;
    markSelectedBaseObjectEdited();
    pushHistory(std::move(before));
    return true;
}

bool RuntimeMapEditor::setSelectedGameplayTags(std::vector<std::string> tags) {
    WorldObject* object = selectedObject();
    if (object == nullptr) {
        return false;
    }
    if (object->gameplayTags == tags) {
        return true;
    }
    HistoryState before = captureState();
    object->gameplayTags = std::move(tags);
    markSelectedBaseObjectEdited();
    pushHistory(std::move(before));
    return true;
}

bool RuntimeMapEditor::addInstance(const std::string& assetId, Vec3 position) {
    WorldAssetDefinition asset;
    asset.id = assetId;
    return addInstance(asset, position);
}

bool RuntimeMapEditor::addInstance(const WorldAssetDefinition& asset, Vec3 position) {
    if (level_ == nullptr || asset.id.empty()) {
        return false;
    }
    HistoryState before = captureState();

    const auto objectExists = [this](const std::string& id) {
        for (const WorldObject& object : level_->objects) {
            if (object.id == id) {
                return true;
            }
        }
        return false;
    };

    std::string id;
    do {
        id = "editor_" + asset.id + "_" + std::to_string(generatedInstanceCounter_);
        ++generatedInstanceCounter_;
    } while (objectExists(id));

    WorldObject object;
    object.id = id;
    object.assetId = asset.id;
    object.position = position;
    object.gameplayTags = asset.defaultTags;
    object.collision.kind = WorldCollisionShapeKind::None;
    level_->objects.push_back(object);
    selectedObjectId_ = id;
    pushHistory(std::move(before));
    return true;
}

bool RuntimeMapEditor::canDeleteSelectedOverlayInstance() const {
    return selectedObject() != nullptr && hasEditorPrefix(selectedObjectId_);
}

bool RuntimeMapEditor::deleteSelectedOverlayInstance() {
    if (!canDeleteSelectedOverlayInstance()) {
        return false;
    }
    HistoryState before = captureState();
    const std::string id = selectedObjectId_;
    const auto oldEnd = level_->objects.end();
    const auto newEnd = std::remove_if(level_->objects.begin(), level_->objects.end(), [&id](const WorldObject& object) {
        return object.id == id;
    });
    if (newEnd == oldEnd) {
        return false;
    }
    level_->objects.erase(newEnd, oldEnd);
    selectedObjectId_.clear();
    pushHistory(std::move(before));
    return true;
}

bool RuntimeMapEditor::canUndo() const {
    return level_ != nullptr && !undoStack_.empty();
}

bool RuntimeMapEditor::canRedo() const {
    return level_ != nullptr && !redoStack_.empty();
}

bool RuntimeMapEditor::undo() {
    if (!canUndo()) {
        return false;
    }
    HistoryEntry entry = std::move(undoStack_.back());
    undoStack_.pop_back();
    restoreState(entry.before);
    redoStack_.push_back(std::move(entry));
    dirty_ = true;
    ++revision_;
    return true;
}

bool RuntimeMapEditor::redo() {
    if (!canRedo()) {
        return false;
    }
    HistoryEntry entry = std::move(redoStack_.back());
    redoStack_.pop_back();
    restoreState(entry.after);
    undoStack_.push_back(std::move(entry));
    dirty_ = true;
    ++revision_;
    return true;
}

std::uint64_t RuntimeMapEditor::revision() const {
    return revision_;
}

EditorOverlayDocument RuntimeMapEditor::buildOverlayDocument() const {
    EditorOverlayDocument document;
    if (level_ == nullptr) {
        return document;
    }

    for (const std::string& id : editedBaseObjectIds_) {
        for (const WorldObject& object : level_->objects) {
            if (object.id == id) {
                document.overrides.push_back(toOverlayObject(object));
                break;
            }
        }
    }

    for (const WorldObject& object : level_->objects) {
        if (hasEditorPrefix(object.id)) {
            document.instances.push_back(toOverlayObject(object));
        }
    }
    return document;
}

bool RuntimeMapEditor::saveOverlay(const std::string& path, std::vector<std::string>& warnings) {
    if (!saveEditorOverlayFile(path, buildOverlayDocument(), warnings)) {
        return false;
    }
    clearDirty();
    return true;
}

bool RuntimeMapEditor::saveOverlay(const std::string& path,
                                   std::vector<std::string>& warnings,
                                   const WorldAssetRegistry& registry) {
    const EditorOverlayDocument document = buildOverlayDocument();
    if (!validateOverlayDocumentForSave(document, registry, warnings)) {
        return false;
    }
    if (!saveEditorOverlayFile(path, document, warnings)) {
        return false;
    }
    clearDirty();
    return true;
}

bool RuntimeMapEditor::dirty() const {
    return dirty_;
}

void RuntimeMapEditor::clearDirty() {
    dirty_ = false;
}

std::vector<WorldObject*> RuntimeMapEditor::objects() {
    std::vector<WorldObject*> result;
    if (level_ == nullptr) {
        return result;
    }
    result.reserve(level_->objects.size());
    for (WorldObject& object : level_->objects) {
        result.push_back(&object);
    }
    return result;
}

RuntimeMapEditor::HistoryState RuntimeMapEditor::captureState() const {
    HistoryState state;
    if (level_ != nullptr) {
        state.objects = level_->objects;
    }
    state.selectedObjectId = selectedObjectId_;
    state.editedBaseObjectIds = editedBaseObjectIds_;
    state.generatedInstanceCounter = generatedInstanceCounter_;
    return state;
}

void RuntimeMapEditor::restoreState(const HistoryState& state) {
    if (level_ != nullptr) {
        level_->objects = state.objects;
    }
    selectedObjectId_ = state.selectedObjectId;
    editedBaseObjectIds_ = state.editedBaseObjectIds;
    generatedInstanceCounter_ = state.generatedInstanceCounter;
}

void RuntimeMapEditor::pushHistory(HistoryState before) {
    undoStack_.push_back({std::move(before), captureState()});
    if (undoStack_.size() > RuntimeEditorHistoryLimit) {
        undoStack_.erase(undoStack_.begin());
    }
    redoStack_.clear();
    dirty_ = true;
    ++revision_;
}

bool RuntimeMapEditor::commitCapturedEdit(HistoryState before) {
    if (level_ == nullptr) {
        return false;
    }
    pushHistory(std::move(before));
    return true;
}

void RuntimeMapEditor::markSelectedBaseObjectEdited() {
    if (selectedObjectId_.empty() || hasEditorPrefix(selectedObjectId_)) {
        return;
    }
    if (std::find(editedBaseObjectIds_.begin(), editedBaseObjectIds_.end(), selectedObjectId_) !=
        editedBaseObjectIds_.end()) {
        return;
    }
    editedBaseObjectIds_.push_back(selectedObjectId_);
}

} // namespace bs3d
