#include "bs3d/core/PlayerMotor.h"

#include <algorithm>
#include <cmath>

namespace bs3d {

namespace {

float rotateToward(float current, float target, float maxDelta) {
    const float delta = wrapAngle(target - current);
    return wrapAngle(current + std::clamp(delta, -maxDelta, maxDelta));
}

constexpr float SlowPlatformCarrySpeed = 3.25f;
constexpr float FastPlatformFallOffSpeed = 5.75f;

CharacterCollisionConfig makeCharacterCollisionConfig(const PlayerMotorConfig& config, int ignoreOwnerId) {
    CharacterCollisionConfig collisionConfig;
    collisionConfig.radius = config.radius;
    collisionConfig.skinWidth = config.skinWidth;
    collisionConfig.stepHeight = config.stepHeight;
    collisionConfig.walkableSlopeDegrees = config.walkableSlopeDegrees;
    collisionConfig.groundProbeDistance = config.groundSnapDistance + config.stepHeight;
    collisionConfig.ignoreBlockerOwnerId = ignoreOwnerId;
    return collisionConfig;
}

} // namespace

PlayerInputIntent PlayerInputIntent::fromInputState(const InputState& input) {
    Vec3 rawMove{};
    if (input.moveForward) {
        rawMove.z += 1.0f;
    }
    if (input.moveBackward) {
        rawMove.z -= 1.0f;
    }
    if (input.moveLeft) {
        rawMove.x -= 1.0f;
    }
    if (input.moveRight) {
        rawMove.x += 1.0f;
    }

    rawMove = normalizeXZ(rawMove);
    const Vec3 cameraForward = forwardFromYaw(input.cameraYawRadians);
    const Vec3 cameraRight = screenRightFromYaw(input.cameraYawRadians);

    PlayerInputIntent intent;
    intent.moveDirection = normalizeXZ(cameraForward * rawMove.z + cameraRight * rawMove.x);
    intent.walk = input.walk;
    intent.sprint = input.sprint;
    intent.jumpPressed = input.jumpPressed;
    intent.interactionLocked = input.interactionLocked;
    return intent;
}

void PlayerMotor::reset(Vec3 position, float yawRadians) {
    state_ = {};
    state_.position = position;
    state_.facingYawRadians = yawRadians;
    state_.grounded = position.y <= 0.0f;
    if (state_.grounded) {
        state_.position.y = 0.0f;
    }
    state_.movementMode = state_.grounded ? PlayerMovementMode::Grounded : PlayerMovementMode::Airborne;
    timeSinceGrounded_ = state_.grounded ? 0.0f : config_.coyoteTime + 1.0f;
    jumpBufferSeconds_ = 0.0f;
    jumpConsumed_ = false;
    supportOwnerGraceSeconds_ = 0.0f;
    supportOwnerId_ = 0;
}

void PlayerMotor::teleportTo(Vec3 position) {
    state_.position = position;
    if (state_.position.y <= 0.0f && state_.velocity.y <= 0.0f) {
        state_.position.y = 0.0f;
        state_.grounded = true;
        state_.movementMode = PlayerMovementMode::Grounded;
        timeSinceGrounded_ = 0.0f;
        jumpConsumed_ = false;
    } else if (state_.position.y > 0.0f) {
        state_.grounded = false;
        state_.movementMode = PlayerMovementMode::Airborne;
    }
}

void PlayerMotor::setFacingYaw(float yawRadians) {
    state_.facingYawRadians = wrapAngle(yawRadians);
}

void PlayerMotor::clearVelocity() {
    state_.velocity = {};
    state_.landingRecoverySeconds = 0.0f;
    state_.staggerSeconds = 0.0f;
    state_.impactIntensity = 0.0f;
    state_.panicAmount = 0.0f;
    bumpCooldownSeconds_ = 0.0f;
    supportOwnerGraceSeconds_ = 0.0f;
    supportOwnerId_ = 0;
}

void PlayerMotor::forceGrounded() {
    state_.grounded = true;
    state_.movementMode = PlayerMovementMode::Grounded;
    state_.speedState = PlayerSpeedState::Idle;
    state_.ground = {true, state_.position.y, {0.0f, 1.0f, 0.0f}, 0.0f, true};
    timeSinceGrounded_ = 0.0f;
    jumpBufferSeconds_ = 0.0f;
    jumpConsumed_ = false;
    supportOwnerGraceSeconds_ = state_.ground.ownerId != 0 ? 0.35f : 0.0f;
    supportOwnerId_ = state_.ground.ownerId;
}

void PlayerMotor::syncToVehicleSeat(Vec3 position, float yawRadians) {
    state_.position = position;
    setFacingYaw(yawRadians);
    clearVelocity();
    forceGrounded();
}

void PlayerMotor::syncAfterVehicleExit(Vec3 position, float yawRadians) {
    state_.position = position;
    setFacingYaw(yawRadians);
    clearVelocity();
    forceGrounded();
}

void PlayerMotor::update(const PlayerInputIntent& inputIntent, const WorldCollision& collision, float deltaSeconds) {
    const float dt = std::max(0.0f, deltaSeconds);
    PlayerInputIntent intent = inputIntent;
    // Re-normalize to protect against unnormalized input from external callers.
    intent.moveDirection = normalizeXZ(intent.moveDirection);
    state_.hitWallThisFrame = false;
    state_.hitOwnerIdThisFrame = 0;
    state_.hitSpeedThisFrame = 0.0f;

    // Decay temporary state: impact intensity, stagger, bump cooldown, platform grace.
    state_.impactIntensity = std::max(0.0f, state_.impactIntensity - dt * 3.0f);
    state_.staggerSeconds = std::max(0.0f, state_.staggerSeconds - dt);
    bumpCooldownSeconds_ = std::max(0.0f, bumpCooldownSeconds_ - dt);
    supportOwnerGraceSeconds_ = std::max(0.0f, supportOwnerGraceSeconds_ - dt);
    if (supportOwnerGraceSeconds_ <= 0.0f) {
        supportOwnerId_ = 0;
    }

    if (intent.interactionLocked) {
        intent.moveDirection = {};
        intent.walk = false;
        intent.sprint = false;
        intent.jumpPressed = false;
        jumpBufferSeconds_ = 0.0f;
    } else if (intent.jumpPressed) {
        jumpBufferSeconds_ = config_.jumpBufferTime;
    } else {
        jumpBufferSeconds_ = std::max(0.0f, jumpBufferSeconds_ - dt);
    }

    const bool wasGrounded = state_.grounded;
    // Phase 1 — ground detection and platform handling.
    const GroundHit currentGround = collision.probeGround(
        state_.position,
        config_.groundSnapDistance,
        config_.walkableSlopeDegrees);

    const bool canStandOnCurrentGround = currentGround.hit && currentGround.walkable &&
                                         std::fabs(state_.position.y - currentGround.height) <=
                                             config_.groundSnapDistance;
    if (canStandOnCurrentGround && state_.velocity.y <= 0.0f) {
        state_.position.y = currentGround.height;
        state_.velocity.y = 0.0f;
        state_.grounded = true;
        state_.ground = currentGround;
        timeSinceGrounded_ = 0.0f;
        jumpConsumed_ = false;
        supportOwnerId_ = currentGround.ownerId;
        supportOwnerGraceSeconds_ = currentGround.ownerId != 0 ? 0.35f : 0.0f;
        const float platformSpeed = std::sqrt(lengthSquaredXZ(currentGround.platformVelocity));
        if (currentGround.ownerId != 0 && platformSpeed > 0.01f) {
            if (platformSpeed <= SlowPlatformCarrySpeed) {
                const Vec3 platformMoved = state_.position + currentGround.platformVelocity * dt;
                const CharacterCollisionConfig platformConfig =
                    makeCharacterCollisionConfig(config_, currentGround.ownerId);
                const CharacterCollisionResult platformResolved =
                    collision.resolveCharacter(state_.position, platformMoved, platformConfig);
                state_.position = platformResolved.position;
                if (platformResolved.hitWall) {
                    state_.hitWallThisFrame = true;
                    state_.hitOwnerIdThisFrame = platformResolved.hitOwnerId;
                    state_.hitSpeedThisFrame = platformSpeed;
                }
            } else if (platformSpeed >= FastPlatformFallOffSpeed) {
                state_.velocity.x += currentGround.platformVelocity.x * 0.38f;
                state_.velocity.z += currentGround.platformVelocity.z * 0.38f;
                state_.velocity.y = std::max(state_.velocity.y, config_.jumpVelocity * 0.28f);
                state_.grounded = false;
                state_.movementMode = PlayerMovementMode::Airborne;
                timeSinceGrounded_ = config_.coyoteTime + 1.0f;
                supportOwnerGraceSeconds_ = 0.16f;
            }
        }
    } else {
        state_.grounded = false;
        if (wasGrounded) {
            timeSinceGrounded_ = 0.0f;
        } else {
            timeSinceGrounded_ += dt;
        }
    }

    // Phase 2 — speed, acceleration, and facing.
    const bool hasMove = lengthSquaredXZ(intent.moveDirection) > 0.0001f;
    float targetSpeed = hasMove ? (intent.sprint ? config_.sprintSpeed : (intent.walk ? config_.walkSpeed : config_.jogSpeed)) : 0.0f;
    float acceleration = config_.acceleration;
    if (state_.statuses.scared && intent.sprint) {
        targetSpeed += 0.75f;
        acceleration += 4.0f;
    }
    if (state_.statuses.tired && intent.sprint) {
        targetSpeed *= 0.82f;
    }
    if (state_.statuses.bruised) {
        targetSpeed *= 0.92f;
        acceleration *= 0.90f;
    }

    if (hasMove && lengthSquaredXZ(state_.velocity) > 0.05f) {
        const Vec3 currentDirection = normalizeXZ(state_.velocity);
        const float intentDot = currentDirection.x * intent.moveDirection.x + currentDirection.z * intent.moveDirection.z;
        if (intentDot < -0.35f) {
            acceleration = std::max(acceleration, config_.quickTurnAcceleration);
        }
    }

    const float staggerControl = state_.staggerSeconds > 0.0f ? 0.72f : 1.0f;
    const Vec3 desiredHorizontalVelocity = intent.moveDirection * (targetSpeed * staggerControl);
    const float maxDelta = (hasMove ? acceleration : config_.deceleration) * dt;
    state_.velocity.x = approach(state_.velocity.x, desiredHorizontalVelocity.x, maxDelta);
    state_.velocity.z = approach(state_.velocity.z, desiredHorizontalVelocity.z, maxDelta);

    const float horizontalSpeed = std::sqrt(lengthSquaredXZ(state_.velocity));
    if (horizontalSpeed > targetSpeed && targetSpeed > 0.0f) {
        const Vec3 clamped = normalizeXZ(state_.velocity) * targetSpeed;
        state_.velocity.x = clamped.x;
        state_.velocity.z = clamped.z;
    }

    // Phase 3 — jump (with coyote time window).
    const bool canCoyoteJump = timeSinceGrounded_ <= config_.coyoteTime;
    if (jumpBufferSeconds_ > 0.0f && !jumpConsumed_ && (state_.grounded || canCoyoteJump)) {
        state_.velocity.y = config_.jumpVelocity;
        state_.grounded = false;
        state_.movementMode = PlayerMovementMode::Airborne;
        jumpConsumed_ = true;
        jumpBufferSeconds_ = 0.0f;
        timeSinceGrounded_ = config_.coyoteTime + 1.0f;
    }

    if (!state_.grounded) {
        state_.velocity.y -= config_.gravity * dt;
    }

    if (hasMove) {
        state_.facingYawRadians = rotateToward(
            state_.facingYawRadians,
            yawFromDirection(intent.moveDirection),
            config_.rotationRadiansPerSecond * dt);
    }

    // Phase 4 — collision resolve, wall bump, ground snap.
    Vec3 desired = state_.position + state_.velocity * dt;
    const CharacterCollisionConfig collisionConfig =
        makeCharacterCollisionConfig(config_, supportOwnerGraceSeconds_ > 0.0f ? supportOwnerId_ : 0);

    const float impactSpeed = std::sqrt(lengthSquaredXZ(state_.velocity));
    const CharacterCollisionResult resolved = collision.resolveCharacter(state_.position, desired, collisionConfig);
    if (resolved.hitWall) {
        state_.hitWallThisFrame = true;
        state_.hitOwnerIdThisFrame = resolved.hitOwnerId;
        state_.hitSpeedThisFrame = impactSpeed;
        const bool shouldBump = impactSpeed > config_.jogSpeed * 0.78f && bumpCooldownSeconds_ <= 0.0f;
        const Vec3 bumpImpulse = shouldBump ? normalizeXZ(state_.velocity) * -1.35f : Vec3{};
        if (impactSpeed > config_.jogSpeed * 0.78f && bumpCooldownSeconds_ <= 0.0f) {
            bumpCooldownSeconds_ = 0.45f;
        }
        if (std::fabs(resolved.position.x - desired.x) > 0.001f) {
            state_.velocity.x = 0.0f;
        }
        if (std::fabs(resolved.position.z - desired.z) > 0.001f) {
            state_.velocity.z = 0.0f;
        }
        if (shouldBump) {
            triggerStagger(bumpImpulse, 0.22f, std::clamp(impactSpeed / config_.sprintSpeed, 0.2f, 1.0f));
        }
    }
    desired = resolved.position;

    const GroundHit desiredGround = collision.probeGround(
        desired,
        config_.groundSnapDistance,
        config_.walkableSlopeDegrees);
    const bool canSnap = desiredGround.hit && desiredGround.walkable &&
                         desired.y <= desiredGround.height + config_.groundSnapDistance &&
                         state_.velocity.y <= 0.0f;

    state_.position = desired;
    if (canSnap) {
        state_.position.y = desiredGround.height;
        state_.velocity.y = 0.0f;
        state_.ground = desiredGround;
        state_.grounded = true;
        timeSinceGrounded_ = 0.0f;
        jumpConsumed_ = false;
        supportOwnerId_ = desiredGround.ownerId;
        supportOwnerGraceSeconds_ = desiredGround.ownerId != 0 ? 0.35f : 0.0f;
        if (!wasGrounded) {
            state_.landingRecoverySeconds = config_.landingRecoverySeconds;
        }
    } else {
        state_.ground = desiredGround;
        state_.grounded = false;
        state_.movementMode = PlayerMovementMode::Airborne;
    }

    state_.landingRecoverySeconds = std::max(0.0f, state_.landingRecoverySeconds - dt);
    state_.panicAmount = state_.statuses.scared && intent.sprint ? 1.0f : 0.0f;
    updateSpeedState(intent);
}

void PlayerMotor::applyImpulse(Vec3 impulse) {
    state_.velocity = state_.velocity + impulse;
    if (impulse.y > 0.0f) {
        state_.grounded = false;
        state_.movementMode = PlayerMovementMode::Airborne;
    }
}

void PlayerMotor::setStatus(PlayerStatus status, bool enabled) {
    switch (status) {
    case PlayerStatus::Scared:
        state_.statuses.scared = enabled;
        break;
    case PlayerStatus::Tired:
        state_.statuses.tired = enabled;
        break;
    case PlayerStatus::Bruised:
        state_.statuses.bruised = enabled;
        break;
    }
}

void PlayerMotor::triggerStagger(Vec3 impulse, float durationSeconds, float intensity) {
    state_.staggerSeconds = std::max(state_.staggerSeconds, std::max(0.0f, durationSeconds));
    state_.impactIntensity = std::max(state_.impactIntensity, std::clamp(intensity, 0.0f, 1.0f));
    state_.velocity = state_.velocity + impulse;
    if (impulse.y > 0.0f) {
        state_.grounded = false;
        state_.movementMode = PlayerMovementMode::Airborne;
        timeSinceGrounded_ = config_.coyoteTime + 1.0f;
    }
}

const PlayerMotorState& PlayerMotor::state() const {
    return state_;
}

const PlayerMotorConfig& PlayerMotor::config() const {
    return config_;
}

PlayerMotorConfig& PlayerMotor::config() {
    return config_;
}

void PlayerMotor::updateSpeedState(const PlayerInputIntent& intent) {
    if (intent.interactionLocked) {
        state_.movementMode = PlayerMovementMode::InteractionLocked;
        state_.speedState = PlayerSpeedState::Idle;
        return;
    }

    if (!state_.grounded) {
        state_.movementMode = PlayerMovementMode::Airborne;
        state_.speedState = PlayerSpeedState::Airborne;
        return;
    }

    state_.movementMode = PlayerMovementMode::Grounded;
    if (state_.landingRecoverySeconds > 0.0f) {
        state_.speedState = PlayerSpeedState::Landing;
        return;
    }

    const float speed = std::sqrt(lengthSquaredXZ(state_.velocity));
    if (speed < 0.12f) {
        state_.speedState = PlayerSpeedState::Idle;
    } else if (intent.sprint) {
        state_.speedState = PlayerSpeedState::Sprint;
    } else if (intent.walk) {
        state_.speedState = PlayerSpeedState::Walk;
    } else {
        state_.speedState = PlayerSpeedState::Jog;
    }
}

} // namespace bs3d
