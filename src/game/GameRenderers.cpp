#include "GameRenderers.h"

#include "CharacterArtModel.h"
#include "CharacterDebugGeometry.h"
#include "VehicleArtModel.h"
#include "WorldGlassRendering.h"

#include "raylib.h"
#include "rlgl.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <vector>

namespace bs3d {

namespace {

Vector3 toRay(Vec3 value) {
    return {value.x, value.y, value.z};
}

float radiansToDegrees(float radians) {
    return radians * 180.0f / Pi;
}

Color color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
    return {r, g, b, a};
}

RenderColor renderColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
    return {r, g, b, a};
}

Color toColor(WorldObjectTint tint) {
    return {tint.r, tint.g, tint.b, tint.a};
}

Color toColor(RenderColor tint) {
    return {tint.r, tint.g, tint.b, tint.a};
}

RenderColor toRenderColor(Color tint) {
    return {tint.r, tint.g, tint.b, tint.a};
}

RenderColor toRenderColor(WorldObjectTint tint) {
    return {tint.r, tint.g, tint.b, tint.a};
}

Color toColor(CharacterArtColor tint) {
    return {tint.r, tint.g, tint.b, tint.a};
}

Color withAlpha(Color value, unsigned char alpha) {
    value.a = alpha;
    return value;
}

RenderColor withAlpha(RenderColor value, unsigned char alpha) {
    value.a = alpha;
    return value;
}

unsigned char scaleChannel(unsigned char channel, float factor) {
    return static_cast<unsigned char>(
        std::clamp(std::lround(static_cast<float>(channel) * factor), 0l, 255l));
}

Color scaleRgb(Color value, float factor) {
    return {scaleChannel(value.r, factor),
            scaleChannel(value.g, factor),
            scaleChannel(value.b, factor),
            value.a};
}

Vec3 lerp(Vec3 from, Vec3 to, float amount) {
    return from + (to - from) * amount;
}

