#pragma once

#include "bs3d/core/Types.h"

namespace bs3d {

struct VehicleCollisionResult;

enum class VehicleConditionBand {
    Igla,
    JeszczePojezdzi,
    CosStuka,
    ZarazWybuchnieAleNieDzis,
    Zlom
};

struct VehicleTuningConfig {
    float maxForwardSpeed = 15.5f;
    float maxReverseSpeed = -4.8f;
    float acceleration = 4.4f;
    float braking = 13.0f;
    float drag = 3.0f;
    float lowSpeedSteeringRadiansPerSecond = 2.75f;
    float highSpeedSteeringRadiansPerSecond = 0.24f;
    float steeringRiseRate = 7.8f;
    float steeringReleaseRate = 6.2f;
    float grip = 8.0f;
    float driftFactor = 0.032f;
    float handbrakeStrength = 1.65f;
    float handbrakeDrag = 5.6f;
    float handbrakeDriftFactor = 0.22f;
    float driftEntryMinSpeed = 3.2f;
    float driftMaxAngleRadians = 0.68f;
    float driftBuildRate = 2.35f;
    float driftCounterSteerRate = 3.5f;
    float driftExitRate = 2.85f;
    float driftSustainFactor = 0.14f;
    float driftSpeedBleedPerSecond = 0.72f;
    float shiftDurationSeconds = 0.26f;
    float shiftAccelerationMultiplier = 0.38f;
    float boostMultiplier = 1.18f;
    float boostWearPerSecond = 0.20f;
    float damageInfluence = 0.15f;
    float minimumSteeringAuthority = 0.62f;
};

struct VehicleSurfaceResponse {
    float gripMultiplier = 1.0f;
    float dragMultiplier = 1.0f;
    float accelerationMultiplier = 1.0f;
    float roughness = 0.0f;
};

struct VehicleRuntimeState {
    Vec3 position{};
    float yawRadians = 0.0f;
    float speed = 0.0f;
    float lateralSlip = 0.0f;
    float instability = 0.0f;
    float hornPulse = 0.0f;
    float hornCooldown = 0.0f;
    float steeringAuthority = 0.0f;
    float steeringInputSmoothed = 0.0f;
    float wheelSpinRadians = 0.0f;
    float frontWheelSteerRadians = 0.0f;
    float suspensionCompression = 0.0f;
    float wheelSlipVisual = 0.0f;
    float driftAngleRadians = 0.0f;
    float driftAmount = 0.0f;
    bool driftActive = false;
    int gear = 1;
    float rpmNormalized = 0.0f;
    float shiftTimerSeconds = 0.0f;
    bool boostActive = false;
    float condition = 100.0f;
    VehicleConditionBand conditionBand = VehicleConditionBand::Igla;
};

struct VehicleMoveProposal {
    VehicleRuntimeState previousState{};
    VehicleRuntimeState proposedState{};
    Vec3 previousPosition{};
    Vec3 desiredPosition{};
};

class VehicleController {
public:
    void reset(Vec3 position, float yawRadians = 0.0f);
    VehicleMoveProposal previewMove(const InputState& input, float deltaSeconds) const;
    VehicleMoveProposal previewMove(const InputState& input,
                                    float deltaSeconds,
                                    const VehicleSurfaceResponse& surface) const;
    void applyMoveResolution(const VehicleMoveProposal& proposal, const VehicleCollisionResult& collisionResult);
    void update(const InputState& input, float deltaSeconds);
    void applyCollisionImpact(float impactSpeed);

    void setPosition(Vec3 position);
    void setYaw(float yawRadians);
    void setCondition(float condition);
    Vec3 position() const;
    float yawRadians() const;
    float speed() const;
    float speedRatio() const;
    float maxForwardSpeed() const;
    float condition() const;
    VehicleConditionBand conditionBand() const;
    float instability() const;
    float lateralSlip() const;
    float hornPulse() const;
    float hornCooldown() const;
    bool boostActive() const;
    const VehicleTuningConfig& config() const;
    const VehicleRuntimeState& runtimeState() const;

private:
    VehicleTuningConfig config_{};
    VehicleRuntimeState state_{};

    VehicleRuntimeState simulate(const VehicleRuntimeState& state,
                                 const InputState& input,
                                 float deltaSeconds,
                                 const VehicleSurfaceResponse& surface) const;
    void updateConditionBand();
};

} // namespace bs3d
