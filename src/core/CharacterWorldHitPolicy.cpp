#include "bs3d/core/CharacterWorldHitPolicy.h"

#include <algorithm>

namespace bs3d {

namespace {

constexpr float MinimumNpcBumpSpeed = 0.45f;

} // namespace

CharacterWorldHit resolveCharacterNpcBodyHit(const PlayerMotorState& state,
                                             int npcOwnerId,
                                             Vec3 attemptedMoveDirection) {
    if (!state.hitWallThisFrame ||
        state.hitOwnerIdThisFrame != npcOwnerId ||
        state.hitSpeedThisFrame < MinimumNpcBumpSpeed) {
        return {};
    }

    Vec3 normal = normalizeXZ(attemptedMoveDirection) * -1.0f;
    if (lengthSquaredXZ(normal) <= 0.0001f) {
        normal = forwardFromYaw(state.facingYawRadians) * -1.0f;
    }

    CharacterWorldHit hit;
    hit.available = true;
    hit.event.source = CharacterImpactSource::Npc;
    hit.event.impactSpeed = std::max(0.0f, state.hitSpeedThisFrame);
    hit.event.impactNormal = normal;
    hit.event.grounded = state.grounded;
    return hit;
}

} // namespace bs3d