float length(Vec3 value) {
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

Vec3 normalized(Vec3 value) {
    const float magnitude = length(value);
    if (magnitude <= 0.0001f) {
        return {};
    }
    return {value.x / magnitude, value.y / magnitude, value.z / magnitude};
}

Vec3 rotateY(Vec3 value, float yawRadians) {
    const float c = std::cos(yawRadians);
    const float s = std::sin(yawRadians);
    return {value.x * c + value.z * s, value.y, -value.x * s + value.z * c};
}

Vec3 transformModelPoint(Vec3 local, const WorldObject& object, Vec3 renderPosition) {
    return renderPosition + rotateY({local.x * object.scale.x, local.y * object.scale.y, local.z * object.scale.z},
                                    object.yawRadians);
}

Color vehicleMaterialColor(VehicleArtMaterial material,
                           Color bodyColor,
                           const VehicleLightVisualState* lights = nullptr) {
    switch (material) {
    case VehicleArtMaterial::BodyPaint:
        return bodyColor;
    case VehicleArtMaterial::DarkTrim:
        return color(31, 31, 29);
    case VehicleArtMaterial::Glass:
        return color(105, 149, 162, 142);
    case VehicleArtMaterial::Headlight:
        if (lights != nullptr) {
            return toColor(lights->headlightColor);
        }
        return color(255, 236, 128);
    case VehicleArtMaterial::Taillight:
        if (lights != nullptr) {
            return toColor(lights->taillightColor);
        }
        return color(220, 38, 42);
    case VehicleArtMaterial::Grille:
        return color(14, 15, 16);
    case VehicleArtMaterial::Rubber:
        return color(23, 23, 22);
    case VehicleArtMaterial::Rim:
        return color(158, 160, 148);
    case VehicleArtMaterial::Exhaust:
        return color(46, 45, 43);
    case VehicleArtMaterial::Dirt:
        return color(86, 73, 56);
    case VehicleArtMaterial::Rust:
        return color(139, 73, 38);
    case VehicleArtMaterial::Primer:
        return color(93, 97, 92);
    case VehicleArtMaterial::Collider:
        return color(70, 225, 115, 45);
    }
    return bodyColor;
}

Color characterMaterialColor(CharacterArtMaterial material, const CharacterArtPalette& palette) {
    return toColor(characterArtColorForMaterial(material, palette));
}

bool isAnimatedWheelPart(const VehicleArtPart& part) {
    return part.wheelPivot || part.tire || part.rim || part.collider;
}

float characterPartPitch(const CharacterArtPart& part, const PlayerPresentationState& pose, float elapsedSeconds) {
    const float panic = pose.panicFlail * std::sin(elapsedSeconds * 24.0f) * 0.50f;
    const float talk = pose.talkGesture * std::sin(elapsedSeconds * 8.0f) * 0.24f;
    const float interact = pose.interactReach * 0.72f;
    const float sit = pose.vehicleSitAmount;
    const float steer = pose.steerAmount * 0.28f;
    const float balance = pose.balanceAmount * 0.54f;
    const float actionWave = pose.actionAmount * std::sin(elapsedSeconds * 18.0f) * 0.24f;
    const float start = pose.startMoveAmount;
    const float stop = pose.stopMoveAmount;
    const float turn = pose.quickTurnAmount;
    const float punch = pose.punchAmount;
    const float dodge = pose.dodgeAmount;
    const float carry = pose.carryAmount;
    const float push = pose.pushAmount;
    const float drink = pose.drinkAmount;
    const float smoke = pose.smokeAmount;
    const float fall = pose.fallAmount;
    const float getUp = pose.getUpAmount;
    const float vault = pose.lowVaultAmount;
    const float jumpOff = pose.jumpOffVehicleAmount;
    const float tired = pose.tiredAmount;
    const float bruised = pose.bruisedAmount;
    switch (part.role) {
    case CharacterPartRole::LeftArm:
        return pose.leftArmSwing * 0.55f + panic + talk - interact * 0.25f -
               sit * (0.82f - steer) - balance + actionWave * 0.35f -
               start * 0.36f + stop * 0.42f + turn * 0.72f -
               carry * 0.98f - push * 1.05f + dodge * 0.46f + fall * 0.82f -
               getUp * 0.36f - vault * 0.58f - jumpOff * 0.86f +
               tired * 0.26f + bruised * 0.18f;
    case CharacterPartRole::RightArm:
        return pose.rightArmSwing * 0.55f - panic - talk - interact -
               sit * (0.82f + steer) - balance - pose.actionAmount * 0.85f -
               start * 0.62f + stop * 0.20f - turn * 0.72f -
               punch * 1.28f - carry * 0.98f - push * 1.10f -
               drink * 1.50f - smoke * 1.22f - dodge * 0.22f + fall * 0.90f -
               getUp * 0.48f - vault * 0.66f - jumpOff * 0.72f +
               tired * 0.18f + bruised * 0.34f;
    case CharacterPartRole::LeftLeg:
    case CharacterPartRole::LeftFoot:
        return pose.leftLegSwing * 0.42f - pose.jumpStretch * 0.22f + sit * 1.05f +
               start * 0.28f - stop * 0.18f + turn * 0.36f +
               dodge * 0.26f + fall * 0.58f - getUp * 0.20f + vault * 0.72f +
               jumpOff * 0.38f + bruised * 0.20f;
    case CharacterPartRole::RightLeg:
    case CharacterPartRole::RightFoot:
        return pose.rightLegSwing * 0.42f - pose.jumpStretch * 0.22f + sit * 1.05f +
               start * 0.10f + stop * 0.30f - turn * 0.36f -
               dodge * 0.18f + fall * 0.46f - getUp * 0.28f + vault * 0.38f -
               jumpOff * 0.22f - bruised * 0.12f;
    case CharacterPartRole::Torso:
        return start * -0.10f + stop * 0.12f + turn * 0.08f -
               punch * 0.08f + dodge * 0.18f + push * -0.14f +
               fall * 0.58f - getUp * 0.22f - vault * 0.16f - jumpOff * 0.18f +
               tired * 0.12f + bruised * 0.08f;
    case CharacterPartRole::Head:
        return drink * -0.18f + smoke * -0.08f + fall * 0.28f - getUp * 0.12f -
               jumpOff * 0.10f + tired * 0.10f + bruised * 0.06f;
    case CharacterPartRole::Static:
        return 0.0f;
    }
    return 0.0f;
}

Vec3 characterPartOffset(const CharacterArtPart& part, const PlayerPresentationState& pose) {
    Vec3 offset{};
    if (pose.vehicleSitAmount > 0.001f) {
        const float sit = pose.vehicleSitAmount;
        switch (part.role) {
        case CharacterPartRole::Torso:
            offset.y -= 0.46f * sit;
            offset.z -= 0.02f * sit;
            break;
        case CharacterPartRole::Head:
            offset.y -= 0.44f * sit;
            offset.z -= 0.02f * sit;
            break;
        case CharacterPartRole::LeftArm:
        case CharacterPartRole::RightArm:
            offset.y -= 0.38f * sit;
            offset.z += 0.22f * sit;
            break;
        case CharacterPartRole::LeftLeg:
        case CharacterPartRole::RightLeg:
            offset.y -= 0.20f * sit;
            offset.z += 0.28f * sit;
            break;
        case CharacterPartRole::LeftFoot:
        case CharacterPartRole::RightFoot:
            offset.y += 0.12f * sit;
            offset.z += 0.46f * sit;
            break;
        case CharacterPartRole::Static:
            break;
        }
    }
    if (pose.balanceAmount > 0.001f &&
        (part.role == CharacterPartRole::LeftArm || part.role == CharacterPartRole::RightArm)) {
        offset.y += 0.08f * pose.balanceAmount;
    }
    if (pose.actionAmount > 0.001f) {
        if (part.role == CharacterPartRole::RightArm) {
            offset.z += 0.12f * pose.actionAmount;
            offset.y += 0.04f * pose.actionAmount;
        }
        if (part.role == CharacterPartRole::Torso) {
            offset.z += 0.03f * pose.actionAmount;
        }
    }
    if (pose.startMoveAmount > 0.001f) {
        if (part.role == CharacterPartRole::Torso || part.role == CharacterPartRole::Head) {
            offset.z += 0.055f * pose.startMoveAmount;
            offset.y -= 0.018f * pose.startMoveAmount;
        }
    }
    if (pose.stopMoveAmount > 0.001f) {
        if (part.role == CharacterPartRole::Torso || part.role == CharacterPartRole::Head) {
            offset.z -= 0.065f * pose.stopMoveAmount;
            offset.y -= 0.012f * pose.stopMoveAmount;
        }
        if (part.role == CharacterPartRole::LeftFoot || part.role == CharacterPartRole::RightFoot) {
            offset.z += 0.06f * pose.stopMoveAmount;
        }
    }
    if (pose.quickTurnAmount > 0.001f) {
        if (part.role == CharacterPartRole::Torso || part.role == CharacterPartRole::Head) {
            offset.x += 0.045f * pose.quickTurnAmount;
            offset.y -= 0.016f * pose.quickTurnAmount;
        }
        if (part.role == CharacterPartRole::LeftArm) {
            offset.x -= 0.055f * pose.quickTurnAmount;
        }
        if (part.role == CharacterPartRole::RightArm) {
            offset.x += 0.055f * pose.quickTurnAmount;
        }
    }
    if (pose.punchAmount > 0.001f) {
        if (part.role == CharacterPartRole::RightArm) {
            offset.z += 0.18f * pose.punchAmount;
            offset.x -= 0.03f * pose.punchAmount;
        }
        if (part.role == CharacterPartRole::Torso || part.role == CharacterPartRole::Head) {
            offset.z += 0.035f * pose.punchAmount;
        }
    }
    if (pose.dodgeAmount > 0.001f) {
        if (part.role == CharacterPartRole::Torso || part.role == CharacterPartRole::Head) {
            offset.x -= 0.16f * pose.dodgeAmount;
            offset.y -= 0.055f * pose.dodgeAmount;
        }
        if (part.role == CharacterPartRole::LeftFoot || part.role == CharacterPartRole::RightFoot) {
            offset.x -= 0.10f * pose.dodgeAmount;
        }
    }
    if (pose.carryAmount > 0.001f || pose.pushAmount > 0.001f) {
        const float hold = std::max(pose.carryAmount, pose.pushAmount);
        if (part.role == CharacterPartRole::LeftArm) {
            offset.x -= 0.08f * hold;
            offset.z += 0.20f * hold;
            offset.y += 0.04f * hold;
        }
        if (part.role == CharacterPartRole::RightArm) {
            offset.x += 0.08f * hold;
            offset.z += 0.20f * hold;
            offset.y += 0.04f * hold;
        }
        if (part.role == CharacterPartRole::Torso) {
            offset.z += 0.035f * pose.pushAmount;
        }
    }
    if (pose.drinkAmount > 0.001f || pose.smokeAmount > 0.001f) {
        const float handToMouth = std::max(pose.drinkAmount, pose.smokeAmount);
        if (part.role == CharacterPartRole::RightArm) {
            offset.y += 0.18f * handToMouth;
            offset.z += 0.10f * handToMouth;
            offset.x -= 0.05f * pose.smokeAmount;
        }
        if (part.role == CharacterPartRole::Head) {
            offset.y += 0.015f * pose.drinkAmount;
            offset.x -= 0.018f * pose.smokeAmount;
        }
    }
    if (pose.fallAmount > 0.001f || pose.getUpAmount > 0.001f) {
        const float low = pose.fallAmount * 0.30f - pose.getUpAmount * 0.10f;
        if (part.role == CharacterPartRole::Torso || part.role == CharacterPartRole::Head) {
            offset.y -= low;
            offset.z -= 0.10f * pose.fallAmount;
        }
        if (part.role == CharacterPartRole::LeftArm || part.role == CharacterPartRole::RightArm) {
            offset.y -= 0.16f * pose.fallAmount;
            offset.z += 0.12f * pose.getUpAmount;
        }
        if (part.role == CharacterPartRole::LeftLeg || part.role == CharacterPartRole::RightLeg ||
            part.role == CharacterPartRole::LeftFoot || part.role == CharacterPartRole::RightFoot) {
            offset.y -= 0.08f * pose.fallAmount;
            offset.z += 0.05f * pose.getUpAmount;
        }
    }
    if (pose.lowVaultAmount > 0.001f) {
        if (part.role == CharacterPartRole::Torso || part.role == CharacterPartRole::Head) {
            offset.y += 0.045f * pose.lowVaultAmount;
            offset.z += 0.055f * pose.lowVaultAmount;
        }
        if (part.role == CharacterPartRole::LeftLeg || part.role == CharacterPartRole::RightLeg ||
            part.role == CharacterPartRole::LeftFoot || part.role == CharacterPartRole::RightFoot) {
            offset.y += 0.08f * pose.lowVaultAmount;
        }
    }
    if (pose.jumpOffVehicleAmount > 0.001f) {
        if (part.role == CharacterPartRole::Torso || part.role == CharacterPartRole::Head) {
            offset.y += 0.035f * pose.jumpOffVehicleAmount;
            offset.z += 0.055f * pose.jumpOffVehicleAmount;
        }
        if (part.role == CharacterPartRole::LeftArm) {
            offset.x -= 0.10f * pose.jumpOffVehicleAmount;
            offset.y += 0.09f * pose.jumpOffVehicleAmount;
        }
        if (part.role == CharacterPartRole::RightArm) {
            offset.x += 0.10f * pose.jumpOffVehicleAmount;
            offset.y += 0.09f * pose.jumpOffVehicleAmount;
        }
        if (part.role == CharacterPartRole::LeftFoot || part.role == CharacterPartRole::RightFoot) {
            offset.z += 0.07f * pose.jumpOffVehicleAmount;
        }
    }
    if (pose.tiredAmount > 0.001f) {
        if (part.role == CharacterPartRole::Torso || part.role == CharacterPartRole::Head) {
            offset.y -= 0.035f * pose.tiredAmount;
            offset.z -= 0.025f * pose.tiredAmount;
        }
        if (part.role == CharacterPartRole::LeftArm || part.role == CharacterPartRole::RightArm) {
            offset.y -= 0.045f * pose.tiredAmount;
        }
    }
    if (pose.bruisedAmount > 0.001f) {
        if (part.role == CharacterPartRole::Torso || part.role == CharacterPartRole::Head) {
            offset.x += 0.035f * pose.bruisedAmount;
            offset.y -= 0.018f * pose.bruisedAmount;
        }
        if (part.role == CharacterPartRole::LeftLeg || part.role == CharacterPartRole::LeftFoot) {
            offset.y -= 0.035f * pose.bruisedAmount;
            offset.x += 0.025f * pose.bruisedAmount;
        }
        if (part.role == CharacterPartRole::RightArm) {
            offset.y -= 0.035f * pose.bruisedAmount;
        }
    }
    if (part.role == CharacterPartRole::Head) {
        offset.y += pose.jumpStretch * 0.025f;
    }
    if (part.role == CharacterPartRole::LeftArm || part.role == CharacterPartRole::RightArm) {
        offset.y += pose.panicFlail * 0.045f;
    }
    if (part.role == CharacterPartRole::LeftLeg || part.role == CharacterPartRole::RightLeg ||
        part.role == CharacterPartRole::LeftFoot || part.role == CharacterPartRole::RightFoot) {
        offset.y += pose.jumpStretch * 0.035f;
    }
    return offset;
}

void drawCharacterPartLocal(const CharacterArtPart& part,
                            const PlayerPresentationState& pose,
                            float elapsedSeconds,
                            const CharacterArtPalette& palette) {
    const Color tint = characterMaterialColor(part.material, palette);
    const Vec3 offset = characterPartOffset(part, pose);
    rlPushMatrix();
    rlTranslatef(part.position.x + offset.x, part.position.y + offset.y, part.position.z + offset.z);
    rlRotatef(radiansToDegrees(characterPartPitch(part, pose, elapsedSeconds)), 1.0f, 0.0f, 0.0f);

    if (part.shape == CharacterArtShape::Sphere) {
        DrawSphere({0.0f, 0.0f, 0.0f}, part.size.x * 0.5f, tint);
        DrawSphereWires({0.0f, 0.0f, 0.0f}, part.size.x * 0.5f, 10, 8, color(34, 33, 32));
    } else {
        DrawCubeV({0.0f, 0.0f, 0.0f}, toRay(part.size), tint);
        DrawCubeWiresV({0.0f, 0.0f, 0.0f}, toRay(part.size), color(34, 33, 32));
    }

    rlPopMatrix();
}

void drawBoxPartLocalTint(const VehicleArtPart& part, Color tint, float bodyDrop) {
    rlPushMatrix();
    rlTranslatef(part.position.x, part.position.y - bodyDrop, part.position.z);
    rlRotatef(radiansToDegrees(part.yawRadians), 0.0f, 1.0f, 0.0f);
    rlRotatef(radiansToDegrees(part.pitchRadians), 1.0f, 0.0f, 0.0f);
    rlRotatef(radiansToDegrees(part.rollRadians), 0.0f, 0.0f, 1.0f);
    DrawCubeV({0.0f, 0.0f, 0.0f}, toRay(part.size), tint);
    DrawCubeWiresV({0.0f, 0.0f, 0.0f}, toRay(part.size), color(32, 31, 30));
    rlPopMatrix();
}

void drawBoxPartLocal(const VehicleArtPart& part,
                      Color bodyColor,
                      float bodyDrop,
                      const VehicleLightVisualState* lights = nullptr) {
    drawBoxPartLocalTint(part, vehicleMaterialColor(part.material, bodyColor, lights), bodyDrop);
}

void drawQuadPanelLocalTint(const VehicleArtPart& part, Color tint, float bodyDrop) {
    std::array<Vec3, 4> corners = part.panelCorners;
    for (Vec3& corner : corners) {
        corner.y -= bodyDrop;
    }

    DrawTriangle3D(toRay(corners[0]), toRay(corners[1]), toRay(corners[2]), tint);
    DrawTriangle3D(toRay(corners[0]), toRay(corners[2]), toRay(corners[3]), tint);
    DrawTriangle3D(toRay(corners[2]), toRay(corners[1]), toRay(corners[0]), tint);
    DrawTriangle3D(toRay(corners[3]), toRay(corners[2]), toRay(corners[0]), tint);

    const Color wire = color(32, 31, 30, tint.a);
    DrawLine3D(toRay(corners[0]), toRay(corners[1]), wire);
    DrawLine3D(toRay(corners[1]), toRay(corners[2]), wire);
    DrawLine3D(toRay(corners[2]), toRay(corners[3]), wire);
    DrawLine3D(toRay(corners[3]), toRay(corners[0]), wire);
}

void drawQuadPanelPartLocal(const VehicleArtPart& part,
                            Color bodyColor,
                            float bodyDrop,
                            const VehicleLightVisualState* lights = nullptr) {
    drawQuadPanelLocalTint(part, vehicleMaterialColor(part.material, bodyColor, lights), bodyDrop);
}

void drawGlassOverlayLocal(Vec3 size, const GlassVisualState& glass) {
    const Color reflection{210, 235, 234, static_cast<unsigned char>(std::clamp(glass.reflectionIntensity, 0.0f, 1.0f) * 130.0f)};
    const Color reflectionDim{134, 194, 210, static_cast<unsigned char>(std::clamp(glass.reflectionIntensity, 0.0f, 1.0f) * 70.0f)};
    const float frontZ = size.z * 0.52f + 0.004f;
    DrawCubeV({-size.x * 0.20f, size.y * 0.08f, frontZ}, {size.x * 0.48f, 0.012f, 0.012f}, reflection);
    DrawCubeV({size.x * 0.26f, -size.y * 0.16f, frontZ}, {size.x * 0.34f, 0.010f, 0.012f}, reflectionDim);

    if (glass.crackAmount <= 0.02f) {
        return;
    }

    const Color crack{232, 240, 232, static_cast<unsigned char>(80.0f + std::clamp(glass.crackAmount, 0.0f, 1.0f) * 135.0f)};
    const Vector3 center{0.0f, 0.0f, frontZ + 0.006f};
    const float reachX = size.x * (0.18f + glass.crackAmount * 0.22f);
    const float reachY = size.y * (0.18f + glass.crackAmount * 0.18f);
    DrawLine3D(center, {reachX, reachY * 0.45f, center.z}, crack);
    DrawLine3D(center, {-reachX * 0.70f, reachY * 0.28f, center.z}, crack);
    DrawLine3D(center, {reachX * 0.38f, -reachY, center.z}, crack);
    DrawLine3D(center, {-reachX * 0.35f, -reachY * 0.65f, center.z}, crack);
}

void drawGlassPanelOverlayLocal(const VehicleArtPart& part, float bodyDrop, const GlassVisualState& glass) {
    std::array<Vec3, 4> corners = part.panelCorners;
    for (Vec3& corner : corners) {
        corner.y -= bodyDrop - 0.004f;
    }

    const Color reflection{210, 235, 234, static_cast<unsigned char>(std::clamp(glass.reflectionIntensity, 0.0f, 1.0f) * 130.0f)};
    const Color reflectionDim{134, 194, 210, static_cast<unsigned char>(std::clamp(glass.reflectionIntensity, 0.0f, 1.0f) * 70.0f)};
    const Vec3 upperA = lerp(corners[0], corners[1], 0.18f);
    const Vec3 upperB = lerp(corners[0], corners[1], 0.62f);
    const Vec3 lowerA = lerp(lerp(corners[3], corners[2], 0.28f), lerp(corners[0], corners[1], 0.28f), 0.38f);
    const Vec3 lowerB = lerp(lerp(corners[3], corners[2], 0.66f), lerp(corners[0], corners[1], 0.66f), 0.38f);
    DrawLine3D(toRay(upperA), toRay(upperB), reflection);
    DrawLine3D(toRay(lowerA), toRay(lowerB), reflectionDim);

    if (glass.crackAmount <= 0.02f) {
        return;
    }

    const Color crack{232, 240, 232, static_cast<unsigned char>(80.0f + std::clamp(glass.crackAmount, 0.0f, 1.0f) * 135.0f)};
    const Vec3 center = (corners[0] + corners[1] + corners[2] + corners[3]) * 0.25f;
    DrawLine3D(toRay(center), toRay(lerp(center, corners[0], 0.42f)), crack);
    DrawLine3D(toRay(center), toRay(lerp(center, corners[1], 0.34f)), crack);
    DrawLine3D(toRay(center), toRay(lerp(center, corners[2], 0.46f)), crack);
    DrawLine3D(toRay(center), toRay(lerp(center, corners[3], 0.38f)), crack);
}

void drawGlassBoxPartLocal(const VehicleArtPart& part, Color tint, float bodyDrop, const GlassVisualState& glass) {
    rlPushMatrix();
    rlTranslatef(part.position.x, part.position.y - bodyDrop, part.position.z);
    rlRotatef(radiansToDegrees(part.yawRadians), 0.0f, 1.0f, 0.0f);
    rlRotatef(radiansToDegrees(part.pitchRadians), 1.0f, 0.0f, 0.0f);
    rlRotatef(radiansToDegrees(part.rollRadians), 0.0f, 0.0f, 1.0f);
    DrawCubeV({0.0f, 0.0f, 0.0f}, toRay(part.size), tint);
    DrawCubeWiresV({0.0f, 0.0f, 0.0f}, toRay(part.size), color(32, 31, 30, 160));
    drawGlassOverlayLocal(part.size, glass);
    rlPopMatrix();
}

void drawGlassQuadPanelPartLocal(const VehicleArtPart& part, Color tint, float bodyDrop, const GlassVisualState& glass) {
    drawQuadPanelLocalTint(part, tint, bodyDrop);
    drawGlassPanelOverlayLocal(part, bodyDrop, glass);
}

void drawCylinderXPartLocal(const VehicleArtPart& part, Color bodyColor, float bodyDrop) {
    const Color tint = vehicleMaterialColor(part.material, bodyColor);
    const float halfLength = part.size.x * 0.5f;
    rlPushMatrix();
    rlTranslatef(part.position.x, part.position.y - bodyDrop, part.position.z);
    rlRotatef(radiansToDegrees(part.yawRadians), 0.0f, 1.0f, 0.0f);
    rlRotatef(radiansToDegrees(part.pitchRadians), 1.0f, 0.0f, 0.0f);
    rlRotatef(radiansToDegrees(part.rollRadians), 0.0f, 0.0f, 1.0f);
    DrawCylinderEx({-halfLength, 0.0f, 0.0f},
                   {halfLength, 0.0f, 0.0f},
                   part.size.y * 0.5f,
                   part.size.z * 0.5f,
                   14,
                   tint);
    DrawCylinderWiresEx({-halfLength, 0.0f, 0.0f},
                        {halfLength, 0.0f, 0.0f},
                        part.size.y * 0.5f,
                        part.size.z * 0.5f,
                        14,
                        color(30, 30, 29));
    rlPopMatrix();
}

void drawVehicleWheelLocal(const VehicleArtPart& part,
                           float bodyDrop,
                           float wheelRadius,
                           float wheelLift,
                           float steerRadians,
                           float spinRadians,
                           float slipVisual,
                           float outward) {
    const float wheelHalfWidth = part.size.x * 0.5f;
    const float visualSteerRadians = steerRadians;
    const float outwardSign = outward < 0.0f ? -1.0f : 1.0f;
    const Vec3 center{part.position.x, part.position.y + wheelLift - bodyDrop * 0.15f, part.position.z};
    const Color tire = color(23, 23, 22);
    const Color sidewall = color(34, 34, 33);
    const Color hub = color(118, 121, 116);
    const Color spoke = color(214, 214, 197);

    rlPushMatrix();
    rlTranslatef(center.x, center.y, center.z);
    rlRotatef(radiansToDegrees(visualSteerRadians), 0.0f, 1.0f, 0.0f);

    DrawCylinderEx({-wheelHalfWidth, 0.0f, 0.0f},
                   {wheelHalfWidth, 0.0f, 0.0f},
                   wheelRadius,
                   wheelRadius,
                   18,
                   tire);
    DrawCylinderWiresEx({-wheelHalfWidth, 0.0f, 0.0f},
                        {wheelHalfWidth, 0.0f, 0.0f},
                        wheelRadius,
                        wheelRadius,
                        18,
                        sidewall);

    const float faceX = outwardSign * (wheelHalfWidth + 0.012f);
    DrawCylinderEx({faceX, 0.0f, 0.0f},
                   {faceX + outwardSign * 0.018f, 0.0f, 0.0f},
                   wheelRadius * 0.50f,
                   wheelRadius * 0.50f,
                   14,
                   hub);
    for (int i = 0; i < 4; ++i) {
        const float angle = spinRadians + static_cast<float>(i) * Pi * 0.5f;
        const Vector3 face{faceX + outwardSign * 0.025f, 0.0f, 0.0f};
        const Vector3 end{face.x,
                          std::cos(angle) * wheelRadius * 0.76f,
                          std::sin(angle) * wheelRadius * 0.76f};
        DrawLine3D(face, end, spoke);
    }

    if (slipVisual > 0.05f) {
        const unsigned char alpha = static_cast<unsigned char>(std::clamp(slipVisual, 0.0f, 1.0f) * 120.0f);
        DrawCubeV({0.0f, -wheelRadius + 0.015f, -0.48f}, {0.10f, 0.025f, 0.72f}, color(18, 18, 17, alpha));
    }

    rlPopMatrix();
}

} // namespace

