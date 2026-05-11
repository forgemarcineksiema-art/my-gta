#pragma once

#include "bs3d/core/CharacterPhysicalReaction.h"
#include "bs3d/core/PlayerMotor.h"

namespace bs3d {

struct CharacterWorldHit {
    bool available = false;
    CharacterImpactEvent event{};
};

CharacterWorldHit resolveCharacterNpcBodyHit(const PlayerMotorState& state,
                                             int npcOwnerId,
                                             Vec3 attemptedMoveDirection);

} // namespace bs3d
