#include "bs3d/render/RendererFactory.h"
#include "bs3d/render/NullRenderer.h"

#ifdef _WIN32
#include "D3D11Renderer.h"
#endif

namespace bs3d {

RendererFactoryResult createRenderer(const RendererFactoryRequest& request) {
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

    if (request.backend == RendererBackendKind::D3D11) {
#ifdef _WIN32
        if (!request.allowExperimentalD3D11Renderer) {
            RendererFactoryResult result;
            result.error =
                "D3D11Renderer factory path is experimental and not active for runtime. "
                "Set allowExperimentalD3D11Renderer to true for test/tool usage.";
            return result;
        }
        RendererFactoryResult result;
        result.renderer = std::make_unique<D3D11Renderer>();
        return result;
#else
        RendererFactoryResult result;
        result.error = "D3D11Renderer is not available on this platform";
        return result;
#endif
    }

    RendererFactoryResult result;
    result.error = "Unknown renderer backend";
    return result;
}

} // namespace bs3d
