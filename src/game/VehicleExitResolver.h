#pragma once

#include "bs3d/core/Types.h"
#include "bs3d/core/WorldCollision.h"

namespace bs3d {

struct VehicleExitResolution {
    bool found = false;
    Vec3 position{};
};

VehicleExitResolution resolveVehicleExitPosition(const WorldCollision& collisionWithoutVehicle,
                                                 Vec3 vehiclePosition,
                                                 float vehicleYawRadians,
                                                 Vec3 preferredDirection);

} // namespace bs3d
