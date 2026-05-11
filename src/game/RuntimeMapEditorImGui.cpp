#include "RuntimeMapEditorImGui.h"

#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
#include "imgui.h"

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

} // namespace

void RuntimeMapEditorImGui::draw(RuntimeMapEditor& editor, const WorldAssetRegistry& registry, Vec3 placementPosition) {
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
        ImGui::Text("ID: %s", selected->id.c_str());
        ImGui::Text("Asset: %s", selected->assetId.c_str());
        ImGui::Text("Zone: %d", static_cast<int>(selected->zoneTag));
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
        if (editor.saveOverlay("data/world/block13_editor_overlay.json", warnings)) {
            lastStatus_ = "Saved data/world/block13_editor_overlay.json";
        } else {
            lastStatus_ = warnings.empty() ? "Save failed" : warnings.front();
        }
    }
    ImGui::Text("Assets: %d", static_cast<int>(registry.definitions().size()));
    ImGui::Text("Dirty: %s", editor.dirty() ? "yes" : "no");
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
#endif
}

} // namespace bs3d
