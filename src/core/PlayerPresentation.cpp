#include "bs3d/core/PlayerPresentation.h"

#include <algorithm>
#include <cmath>

namespace bs3d {

namespace {

float smooth(float current, float target, float response, float dt) {
    if (dt <= 0.0f) {
        return current;
    }
    const float alpha = 1.0f - std::exp(-response * dt);
    return current + (target - current) * alpha;
}

CharacterPoseKind poseKindFor(const PlayerMotorState& motorState,
                              float worldTension,
                              const PlayerPresentationContext& context) {
    if (context.inVehicle) {
        if (context.steerInput < -0.35f) {
            return CharacterPoseKind::SteerLeft;
        }
        if (context.steerInput > 0.35f) {
            return CharacterPoseKind::SteerRight;
        }
        return CharacterPoseKind::SitVehicle;
    }
    if (context.interactActive) {
        return CharacterPoseKind::Interact;
    }
    if (context.talkActive) {
        return CharacterPoseKind::Talk;
    }
    if (motorState.staggerSeconds > 0.0f) {
        return CharacterPoseKind::Stagger;
    }
    if (motorState.speedState == PlayerSpeedState::Landing || motorState.landingRecoverySeconds > 0.0f) {
        return CharacterPoseKind::Land;
    }
    if (!motorState.grounded || motorState.speedState == PlayerSpeedState::Airborne) {
        return CharacterPoseKind::Jump;
    }
    if (context.standingOnVehicle && lengthSquaredXZ(motorState.velocity) < 0.35f) {
        return CharacterPoseKind::StandVehicle;
    }
    if (context.carryingObject) {
        return CharacterPoseKind::CarryObject;
    }
    if (motorState.speedState == PlayerSpeedState::Sprint) {
        const float panic = std::max(motorState.panicAmount, worldTension);
        return panic > 0.35f ? CharacterPoseKind::PanicSprint : CharacterPoseKind::Sprint;
    }
    if (motorState.speedState == PlayerSpeedState::Jog) {
        return CharacterPoseKind::Jog;
    }
    if (motorState.speedState == PlayerSpeedState::Walk) {
        return CharacterPoseKind::Walk;
    }
    return CharacterPoseKind::Idle;
}

bool canUseLocomotionTransitions(const PlayerMotorState& motorState, const PlayerPresentationContext& context) {
    return !context.inVehicle &&
           !context.interactActive &&
           !context.talkActive &&
           motorState.grounded &&
           motorState.staggerSeconds <= 0.0f &&
           motorState.speedState != PlayerSpeedState::Landing &&
           motorState.speedState != PlayerSpeedState::Airborne;
}

bool isVehicleSeatedPose(CharacterPoseKind poseKind) {
    return poseKind == CharacterPoseKind::SitVehicle ||
           poseKind == CharacterPoseKind::SteerLeft ||
           poseKind == CharacterPoseKind::SteerRight;
}

} // namespace

void PlayerPresentation::playOneShot(CharacterPoseKind kind, float durationSeconds) {
    oneShotKind_ = kind;
    oneShotDurationSeconds_ = std::max(0.01f, durationSeconds);
    oneShotSeconds_ = oneShotDurationSeconds_;
}

bool PlayerPresentation::oneShotActive() const {
    return oneShotSeconds_ > 0.0f;
}

void PlayerPresentation::update(const PlayerMotorState& motorState,
                                float deltaSeconds,
                                float worldTension,
                                const PlayerPresentationContext& context) {
    const float dt = std::max(0.0f, deltaSeconds);
    if (oneShotSeconds_ > 0.0f) {
        oneShotSeconds_ = std::max(0.0f, oneShotSeconds_ - dt);
    }
    startMoveSeconds_ = std::max(0.0f, startMoveSeconds_ - dt);
    stopMoveSeconds_ = std::max(0.0f, stopMoveSeconds_ - dt);
    quickTurnSeconds_ = std::max(0.0f, quickTurnSeconds_ - dt);
    jumpOffVehicleSeconds_ = std::max(0.0f, jumpOffVehicleSeconds_ - dt);

    const float speed = std::sqrt(lengthSquaredXZ(motorState.velocity));
    const Vec3 moveDirection = normalizeXZ(motorState.velocity);
    const bool onDynamicSupport = context.standingOnVehicle ||
                                  (motorState.grounded && motorState.ground.ownerId != 0);
    if (wasOnDynamicSupport_ && !onDynamicSupport && !motorState.grounded) {
        jumpOffVehicleSeconds_ = 0.30f;
    }
    if (canUseLocomotionTransitions(motorState, context)) {
        if (previousXzSpeed_ < 0.30f && speed > 2.25f) {
            startMoveSeconds_ = 0.18f;
        }
        if (previousXzSpeed_ > 1.15f && speed < 0.35f) {
            stopMoveSeconds_ = 0.17f;
        }
        if (hasPreviousMoveDirection_ && previousXzSpeed_ > 1.80f && speed > 1.80f) {
            const float dot = previousMoveDirection_.x * moveDirection.x + previousMoveDirection_.z * moveDirection.z;
            if (dot < -0.52f) {
                quickTurnSeconds_ = 0.24f;
                startMoveSeconds_ = 0.0f;
                stopMoveSeconds_ = 0.0f;
            }
        }
    }

    const bool oneShotStillActive = oneShotSeconds_ > 0.0f;
    state_.oneShotNormalizedTime = oneShotDurationSeconds_ > 0.0f
                                       ? 1.0f - std::clamp(oneShotSeconds_ / oneShotDurationSeconds_, 0.0f, 1.0f)
                                       : 0.0f;
    if (oneShotStillActive) {
        state_.poseKind = oneShotKind_;
    } else if (jumpOffVehicleSeconds_ > 0.0f) {
        state_.poseKind = CharacterPoseKind::JumpOffVehicle;
    } else if (quickTurnSeconds_ > 0.0f) {
        state_.poseKind = CharacterPoseKind::QuickTurn;
    } else if (startMoveSeconds_ > 0.0f) {
        state_.poseKind = CharacterPoseKind::StartMove;
    } else if (stopMoveSeconds_ > 0.0f) {
        state_.poseKind = CharacterPoseKind::StopMove;
    } else {
        state_.poseKind = poseKindFor(motorState, worldTension, context);
    }
    const Vec3 right = screenRightFromYaw(motorState.facingYawRadians);
    const float sideSpeed = motorState.velocity.x * right.x + motorState.velocity.z * right.z;
    const float targetLean = std::clamp(-sideSpeed * 0.035f, -0.18f, 0.18f);

    state_.leanRadians = smooth(state_.leanRadians, targetLean, 10.0f, dt);
    if (motorState.grounded && speed > 0.15f) {
        state_.stepPhase += speed * dt * 5.8f;
    }
    const bool actionLocksLocomotion = state_.poseKind == CharacterPoseKind::SitVehicle ||
                                       state_.poseKind == CharacterPoseKind::SteerLeft ||
                                       state_.poseKind == CharacterPoseKind::SteerRight ||
                                       state_.poseKind == CharacterPoseKind::Talk ||
                                       state_.poseKind == CharacterPoseKind::Interact ||
                                       state_.poseKind == CharacterPoseKind::StandVehicle;
    const float targetLocomotion = motorState.grounded && !actionLocksLocomotion
                                       ? std::clamp(speed / 7.2f, 0.0f, 1.0f)
                                       : 0.0f;
    state_.locomotionAmount = smooth(state_.locomotionAmount, targetLocomotion, 18.0f, dt);
    const float swing = std::sin(state_.stepPhase) * state_.locomotionAmount;
    state_.leftLegSwing = swing;
    state_.rightLegSwing = -swing;
    state_.leftArmSwing = -swing * 0.65f;
    state_.rightArmSwing = swing * 0.65f;
    state_.bobHeight = motorState.grounded ? std::sin(state_.stepPhase) * std::min(speed * 0.012f, 0.065f) : 0.0f;
    state_.sway = motorState.grounded ? std::sin(state_.stepPhase * 0.5f) * std::min(speed * 0.008f, 0.045f) : 0.0f;
    const float targetDip = motorState.speedState == PlayerSpeedState::Landing
                                ? std::clamp(motorState.landingRecoverySeconds * 0.55f, 0.0f, 0.10f)
                                : 0.0f;
    state_.landingDip = smooth(state_.landingDip, targetDip, 18.0f, dt);
    const float jumpTarget = !motorState.grounded || motorState.speedState == PlayerSpeedState::Airborne ? 1.0f : 0.0f;
    state_.jumpStretch = smooth(state_.jumpStretch, jumpTarget, 16.0f, dt);
    state_.talkGesture = smooth(state_.talkGesture,
                                state_.poseKind == CharacterPoseKind::Talk ? 1.0f : 0.0f,
                                12.0f,
                                dt);
    state_.interactReach = smooth(state_.interactReach,
                                  state_.poseKind == CharacterPoseKind::Interact ? 1.0f : 0.0f,
                                  14.0f,
                                  dt);
    state_.vehicleSitAmount = smooth(state_.vehicleSitAmount,
                                     isVehicleSeatedPose(state_.poseKind) ? 1.0f : 0.0f,
                                     16.0f,
                                     dt);
    state_.steerAmount = smooth(state_.steerAmount,
                                isVehicleSeatedPose(state_.poseKind)
                                    ? std::clamp(context.steerInput, -1.0f, 1.0f)
                                    : 0.0f,
                                14.0f,
                                dt);
    state_.balanceAmount = smooth(state_.balanceAmount,
                                  state_.poseKind == CharacterPoseKind::StandVehicle ? 1.0f : 0.0f,
                                  10.0f,
                                  dt);
    const float actionTarget = oneShotStillActive
                                   ? std::clamp(oneShotSeconds_ / std::max(oneShotDurationSeconds_, 0.01f), 0.0f, 1.0f)
                                   : 0.0f;
    state_.actionAmount = smooth(state_.actionAmount, actionTarget, 18.0f, dt);
    state_.startMoveAmount = smooth(state_.startMoveAmount,
                                    state_.poseKind == CharacterPoseKind::StartMove ? 1.0f : 0.0f,
                                    18.0f,
                                    dt);
    state_.stopMoveAmount = smooth(state_.stopMoveAmount,
                                   state_.poseKind == CharacterPoseKind::StopMove ? 1.0f : 0.0f,
                                   18.0f,
                                   dt);
    state_.quickTurnAmount = smooth(state_.quickTurnAmount,
                                    state_.poseKind == CharacterPoseKind::QuickTurn ? 1.0f : 0.0f,
                                    20.0f,
                                    dt);
    state_.punchAmount = smooth(state_.punchAmount,
                                state_.poseKind == CharacterPoseKind::Punch ? 1.0f : 0.0f,
                                22.0f,
                                dt);
    state_.dodgeAmount = smooth(state_.dodgeAmount,
                                state_.poseKind == CharacterPoseKind::Dodge ? 1.0f : 0.0f,
                                18.0f,
                                dt);
    state_.carryAmount = smooth(state_.carryAmount,
                                state_.poseKind == CharacterPoseKind::CarryObject ? 1.0f : 0.0f,
                                16.0f,
                                dt);
    state_.pushAmount = smooth(state_.pushAmount,
                               state_.poseKind == CharacterPoseKind::PushObject ? 1.0f : 0.0f,
                               18.0f,
                               dt);
    state_.drinkAmount = smooth(state_.drinkAmount,
                                state_.poseKind == CharacterPoseKind::Drink ? 1.0f : 0.0f,
                                14.0f,
                                dt);
    state_.smokeAmount = smooth(state_.smokeAmount,
                                state_.poseKind == CharacterPoseKind::Smoke ? 1.0f : 0.0f,
                                14.0f,
                                dt);
    state_.fallAmount = smooth(state_.fallAmount,
                               state_.poseKind == CharacterPoseKind::Fall ? 1.0f : 0.0f,
                               18.0f,
                               dt);
    state_.getUpAmount = smooth(state_.getUpAmount,
                                state_.poseKind == CharacterPoseKind::GetUp ? 1.0f : 0.0f,
                                16.0f,
                                dt);
    state_.lowVaultAmount = smooth(state_.lowVaultAmount,
                                   state_.poseKind == CharacterPoseKind::LowVault ? 1.0f : 0.0f,
                                   18.0f,
                                   dt);
    state_.jumpOffVehicleAmount = smooth(state_.jumpOffVehicleAmount,
                                         state_.poseKind == CharacterPoseKind::JumpOffVehicle ? 1.0f : 0.0f,
                                         18.0f,
                                         dt);
    const float panicTarget = motorState.speedState == PlayerSpeedState::Sprint
                                  ? std::clamp(std::max(motorState.panicAmount, worldTension) * speed / 7.0f, 0.0f, 1.0f)
                                  : 0.0f;
    state_.panicFlail = smooth(state_.panicFlail, panicTarget, 12.0f, dt);
    state_.staggerAmount = smooth(state_.staggerAmount,
                                  motorState.staggerSeconds > 0.0f ? motorState.impactIntensity : 0.0f,
                                  16.0f,
                                  dt);
    state_.tiredAmount = smooth(state_.tiredAmount,
                                motorState.statuses.tired ? 1.0f : 0.0f,
                                8.0f,
                                dt);
    state_.bruisedAmount = smooth(state_.bruisedAmount,
                                  motorState.statuses.bruised ? 1.0f : 0.0f,
                                  9.0f,
                                  dt);

    previousXzSpeed_ = speed;
    if (speed > 0.25f) {
        previousMoveDirection_ = moveDirection;
        hasPreviousMoveDirection_ = true;
    }
    wasOnDynamicSupport_ = onDynamicSupport;
}

const PlayerPresentationState& PlayerPresentation::state() const {
    return state_;
}

std::vector<CharacterPoseKind> characterPoseCatalog() {
    return {
        CharacterPoseKind::Idle,
        CharacterPoseKind::Walk,
        CharacterPoseKind::Jog,
        CharacterPoseKind::Sprint,
        CharacterPoseKind::StartMove,
        CharacterPoseKind::StopMove,
        CharacterPoseKind::QuickTurn,
        CharacterPoseKind::Jump,
        CharacterPoseKind::Land,
        CharacterPoseKind::Stagger,
        CharacterPoseKind::Talk,
        CharacterPoseKind::Interact,
        CharacterPoseKind::EnterVehicle,
        CharacterPoseKind::ExitVehicle,
        CharacterPoseKind::SitVehicle,
        CharacterPoseKind::SteerLeft,
        CharacterPoseKind::SteerRight,
        CharacterPoseKind::StandVehicle,
        CharacterPoseKind::JumpOffVehicle,
        CharacterPoseKind::LowVault,
        CharacterPoseKind::CarryObject,
        CharacterPoseKind::PushObject,
        CharacterPoseKind::Punch,
        CharacterPoseKind::Dodge,
        CharacterPoseKind::Fall,
        CharacterPoseKind::GetUp,
        CharacterPoseKind::Drink,
        CharacterPoseKind::Smoke,
        CharacterPoseKind::PanicSprint,
    };
}

} // namespace bs3d
