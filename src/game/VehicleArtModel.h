#pragma once

#include "bs3d/core/Types.h"

#include <array>
#include <string>
#include <vector>

namespace bs3d {

enum class VehicleArtShape {
    Box,
    CylinderX,
    QuadPanel
};

enum class VehicleArtMaterial {
    BodyPaint,
    DarkTrim,
    Glass,
    Headlight,
    Taillight,
    Grille,
    Rubber,
    Rim,
    Exhaust,
    Dirt,
    Rust,
    Primer,
    Collider
};

struct VehicleArtPart {
    std::string name;
    VehicleArtShape shape = VehicleArtShape::Box;
    VehicleArtMaterial material = VehicleArtMaterial::BodyPaint;
    Vec3 position{};
    Vec3 size{1.0f, 1.0f, 1.0f};
    std::array<Vec3, 4> panelCorners{};
    float yawRadians = 0.0f;
    float pitchRadians = 0.0f;
    float rollRadians = 0.0f;
    bool wheelPivot = false;
    bool frontWheel = false;
    bool tire = false;
    bool rim = false;
    bool collider = false;
    bool render = true;
};

struct VehicleArtModelSpec {
    std::string assetId;
    std::string modelPath;
    Vec3 bounds{};
    std::vector<VehicleArtPart> parts;
};

const VehicleArtModelSpec& vehicleArtModelSpec();
const VehicleArtPart* findVehicleArtPart(const VehicleArtModelSpec& spec, const std::string& name);

} // namespace bs3d