Vec3 vehicleDriverSeatPosition(Vec3 vehiclePosition, float vehicleYawRadians) {
    const Vec3 forward = forwardFromYaw(vehicleYawRadians);
    const Vec3 right = screenRightFromYaw(vehicleYawRadians);
    return vehiclePosition - right * 0.24f + forward * 0.04f + Vec3{0.0f, -0.16f, 0.0f};
}

bool shouldRenderCharacterPart(CharacterPartRole role, CharacterRenderOptions options) {
    if (!options.hideLowerBody) {
        return true;
    }
    return role != CharacterPartRole::LeftLeg &&
           role != CharacterPartRole::RightLeg &&
           role != CharacterPartRole::LeftFoot &&
           role != CharacterPartRole::RightFoot;
}

const WorldPresentationStyle& defaultWorldPresentationStyle() {
    static const WorldPresentationStyle style{
        renderColor(126, 154, 161),
        renderColor(77, 96, 70),
        renderColor(178, 163, 128, 88),
        96.0f,
        150.0f};
    return style;
}

const char* worldRenderIsolationModeName(WorldRenderIsolationMode mode) {
    switch (mode) {
    case WorldRenderIsolationMode::Full:
        return "Full";
    case WorldRenderIsolationMode::OpaqueOnly:
        return "OpaqueOnly";
    case WorldRenderIsolationMode::NoDecals:
        return "NoDecals";
    case WorldRenderIsolationMode::NoGlass:
        return "NoGlass";
    case WorldRenderIsolationMode::NoLegacyWorld:
        return "NoLegacyWorld";
    case WorldRenderIsolationMode::WorldObjectsOnly:
        return "WorldObjectsOnly";
    case WorldRenderIsolationMode::DebugShopShell:
        return "DebugShopShell";
    case WorldRenderIsolationMode::DebugShopShellWireframe:
        return "DebugShopShellWireframe";
    case WorldRenderIsolationMode::DebugShopShellNoCull:
        return "DebugShopShellNoCull";
    case WorldRenderIsolationMode::DebugShopShellNormals:
        return "DebugShopShellNormals";
    case WorldRenderIsolationMode::DebugShopShellBounds:
        return "DebugShopShellBounds";
    case WorldRenderIsolationMode::DebugShopDoorWireframe:
        return "DebugShopDoorWireframe";
    case WorldRenderIsolationMode::DebugShopDoorCube:
        return "DebugShopDoorCube";
    case WorldRenderIsolationMode::AssetPreview:
        return "AssetPreview";
    case WorldRenderIsolationMode::AssetPreviewWireframe:
        return "AssetPreviewWireframe";
    case WorldRenderIsolationMode::AssetPreviewNoCull:
        return "AssetPreviewNoCull";
    case WorldRenderIsolationMode::AssetPreviewNormals:
        return "AssetPreviewNormals";
    case WorldRenderIsolationMode::AssetPreviewBounds:
        return "AssetPreviewBounds";
    }

    return "Full";
}

