#pragma once

#include "bs3d/backend/BackendKinds.h"
#include "bs3d/render/IRenderer.h"

#include <memory>
#include <string>

namespace bs3d {

/// Configuration for createRenderer().
struct RendererFactoryRequest {
    RendererBackendKind backend = RendererBackendKind::Raylib;
    bool useNullRenderer = false;
    bool allowExperimentalD3D11Renderer = false;
};

/// Result of createRenderer().
struct RendererFactoryResult {
    std::unique_ptr<IRenderer> renderer;
    std::string error;

    bool ok() const { return renderer != nullptr && error.empty(); }
};

/// Backend-neutral renderer factory skeleton.
///
/// This is a future seam — it is not wired into the runtime yet.
/// Currently only the NullRenderer path is functional.
/// The D3D11 path is experimental — it requires explicit opt-in via
/// allowExperimentalD3D11Renderer and returns an uninitialized D3D11Renderer
/// for test/tool use only.  It does not create a device/window/swapchain.
RendererFactoryResult createRenderer(const RendererFactoryRequest& request);

} // namespace bs3d
