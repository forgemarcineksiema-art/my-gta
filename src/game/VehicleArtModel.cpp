#include "VehicleArtModel.h"

#include <algorithm>
#include <utility>

namespace bs3d {

namespace {

VehicleArtPart part(std::string name,
                    VehicleArtShape shape,
                    VehicleArtMaterial material,
                    Vec3 position,
                    Vec3 size,
                    float yawRadians = 0.0f,
                    float pitchRadians = 0.0f,
                    float rollRadians = 0.0f) {
    VehicleArtPart result;
    result.name = std::move(name);
    result.shape = shape;
    result.material = material;
    result.position = position;
    result.size = size;
    result.yawRadians = yawRadians;
    result.pitchRadians = pitchRadians;
    result.rollRadians = rollRadians;
    return result;
}

VehicleArtPart quadPanel(std::string name,
                         VehicleArtMaterial material,
                         std::array<Vec3, 4> corners) {
    VehicleArtPart result;
    result.name = std::move(name);
    result.shape = VehicleArtShape::QuadPanel;
    result.material = material;
    result.panelCorners = corners;

    Vec3 minCorner = corners[0];
    Vec3 maxCorner = corners[0];
    Vec3 center{};
    for (const Vec3& corner : corners) {
        center = center + corner;
        minCorner.x = std::min(minCorner.x, corner.x);
        minCorner.y = std::min(minCorner.y, corner.y);
        minCorner.z = std::min(minCorner.z, corner.z);
        maxCorner.x = std::max(maxCorner.x, corner.x);
        maxCorner.y = std::max(maxCorner.y, corner.y);
        maxCorner.z = std::max(maxCorner.z, corner.z);
    }

    result.position = center * 0.25f;
    result.size = {maxCorner.x - minCorner.x, maxCorner.y - minCorner.y, maxCorner.z - minCorner.z};
    return result;
}

VehicleArtPart wheelPivot(std::string name, Vec3 position, bool frontWheel) {
    VehicleArtPart result = part(std::move(name),
                                 VehicleArtShape::CylinderX,
                                 VehicleArtMaterial::Rubber,
                                 position,
                                 {0.24f, 0.58f, 0.58f});
    result.wheelPivot = true;
    result.frontWheel = frontWheel;
    result.render = false;
    return result;
}

VehicleArtPart tire(std::string name, Vec3 position, bool frontWheel) {
    VehicleArtPart result = part(std::move(name),
                                 VehicleArtShape::CylinderX,
                                 VehicleArtMaterial::Rubber,
                                 position,
                                 {0.24f, 0.58f, 0.58f});
    result.tire = true;
    result.frontWheel = frontWheel;
    return result;
}

VehicleArtPart rim(std::string name, Vec3 position, bool frontWheel) {
    VehicleArtPart result = part(std::move(name),
                                 VehicleArtShape::CylinderX,
                                 VehicleArtMaterial::Rim,
                                 position,
                                 {0.27f, 0.34f, 0.34f});
    result.rim = true;
    result.frontWheel = frontWheel;
    return result;
}

} // namespace

const VehicleArtModelSpec& vehicleArtModelSpec() {
    static const VehicleArtModelSpec spec{
        "vehicle_gruz_e36",
        "models/gruz_e36.gltf",
        {1.92f, 1.32f, 3.56f},
        {
            part("Car_Body", VehicleArtShape::Box, VehicleArtMaterial::BodyPaint, {0.0f, 0.52f, 0.0f}, {1.76f, 0.58f, 2.70f}),
            part("Hood", VehicleArtShape::Box, VehicleArtMaterial::BodyPaint, {0.0f, 0.82f, 0.82f}, {1.50f, 0.070f, 1.12f}, 0.0f, 0.05f),
            quadPanel("Roof",
                      VehicleArtMaterial::BodyPaint,
                      {{{-0.59f, 1.19f, 0.04f},
                        {0.59f, 1.19f, 0.04f},
                        {0.53f, 1.17f, -0.84f},
                        {-0.53f, 1.17f, -0.84f}}}),
            part("Trunk", VehicleArtShape::Box, VehicleArtMaterial::BodyPaint, {0.0f, 0.82f, -1.18f}, {1.48f, 0.055f, 0.72f}, 0.0f, -0.02f),
            part("Front_Bumper", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {0.0f, 0.43f, 1.55f}, {1.80f, 0.22f, 0.22f}),
            part("Rear_Bumper", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {0.0f, 0.43f, -1.54f}, {1.80f, 0.22f, 0.22f}),
            part("Fender_FL", VehicleArtShape::Box, VehicleArtMaterial::BodyPaint, {-0.82f, 0.58f, 1.04f}, {0.15f, 0.30f, 0.74f}),
            part("Fender_FR", VehicleArtShape::Box, VehicleArtMaterial::BodyPaint, {0.82f, 0.58f, 1.04f}, {0.15f, 0.30f, 0.74f}),
            part("Fender_RL", VehicleArtShape::Box, VehicleArtMaterial::BodyPaint, {-0.82f, 0.58f, -1.06f}, {0.15f, 0.30f, 0.74f}),
            part("Fender_RR", VehicleArtShape::Box, VehicleArtMaterial::BodyPaint, {0.82f, 0.58f, -1.06f}, {0.15f, 0.30f, 0.74f}),
            part("Door_L", VehicleArtShape::Box, VehicleArtMaterial::BodyPaint, {-0.91f, 0.68f, -0.12f}, {0.065f, 0.48f, 0.96f}),
            part("Door_R", VehicleArtShape::Box, VehicleArtMaterial::BodyPaint, {0.91f, 0.68f, -0.12f}, {0.065f, 0.48f, 0.96f}),
            quadPanel("Windshield",
                      VehicleArtMaterial::Glass,
                      {{{-0.55f, 1.145f, 0.06f},
                        {0.55f, 1.145f, 0.06f},
                        {0.70f, 0.87f, 0.66f},
                        {-0.70f, 0.87f, 0.66f}}}),
            quadPanel("Rear_Window",
                      VehicleArtMaterial::Glass,
                      {{{-0.51f, 1.125f, -0.84f},
                        {0.51f, 1.125f, -0.84f},
                        {0.68f, 0.88f, -1.18f},
                        {-0.68f, 0.88f, -1.18f}}}),
            quadPanel("Window_L",
                      VehicleArtMaterial::Glass,
                      {{{-0.62f, 1.155f, 0.03f},
                        {-0.57f, 1.140f, -0.83f},
                        {-0.76f, 0.82f, -0.98f},
                        {-0.78f, 0.82f, 0.48f}}}),
            quadPanel("Window_R",
                      VehicleArtMaterial::Glass,
                      {{{0.62f, 1.155f, 0.03f},
                        {0.57f, 1.140f, -0.83f},
                        {0.76f, 0.82f, -0.98f},
                        {0.78f, 0.82f, 0.48f}}}),
            part("Pillar_A_L", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {-0.84f, 1.00f, 0.34f}, {0.070f, 0.34f, 0.085f}, 0.0f, 0.38f),
            part("Pillar_A_R", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {0.84f, 1.00f, 0.34f}, {0.070f, 0.34f, 0.085f}, 0.0f, 0.38f),
            part("Pillar_B_L", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {-0.84f, 1.00f, -0.27f}, {0.070f, 0.36f, 0.075f}),
            part("Pillar_B_R", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {0.84f, 1.00f, -0.27f}, {0.070f, 0.36f, 0.075f}),
            part("Pillar_C_L", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {-0.84f, 0.99f, -0.78f}, {0.075f, 0.33f, 0.090f}, 0.0f, -0.32f),
            part("Pillar_C_R", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {0.84f, 0.99f, -0.78f}, {0.075f, 0.33f, 0.090f}, 0.0f, -0.32f),
            part("Cabin_Sill_L", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {-0.88f, 0.82f, -0.25f}, {0.045f, 0.060f, 1.16f}),
            part("Cabin_Sill_R", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {0.88f, 0.82f, -0.25f}, {0.045f, 0.060f, 1.16f}),
            part("Roof_Rail_L", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {-0.63f, 1.155f, -0.38f}, {0.075f, 0.055f, 0.95f}),
            part("Roof_Rail_R", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {0.63f, 1.155f, -0.38f}, {0.075f, 0.055f, 0.95f}),
            part("Windshield_Header", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {0.0f, 1.145f, 0.05f}, {1.20f, 0.055f, 0.070f}),
            part("Rear_Window_Header", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {0.0f, 1.125f, -0.84f}, {1.08f, 0.055f, 0.070f}),
            part("Headlight_L", VehicleArtShape::Box, VehicleArtMaterial::Headlight, {-0.42f, 0.57f, 1.66f}, {0.34f, 0.11f, 0.065f}),
            part("Headlight_R", VehicleArtShape::Box, VehicleArtMaterial::Headlight, {0.42f, 0.57f, 1.66f}, {0.34f, 0.11f, 0.065f}),
            part("Taillight_L", VehicleArtShape::Box, VehicleArtMaterial::Taillight, {-0.50f, 0.56f, -1.66f}, {0.28f, 0.11f, 0.065f}),
            part("Taillight_R", VehicleArtShape::Box, VehicleArtMaterial::Taillight, {0.50f, 0.56f, -1.66f}, {0.28f, 0.11f, 0.065f}),
            part("Grille", VehicleArtShape::Box, VehicleArtMaterial::Grille, {0.0f, 0.57f, 1.68f}, {0.36f, 0.12f, 0.055f}),
            part("Mirror_L", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {-1.00f, 0.91f, 0.30f}, {0.14f, 0.10f, 0.20f}),
            part("Mirror_R", VehicleArtShape::Box, VehicleArtMaterial::DarkTrim, {1.00f, 0.91f, 0.30f}, {0.14f, 0.10f, 0.20f}),
            part("Exhaust", VehicleArtShape::CylinderX, VehicleArtMaterial::Exhaust, {-0.54f, 0.29f, -1.73f}, {0.30f, 0.08f, 0.08f}),
            part("Dirt_Rocker_L", VehicleArtShape::Box, VehicleArtMaterial::Dirt, {-0.946f, 0.39f, -0.10f}, {0.034f, 0.13f, 1.00f}),
            part("Dirt_Rocker_R", VehicleArtShape::Box, VehicleArtMaterial::Dirt, {0.946f, 0.39f, -0.10f}, {0.034f, 0.13f, 1.00f}),
            part("Rust_Fender_FL", VehicleArtShape::Box, VehicleArtMaterial::Rust, {-0.905f, 0.53f, 1.28f}, {0.030f, 0.16f, 0.22f}),
            part("Rust_Fender_RR", VehicleArtShape::Box, VehicleArtMaterial::Rust, {0.905f, 0.51f, -1.24f}, {0.030f, 0.17f, 0.24f}),
            part("Rust_Door_L", VehicleArtShape::Box, VehicleArtMaterial::Rust, {-0.948f, 0.54f, -0.22f}, {0.030f, 0.15f, 0.30f}),
            part("Primer_Door_R", VehicleArtShape::Box, VehicleArtMaterial::Primer, {0.948f, 0.73f, -0.08f}, {0.032f, 0.22f, 0.42f}),
            part("Hood_Chipped_Edge", VehicleArtShape::Box, VehicleArtMaterial::Dirt, {0.0f, 0.865f, 1.38f}, {1.16f, 0.026f, 0.070f}),
            part("Trunk_Dust_Strip", VehicleArtShape::Box, VehicleArtMaterial::Dirt, {0.0f, 0.852f, -1.48f}, {1.20f, 0.024f, 0.060f}),
            part("Bumper_Scuff_Front", VehicleArtShape::Box, VehicleArtMaterial::Dirt, {-0.46f, 0.45f, 1.675f}, {0.48f, 0.065f, 0.030f}),
            part("Bumper_Scuff_Rear", VehicleArtShape::Box, VehicleArtMaterial::Dirt, {0.52f, 0.43f, -1.675f}, {0.50f, 0.065f, 0.030f}),
            wheelPivot("Wheel_FL", {-0.98f, 0.32f, 1.05f}, true),
            wheelPivot("Wheel_FR", {0.98f, 0.32f, 1.05f}, true),
            wheelPivot("Wheel_RL", {-0.98f, 0.32f, -1.05f}, false),
            wheelPivot("Wheel_RR", {0.98f, 0.32f, -1.05f}, false),
            rim("Rim_FL", {-0.98f, 0.32f, 1.05f}, true),
            rim("Rim_FR", {0.98f, 0.32f, 1.05f}, true),
            rim("Rim_RL", {-0.98f, 0.32f, -1.05f}, false),
            rim("Rim_RR", {0.98f, 0.32f, -1.05f}, false),
            tire("Tire_FL", {-0.98f, 0.32f, 1.05f}, true),
            tire("Tire_FR", {0.98f, 0.32f, 1.05f}, true),
            tire("Tire_RL", {-0.98f, 0.32f, -1.05f}, false),
            tire("Tire_RR", {0.98f, 0.32f, -1.05f}, false),
            [] {
                VehicleArtPart collider = part("Collider_Body", VehicleArtShape::Box, VehicleArtMaterial::Collider, {0.0f, 0.62f, 0.0f}, {1.90f, 1.18f, 3.38f});
                collider.collider = true;
                collider.render = false;
                return collider;
            }(),
            [] {
                VehicleArtPart collider = part("Collider_Wheels", VehicleArtShape::Box, VehicleArtMaterial::Collider, {0.0f, 0.35f, 0.0f}, {2.05f, 0.72f, 2.70f});
                collider.collider = true;
                collider.render = false;
                return collider;
            }()
        }};
    return spec;
}

const VehicleArtPart* findVehicleArtPart(const VehicleArtModelSpec& spec, const std::string& name) {
    for (const VehicleArtPart& part : spec.parts) {
        if (part.name == name) {
            return &part;
        }
    }
    return nullptr;
}

} // namespace bs3d
