#pragma once

#include "bs3d/core/Types.h"

namespace bs3d {

enum class ChaseState {
    Inactive,
    Running,
    Escaped,
    Caught
};

enum class ChaseResult {
    Inactive,
    Running,
    Escaped,
    Caught
};

struct ChaseUpdateContext {
    Vec3 playerPosition{};
    Vec3 playerVelocity{};
    Vec3 pursuerPosition{};
    Vec3 pursuerVelocity{};
    bool lineOfSight = true;
    bool contactRecoverActive = false;
};

class ChaseSystem {
public:
    void start(Vec3 pursuerPosition, float graceSeconds = 0.0f);
    void stop();
    ChaseResult update(Vec3 playerPosition, float deltaSeconds);
    ChaseResult update(Vec3 playerPosition, Vec3 pursuerPosition, float deltaSeconds);
    ChaseResult updateWithContext(const ChaseUpdateContext& context, float deltaSeconds);

    ChaseState state() const;
    Vec3 pursuerPosition() const;
    float escapeProgress() const;
    float caughtProgress() const;
    float pressure() const;
    float pursuerFollowSpeed(float distanceToPlayer, float playerSpeedRatio) const;
    void setDifficulty(float factor);
    float requiredEscapeSeconds() const;
    float escapeDistance() const;
    float failDistance() const;

private:
    ChaseState state_ = ChaseState::Inactive;
    Vec3 pursuerPosition_{};
    float escapeTimerSeconds_ = 0.0f;
    float caughtTimerSeconds_ = 0.0f;
    float graceTimerSeconds_ = 0.0f;
    float pressure_ = 0.0f;
    float requiredEscapeSeconds_ = 2.3f;
    float requiredCaughtSeconds_ = 1.0f;
    float escapeDistance_ = 18.0f;
    float failDistance_ = 2.4f;
    float controlledCatchRelativeSpeed_ = 2.8f;
    float hiddenEscapeDistance_ = 11.5f;
    float baseRequiredEscapeSeconds_ = 2.3f;
    float baseEscapeDistance_ = 18.0f;
    float baseFailDistance_ = 2.4f;
};

} // namespace bs3d
