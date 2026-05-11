#include "bs3d/core/CharacterAction.h"

namespace bs3d {

namespace {

CharacterActionRequest action(CharacterPoseKind poseKind, float durationSeconds) {
    CharacterActionRequest request;
    request.available = true;
    request.poseKind = poseKind;
    request.durationSeconds = durationSeconds;
    return request;
}

CharacterActionRequest fallWithGetUp() {
    CharacterActionRequest request = action(CharacterPoseKind::Fall, 0.62f);
    request.followUpAvailable = true;
    request.followUpPoseKind = CharacterPoseKind::GetUp;
    request.followUpDelaySeconds = request.durationSeconds;
    request.followUpDurationSeconds = 0.54f;
    return request;
}

} // namespace

CharacterActionRequest resolveCharacterAction(const InputState& input) {
    if (input.fallActionPressed) {
        return fallWithGetUp();
    }
    if (input.lowVaultActionPressed) {
        return action(CharacterPoseKind::LowVault, 0.46f);
    }
    if (input.secondaryActionPressed) {
        return action(CharacterPoseKind::Dodge, 0.34f);
    }
    if (input.primaryActionPressed) {
        return action(CharacterPoseKind::Punch, 0.34f);
    }
    if (input.pushActionPressed) {
        return action(CharacterPoseKind::PushObject, 0.48f);
    }
    if (input.carryActionPressed) {
        return action(CharacterPoseKind::CarryObject, 0.58f);
    }
    if (input.drinkActionPressed) {
        return action(CharacterPoseKind::Drink, 0.74f);
    }
    if (input.smokeActionPressed) {
        return action(CharacterPoseKind::Smoke, 0.74f);
    }
    return {};
}

bool canStartCharacterAction(const CharacterActionRequest& request, const CharacterActionStartContext& context) {
    if (!request.available) {
        return false;
    }
    if (request.poseKind == CharacterPoseKind::LowVault) {
        return context.lowVaultAvailable;
    }
    if (request.poseKind == CharacterPoseKind::PushObject) {
        return context.pushTargetAvailable;
    }
    return true;
}

} // namespace bs3d
