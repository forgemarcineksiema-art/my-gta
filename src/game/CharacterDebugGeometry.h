#pragma once

#include "bs3d/core/Types.h"

#include <vector>

namespace bs3d {

struct CharacterCapsuleDebugGeometry {
    Vec3 baseCenter{};
    Vec3 topCenter{};
    float radius = 0.0f;
    std::vector<float> ringHeights;
    int verticalSegments = 0;
};

CharacterCapsuleDebugGeometry buildCharacterCapsuleDebugGeometry(Vec3 baseCenter,
                                                                 float capsuleRadius,
                                                                 float capsuleHeight);

} // namespace bs3d
