#pragma once

#include "EditorOverlayData.h"
#include "IntroLevelBuilder.h"

#include <string>
#include <vector>

namespace bs3d {

struct WorldAssetDefinition;

Vec3 runtimeEditorPlacementPosition(Vec3 anchor, float yawRadians, float distanceMeters = 3.0f);
bool runtimeEditorAssetMatchesFilter(const WorldAssetDefinition& asset, const std::string& filter);

class RuntimeMapEditor {
public:
    void attach(IntroLevelData& level);
    void attach(IntroLevelData& level, const EditorOverlayDocument& loadedOverlay);
    bool selectObject(const std::string& id);
    const std::string& selectedObjectId() const;
    WorldObject* selectedObject();
    const WorldObject* selectedObject() const;
    bool setSelectedPosition(Vec3 position);
    bool setSelectedScale(Vec3 scale);
    bool setSelectedYaw(float yawRadians);
    bool setSelectedAssetId(const std::string& assetId);
    bool setSelectedZoneTag(WorldLocationTag zoneTag);
    bool setSelectedGameplayTags(std::vector<std::string> tags);
    bool addInstance(const std::string& assetId, Vec3 position);
    bool addInstance(const WorldAssetDefinition& asset, Vec3 position);
    bool canDeleteSelectedOverlayInstance() const;
    bool deleteSelectedOverlayInstance();
    bool canUndo() const;
    bool canRedo() const;
    bool undo();
    bool redo();
    EditorOverlayDocument buildOverlayDocument() const;
    bool saveOverlay(const std::string& path, std::vector<std::string>& warnings);
    bool dirty() const;
    void clearDirty();
    std::vector<WorldObject*> objects();

private:
    struct HistoryState {
        std::vector<WorldObject> objects;
        std::string selectedObjectId;
        std::vector<std::string> editedBaseObjectIds;
        int generatedInstanceCounter = 0;
    };

    struct HistoryEntry {
        HistoryState before;
        HistoryState after;
    };

    HistoryState captureState() const;
    void restoreState(const HistoryState& state);
    void pushHistory(HistoryState before);
    void markSelectedBaseObjectEdited();

    IntroLevelData* level_ = nullptr;
    std::string selectedObjectId_;
    std::vector<std::string> editedBaseObjectIds_;
    std::vector<HistoryEntry> undoStack_;
    std::vector<HistoryEntry> redoStack_;
    bool dirty_ = false;
    int generatedInstanceCounter_ = 0;
};

} // namespace bs3d
