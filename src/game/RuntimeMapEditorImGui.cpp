#include "RuntimeMapEditorImGui.h"

#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
#include "imgui.h"

#include <sstream>
#include <string>
#include <vector>
#endif

namespace bs3d {

namespace {

std::string joinTags(const std::vector<std::string>& tags) {
    std::string result;
    for (const std::string& tag : tags) {
        if (!result.empty()) {
            result += ", ";
        }
        result += tag;
    }
    return result;
}

std::string assetSummary(const WorldAssetDefinition& asset) {
    std::string summary = asset.id + " [" + asset.renderBucket + "/" + asset.defaultCollision + "]";
    const std::string tags = joinTags(asset.defaultTags);
    if (!tags.empty()) {
        summary += " tags: " + tags;
    }
    return summary;
}

const char* zoneTagName(WorldLocationTag tag) {
    switch (tag) {
    case WorldLocationTag::Unknown:   return "Unknown";
    case WorldLocationTag::Block:     return "Block";
    case WorldLocationTag::Shop:      return "Shop";
    case WorldLocationTag::Parking:   return "Parking";
    case WorldLocationTag::Garage:    return "Garage";
    case WorldLocationTag::Trash:     return "Trash";
    case WorldLocationTag::RoadLoop:  return "RoadLoop";
    }
    return "Unknown";
}

WorldLocationTag zoneTagFromIndex(int index) {
    switch (index) {
    case 0: return WorldLocationTag::Unknown;
    case 1: return WorldLocationTag::Block;
    case 2: return WorldLocationTag::Shop;
    case 3: return WorldLocationTag::Parking;
    case 4: return WorldLocationTag::Garage;
    case 5: return WorldLocationTag::Trash;
    case 6: return WorldLocationTag::RoadLoop;
    }
    return WorldLocationTag::Unknown;
}

int zoneTagIndex(WorldLocationTag tag) {
    switch (tag) {
    case WorldLocationTag::Unknown:  return 0;
    case WorldLocationTag::Block:    return 1;
    case WorldLocationTag::Shop:     return 2;
    case WorldLocationTag::Parking:  return 3;
    case WorldLocationTag::Garage:   return 4;
    case WorldLocationTag::Trash:    return 5;
    case WorldLocationTag::RoadLoop: return 6;
    }
    return 0;
}

std::vector<std::string> splitTags(const std::string& text) {
    std::vector<std::string> tags;
    std::istringstream stream(text);
    std::string token;
    while (std::getline(stream, token, ',')) {
        while (!token.empty() && token.front() == ' ') {
            token.erase(0, 1);
        }
        while (!token.empty() && token.back() == ' ') {
            token.pop_back();
        }
        if (!token.empty()) {
            tags.push_back(token);
        }
    }
    return tags;
}

} // namespace

bool RuntimeMapEditorImGui::showCollision() const {
    return showCollision_;
}

void RuntimeMapEditorImGui::draw(RuntimeMapEditor& editor,
                                 const WorldAssetRegistry& registry,
                                 Vec3 placementPosition,
                                 const std::string& overlayPath) {
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
    ImGui::Begin("Objects");
    static char filter[128] = "";
    ImGui::InputText("Filter", filter, sizeof(filter));
    for (WorldObject* object : editor.objects()) {
        const std::string label = object->id + " [" + object->assetId + "]";
        if (filter[0] != '\0' && label.find(filter) == std::string::npos) {
            continue;
        }
        if (ImGui::Selectable(label.c_str(), editor.selectedObjectId() == object->id)) {
            editor.selectObject(object->id);
        }
    }
    ImGui::End();

    ImGui::Begin("Inspector");
    if (WorldObject* selected = editor.selectedObject()) {
        float position[3] = {selected->position.x, selected->position.y, selected->position.z};
        if (ImGui::DragFloat3("Position", position, 0.05f)) {
            editor.setSelectedPosition({position[0], position[1], position[2]});
        }
        float scale[3] = {selected->scale.x, selected->scale.y, selected->scale.z};
        if (ImGui::DragFloat3("Scale", scale, 0.02f, 0.01f, 100.0f)) {
            editor.setSelectedScale({scale[0], scale[1], scale[2]});
        }
        float yaw = selected->yawRadians;
        if (ImGui::DragFloat("Yaw", &yaw, 0.01f)) {
            editor.setSelectedYaw(yaw);
        }
        ImGui::Separator();
        ImGui::Text("ID: %s", selected->id.c_str());

        // Editable Asset ID
        static char assetIdBuf[128] = "";
        if (assetIdBuf[0] == '\0' || editor.selectedObjectId() != lastInspectedId_) {
            const std::size_t len = selected->assetId.copy(assetIdBuf, sizeof(assetIdBuf) - 1);
            assetIdBuf[len] = '\0';
            lastInspectedId_ = editor.selectedObjectId();
        }
        if (ImGui::InputText("Asset", assetIdBuf, sizeof(assetIdBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            editor.setSelectedAssetId(assetIdBuf);
        }

        // Editable Zone Tag (combo)
        int zoneIdx = zoneTagIndex(selected->zoneTag);
        if (ImGui::Combo("Zone", &zoneIdx,
                         "Unknown\0Block\0Shop\0Parking\0Garage\0Trash\0RoadLoop\0")) {
            editor.setSelectedZoneTag(zoneTagFromIndex(zoneIdx));
        }

        // Editable Gameplay Tags
        static char tagsBuf[512] = "";
        if (tagsBuf[0] == '\0' || editor.selectedObjectId() != lastInspectedId_) {
            const std::string joined = joinTags(selected->gameplayTags);
            const std::size_t len = joined.copy(tagsBuf, sizeof(tagsBuf) - 1);
            tagsBuf[len] = '\0';
            lastInspectedId_ = editor.selectedObjectId();
        }
        if (ImGui::InputText("Tags", tagsBuf, sizeof(tagsBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            editor.setSelectedGameplayTags(splitTags(tagsBuf));
        }

        ImGui::Separator();
        ImGui::Text("Dirty: %s", editor.dirty() ? "yes" : "no");
        if (editor.canDeleteSelectedOverlayInstance()) {
            if (ImGui::Button("Delete Selected")) {
                if (editor.deleteSelectedOverlayInstance()) {
                    lastStatus_ = "Deleted selected editor instance";
                }
            }
        }
    } else {
        ImGui::Text("No object selected");
    }
    ImGui::End();

    ImGui::Begin("Blok 13 Runtime Editor");
    if (!editor.canUndo()) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Undo")) {
        if (editor.undo()) {
            lastStatus_ = "Undid last editor change";
        }
    }
    if (!editor.canUndo()) {
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    if (!editor.canRedo()) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Redo")) {
        if (editor.redo()) {
            lastStatus_ = "Redid editor change";
        }
    }
    if (!editor.canRedo()) {
        ImGui::EndDisabled();
    }
    if (ImGui::Button("Save Overlay")) {
        std::vector<std::string> warnings;
        if (editor.saveOverlay(overlayPath, warnings)) {
            lastStatus_ = "Saved " + overlayPath;
        } else {
            lastStatus_ = warnings.empty() ? "Save failed" : warnings.front();
        }
    }
    ImGui::Text("Assets: %d", static_cast<int>(registry.definitions().size()));
    ImGui::Text("Dirty: %s", editor.dirty() ? "yes" : "no");
    ImGui::Checkbox("Show Collision", &showCollision_);
    if (!lastStatus_.empty()) {
        ImGui::Text("%s", lastStatus_.c_str());
    }
    ImGui::End();

    ImGui::Begin("Assets");
    static char assetFilter[128] = "";
    ImGui::InputText("Asset Filter", assetFilter, sizeof(assetFilter));
    ImGui::Text("Place at: %.2f %.2f %.2f", placementPosition.x, placementPosition.y, placementPosition.z);
    for (const WorldAssetDefinition& asset : registry.definitions()) {
        if (!runtimeEditorAssetMatchesFilter(asset, assetFilter)) {
            continue;
        }
        const std::string label = assetSummary(asset);
        if (ImGui::Selectable(label.c_str())) {
            editor.addInstance(asset, placementPosition);
        }
        if (ImGui::IsItemHovered()) {
            const std::string tags = joinTags(asset.defaultTags);
            ImGui::SetTooltip("model: %s\norigin: %s\nrender: %s\ncollision: %s\ntags: %s",
                              asset.modelPath.c_str(),
                              asset.originPolicy.c_str(),
                              asset.renderBucket.c_str(),
                              asset.defaultCollision.c_str(),
                              tags.empty() ? "(none)" : tags.c_str());
        }
    }
    ImGui::End();
#else
    (void)editor;
    (void)registry;
    (void)placementPosition;
    (void)overlayPath;
#endif
}

} // namespace bs3d
