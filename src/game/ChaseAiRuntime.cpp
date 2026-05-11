#include "ChaseAiRuntime.h"

#include <algorithm>
#include <cmath>

namespace bs3d {

namespace {

constexpr float ContactRecoverSeconds = 0.95f;
constexpr float ContactDistance = 4.15f;
constexpr float SearchAfterLostSeconds = 1.0f;
constexpr float ForgetTargetSeconds = 5.0f;

} // namespace

ChaseAiCommand updateChaseAiRuntime(ChaseAiRuntimeState& state, const ChaseAiInput& input) {
    const float dt = std::max(0.0f, input.deltaSeconds);
    const float distanceToTarget = distanceXZ(input.pursuerPosition, input.targetPosition);
    const bool canSeeTarget = input.witnessedBySensor && !input.lineOfSightBlocked;
    if (canSeeTarget) {
        state.blackboard.hasKnownTarget = true;
        state.blackboard.lastKnownTarget = input.targetPosition;
        state.blackboard.lastKnownVelocity = input.targetVelocity;
        state.blackboard.timeSinceSeenSeconds = 0.0f;
    } else {
        state.blackboard.timeSinceSeenSeconds += dt;
    }

    if (input.collisionHit || input.separatedFromTarget || distanceToTarget <= ContactDistance) {
        state.mode = ChaseAiMode::ContactRecover;
        state.contactRecoverSeconds = std::max(state.contactRecoverSeconds, ContactRecoverSeconds);
    }

    ChaseAiCommand command;
    const Vec3 targetForward = forwardFromYaw(input.targetYawRadians);
    const Vec3 targetRight = screenRightFromYaw(input.targetYawRadians);

    if (state.mode != ChaseAiMode::ContactRecover && !canSeeTarget) {
        if (!state.blackboard.hasKnownTarget ||
            state.blackboard.timeSinceSeenSeconds > ForgetTargetSeconds) {
            state.mode = ChaseAiMode::Patrol;
            state.blackboard.hasKnownTarget = false;
        } else if (state.blackboard.timeSinceSeenSeconds >= SearchAfterLostSeconds) {
            state.mode = ChaseAiMode::Search;
        }
    }

    if (canSeeTarget && state.mode != ChaseAiMode::ContactRecover) {
        state.mode = ChaseAiMode::Pursue;
    }

    if (state.mode == ChaseAiMode::ContactRecover) {
        state.contactRecoverSeconds = std::max(0.0f, state.contactRecoverSeconds - dt);
        const Vec3 away = lengthSquaredXZ(input.pursuerPosition - input.targetPosition) > 0.0001f
                              ? normalizeXZ(input.pursuerPosition - input.targetPosition)
                              : targetForward * -1.0f;
        const float sideSign = state.sideBias >= 0.0f ? 1.0f : -1.0f;
        command.mode = ChaseAiMode::ContactRecover;
        command.aimPoint = input.pursuerPosition + away * 5.0f + targetRight * (sideSign * 1.8f);
        command.speedScale = 0.42f;
        command.contactRecoverActive = true;
        command.lineOfSight = false;
        if (state.contactRecoverSeconds <= 0.0f) {
            state.mode = ChaseAiMode::Pursue;
            state.sideBias = -state.sideBias;
        }
        return command;
    }

    if (state.mode == ChaseAiMode::Patrol) {
        const float patrolRadius = input.hasPatrolInterest
                                       ? std::clamp(input.patrolInterestRadius, 2.0f, 8.0f)
                                       : 3.0f;
        const Vec3 patrolCenter = input.hasPatrolInterest ? input.patrolInterestPosition : input.pursuerPosition;
        command.mode = ChaseAiMode::Patrol;
        command.aimPoint = patrolCenter +
                           Vec3{std::cos(input.elapsedSeconds * 0.45f) * patrolRadius,
                                0.0f,
                                std::sin(input.elapsedSeconds * 0.45f) * patrolRadius};
        command.speedScale = 0.35f;
        command.lineOfSight = false;
        return command;
    }

    if (state.mode == ChaseAiMode::Search) {
        state.searchSeconds += dt;
        const float sideSign = std::sin(input.elapsedSeconds * 1.7f) >= 0.0f ? 1.0f : -1.0f;
        command.mode = ChaseAiMode::Search;
        command.aimPoint = state.blackboard.lastKnownTarget +
                           normalizeXZ(state.blackboard.lastKnownVelocity + targetRight * sideSign) * 3.5f;
        command.speedScale = 0.58f;
        command.lineOfSight = false;
        return command;
    }

    const float sidePulse = std::sin(input.elapsedSeconds * 0.8f) >= 0.0f ? 1.0f : -1.0f;
    state.sideBias = sidePulse;
    const float targetSpeed = std::min(distanceXZ(input.targetVelocity, {}), 16.0f);
    const float rearOffset = 3.8f + targetSpeed * 0.08f;
    const float sideOffset = 1.75f * state.sideBias;

    command.mode = ChaseAiMode::Pursue;
    command.aimPoint = input.targetPosition - targetForward * rearOffset + targetRight * sideOffset;
    command.speedScale = 1.0f;
    command.contactRecoverActive = false;
    command.lineOfSight = !input.lineOfSightBlocked;
    return command;
}

const char* chaseAiModeName(ChaseAiMode mode) {
    switch (mode) {
    case ChaseAiMode::Patrol:
        return "Patrol";
    case ChaseAiMode::Pursue:
        return "Pursue";
    case ChaseAiMode::Search:
        return "Search";
    case ChaseAiMode::ContactRecover:
        return "ContactRecover";
    }
    return "Unknown";
}

} // namespace bs3d
