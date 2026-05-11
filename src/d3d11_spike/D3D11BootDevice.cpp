#include "D3D11BootDevice.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace d3d11_spike {

std::string formatHresult(HRESULT hr) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
        << static_cast<unsigned long>(hr);
    return out.str();
}

std::string formatWin32Error(DWORD error) {
    return std::to_string(static_cast<unsigned long>(error));
}

D3D11State createD3D11State(HWND window, int width, int height) {
    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferDesc.Width = static_cast<UINT>(width);
    swapChainDesc.BufferDesc.Height = static_cast<UINT>(height);
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = window;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D11State state;
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if !defined(NDEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                               D3D_DRIVER_TYPE_HARDWARE,
                                               nullptr,
                                               deviceFlags,
                                               featureLevels,
                                               static_cast<UINT>(std::size(featureLevels)),
                                               D3D11_SDK_VERSION,
                                               &swapChainDesc,
                                               state.swapChain.GetAddressOf(),
                                               state.device.GetAddressOf(),
                                               &featureLevel,
                                               state.context.GetAddressOf());

#if !defined(NDEBUG)
    if (FAILED(hr) && (deviceFlags & D3D11_CREATE_DEVICE_DEBUG) != 0U) {
        std::cout << "D3D11 debug layer unavailable; retrying without debug layer\n";
        deviceFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
        state.swapChain.Reset();
        state.device.Reset();
        state.context.Reset();
        hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                           D3D_DRIVER_TYPE_HARDWARE,
                                           nullptr,
                                           deviceFlags,
                                           featureLevels,
                                           static_cast<UINT>(std::size(featureLevels)),
                                           D3D11_SDK_VERSION,
                                           &swapChainDesc,
                                           state.swapChain.GetAddressOf(),
                                           state.device.GetAddressOf(),
                                           &featureLevel,
                                           state.context.GetAddressOf());
    }
#endif

    if (FAILED(hr)) {
        throw std::runtime_error("D3D11CreateDeviceAndSwapChain failed with HRESULT " + formatHresult(hr));
    }

    std::cout << "D3D11 device created\n";
    std::cout << "swapchain created\n";

    ComPtr<ID3D11Texture2D> backBuffer;
    hr = state.swapChain->GetBuffer(0,
                                    __uuidof(ID3D11Texture2D),
                                    reinterpret_cast<void**>(backBuffer.GetAddressOf()));
    if (FAILED(hr)) {
        throw std::runtime_error("IDXGISwapChain::GetBuffer failed with HRESULT " + formatHresult(hr));
    }

    hr = state.device->CreateRenderTargetView(backBuffer.Get(), nullptr, state.renderTargetView.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreateRenderTargetView failed with HRESULT " + formatHresult(hr));
    }

    state.viewport.Width = static_cast<float>(width);
    state.viewport.Height = static_cast<float>(height);
    state.viewport.MinDepth = 0.0f;
    state.viewport.MaxDepth = 1.0f;

    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = static_cast<UINT>(width);
    depthDesc.Height = static_cast<UINT>(height);
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = state.device->CreateTexture2D(&depthDesc, nullptr, state.depthStencilTexture.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreateTexture2D failed for depth stencil with HRESULT " +
                                 formatHresult(hr));
    }

    hr = state.device->CreateDepthStencilView(state.depthStencilTexture.Get(), nullptr,
                                              state.depthStencilView.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreateDepthStencilView failed with HRESULT " + formatHresult(hr));
    }

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = FALSE;

    hr = state.device->CreateDepthStencilState(&depthStencilDesc, state.depthStencilState.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreateDepthStencilState failed with HRESULT " + formatHresult(hr));
    }

    std::cout << "depth buffer created\n";

    return state;
}

} // namespace d3d11_spike
