#pragma once

#include "bs3d/render/IRenderer.h"
#include "bs3d/render/RenderFrameValidation.h"

namespace bs3d {

/// Backend-neutral no-op renderer for testing and inspection.
///
/// Implements IRenderer but does not open a window, use a GPU, or draw
/// anything.  It records statistics and validation results from every
/// consumed RenderFrame so tests can inspect them.
class NullRenderer final : public IRenderer {
public:
    const char* backendName() const override { return "null"; }

    void renderFrame(const RenderFrame& frame) override {
        ++renderCalls_;
        lastStats_ = summarizeRenderFrame(frame);
        lastValidation_ = validateRenderFrame(frame);
    }

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
