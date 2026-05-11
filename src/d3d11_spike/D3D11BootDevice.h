#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>

#if __has_include(<wrl/client.h>)
#include <wrl/client.h>
#define BS3D_D3D11_BOOT_HAS_WRL 1
#else
#define BS3D_D3D11_BOOT_HAS_WRL 0
#endif

#include <string>

namespace d3d11_spike {

#if BS3D_D3D11_BOOT_HAS_WRL
template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
#else
template <typename T>
class ComPtr {
public:
    ComPtr() = default;
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;
    ComPtr(ComPtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }
    ComPtr& operator=(ComPtr&& other) noexcept {
        if (this != &other) {
            Reset();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ~ComPtr() {
        Reset();
    }

    T* Get() const {
        return ptr_;
    }

    T** GetAddressOf() {
        return &ptr_;
    }

    void Reset() {
        if (ptr_ != nullptr) {
            ptr_->Release();
            ptr_ = nullptr;
        }
    }

    T* operator->() const {
        return ptr_;
    }

private:
    T* ptr_ = nullptr;
};
#endif

std::string formatHresult(HRESULT hr);
std::string formatWin32Error(DWORD error);

struct D3D11State {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapChain;
    ComPtr<ID3D11RenderTargetView> renderTargetView;
    ComPtr<ID3D11Texture2D> depthStencilTexture;
    ComPtr<ID3D11DepthStencilView> depthStencilView;
    ComPtr<ID3D11DepthStencilState> depthStencilState;
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    ComPtr<ID3D11Buffer> constantBuffer;
    D3D11_VIEWPORT viewport{};
};

D3D11State createD3D11State(HWND window, int width, int height);

} // namespace d3d11_spike
