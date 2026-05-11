#include "D3D11Renderer.h"

#include "RenderExtraction.h"
#include "RenderFrameDump.h"
#include "bs3d/render/RenderFrameBuilder.h"
#include "bs3d/render/RendererFactory.h"
#include "bs3d/render/WorldRenderList.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct SmokeOptions {
    int frames = 120;
    bool drawBox = false;
    bool drawTwoBoxes = false;
    bool drawDebugLines = false;
    bool useCamera = false;
    bool useBuilderFrame = false;
    bool useExtractionFrame = false;
    bool useFactory = false;
    std::string loadFramePath;
};

LRESULT CALLBACK smokeWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
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

std::string formatWin32Error(DWORD error) {
    return std::to_string(static_cast<unsigned long>(error));
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

SmokeOptions parseOptions(int argc, char** argv) {
    SmokeOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--frames") {
            options.frames = parseNonNegativeInt(requireValue(index, argc, argv, arg), arg);
        } else if (arg == "--box") {
            options.drawBox = true;
        } else if (arg == "--two-boxes") {
            options.drawBox = true;
            options.drawTwoBoxes = true;
        } else if (arg == "--debug-lines") {
            options.drawDebugLines = true;
        } else if (arg == "--camera") {
            options.useCamera = true;
        } else if (arg == "--builder-frame") {
            options.useBuilderFrame = true;
        } else if (arg == "--extraction-frame") {
            options.useExtractionFrame = true;
        } else if (arg == "--factory") {
            options.useFactory = true;
        } else if (arg == "--load-frame") {
            options.loadFramePath = requireValue(index, argc, argv, arg);
        } else if (arg == "--help") {
            std::cout << "Usage: bs3d_d3d11_renderer_smoke [--frames <count>] [--box] [--two-boxes] [--debug-lines] [--camera] [--builder-frame] [--extraction-frame] [--factory] [--load-frame <path>]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("unknown option: " + arg);
        }
    }
    return options;
}

HWND createSmokeWindow(HINSTANCE instance, int width, int height) {
    const wchar_t* className = L"BS3DD3D11RendererSmokeWindow";

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = smokeWindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.lpszClassName = className;

    if (RegisterClassExW(&windowClass) == 0) {
        throw std::runtime_error("RegisterClassExW failed with Win32 error " +
                                 formatWin32Error(GetLastError()));
    }

    RECT rect{0, 0, width, height};
    if (AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE) == FALSE) {
        throw std::runtime_error("AdjustWindowRect failed with Win32 error " +
                                 formatWin32Error(GetLastError()));
    }

    HWND window = CreateWindowExW(0,
                                  className,
                                  L"Blok 13 D3D11Renderer Smoke",
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  rect.right - rect.left,
                                  rect.bottom - rect.top,
                                  nullptr,
                                  nullptr,
                                  instance,
                                  nullptr);
    if (window == nullptr) {
        throw std::runtime_error("CreateWindowExW failed with Win32 error " +
                                 formatWin32Error(GetLastError()));
    }

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    std::cout << "window created\n";
    return window;
}

