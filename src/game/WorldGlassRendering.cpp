#include "WorldGlassRendering.h"

#include <algorithm>
#include <cmath>

namespace bs3d {

namespace {

bool hasTag(const WorldObject& object, const std::string& tag) {
    return std::find(object.gameplayTags.begin(), object.gameplayTags.end(), tag) != object.gameplayTags.end();
}

float distanceSquared(Vec3 a, Vec3 b) {
    const Vec3 d = a - b;
    return d.x * d.x + d.y * d.y + d.z * d.z;
}

unsigned char clampByte(int value) {
    return static_cast<unsigned char>(std::clamp(value, 0, 255));
}

} // namespace

bool isGlassAssetId(const std::string& assetId) {
    return assetId == "shop_window" ||
           assetId == "block_window_strip" ||
           assetId.find("window") != std::string::npos ||
           assetId.find("glass") != std::string::npos;
}

bool isWorldGlassObject(const WorldObject& object) {
    return hasTag(object, "glass_surface") || isGlassAssetId(object.assetId);
}

bool isVehicleGlassPart(const VehicleArtPart& part) {
    return part.material == VehicleArtMaterial::Glass;
}

float glassCrackAmountForImpact(float impactSpeed) {
    if (impactSpeed < 5.5f) {
        return 0.0f;
    }
    return std::clamp((impactSpeed - 5.5f) / 8.5f, 0.0f, 1.0f);
}

GlassVisualState worldGlassVisualState(const WorldObject& object, float nightAmount) {
    const float night = std::clamp(nightAmount, 0.0f, 1.0f);
    GlassVisualState state;
    state.tint = {clampByte(92 + static_cast<int>(night * 18.0f)),
                  clampByte(142 + static_cast<int>(night * 24.0f)),
                  clampByte(158 + static_cast<int>(night * 34.0f)),
                  clampByte(106 + static_cast<int>(night * 28.0f))};
    state.reflectionIntensity = 0.34f + night * 0.28f;
    state.crackAmount = hasTag(object, "glass_cracked") ? 0.38f : 0.0f;
    state.renderAfterOpaque = true;
    state.blocksUi = false;
    return state;
}

GlassVisualState vehicleGlassVisualState(float condition, float lastImpactSpeed) {
    GlassVisualState state;
    state.tint = {103, 151, 168, 130};
    state.reflectionIntensity = 0.42f;
    const float conditionCrack = std::clamp((72.0f - condition) / 42.0f, 0.0f, 0.72f);
    state.crackAmount = std::max(conditionCrack, glassCrackAmountForImpact(lastImpactSpeed));
    state.renderAfterOpaque = true;
    state.blocksUi = false;
    return state;
}

std::vector<const WorldObject*> sortedGlassObjectsBackToFront(const std::vector<WorldObject>& objects,
                                                             Vec3 cameraPosition) {
    std::vector<const WorldObject*> result;
    for (const WorldObject& object : objects) {
        if (isWorldGlassObject(object)) {
            result.push_back(&object);
        }
    }

    std::sort(result.begin(), result.end(), [cameraPosition](const WorldObject* lhs, const WorldObject* rhs) {
        return distanceSquared(lhs->position, cameraPosition) > distanceSquared(rhs->position, cameraPosition);
    });
    return result;
}

} // namespace bs3d
