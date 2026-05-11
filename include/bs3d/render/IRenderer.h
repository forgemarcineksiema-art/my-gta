#pragma once

#include "bs3d/render/RenderFrame.h"

namespace bs3d {

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual const char* backendName() const = 0;
    virtual void renderFrame(const RenderFrame& frame) = 0;
};

} // namespace bs3d
