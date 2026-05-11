#include "bs3d/core/CameraRig.h"

#include "bs3d/core/WorldCollision.h"

#include <algorithm>
#include <cmath>

namespace bs3d {

namespace {

float smoothAlpha(float response, float deltaSeconds) {
    if (deltaSeconds <= 0.0f) {
        return 0.0f;
    }

    return 1.0f - std::exp(-response * deltaSeconds);
}

Vec3 lerp(Vec3 from, Vec3 to, float alpha) {
    return from + (to - from) * alpha;
}

float vecLength(Vec3 value) {
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

float lerpFloat(float from, float to, float alpha) {
    return from + (to - from) * alpha;
}

bool isDriving(CameraRigMode mode) {
    return mode == CameraRigMode::Driving || mode == CameraRigMode::Vehicle;
}

bool isSprint(CameraRigMode mode) {
    return mode == CameraRigMode::OnFootSprint;
}

float visualYawFromState(const CameraRigState& state) {
    return yawFromDirection(state.target - state.position);
}

} // namespace

CameraRigMode resolveGameplayCameraMode(const GameplayCameraModeInput& input) {
    if (input.playerInVehicle) {
        return CameraRigMode::Vehicle;
    }
    if (input.explicitInteractionCameraActive) {
        return CameraRigMode::Interaction;
    }
    if (input.sprintSpeed > 0.0f && input.playerSpeed > input.sprintSpeed * 0.72f) {
        return CameraRigMode::OnFootSprint;
    }
    return CameraRigMode::OnFootNormal;
}

CameraRigMode resolveStableCameraMode(CameraRigMode mode, bool stableCameraMode) {
    if (!stableCameraMode) {
        return mode;
    }
    return mode == CameraRigMode::Interaction ? CameraRigMode::Interaction : CameraRigMode::OnFootNormal;
}

CameraRigState CameraRig::update(const CameraRigTarget& target, float deltaSeconds, const WorldCollision* collision) {
    const float dt = std::max(0.0f, deltaSeconds);
    if (!initialized_) {
        cameraYawRadians_ = target.yawRadians;
        pitchRadians_ = isDriving(target.mode) ? -0.42f : -0.36f;
    }

    CameraRigState desired = desiredState(target);
    desired.desiredPosition = desired.position;
    desired.fullBoomLength = vecLength(desired.desiredPosition - desired.target);
    if (collision != nullptr) {
        desired.position = collision->resolveCameraBoom(desired.target, desired.position, config_.boomCollisionRadius);
    }
    desired.boomLength = vecLength(desired.position - desired.target);
    if (!isDriving(target.mode) && desired.boomLength + 0.34f < desired.fullBoomLength) {
        desired.target.y += 0.16f;
        desired.position.y += 0.10f;
        desired.fovY = std::max(54.0f, desired.fovY - 2.0f);
        desired.boomLength = vecLength(desired.position - desired.target);
    }

    if (!initialized_) {
        state_ = desired;
        state_.visualYawRadians = visualYawFromState(state_);
        initialized_ = true;
        return state_;
    }

    const float targetAlpha = smoothAlpha(isDriving(target.mode) ? 4.8f : 34.0f, dt);
    const float positionAlpha = smoothAlpha(isDriving(target.mode) ? 4.6f : 18.0f, dt);
    const bool desiredBoomIsOccluded = desired.boomLength + 0.02f < desired.fullBoomLength;
    const float currentBoomLength = vecLength(state_.position - state_.target);
    const bool boomNeedsImmediateShortening =
        desiredBoomIsOccluded && desired.boomLength + 0.10f < currentBoomLength;

    state_.target = lerp(state_.target, desired.target, targetAlpha);
    if (manualLookThisFrame_) {
        state_.position = state_.target + (desired.position - desired.target);
    } else if (boomNeedsImmediateShortening) {
        state_.position = desired.position;
    } else {
        state_.position = lerp(state_.position, desired.position, positionAlpha);
    }
    state_.desiredPosition = desired.desiredPosition;
    state_.desiredTarget = desired.target;
    state_.fovY = lerpFloat(state_.fovY, desired.fovY, smoothAlpha(3.0f, dt));
    state_.cameraYawRadians = cameraYawRadians_;
    state_.visualYawRadians = visualYawFromState(state_);
    state_.pitchRadians = pitchRadians_;
    state_.boomLength = vecLength(state_.position - state_.target);
    state_.fullBoomLength = desired.fullBoomLength;
    return state_;
}

float CameraRig::beginFrame(const CameraRigControlInput& input, float deltaSeconds) {
    const float dt = std::max(0.0f, deltaSeconds);
    manualLookThisFrame_ = false;
    if (!initialized_) {
        cameraYawRadians_ = input.followYawRadians;
        pitchRadians_ = isDriving(input.mode) ? -0.42f : -0.34f;
        lastMode_ = input.mode;
    }

    if (input.mode != lastMode_) {
        lastMode_ = input.mode;
        recenterDelaySeconds_ = 0.0f;
    }

    const bool mouseMoved = input.mouseLookActive &&
                            (std::fabs(input.mouseDeltaX) > 0.001f || std::fabs(input.mouseDeltaY) > 0.001f);
    if (mouseMoved) {
        manualLookThisFrame_ = true;
        const float mouseXSign = input.invertMouseX ? 1.0f : -1.0f;
        cameraYawRadians_ = wrapAngle(cameraYawRadians_ + input.mouseDeltaX * mouseXSign * config_.mouseSensitivity);
        pitchRadians_ = std::clamp(pitchRadians_ - input.mouseDeltaY * config_.mouseSensitivity,
                                   config_.minPitchRadians,
                                   config_.maxPitchRadians);
        recenterDelaySeconds_ = isDriving(input.mode) ? config_.drivingRecenterDelaySeconds
                                : isSprint(input.mode) ? config_.sprintRecenterDelaySeconds
                                                       : config_.walkingRecenterDelaySeconds;
    } else {
        recenterDelaySeconds_ = std::max(0.0f, recenterDelaySeconds_ - dt);
        const float tension = std::clamp(input.tension + (input.chaseActive ? 0.35f : 0.0f), 0.0f, 1.0f);
        const bool shouldRecenter = isDriving(input.mode) || isSprint(input.mode);
        if (shouldRecenter && recenterDelaySeconds_ <= 0.0f) {
            const float response = isDriving(input.mode) ? config_.drivingRecenterResponse
                                                          : (input.speedRatio > 0.85f ? 2.1f : 1.25f) + tension * 0.75f;
            cameraYawRadians_ = wrapAngle(cameraYawRadians_ +
                                          wrapAngle(input.followYawRadians - cameraYawRadians_) *
                                              smoothAlpha(response, dt));
        }
    }

    state_.cameraYawRadians = cameraYawRadians_;
    state_.pitchRadians = pitchRadians_;
    return cameraYawRadians_;
}

void CameraRig::reset(const CameraRigTarget& target) {
    cameraYawRadians_ = target.yawRadians;
    pitchRadians_ = isDriving(target.mode) ? -0.40f : -0.34f;
    lastMode_ = target.mode;
    recenterDelaySeconds_ = 0.0f;
    manualLookThisFrame_ = false;
    state_ = desiredState(target);
    state_.visualYawRadians = visualYawFromState(state_);
    initialized_ = true;
}

const CameraRigState& CameraRig::state() const {
    return state_;
}

CameraRigState CameraRig::desiredState(const CameraRigTarget& target) const {
    const Vec3 forward = forwardFromYaw(cameraYawRadians_);
    const float speedBonus = std::clamp(target.speedRatio, 0.0f, 1.0f);
    const float tension = std::clamp(target.tension + (target.chaseActive ? 0.25f : 0.0f), 0.0f, 1.0f);
    const Vec3 lookAheadDirection = normalizeXZ(target.velocity);
    float lookAhead = config_.walkingLookAhead * speedBonus;

    float distance = 4.95f;
    float height = 0.35f;
    float targetHeight = 1.58f;
    float fov = 64.0f;

    if (isSprint(target.mode)) {
        distance = 5.75f + tension * 0.45f;
        height = 0.45f;
        targetHeight = 1.62f;
        fov = 66.0f + speedBonus * 4.0f + tension * 3.0f;
        lookAhead = config_.sprintLookAhead * std::max(0.35f, speedBonus) + tension * 0.55f;
    }

    if (target.mode == CameraRigMode::Interaction) {
        distance = 4.10f;
        height = 0.25f;
        targetHeight = 1.45f;
        fov = 58.0f;
        lookAhead = 0.4f;
    }

    if (isDriving(target.mode)) {
        const float drivingSpeedLook = std::clamp((speedBonus - 0.18f) / 0.82f, 0.0f, 1.0f);
        distance = 5.8f + drivingSpeedLook * 1.35f;
        height = 0.82f + drivingSpeedLook * 0.18f;
        targetHeight = 1.35f;
        fov = 68.0f + drivingSpeedLook * 6.8f + tension * 2.0f;
        lookAhead = 0.22f + config_.drivingLookAhead * drivingSpeedLook;
    }

    if (target.chaseActive) {
        distance += 0.85f;
        height += 0.20f;
        fov += 2.0f;
    }

    if (!isDriving(target.mode) && !isSprint(target.mode) && target.mode != CameraRigMode::Interaction) {
        distance = std::max(4.6f, distance - tension * 0.35f);
        fov += tension * 2.5f;
    }

    CameraRigState desired;
    const float horizontalDistance = std::cos(pitchRadians_) * distance;
    const float pitchHeight = -std::sin(pitchRadians_) * distance;
    desired.target = target.position + Vec3{0.0f, targetHeight, 0.0f} + lookAheadDirection * lookAhead;
    desired.desiredTarget = desired.target;
    desired.position = desired.target - forward * horizontalDistance + Vec3{0.0f, height + pitchHeight, 0.0f};
    desired.desiredPosition = desired.position;
    desired.fovY = fov;
    desired.cameraYawRadians = cameraYawRadians_;
    desired.visualYawRadians = visualYawFromState(desired);
    desired.pitchRadians = pitchRadians_;
    desired.boomLength = vecLength(desired.position - desired.target);
    desired.fullBoomLength = desired.boomLength;
    return desired;
}

} // namespace bs3d
