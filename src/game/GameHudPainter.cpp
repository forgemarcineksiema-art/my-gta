#include "GameHudPainter.h"

#include <algorithm>

namespace bs3d {

HudColor hudColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return {r, g, b, a};
}

HudColor hudFade(HudColor color, float alpha) {
    color.a = static_cast<unsigned char>(255.0f * std::clamp(alpha, 0.0f, 1.0f));
    return color;
}

} // namespace bs3d
