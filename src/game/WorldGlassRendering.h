#pragma once

#include "VehicleArtModel.h"
#include "WorldArtTypes.h"

#include <string>
#include <vector>

namespace bs3d {

struct GlassVisualState {
    WorldObjectTint tint{104, 154, 172, 118};
    float reflectionIntensity = 0.35f;
    float crackAmount = 0.0f;
    bool renderAfterOpaque = true;
    bool blocksUi = false;
};

bool isGlassAssetId(const std::string& assetId);
bool isWorldGlassObject(const WorldObject& object);
bool isVehicleGlassPart(const VehicleArtPart& part);
float glassCrackAmountForImpact(float impactSpeed);
GlassVisualState worldGlassVisualState(const WorldObject& object, float nightAmount = 0.0f);
GlassVisualState vehicleGlassVisualState(float condition, float lastImpactSpeed = 0.0f);
std::vector<const WorldObject*> sortedGlassObjectsBackToFront(const std::vector<WorldObject>& objects,
                                                             Vec3 cameraPosition);

} // namespace bs3d
