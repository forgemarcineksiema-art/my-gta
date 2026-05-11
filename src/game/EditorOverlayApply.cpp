#include "EditorOverlayApply.h"

#include <unordered_set>

namespace bs3d {

namespace {

WorldObject toWorldObject(const EditorOverlayObject& object) {
    WorldObject world;
    world.id = object.id;
    world.assetId = object.assetId;
    world.position = object.position;
    world.scale = object.scale;
    world.yawRadians = object.yawRadians;
    world.zoneTag = object.zoneTag;
    world.gameplayTags = object.gameplayTags;
    world.collision.kind = WorldCollisionShapeKind::None;
    return world;
}

} // namespace

EditorOverlayApplyResult applyEditorOverlay(IntroLevelData& level,
                                            const EditorOverlayDocument& overlay) {
    EditorOverlayApplyResult result;
    std::unordered_set<std::string> ids;
    ids.reserve(level.objects.size() + overlay.instances.size());

    for (WorldObject& object : level.objects) {
        ids.insert(object.id);
        for (const EditorOverlayObject& overrideObject : overlay.overrides) {
            if (overrideObject.id != object.id) {
                continue;
            }
            object.assetId = overrideObject.assetId;
            object.position = overrideObject.position;
            object.scale = overrideObject.scale;
            object.yawRadians = overrideObject.yawRadians;
            object.zoneTag = overrideObject.zoneTag;
            object.gameplayTags = overrideObject.gameplayTags;
            ++result.appliedOverrides;
        }
    }

    for (const EditorOverlayObject& overrideObject : overlay.overrides) {
        if (ids.find(overrideObject.id) == ids.end()) {
            result.warnings.push_back("overlay override target not found: " + overrideObject.id);
        }
    }

    for (const EditorOverlayObject& instance : overlay.instances) {
        if (!ids.insert(instance.id).second) {
            result.warnings.push_back("overlay instance id collides: " + instance.id);
            continue;
        }
        level.objects.push_back(toWorldObject(instance));
        ++result.appliedInstances;
    }

    return result;
}

} // namespace bs3d