WorldRenderIsolationMode nextWorldRenderIsolationMode(WorldRenderIsolationMode mode) {
    switch (mode) {
    case WorldRenderIsolationMode::Full:
        return WorldRenderIsolationMode::OpaqueOnly;
    case WorldRenderIsolationMode::OpaqueOnly:
        return WorldRenderIsolationMode::NoDecals;
    case WorldRenderIsolationMode::NoDecals:
        return WorldRenderIsolationMode::NoGlass;
    case WorldRenderIsolationMode::NoGlass:
        return WorldRenderIsolationMode::NoLegacyWorld;
    case WorldRenderIsolationMode::NoLegacyWorld:
        return WorldRenderIsolationMode::WorldObjectsOnly;
    case WorldRenderIsolationMode::WorldObjectsOnly:
        return WorldRenderIsolationMode::DebugShopShell;
    case WorldRenderIsolationMode::DebugShopShell:
        return WorldRenderIsolationMode::DebugShopShellWireframe;
    case WorldRenderIsolationMode::DebugShopShellWireframe:
        return WorldRenderIsolationMode::DebugShopShellNoCull;
    case WorldRenderIsolationMode::DebugShopShellNoCull:
        return WorldRenderIsolationMode::DebugShopShellNormals;
    case WorldRenderIsolationMode::DebugShopShellNormals:
        return WorldRenderIsolationMode::DebugShopShellBounds;
    case WorldRenderIsolationMode::DebugShopShellBounds:
        return WorldRenderIsolationMode::DebugShopDoorWireframe;
    case WorldRenderIsolationMode::DebugShopDoorWireframe:
        return WorldRenderIsolationMode::DebugShopDoorCube;
    case WorldRenderIsolationMode::DebugShopDoorCube:
        return WorldRenderIsolationMode::AssetPreview;
    case WorldRenderIsolationMode::AssetPreview:
        return WorldRenderIsolationMode::AssetPreviewWireframe;
    case WorldRenderIsolationMode::AssetPreviewWireframe:
        return WorldRenderIsolationMode::AssetPreviewNoCull;
    case WorldRenderIsolationMode::AssetPreviewNoCull:
        return WorldRenderIsolationMode::AssetPreviewNormals;
    case WorldRenderIsolationMode::AssetPreviewNormals:
        return WorldRenderIsolationMode::AssetPreviewBounds;
    case WorldRenderIsolationMode::AssetPreviewBounds:
        return WorldRenderIsolationMode::Full;
    }

    return WorldRenderIsolationMode::Full;
}

bool isAssetPreviewIsolationMode(WorldRenderIsolationMode mode) {
    return mode == WorldRenderIsolationMode::AssetPreview ||
           mode == WorldRenderIsolationMode::AssetPreviewWireframe ||
           mode == WorldRenderIsolationMode::AssetPreviewNoCull ||
           mode == WorldRenderIsolationMode::AssetPreviewNormals ||
           mode == WorldRenderIsolationMode::AssetPreviewBounds;
}

VehicleLightVisualState vehicleLightVisualState(const VehicleRuntimeState& state) {
    const float health = std::clamp(state.condition / 100.0f, 0.0f, 1.0f);
    const float wear = 1.0f - health;
    const float rpm = std::clamp(state.rpmNormalized, 0.0f, 1.0f);
    const float reverse = state.speed < -0.05f
        ? std::clamp(-state.speed / 4.8f, 0.0f, 1.0f)
        : 0.0f;
    const float drift = state.driftActive ? std::clamp(state.driftAmount, 0.0f, 1.0f) : 0.0f;
    const float boost = state.boostActive ? 0.18f : 0.0f;

    VehicleLightVisualState visual;
    visual.headlightGlow = std::clamp(0.26f + rpm * 0.18f + boost - wear * 0.14f, 0.16f, 0.62f);
    visual.taillightGlow = std::clamp(0.27f + reverse * 0.42f + drift * 0.08f - wear * 0.10f, 0.16f, 0.74f);
    visual.headlightColor = toRenderColor(scaleRgb(color(255, 236, 128), 0.72f + visual.headlightGlow * 0.50f));
    visual.taillightColor = toRenderColor(scaleRgb(color(220, 38, 42), 0.66f + visual.taillightGlow * 0.55f));
    return visual;
}

std::vector<WorldAtmosphereBand> buildWorldAtmosphereBands(Vec3 cameraPosition,
                                                           const WorldPresentationStyle& style) {
    if (style.horizonHazeColor.a == 0 || style.groundPlaneSize <= 0.0f || style.worldCullDistance <= 0.0f) {
        return {};
    }

    const float outerRadius = std::min(style.groundPlaneSize * 0.50f, style.worldCullDistance * 0.42f);
    const float innerRadius = outerRadius * 0.64f;
    const unsigned char innerAlpha =
        static_cast<unsigned char>(std::max(12, static_cast<int>(style.horizonHazeColor.a) / 3));
    const Vec3 center{cameraPosition.x, 0.015f, cameraPosition.z};
    return {
        {center, innerRadius * 0.94f, innerRadius, 0.022f, withAlpha(style.horizonHazeColor, innerAlpha)},
        {center, outerRadius * 0.82f, outerRadius, 0.030f, style.horizonHazeColor},
    };
}

void WorldRenderer::drawWorldGround() {
    const WorldPresentationStyle& style = defaultWorldPresentationStyle();
    DrawPlane({0.0f, -0.08f, 0.0f},
              {style.groundPlaneSize, style.groundPlaneSize},
              toColor(style.groundColor));
}

