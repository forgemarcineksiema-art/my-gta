#pragma once

#include "bs3d/core/Types.h"

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace bs3d {

enum class CollisionLayer : std::uint32_t {
    None = 0,
    WorldStatic = 1u << 0,
    PlayerBlocker = 1u << 1,
    VehicleBlocker = 1u << 2,
    CameraBlocker = 1u << 3,
    InteractionTrigger = 1u << 4,
    WorldEventTrigger = 1u << 5,
    WalkableSurface = 1u << 6,
    DynamicProp = 1u << 7,
    NoCollision = 1u << 8
};

using CollisionLayerMask = std::uint32_t;

constexpr CollisionLayerMask collisionLayerMask(CollisionLayer layer) {
    return static_cast<CollisionLayerMask>(layer);
}

constexpr bool hasCollisionLayer(CollisionLayerMask mask, CollisionLayer layer) {
    return (mask & collisionLayerMask(layer)) != 0;
}

namespace CollisionMasks {
constexpr CollisionLayerMask None = 0;
constexpr CollisionLayerMask Player = collisionLayerMask(CollisionLayer::WorldStatic) |
                                      collisionLayerMask(CollisionLayer::PlayerBlocker);
constexpr CollisionLayerMask Vehicle = collisionLayerMask(CollisionLayer::WorldStatic) |
                                       collisionLayerMask(CollisionLayer::VehicleBlocker) |
                                       collisionLayerMask(CollisionLayer::DynamicProp);
constexpr CollisionLayerMask Camera = collisionLayerMask(CollisionLayer::WorldStatic) |
                                      collisionLayerMask(CollisionLayer::CameraBlocker);
constexpr CollisionLayerMask Walkable = collisionLayerMask(CollisionLayer::WalkableSurface);
constexpr CollisionLayerMask Trigger = collisionLayerMask(CollisionLayer::InteractionTrigger) |
                                       collisionLayerMask(CollisionLayer::WorldEventTrigger);
} // namespace CollisionMasks

enum class CollisionResponseKind {
    Unspecified,
    None,
    StaticSolid,
    Walkable,
    TriggerOnly,
    PropStatic,
    PropFakeDynamic,
    PropDynamicLite,
    BreakableLite
};

struct CollisionProfile {
    CollisionLayerMask layers = CollisionMasks::None;
    CollisionResponseKind responseKind = CollisionResponseKind::Unspecified;
    bool isTrigger = false;
    bool isWalkable = false;
    bool blocksCamera = false;
    int ownerId = 0;
    Vec3 platformVelocity{};

    static CollisionProfile noCollision();
    static CollisionProfile solidWorld();
    static CollisionProfile playerBlocker();
    static CollisionProfile vehicleBlocker();
    static CollisionProfile cameraBlocker();
    static CollisionProfile walkableSurface();
    static CollisionProfile dynamicProp();
    static CollisionProfile trigger(CollisionLayer triggerLayer);
};

struct CollisionBox {
    Vec3 center{};
    Vec3 size{1.0f, 1.0f, 1.0f};
    CollisionProfile profile = CollisionProfile::solidWorld();
};

struct OrientedCollisionBox {
    Vec3 center{};
    Vec3 size{1.0f, 1.0f, 1.0f};
    float yawRadians = 0.0f;
    CollisionProfile profile = CollisionProfile::solidWorld();
};

struct GroundHit {
    bool hit = false;
    float height = 0.0f;
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float slopeDegrees = 0.0f;
    bool walkable = true;
    int ownerId = 0;
    Vec3 platformVelocity{};
};

struct CharacterCollisionConfig {
    float radius = 0.45f;
    float skinWidth = 0.05f;
    float stepHeight = 0.35f;
    float walkableSlopeDegrees = 38.0f;
    float groundProbeDistance = 0.65f;
    int ignoreBlockerOwnerId = 0;
};

struct CharacterCollisionResult {
    Vec3 position{};
    bool hitWall = false;
    bool stepped = false;
    bool blockedByStep = false;
    bool blockedBySlope = false;
    int hitOwnerId = 0;
    CollisionProfile hitProfile{};
};

struct VehicleCollisionConfig {
    float radius = 0.90f;
    float halfLength = 0.75f;
    float speed = 0.0f;
    float lateralSlip = 0.0f;
    Vec3 velocity{};
};

struct VehiclePlayerCollisionConfig {
    Vec3 bodySize{1.90f, 0.78f, 3.30f};
    Vec3 topSize{1.70f, 0.06f, 2.95f};
    float topHeight = 0.92f;
    float sideThickness = 0.18f;
    Vec3 platformVelocity{};
    int ownerId = 7001;
};

enum class VehicleHitZone {
    None,
    Front,
    Rear,
    LeftSide,
    RightSide,
    Center
};

struct VehicleCollisionResult {
    Vec3 position{};
    std::array<Vec3, 3> circles{};
    bool hit = false;
    bool hitFront = false;
    bool hitCenter = false;
    bool hitRear = false;
    int hitObjectId = 0;
    Vec3 normal{};
    CollisionProfile collisionProfile{};
    VehicleHitZone hitZone = VehicleHitZone::None;
    Vec3 impactNormal{};
    float impactSpeed = 0.0f;
    CollisionProfile hitProfile{};
};

struct CollisionBroadphaseStats {
    int staticShapeCount = 0;
    int dynamicShapeCount = 0;
    int staticBucketCount = 0;
    int dynamicBucketCount = 0;
};

struct GroundSurfaceDebug {
    Vec3 center{};
    Vec3 size{};
    float yawRadians = 0.0f;
    float height = 0.0f;
    CollisionProfile profile = CollisionProfile::walkableSurface();
};

struct CollisionDebugSnapshot {
    std::vector<CollisionBox> staticBoxes;
    std::vector<OrientedCollisionBox> staticOrientedBoxes;
    std::vector<CollisionBox> dynamicBoxes;
    std::vector<OrientedCollisionBox> dynamicOrientedBoxes;
    std::vector<GroundSurfaceDebug> groundSurfaces;
};

class WorldCollision {
public:
    WorldCollision();

    void clear();
    void clearDynamic();
    void addBox(Vec3 center, Vec3 size);
    void addBox(Vec3 center, Vec3 size, CollisionProfile profile);
    void addOrientedBox(Vec3 center, Vec3 size, float yawRadians, CollisionProfile profile);
    void addGroundPlane(float height);
    void addGroundBox(Vec3 center, Vec3 size);
    void addGroundBox(Vec3 center, Vec3 size, CollisionProfile profile);
    void addGroundBox(Vec3 center, Vec3 size, float yawRadians, CollisionProfile profile);
    void addRamp(Vec3 center, Vec3 size, float startHeight, float endHeight);
    void addRamp(Vec3 center, Vec3 size, float startHeight, float endHeight, CollisionProfile profile);
    void addDynamicBox(Vec3 center, Vec3 size, CollisionProfile profile);
    void addDynamicOrientedBox(Vec3 center, Vec3 size, float yawRadians, CollisionProfile profile);
    void addDynamicGroundBox(Vec3 center, Vec3 size, float yawRadians, CollisionProfile profile);
    void addDynamicVehiclePlayerCollision(Vec3 position,
                                          float yawRadians,
                                          const VehiclePlayerCollisionConfig& config = {});

    bool isCircleBlocked(Vec3 center, float radius) const;
    bool isCircleBlocked(Vec3 center, float radius, CollisionLayerMask mask) const;
    Vec3 resolveCircle(Vec3 from, Vec3 desired, float radius) const;
    Vec3 resolveCircle(Vec3 from, Vec3 desired, float radius, CollisionLayerMask mask) const;
    VehicleCollisionResult resolveVehicleCapsule(Vec3 from,
                                                 Vec3 desired,
                                                 float yawRadians,
                                                 const VehicleCollisionConfig& config) const;
    GroundHit probeGround(Vec3 position, float maxDistance, float walkableSlopeDegrees) const;
    CharacterCollisionResult resolveCharacter(Vec3 from, Vec3 desired, const CharacterCollisionConfig& config) const;
    Vec3 resolveCameraBoom(Vec3 target, Vec3 desiredCamera, float radius) const;
    const std::vector<CollisionBox>& boxes() const;
    CollisionDebugSnapshot debugSnapshot() const;
    CollisionBroadphaseStats broadphaseStats() const;
    int debugBroadphaseCandidateCount(Vec3 center, float radius, CollisionLayerMask mask) const;
    int debugBroadphaseSegmentCandidateCount(Vec3 start,
                                             Vec3 end,
                                             float radius,
                                             CollisionLayerMask mask) const;

private:
    enum class GroundType {
        Plane,
        Box,
        RampZ
    };

    struct GroundSurface {
        GroundType type = GroundType::Plane;
        Vec3 center{};
        Vec3 size{1.0f, 0.0f, 1.0f};
        float yawRadians = 0.0f;
        float startHeight = 0.0f;
        float endHeight = 0.0f;
        CollisionProfile profile = CollisionProfile::walkableSurface();
    };

    std::vector<CollisionBox> boxes_;
    std::vector<OrientedCollisionBox> orientedBoxes_;
    std::vector<CollisionBox> dynamicBoxes_;
    std::vector<OrientedCollisionBox> dynamicOrientedBoxes_;
    std::vector<GroundSurface> groundSurfaces_;
    std::vector<GroundSurface> dynamicGroundSurfaces_;
    std::unordered_map<long long, std::vector<int>> staticBoxBuckets_;
    std::unordered_map<long long, std::vector<int>> staticOrientedBoxBuckets_;
    std::unordered_map<long long, std::vector<int>> dynamicBoxBuckets_;
    std::unordered_map<long long, std::vector<int>> dynamicOrientedBoxBuckets_;

    CharacterCollisionResult resolveCharacterStep(Vec3 from, Vec3 desired, const CharacterCollisionConfig& config) const;
    bool isCharacterCircleBlocked(Vec3 center, float radius) const;
    bool isCharacterCircleBlocked(Vec3 center, float radius, int ignoreOwnerId) const;
};

} // namespace bs3d
