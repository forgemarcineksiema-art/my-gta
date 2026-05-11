#include "ChaseVehicleRuntime.h"

#include <algorithm>

namespace bs3d {

namespace {

Vec3 safeAwayFromTarget(Vec3 chaserPosition, Vec3 targetPosition, float targetYawRadians) {
    Vec3 away = normalizeXZ(chaserPosition - targetPosition);
    if (lengthSquaredXZ(away) <= 0.0001f) {
        away = forwardFromYaw(targetYawRadians) * -1.0f;
    }
    return away;
}

} // namespace

ChaseVehicleStep advanceChaseVehicle(Vec3 chaserPosition,
                                     float chaserYawRadians,
                                     Vec3 targetVehiclePosition,
                                     float targetVehicleYawRadians,
                                     float followSpeed,
                                     float deltaSeconds,
                                     const WorldCollision& baseCollision,
                                     const ChaseVehicleRuntimeConfig& config) {
    ChaseVehicleStep step;
    step.position = chaserPosition;
    step.yawRadians = chaserYawRadians;
    step.distanceToTarget = distanceXZ(chaserPosition, targetVehiclePosition);

    const float minSeparation = std::max(0.1f, config.minimumCenterSeparation);
    Vec3 start = chaserPosition;
    if (distanceXZ(start, targetVehiclePosition) < minSeparation) {
        start = targetVehiclePosition + safeAwayFromTarget(start, targetVehiclePosition, targetVehicleYawRadians) *
                                          minSeparation;
        step.separatedFromTarget = true;
    }

    const Vec3 pursuitTarget = config.hasPursuitTarget ? config.pursuitTarget : targetVehiclePosition;
    const Vec3 toTarget = normalizeXZ(pursuitTarget - start);
    const float dt = std::max(0.0f, deltaSeconds);
    float yaw = chaserYawRadians;
    Vec3 desired = start;
    if (lengthSquaredXZ(toTarget) > 0.0001f) {
        yaw = yawFromDirection(toTarget);
        const float currentDistance = distanceXZ(start, pursuitTarget);
        const float requestedMove = std::max(0.0f, followSpeed) * dt;
        const float allowedMove = config.hasPursuitTarget
                                      ? requestedMove
                                      : std::max(0.0f, currentDistance - minSeparation);
        const float moveDistance = std::min(requestedMove, allowedMove);
        desired = start + toTarget * moveDistance;
        step.separatedFromTarget = step.separatedFromTarget || requestedMove > allowedMove + 0.001f;
    }

    const float desiredDistance = distanceXZ(desired, targetVehiclePosition);
    if (desiredDistance < minSeparation) {
        desired = targetVehiclePosition - toTarget * minSeparation;
        step.separatedFromTarget = true;
    }

    WorldCollision collision = baseCollision;
    collision.addDynamicVehiclePlayerCollision(targetVehiclePosition,
                                               targetVehicleYawRadians,
                                               config.targetVehicleCollision);

    VehicleCollisionConfig collisionConfig = config.collision;
    collisionConfig.speed = std::max(collisionConfig.speed, std::max(0.0f, followSpeed));
    const VehicleCollisionResult result =
        collision.resolveVehicleCapsule(start, desired, yaw, collisionConfig);

    step.position = result.position;
    step.yawRadians = yaw;
    step.collision = result;
    step.distanceToTarget = distanceXZ(step.position, targetVehiclePosition);
    if (step.distanceToTarget < minSeparation - 0.01f) {
        step.position = targetVehiclePosition +
                        safeAwayFromTarget(step.position, targetVehiclePosition, targetVehicleYawRadians) *
                            minSeparation;
        step.distanceToTarget = minSeparation;
        step.separatedFromTarget = true;
    }

    return step;
}

} // namespace bs3d
