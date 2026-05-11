#pragma once

#include "bs3d/core/Types.h"
#include "bs3d/core/WorldCollision.h"

namespace bs3d {

enum class PlayerMovementMode {
    Grounded,
    Airborne,
    InteractionLocked
};

enum class PlayerSpeedState {
    Idle,
    Walk,
    Jog,
    Sprint,
    Airborne,
    Landing
};

enum class PlayerStatus {
    Scared,
    Tired,
    Bruised
};

struct PlayerStatusState {
    bool scared = false;
    bool tired = false;
    bool bruised = false;
};

struct PlayerInputIntent {
    Vec3 moveDirection{};
    bool walk = false;
    bool sprint = false;
    bool jumpPressed = false;
    bool interactionLocked = false;

    static PlayerInputIntent fromInputState(const InputState& input);
};

struct PlayerMotorConfig {
    float walkSpeed = 2.7f;
    float jogSpeed = 4.8f;
    float sprintSpeed = 7.2f;
    float acceleration = 34.0f;
    float deceleration = 30.0f;
    float quickTurnAcceleration = 58.0f;
    float rotationRadiansPerSecond = 14.0f;
    float jumpVelocity = 6.4f;
    float gravity = 18.0f;
    float coyoteTime = 0.12f;
    float jumpBufferTime = 0.10f;
    float groundSnapDistance = 0.28f;
    float stepHeight = 0.36f;
    float walkableSlopeDegrees = 38.0f;
    float radius = 0.45f;
    float skinWidth = 0.05f;
    float landingRecoverySeconds = 0.16f;
};

struct PlayerMotorState {
    Vec3 position{};
    Vec3 velocity{};
    float facingYawRadians = 0.0f;
    bool grounded = true;
    GroundHit ground{};
    PlayerMovementMode movementMode = PlayerMovementMode::Grounded;
    PlayerSpeedState speedState = PlayerSpeedState::Idle;
    float landingRecoverySeconds = 0.0f;
    float staggerSeconds = 0.0f;
    float impactIntensity = 0.0f;
    float panicAmount = 0.0f;
    bool hitWallThisFrame = false;
    int hitOwnerIdThisFrame = 0;
    float hitSpeedThisFrame = 0.0f;
    PlayerStatusState statuses{};
};

class PlayerMotor {
public:
    void reset(Vec3 position, float yawRadians = 0.0f);
    void teleportTo(Vec3 position);
    void setFacingYaw(float yawRadians);
    void clearVelocity();
    void forceGrounded();
    void syncToVehicleSeat(Vec3 position, float yawRadians);
    void syncAfterVehicleExit(Vec3 position, float yawRadians);
    void update(const PlayerInputIntent& intent, const WorldCollision& collision, float deltaSeconds);
    void applyImpulse(Vec3 impulse);
    void setStatus(PlayerStatus status, bool enabled);
    void triggerStagger(Vec3 impulse, float durationSeconds, float intensity);

    const PlayerMotorState& state() const;
    const PlayerMotorConfig& config() const;
    PlayerMotorConfig& config();

private:
    PlayerMotorConfig config_{};
    PlayerMotorState state_{};
    float timeSinceGrounded_ = 0.0f;
    float jumpBufferSeconds_ = 0.0f;
    float bumpCooldownSeconds_ = 0.0f;
    float supportOwnerGraceSeconds_ = 0.0f;
    int supportOwnerId_ = 0;
    bool jumpConsumed_ = false;

    void updateSpeedState(const PlayerInputIntent& intent);
};

} // namespace bs3d
