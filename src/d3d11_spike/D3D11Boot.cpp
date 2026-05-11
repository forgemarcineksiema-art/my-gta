#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#if __has_include(<wrl/client.h>)
#include <wrl/client.h>
#define BS3D_D3D11_BOOT_HAS_WRL 1
#else
#define BS3D_D3D11_BOOT_HAS_WRL 0
#endif

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

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

struct BootOptions {
    int frames = 120;
};

struct D3D11State {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapChain;
    ComPtr<ID3D11RenderTargetView> renderTargetView;
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11Buffer> vertexBuffer;
    D3D11_VIEWPORT viewport{};
};

struct TriangleVertex {
    float position[3];
    float color[4];
};

std::string formatHresult(HRESULT hr) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
        << static_cast<unsigned long>(hr);
    return out.str();
}

std::string formatWin32Error(DWORD error) {
    return std::to_string(static_cast<unsigned long>(error));
}

ComPtr<ID3DBlob> compileShader(const char* source, const char* entryPoint, const char* target) {
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if !defined(NDEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;
    const HRESULT hr = D3DCompile(source,
                                  std::strlen(source),
                                  "bs3d_d3d11_boot_triangle",
                                  nullptr,
                                  nullptr,
                                  entryPoint,
                                  target,
                                  compileFlags,
                                  0,
                                  shaderBlob.GetAddressOf(),
                                  errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        std::string errorText = "no compiler diagnostics";
        if (errorBlob.Get() != nullptr && errorBlob->GetBufferPointer() != nullptr) {
            errorText.assign(static_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
        }
        throw std::runtime_error(std::string("D3DCompile failed for ") + entryPoint + " (" + target +
                                 ") with HRESULT " + formatHresult(hr) + ": " + errorText);
    }

    return shaderBlob;
}

int parseNonNegativeInt(const std::string& value, const std::string& optionName) {
    std::size_t parsed = 0;
    int result = 0;
    try {
        result = std::stoi(value, &parsed);
    } catch (const std::exception&) {
        throw std::runtime_error(optionName + " requires a non-negative integer");
    }
    if (parsed != value.size() || result < 0) {
        throw std::runtime_error(optionName + " requires a non-negative integer");
    }
    return result;
}

std::string requireValue(int& index, int argc, char** argv, const std::string& optionName) {
    if (index + 1 >= argc) {
        throw std::runtime_error(optionName + " requires a value");
    }
    return argv[++index];
}

BootOptions parseOptions(int argc, char** argv) {
    BootOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--frames") {
            options.frames = parseNonNegativeInt(requireValue(index, argc, argv, arg), arg);
        } else if (arg == "--help") {
            std::cout << "Usage: bs3d_d3d11_boot [--frames <count>]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("unknown option: " + arg);
        }
    }
    return options;
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
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

HWND createBootWindow(HINSTANCE instance, int width, int height) {
    const wchar_t* className = L"BS3DD3D11BootSpikeWindow";

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.lpszClassName = className;

    if (RegisterClassExW(&windowClass) == 0) {
        throw std::runtime_error("RegisterClassExW failed with Win32 error " + formatWin32Error(GetLastError()));
    }

    RECT rect{0, 0, width, height};
    if (AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE) == FALSE) {
        throw std::runtime_error("AdjustWindowRect failed with Win32 error " + formatWin32Error(GetLastError()));
    }

    const int windowWidth = rect.right - rect.left;
    const int windowHeight = rect.bottom - rect.top;
    HWND window = CreateWindowExW(0,
                                  className,
                                  L"Blok 13 D3D11 Boot Spike",
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  windowWidth,
                                  windowHeight,
                                  nullptr,
                                  nullptr,
                                  instance,
                                  nullptr);
    if (window == nullptr) {
        throw std::runtime_error("CreateWindowExW failed with Win32 error " + formatWin32Error(GetLastError()));
    }

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    std::cout << "window created\n";
    return window;
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

    return state;
}

