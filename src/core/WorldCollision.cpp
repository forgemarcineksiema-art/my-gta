#include "bs3d/core/WorldCollision.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace bs3d {

CollisionProfile CollisionProfile::noCollision() {
    return {collisionLayerMask(CollisionLayer::NoCollision), CollisionResponseKind::None, false, false, false, 0};
}

CollisionProfile CollisionProfile::solidWorld() {
    return {collisionLayerMask(CollisionLayer::WorldStatic) |
                collisionLayerMask(CollisionLayer::PlayerBlocker) |
                collisionLayerMask(CollisionLayer::VehicleBlocker) |
                collisionLayerMask(CollisionLayer::CameraBlocker),
            CollisionResponseKind::StaticSolid,
            false,
            false,
            true,
            0};
}

CollisionProfile CollisionProfile::playerBlocker() {
    return {collisionLayerMask(CollisionLayer::PlayerBlocker),
            CollisionResponseKind::StaticSolid,
            false,
            false,
            false,
            0};
}

CollisionProfile CollisionProfile::vehicleBlocker() {
    return {collisionLayerMask(CollisionLayer::VehicleBlocker),
            CollisionResponseKind::StaticSolid,
            false,
            false,
            false,
            0};
}

CollisionProfile CollisionProfile::cameraBlocker() {
    return {collisionLayerMask(CollisionLayer::CameraBlocker),
            CollisionResponseKind::StaticSolid,
            false,
            false,
            true,
            0};
}

CollisionProfile CollisionProfile::walkableSurface() {
    return {collisionLayerMask(CollisionLayer::WalkableSurface),
            CollisionResponseKind::Walkable,
            false,
            true,
            false,
            0};
}

CollisionProfile CollisionProfile::dynamicProp() {
    return {collisionLayerMask(CollisionLayer::DynamicProp) |
                collisionLayerMask(CollisionLayer::PlayerBlocker) |
                collisionLayerMask(CollisionLayer::VehicleBlocker),
            CollisionResponseKind::PropDynamicLite,
            false,
            false,
            false,
            0};
}

CollisionProfile CollisionProfile::trigger(CollisionLayer triggerLayer) {
    return {collisionLayerMask(triggerLayer),
            CollisionResponseKind::TriggerOnly,
            true,
            false,
            false,
            0};
}

