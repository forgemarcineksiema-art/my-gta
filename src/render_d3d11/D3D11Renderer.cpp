#include "D3D11Renderer.h"

namespace bs3d {

void D3D11Renderer::renderFrame(const RenderFrame& frame) {
    ++renderCalls_;
    lastStats_ = summarizeRenderFrame(frame);
    lastValidation_ = validateRenderFrame(frame);
}

} // namespace bs3d
