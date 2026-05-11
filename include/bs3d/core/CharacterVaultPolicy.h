#pragma once

#include "bs3d/core/Types.h"
#include "bs3d/core/WorldCollision.h"

namespace bs3d {

struct CharacterVaultRequest {
    Vec3 position{};
    Vec3 forward{0.0f, 0.0f, 1.0f};
    float reach = 0.78f;
    float landingDistance = 0.75f;
    float characterRadius = 0.45f;
    float skinWidth = 0.05f;
    float stepHeight = 0.36f;
    float minObstacleHeight = 0.18f;
    float maxObstacleHeight = 0.68f;
    float walkableSlopeDegrees = 38.0f;
};

struct CharacterVaultResult {
    bool available = false;
    float obstacleHeight = 0.0f;
    Vec3 takeoffImpulse{};
};

CharacterVaultResult resolveCharacterLowVault(const WorldCollision& collision,
                                              const CharacterVaultRequest& request);

} // namespace bs3d