namespace {

constexpr float VehicleTopSupportMargin = 0.12f;
constexpr float BroadphaseCellSize = 8.0f;

float minAbs(float a, float b) {
    return std::fabs(a) < std::fabs(b) ? a : b;
}

float degrees(float radians) {
    return radians * 180.0f / Pi;
}

float length(Vec3 value) {
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

Vec3 normalize(Vec3 value) {
    const float len = length(value);
    if (len <= 0.0001f) {
        return {};
    }
    return value / len;
}

bool inRectXZ(Vec3 point, Vec3 center, Vec3 size) {
    return point.x >= center.x - size.x * 0.5f && point.x <= center.x + size.x * 0.5f &&
           point.z >= center.z - size.z * 0.5f && point.z <= center.z + size.z * 0.5f;
}

bool profileMatches(const CollisionProfile& profile, CollisionLayerMask mask) {
    return (profile.layers & mask) != 0 && !hasCollisionLayer(profile.layers, CollisionLayer::NoCollision) &&
           !profile.isTrigger;
}

int broadphaseCell(float value) {
    return static_cast<int>(std::floor(value / BroadphaseCellSize));
}

long long broadphaseKey(int x, int z) {
    const auto ux = static_cast<std::uint32_t>(x);
    const auto uz = static_cast<std::uint32_t>(z);
    const auto key = (static_cast<std::uint64_t>(ux) << 32) | static_cast<std::uint64_t>(uz);
    return static_cast<long long>(key);
}

void addAabbToBuckets(std::unordered_map<long long, std::vector<int>>& buckets,
                      Vec3 center,
                      Vec3 size,
                      int index) {
    const float halfX = std::fabs(size.x) * 0.5f;
    const float halfZ = std::fabs(size.z) * 0.5f;
    const int minX = broadphaseCell(center.x - halfX);
    const int maxX = broadphaseCell(center.x + halfX);
    const int minZ = broadphaseCell(center.z - halfZ);
    const int maxZ = broadphaseCell(center.z + halfZ);
    for (int x = minX; x <= maxX; ++x) {
        for (int z = minZ; z <= maxZ; ++z) {
            buckets[broadphaseKey(x, z)].push_back(index);
        }
    }
}

std::vector<int> gatherBroadphaseCandidates(const std::unordered_map<long long, std::vector<int>>& buckets,
                                            Vec3 center,
                                            float radius) {
    std::vector<int> candidates;
    std::unordered_set<int> seen;
    const int minX = broadphaseCell(center.x - radius);
    const int maxX = broadphaseCell(center.x + radius);
    const int minZ = broadphaseCell(center.z - radius);
    const int maxZ = broadphaseCell(center.z + radius);
    for (int x = minX; x <= maxX; ++x) {
        for (int z = minZ; z <= maxZ; ++z) {
            const auto found = buckets.find(broadphaseKey(x, z));
            if (found == buckets.end()) {
                continue;
            }
            for (int index : found->second) {
                if (seen.insert(index).second) {
                    candidates.push_back(index);
                }
            }
        }
    }
    return candidates;
}

std::vector<int> gatherBroadphaseSegmentCandidates(const std::unordered_map<long long, std::vector<int>>& buckets,
                                                   Vec3 start,
                                                   Vec3 end,
                                                   float radius) {
    const Vec3 center = (start + end) * 0.5f;
    const float halfX = std::fabs(end.x - start.x) * 0.5f + radius;
    const float halfZ = std::fabs(end.z - start.z) * 0.5f + radius;
    const float queryRadius = std::sqrt(halfX * halfX + halfZ * halfZ);
    return gatherBroadphaseCandidates(buckets, center, queryRadius);
}

Vec3 rotateY(Vec3 value, float yawRadians) {
    const float c = std::cos(yawRadians);
    const float s = std::sin(yawRadians);
    return {value.x * c - value.z * s, value.y, value.x * s + value.z * c};
}

bool inOrientedRectXZ(Vec3 point, Vec3 center, Vec3 size, float yawRadians) {
    const Vec3 local = rotateY(point - center, -yawRadians);
    return std::fabs(local.x) <= size.x * 0.5f && std::fabs(local.z) <= size.z * 0.5f;
}

bool verticallyOverlapsFoot(Vec3 center, const CollisionBox& box) {
    const float top = box.center.y + box.size.y * 0.5f;
    const float bottom = box.center.y - box.size.y * 0.5f;
    return center.y < top - 0.02f && center.y > bottom - 0.35f;
}

bool verticallyOverlapsFoot(Vec3 center, const OrientedCollisionBox& box) {
    const float top = box.center.y + box.size.y * 0.5f;
    const float bottom = box.center.y - box.size.y * 0.5f;
    return center.y < top - 0.02f && center.y > bottom - 0.35f;
}

bool circleBlockedByBox(Vec3 center, float radius, const CollisionBox& box, bool useVertical) {
    if (useVertical && !verticallyOverlapsFoot(center, box)) {
        return false;
    }
    const float halfX = box.size.x * 0.5f;
    const float halfZ = box.size.z * 0.5f;
    const float minX = box.center.x - halfX;
    const float maxX = box.center.x + halfX;
    const float minZ = box.center.z - halfZ;
    const float maxZ = box.center.z + halfZ;

    const float nearestX = std::clamp(center.x, minX, maxX);
    const float nearestZ = std::clamp(center.z, minZ, maxZ);
    const float dx = center.x - nearestX;
    const float dz = center.z - nearestZ;

    return (dx * dx + dz * dz) < radius * radius ||
           (center.x >= minX && center.x <= maxX && center.z >= minZ && center.z <= maxZ);
}

bool circleBlockedByOrientedBox(Vec3 center, float radius, const OrientedCollisionBox& box, bool useVertical) {
    if (useVertical && !verticallyOverlapsFoot(center, box)) {
        return false;
    }

    const Vec3 local = rotateY(center - box.center, -box.yawRadians);
    const float halfX = box.size.x * 0.5f;
    const float halfZ = box.size.z * 0.5f;
    const float nearestX = std::clamp(local.x, -halfX, halfX);
    const float nearestZ = std::clamp(local.z, -halfZ, halfZ);
    const float dx = local.x - nearestX;
    const float dz = local.z - nearestZ;

    return (dx * dx + dz * dz) < radius * radius ||
           (local.x >= -halfX && local.x <= halfX && local.z >= -halfZ && local.z <= halfZ);
}

std::array<Vec3, 3> vehicleCircles(Vec3 center, float yawRadians, float halfLength) {
    const Vec3 forward = forwardFromYaw(yawRadians);
    return {center + forward * halfLength, center, center - forward * halfLength};
}

bool segmentAabb(Vec3 origin, Vec3 direction, float maxDistance, const CollisionBox& box, float radius, float& hitDistance) {
    const Vec3 half = box.size * 0.5f + Vec3{radius, radius, radius};
    const Vec3 min = box.center - half;
    const Vec3 max = box.center + half;
    float tMin = 0.0f;
    float tMax = maxDistance;

    const auto clipAxis = [&](float start, float dir, float axisMin, float axisMax) {
        if (std::fabs(dir) <= 0.0001f) {
            return start >= axisMin && start <= axisMax;
        }

        float t1 = (axisMin - start) / dir;
        float t2 = (axisMax - start) / dir;
        if (t1 > t2) {
            std::swap(t1, t2);
        }
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        return tMin <= tMax;
    };

    if (!clipAxis(origin.x, direction.x, min.x, max.x)) {
        return false;
    }
    if (!clipAxis(origin.y, direction.y, min.y, max.y)) {
        return false;
    }
    if (!clipAxis(origin.z, direction.z, min.z, max.z)) {
        return false;
    }

    hitDistance = tMin;
    return hitDistance >= 0.0f && hitDistance <= maxDistance;
}

bool segmentObb(Vec3 origin,
                Vec3 direction,
                float maxDistance,
                const OrientedCollisionBox& box,
                float radius,
                float& hitDistance) {
    const Vec3 localOrigin = rotateY(origin - box.center, -box.yawRadians);
    const Vec3 localDirection = rotateY(direction, -box.yawRadians);
    const CollisionBox localBox{{}, box.size, box.profile};
    return segmentAabb(localOrigin, localDirection, maxDistance, localBox, radius, hitDistance);
}

} // namespace

WorldCollision::WorldCollision() {
    addGroundPlane(0.0f);
}

void WorldCollision::clear() {
    boxes_.clear();
    orientedBoxes_.clear();
    dynamicBoxes_.clear();
    dynamicOrientedBoxes_.clear();
    groundSurfaces_.clear();
    dynamicGroundSurfaces_.clear();
    staticBoxBuckets_.clear();
    staticOrientedBoxBuckets_.clear();
    dynamicBoxBuckets_.clear();
    dynamicOrientedBoxBuckets_.clear();
}

void WorldCollision::clearDynamic() {
    dynamicBoxes_.clear();
    dynamicOrientedBoxes_.clear();
    dynamicGroundSurfaces_.clear();
    dynamicBoxBuckets_.clear();
    dynamicOrientedBoxBuckets_.clear();
}

void WorldCollision::addBox(Vec3 center, Vec3 size) {
    addBox(center, size, CollisionProfile::solidWorld());
}

void WorldCollision::addBox(Vec3 center, Vec3 size, CollisionProfile profile) {
    boxes_.push_back({center, size, profile});
    addAabbToBuckets(staticBoxBuckets_, center, size, static_cast<int>(boxes_.size() - 1));
}

void WorldCollision::addOrientedBox(Vec3 center, Vec3 size, float yawRadians, CollisionProfile profile) {
    orientedBoxes_.push_back({center, size, yawRadians, profile});
    const float radius = std::sqrt(size.x * size.x + size.z * size.z);
    addAabbToBuckets(staticOrientedBoxBuckets_, center, {radius, size.y, radius}, static_cast<int>(orientedBoxes_.size() - 1));
}

void WorldCollision::addGroundPlane(float height) {
    groundSurfaces_.push_back({GroundType::Plane, {}, {}, 0.0f, height, height, CollisionProfile::walkableSurface()});
}

void WorldCollision::addGroundBox(Vec3 center, Vec3 size) {
    addGroundBox(center, size, CollisionProfile::walkableSurface());
}

void WorldCollision::addGroundBox(Vec3 center, Vec3 size, CollisionProfile profile) {
    addGroundBox(center, size, 0.0f, profile);
}

void WorldCollision::addGroundBox(Vec3 center, Vec3 size, float yawRadians, CollisionProfile profile) {
    profile.isWalkable = true;
    profile.layers |= collisionLayerMask(CollisionLayer::WalkableSurface);
    groundSurfaces_.push_back({GroundType::Box, center, size, yawRadians, center.y + size.y, center.y + size.y, profile});
}

void WorldCollision::addRamp(Vec3 center, Vec3 size, float startHeight, float endHeight) {
    addRamp(center, size, startHeight, endHeight, CollisionProfile::walkableSurface());
}

void WorldCollision::addRamp(Vec3 center, Vec3 size, float startHeight, float endHeight, CollisionProfile profile) {
    profile.isWalkable = true;
    profile.layers |= collisionLayerMask(CollisionLayer::WalkableSurface);
    groundSurfaces_.push_back({GroundType::RampZ, center, size, 0.0f, startHeight, endHeight, profile});
}

void WorldCollision::addDynamicBox(Vec3 center, Vec3 size, CollisionProfile profile) {
    dynamicBoxes_.push_back({center, size, profile});
    addAabbToBuckets(dynamicBoxBuckets_, center, size, static_cast<int>(dynamicBoxes_.size() - 1));
}

void WorldCollision::addDynamicOrientedBox(Vec3 center, Vec3 size, float yawRadians, CollisionProfile profile) {
    dynamicOrientedBoxes_.push_back({center, size, yawRadians, profile});
    const float radius = std::sqrt(size.x * size.x + size.z * size.z);
    addAabbToBuckets(dynamicOrientedBoxBuckets_, center, {radius, size.y, radius}, static_cast<int>(dynamicOrientedBoxes_.size() - 1));
}

void WorldCollision::addDynamicGroundBox(Vec3 center, Vec3 size, float yawRadians, CollisionProfile profile) {
    profile.isWalkable = true;
    profile.layers |= collisionLayerMask(CollisionLayer::WalkableSurface);
    dynamicGroundSurfaces_.push_back(
        {GroundType::Box, center, size, yawRadians, center.y + size.y, center.y + size.y, profile});
}

void WorldCollision::addDynamicVehiclePlayerCollision(Vec3 position,
                                                      float yawRadians,
                                                      const VehiclePlayerCollisionConfig& config) {
    CollisionProfile bodyProfile = CollisionProfile::playerBlocker();
    bodyProfile.layers |= collisionLayerMask(CollisionLayer::VehicleBlocker);
    bodyProfile.responseKind = CollisionResponseKind::StaticSolid;
    bodyProfile.ownerId = config.ownerId;

    const float width = std::max(config.bodySize.x, config.sideThickness * 2.0f);
    const float length = std::max(config.bodySize.z, config.sideThickness * 2.0f);
    const float height = std::max(config.bodySize.y, 0.05f);
    const float sideThickness = std::clamp(config.sideThickness, 0.05f, std::min(width, length) * 0.45f);
    const float halfWidth = width * 0.5f;
    const float halfLength = length * 0.5f;
    const float sideOffset = halfWidth - sideThickness * 0.5f;
    const float endOffset = halfLength - sideThickness * 0.5f;

    const auto localToWorld = [&](Vec3 local) {
        return position + rotateY(local, yawRadians);
    };

    addDynamicOrientedBox(localToWorld({sideOffset, height * 0.5f, 0.0f}),
                          {sideThickness, height, length},
                          yawRadians,
                          bodyProfile);
    addDynamicOrientedBox(localToWorld({-sideOffset, height * 0.5f, 0.0f}),
                          {sideThickness, height, length},
                          yawRadians,
                          bodyProfile);
    addDynamicOrientedBox(localToWorld({0.0f, height * 0.5f, endOffset}),
                          {width, height, sideThickness},
                          yawRadians,
                          bodyProfile);
    addDynamicOrientedBox(localToWorld({0.0f, height * 0.5f, -endOffset}),
                          {width, height, sideThickness},
                          yawRadians,
                          bodyProfile);

    const float topThickness = std::max(config.topSize.y, 0.03f);
    const Vec3 topSupportSize{
        config.topSize.x + VehicleTopSupportMargin * 2.0f,
        topThickness,
        config.topSize.z + VehicleTopSupportMargin * 2.0f,
    };
    CollisionProfile topProfile = CollisionProfile::walkableSurface();
    topProfile.ownerId = config.ownerId;
    topProfile.platformVelocity = config.platformVelocity;
    addDynamicGroundBox(position + Vec3{0.0f, config.topHeight - topThickness, 0.0f},
                        topSupportSize,
                        yawRadians,
                        topProfile);
}

bool WorldCollision::isCircleBlocked(Vec3 center, float radius) const {
    return isCircleBlocked(center, radius, CollisionMasks::Player | CollisionMasks::Vehicle | CollisionMasks::Camera);
}

bool WorldCollision::isCircleBlocked(Vec3 center, float radius, CollisionLayerMask mask) const {
    for (int index : gatherBroadphaseCandidates(staticBoxBuckets_, center, radius)) {
        const CollisionBox& box = boxes_[static_cast<std::size_t>(index)];
        if (profileMatches(box.profile, mask) && circleBlockedByBox(center, radius, box, false)) {
            return true;
        }
    }
    for (int index : gatherBroadphaseCandidates(staticOrientedBoxBuckets_, center, radius)) {
        const OrientedCollisionBox& box = orientedBoxes_[static_cast<std::size_t>(index)];
        if (profileMatches(box.profile, mask) && circleBlockedByOrientedBox(center, radius, box, false)) {
            return true;
        }
    }
    for (int index : gatherBroadphaseCandidates(dynamicBoxBuckets_, center, radius)) {
        const CollisionBox& box = dynamicBoxes_[static_cast<std::size_t>(index)];
        if (profileMatches(box.profile, mask) && circleBlockedByBox(center, radius, box, false)) {
            return true;
        }
    }
    for (int index : gatherBroadphaseCandidates(dynamicOrientedBoxBuckets_, center, radius)) {
        const OrientedCollisionBox& box = dynamicOrientedBoxes_[static_cast<std::size_t>(index)];
        if (profileMatches(box.profile, mask) && circleBlockedByOrientedBox(center, radius, box, false)) {
            return true;
        }
    }

    return false;
}

Vec3 WorldCollision::resolveCircle(Vec3 from, Vec3 desired, float radius) const {
    return resolveCircle(from, desired, radius, CollisionMasks::Player | CollisionMasks::Vehicle | CollisionMasks::Camera);
}

Vec3 WorldCollision::resolveCircle(Vec3 from, Vec3 desired, float radius, CollisionLayerMask mask) const {
    const Vec3 delta = desired - from;
    const float travelDistance = length(delta);
    const float maxStepDistance = std::max(0.05f, radius * 0.75f);
    const int steps = std::max(1, std::min(96, static_cast<int>(std::ceil(travelDistance / maxStepDistance))));

    if (steps > 1 && !isCircleBlocked(from, radius, mask)) {
        Vec3 current = from;
        for (int step = 1; step <= steps; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(steps);
            const Vec3 stepDesired = from + delta * t;
            const Vec3 next = resolveCircle(current, stepDesired, radius, mask);
            const bool madeProgress = distanceSquaredXZ(next, current) > 0.000001f;
            current = next;
            if (!madeProgress && isCircleBlocked(stepDesired, radius, mask)) {
                break;
            }
        }

        return current;
    }

    if (!isCircleBlocked(desired, radius, mask)) {
        return desired;
    }

    Vec3 resolved = desired;
    const auto resolveAgainstBox = [&](const CollisionBox& box) {
        if (!profileMatches(box.profile, mask)) {
            return;
        }
        const float halfX = box.size.x * 0.5f;
        const float halfZ = box.size.z * 0.5f;
        const float minX = box.center.x - halfX;
        const float maxX = box.center.x + halfX;
        const float minZ = box.center.z - halfZ;
        const float maxZ = box.center.z + halfZ;
        const float expandedHalfX = halfX + radius;
        const float expandedHalfZ = halfZ + radius;
        const float localX = resolved.x - box.center.x;
        const float localZ = resolved.z - box.center.z;

        if (std::fabs(localX) <= expandedHalfX && std::fabs(localZ) <= expandedHalfZ) {
            if (from.x <= minX - radius) {
                resolved.x = minX - radius;
                return;
            }

            if (from.x >= maxX + radius) {
                resolved.x = maxX + radius;
                return;
            }

            if (from.z <= minZ - radius) {
                resolved.z = minZ - radius;
                return;
            }

            if (from.z >= maxZ + radius) {
                resolved.z = maxZ + radius;
                return;
            }

            const float pushLeft = -expandedHalfX - localX;
            const float pushRight = expandedHalfX - localX;
            const float pushDown = -expandedHalfZ - localZ;
            const float pushUp = expandedHalfZ - localZ;
            const float pushX = minAbs(pushLeft, pushRight);
            const float pushZ = minAbs(pushDown, pushUp);

            if (std::fabs(pushX) < std::fabs(pushZ)) {
                resolved.x += pushX;
            } else {
                resolved.z += pushZ;
            }
        }
    };

    for (int index : gatherBroadphaseCandidates(staticBoxBuckets_, resolved, radius)) {
        resolveAgainstBox(boxes_[static_cast<std::size_t>(index)]);
    }
    for (int index : gatherBroadphaseCandidates(dynamicBoxBuckets_, resolved, radius)) {
        resolveAgainstBox(dynamicBoxes_[static_cast<std::size_t>(index)]);
    }
    for (int index : gatherBroadphaseCandidates(staticOrientedBoxBuckets_, resolved, radius)) {
        const OrientedCollisionBox& box = orientedBoxes_[static_cast<std::size_t>(index)];
        if (profileMatches(box.profile, mask) && circleBlockedByOrientedBox(resolved, radius, box, false)) {
            const Vec3 local = rotateY(resolved - box.center, -box.yawRadians);
            const float halfX = box.size.x * 0.5f;
            const float halfZ = box.size.z * 0.5f;
            const float expandedHalfX = halfX + radius;
            const float expandedHalfZ = halfZ + radius;
            const float pushLeft = -expandedHalfX - local.x;
            const float pushRight = expandedHalfX - local.x;
            const float pushDown = -expandedHalfZ - local.z;
            const float pushUp = expandedHalfZ - local.z;
            Vec3 localPush{};
            if (std::fabs(minAbs(pushLeft, pushRight)) < std::fabs(minAbs(pushDown, pushUp))) {
                localPush.x = minAbs(pushLeft, pushRight);
            } else {
                localPush.z = minAbs(pushDown, pushUp);
            }
            resolved = resolved + rotateY(localPush, box.yawRadians);
        }
    }
    for (int index : gatherBroadphaseCandidates(dynamicOrientedBoxBuckets_, resolved, radius)) {
        const OrientedCollisionBox& box = dynamicOrientedBoxes_[static_cast<std::size_t>(index)];
        if (profileMatches(box.profile, mask) && circleBlockedByOrientedBox(resolved, radius, box, false)) {
            const Vec3 local = rotateY(resolved - box.center, -box.yawRadians);
            const float halfX = box.size.x * 0.5f;
            const float halfZ = box.size.z * 0.5f;
            const float expandedHalfX = halfX + radius;
            const float expandedHalfZ = halfZ + radius;
            const float pushLeft = -expandedHalfX - local.x;
            const float pushRight = expandedHalfX - local.x;
            const float pushDown = -expandedHalfZ - local.z;
            const float pushUp = expandedHalfZ - local.z;
            Vec3 localPush{};
            if (std::fabs(minAbs(pushLeft, pushRight)) < std::fabs(minAbs(pushDown, pushUp))) {
                localPush.x = minAbs(pushLeft, pushRight);
            } else {
                localPush.z = minAbs(pushDown, pushUp);
            }
            resolved = resolved + rotateY(localPush, box.yawRadians);
        }
    }

    if (isCircleBlocked(resolved, radius, mask)) {
        return from;
    }

    return resolved;
}

VehicleCollisionResult WorldCollision::resolveVehicleCapsule(Vec3 from,
                                                             Vec3 desired,
                                                             float yawRadians,
                                                             const VehicleCollisionConfig& config) const {
    const Vec3 forward = forwardFromYaw(yawRadians);
    const Vec3 right = screenRightFromYaw(yawRadians);
    const Vec3 configuredVelocity = lengthSquaredXZ(config.velocity) > 0.0001f
                                        ? config.velocity
                                        : forward * config.speed + right * config.lateralSlip;

    const auto hitProfileForCircle = [&](Vec3 center) {
        for (const CollisionBox& box : boxes_) {
            if (profileMatches(box.profile, CollisionMasks::Vehicle) &&
                circleBlockedByBox(center, config.radius, box, false)) {
                return box.profile;
            }
        }
        for (const OrientedCollisionBox& box : orientedBoxes_) {
            if (profileMatches(box.profile, CollisionMasks::Vehicle) &&
                circleBlockedByOrientedBox(center, config.radius, box, false)) {
                return box.profile;
            }
        }
        for (const CollisionBox& box : dynamicBoxes_) {
            if (profileMatches(box.profile, CollisionMasks::Vehicle) &&
                circleBlockedByBox(center, config.radius, box, false)) {
                return box.profile;
            }
        }
        for (const OrientedCollisionBox& box : dynamicOrientedBoxes_) {
            if (profileMatches(box.profile, CollisionMasks::Vehicle) &&
                circleBlockedByOrientedBox(center, config.radius, box, false)) {
                return box.profile;
            }
        }
        return CollisionProfile{};
    };

    const auto zoneFromNormal = [&](const VehicleCollisionResult& result, Vec3 normal) {
        const float frontDot = normal.x * forward.x + normal.z * forward.z;
        const float rightDot = normal.x * right.x + normal.z * right.z;
        if (std::fabs(rightDot) > std::fabs(frontDot) * 1.10f && std::fabs(rightDot) > 0.25f) {
            return rightDot < 0.0f ? VehicleHitZone::RightSide : VehicleHitZone::LeftSide;
        }
        if (frontDot < -0.25f) {
            return VehicleHitZone::Front;
        }
        if (frontDot > 0.25f) {
            return VehicleHitZone::Rear;
        }
        if (result.hitFront && !result.hitRear) {
            return VehicleHitZone::Front;
        }
        if (result.hitRear && !result.hitFront) {
            return VehicleHitZone::Rear;
        }
        if (result.hitCenter) {
            return VehicleHitZone::Center;
        }
        return VehicleHitZone::None;
    };

    const auto annotateHit = [&](VehicleCollisionResult& result, Vec3 attempted, Vec3 accepted) {
        Vec3 normal = normalizeXZ(accepted - attempted);
        if (lengthSquaredXZ(normal) <= 0.0001f) {
            normal = normalizeXZ(from - desired);
        }
        result.impactNormal = normal;
        result.normal = normal;
        result.impactSpeed = result.hit
                                 ? std::max(0.0f,
                                            -(configuredVelocity.x * normal.x + configuredVelocity.z * normal.z))
                                 : 0.0f;
        result.hitZone = result.hit ? zoneFromNormal(result, normal) : VehicleHitZone::None;
        if (result.hitProfile.layers == CollisionMasks::None) {
            if (result.hitFront) {
                result.hitProfile = hitProfileForCircle(result.circles[0]);
            } else if (result.hitCenter) {
                result.hitProfile = hitProfileForCircle(result.circles[1]);
            } else if (result.hitRear) {
                result.hitProfile = hitProfileForCircle(result.circles[2]);
            }
        }
        result.collisionProfile = result.hitProfile;
        result.hitObjectId = result.hitProfile.ownerId;
    };

    const auto blockedInfo = [&](Vec3 center, VehicleCollisionResult& result) {
        result.circles = vehicleCircles(center, yawRadians, config.halfLength);
        result.hitFront = isCircleBlocked(result.circles[0], config.radius, CollisionMasks::Vehicle);
        result.hitCenter = isCircleBlocked(result.circles[1], config.radius, CollisionMasks::Vehicle);
        result.hitRear = isCircleBlocked(result.circles[2], config.radius, CollisionMasks::Vehicle);
        result.hit = result.hitFront || result.hitCenter || result.hitRear;
        if (result.hitFront) {
            result.hitProfile = hitProfileForCircle(result.circles[0]);
        } else if (result.hitCenter) {
            result.hitProfile = hitProfileForCircle(result.circles[1]);
        } else if (result.hitRear) {
            result.hitProfile = hitProfileForCircle(result.circles[2]);
        } else {
            result.hitProfile = {};
        }
        result.collisionProfile = result.hitProfile;
        result.hitObjectId = result.hitProfile.ownerId;
        return result.hit;
    };

    const Vec3 delta = desired - from;
    const float travelDistance = length(delta);
    const float maxStepDistance = std::max(0.05f, config.radius * 0.75f);
    const int steps = std::max(1, std::min(96, static_cast<int>(std::ceil(travelDistance / maxStepDistance))));

    VehicleCollisionResult desiredResult;
    desiredResult.position = desired;
    const bool desiredBlocked = blockedInfo(desired, desiredResult);
    if (!desiredBlocked && steps <= 1) {
        desiredResult.hitZone = VehicleHitZone::None;
        desiredResult.impactSpeed = 0.0f;
        return desiredResult;
    }
    if (desiredBlocked) {
        annotateHit(desiredResult, desired, from);
    }

    Vec3 current = from;
    VehicleCollisionResult currentResult;
    currentResult.position = current;
    blockedInfo(current, currentResult);
    if (currentResult.hit) {
        annotateHit(currentResult, desired, current);
        return currentResult;
    }

    for (int step = 1; step <= steps; ++step) {
        const float t = static_cast<float>(step) / static_cast<float>(steps);
        const Vec3 stepDesired = from + delta * t;
        VehicleCollisionResult stepResult;
        stepResult.position = stepDesired;
        if (blockedInfo(stepDesired, stepResult)) {
            const Vec3 tryX{stepDesired.x, current.y, current.z};
            const Vec3 tryZ{current.x, current.y, stepDesired.z};

            VehicleCollisionResult xResult;
            xResult.position = tryX;
            const bool xBlocked = blockedInfo(tryX, xResult);

            VehicleCollisionResult zResult;
            zResult.position = tryZ;
            const bool zBlocked = blockedInfo(tryZ, zResult);

            VehicleCollisionResult* chosen = nullptr;
            if (!xBlocked && !zBlocked) {
                const float xProgress = distanceSquaredXZ(tryX, current);
                const float zProgress = distanceSquaredXZ(tryZ, current);
                chosen = zProgress >= xProgress ? &zResult : &xResult;
            } else if (!zBlocked) {
                chosen = &zResult;
            } else if (!xBlocked) {
                chosen = &xResult;
            }

            if (chosen == nullptr || distanceSquaredXZ(chosen->position, current) <= 0.000001f) {
                currentResult.hit = true;
                currentResult.hitFront = stepResult.hitFront;
                currentResult.hitCenter = stepResult.hitCenter;
                currentResult.hitRear = stepResult.hitRear;
                currentResult.hitProfile = stepResult.hitProfile;
                currentResult.collisionProfile = stepResult.collisionProfile;
                currentResult.hitObjectId = stepResult.hitObjectId;
                annotateHit(currentResult, stepDesired, current);
                return currentResult;
            }

            current = chosen->position;
            currentResult = *chosen;
            currentResult.position = current;
            currentResult.hit = true;
            currentResult.hitFront = stepResult.hitFront;
            currentResult.hitCenter = stepResult.hitCenter;
            currentResult.hitRear = stepResult.hitRear;
            currentResult.hitProfile = stepResult.hitProfile;
            currentResult.collisionProfile = stepResult.collisionProfile;
            currentResult.hitObjectId = stepResult.hitObjectId;
            annotateHit(currentResult, stepDesired, current);
            continue;
        }
        current = stepDesired;
        currentResult.position = current;
        currentResult.circles = stepResult.circles;
    }

    return currentResult;
}

GroundHit WorldCollision::probeGround(Vec3 position, float maxDistance, float walkableSlopeDegrees) const {
    GroundHit best;
    float bestHeight = -100000.0f;

    const auto probeSurface = [&](const GroundSurface& surface) {
        if (!surface.profile.isWalkable || (surface.profile.layers & CollisionMasks::Walkable) == 0) {
            return;
        }
        GroundHit hit;
        if (surface.type == GroundType::Plane) {
            hit.hit = true;
            hit.height = surface.startHeight;
            hit.normal = {0.0f, 1.0f, 0.0f};
            hit.slopeDegrees = 0.0f;
        } else if (surface.type == GroundType::Box) {
            if (!inOrientedRectXZ(position, surface.center, surface.size, surface.yawRadians)) {
                return;
            }
            hit.hit = true;
            hit.height = surface.startHeight;
            hit.normal = {0.0f, 1.0f, 0.0f};
            hit.slopeDegrees = 0.0f;
        } else {
            if (!inRectXZ(position, surface.center, surface.size)) {
                return;
            }
            const float minZ = surface.center.z - surface.size.z * 0.5f;
            const float t = surface.size.z <= 0.0001f ? 0.0f : std::clamp((position.z - minZ) / surface.size.z, 0.0f, 1.0f);
            const float deltaHeight = surface.endHeight - surface.startHeight;
            hit.hit = true;
            hit.height = surface.startHeight + deltaHeight * t;
            hit.normal = normalize({0.0f, surface.size.z, -deltaHeight});
            hit.slopeDegrees = degrees(std::atan2(std::fabs(deltaHeight), std::max(surface.size.z, 0.0001f)));
        }

        hit.ownerId = surface.profile.ownerId;
        hit.platformVelocity = surface.profile.platformVelocity;
        hit.walkable = hit.slopeDegrees <= walkableSlopeDegrees + 0.001f;
        if (hit.height <= position.y + maxDistance && hit.height >= position.y - maxDistance && hit.height >= bestHeight) {
            best = hit;
            bestHeight = hit.height;
        }
    };

    for (const GroundSurface& surface : groundSurfaces_) {
        probeSurface(surface);
    }
    for (const GroundSurface& surface : dynamicGroundSurfaces_) {
        probeSurface(surface);
    }

    return best;
}

CharacterCollisionResult WorldCollision::resolveCharacter(Vec3 from, Vec3 desired, const CharacterCollisionConfig& config) const {
    const Vec3 delta = desired - from;
    const float travelDistance = length(delta);
    const float maxStepDistance = std::max(0.05f, (config.radius + config.skinWidth) * 0.65f);
    const int steps = std::max(1, std::min(32, static_cast<int>(std::ceil(travelDistance / maxStepDistance))));

    if (steps <= 1) {
        return resolveCharacterStep(from, desired, config);
    }

    CharacterCollisionResult result;
    Vec3 current = from;
    for (int step = 1; step <= steps; ++step) {
        const float t = static_cast<float>(step) / static_cast<float>(steps);
        const Vec3 stepDesired = from + delta * t;
        const CharacterCollisionResult stepResult = resolveCharacterStep(current, stepDesired, config);

        result.hitWall = result.hitWall || stepResult.hitWall;
        result.stepped = result.stepped || stepResult.stepped;
        result.blockedByStep = result.blockedByStep || stepResult.blockedByStep;
        result.blockedBySlope = result.blockedBySlope || stepResult.blockedBySlope;
        if (stepResult.hitOwnerId != 0 || result.hitOwnerId == 0) {
            result.hitOwnerId = stepResult.hitOwnerId;
            result.hitProfile = stepResult.hitProfile;
        }

        const bool madeProgress = length(stepResult.position - current) > 0.0001f;
        current = stepResult.position;
        if (!madeProgress && (stepResult.hitWall || stepResult.blockedByStep || stepResult.blockedBySlope)) {
            break;
        }
    }

    result.position = current;
    return result;
}

CharacterCollisionResult WorldCollision::resolveCharacterStep(Vec3 from, Vec3 desired, const CharacterCollisionConfig& config) const {
    CharacterCollisionResult result;
    result.position = desired;

    const GroundHit desiredGround = probeGround(
        desired + Vec3{0.0f, config.groundProbeDistance, 0.0f},
        config.groundProbeDistance + config.stepHeight + 2.0f,
        config.walkableSlopeDegrees);
    if (desiredGround.hit) {
        const float heightDelta = desiredGround.height - from.y;
        if (!desiredGround.walkable) {
            result.position = from;
            result.blockedBySlope = true;
            return result;
        }
        if (desiredGround.slopeDegrees <= 0.001f && heightDelta > config.stepHeight) {
            result.position = from;
            result.blockedByStep = true;
            return result;
        }
        if (heightDelta > 0.001f && desired.y <= desiredGround.height + config.stepHeight) {
            result.position.y = desiredGround.height;
            result.stepped = heightDelta > 0.03f;
        }
    }

    const float effectiveRadius = config.radius + config.skinWidth;
    if (!isCharacterCircleBlocked(result.position, effectiveRadius, config.ignoreBlockerOwnerId)) {
        return result;
    }

    const auto markHit = [&](const CollisionProfile& profile) {
        if (result.hitOwnerId == 0) {
            result.hitOwnerId = profile.ownerId;
            result.hitProfile = profile;
        }
    };
    const auto findHitProfile = [&]() {
        const auto shouldSkipOwner = [&](const CollisionProfile& profile) {
            return config.ignoreBlockerOwnerId != 0 && profile.ownerId == config.ignoreBlockerOwnerId;
        };
        for (const CollisionBox& box : boxes_) {
            if (!shouldSkipOwner(box.profile) &&
                profileMatches(box.profile, CollisionMasks::Player) &&
                circleBlockedByBox(result.position, effectiveRadius, box, true)) {
                markHit(box.profile);
                return;
            }
        }
        for (const OrientedCollisionBox& box : orientedBoxes_) {
            if (!shouldSkipOwner(box.profile) &&
                profileMatches(box.profile, CollisionMasks::Player) &&
                circleBlockedByOrientedBox(result.position, effectiveRadius, box, true)) {
                markHit(box.profile);
                return;
            }
        }
        for (const CollisionBox& box : dynamicBoxes_) {
            if (!shouldSkipOwner(box.profile) &&
                profileMatches(box.profile, CollisionMasks::Player) &&
                circleBlockedByBox(result.position, effectiveRadius, box, true)) {
                markHit(box.profile);
                return;
            }
        }
        for (const OrientedCollisionBox& box : dynamicOrientedBoxes_) {
            if (!shouldSkipOwner(box.profile) &&
                profileMatches(box.profile, CollisionMasks::Player) &&
                circleBlockedByOrientedBox(result.position, effectiveRadius, box, true)) {
                markHit(box.profile);
                return;
            }
        }
    };
    findHitProfile();

    const Vec3 tryX{result.position.x, result.position.y, from.z};
    const Vec3 tryZ{from.x, result.position.y, result.position.z};
    const bool xFree = !isCharacterCircleBlocked(tryX, effectiveRadius, config.ignoreBlockerOwnerId);
    const bool zFree = !isCharacterCircleBlocked(tryZ, effectiveRadius, config.ignoreBlockerOwnerId);

    result.hitWall = true;
    if (zFree && (!xFree || std::fabs(result.position.z - from.z) >= std::fabs(result.position.x - from.x))) {
        result.position = tryZ;
    } else if (xFree) {
        result.position = tryX;
    } else {
        result.position = from;
    }
    return result;
}

bool WorldCollision::isCharacterCircleBlocked(Vec3 center, float radius) const {
    return isCharacterCircleBlocked(center, radius, 0);
}

bool WorldCollision::isCharacterCircleBlocked(Vec3 center, float radius, int ignoreOwnerId) const {
    const auto shouldSkipOwner = [&](const CollisionProfile& profile) {
        return ignoreOwnerId != 0 && profile.ownerId == ignoreOwnerId;
    };

    for (int index : gatherBroadphaseCandidates(staticBoxBuckets_, center, radius)) {
        const CollisionBox& box = boxes_[static_cast<std::size_t>(index)];
        if (!shouldSkipOwner(box.profile) &&
            profileMatches(box.profile, CollisionMasks::Player) &&
            circleBlockedByBox(center, radius, box, true)) {
            return true;
        }
    }
    for (int index : gatherBroadphaseCandidates(staticOrientedBoxBuckets_, center, radius)) {
        const OrientedCollisionBox& box = orientedBoxes_[static_cast<std::size_t>(index)];
        if (!shouldSkipOwner(box.profile) &&
            profileMatches(box.profile, CollisionMasks::Player) &&
            circleBlockedByOrientedBox(center, radius, box, true)) {
            return true;
        }
    }
    for (int index : gatherBroadphaseCandidates(dynamicBoxBuckets_, center, radius)) {
        const CollisionBox& box = dynamicBoxes_[static_cast<std::size_t>(index)];
        if (!shouldSkipOwner(box.profile) &&
            profileMatches(box.profile, CollisionMasks::Player) &&
            circleBlockedByBox(center, radius, box, true)) {
            return true;
        }
    }
    for (int index : gatherBroadphaseCandidates(dynamicOrientedBoxBuckets_, center, radius)) {
        const OrientedCollisionBox& box = dynamicOrientedBoxes_[static_cast<std::size_t>(index)];
        if (!shouldSkipOwner(box.profile) &&
            profileMatches(box.profile, CollisionMasks::Player) &&
            circleBlockedByOrientedBox(center, radius, box, true)) {
            return true;
        }
    }
    return false;
}

Vec3 WorldCollision::resolveCameraBoom(Vec3 target, Vec3 desiredCamera, float radius) const {
    const Vec3 delta = desiredCamera - target;
    const float maxDistance = length(delta);
    if (maxDistance <= 0.0001f) {
        return desiredCamera;
    }

    const Vec3 direction = delta / maxDistance;
    float nearest = maxDistance;
    for (int index : gatherBroadphaseSegmentCandidates(staticBoxBuckets_, target, desiredCamera, radius)) {
        const CollisionBox& box = boxes_[static_cast<std::size_t>(index)];
        if (!profileMatches(box.profile, CollisionMasks::Camera)) {
            continue;
        }
        float hit = 0.0f;
        if (segmentAabb(target, direction, maxDistance, box, radius, hit)) {
            nearest = std::min(nearest, hit);
        }
    }
    for (int index : gatherBroadphaseSegmentCandidates(staticOrientedBoxBuckets_, target, desiredCamera, radius)) {
        const OrientedCollisionBox& box = orientedBoxes_[static_cast<std::size_t>(index)];
        if (!profileMatches(box.profile, CollisionMasks::Camera)) {
            continue;
        }
        float hit = 0.0f;
        if (segmentObb(target, direction, maxDistance, box, radius, hit)) {
            nearest = std::min(nearest, hit);
        }
    }
    for (int index : gatherBroadphaseSegmentCandidates(dynamicBoxBuckets_, target, desiredCamera, radius)) {
        const CollisionBox& box = dynamicBoxes_[static_cast<std::size_t>(index)];
        if (!profileMatches(box.profile, CollisionMasks::Camera)) {
            continue;
        }
        float hit = 0.0f;
        if (segmentAabb(target, direction, maxDistance, box, radius, hit)) {
            nearest = std::min(nearest, hit);
        }
    }
    for (int index : gatherBroadphaseSegmentCandidates(dynamicOrientedBoxBuckets_, target, desiredCamera, radius)) {
        const OrientedCollisionBox& box = dynamicOrientedBoxes_[static_cast<std::size_t>(index)];
        if (!profileMatches(box.profile, CollisionMasks::Camera)) {
            continue;
        }
        float hit = 0.0f;
        if (segmentObb(target, direction, maxDistance, box, radius, hit)) {
            nearest = std::min(nearest, hit);
        }
    }

    if (nearest >= maxDistance) {
        return desiredCamera;
    }

    const float safeDistance = std::max(0.05f, nearest - 0.08f);
    return target + direction * safeDistance;
}

const std::vector<CollisionBox>& WorldCollision::boxes() const {
    return boxes_;
}

CollisionDebugSnapshot WorldCollision::debugSnapshot() const {
    CollisionDebugSnapshot snapshot;
    snapshot.staticBoxes = boxes_;
    snapshot.staticOrientedBoxes = orientedBoxes_;
    snapshot.dynamicBoxes = dynamicBoxes_;
    snapshot.dynamicOrientedBoxes = dynamicOrientedBoxes_;
    snapshot.groundSurfaces.reserve(groundSurfaces_.size() + dynamicGroundSurfaces_.size());

    const auto appendGround = [&](const GroundSurface& surface) {
        snapshot.groundSurfaces.push_back({surface.center,
                                           surface.size,
                                           surface.yawRadians,
                                           std::max(surface.startHeight, surface.endHeight),
                                           surface.profile});
    };
    for (const GroundSurface& surface : groundSurfaces_) {
        appendGround(surface);
    }
    for (const GroundSurface& surface : dynamicGroundSurfaces_) {
        appendGround(surface);
    }
    return snapshot;
}

CollisionBroadphaseStats WorldCollision::broadphaseStats() const {
    return {static_cast<int>(boxes_.size() + orientedBoxes_.size()),
            static_cast<int>(dynamicBoxes_.size() + dynamicOrientedBoxes_.size()),
            static_cast<int>(staticBoxBuckets_.size() + staticOrientedBoxBuckets_.size()),
            static_cast<int>(dynamicBoxBuckets_.size() + dynamicOrientedBoxBuckets_.size())};
}

int WorldCollision::debugBroadphaseCandidateCount(Vec3 center, float radius, CollisionLayerMask mask) const {
    int count = 0;
    const auto countBoxes = [&](const std::vector<int>& candidates, const auto& boxes) {
        int local = 0;
        for (int index : candidates) {
            const auto& box = boxes[static_cast<std::size_t>(index)];
            if (profileMatches(box.profile, mask)) {
                ++local;
            }
        }
        return local;
    };
    count += countBoxes(gatherBroadphaseCandidates(staticBoxBuckets_, center, radius), boxes_);
    count += countBoxes(gatherBroadphaseCandidates(staticOrientedBoxBuckets_, center, radius), orientedBoxes_);
    count += countBoxes(gatherBroadphaseCandidates(dynamicBoxBuckets_, center, radius), dynamicBoxes_);
    count += countBoxes(gatherBroadphaseCandidates(dynamicOrientedBoxBuckets_, center, radius), dynamicOrientedBoxes_);
    return count;
}

int WorldCollision::debugBroadphaseSegmentCandidateCount(Vec3 start,
                                                         Vec3 end,
                                                         float radius,
                                                         CollisionLayerMask mask) const {
    int count = 0;
    const auto countBoxes = [&](const std::vector<int>& candidates, const auto& boxes) {
        int local = 0;
        for (int index : candidates) {
            const auto& box = boxes[static_cast<std::size_t>(index)];
            if (profileMatches(box.profile, mask)) {
                ++local;
            }
        }
        return local;
    };
    count += countBoxes(gatherBroadphaseSegmentCandidates(staticBoxBuckets_, start, end, radius), boxes_);
    count += countBoxes(gatherBroadphaseSegmentCandidates(staticOrientedBoxBuckets_, start, end, radius), orientedBoxes_);
    count += countBoxes(gatherBroadphaseSegmentCandidates(dynamicBoxBuckets_, start, end, radius), dynamicBoxes_);
    count += countBoxes(gatherBroadphaseSegmentCandidates(dynamicOrientedBoxBuckets_, start, end, radius),
                        dynamicOrientedBoxes_);
    return count;
}

} // namespace bs3d
