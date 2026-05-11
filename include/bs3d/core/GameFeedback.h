#pragma once

namespace bs3d {

enum class FeedbackEvent {
    MarkerReached,
    ChaseWarning,
    MissionFailed,
    MissionComplete,
    PrzypalNotice
};

float resolveCameraWorldTension(float chasePressure, float heatPressure);

struct CameraFeedbackInput {
    bool stableCameraMode = true;
    bool playerInVehicle = false;
    bool chaseActive = false;
    float screenShake = 0.0f;
    float comedyZoom = 0.0f;
    float worldTension = 0.0f;
    float vehicleInstability = 0.0f;
    bool vehicleBoostActive = false;
    float hornPulse = 0.0f;
};

struct CameraFeedbackOutput {
    float screenShake = 0.0f;
    float comedyZoom = 0.0f;
    float cameraTension = 0.0f;
    float vehicleShake = 0.0f;
};

CameraFeedbackOutput resolveCameraFeedback(const CameraFeedbackInput& input);

class GameFeedback {
public:
    void trigger(FeedbackEvent event);
    bool triggerComedyEvent(float intensity);
    void update(float deltaSeconds);
    void setWorldTension(float intensity);
    float worldTension() const;

    // Transitional aliases for older game/app code. Prefer worldTension().
    void setChaseIntensity(float intensity);

    float screenShake() const;
    float hudPulse() const;
    float flashAlpha() const;
    float chaseIntensity() const;
    float comedyZoom() const;
    float comedyFreeze() const;

private:
    void startShake(float duration);
    void startPulse(float duration);
    void startFlash(float duration);

    float shakeTimer_ = 0.0f;
    float shakeDuration_ = 0.001f;
    float pulseTimer_ = 0.0f;
    float pulseDuration_ = 0.001f;
    float flashTimer_ = 0.0f;
    float flashDuration_ = 0.001f;
    float worldTension_ = 0.0f;
    float comedyTimer_ = 0.0f;
    float comedyDuration_ = 0.001f;
    float comedyCooldown_ = 0.0f;
    float comedyIntensity_ = 0.0f;

    static float normalizedTimer(float timer, float duration);
};

} // namespace bs3d
