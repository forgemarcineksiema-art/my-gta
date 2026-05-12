#pragma once

#include "D3D11MeshCache.h"
#include "bs3d/render/IRenderer.h"
#include "bs3d/render/RenderFrameValidation.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <string>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11BlendState;
struct ID3D11Buffer;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilView;
struct ID3D11InputLayout;
struct ID3D11PixelShader;
struct ID3D11RasterizerState;
struct ID3D11RenderTargetView;
struct ID3D11Texture2D;
struct ID3D11VertexShader;
struct IDXGISwapChain;

namespace bs3d {

struct CpuMeshData;

struct D3D11RendererFrameStats {
    int drawnBoxes = 0;
    int drawnMeshes = 0;
    int skippedMissingMeshes = 0;
    int skippedUnsupportedKinds = 0;
    int skippedUnsupportedBuckets = 0;
    int skippedPrimitives = 0;
    int drawnDebugLines = 0;
    int skippedDebugLines = 0;
};

struct D3D11RendererConfig {
    HWND window = nullptr;
    int width = 0;
    int height = 0;
    bool enableDebugLayer = false;
};

class D3D11Renderer final : public IRenderer {
public:
    D3D11Renderer() = default;
    D3D11Renderer(const D3D11Renderer&) = delete;
    D3D11Renderer& operator=(const D3D11Renderer&) = delete;
    ~D3D11Renderer() override;

    const char* backendName() const override { return "d3d11"; }

    bool initialize(const D3D11RendererConfig& config, std::string* error = nullptr);
    bool isInitialized() const;
    void shutdown();

    void renderFrame(const RenderFrame& frame) override;

    int renderCalls() const { return renderCalls_; }
    const RenderFrameStats& lastStats() const { return lastStats_; }
    const RenderFrameValidationResult& lastValidation() const { return lastValidation_; }
    bool lastFrameValid() const { return lastValidation_.valid; }

    const D3D11RendererFrameStats& lastD3D11Stats() const { return lastD3D11Stats_; }

    void setDrawBoxWireOverlay(bool enabled) { drawBoxWireOverlay_ = enabled; }
    bool drawBoxWireOverlayEnabled() const { return drawBoxWireOverlay_; }

    bool uploadTestMesh(MeshHandle handle, const CpuMeshData& mesh, std::string* error = nullptr);

private:
    int renderCalls_ = 0;
    RenderFrameStats lastStats_{};
    RenderFrameValidationResult lastValidation_{};
    D3D11RendererFrameStats lastD3D11Stats_{};
    int width_ = 0;
    int height_ = 0;

    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    ID3D11RenderTargetView* renderTargetView_ = nullptr;
    ID3D11Texture2D* depthStencilTexture_ = nullptr;
    ID3D11DepthStencilView* depthStencilView_ = nullptr;
    ID3D11DepthStencilState* depthStencilState_ = nullptr;
    ID3D11BlendState* opaqueBlendState_ = nullptr;
    ID3D11BlendState* alphaBlendState_ = nullptr;
    ID3D11VertexShader* vertexShader_ = nullptr;
    ID3D11PixelShader* pixelShader_ = nullptr;
    ID3D11InputLayout* inputLayout_ = nullptr;
    ID3D11Buffer* vertexBuffer_ = nullptr;
    ID3D11Buffer* indexBuffer_ = nullptr;
    ID3D11Buffer* constantBuffer_ = nullptr;
    ID3D11VertexShader* lineVertexShader_ = nullptr;
    ID3D11PixelShader* linePixelShader_ = nullptr;
    ID3D11InputLayout* lineInputLayout_ = nullptr;
    ID3D11Buffer* lineVertexBuffer_ = nullptr;

    bool drawBoxWireOverlay_ = false;
    ID3D11RasterizerState* wireframeRasterizer_ = nullptr;

    D3D11MeshCache meshCache_;
};

} // namespace bs3d
