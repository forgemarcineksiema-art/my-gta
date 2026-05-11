#include "bs3d/core/ChaseSystem.h"

#include <algorithm>

namespace bs3d {

void ChaseSystem::start(Vec3 pursuerPosition, float graceSeconds) {
    state_ = ChaseState::Running;
    pursuerPosition_ = pursuerPosition;
    escapeTimerSeconds_ = 0.0f;
    caughtTimerSeconds_ = 0.0f;
    graceTimerSeconds_ = std::max(0.0f, graceSeconds);
    pressure_ = 0.0f;
}

void ChaseSystem::stop() {
    state_ = ChaseState::Inactive;
    escapeTimerSeconds_ = 0.0f;
    caughtTimerSeconds_ = 0.0f;
    graceTimerSeconds_ = 0.0f;
    pressure_ = 0.0f;
}

ChaseResult ChaseSystem::update(Vec3 playerPosition, float deltaSeconds) {
    return update(playerPosition, pursuerPosition_, deltaSeconds);
}

ChaseResult ChaseSystem::update(Vec3 playerPosition, Vec3 pursuerPosition, float deltaSeconds) {
    ChaseUpdateContext context;
    context.playerPosition = playerPosition;
    context.pursuerPosition = pursuerPosition;
    context.lineOfSight = true;
    return updateWithContext(context, deltaSeconds);
}

ChaseResult ChaseSystem::updateWithContext(const ChaseUpdateContext& context, float deltaSeconds) {
    if (state_ != ChaseState::Running) {
        return ChaseResult::Inactive;
    }

    pursuerPosition_ = context.pursuerPosition;
    const float distance = distanceXZ(context.playerPosition, pursuerPosition_);
    const float dt = std::max(0.0f, deltaSeconds);
    pressure_ = std::clamp(1.0f - distance / escapeDistance_, 0.0f, 1.0f);
    const float relativeSpeed = distanceXZ(context.playerVelocity, context.pursuerVelocity);
    const bool controlledCatch = context.lineOfSight &&
                                 !context.contactRecoverActive &&
                                 relativeSpeed <= controlledCatchRelativeSpeed_;

    if (graceTimerSeconds_ > 0.0f) {
        graceTimerSeconds_ = std::max(0.0f, graceTimerSeconds_ - dt);
        caughtTimerSeconds_ = 0.0f;
        if (distance >= escapeDistance_ || (!context.lineOfSight && distance >= hiddenEscapeDistance_)) {
            escapeTimerSeconds_ += dt;
        } else {
            escapeTimerSeconds_ = 0.0f;
        }
        return ChaseResult::Running;
    }

    if (distance <= failDistance_ && controlledCatch) {
        caughtTimerSeconds_ += dt;
        escapeTimerSeconds_ = 0.0f;
        pressure_ = std::max(pressure_, caughtProgress());
        if (caughtTimerSeconds_ >= requiredCaughtSeconds_) {
            state_ = ChaseState::Caught;
            return ChaseResult::Caught;
        }

        return ChaseResult::Running;
    } else {
        caughtTimerSeconds_ = 0.0f;
    }

    if (distance >= escapeDistance_ || (!context.lineOfSight && distance >= hiddenEscapeDistance_)) {
        escapeTimerSeconds_ += dt;
    } else {
        escapeTimerSeconds_ = 0.0f;
    }

    if (escapeTimerSeconds_ >= requiredEscapeSeconds_) {
        state_ = ChaseState::Escaped;
        return ChaseResult::Escaped;
    }

    return ChaseResult::Running;
}

ChaseState ChaseSystem::state() const {
    return state_;
}

Vec3 ChaseSystem::pursuerPosition() const {
    return pursuerPosition_;
}

float ChaseSystem::escapeProgress() const {
    return std::clamp(escapeTimerSeconds_ / requiredEscapeSeconds_, 0.0f, 1.0f);
}

float ChaseSystem::caughtProgress() const {
    return std::clamp(caughtTimerSeconds_ / requiredCaughtSeconds_, 0.0f, 1.0f);
}

float ChaseSystem::pressure() const {
    if (state_ != ChaseState::Running) {
        return 0.0f;
    }

    return pressure_;
}

float ChaseSystem::pursuerFollowSpeed(float distanceToPlayer, float playerSpeedRatio) const {
    const float distancePressure = std::clamp(distanceToPlayer / escapeDistance_, 0.0f, 1.6f);
    const float playerBonus = std::clamp(playerSpeedRatio, 0.0f, 1.0f) * 1.35f;
    const float rubberbandBonus = std::max(0.0f, distancePressure - 0.45f) * 4.0f;
    return std::clamp(6.1f + playerBonus + rubberbandBonus, 4.8f, 12.4f);
}

void ChaseSystem::setDifficulty(float factor) {
    const float clamped = std::clamp(factor, 1.0f, 2.0f);
    requiredEscapeSeconds_ = baseRequiredEscapeSeconds_ * clamped;
    escapeDistance_ = baseEscapeDistance_ * clamped;
    failDistance_ = baseFailDistance_ * clamped;
}

float ChaseSystem::requiredEscapeSeconds() const {
    return requiredEscapeSeconds_;
}

float ChaseSystem::escapeDistance() const {
    return escapeDistance_;
}

float ChaseSystem::failDistance() const {
    return failDistance_;
}

} // namespace bs3d
