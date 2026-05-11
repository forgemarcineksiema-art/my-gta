#include "CharacterPosePreview.h"

#include <algorithm>

namespace bs3d {

namespace {

const std::vector<CharacterPoseKind>& poseCatalog() {
    static const std::vector<CharacterPoseKind> catalog = characterPoseCatalog();
    return catalog;
}

void setLocomotionPreview(PlayerPresentationState& state, float amount, float swing) {
    state.locomotionAmount = amount;
    state.leftLegSwing = swing;
    state.rightLegSwing = -swing;
    state.leftArmSwing = -swing * 0.65f;
    state.rightArmSwing = swing * 0.65f;
    state.bobHeight = amount * 0.045f;
}

} // namespace

std::string characterPoseLabel(CharacterPoseKind poseKind) {
    switch (poseKind) {
    case CharacterPoseKind::Idle:
        return "Idle";
    case CharacterPoseKind::Walk:
        return "Walk";
    case CharacterPoseKind::Jog:
        return "Jog";
    case CharacterPoseKind::Sprint:
        return "Sprint";
    case CharacterPoseKind::StartMove:
        return "StartMove";
    case CharacterPoseKind::StopMove:
        return "StopMove";
    case CharacterPoseKind::QuickTurn:
        return "QuickTurn";
    case CharacterPoseKind::PanicSprint:
        return "PanicSprint";
    case CharacterPoseKind::Jump:
        return "Jump";
    case CharacterPoseKind::Land:
        return "Land";
    case CharacterPoseKind::Stagger:
        return "Stagger";
    case CharacterPoseKind::Talk:
        return "Talk";
    case CharacterPoseKind::Interact:
        return "Interact";
    case CharacterPoseKind::EnterVehicle:
        return "EnterVehicle";
    case CharacterPoseKind::ExitVehicle:
        return "ExitVehicle";
    case CharacterPoseKind::SitVehicle:
        return "SitVehicle";
    case CharacterPoseKind::SteerLeft:
        return "SteerLeft";
    case CharacterPoseKind::SteerRight:
        return "SteerRight";
    case CharacterPoseKind::StandVehicle:
        return "StandVehicle";
    case CharacterPoseKind::JumpOffVehicle:
        return "JumpOffVehicle";
    case CharacterPoseKind::LowVault:
        return "LowVault";
    case CharacterPoseKind::CarryObject:
        return "CarryObject";
    case CharacterPoseKind::PushObject:
        return "PushObject";
    case CharacterPoseKind::Punch:
        return "Punch";
    case CharacterPoseKind::Dodge:
        return "Dodge";
    case CharacterPoseKind::Fall:
        return "Fall";
    case CharacterPoseKind::GetUp:
        return "GetUp";
    case CharacterPoseKind::Drink:
        return "Drink";
    case CharacterPoseKind::Smoke:
        return "Smoke";
    }
    return "Unknown";
}

bool CharacterPosePreview::active() const {
    return active_;
}

int CharacterPosePreview::activeIndex() const {
    return active_ ? activeIndex_ : -1;
}

int CharacterPosePreview::count() const {
    return static_cast<int>(poseCatalog().size());
}

CharacterPoseKind CharacterPosePreview::activePose() const {
    if (poseCatalog().empty()) {
        return CharacterPoseKind::Idle;
    }
    const int clampedIndex = std::clamp(activeIndex_, 0, count() - 1);
    return poseCatalog()[static_cast<std::size_t>(clampedIndex)];
}

std::string CharacterPosePreview::activeLabel() const {
    return characterPoseLabel(activePose());
}

void CharacterPosePreview::toggle() {
    active_ = !active_;
    if (active_) {
        activeIndex_ = std::clamp(activeIndex_, 0, std::max(0, count() - 1));
    }
}

void CharacterPosePreview::next() {
    if (count() <= 0) {
        activeIndex_ = 0;
        return;
    }
    activeIndex_ = (activeIndex_ + 1) % count();
}

void CharacterPosePreview::previous() {
    if (count() <= 0) {
        activeIndex_ = 0;
        return;
    }
    activeIndex_ = (activeIndex_ + count() - 1) % count();
}

PlayerPresentationState CharacterPosePreview::activePreviewState() const {
    return previewStateFor(activePose());
}

PlayerPresentationState CharacterPosePreview::previewStateFor(CharacterPoseKind poseKind) const {
    PlayerPresentationState state;
    state.poseKind = poseKind;

    switch (poseKind) {
    case CharacterPoseKind::Walk:
        setLocomotionPreview(state, 0.34f, 0.65f);
        break;
    case CharacterPoseKind::Jog:
        setLocomotionPreview(state, 0.58f, 0.85f);
        break;
    case CharacterPoseKind::Sprint:
        setLocomotionPreview(state, 0.95f, 1.0f);
        break;
    case CharacterPoseKind::PanicSprint:
        setLocomotionPreview(state, 1.0f, 1.0f);
        state.panicFlail = 1.0f;
        break;
    case CharacterPoseKind::StartMove:
        state.startMoveAmount = 1.0f;
        setLocomotionPreview(state, 0.42f, 0.45f);
        break;
    case CharacterPoseKind::StopMove:
        state.stopMoveAmount = 1.0f;
        setLocomotionPreview(state, 0.18f, 0.30f);
        break;
    case CharacterPoseKind::QuickTurn:
        state.quickTurnAmount = 1.0f;
        setLocomotionPreview(state, 0.55f, 0.45f);
        break;
    case CharacterPoseKind::Jump:
        state.jumpStretch = 1.0f;
        break;
    case CharacterPoseKind::Land:
        state.landingDip = 0.10f;
        break;
    case CharacterPoseKind::Stagger:
        state.staggerAmount = 0.8f;
        break;
    case CharacterPoseKind::Talk:
        state.talkGesture = 1.0f;
        break;
    case CharacterPoseKind::Interact:
        state.interactReach = 1.0f;
        break;
    case CharacterPoseKind::EnterVehicle:
    case CharacterPoseKind::ExitVehicle:
        state.actionAmount = 1.0f;
        break;
    case CharacterPoseKind::SitVehicle:
        state.vehicleSitAmount = 1.0f;
        break;
    case CharacterPoseKind::SteerLeft:
        state.vehicleSitAmount = 1.0f;
        state.steerAmount = -1.0f;
        break;
    case CharacterPoseKind::SteerRight:
        state.vehicleSitAmount = 1.0f;
        state.steerAmount = 1.0f;
        break;
    case CharacterPoseKind::StandVehicle:
        state.balanceAmount = 1.0f;
        break;
    case CharacterPoseKind::JumpOffVehicle:
        state.jumpOffVehicleAmount = 1.0f;
        state.jumpStretch = 0.7f;
        break;
    case CharacterPoseKind::LowVault:
        state.lowVaultAmount = 1.0f;
        break;
    case CharacterPoseKind::CarryObject:
        state.carryAmount = 1.0f;
        break;
    case CharacterPoseKind::PushObject:
        state.pushAmount = 1.0f;
        break;
    case CharacterPoseKind::Punch:
        state.punchAmount = 1.0f;
        state.actionAmount = 1.0f;
        break;
    case CharacterPoseKind::Dodge:
        state.dodgeAmount = 1.0f;
        break;
    case CharacterPoseKind::Fall:
        state.fallAmount = 1.0f;
        break;
    case CharacterPoseKind::GetUp:
        state.getUpAmount = 1.0f;
        break;
    case CharacterPoseKind::Drink:
        state.drinkAmount = 1.0f;
        break;
    case CharacterPoseKind::Smoke:
        state.smokeAmount = 1.0f;
        break;
    case CharacterPoseKind::Idle:
        break;
    }

    return state;
}

} // namespace bs3d