void createTrianglePipeline(D3D11State& state) {
    const char* shaderSource = R"(
struct VSInput {
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput {
    float4 position : SV_Position;
    float4 color : COLOR;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_Target {
    return input.color;
}
)";

    ComPtr<ID3DBlob> vertexShaderBlob = compileShader(shaderSource, "VSMain", "vs_5_0");
    ComPtr<ID3DBlob> pixelShaderBlob = compileShader(shaderSource, "PSMain", "ps_5_0");

    HRESULT hr = state.device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
                                                  vertexShaderBlob->GetBufferSize(),
                                                  nullptr,
                                                  state.vertexShader.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreateVertexShader failed with HRESULT " + formatHresult(hr));
    }

    hr = state.device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
                                         pixelShaderBlob->GetBufferSize(),
                                         nullptr,
                                         state.pixelShader.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreatePixelShader failed with HRESULT " + formatHresult(hr));
    }

    const D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = state.device->CreateInputLayout(inputElements,
                                         static_cast<UINT>(std::size(inputElements)),
                                         vertexShaderBlob->GetBufferPointer(),
                                         vertexShaderBlob->GetBufferSize(),
                                         state.inputLayout.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreateInputLayout failed with HRESULT " + formatHresult(hr));
    }

    const TriangleVertex vertices[] = {
        {{0.0f, 0.65f, 0.0f}, {1.0f, 0.18f, 0.12f, 1.0f}},
        {{0.65f, -0.55f, 0.0f}, {0.12f, 0.85f, 0.22f, 1.0f}},
        {{-0.65f, -0.55f, 0.0f}, {0.16f, 0.45f, 1.0f, 1.0f}},
    };

    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(vertices));
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData{};
    vertexData.pSysMem = vertices;

    hr = state.device->CreateBuffer(&vertexBufferDesc, &vertexData, state.vertexBuffer.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreateBuffer failed for triangle vertex buffer with HRESULT " +
                                 formatHresult(hr));
    }

    std::cout << "triangle pipeline created\n";
}

int runBootSpike(const BootOptions& options) {
    constexpr int width = 1280;
    constexpr int height = 720;

    HINSTANCE instance = GetModuleHandleW(nullptr);
    HWND window = createBootWindow(instance, width, height);
    D3D11State d3d11 = createD3D11State(window, width, height);
    createTrianglePipeline(d3d11);

    int renderedFrames = 0;
    bool running = true;
    MSG message{};
    while (running && renderedFrames < options.frames) {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != FALSE) {
            if (message.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        if (!running) {
            break;
        }

        if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) {
            DestroyWindow(window);
            running = false;
            break;
        }

        const float clearColor[] = {0.06f, 0.16f, 0.42f, 1.0f};
        ID3D11RenderTargetView* renderTargetView = d3d11.renderTargetView.Get();
        d3d11.context->OMSetRenderTargets(1, &renderTargetView, nullptr);
        d3d11.context->ClearRenderTargetView(d3d11.renderTargetView.Get(), clearColor);
        d3d11.context->RSSetViewports(1, &d3d11.viewport);

        ID3D11Buffer* vertexBuffers[] = {d3d11.vertexBuffer.Get()};
        const UINT strides[] = {sizeof(TriangleVertex)};
        const UINT offsets[] = {0};
        d3d11.context->IASetInputLayout(d3d11.inputLayout.Get());
        d3d11.context->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
        d3d11.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        d3d11.context->VSSetShader(d3d11.vertexShader.Get(), nullptr, 0);
        d3d11.context->PSSetShader(d3d11.pixelShader.Get(), nullptr, 0);
        d3d11.context->Draw(3, 0);

        const HRESULT hr = d3d11.swapChain->Present(1, 0);
        if (FAILED(hr)) {
            throw std::runtime_error("IDXGISwapChain::Present failed with HRESULT " + formatHresult(hr));
        }

        ++renderedFrames;
    }

    std::cout << "rendered " << renderedFrames << " frames\n";
    if (IsWindow(window) != FALSE) {
        DestroyWindow(window);
    }
    std::cout << "shutdown clean\n";
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        const BootOptions options = parseOptions(argc, argv);
        return runBootSpike(options);
    } catch (const std::exception& ex) {
        std::cerr << "D3D11 boot spike failed: " << ex.what() << '\n';
        return 1;
    }
}
