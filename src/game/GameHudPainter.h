#pragma once

#include "WorldArtTypes.h"

#include <string>

namespace bs3d {

using HudColor = WorldObjectTint;

struct HudPoint {
    float x = 0.0f;
    float y = 0.0f;
};

class HudDrawSink {
public:
    virtual ~HudDrawSink() = default;
    virtual void fillRect(int x, int y, int width, int height, HudColor color) = 0;
    virtual void strokeRect(int x, int y, int width, int height, HudColor color) = 0;
    virtual void text(const std::string& value, int x, int y, int fontSize, HudColor color) = 0;
    virtual void circle(HudPoint center, float radius, HudColor color) = 0;
    virtual void circleLines(int x, int y, float radius, HudColor color) = 0;
    virtual void triangle(HudPoint a, HudPoint b, HudPoint c, HudColor color) = 0;
};

HudColor hudColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);
HudColor hudFade(HudColor color, float alpha);

} // namespace bs3d
