#pragma once

#include <vector>

namespace bs3d {

struct WorldObject;

struct WorldRenderList {
    std::vector<const WorldObject*> opaque;
    std::vector<const WorldObject*> translucent;
    std::vector<const WorldObject*> glass;
    std::vector<const WorldObject*> transparent;
    int culled = 0;
};

} // namespace bs3d
