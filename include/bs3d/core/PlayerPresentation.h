#pragma once

#include "bs3d/core/PlayerMotor.h"

#include <vector>

namespace bs3d {

enum class CharacterPoseKind {
    Idle,
    Walk,
    Jog,
    Sprint,
    StartMove,
    StopMove,
    QuickTurn,
    PanicSprint,
    Jump,
    Land,
    Stagger,
    Talk,
    Interact,
    EnterVehicle,
    ExitVehicle,
    SitVehicle,
    SteerLeft,
    SteerRight,
    StandVehicle,
    JumpOffVehicle,
    LowVault,
    CarryObject,
    PushObject,
    Punch,
    Dodge,
    Fall,
    GetUp,
    Drink,
    Smoke
};

struct PlayerPresentationContext {
    bool inVehicle = false;
    bool talkActive = false;
    bool interactActive = false;
    bool standingOnVehicle = false;
    bool carryingObject = false;
    float steerInput = 0.0f;
};

struct PlayerPresentationState {
    CharacterPoseKind poseKind = CharacterPoseKind::Idle;
    float leanRadians = 0.0f;
    float bobHeight = 0.0f;
    float sway = 0.0f;
    float landingDip = 0.0f;
    float stepPhase = 0.0f;
    float locomotionAmount = 0.0f;
    float leftLegSwing = 0.0f;
    float rightLegSwing = 0.0f;
    float leftArmSwing = 0.0f;
    float rightArmSwing = 0.0f;
    float jumpStretch = 0.0f;
    float talkGesture = 0.0f;
    float interactReach = 0.0f;
    float vehicleSitAmount = 0.0f;
    float steerAmount = 0.0f;
    float balanceAmount = 0.0f;
    float actionAmount = 0.0f;
    float oneShotNormalizedTime = 0.0f;
    float startMoveAmount = 0.0f;
    float stopMoveAmount = 0.0f;
    float quickTurnAmount = 0.0f;
    float punchAmount = 0.0f;
    float dodgeAmount = 0.0f;
    float carryAmount = 0.0f;
    float pushAmount = 0.0f;
    float drinkAmount = 0.0f;
    float smokeAmount = 0.0f;
    float fallAmount = 0.0f;
    float getUpAmount = 0.0f;
    float lowVaultAmount = 0.0f;
    float jumpOffVehicleAmount = 0.0f;
    float panicFlail = 0.0f;
    float staggerAmount = 0.0f;
    float tiredAmount = 0.0f;
    float bruisedAmount = 0.0f;
};

class PlayerPresentation {
public:
    void playOneShot(CharacterPoseKind kind, float durationSeconds);
    bool oneShotActive() const;
    void update(const PlayerMotorState& motorState,
                float deltaSeconds,
                float worldTension = 0.0f,
                const PlayerPresentationContext& context = {});
    const PlayerPresentationState& state() const;

private:
    PlayerPresentationState state_{};
    CharacterPoseKind oneShotKind_ = CharacterPoseKind::Idle;
    float oneShotSeconds_ = 0.0f;
    float oneShotDurationSeconds_ = 0.0f;
    float startMoveSeconds_ = 0.0f;
    float stopMoveSeconds_ = 0.0f;
    float quickTurnSeconds_ = 0.0f;
    float jumpOffVehicleSeconds_ = 0.0f;
    float previousXzSpeed_ = 0.0f;
    Vec3 previousMoveDirection_{};
    bool hasPreviousMoveDirection_ = false;
    bool wasOnDynamicSupport_ = false;
};

std::vector<CharacterPoseKind> characterPoseCatalog();

} // namespace bs3d
