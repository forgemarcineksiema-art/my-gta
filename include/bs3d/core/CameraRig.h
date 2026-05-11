#pragma once

#include "bs3d/core/Types.h"

namespace bs3d {

class WorldCollision;

enum class CameraRigMode {
    OnFootNormal,
    Walking = OnFootNormal,
    OnFootSprint,
    Interaction,
    Vehicle,
    Driving = Vehicle
};

struct CameraRigTarget {
    Vec3 position{};
    Vec3 velocity{};
    float yawRadians = 0.0f;
    CameraRigMode mode = CameraRigMode::Walking;
    float speedRatio = 0.0f;
    bool chaseActive = false;
    float tension = 0.0f;
};

struct CameraRigState {
    Vec3 position{};
    Vec3 desiredPosition{};
    Vec3 target{};
    Vec3 desiredTarget{};
    float fovY = 55.0f;
    float cameraYawRadians = 0.0f;
    float visualYawRadians = 0.0f;
    float pitchRadians = -0.38f;
    float boomLength = 0.0f;
    float fullBoomLength = 0.0f;
};

struct CameraRigControlInput {
    CameraRigMode mode = CameraRigMode::Walking;
    float followYawRadians = 0.0f;
    float speedRatio = 0.0f;
    bool chaseActive = false;
    float tension = 0.0f;
    bool mouseLookActive = true;
    bool invertMouseX = false;
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
};

struct CameraRigConfig {
    float mouseSensitivity = 0.0024f;
    float minPitchRadians = -0.82f;
    float maxPitchRadians = -0.14f;
    float walkingRecenterDelaySeconds = 2.4f;
    float sprintRecenterDelaySeconds = 1.15f;
    float drivingRecenterDelaySeconds = 1.45f;
    float drivingRecenterResponse = 3.0f;
    float boomCollisionRadius = 0.22f;
    float walkingLookAhead = 0.0f;
    float sprintLookAhead = 0.40f;
    float drivingLookAhead = 0.95f;
};

struct GameplayCameraModeInput {
    bool playerInVehicle = false;
    bool explicitInteractionCameraActive = false;
    // Informational only. Dialogue subtitles must not change camera framing.
    bool dialogueLineActive = false;
    float playerSpeed = 0.0f;
    float sprintSpeed = 0.0f;
};

CameraRigMode resolveGameplayCameraMode(const GameplayCameraModeInput& input);
CameraRigMode resolveStableCameraMode(CameraRigMode mode, bool stableCameraMode);

class CameraRig {
public:
    float beginFrame(const CameraRigControlInput& input, float deltaSeconds);
    CameraRigState update(const CameraRigTarget& target, float deltaSeconds, const WorldCollision* collision = nullptr);
    void reset(const CameraRigTarget& target);
    const CameraRigState& state() const;

private:
    CameraRigState state_{};
    CameraRigConfig config_{};
    bool initialized_ = false;
    CameraRigMode lastMode_ = CameraRigMode::Walking;
    float cameraYawRadians_ = 0.0f;
    float pitchRadians_ = -0.38f;
    float recenterDelaySeconds_ = 0.0f;
    bool manualLookThisFrame_ = false;

    CameraRigState desiredState(const CameraRigTarget& target) const;
};

} // namespace bs3d
