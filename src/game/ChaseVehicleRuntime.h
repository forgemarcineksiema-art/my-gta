#pragma once

#include "bs3d/core/Types.h"
#include "bs3d/core/WorldCollision.h"

namespace bs3d {

struct ChaseVehicleRuntimeConfig {
    VehicleCollisionConfig collision{};
    VehiclePlayerCollisionConfig targetVehicleCollision{};
    float minimumCenterSeparation = 3.55f;
    bool hasPursuitTarget = false;
    Vec3 pursuitTarget{};
};

struct ChaseVehicleStep {
    Vec3 position{};
    float yawRadians = 0.0f;
    VehicleCollisionResult collision{};
    bool separatedFromTarget = false;
    float distanceToTarget = 0.0f;
};

ChaseVehicleStep advanceChaseVehicle(Vec3 chaserPosition,
                                     float chaserYawRadians,
                                     Vec3 targetVehiclePosition,
                                     float targetVehicleYawRadians,
                                     float followSpeed,
                                     float deltaSeconds,
                                     const WorldCollision& baseCollision,
                                     const ChaseVehicleRuntimeConfig& config = {});

} // namespace bs3d
