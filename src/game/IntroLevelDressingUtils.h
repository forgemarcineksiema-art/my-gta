#pragma once

#include "IntroLevelBuilder.h"

#include <string>
#include <utility>
#include <vector>

namespace bs3d::IntroLevelDressingUtils {

inline WorldObjectTint objectTint(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
    return {r, g, b, a};
}

inline WorldCollisionShape noCollision() {
    return {WorldCollisionShapeKind::None, {}, {}};
}

inline void addDecor(IntroLevelData& level,
                     std::string id,
                     std::string assetId,
                     Vec3 position,
                     Vec3 scale,
                     WorldLocationTag tag,
                     std::vector<std::string> gameplayTags,
                     float yawRadians = 0.0f) {
    bool hasCameraSafeTag = false;
    for (const std::string& tagName : gameplayTags) {
        if (tagName == "camera_safe") {
            hasCameraSafeTag = true;
            break;
        }
    }
    if (!hasCameraSafeTag) {
        gameplayTags.push_back("camera_safe");
    }
    level.objects.push_back({std::move(id),
                             std::move(assetId),
                             position,
                             scale,
                             yawRadians,
                             noCollision(),
                             tag,
                             std::move(gameplayTags)});
}

inline void addTintedDecor(IntroLevelData& level,
                           std::string id,
                           std::string assetId,
                           Vec3 position,
                           Vec3 scale,
                           WorldLocationTag tag,
                           std::vector<std::string> gameplayTags,
                           WorldObjectTint tintOverride,
                           float yawRadians = 0.0f) {
    bool hasCameraSafeTag = false;
    for (const std::string& tagName : gameplayTags) {
        if (tagName == "camera_safe") {
            hasCameraSafeTag = true;
            break;
        }
    }
    if (!hasCameraSafeTag) {
        gameplayTags.push_back("camera_safe");
    }
    level.objects.push_back({std::move(id),
                             std::move(assetId),
                             position,
                             scale,
                             yawRadians,
                             noCollision(),
                             tag,
                             std::move(gameplayTags),
                             true,
                             tintOverride});
}

} // namespace bs3d::IntroLevelDressingUtils
