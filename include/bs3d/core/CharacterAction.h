#pragma once

#include "bs3d/core/PlayerPresentation.h"
#include "bs3d/core/Types.h"

namespace bs3d {

struct CharacterActionRequest {
    bool available = false;
    CharacterPoseKind poseKind = CharacterPoseKind::Idle;
    float durationSeconds = 0.0f;
    bool followUpAvailable = false;
    CharacterPoseKind followUpPoseKind = CharacterPoseKind::Idle;
    float followUpDelaySeconds = 0.0f;
    float followUpDurationSeconds = 0.0f;
};

struct CharacterActionStartContext {
    bool lowVaultAvailable = false;
    bool pushTargetAvailable = false;
};

CharacterActionRequest resolveCharacterAction(const InputState& input);
bool canStartCharacterAction(const CharacterActionRequest& request, const CharacterActionStartContext& context);

} // namespace bs3d
