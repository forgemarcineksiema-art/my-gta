#pragma once

#include "bs3d/core/Types.h"

namespace bs3d {

enum class ChaseAiMode {
    Patrol,
    Pursue,
    Search,
    ContactRecover
};

struct PoliceBlackboard {
    bool hasKnownTarget = false;
    Vec3 lastKnownTarget{};
    Vec3 lastKnownVelocity{};
    float timeSinceSeenSeconds = 0.0f;
};

struct ChaseAiRuntimeState {
    ChaseAiMode mode = ChaseAiMode::Pursue;
    float contactRecoverSeconds = 0.0f;
    float searchSeconds = 0.0f;
    float sideBias = 1.0f;
    PoliceBlackboard blackboard{};
};

struct ChaseAiInput {
    Vec3 pursuerPosition{};
    Vec3 targetPosition{};
    Vec3 targetVelocity{};
    float targetYawRadians = 0.0f;
    float deltaSeconds = 0.0f;
    float elapsedSeconds = 0.0f;
    bool collisionHit = false;
    bool separatedFromTarget = false;
    bool witnessedBySensor = true;
    bool lineOfSightBlocked = false;
    bool hasPatrolInterest = false;
    Vec3 patrolInterestPosition{};
    float patrolInterestRadius = 0.0f;
};

struct ChaseAiCommand {
    ChaseAiMode mode = ChaseAiMode::Pursue;
    Vec3 aimPoint{};
    float speedScale = 1.0f;
    bool contactRecoverActive = false;
    bool lineOfSight = true;
};

ChaseAiCommand updateChaseAiRuntime(ChaseAiRuntimeState& state, const ChaseAiInput& input);
const char* chaseAiModeName(ChaseAiMode mode);

} // namespace bs3d
