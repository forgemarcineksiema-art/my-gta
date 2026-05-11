#pragma once

#include "bs3d/core/Types.h"

#include <cstdint>

namespace bs3d {

struct RenderColor {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

struct RenderCamera {
    Vec3 position{};
    Vec3 target{};
    Vec3 up{0.0f, 1.0f, 0.0f};
    float fovy = 45.0f;
    int projection = 0;
};

struct WorldPresentationStyle {
    RenderColor skyColor{};
    RenderColor groundColor{};
    RenderColor horizonHazeColor{};
    float groundPlaneSize = 78.0f;
    float worldCullDistance = 140.0f;
};

} // namespace bs3d