void WorldRenderer::drawWorldAtmosphere(Vec3 cameraPosition) {
    const std::vector<WorldAtmosphereBand> bands =
        buildWorldAtmosphereBands(cameraPosition, defaultWorldPresentationStyle());
    if (bands.empty()) {
        return;
    }

    BeginBlendMode(BLEND_ALPHA);
    for (const WorldAtmosphereBand& band : bands) {
        DrawCylinder(toRay(band.center),
                     band.radiusTop,
                     band.radiusBottom,
                     band.height,
                     64,
                     toColor(band.color));
    }
    EndBlendMode();
}

Color resolveFallbackColor(const WorldObject& object,
                           const WorldAssetDefinition* definition,
                           Color drawTint,
                           bool forceTint) {
    if (forceTint || object.hasTintOverride) {
        return drawTint;
    }
    return definition != nullptr ? toColor(definition->fallbackColor) : color(180, 180, 180);
}

bool isDebugShopDoorTarget(const WorldObject& object) {
    return object.id == "shop_door_front";
}

bool isDebugShopShellTarget(const WorldObject& object) {
    return object.id == "shop_zenona";
}

bool isDebugShopDoorMode(WorldRenderIsolationMode mode) {
    return mode == WorldRenderIsolationMode::DebugShopDoorWireframe ||
           mode == WorldRenderIsolationMode::DebugShopDoorCube;
}

bool isDebugShopShellMode(WorldRenderIsolationMode mode) {
    return mode == WorldRenderIsolationMode::DebugShopShell ||
           mode == WorldRenderIsolationMode::DebugShopShellWireframe ||
           mode == WorldRenderIsolationMode::DebugShopShellNoCull ||
           mode == WorldRenderIsolationMode::DebugShopShellNormals ||
           mode == WorldRenderIsolationMode::DebugShopShellBounds;
}

bool isDebugWorldObjectIsolationMode(WorldRenderIsolationMode mode) {
    return isDebugShopShellMode(mode) || isDebugShopDoorMode(mode);
}

WorldObject previewObjectForAsset(const WorldAssetDefinition& definition) {
    WorldObject object;
    object.id = "asset_preview";
    object.assetId = definition.id;
    object.position = {0.0f, 0.0f, 0.0f};
    object.scale = {1.0f, 1.0f, 1.0f};
    object.collision.kind = WorldCollisionShapeKind::None;
    return object;
}

void drawModelBounds(const Model& model, const WorldObject& object, Vec3 renderPosition) {
    const BoundingBox bounds = GetModelBoundingBox(model);
    const Vec3 center{(bounds.min.x + bounds.max.x) * 0.5f,
                      (bounds.min.y + bounds.max.y) * 0.5f,
                      (bounds.min.z + bounds.max.z) * 0.5f};
    const Vec3 size{bounds.max.x - bounds.min.x,
                    bounds.max.y - bounds.min.y,
                    bounds.max.z - bounds.min.z};

    rlPushMatrix();
    rlTranslatef(renderPosition.x, renderPosition.y, renderPosition.z);
    rlRotatef(radiansToDegrees(object.yawRadians), 0.0f, 1.0f, 0.0f);
    DrawCubeWiresV(toRay({center.x * object.scale.x, center.y * object.scale.y, center.z * object.scale.z}),
                   toRay({size.x * object.scale.x, size.y * object.scale.y, size.z * object.scale.z}),
                   color(255, 236, 128));
    rlPopMatrix();
}

void drawModelNormals(const Model& model, const WorldObject& object, Vec3 renderPosition) {
    for (int meshIndex = 0; meshIndex < model.meshCount; ++meshIndex) {
        const Mesh& mesh = model.meshes[meshIndex];
        if (mesh.vertices == nullptr || mesh.normals == nullptr) {
            continue;
        }
        for (int vertex = 0; vertex + 2 < mesh.vertexCount; vertex += 3) {
            Vec3 center{};
            Vec3 normal{};
            for (int offset = 0; offset < 3; ++offset) {
                const int index = vertex + offset;
                center = center + Vec3{mesh.vertices[index * 3 + 0],
                                       mesh.vertices[index * 3 + 1],
                                       mesh.vertices[index * 3 + 2]};
                normal = normal + Vec3{mesh.normals[index * 3 + 0],
                                       mesh.normals[index * 3 + 1],
                                       mesh.normals[index * 3 + 2]};
            }
            center = center * (1.0f / 3.0f);
            normal = normalized(normal);
            const Vec3 start = transformModelPoint(center, object, renderPosition);
            const Vec3 end = start + rotateY(normal, object.yawRadians) * 0.22f;
            DrawLine3D(toRay(start), toRay(end), color(80, 190, 255));
        }
    }
}

void drawWorldObjectCubeFallback(const WorldObject& object,
                                 const WorldAssetDefinition* definition,
                                 Vec3 renderPosition,
                                 Color drawTint,
                                 bool forceTint) {
    const Vec3 fallbackSize = definition != nullptr
                                  ? Vec3{definition->fallbackSize.x * object.scale.x,
                                         definition->fallbackSize.y * object.scale.y,
                                         definition->fallbackSize.z * object.scale.z}
                                  : object.scale;
    const Color fallbackColor = resolveFallbackColor(object, definition, drawTint, forceTint);
    const Color wireColor = fallbackColor.a < 255
                                ? color(42, 42, 41, std::max<unsigned char>(80, fallbackColor.a))
                                : color(42, 42, 41);

    rlPushMatrix();
    rlTranslatef(renderPosition.x, renderPosition.y, renderPosition.z);
    rlRotatef(radiansToDegrees(object.yawRadians), 0.0f, 1.0f, 0.0f);
    DrawCubeV({0.0f, fallbackSize.y * 0.5f, 0.0f}, toRay(fallbackSize), fallbackColor);
    DrawCubeWiresV({0.0f, fallbackSize.y * 0.5f, 0.0f}, toRay(fallbackSize), wireColor);
    rlPopMatrix();
}

void drawWorldObjectWireFallback(const WorldObject& object,
                                 const WorldAssetDefinition* definition,
                                 Vec3 renderPosition,
                                 Color drawTint,
                                 bool forceTint) {
    const Vec3 fallbackSize = definition != nullptr
                                  ? Vec3{definition->fallbackSize.x * object.scale.x,
                                         definition->fallbackSize.y * object.scale.y,
                                         definition->fallbackSize.z * object.scale.z}
                                  : object.scale;
    const Color fallbackColor = resolveFallbackColor(object, definition, drawTint, forceTint);

    rlPushMatrix();
    rlTranslatef(renderPosition.x, renderPosition.y, renderPosition.z);
    rlRotatef(radiansToDegrees(object.yawRadians), 0.0f, 1.0f, 0.0f);
    DrawCubeWiresV({0.0f, fallbackSize.y * 0.5f, 0.0f}, toRay(fallbackSize), fallbackColor);
    rlPopMatrix();
}

void drawWorldObjectOpaque(const WorldObject& object,
                           const WorldAssetRegistry& registry,
                           const WorldModelCache& modelCache,
                           Color drawTint,
                           bool forceTint = false,
                           WorldRenderIsolationMode mode = WorldRenderIsolationMode::Full) {
    const WorldAssetDefinition* definition = registry.find(object.assetId);
    const Vec3 renderPosition = definition != nullptr && definition->hasVisualOffset
                                    ? object.position + definition->visualOffset
                                    : object.position;
    if (mode == WorldRenderIsolationMode::DebugShopDoorCube && isDebugShopDoorTarget(object)) {
        drawWorldObjectCubeFallback(object, definition, renderPosition, drawTint, forceTint);
        return;
    }
    if (const Model* model = modelCache.findLoadedModel(object.assetId)) {
        if (isDebugShopShellTarget(object) && mode == WorldRenderIsolationMode::DebugShopShellWireframe) {
            DrawModelWiresEx(*model,
                             toRay(renderPosition),
                             {0.0f, 1.0f, 0.0f},
                             radiansToDegrees(object.yawRadians),
                             toRay(object.scale),
                             drawTint);
            return;
        }
        if (mode == WorldRenderIsolationMode::DebugShopDoorWireframe && isDebugShopDoorTarget(object)) {
            DrawModelWiresEx(*model,
                             toRay(renderPosition),
                             {0.0f, 1.0f, 0.0f},
                             radiansToDegrees(object.yawRadians),
                             toRay(object.scale),
                             drawTint);
            return;
        }
        if (isDebugShopShellTarget(object) && mode == WorldRenderIsolationMode::DebugShopShellNoCull) {
            rlDisableBackfaceCulling();
            DrawModelEx(*model,
                        toRay(renderPosition),
                        {0.0f, 1.0f, 0.0f},
                        radiansToDegrees(object.yawRadians),
                        toRay(object.scale),
                        drawTint);
            rlEnableBackfaceCulling();
            return;
        }
        DrawModelEx(*model,
                    toRay(renderPosition),
                    {0.0f, 1.0f, 0.0f},
                    radiansToDegrees(object.yawRadians),
                    toRay(object.scale),
                    drawTint);
        if (isDebugShopShellTarget(object) && mode == WorldRenderIsolationMode::DebugShopShellNormals) {
            drawModelNormals(*model, object, renderPosition);
        }
        if (isDebugShopShellTarget(object) && mode == WorldRenderIsolationMode::DebugShopShellBounds) {
            drawModelBounds(*model, object, renderPosition);
        }
        return;
    }

    if (mode == WorldRenderIsolationMode::DebugShopShellWireframe && isDebugShopShellTarget(object)) {
        drawWorldObjectWireFallback(object, definition, renderPosition, drawTint, forceTint);
        return;
    }
    if (mode == WorldRenderIsolationMode::DebugShopDoorWireframe && isDebugShopDoorTarget(object)) {
        drawWorldObjectWireFallback(object, definition, renderPosition, drawTint, forceTint);
        return;
    }
    drawWorldObjectCubeFallback(object, definition, renderPosition, drawTint, forceTint);
}

