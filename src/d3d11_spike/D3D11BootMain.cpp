#include "D3D11BootDevice.h"
#include "D3D11BootPipeline.h"
#include "D3D11BootWindow.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace d3d11_spike;

struct BootOptions {
    int frames = 120;
};

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

int runBootSpike(const BootOptions& options) {
    constexpr int width = 1280;
    constexpr int height = 720;

    HINSTANCE instance = GetModuleHandleW(nullptr);
    HWND window = createBootWindow(instance, width, height);
    D3D11State d3d11 = createD3D11State(window, width, height);
    createCubePipeline(d3d11);

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
        ID3D11DepthStencilView* depthStencilView = d3d11.depthStencilView.Get();
        d3d11.context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
        d3d11.context->ClearRenderTargetView(d3d11.renderTargetView.Get(), clearColor);
        d3d11.context->ClearDepthStencilView(d3d11.depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        d3d11.context->OMSetDepthStencilState(d3d11.depthStencilState.Get(), 0);
        d3d11.context->RSSetViewports(1, &d3d11.viewport);
        updateTransformConstants(d3d11, renderedFrames, d3d11.viewport.Width / d3d11.viewport.Height);

        ID3D11Buffer* vertexBuffers[] = {d3d11.vertexBuffer.Get()};
        const UINT strides[] = {sizeof(Vertex)};
        const UINT offsets[] = {0};
        ID3D11Buffer* constantBuffers[] = {d3d11.constantBuffer.Get()};
        d3d11.context->IASetInputLayout(d3d11.inputLayout.Get());
        d3d11.context->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
        d3d11.context->IASetIndexBuffer(d3d11.indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        d3d11.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        d3d11.context->VSSetShader(d3d11.vertexShader.Get(), nullptr, 0);
        d3d11.context->VSSetConstantBuffers(0, 1, constantBuffers);
        d3d11.context->PSSetShader(d3d11.pixelShader.Get(), nullptr, 0);
        d3d11.context->DrawIndexed(36, 0, 0);

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

} // namespace

int main(int argc, char** argv) {
    try {
        const BootOptions options = parseOptions(argc, argv);
        return runBootSpike(options);
    } catch (const std::exception& ex) {
        std::cerr << "D3D11 boot spike failed: " << ex.what() << '\n';
        return 1;
    }
}
