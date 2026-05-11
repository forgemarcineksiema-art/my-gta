#include "bs3d/core/CharacterVaultPolicy.h"

#include <algorithm>

namespace bs3d {

CharacterVaultResult resolveCharacterLowVault(const WorldCollision& collision,
                                              const CharacterVaultRequest& request) {
    CharacterVaultResult result;
    const Vec3 forward = normalizeXZ(request.forward);
    if (lengthSquaredXZ(forward) <= 0.0001f) {
        return result;
    }

    const Vec3 sample = request.position + forward * std::max(0.05f, request.reach);
    const float probeHeight = request.maxObstacleHeight + 0.40f;
    const GroundHit ground = collision.probeGround(
        sample + Vec3{0.0f, probeHeight, 0.0f},
        probeHeight + 0.20f,
        request.walkableSlopeDegrees);
    if (!ground.hit || !ground.walkable) {
        return result;
    }

    const float obstacleHeight = ground.height - request.position.y;
    if (obstacleHeight < request.minObstacleHeight || obstacleHeight > request.maxObstacleHeight) {
        return result;
    }

    Vec3 landing = request.position + forward * (std::max(0.05f, request.reach) +
                                                std::max(0.05f, request.landingDistance));
    const GroundHit landingGround = collision.probeGround(
        landing + Vec3{0.0f, probeHeight, 0.0f},
        probeHeight + request.stepHeight + 0.20f,
        request.walkableSlopeDegrees);
    if (!landingGround.hit || !landingGround.walkable) {
        return result;
    }
    landing.y = landingGround.height;

    CharacterCollisionConfig landingConfig;
    landingConfig.radius = request.characterRadius;
    landingConfig.skinWidth = request.skinWidth;
    landingConfig.stepHeight = request.stepHeight;
    landingConfig.walkableSlopeDegrees = request.walkableSlopeDegrees;
    landingConfig.groundProbeDistance = request.stepHeight + 0.30f;
    const CharacterCollisionResult landingClearance =
        collision.resolveCharacter(landing, landing, landingConfig);
    if (landingClearance.hitWall || landingClearance.blockedBySlope || landingClearance.blockedByStep) {
        return result;
    }

    result.available = true;
    result.obstacleHeight = obstacleHeight;
    const float heightT = std::clamp((obstacleHeight - request.minObstacleHeight) /
                                         std::max(0.001f, request.maxObstacleHeight - request.minObstacleHeight),
                                     0.0f,
                                     1.0f);
    result.takeoffImpulse = forward * (1.05f + heightT * 0.20f) + Vec3{0.0f, 0.58f + heightT * 0.22f, 0.0f};
    return result;
}

} // namespace bs3d