void drawAssetPreviewObject(const WorldAssetDefinition& definition,
                            const WorldModelCache& modelCache,
                            WorldRenderIsolationMode mode) {
    const WorldObject object = previewObjectForAsset(definition);
    const Vec3 renderPosition = object.position + definition.visualOffset;
    const Color tint = toColor(definition.fallbackColor);
    if (const Model* model = modelCache.findLoadedModel(definition.id)) {
        if (mode == WorldRenderIsolationMode::AssetPreviewWireframe) {
            DrawModelWiresEx(*model,
                             toRay(renderPosition),
                             {0.0f, 1.0f, 0.0f},
                             0.0f,
                             toRay(object.scale),
                             color(255, 236, 128));
            return;
        }
        if (mode == WorldRenderIsolationMode::AssetPreviewNoCull) {
            rlDisableBackfaceCulling();
            DrawModelEx(*model, toRay(renderPosition), {0.0f, 1.0f, 0.0f}, 0.0f, toRay(object.scale), tint);
            rlEnableBackfaceCulling();
            return;
        }
        DrawModelEx(*model, toRay(renderPosition), {0.0f, 1.0f, 0.0f}, 0.0f, toRay(object.scale), tint);
        if (mode == WorldRenderIsolationMode::AssetPreviewNormals) {
            drawModelNormals(*model, object, renderPosition);
        }
        if (mode == WorldRenderIsolationMode::AssetPreviewBounds) {
            drawModelBounds(*model, object, renderPosition);
        }
        return;
    }

    if (mode == WorldRenderIsolationMode::AssetPreviewWireframe) {
        drawWorldObjectWireFallback(object, &definition, renderPosition, color(255, 236, 128), true);
        return;
    }
    drawWorldObjectCubeFallback(object, &definition, renderPosition, tint, true);
}

void drawAssetPreviewOriginMarker() {
    DrawSphere({0.0f, 0.0f, 0.0f}, 0.035f, color(255, 236, 128));
    DrawLine3D({0.0f, 0.0f, 0.0f}, {0.45f, 0.0f, 0.0f}, RED);
    DrawLine3D({0.0f, 0.0f, 0.0f}, {0.0f, 0.45f, 0.0f}, GREEN);
    DrawLine3D({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.45f}, BLUE);
}

void drawWorldGlassObject(const WorldObject& object,
                          const WorldAssetRegistry& registry,
                          const WorldModelCache& modelCache) {
    const GlassVisualState glass = worldGlassVisualState(object);
    const Color glassTint{glass.tint.r, glass.tint.g, glass.tint.b, glass.tint.a};
    drawWorldObjectOpaque(object, registry, modelCache, glassTint, true);

    const WorldAssetDefinition* definition = registry.find(object.assetId);
    const Vec3 renderPosition = definition != nullptr && definition->hasVisualOffset
                                    ? object.position + definition->visualOffset
                                    : object.position;
    const Vec3 fallbackSize = definition != nullptr
                                  ? Vec3{definition->fallbackSize.x * object.scale.x,
                                         definition->fallbackSize.y * object.scale.y,
                                         definition->fallbackSize.z * object.scale.z}
                                  : object.scale;
    rlPushMatrix();
    rlTranslatef(renderPosition.x, renderPosition.y, renderPosition.z);
    rlRotatef(radiansToDegrees(object.yawRadians), 0.0f, 1.0f, 0.0f);
    rlTranslatef(0.0f, fallbackSize.y * 0.5f, 0.0f);
    drawGlassOverlayLocal(fallbackSize, glass);
    rlPopMatrix();
}

float objectRenderRadius(const WorldObject& object, const WorldAssetRegistry& registry) {
    Vec3 visualSize = object.scale;
    if (const WorldAssetDefinition* definition = registry.find(object.assetId)) {
        visualSize = {std::fabs(definition->fallbackSize.x * object.scale.x),
                      std::fabs(definition->fallbackSize.y * object.scale.y),
                      std::fabs(definition->fallbackSize.z * object.scale.z)};
    }
    const float maxVisual = std::max({std::fabs(visualSize.x), std::fabs(visualSize.y), std::fabs(visualSize.z)});
    const float maxCollision = std::max({std::fabs(object.collision.size.x),
                                         std::fabs(object.collision.size.y),
                                         std::fabs(object.collision.size.z)});
    return std::max(0.75f, std::max(maxVisual, maxCollision) * 0.75f);
}

float distanceSquared3D(Vec3 a, Vec3 b) {
    const Vec3 delta = a - b;
    return delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
}

bool hasTranslucentWorldMaterial(const WorldObject& object, const WorldAssetRegistry& registry) {
    if (object.hasTintOverride && object.tintOverride.a < 255) {
        return true;
    }
    if (const WorldAssetDefinition* definition = registry.find(object.assetId)) {
        return definition->fallbackColor.a < 255;
    }
    return false;
}

bool hasTag(const std::vector<std::string>& tags, const char* expected) {
    return std::find(tags.begin(), tags.end(), expected) != tags.end();
}

bool isWorldDecalObject(const WorldObject& object, const WorldAssetRegistry& registry) {
    if (hasTag(object.gameplayTags, "decal") ||
        hasTag(object.gameplayTags, "ground_patch") ||
        hasTag(object.gameplayTags, "asphalt_patch") ||
        hasTag(object.gameplayTags, "grass_patch") ||
        hasTag(object.gameplayTags, "irregular_ground_patch") ||
        hasTag(object.gameplayTags, "subtle_decal")) {
        return true;
    }

    const WorldAssetDefinition* definition = registry.find(object.assetId);
    return definition != nullptr &&
           (definition->renderBucket == "Decal" ||
            hasTag(definition->defaultTags, "decal") ||
            hasTag(definition->defaultTags, "ground_patch"));
}

void sortBackToFront(std::vector<const WorldObject*>& objects, Vec3 cameraPosition) {
    std::sort(objects.begin(), objects.end(), [cameraPosition](const WorldObject* lhs, const WorldObject* rhs) {
        return distanceSquared3D(lhs->position, cameraPosition) > distanceSquared3D(rhs->position, cameraPosition);
    });
}

WorldRenderList buildWorldRenderList(const std::vector<WorldObject>& objects,
                                     const WorldAssetRegistry& registry,
                                     Vec3 cameraPosition,
                                     float maxDistance,
                                     WorldRenderIsolationMode mode) {
    WorldRenderList list;
    const float maxDistanceSq = maxDistance * maxDistance;
    for (const WorldObject& object : objects) {
        const WorldAssetDefinition* definition = registry.find(object.assetId);
        if (definition != nullptr &&
            (!definition->renderInGameplay ||
             definition->renderBucket == "DebugOnly" ||
             definition->assetType == "DebugOnly")) {
            continue;
        }
        if (isDebugWorldObjectIsolationMode(mode) &&
            ((isDebugShopShellMode(mode) && !isDebugShopShellTarget(object)) ||
             (isDebugShopDoorMode(mode) && !isDebugShopDoorTarget(object)))) {
            continue;
        }
        const bool glassObject = isWorldGlassObject(object) || (definition != nullptr && definition->renderBucket == "Glass");
        const bool decalObject = isWorldDecalObject(object, registry);
        if (mode == WorldRenderIsolationMode::NoGlass && glassObject) {
            continue;
        }
        if (mode == WorldRenderIsolationMode::NoDecals && decalObject) {
            continue;
        }

        const float radius = objectRenderRadius(object, registry);
        const float expandedMax = maxDistance + radius;
        if (distanceSquared3D(object.position, cameraPosition) > expandedMax * expandedMax &&
            distanceSquaredXZ(object.position, cameraPosition) > maxDistanceSq) {
            ++list.culled;
            continue;
        }
        if (glassObject) {
            list.glass.push_back(&object);
            list.transparent.push_back(&object);
        } else if (decalObject) {
            list.translucent.push_back(&object);
            list.transparent.push_back(&object);
        } else if (hasTranslucentWorldMaterial(object, registry)) {
            list.translucent.push_back(&object);
            list.transparent.push_back(&object);
        } else {
            list.opaque.push_back(&object);
        }
    }

    sortBackToFront(list.translucent, cameraPosition);
    sortBackToFront(list.glass, cameraPosition);
    sortBackToFront(list.transparent, cameraPosition);
    return list;
}

RenderColor memoryHotspotColor(WorldLocationTag location) {
    switch (location) {
    case WorldLocationTag::Block:
        return renderColor(80, 190, 255, 145);
    case WorldLocationTag::Shop:
        return renderColor(75, 220, 105, 150);
    case WorldLocationTag::Parking:
        return renderColor(238, 211, 79, 150);
    case WorldLocationTag::Garage:
        return renderColor(241, 146, 66, 150);
    case WorldLocationTag::Trash:
        return renderColor(175, 118, 220, 150);
    case WorldLocationTag::RoadLoop:
        return renderColor(230, 88, 70, 150);
    case WorldLocationTag::Unknown:
        break;
    }

    return renderColor(214, 217, 210, 130);
}

RenderColor rewirPressureColor(RewirPressureLevel level) {
    switch (level) {
    case RewirPressureLevel::Watchful:
        return renderColor(80, 190, 255, 210);
    case RewirPressureLevel::Alert:
        return renderColor(255, 110, 110, 230);
    case RewirPressureLevel::Quiet:
        break;
    }

    return renderColor(214, 217, 210, 180);
}

