#pragma once

#include "bs3d/core/Types.h"
#include "bs3d/core/WorldCollision.h"
#include "bs3d/core/WorldEventLedger.h"

#include <string>
#include <vector>

namespace bs3d {

enum class WorldCollisionShapeKind {
    Unspecified,
    None,
    Box,
    GroundBox,
    RampZ,
    OrientedBox,
    TriggerSphere,
    TriggerBox
};

struct WorldCollisionShape {
    WorldCollisionShapeKind kind = WorldCollisionShapeKind::Unspecified;
    Vec3 offset{};
    Vec3 size{1.0f, 1.0f, 1.0f};
    float yawRadians = 0.0f;
    float startHeight = 0.0f;
    float endHeight = 0.0f;
};

struct WorldObjectTint {
    unsigned char r = 255;
    unsigned char g = 255;
    unsigned char b = 255;
    unsigned char a = 255;
};

struct WorldProp {
    Vec3 center{};
    Vec3 size{};
    WorldObjectTint fill{};
    bool collidable = true;
};

struct WorldObject {
    std::string id;
    std::string assetId;
    Vec3 position{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
    float yawRadians = 0.0f;
    WorldCollisionShape collision{};
    WorldLocationTag zoneTag = WorldLocationTag::Unknown;
    std::vector<std::string> gameplayTags;
    bool hasTintOverride = false;
    WorldObjectTint tintOverride{};
    CollisionProfile collisionProfile{};
};

enum class WorldZoneShapeKind {
    Sphere,
    Box
};

struct WorldZone {
    std::string id;
    WorldLocationTag tag = WorldLocationTag::Unknown;
    Vec3 center{};
    Vec3 size{1.0f, 1.0f, 1.0f};
    float radius = 1.0f;
    int priority = 0;
    WorldZoneShapeKind shape = WorldZoneShapeKind::Sphere;
};

struct WorldLandmark {
    std::string id;
    std::string label;
    Vec3 position{};
    std::string role;
};

struct WorldViewpoint {
    std::string id;
    std::string label;
    Vec3 position{};
    Vec3 target{};
};

} // namespace bs3d
