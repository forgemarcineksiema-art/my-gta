#pragma once

#include "WorldArtTypes.h"

#include <string>
#include <vector>

namespace bs3d {

struct EditorOverlayObject {
    std::string id;
    std::string assetId;
    Vec3 position{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
    float yawRadians = 0.0f;
    WorldLocationTag zoneTag = WorldLocationTag::Unknown;
    std::vector<std::string> gameplayTags;
};

struct EditorOverlayDocument {
    int schemaVersion = 1;
    std::vector<EditorOverlayObject> overrides;
    std::vector<EditorOverlayObject> instances;
};

} // namespace bs3d
