#include "bs3d/core/VehicleController.h"

#include "bs3d/core/WorldCollision.h"

#include <algorithm>
#include <cmath>

namespace bs3d {

namespace {

VehicleConditionBand bandForCondition(float condition) {
    if (condition >= 85.0f) {
        return VehicleConditionBand::Igla;
    }
    if (condition >= 65.0f) {
        return VehicleConditionBand::JeszczePojezdzi;
    }
    if (condition >= 42.0f) {
        return VehicleConditionBand::CosStuka;
    }
    if (condition >= 18.0f) {
        return VehicleConditionBand::ZarazWybuchnieAleNieDzis;
    }
    return VehicleConditionBand::Zlom;
}

constexpr float WheelRadiusMeters = 0.34f;
constexpr float MaxFrontWheelSteerRadians = 0.48f;

float gearUpperSpeed(int gear) {
    switch (std::clamp(gear, 1, 4)) {
    case 1:
        return 4.4f;
    case 2:
        return 8.4f;
    case 3:
        return 12.4f;
    default:
        return 15.5f;
    }
}

float gearLowerSpeed(int gear) {
    switch (std::clamp(gear, 1, 4)) {
    case 1:
        return 0.0f;
    case 2:
        return 3.4f;
    case 3:
        return 7.4f;
    default:
        return 11.2f;
    }
}

int gearForSpeed(float speed) {
    const float forwardSpeed = std::max(0.0f, speed);
    if (forwardSpeed < 4.2f) {
        return 1;
    }
    if (forwardSpeed < 8.1f) {
        return 2;
    }
    if (forwardSpeed < 12.0f) {
        return 3;
    }
    return 4;
}

float signedDirection(float value, float deadzone = 0.001f) {
    if (value > deadzone) {
        return 1.0f;
    }
    if (value < -deadzone) {
        return -1.0f;
    }
    return 0.0f;
}

} // namespace

void VehicleController::reset(Vec3 position, float yawRadians) {
    state_ = {};
    state_.position = position;
    state_.yawRadians = wrapAngle(yawRadians);
    state_.condition = 100.0f;
    state_.conditionBand = VehicleConditionBand::Igla;
}

VehicleRuntimeState VehicleController::simulate(const VehicleRuntimeState& state,
                                                const InputState& input,
                                                float deltaSeconds,
                                                const VehicleSurfaceResponse& surface) const {
    VehicleRuntimeState next = state;
    const float dt = std::max(0.0f, deltaSeconds);

    // Phase 1 — decay temporary state: horn, instability, lateral slip, drift angle, shift timer.
    const float speedBeforeInput = next.speed;
    next.hornPulse = approach(next.hornPulse, 0.0f, 2.2f * dt);
    next.hornCooldown = std::max(0.0f, next.hornCooldown - dt);
    next.instability = approach(next.instability, 0.0f, 1.45f * dt);
    const float surfaceGrip = std::max(0.15f, surface.gripMultiplier);
    const float surfaceDrag = std::max(0.25f, surface.dragMultiplier);
    const float surfaceAcceleration = std::max(0.10f, surface.accelerationMultiplier);
    const float slipClearRate = input.handbrake || state.driftActive
                                    ? config_.grip * 0.24f * surfaceGrip
                                    : config_.grip * surfaceGrip;
    next.lateralSlip = approach(next.lateralSlip, 0.0f, slipClearRate * dt);
    next.driftAngleRadians = approach(next.driftAngleRadians, 0.0f, config_.driftExitRate * 0.18f * dt);
    next.shiftTimerSeconds = std::max(0.0f, next.shiftTimerSeconds - dt);

    if (input.hornPressed && next.hornCooldown <= 0.0f) {
        next.hornPulse = 1.0f;
        next.hornCooldown = 0.42f;
    }

    next.boostActive = input.vehicleBoost && input.accelerate && !input.brake && next.speed >= -0.1f;
    const float damageRatio = std::clamp((100.0f - next.condition) / 100.0f, 0.0f, 1.0f);
    const float conditionMultiplier = 1.0f - damageRatio * config_.damageInfluence;
    const float speedRatioBefore = config_.maxForwardSpeed <= 0.0f
                                       ? 0.0f
                                       : std::clamp(std::fabs(next.speed) / config_.maxForwardSpeed, 0.0f, 1.0f);
    const bool steeringInput = input.steerLeft || input.steerRight;
    const bool handbrakeDriftIntent = input.handbrake &&
                                      steeringInput &&
                                      next.speed > config_.driftEntryMinSpeed;
    const int wantedGear = next.speed < -0.1f ? 1 : gearForSpeed(next.speed);
    if (wantedGear > next.gear) {
        next.gear = wantedGear;
        next.shiftTimerSeconds = config_.shiftDurationSeconds;
    } else if (wantedGear < next.gear && next.shiftTimerSeconds <= 0.0f) {
        next.gear = wantedGear;
    }
    const float shiftMultiplier = next.shiftTimerSeconds > 0.0f ? config_.shiftAccelerationMultiplier : 1.0f;
    const float gearTorque = next.gear == 1 ? 1.0f
                            : next.gear == 2 ? 0.88f
                            : next.gear == 3 ? 0.76f
                                             : 0.62f;
    // Phase 2 — acceleration, braking, drag, and handbrake speed modulation.
    if (input.accelerate && !input.brake) {
        if (next.speed < -0.05f) {
            next.speed = approach(next.speed, 0.0f, config_.braking * conditionMultiplier * dt);
        } else {
            const float accelerationCurve = std::max(0.22f, 1.0f - speedRatioBefore * 0.68f);
            const float boost = next.boostActive ? config_.boostMultiplier : 1.0f;
            next.speed += config_.acceleration * accelerationCurve * gearTorque * shiftMultiplier *
                          boost * conditionMultiplier * surfaceAcceleration * dt;
        }
    }

    if (input.brake) {
        if (next.speed > 0.05f) {
            next.speed = approach(next.speed, 0.0f, config_.braking * conditionMultiplier * dt);
        } else if (!input.accelerate) {
            next.speed -= config_.acceleration * 0.65f * dt;
        }
    }

    if (!input.accelerate && !input.brake) {
        next.speed = approach(next.speed, 0.0f, config_.drag * surfaceDrag * dt);
    }

    if (input.handbrake) {
        const float dragScale = handbrakeDriftIntent ? 0.40f : 1.0f;
        next.speed = approach(next.speed, 0.0f, config_.handbrakeDrag * dragScale * surfaceDrag * dt);
        if (handbrakeDriftIntent) {
            next.speed = std::max(0.0f,
                                  next.speed -
                                      config_.driftSpeedBleedPerSecond * (0.45f + speedRatioBefore) * dt);
        }
        next.instability = std::max(next.instability, 0.28f + speedRatioBefore * 0.35f);
        next.condition = std::max(0.0f, next.condition - 0.05f * dt);
    }

    if (next.boostActive) {
        next.instability = std::max(next.instability, 0.18f + speedRatioBefore * 0.20f);
        next.condition = std::max(0.0f, next.condition - config_.boostWearPerSecond * dt);
    }

    next.speed = std::clamp(next.speed, config_.maxReverseSpeed, config_.maxForwardSpeed);
    const float gearLow = gearLowerSpeed(next.gear);
    const float gearHigh = std::min(config_.maxForwardSpeed, gearUpperSpeed(next.gear));
    if (next.speed < -0.05f) {
        next.rpmNormalized = std::clamp(std::fabs(next.speed) / std::fabs(config_.maxReverseSpeed), 0.0f, 1.0f);
    } else {
        next.rpmNormalized = std::clamp((next.speed - gearLow) / std::max(0.1f, gearHigh - gearLow), 0.0f, 1.0f);
    }

    // Phase 3 — steering, yaw, and front-wheel visual.
    const float speedFactor = std::clamp(std::fabs(next.speed) / config_.maxForwardSpeed, 0.0f, 1.0f);
    const bool drivingIntent = input.accelerate || input.brake ||
                               std::fabs(speedBeforeInput) > 0.08f ||
                               std::fabs(next.speed) > 0.08f;
    const float steeringSpeed = std::max(std::fabs(speedBeforeInput), std::fabs(next.speed));
    float steeringAuthority = std::clamp(steeringSpeed / 1.15f, 0.0f, 1.0f);
    if (steeringInput && drivingIntent) {
        steeringAuthority = std::max(steeringAuthority, config_.minimumSteeringAuthority);
    }
    if (!drivingIntent) {
        steeringAuthority = 0.0f;
    }
    next.steeringAuthority = steeringAuthority;
    const float highSpeedBlend = speedFactor;
    const float steeringRate = config_.lowSpeedSteeringRadiansPerSecond +
                               (config_.highSpeedSteeringRadiansPerSecond - config_.lowSpeedSteeringRadiansPerSecond) *
                                   highSpeedBlend;
    const float handbrakeMultiplier = input.handbrake ? config_.handbrakeStrength : 1.0f;
    const float turnAmount = std::min(steeringRate * handbrakeMultiplier * steeringAuthority * dt, Pi * 0.95f);
    const float reverseSteeringSign = next.speed < -0.05f ? -1.0f : 1.0f;
    float rawSteerSign = 0.0f;
    if (input.steerLeft) {
        rawSteerSign -= 1.0f;
    }

    if (input.steerRight) {
        rawSteerSign += 1.0f;
    }
    rawSteerSign = std::clamp(rawSteerSign, -1.0f, 1.0f);
    const float steerRampRate = rawSteerSign == 0.0f ? config_.steeringReleaseRate : config_.steeringRiseRate;
    next.steeringInputSmoothed = approach(next.steeringInputSmoothed, rawSteerSign, steerRampRate * dt);
    const float steerSign = next.steeringInputSmoothed;
    if (steerSign < -0.001f) {
        next.yawRadians += turnAmount * reverseSteeringSign * std::fabs(steerSign);
    }
    if (steerSign > 0.001f) {
        next.yawRadians -= turnAmount * reverseSteeringSign * std::fabs(steerSign);
    }
    next.yawRadians = wrapAngle(next.yawRadians);

    const float steerVisualTarget = -steerSign * MaxFrontWheelSteerRadians;
    next.frontWheelSteerRadians = approach(next.frontWheelSteerRadians, steerVisualTarget, 6.8f * dt);

    // Phase 4 — drift entry, sustain, counter-steer, and exit.
    const float steerDirection = signedDirection(steerSign);
    const float previousDriftDirection = signedDirection(next.driftAngleRadians, 0.025f);
    const float driftDirection = previousDriftDirection != 0.0f ? previousDriftDirection : steerDirection;
    const bool canForwardDrift = next.speed > config_.driftEntryMinSpeed &&
                                 steerDirection != 0.0f &&
                                 steeringAuthority > 0.0f;
    const bool driftEntry = input.handbrake && canForwardDrift;
    const bool counterSteer = driftDirection != 0.0f &&
                              steerDirection != 0.0f &&
                              driftDirection != steerDirection;
    float driftAngleTarget = 0.0f;
    if (driftEntry && !counterSteer) {
        const float driftSpeedRatio = std::clamp((std::fabs(next.speed) - config_.driftEntryMinSpeed) /
                                                     std::max(0.1f, config_.maxForwardSpeed - config_.driftEntryMinSpeed),
                                                 0.0f,
                                                 1.0f);
        driftAngleTarget = steerSign * config_.driftMaxAngleRadians * (0.58f + driftSpeedRatio * 0.42f);
    }

    const float driftRate = driftEntry && !counterSteer ? config_.driftBuildRate
                            : counterSteer                 ? config_.driftCounterSteerRate
                                                           : config_.driftExitRate;
    next.driftAngleRadians = approach(next.driftAngleRadians, driftAngleTarget, driftRate * dt);
    if (std::fabs(next.speed) < config_.driftEntryMinSpeed * 0.35f) {
        next.driftAngleRadians = approach(next.driftAngleRadians, 0.0f, config_.driftExitRate * 2.2f * dt);
    }
    next.driftAngleRadians = std::clamp(next.driftAngleRadians,
                                        -config_.driftMaxAngleRadians,
                                        config_.driftMaxAngleRadians);
    next.driftAmount = std::clamp(std::fabs(next.driftAngleRadians) /
                                      std::max(0.001f, config_.driftMaxAngleRadians),
                                  0.0f,
                                  1.0f);
    next.driftActive = next.driftAmount > 0.045f && std::fabs(next.speed) > 0.70f;

    // Phase 5 — lateral slip integration, position update, wheel spin, suspension, visual cue.
    const Vec3 forward = forwardFromYaw(next.yawRadians);
    const Vec3 right = screenRightFromYaw(next.yawRadians);
    float targetSlip = steerSign * next.speed * (input.handbrake ? config_.handbrakeDriftFactor : config_.driftFactor);
    float slipResponse = input.handbrake ? 8.0f : 4.5f;
    if (next.driftActive) {
        const float activeDriftDirection = signedDirection(next.driftAngleRadians, 0.005f);
        const float sustainFactor = input.handbrake ? config_.handbrakeDriftFactor : config_.driftSustainFactor;
        targetSlip = activeDriftDirection * next.speed * sustainFactor * (counterSteer ? 0.50f : 1.0f);
        slipResponse = counterSteer ? 7.2f : (input.handbrake ? 8.4f : 5.8f);
    }
    const float surfaceSlipMultiplier =
        std::clamp(1.0f / std::pow(std::max(0.22f, surfaceGrip), 3.0f), 1.0f, 4.2f);
    targetSlip *= surfaceSlipMultiplier;
    targetSlip += steerSign * (1.0f - std::min(surfaceGrip, 1.0f)) * 0.72f *
                  std::clamp(std::fabs(next.speed) / 3.0f, 0.0f, 1.0f);
    slipResponse *= std::clamp(1.12f - surfaceGrip * 0.12f, 1.0f, 1.10f);
    next.lateralSlip = approach(next.lateralSlip, targetSlip, slipResponse * dt);
    next.position = next.position + forward * (next.speed * dt) + right * (next.lateralSlip * dt);
    next.wheelSpinRadians = wrapAngle(next.wheelSpinRadians + (next.speed / WheelRadiusMeters) * dt);

    const float brakingLoad = input.brake
                                  ? 0.045f +
                                        std::clamp(std::fabs(speedBeforeInput) / config_.maxForwardSpeed, 0.0f, 1.0f) *
                                            0.075f
                                  : 0.0f;
    const float handbrakeLoad = input.handbrake ? 0.06f : 0.0f;
    const float driftLoad = next.driftAmount * 0.035f;
    const float slipLoad = std::clamp(std::fabs(next.lateralSlip) / 2.2f, 0.0f, 1.0f) * 0.08f;
    const float boostLoad = next.boostActive ? 0.035f : 0.0f;
    const float suspensionTarget = std::clamp(0.018f + brakingLoad + handbrakeLoad + slipLoad + driftLoad + boostLoad,
                                              0.0f,
                                              0.18f);
    next.suspensionCompression = approach(next.suspensionCompression, suspensionTarget, 4.8f * dt);
    const float roughnessLoad = std::clamp(surface.roughness, 0.0f, 1.0f) * 0.12f;
    const float handbrakeSlipCue = input.handbrake && steeringInput
                                       ? 0.16f + (std::fabs(next.speed) > 1.0f ? 0.20f : 0.0f)
                                       : 0.0f;
    const float slipVisualTarget = std::clamp(std::fabs(next.lateralSlip) / 0.85f +
                                                  handbrakeSlipCue +
                                                  next.driftAmount * 0.40f + roughnessLoad,
                                              0.0f,
                                              1.0f);
    next.wheelSlipVisual = approach(next.wheelSlipVisual, slipVisualTarget, 5.5f * dt);
    next.conditionBand = bandForCondition(next.condition);
    return next;
}

VehicleMoveProposal VehicleController::previewMove(const InputState& input, float deltaSeconds) const {
    return previewMove(input, deltaSeconds, VehicleSurfaceResponse{});
}

VehicleMoveProposal VehicleController::previewMove(const InputState& input,
                                                   float deltaSeconds,
                                                   const VehicleSurfaceResponse& surface) const {
    VehicleMoveProposal proposal;
    proposal.previousState = state_;
    proposal.previousPosition = state_.position;
    proposal.proposedState = simulate(state_, input, deltaSeconds, surface);
    proposal.desiredPosition = proposal.proposedState.position;
    return proposal;
}

void VehicleController::applyMoveResolution(const VehicleMoveProposal& proposal,
                                            const VehicleCollisionResult& collisionResult) {
    state_ = proposal.proposedState;
    state_.position = collisionResult.position;
    const bool movedLessThanDesired = distanceSquaredXZ(collisionResult.position, proposal.desiredPosition) > 0.0004f;
    if ((collisionResult.hit || movedLessThanDesired) && collisionResult.impactSpeed > 0.0f) {
        applyCollisionImpact(collisionResult.impactSpeed);
    } else {
        updateConditionBand();
    }
}

void VehicleController::update(const InputState& input, float deltaSeconds) {
    VehicleMoveProposal proposal = previewMove(input, deltaSeconds);
    VehicleCollisionResult clearMove;
    clearMove.position = proposal.desiredPosition;
    applyMoveResolution(proposal, clearMove);
}

void VehicleController::applyCollisionImpact(float impactSpeed) {
    const float impact = std::max(0.0f, impactSpeed - 2.0f);
    if (impact <= 0.0f) {
        return;
    }

    state_.condition = std::max(0.0f, state_.condition - impact * 1.15f);
    state_.instability = std::max(state_.instability, std::clamp(impactSpeed / 18.0f, 0.15f, 1.0f));
    state_.lateralSlip = std::max(state_.lateralSlip, std::clamp(impactSpeed * 0.025f, 0.0f, 0.8f));
    state_.suspensionCompression = std::max(state_.suspensionCompression, std::clamp(impactSpeed / 80.0f, 0.04f, 0.16f));
    state_.wheelSlipVisual = std::max(state_.wheelSlipVisual, std::clamp(impactSpeed / 14.0f, 0.20f, 1.0f));
    state_.driftAngleRadians = approach(state_.driftAngleRadians, 0.0f, impactSpeed * 0.02f);
    state_.driftAmount = std::clamp(std::fabs(state_.driftAngleRadians) /
                                        std::max(0.001f, config_.driftMaxAngleRadians),
                                    0.0f,
                                    1.0f);
    state_.driftActive = state_.driftAmount > 0.045f && std::fabs(state_.speed) > 0.70f;
    state_.speed *= impactSpeed > 8.0f ? 0.35f : 0.62f;
    updateConditionBand();
}

void VehicleController::setPosition(Vec3 position) {
    state_.position = position;
}

void VehicleController::setYaw(float yawRadians) {
    state_.yawRadians = wrapAngle(yawRadians);
}

void VehicleController::setCondition(float condition) {
    state_.condition = std::clamp(condition, 0.0f, 100.0f);
    updateConditionBand();
}

Vec3 VehicleController::position() const {
    return state_.position;
}

float VehicleController::yawRadians() const {
    return state_.yawRadians;
}

float VehicleController::speed() const {
    return state_.speed;
}

float VehicleController::speedRatio() const {
    if (config_.maxForwardSpeed <= 0.0f) {
        return 0.0f;
    }

    return std::clamp(std::fabs(state_.speed) / config_.maxForwardSpeed, 0.0f, 1.0f);
}

float VehicleController::maxForwardSpeed() const {
    return config_.maxForwardSpeed;
}

float VehicleController::condition() const {
    return state_.condition;
}

VehicleConditionBand VehicleController::conditionBand() const {
    return state_.conditionBand;
}

float VehicleController::instability() const {
    return state_.instability;
}

float VehicleController::lateralSlip() const {
    return state_.lateralSlip;
}

float VehicleController::hornPulse() const {
    return state_.hornPulse;
}

float VehicleController::hornCooldown() const {
    return state_.hornCooldown;
}

bool VehicleController::boostActive() const {
    return state_.boostActive;
}

const VehicleTuningConfig& VehicleController::config() const {
    return config_;
}

const VehicleRuntimeState& VehicleController::runtimeState() const {
    return state_;
}

void VehicleController::updateConditionBand() {
    state_.conditionBand = bandForCondition(state_.condition);
}

} // namespace bs3d