float rewirPressurePatrolRadius(float score) {
    return std::clamp(3.0f + score, 4.0f, 8.0f);
}

std::vector<MemoryHotspotDebugMarker> buildMemoryHotspotDebugMarkers(
    const std::vector<WorldEventHotspot>& hotspots) {
    std::vector<MemoryHotspotDebugMarker> markers;
    markers.reserve(hotspots.size());
    for (const WorldEventHotspot& hotspot : hotspots) {
        if (hotspot.location == WorldLocationTag::Unknown) {
            continue;
        }

        MemoryHotspotDebugMarker marker;
        marker.location = hotspot.location;
        marker.position = hotspot.position;
        marker.score = hotspot.score;
        marker.radius = std::clamp(0.34f + hotspot.score * 0.075f, 0.36f, 0.86f);
        marker.color = memoryHotspotColor(hotspot.location);
        marker.source = hotspot.source;
        markers.push_back(marker);
    }
    return markers;
}

std::vector<RewirPressureDebugMarker> buildRewirPressureDebugMarkers(const RewirPressureSnapshot& pressure) {
    if (!pressure.active || !pressure.patrolInterest) {
        return {};
    }

    RewirPressureDebugMarker marker;
    marker.level = pressure.level;
    marker.location = pressure.location;
    marker.position = pressure.position;
    marker.score = pressure.score;
    marker.radius = rewirPressurePatrolRadius(pressure.score);
    marker.color = rewirPressureColor(pressure.level);
    marker.source = pressure.source;
    return {marker};
}

void WorldRenderer::drawWorldOpaque(const WorldRenderList& renderList,
                                    const WorldAssetRegistry& registry,
                                    const WorldModelCache& modelCache,
                                    WorldRenderIsolationMode mode) {
    for (const WorldObject* object : renderList.opaque) {
        const Color drawTint = object->hasTintOverride ? toColor(object->tintOverride) : WHITE;
        drawWorldObjectOpaque(*object, registry, modelCache, drawTint, false, mode);
    }
}

void WorldRenderer::drawWorldTransparent(const WorldRenderList& renderList,
                                         const WorldAssetRegistry& registry,
                                         const WorldModelCache& modelCache,
                                         WorldRenderIsolationMode mode) {
    BeginBlendMode(BLEND_ALPHA);
    for (const WorldObject* object : renderList.transparent) {
        if (isWorldGlassObject(*object)) {
            drawWorldGlassObject(*object, registry, modelCache);
        } else {
            const Color drawTint = object->hasTintOverride ? toColor(object->tintOverride) : WHITE;
            drawWorldObjectOpaque(*object, registry, modelCache, drawTint, false, mode);
        }
    }
    EndBlendMode();
}

void WorldRenderer::drawAssetPreview(const WorldAssetDefinition& definition,
                                     const WorldModelCache& modelCache,
                                     WorldRenderIsolationMode mode) {
    const bool blended = definition.fallbackColor.a < 255 ||
                         definition.renderBucket == "Glass" ||
                         definition.renderBucket == "Decal" ||
                         definition.renderBucket == "Translucent";
    if (blended) {
        BeginBlendMode(BLEND_ALPHA);
    }
    drawAssetPreviewObject(definition, modelCache, mode);
    if (blended) {
        EndBlendMode();
    }
    drawAssetPreviewOriginMarker();
}

void WorldRenderer::drawWorldCollisionDebug(const std::vector<WorldObject>& objects) {
    for (const WorldObject& object : objects) {
        const Vec3 center = object.position + object.collision.offset;
        Color debugColor = color(255, 110, 110);
        if (hasCollisionLayer(object.collisionProfile.layers, CollisionLayer::WalkableSurface)) {
            debugColor = color(95, 220, 117);
        } else if (hasCollisionLayer(object.collisionProfile.layers, CollisionLayer::DynamicProp)) {
            debugColor = color(255, 178, 70);
        } else if (hasCollisionLayer(object.collisionProfile.layers, CollisionLayer::CameraBlocker)) {
            debugColor = color(160, 120, 255);
        } else if (hasCollisionLayer(object.collisionProfile.layers, CollisionLayer::VehicleBlocker) &&
                   !hasCollisionLayer(object.collisionProfile.layers, CollisionLayer::CameraBlocker)) {
            debugColor = color(255, 160, 90);
        }
        switch (object.collision.kind) {
        case WorldCollisionShapeKind::Box:
            DrawCubeWiresV(toRay(center), toRay(object.collision.size), debugColor);
            break;
        case WorldCollisionShapeKind::OrientedBox:
            rlPushMatrix();
            rlTranslatef(center.x, center.y, center.z);
            rlRotatef(radiansToDegrees(object.collision.yawRadians + object.yawRadians), 0.0f, 1.0f, 0.0f);
            DrawCubeWiresV({0.0f, 0.0f, 0.0f}, toRay(object.collision.size), debugColor);
            rlPopMatrix();
            break;
        case WorldCollisionShapeKind::GroundBox:
            DrawCubeWiresV(toRay(center), toRay(object.collision.size), debugColor);
            break;
        case WorldCollisionShapeKind::RampZ:
            DrawCubeWiresV(toRay(center), toRay(object.collision.size + Vec3{0.0f, 0.25f, 0.0f}), color(80, 190, 255));
            break;
        case WorldCollisionShapeKind::TriggerSphere:
            DrawSphereWires(toRay(center), object.collision.size.x, 16, 8, color(80, 190, 255));
            break;
        case WorldCollisionShapeKind::TriggerBox:
            DrawCubeWiresV(toRay(center), toRay(object.collision.size), color(80, 190, 255));
            break;
        case WorldCollisionShapeKind::None:
        case WorldCollisionShapeKind::Unspecified:
            break;
        }
    }
}

void WorldRenderer::drawSelectionHighlight(const WorldObject& object) {
    const Vec3 center = object.position + object.collision.offset;
    const Color selColor = color(80, 210, 255);
    const float offset = 0.02f;
    switch (object.collision.kind) {
    case WorldCollisionShapeKind::Box:
    case WorldCollisionShapeKind::GroundBox:
    case WorldCollisionShapeKind::TriggerBox:
        DrawCubeWiresV(toRay(center), toRay(object.collision.size + Vec3{offset, offset, offset}), selColor);
        break;
    case WorldCollisionShapeKind::OrientedBox:
        rlPushMatrix();
        rlTranslatef(center.x, center.y, center.z);
        rlRotatef(radiansToDegrees(object.collision.yawRadians + object.yawRadians), 0.0f, 1.0f, 0.0f);
        DrawCubeWiresV({0.0f, 0.0f, 0.0f}, toRay(object.collision.size + Vec3{offset, offset, offset}), selColor);
        rlPopMatrix();
        break;
    default:
        DrawCubeWiresV(toRay(center), toRay(object.collision.size + Vec3{offset, offset, offset}), selColor);
        break;
    }
}

void WorldRenderer::drawMarker(Vec3 position, float radius, RenderColor markerColor) {
    DrawCylinder(toRay(position + Vec3{0.0f, 0.025f, 0.0f}), radius, radius, 0.05f, 28, toColor(markerColor));
    DrawCylinderWires(toRay(position + Vec3{0.0f, 0.065f, 0.0f}), radius, radius, 0.06f, 28, Fade(RAYWHITE, 0.72f));
}

void WorldRenderer::drawMemoryHotspotDebugMarkers(const std::vector<MemoryHotspotDebugMarker>& markers,
                                                  float elapsedSeconds) {
    for (const MemoryHotspotDebugMarker& marker : markers) {
        const float pulse = 1.0f + std::sin(elapsedSeconds * 4.0f + marker.radius * 3.0f) * 0.08f;
        const float radius = marker.radius * pulse;
        const Vec3 base = marker.position + Vec3{0.0f, 0.09f, 0.0f};
        const Vec3 top = marker.position + Vec3{0.0f, 1.55f + std::sin(elapsedSeconds * 5.5f) * 0.05f, 0.0f};

        const Color markerColor = toColor(marker.color);
        DrawCylinder(toRay(base), radius, radius, 0.055f, 28, markerColor);
        DrawCylinderWires(toRay(base + Vec3{0.0f, 0.04f, 0.0f}), radius, radius, 0.06f, 28, Fade(RAYWHITE, 0.70f));
        DrawLine3D(toRay(base + Vec3{0.0f, 0.18f, 0.0f}), toRay(top), Fade(markerColor, 0.60f));
        DrawSphere(toRay(top), 0.12f + std::min(marker.score, 5.0f) * 0.015f, markerColor);
    }
}

void WorldRenderer::drawRewirPressureDebugMarkers(const std::vector<RewirPressureDebugMarker>& markers,
                                                  float elapsedSeconds) {
    for (const RewirPressureDebugMarker& marker : markers) {
        const float pulse = 1.0f + std::sin(elapsedSeconds * 3.4f + marker.radius) * 0.045f;
        const float radius = marker.radius * pulse;
        const Vec3 base = marker.position + Vec3{0.0f, 0.12f, 0.0f};
        const Vec3 top = marker.position + Vec3{0.0f, 2.05f + std::sin(elapsedSeconds * 4.8f) * 0.06f, 0.0f};

        const Color markerColor = toColor(marker.color);
        DrawCylinderWires(toRay(base), radius, radius, 0.08f, 40, markerColor);
        DrawCylinderWires(toRay(base + Vec3{0.0f, 0.06f, 0.0f}), radius * 0.74f, radius * 0.74f, 0.08f, 40,
                          Fade(markerColor, 0.72f));
        DrawLine3D(toRay(base + Vec3{0.0f, 0.22f, 0.0f}), toRay(top), Fade(markerColor, 0.82f));
        DrawSphereWires(toRay(top), 0.18f + std::min(marker.score, 5.0f) * 0.02f, 12, 8, markerColor);
    }
}

