#include "CharacterDebugGeometry.h"

#include <algorithm>

namespace bs3d {

CharacterCapsuleDebugGeometry buildCharacterCapsuleDebugGeometry(Vec3 baseCenter,
                                                                 float capsuleRadius,
                                                                 float capsuleHeight) {
    CharacterCapsuleDebugGeometry geometry;
    geometry.baseCenter = baseCenter;
    geometry.topCenter = baseCenter + Vec3{0.0f, std::max(0.0f, capsuleHeight), 0.0f};
    geometry.radius = std::max(0.0f, capsuleRadius);
    geometry.ringHeights = {
        baseCenter.y + 0.08f,
        baseCenter.y + std::max(0.0f, capsuleHeight) * 0.52f,
        baseCenter.y + std::max(0.0f, capsuleHeight) - 0.08f,
    };
    geometry.verticalSegments = 4;
    return geometry;
}

} // namespace bs3d
