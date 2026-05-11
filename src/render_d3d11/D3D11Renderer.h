#pragma once

#include "bs3d/render/IRenderer.h"
#include "bs3d/render/RenderFrameValidation.h"

namespace bs3d {

class D3D11Renderer final : public IRenderer {
public:
    const char* backendName() const override { return "d3d11"; }

    void renderFrame(const RenderFrame& frame) override;

    int renderCalls() const { return renderCalls_; }
    const RenderFrameStats& lastStats() const { return lastStats_; }
    const RenderFrameValidationResult& lastValidation() const { return lastValidation_; }
    bool lastFrameValid() const { return lastValidation_.valid; }

private:
    int renderCalls_ = 0;
    RenderFrameStats lastStats_{};
    RenderFrameValidationResult lastValidation_{};
};

} // namespace bs3d