bs3d::RenderFrame makeSmokeFrame(const SmokeOptions& options, int frameIndex) {
    if (!options.loadFramePath.empty()) {
        bs3d::RenderFrame frame;
        std::string error;
        if (!bs3d::readRenderFrameDump(options.loadFramePath, frame, &error)) {
            throw std::runtime_error("failed to load frame dump " + options.loadFramePath + ": " + error);
        }
        if (options.useCamera) {
            frame.camera.position = {0.0f, 2.0f, -5.0f};
            frame.camera.target = {0.0f, 0.0f, 0.0f};
            frame.camera.up = {0.0f, 1.0f, 0.0f};
            frame.camera.fovy = 60.0f;
        }
        return frame;
    }

    if (options.useBuilderFrame) {
        bs3d::RenderFrameBuilder builder;

        if (options.useCamera) {
            bs3d::RenderCamera camera;
            camera.position = {0.0f, 2.0f, -5.0f};
            camera.target = {0.0f, 0.0f, 0.0f};
            camera.up = {0.0f, 1.0f, 0.0f};
            camera.fovy = 60.0f;
            builder.setCamera(camera);
        }

        {
            bs3d::RenderPrimitiveCommand box;
            box.kind = bs3d::RenderPrimitiveKind::Box;
            box.bucket = bs3d::RenderBucket::Opaque;
            box.transform.position = {0.0f, 0.0f, 0.0f};
            box.transform.scale = {1.0f, 1.0f, 1.0f};
            box.transform.yawRadians = static_cast<float>(frameIndex) * 0.08f;
            box.size = {1.4f, 1.4f, 1.4f};
            box.tint = {255, 180, 60, 255};
            box.sourceId = "builder_smoke_opaque_box";
            builder.addPrimitive(box);
        }

        {
            bs3d::RenderPrimitiveCommand box;
            box.kind = bs3d::RenderPrimitiveKind::Box;
            box.bucket = bs3d::RenderBucket::Vehicle;
            box.transform.position = {0.35f, 0.0f, -0.35f};
            box.transform.scale = {1.0f, 1.0f, 1.0f};
            box.transform.yawRadians = -static_cast<float>(frameIndex) * 0.05f;
            box.size = {1.2f, 1.2f, 1.2f};
            box.tint = {80, 170, 255, 255};
            box.sourceId = "builder_smoke_vehicle_box";
            builder.addPrimitive(box);
        }

        builder.addDebugLine({-2.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {255, 40, 40, 255});
        builder.addDebugLine({0.0f, -2.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, {40, 255, 80, 255});
        builder.addDebugLine({0.0f, 0.0f, -2.0f}, {0.0f, 0.0f, 2.0f}, {80, 140, 255, 255});

        const auto validationResult = builder.validate();
        if (!validationResult.valid) {
            std::cerr << "RenderFrameBuilder smoke frame validation failed: " << validationResult.message << '\n';
        }

        return builder.build();
    }

    if (options.useExtractionFrame) {
        bs3d::RenderFrameBuilder builder;

        if (options.useCamera) {
            bs3d::RenderCamera camera;
            camera.position = {0.0f, 4.0f, -8.0f};
            camera.target = {2.0f, 0.0f, 1.0f};
            camera.up = {0.0f, 1.0f, 0.0f};
            camera.fovy = 60.0f;
            builder.setCamera(camera);
        }

        bs3d::WorldObject obj0;
        obj0.id = "smoke_extraction_shop";
        obj0.assetId = "shop_1";
        obj0.position = {0.0f, 0.0f, 0.0f};
        obj0.scale = {1.0f, 1.0f, 1.0f};

        bs3d::WorldObject obj1;
        obj1.id = "smoke_extraction_vehicle";
        obj1.assetId = "vehicle_1";
        obj1.position = {2.5f, 0.0f, 1.5f};
        obj1.scale = {1.0f, 1.0f, 1.0f};
        obj1.yawRadians = 0.3f;

        bs3d::WorldObject obj2;
        obj2.id = "smoke_extraction_barrier";
        obj2.assetId = "barrier_1";
        obj2.position = {1.0f, 0.0f, -2.0f};
        obj2.scale = {0.8f, 1.0f, 1.2f};

        bs3d::WorldObject obj3;
        obj3.id = "smoke_extraction_glass";
        obj3.assetId = "glass_1";
        obj3.position = {-1.5f, 0.0f, 0.5f};
        obj3.scale = {1.0f, 1.0f, 1.0f};

        bs3d::WorldObject obj4;
        obj4.id = "smoke_extraction_missing";
        obj4.assetId = "nonexistent_asset";
        obj4.position = {5.0f, 0.0f, 5.0f};

        bs3d::WorldObject obj5;
        obj5.id = "smoke_extraction_debug_only";
        obj5.assetId = "debug_only_1";
        obj5.position = {-3.0f, 0.0f, 3.0f};

        bs3d::WorldAssetDefinition shopDef;
        shopDef.id = "shop_1";
        shopDef.fallbackSize = {1.4f, 1.2f, 1.4f};
        shopDef.fallbackColor = {220, 200, 60, 255};
        shopDef.renderBucket = "Opaque";

        bs3d::WorldAssetDefinition vehicleDef;
        vehicleDef.id = "vehicle_1";
        vehicleDef.fallbackSize = {1.6f, 0.9f, 3.0f};
        vehicleDef.fallbackColor = {80, 120, 255, 255};
        vehicleDef.renderBucket = "Vehicle";

        bs3d::WorldAssetDefinition barrierDef;
        barrierDef.id = "barrier_1";
        barrierDef.fallbackSize = {1.8f, 0.6f, 0.3f};
        barrierDef.fallbackColor = {255, 80, 40, 255};
        barrierDef.renderBucket = "Decal";

        bs3d::WorldAssetDefinition glassDef;
        glassDef.id = "glass_1";
        glassDef.fallbackSize = {1.0f, 1.0f, 1.0f};
        glassDef.fallbackColor = {180, 220, 255, 128};
        glassDef.renderBucket = "Glass";

        bs3d::WorldAssetDefinition debugOnlyDef;
        debugOnlyDef.id = "debug_only_1";
        debugOnlyDef.fallbackSize = {1.0f, 1.0f, 1.0f};
        debugOnlyDef.fallbackColor = {255, 0, 255, 255};
        debugOnlyDef.renderBucket = "DebugOnly";

        std::vector<bs3d::WorldAssetDefinition> definitions = {shopDef, vehicleDef, barrierDef, glassDef, debugOnlyDef};

        bs3d::WorldRenderList renderList;
        renderList.opaque = {&obj0};
        renderList.translucent = {&obj3};
        renderList.glass = {};
        renderList.transparent = {&obj1, &obj2, &obj4, &obj5};

        const auto stats = bs3d::addWorldRenderListFallbackBoxes(builder, renderList, definitions);
        (void)stats;

        builder.addDebugLine({-3.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f}, {255, 40, 40, 255});
        builder.addDebugLine({0.0f, -3.0f, 0.0f}, {0.0f, 3.0f, 0.0f}, {40, 255, 80, 255});
        builder.addDebugLine({0.0f, 0.0f, -3.0f}, {0.0f, 0.0f, 3.0f}, {80, 140, 255, 255});

        const auto validationResult = builder.validate();
        if (!validationResult.valid) {
            std::cerr << "Extraction smoke frame validation failed: " << validationResult.message << '\n';
        }

        return builder.build();
    }

    bs3d::RenderFrame frame;

    if (options.useCamera) {
        frame.camera.position = {0.0f, 2.0f, -5.0f};
        frame.camera.target = {0.0f, 0.0f, 0.0f};
        frame.camera.up = {0.0f, 1.0f, 0.0f};
        frame.camera.fovy = 60.0f;
    }

    if (options.drawBox) {
        bs3d::RenderPrimitiveCommand box;
        box.kind = bs3d::RenderPrimitiveKind::Box;
        box.bucket = bs3d::RenderBucket::Opaque;
        box.transform.position = {0.0f, 0.0f, 0.0f};
        box.transform.scale = {1.0f, 1.0f, 1.0f};
        box.transform.yawRadians = static_cast<float>(frameIndex) * 0.08f;
        box.size = {1.4f, 1.4f, 1.4f};
        box.tint = {255, 180, 60, 255};
        box.sourceId = "d3d11_renderer_smoke_box";
        frame.primitives.push_back(box);
        if (options.drawTwoBoxes) {
            bs3d::RenderPrimitiveCommand secondBox = box;
            secondBox.transform.position = {0.35f, 0.0f, -0.35f};
            secondBox.transform.yawRadians = -static_cast<float>(frameIndex) * 0.05f;
            secondBox.size = {1.2f, 1.2f, 1.2f};
            secondBox.tint = {80, 170, 255, 255};
            secondBox.sourceId = "d3d11_renderer_smoke_depth_box";
            frame.primitives.push_back(secondBox);
        }
    }

    if (options.drawDebugLines) {
        frame.debugLines.push_back({{-2.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {255, 40, 40, 255}});
        frame.debugLines.push_back({{0.0f, -2.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, {40, 255, 80, 255}});
        frame.debugLines.push_back({{0.0f, 0.0f, -2.0f}, {0.0f, 0.0f, 2.0f}, {80, 140, 255, 255}});
        frame.debugLines.push_back({{-1.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 1.0f}, {255, 255, 255, 255}});
        frame.debugLines.push_back({{-1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, -1.0f}, {255, 255, 255, 255}});
    }
    return frame;
}

int runSmoke(const SmokeOptions& options) {
    constexpr int width = 640;
    constexpr int height = 360;

    HINSTANCE instance = GetModuleHandleW(nullptr);
    HWND window = createSmokeWindow(instance, width, height);

    bs3d::D3D11RendererConfig config;
    config.window = window;
    config.width = width;
    config.height = height;
#if !defined(NDEBUG)
    config.enableDebugLayer = true;
#endif

    bs3d::D3D11Renderer* renderer = nullptr;
    std::unique_ptr<bs3d::IRenderer> factoryOwned;
    bs3d::D3D11Renderer directRenderer;

    std::string error;

    if (options.useFactory) {
        bs3d::RendererFactoryRequest factoryRequest;
        factoryRequest.backend = bs3d::RendererBackendKind::D3D11;
        factoryRequest.allowExperimentalD3D11Renderer = true;

        auto factoryResult = bs3d::createRenderer(factoryRequest);
        if (!factoryResult.ok()) {
            if (IsWindow(window) != FALSE) {
                DestroyWindow(window);
            }
            throw std::runtime_error("RendererFactory failed for D3D11 with opt-in: " + factoryResult.error);
        }

        if (std::string(factoryResult.renderer->backendName()) != "d3d11") {
            if (IsWindow(window) != FALSE) {
                DestroyWindow(window);
            }
            throw std::runtime_error("RendererFactory returned wrong backend: " +
                                     std::string(factoryResult.renderer->backendName()));
        }

        auto* d3d11 = dynamic_cast<bs3d::D3D11Renderer*>(factoryResult.renderer.get());
        if (d3d11 == nullptr) {
            if (IsWindow(window) != FALSE) {
                DestroyWindow(window);
            }
            throw std::runtime_error("RendererFactory returned non-D3D11Renderer for D3D11 backend");
        }

        factoryOwned = std::move(factoryResult.renderer);
        renderer = d3d11;
        std::cout << "D3D11Renderer created through RendererFactory\n";
    } else {
        renderer = &directRenderer;
    }

    if (!renderer->initialize(config, &error)) {
        if (IsWindow(window) != FALSE) {
            DestroyWindow(window);
        }
        throw std::runtime_error("D3D11Renderer initialize failed: " + error);
    }
    std::cout << "D3D11Renderer initialized\n";

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

        renderer->renderFrame(makeSmokeFrame(options, renderedFrames));
        ++renderedFrames;
    }

    renderer->shutdown();
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
        const SmokeOptions options = parseOptions(argc, argv);
        return runSmoke(options);
    } catch (const std::exception& ex) {
        std::cerr << "D3D11Renderer smoke failed: " << ex.what() << '\n';
        return 1;
    }
}
