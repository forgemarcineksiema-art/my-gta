#pragma once

#include "bs3d/core/PlayerController.h"
#include "bs3d/core/PlayerPresentation.h"
#include "bs3d/core/Types.h"

namespace bs3d {

enum class CharacterImpactSource {
    Wall,
    Vehicle,
    Npc,
    Prop,
    Platform
};

enum class CharacterReactionKind {
    None,
    Flinch,
    Stagger,
    FallLite
};

struct CharacterImpactEvent {
    CharacterImpactSource source = CharacterImpactSource::Wall;
    float impactSpeed = 0.0f;
    Vec3 impactNormal{};
    bool grounded = true;
};

struct CharacterPhysicalReaction {
    CharacterReactionKind kind = CharacterReactionKind::None;
    Vec3 impulse{};
    float staggerSeconds = 0.0f;
    float intensity = 0.0f;
    CharacterPoseKind poseKind = CharacterPoseKind::Idle;
    float poseDurationSeconds = 0.0f;
    bool followUpAvailable = false;
    CharacterPoseKind followUpPoseKind = CharacterPoseKind::Idle;
    float followUpDelaySeconds = 0.0f;
    float followUpDurationSeconds = 0.0f;
};

struct CharacterReactionApplicationResult {
    bool applied = false;
    float feedbackIntensity = 0.0f;
    bool followUpAvailable = false;
    CharacterPoseKind followUpPoseKind = CharacterPoseKind::Idle;
    float followUpDelaySeconds = 0.0f;
    float followUpDurationSeconds = 0.0f;
};

CharacterPhysicalReaction resolveCharacterPhysicalReaction(const CharacterImpactEvent& event);
CharacterReactionApplicationResult applyCharacterPhysicalReaction(PlayerController& player,
                                                                  PlayerPresentation& presentation,
                                                                  const CharacterPhysicalReaction& reaction);

} // namespace bs3d
