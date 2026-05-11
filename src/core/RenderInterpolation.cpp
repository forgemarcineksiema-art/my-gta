#include "bs3d/core/RenderInterpolation.h"

#include <algorithm>
#include <cmath>

namespace bs3d {

float clampInterpolationAlpha(float alpha) {
    return std::clamp(alpha, 0.0f, 1.0f);
}

float interpolateFloat(float from, float to, float alpha) {
    const float t = clampInterpolationAlpha(alpha);
    return from + (to - from) * t;
}

Vec3 interpolateVec3(Vec3 from, Vec3 to, float alpha) {
    const float t = clampInterpolationAlpha(alpha);
    return from + (to - from) * t;
}

float interpolateYawRadians(float from, float to, float alpha) {
    float delta = to - from;
    while (delta > Pi) {
        delta -= 2.0f * Pi;
    }
    while (delta < -Pi) {
        delta += 2.0f * Pi;
    }
    return from + delta * clampInterpolationAlpha(alpha);
}

VehicleRuntimeState interpolateVehicleRuntimeState(const VehicleRuntimeState& previous,
                                                   const VehicleRuntimeState& current,
                                                   float alpha) {
    VehicleRuntimeState state = current;
    state.position = interpolateVec3(previous.position, current.position, alpha);
    state.yawRadians = interpolateYawRadians(previous.yawRadians, current.yawRadians, alpha);
    state.speed = interpolateFloat(previous.speed, current.speed, alpha);
    state.lateralSlip = interpolateFloat(previous.lateralSlip, current.lateralSlip, alpha);
    state.instability = interpolateFloat(previous.instability, current.instability, alpha);
    state.hornPulse = interpolateFloat(previous.hornPulse, current.hornPulse, alpha);
    state.hornCooldown = interpolateFloat(previous.hornCooldown, current.hornCooldown, alpha);
    state.steeringAuthority = interpolateFloat(previous.steeringAuthority, current.steeringAuthority, alpha);
    state.steeringInputSmoothed =
        interpolateFloat(previous.steeringInputSmoothed, current.steeringInputSmoothed, alpha);
    state.wheelSpinRadians = interpolateYawRadians(previous.wheelSpinRadians, current.wheelSpinRadians, alpha);
    state.frontWheelSteerRadians =
        interpolateFloat(previous.frontWheelSteerRadians, current.frontWheelSteerRadians, alpha);
    state.suspensionCompression =
        interpolateFloat(previous.suspensionCompression, current.suspensionCompression, alpha);
    state.wheelSlipVisual = interpolateFloat(previous.wheelSlipVisual, current.wheelSlipVisual, alpha);
    state.driftAngleRadians = interpolateYawRadians(previous.driftAngleRadians, current.driftAngleRadians, alpha);
    state.driftAmount = interpolateFloat(previous.driftAmount, current.driftAmount, alpha);
    state.rpmNormalized = interpolateFloat(previous.rpmNormalized, current.rpmNormalized, alpha);
    state.shiftTimerSeconds = interpolateFloat(previous.shiftTimerSeconds, current.shiftTimerSeconds, alpha);
    state.condition = interpolateFloat(previous.condition, current.condition, alpha);
    return state;
}

CameraRigState interpolateCameraRigState(const CameraRigState& previous, const CameraRigState& current, float alpha) {
    CameraRigState state = current;
    state.position = interpolateVec3(previous.position, current.position, alpha);
    state.desiredPosition = interpolateVec3(previous.desiredPosition, current.desiredPosition, alpha);
    state.target = interpolateVec3(previous.target, current.target, alpha);
    state.desiredTarget = interpolateVec3(previous.desiredTarget, current.desiredTarget, alpha);
    state.fovY = interpolateFloat(previous.fovY, current.fovY, alpha);
    state.cameraYawRadians = interpolateYawRadians(previous.cameraYawRadians, current.cameraYawRadians, alpha);
    state.visualYawRadians = interpolateYawRadians(previous.visualYawRadians, current.visualYawRadians, alpha);
    state.pitchRadians = interpolateFloat(previous.pitchRadians, current.pitchRadians, alpha);
    state.boomLength = interpolateFloat(previous.boomLength, current.boomLength, alpha);
    state.fullBoomLength = interpolateFloat(previous.fullBoomLength, current.fullBoomLength, alpha);
    return state;
}

} // namespace bs3d