void WorldRenderer::drawObjectiveBeacon(const ObjectiveMarker& marker, float pulse, float elapsedSeconds) {
    if (!marker.active) {
        return;
    }

    const Vec3 top = marker.position + Vec3{0.0f, 2.35f + std::sin(elapsedSeconds * 5.0f) * 0.08f, 0.0f};
    const Color markerColor = toColor(marker.color);
    DrawSphere(toRay(top), 0.18f * pulse, markerColor);
    DrawCylinderWires(toRay(marker.position + Vec3{0.0f, 1.55f, 0.0f}), 0.34f * pulse, 0.34f * pulse, 0.05f, 24, markerColor);
    DrawLine3D(toRay(marker.position + Vec3{0.0f, 1.25f, 0.0f}), toRay(top), Fade(markerColor, 0.55f));
}

void WorldRenderer::drawPlayer(Vec3 position,
                               float yawRadians,
                               const PlayerPresentationState& pose,
                               float elapsedSeconds,
                               CharacterRenderOptions options) {
    const float staggerWobble = std::sin(elapsedSeconds * 31.0f) * pose.staggerAmount * 0.08f;
    const Vec3 poseOffset{pose.sway + staggerWobble, pose.bobHeight - pose.landingDip, 0.0f};
    const CharacterArtPalette& palette =
        options.palette != nullptr ? *options.palette : defaultCharacterArtPalette();

    rlPushMatrix();
    rlTranslatef(position.x + poseOffset.x, position.y + poseOffset.y, position.z);
    rlRotatef(radiansToDegrees(yawRadians), 0.0f, 1.0f, 0.0f);
    rlRotatef(radiansToDegrees(pose.leanRadians + pose.staggerAmount * 0.10f), 0.0f, 0.0f, 1.0f);

    for (const CharacterArtPart& part : characterArtModelSpec().parts) {
        if (!part.render || !shouldRenderCharacterPart(part.role, options)) {
            continue;
        }
        drawCharacterPartLocal(part, pose, elapsedSeconds, palette);
    }

    rlPopMatrix();
}

void WorldRenderer::drawCharacterCapsuleDebug(Vec3 position, RenderColor tint) {
    const Color rayTint = toColor(tint);
    const CharacterArtModelSpec& spec = characterArtModelSpec();
    const CharacterCapsuleDebugGeometry geometry =
        buildCharacterCapsuleDebugGeometry(position, spec.capsuleRadius, spec.capsuleHeight);

    const Color softTint = Fade(rayTint, 0.72f);
    DrawCylinderWires(toRay((geometry.baseCenter + geometry.topCenter) * 0.5f),
                      geometry.radius,
                      geometry.radius,
                      spec.capsuleHeight,
                      24,
                      softTint);
    for (float ringHeight : geometry.ringHeights) {
        DrawCylinderWires({position.x, ringHeight, position.z},
                          geometry.radius,
                          geometry.radius,
                          0.012f,
                          24,
                          rayTint);
    }

    const Vec3 offsets[] = {
        {geometry.radius, 0.0f, 0.0f},
        {-geometry.radius, 0.0f, 0.0f},
        {0.0f, 0.0f, geometry.radius},
        {0.0f, 0.0f, -geometry.radius},
    };
    for (const Vec3& offset : offsets) {
        DrawLine3D(toRay(geometry.baseCenter + offset), toRay(geometry.topCenter + offset), rayTint);
    }
}

void WorldRenderer::drawVehicle(Vec3 position, float yawRadians, RenderColor bodyColor) {
    VehicleRuntimeState state;
    state.position = position;
    state.yawRadians = yawRadians;
    drawVehicle(state, bodyColor);
}

void WorldRenderer::drawVehicle(const VehicleRuntimeState& state, RenderColor bodyColor) {
    drawVehicleOpaque(state, bodyColor);
    drawVehicleGlass(state);
}

void WorldRenderer::drawVehicleOpaque(const VehicleRuntimeState& state, RenderColor bodyColor) {
    const Color rayBodyColor = toColor(bodyColor);
    const Vec3 position = state.position;
    const float yawRadians = state.yawRadians;
    const float compression = std::clamp(state.suspensionCompression, 0.0f, 0.18f);
    const float bodyDrop = compression * 0.28f;
    const float wheelLift = compression * 0.08f;
    const float frontSteer = std::clamp(state.frontWheelSteerRadians, -0.55f, 0.55f);
    const float slipVisual = std::clamp(state.wheelSlipVisual, 0.0f, 1.0f);
    const VehicleLightVisualState lights = vehicleLightVisualState(state);

    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(radiansToDegrees(yawRadians), 0.0f, 1.0f, 0.0f);

    const VehicleArtModelSpec& spec = vehicleArtModelSpec();
    for (const VehicleArtPart& part : spec.parts) {
        if (!part.render || isAnimatedWheelPart(part) || part.material == VehicleArtMaterial::Glass) {
            continue;
        }
        if (part.shape == VehicleArtShape::CylinderX) {
            drawCylinderXPartLocal(part, rayBodyColor, bodyDrop);
        } else if (part.shape == VehicleArtShape::QuadPanel) {
            drawQuadPanelPartLocal(part, rayBodyColor, bodyDrop, &lights);
        } else {
            drawBoxPartLocal(part, rayBodyColor, bodyDrop, &lights);
        }
    }

    for (const VehicleArtPart& part : spec.parts) {
        if (!part.wheelPivot) {
            continue;
        }
        const float steer = part.frontWheel ? frontSteer : 0.0f;
        const float localSlip = part.frontWheel ? slipVisual : slipVisual * 0.85f;
        const float outward = part.position.x < 0.0f ? -1.0f : 1.0f;
        drawVehicleWheelLocal(part,
                              bodyDrop,
                              part.size.y * 0.5f,
                              wheelLift,
                              steer,
                              state.wheelSpinRadians,
                              localSlip,
                              outward);
    }

    rlPopMatrix();
}

void WorldRenderer::drawVehicleGlass(const VehicleRuntimeState& state) {
    const float compression = std::clamp(state.suspensionCompression, 0.0f, 0.18f);
    const float bodyDrop = compression * 0.28f;
    const GlassVisualState vehicleGlass = vehicleGlassVisualState(state.condition, 0.0f);
    const Color vehicleGlassTint{vehicleGlass.tint.r, vehicleGlass.tint.g, vehicleGlass.tint.b, vehicleGlass.tint.a};
    const VehicleArtModelSpec& spec = vehicleArtModelSpec();

    rlPushMatrix();
    rlTranslatef(state.position.x, state.position.y, state.position.z);
    rlRotatef(radiansToDegrees(state.yawRadians), 0.0f, 1.0f, 0.0f);
    BeginBlendMode(BLEND_ALPHA);
    for (const VehicleArtPart& part : spec.parts) {
        if (!part.render || part.material != VehicleArtMaterial::Glass) {
            continue;
        }
        if (part.shape == VehicleArtShape::QuadPanel) {
            drawGlassQuadPanelPartLocal(part, vehicleGlassTint, bodyDrop, vehicleGlass);
        } else {
            drawGlassBoxPartLocal(part, vehicleGlassTint, bodyDrop, vehicleGlass);
        }
    }
    EndBlendMode();
    rlPopMatrix();
}

void WorldRenderer::drawVehicleSmoke(const VehicleController& vehicle, float elapsedSeconds) {
    drawVehicleSmoke(vehicle.runtimeState(), elapsedSeconds);
}

void WorldRenderer::drawVehicleSmoke(const VehicleRuntimeState& state, float elapsedSeconds) {
    const float conditionSmoke = std::clamp((100.0f - state.condition) / 100.0f, 0.0f, 1.0f) * 0.85f;
    const float boostSmoke = state.boostActive ? 0.30f : 0.0f;
    const float instabilitySmoke = state.instability * 0.35f;
    const float smokeAmount = std::max(conditionSmoke, std::max(boostSmoke, instabilitySmoke));
    if (smokeAmount <= 0.02f) {
        return;
    }

    const Vec3 forward = forwardFromYaw(state.yawRadians);
    const Vec3 right = screenRightFromYaw(state.yawRadians);
    for (int i = 0; i < 4; ++i) {
        const float phase = elapsedSeconds * (2.2f + static_cast<float>(i) * 0.3f) + static_cast<float>(i);
        const Vec3 puff = state.position - forward * (1.55f + static_cast<float>(i) * 0.18f) +
                          right * (std::sin(phase) * 0.12f) +
                          Vec3{0.0f, 0.50f + static_cast<float>(i) * 0.12f, 0.0f};
        DrawSphere(toRay(puff), 0.18f + smokeAmount * 0.22f, Fade(color(74, 72, 68), 0.18f + smokeAmount * 0.25f));
    }

    if (state.hornPulse > 0.01f) {
        const float radius = 2.2f + state.hornPulse * 1.1f;
        DrawCylinderWires(toRay(state.position + Vec3{0.0f, 0.12f, 0.0f}), radius, radius, 0.08f, 28,
                          color(238, 211, 79, static_cast<unsigned char>(120.0f * state.hornPulse)));
    }
}

} // namespace bs3d
