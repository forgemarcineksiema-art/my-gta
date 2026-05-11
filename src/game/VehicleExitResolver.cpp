#include "VehicleExitResolver.h"

#include <array>

namespace bs3d {

namespace {

bool directionAlreadyListed(const std::array<Vec3, 8>& directions, int count, Vec3 candidate) {
    for (int i = 0; i < count; ++i) {
        if (distanceSquaredXZ(directions[static_cast<std::size_t>(i)], candidate) < 0.01f) {
            return true;
        }
    }
    return false;
}

} // namespace

VehicleExitResolution resolveVehicleExitPosition(const WorldCollision& collisionWithoutVehicle,
                                                 Vec3 vehiclePosition,
                                                 float vehicleYawRadians,
                                                 Vec3 preferredDirection) {
    WorldCollision queryCollision = collisionWithoutVehicle;
    queryCollision.addDynamicVehiclePlayerCollision(vehiclePosition, vehicleYawRadians);

    const Vec3 forward = forwardFromYaw(vehicleYawRadians);
    const Vec3 right = screenRightFromYaw(vehicleYawRadians);
    Vec3 preferred = normalizeXZ(preferredDirection);
    if (lengthSquaredXZ(preferred) <= 0.0001f) {
        preferred = right;
    }

    std::array<Vec3, 8> directions{};
    int directionCount = 0;
    const auto addDirection = [&](Vec3 direction) {
        const Vec3 normalized = normalizeXZ(direction);
        if (lengthSquaredXZ(normalized) <= 0.0001f ||
            directionAlreadyListed(directions, directionCount, normalized)) {
            return;
        }
        directions[static_cast<std::size_t>(directionCount++)] = normalized;
    };

    addDirection(preferred);
    addDirection(preferred * -1.0f);
    addDirection(forward * -1.0f);
    addDirection(forward);
    addDirection(preferred + forward);
    addDirection(preferred - forward);
    addDirection(preferred * -1.0f + forward);
    addDirection(preferred * -1.0f - forward);

    constexpr std::array<float, 4> distances{1.85f, 2.25f, 2.70f, 3.20f};
    CharacterCollisionConfig characterConfig;

    for (int dirIndex = 0; dirIndex < directionCount; ++dirIndex) {
        const Vec3 direction = directions[static_cast<std::size_t>(dirIndex)];
        for (float distance : distances) {
            Vec3 candidate = vehiclePosition + direction * distance;
            const GroundHit ground = queryCollision.probeGround(
                candidate + Vec3{0.0f, 1.35f, 0.0f},
                2.5f,
                characterConfig.walkableSlopeDegrees);

            if (!ground.hit || !ground.walkable || ground.ownerId != 0) {
                continue;
            }
            if (ground.height > vehiclePosition.y + characterConfig.stepHeight + 0.05f) {
                continue;
            }

            candidate.y = ground.height;
            const CharacterCollisionResult resolved =
                queryCollision.resolveCharacter(candidate, candidate, characterConfig);
            if (!resolved.hitWall && !resolved.blockedByStep && !resolved.blockedBySlope) {
                return {true, resolved.position};
            }
            break;
        }
    }

    return {};
}

} // namespace bs3d
