#include "bs3d/core/PlayerController.h"

namespace bs3d {

void PlayerController::reset(Vec3 position, float yawRadians) {
    motor_.reset(position, yawRadians);
}

void PlayerController::update(const InputState& input, const WorldCollision& collision, float deltaSeconds) {
    motor_.update(PlayerInputIntent::fromInputState(input), collision, deltaSeconds);
}

void PlayerController::teleportTo(Vec3 position) {
    motor_.teleportTo(position);
}

void PlayerController::setFacingYaw(float yawRadians) {
    motor_.setFacingYaw(yawRadians);
}

void PlayerController::clearVelocity() {
    motor_.clearVelocity();
}

void PlayerController::forceGrounded() {
    motor_.forceGrounded();
}

void PlayerController::syncToVehicleSeat(Vec3 position, float yawRadians) {
    motor_.syncToVehicleSeat(position, yawRadians);
}

void PlayerController::syncAfterVehicleExit(Vec3 position, float yawRadians) {
    motor_.syncAfterVehicleExit(position, yawRadians);
}

void PlayerController::setPosition(Vec3 position) {
    teleportTo(position);
}

void PlayerController::setYaw(float yawRadians) {
    setFacingYaw(yawRadians);
}

Vec3 PlayerController::position() const {
    return motor_.state().position;
}

Vec3 PlayerController::velocity() const {
    return motor_.state().velocity;
}

float PlayerController::verticalVelocity() const {
    return motor_.state().velocity.y;
}

float PlayerController::yawRadians() const {
    return motor_.state().facingYawRadians;
}

float PlayerController::walkSpeed() const {
    return motor_.config().walkSpeed;
}

float PlayerController::jogSpeed() const {
    return motor_.config().jogSpeed;
}

float PlayerController::sprintSpeed() const {
    return motor_.config().sprintSpeed;
}

bool PlayerController::isGrounded() const {
    return motor_.state().grounded;
}

const PlayerMotorState& PlayerController::motorState() const {
    return motor_.state();
}

void PlayerController::applyImpulse(Vec3 impulse) {
    motor_.applyImpulse(impulse);
}

void PlayerController::setStatus(PlayerStatus status, bool enabled) {
    motor_.setStatus(status, enabled);
}

void PlayerController::triggerStagger(Vec3 impulse, float durationSeconds, float intensity) {
    motor_.triggerStagger(impulse, durationSeconds, intensity);
}

} // namespace bs3d
