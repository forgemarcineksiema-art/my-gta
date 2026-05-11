#include "bs3d/core/GameFeedback.h"

#include <algorithm>

namespace bs3d {

float resolveCameraWorldTension(float chasePressure, float /*heatPressure*/) {
    return std::clamp(chasePressure, 0.0f, 1.0f);
}

CameraFeedbackOutput resolveCameraFeedback(const CameraFeedbackInput& input) {
    CameraFeedbackOutput output;
    const float worldTension = std::clamp(input.worldTension, 0.0f, 1.0f);

    if (input.stableCameraMode) {
        return output;
    }

    if (input.playerInVehicle) {
        output.vehicleShake = std::clamp(input.vehicleInstability, 0.0f, 1.0f) * 0.05f +
                              (input.vehicleBoostActive ? 0.08f : 0.0f) +
                              std::clamp(input.hornPulse, 0.0f, 1.0f) * 0.04f;
    }
    output.screenShake = std::clamp(input.screenShake, 0.0f, 1.0f) + output.vehicleShake;
    output.comedyZoom = std::clamp(input.comedyZoom, 0.0f, 1.0f);
    output.cameraTension = worldTension;
    return output;
}

void GameFeedback::trigger(FeedbackEvent event) {
    switch (event) {
    case FeedbackEvent::MarkerReached:
        startPulse(0.35f);
        break;
    case FeedbackEvent::ChaseWarning:
        startShake(0.25f);
        startPulse(0.45f);
        break;
    case FeedbackEvent::MissionFailed:
        startShake(0.7f);
        startPulse(0.8f);
        startFlash(0.75f);
        break;
    case FeedbackEvent::MissionComplete:
        startPulse(0.9f);
        startFlash(0.8f);
        break;
    case FeedbackEvent::PrzypalNotice:
        startPulse(0.35f);
        break;
    }
}

bool GameFeedback::triggerComedyEvent(float intensity) {
    if (comedyCooldown_ > 0.0f) {
        return false;
    }

    comedyIntensity_ = std::clamp(intensity, 0.0f, 1.0f);
    comedyTimer_ = 0.42f;
    comedyDuration_ = 0.42f;
    comedyCooldown_ = 1.15f;
    startShake(0.18f + comedyIntensity_ * 0.16f);
    startPulse(0.22f);
    return true;
}

void GameFeedback::update(float deltaSeconds) {
    const float dt = std::max(0.0f, deltaSeconds);
    shakeTimer_ = std::max(0.0f, shakeTimer_ - dt);
    pulseTimer_ = std::max(0.0f, pulseTimer_ - dt);
    flashTimer_ = std::max(0.0f, flashTimer_ - dt);
    comedyTimer_ = std::max(0.0f, comedyTimer_ - dt);
    comedyCooldown_ = std::max(0.0f, comedyCooldown_ - dt);
    if (comedyTimer_ <= 0.0f) {
        comedyIntensity_ = 0.0f;
    }
    worldTension_ = std::max(0.0f, worldTension_ - dt * 0.35f);
}

void GameFeedback::setWorldTension(float intensity) {
    worldTension_ = std::clamp(intensity, 0.0f, 1.0f);
}

float GameFeedback::worldTension() const {
    return worldTension_;
}

void GameFeedback::setChaseIntensity(float intensity) {
    setWorldTension(intensity);
}

float GameFeedback::screenShake() const {
    return normalizedTimer(shakeTimer_, shakeDuration_);
}

float GameFeedback::hudPulse() const {
    return normalizedTimer(pulseTimer_, pulseDuration_);
}

float GameFeedback::flashAlpha() const {
    return normalizedTimer(flashTimer_, flashDuration_);
}

float GameFeedback::chaseIntensity() const {
    return worldTension();
}

float GameFeedback::comedyZoom() const {
    return normalizedTimer(comedyTimer_, comedyDuration_) * comedyIntensity_;
}

float GameFeedback::comedyFreeze() const {
    const float t = normalizedTimer(comedyTimer_, comedyDuration_);
    return t > 0.72f ? t * comedyIntensity_ : 0.0f;
}

void GameFeedback::startShake(float duration) {
    if (duration >= shakeTimer_) {
        shakeTimer_ = duration;
        shakeDuration_ = duration;
    }
}

void GameFeedback::startPulse(float duration) {
    if (duration >= pulseTimer_) {
        pulseTimer_ = duration;
        pulseDuration_ = duration;
    }
}

void GameFeedback::startFlash(float duration) {
    if (duration >= flashTimer_) {
        flashTimer_ = duration;
        flashDuration_ = duration;
    }
}

float GameFeedback::normalizedTimer(float timer, float duration) {
    if (duration <= 0.0f) {
        return 0.0f;
    }

    return std::clamp(timer / duration, 0.0f, 1.0f);
}

} // namespace bs3d
