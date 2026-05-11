#include "D3D11Renderer.h"

#include <d3d11.h>
#include <dxgi.h>

#include <iomanip>
#include <iterator>
#include <sstream>

namespace bs3d {

namespace {

std::string formatHresult(HRESULT hr) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
        << static_cast<unsigned long>(hr);
    return out.str();
}

void assignError(std::string* error, const std::string& message) {
    if (error != nullptr) {
        *error = message;
    }
}

template <typename T>
void releaseAndNull(T*& ptr) {
    if (ptr != nullptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

} // namespace

D3D11Renderer::~D3D11Renderer() {
    shutdown();
}

bool D3D11Renderer::initialize(const D3D11RendererConfig& config, std::string* error) {
    shutdown();

    if (config.window == nullptr) {
        assignError(error, "D3D11Renderer requires a valid HWND");
        return false;
    }
    if (config.width <= 0 || config.height <= 0) {
        assignError(error, "D3D11Renderer requires positive width and height");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferDesc.Width = static_cast<UINT>(config.width);
    swapChainDesc.BufferDesc.Height = static_cast<UINT>(config.height);
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = config.window;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (config.enableDebugLayer) {
        deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                               D3D_DRIVER_TYPE_HARDWARE,
                                               nullptr,
                                               deviceFlags,
                                               featureLevels,
                                               static_cast<UINT>(std::size(featureLevels)),
                                               D3D11_SDK_VERSION,
                                               &swapChainDesc,
                                               &swapChain_,
                                               &device_,
                                               &featureLevel,
                                               &context_);

    if (FAILED(hr) && config.enableDebugLayer) {
        shutdown();
        deviceFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                           D3D_DRIVER_TYPE_HARDWARE,
                                           nullptr,
                                           deviceFlags,
                                           featureLevels,
                                           static_cast<UINT>(std::size(featureLevels)),
                                           D3D11_SDK_VERSION,
                                           &swapChainDesc,
                                           &swapChain_,
                                           &device_,
                                           &featureLevel,
                                           &context_);
    }

    if (FAILED(hr)) {
        shutdown();
        assignError(error, "D3D11CreateDeviceAndSwapChain failed with HRESULT " + formatHresult(hr));
        return false;
    }

    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr)) {
        shutdown();
        assignError(error, "IDXGISwapChain::GetBuffer failed with HRESULT " + formatHresult(hr));
        return false;
    }

    hr = device_->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView_);
    backBuffer->Release();
    if (FAILED(hr)) {
        shutdown();
        assignError(error, "ID3D11Device::CreateRenderTargetView failed with HRESULT " + formatHresult(hr));
        return false;
    }

    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool D3D11Renderer::isInitialized() const {
    return device_ != nullptr && context_ != nullptr && swapChain_ != nullptr && renderTargetView_ != nullptr;
}

void D3D11Renderer::shutdown() {
    releaseAndNull(renderTargetView_);
    releaseAndNull(swapChain_);
    releaseAndNull(context_);
    releaseAndNull(device_);
}

void D3D11Renderer::renderFrame(const RenderFrame& frame) {
    ++renderCalls_;
    lastStats_ = summarizeRenderFrame(frame);
    lastValidation_ = validateRenderFrame(frame);

    if (!isInitialized()) {
        return;
    }

    const float clearColor[] = {0.05f, 0.08f, 0.16f, 1.0f};
    ID3D11RenderTargetView* renderTargetView = renderTargetView_;
    context_->OMSetRenderTargets(1, &renderTargetView, nullptr);
    context_->ClearRenderTargetView(renderTargetView_, clearColor);
    swapChain_->Present(1, 0);
}

} // namespace bs3d
