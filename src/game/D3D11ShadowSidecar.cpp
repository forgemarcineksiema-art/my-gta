#include "D3D11ShadowSidecar.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "D3D11Renderer.h"
#include "bs3d/render/RendererFactory.h"

#include <string>

namespace bs3d {

namespace {

constexpr int ShadowWindowWidth = 640;
constexpr int ShadowWindowHeight = 360;

std::string formatWin32Error(DWORD error) {
    return std::to_string(static_cast<unsigned long>(error));
}

LRESULT CALLBACK shadowSidecarWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    (void)wParam;
    (void)lParam;

    switch (message) {
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

HWND createShadowWindow(HINSTANCE instance) {
    const wchar_t* className = L"BS3DD3D11ShadowSidecarWindow";

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = shadowSidecarWindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.lpszClassName = className;

    if (RegisterClassExW(&windowClass) == 0) {
        return nullptr;
    }

    RECT rect{0, 0, ShadowWindowWidth, ShadowWindowHeight};
    if (AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE) == FALSE) {
        return nullptr;
    }

    HWND window = CreateWindowExW(0,
                                  className,
                                  L"Blok 13 D3D11 Shadow",
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  rect.right - rect.left,
                                  rect.bottom - rect.top,
                                  nullptr,
                                  nullptr,
                                  instance,
                                  nullptr);
    if (window != nullptr) {
        ShowWindow(window, SW_SHOW);
        UpdateWindow(window);
    }
    return window;
}

} // namespace

struct D3D11ShadowSidecar::Impl {
    HWND window = nullptr;
    D3D11Renderer* renderer = nullptr;
    std::unique_ptr<IRenderer> factoryOwned;
    std::string error;
    bool initialized = false;
};

D3D11ShadowSidecar::D3D11ShadowSidecar() : impl_(std::make_unique<Impl>()) {}

D3D11ShadowSidecar::~D3D11ShadowSidecar() {
    shutdown();
}

bool D3D11ShadowSidecar::initialize() {
    shutdown();

    HINSTANCE instance = GetModuleHandleW(nullptr);
    impl_->window = createShadowWindow(instance);
    if (impl_->window == nullptr) {
        impl_->error = "D3D11 shadow sidecar: createWindow failed with Win32 error " +
                       formatWin32Error(GetLastError());
        return false;
    }

    RendererFactoryRequest factoryRequest;
    factoryRequest.backend = RendererBackendKind::D3D11;
    factoryRequest.allowExperimentalD3D11Renderer = true;

    RendererFactoryResult factoryResult = createRenderer(factoryRequest);
    if (!factoryResult.ok()) {
        DestroyWindow(impl_->window);
        impl_->window = nullptr;
        impl_->error = "D3D11 shadow sidecar: RendererFactory failed: " + factoryResult.error;
        return false;
    }

    auto* d3d11 = dynamic_cast<D3D11Renderer*>(factoryResult.renderer.get());
    if (d3d11 == nullptr) {
        DestroyWindow(impl_->window);
        impl_->window = nullptr;
        impl_->error = "D3D11 shadow sidecar: factory returned non-D3D11Renderer";
        return false;
    }

    impl_->factoryOwned = std::move(factoryResult.renderer);
    impl_->renderer = d3d11;

    D3D11RendererConfig config;
    config.window = impl_->window;
    config.width = ShadowWindowWidth;
    config.height = ShadowWindowHeight;
#if !defined(NDEBUG)
    config.enableDebugLayer = true;
#endif

    std::string error;
    if (!impl_->renderer->initialize(config, &error)) {
        impl_->factoryOwned.reset();
        impl_->renderer = nullptr;
        DestroyWindow(impl_->window);
        impl_->window = nullptr;
        impl_->error = "D3D11 shadow sidecar: D3D11Renderer init failed: " + error;
        return false;
    }

    impl_->initialized = true;
    impl_->error.clear();
    return true;
}

void D3D11ShadowSidecar::shutdown() {
    if (impl_->renderer != nullptr) {
        impl_->renderer->shutdown();
        impl_->renderer = nullptr;
    }
    impl_->factoryOwned.reset();
    if (impl_->window != nullptr) {
        if (IsWindow(impl_->window) != FALSE) {
            DestroyWindow(impl_->window);
        }
        impl_->window = nullptr;
    }
    impl_->initialized = false;
}

bool D3D11ShadowSidecar::isInitialized() const {
    return impl_->initialized;
}

void D3D11ShadowSidecar::submit(const RenderFrame& frame) {
    if (!impl_->initialized || impl_->renderer == nullptr) {
        return;
    }
    impl_->renderer->renderFrame(frame);
    pumpMessages();
}

const char* D3D11ShadowSidecar::lastError() const {
    return impl_->error.c_str();
}

void D3D11ShadowSidecar::pumpMessages() {
    if (impl_->window == nullptr) {
        return;
    }
    MSG message{};
    while (PeekMessageW(&message, impl_->window, 0, 0, PM_REMOVE) != FALSE) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

} // namespace bs3d

#else // _WIN32

#include <string>

namespace bs3d {

struct D3D11ShadowSidecar::Impl {
    std::string error = "D3D11 shadow sidecar is Windows-only";
    bool initialized = false;
};

D3D11ShadowSidecar::D3D11ShadowSidecar() : impl_(std::make_unique<Impl>()) {}
D3D11ShadowSidecar::~D3D11ShadowSidecar() = default;

bool D3D11ShadowSidecar::initialize() { impl_->error = "D3D11 shadow sidecar is Windows-only"; return false; }
void D3D11ShadowSidecar::shutdown() {}
bool D3D11ShadowSidecar::isInitialized() const { return false; }
void D3D11ShadowSidecar::submit(const RenderFrame&) {}
const char* D3D11ShadowSidecar::lastError() const { return impl_->error.c_str(); }
void D3D11ShadowSidecar::pumpMessages() {}

} // namespace bs3d

#endif
