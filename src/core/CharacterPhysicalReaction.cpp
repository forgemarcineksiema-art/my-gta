#include "bs3d/core/CharacterPhysicalReaction.h"

#include <algorithm>

namespace bs3d {

namespace {

Vec3 safeImpactNormal(Vec3 normal) {
    Vec3 flat = normalizeXZ(normal);
    if (lengthSquaredXZ(flat) <= 0.0001f) {
        return {0.0f, 0.0f, -1.0f};
    }
    return flat;
}

CharacterPhysicalReaction makeStagger(CharacterReactionKind kind,
                                      Vec3 normal,
                                      float speed,
                                      float impulseScale,
                                      float minDuration,
                                      float maxDuration,
                                      float poseDuration) {
    const float amount = std::clamp(speed / 6.0f, 0.0f, 1.0f);
    CharacterPhysicalReaction reaction;
    reaction.kind = kind;
    reaction.poseKind = CharacterPoseKind::Stagger;
    reaction.poseDurationSeconds = poseDuration;
    reaction.staggerSeconds = std::clamp(minDuration + amount * (maxDuration - minDuration), minDuration, maxDuration);
    reaction.intensity = std::clamp(0.18f + amount * 0.65f, 0.0f, 1.0f);
    reaction.impulse = safeImpactNormal(normal) * std::clamp(speed * impulseScale, 0.12f, 1.8f);
    return reaction;
}

CharacterPhysicalReaction makeFallLite(Vec3 normal, float speed, bool grounded) {
    const float amount = std::clamp(speed / 9.5f, 0.0f, 1.0f);
    CharacterPhysicalReaction reaction;
    reaction.kind = CharacterReactionKind::FallLite;
    reaction.poseKind = CharacterPoseKind::Fall;
    reaction.poseDurationSeconds = 0.54f + amount * 0.12f;
    reaction.staggerSeconds = std::clamp(0.42f + amount * 0.24f, 0.42f, 0.68f);
    reaction.intensity = std::clamp(0.48f + amount * 0.52f, 0.0f, 1.0f);
    reaction.impulse = safeImpactNormal(normal) * std::clamp(speed * 0.28f, 1.1f, 3.1f);
    reaction.impulse.y = grounded ? (0.60f + amount * 0.55f) : 0.25f;
    reaction.followUpAvailable = true;
    reaction.followUpPoseKind = CharacterPoseKind::GetUp;
    reaction.followUpDelaySeconds = reaction.poseDurationSeconds;
    reaction.followUpDurationSeconds = 0.52f;
    return reaction;
}

} // namespace

CharacterPhysicalReaction resolveCharacterPhysicalReaction(const CharacterImpactEvent& event) {
    const float speed = std::max(0.0f, event.impactSpeed);
    if (speed < 0.55f) {
        return {};
    }

    switch (event.source) {
    case CharacterImpactSource::Vehicle:
        if (speed >= 5.8f) {
            return makeFallLite(event.impactNormal, speed, event.grounded);
        }
        return makeStagger(CharacterReactionKind::Stagger, event.impactNormal, speed, 0.16f, 0.18f, 0.34f, 0.34f);
    case CharacterImpactSource::Platform:
        if (speed >= 5.2f) {
            return makeFallLite(event.impactNormal, speed, event.grounded);
        }
        return makeStagger(CharacterReactionKind::Stagger, event.impactNormal, speed, 0.12f, 0.14f, 0.28f, 0.28f);
    case CharacterImpactSource::Npc:
        return makeStagger(CharacterReactionKind::Flinch, event.impactNormal, speed, 0.08f, 0.08f, 0.18f, 0.20f);
    case CharacterImpactSource::Prop:
        return makeStagger(speed > 3.2f ? CharacterReactionKind::Stagger : CharacterReactionKind::Flinch,
                           event.impactNormal,
                           speed,
                           0.10f,
                           0.10f,
                           0.26f,
                           0.24f);
    case CharacterImpactSource::Wall:
        return makeStagger(speed > 2.4f ? CharacterReactionKind::Stagger : CharacterReactionKind::Flinch,
                           event.impactNormal,
                           speed,
                           0.09f,
                           0.08f,
                           0.30f,
                           0.22f);
    }

    return {};
}

CharacterReactionApplicationResult applyCharacterPhysicalReaction(PlayerController& player,
                                                                  PlayerPresentation& presentation,
                                                                  const CharacterPhysicalReaction& reaction) {
    CharacterReactionApplicationResult result;
    if (reaction.kind == CharacterReactionKind::None) {
        return result;
    }

    player.triggerStagger(reaction.impulse, reaction.staggerSeconds, reaction.intensity);
    if (reaction.poseKind != CharacterPoseKind::Idle && reaction.poseDurationSeconds > 0.0f) {
        presentation.playOneShot(reaction.poseKind, reaction.poseDurationSeconds);
    }

    result.applied = true;
    result.feedbackIntensity = std::clamp(reaction.intensity, 0.0f, 1.0f);
    result.followUpAvailable = reaction.followUpAvailable;
    result.followUpPoseKind = reaction.followUpPoseKind;
    result.followUpDelaySeconds = reaction.followUpDelaySeconds;
    result.followUpDurationSeconds = reaction.followUpDurationSeconds;
    return result;
}

} // namespace bs3d
