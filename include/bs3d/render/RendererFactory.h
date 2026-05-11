#pragma once

#include "bs3d/backend/BackendKinds.h"
#include "bs3d/render/IRenderer.h"
#include "bs3d/render/NullRenderer.h"

#include <memory>
#include <string>

namespace bs3d {

/// Configuration for createRenderer().
struct RendererFactoryRequest {
    RendererBackendKind backend = RendererBackendKind::Raylib;
    bool useNullRenderer = false;
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
inline RendererFactoryResult createRenderer(const RendererFactoryRequest& request) {
    if (request.useNullRenderer) {
        RendererFactoryResult result;
        result.renderer = std::make_unique<NullRenderer>();
        return result;
    }

    if (request.backend == RendererBackendKind::Raylib) {
        RendererFactoryResult result;
        result.error =
            "Raylib IRenderer adapter is not implemented yet; "
            "legacy runtime still uses WorldRenderer/HudRenderer/DebugRenderer";
        return result;
    }

    RendererFactoryResult result;
    result.error = "Unknown renderer backend";
    return result;
}

} // namespace bs3d
