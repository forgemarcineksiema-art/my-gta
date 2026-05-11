#pragma once

#include "bs3d/core/PlayerMotor.h"
#include "bs3d/core/Types.h"
#include "bs3d/core/WorldCollision.h"

namespace bs3d {

class PlayerController {
public:
    void reset(Vec3 position, float yawRadians = 0.0f);
    void update(const InputState& input, const WorldCollision& collision, float deltaSeconds);

    void teleportTo(Vec3 position);
    void setFacingYaw(float yawRadians);
    void clearVelocity();
    void forceGrounded();
    void syncToVehicleSeat(Vec3 position, float yawRadians);
    void syncAfterVehicleExit(Vec3 position, float yawRadians);
    void setPosition(Vec3 position);
    void setYaw(float yawRadians);
    Vec3 position() const;
    Vec3 velocity() const;
    float verticalVelocity() const;
    float yawRadians() const;
    float walkSpeed() const;
    float jogSpeed() const;
    float sprintSpeed() const;
    bool isGrounded() const;
    const PlayerMotorState& motorState() const;
    void applyImpulse(Vec3 impulse);
    void setStatus(PlayerStatus status, bool enabled);
    void triggerStagger(Vec3 impulse, float durationSeconds, float intensity);

private:
    PlayerMotor motor_{};
};

} // namespace bs3d
