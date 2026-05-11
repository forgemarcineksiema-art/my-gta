#pragma once

#include "bs3d/core/CameraRig.h"
#include "bs3d/core/Types.h"
#include "bs3d/core/VehicleController.h"

namespace bs3d {

float clampInterpolationAlpha(float alpha);
float interpolateFloat(float from, float to, float alpha);
Vec3 interpolateVec3(Vec3 from, Vec3 to, float alpha);
float interpolateYawRadians(float from, float to, float alpha);
VehicleRuntimeState interpolateVehicleRuntimeState(const VehicleRuntimeState& previous,
                                                   const VehicleRuntimeState& current,
                                                   float alpha);
CameraRigState interpolateCameraRigState(const CameraRigState& previous, const CameraRigState& current, float alpha);

} // namespace bs3d
