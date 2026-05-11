#pragma once

#include "bs3d/render/RenderFrameValidation.h"

#include <memory>
#include <string>

namespace bs3d {

struct RenderFrame;

/// Dev-only sidecar: opens a separate Win32 window and renders shadow
/// RenderFrames through an experimental D3D11Renderer.
///
/// This does not replace raylib rendering.  It is activated by
/// --d3d11-shadow-window (implies --renderframe-shadow).
class D3D11ShadowSidecar {
public:
    D3D11ShadowSidecar();
    ~D3D11ShadowSidecar();
    D3D11ShadowSidecar(const D3D11ShadowSidecar&) = delete;
    D3D11ShadowSidecar& operator=(const D3D11ShadowSidecar&) = delete;

    bool initialize();
    void shutdown();
    bool isInitialized() const;
    bool isCloseRequested() const;
    bool hasError() const;
    void submit(const RenderFrame& frame);
    const char* lastError() const;

    int renderCalls() const;
    RenderFrameStats lastStats() const;
    bool lastFrameValid() const;

    void pumpMessages();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace bs3d
