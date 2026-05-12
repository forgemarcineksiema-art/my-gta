#include "D3D11Renderer.h"

#include "RenderFrameDump.h"
#include "bs3d/render/RenderFrame.h"
#include "bs3d/render/RenderFrameValidation.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct ShellOptions {
    std::string loadFramePath;
    int frames = 120;
    bool diagnostics = false;
    bool orbitCamera = false;
    bool autoOrbit = false;
};

LRESULT CALLBACK shellWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
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

ShellOptions parseOptions(int argc, char** argv) {
    ShellOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--frames") {
            options.frames = parseNonNegativeInt(requireValue(index, argc, argv, arg), arg);
        } else if (arg == "--load-frame") {
            options.loadFramePath = requireValue(index, argc, argv, arg);
        } else if (arg == "--diagnostics") {
            options.diagnostics = true;
        } else if (arg == "--orbit-camera") {
            options.orbitCamera = true;
        } else if (arg == "--auto-orbit") {
            options.orbitCamera = true;
            options.autoOrbit = true;
        } else if (arg == "--help") {
            std::cout << "Usage: bs3d_d3d11_game_shell --load-frame <path> [--frames <count>] [--diagnostics]\n"
                      << "                                [--orbit-camera] [--auto-orbit]\n"
                      << "\n"
                      << "Standalone D3D11 main-window shell that loads a RenderFrameDump v1 file\n"
                      << "and renders it through D3D11Renderer.\n"
                      << "\n"
                      << "  --load-frame <path>   required: path to RenderFrameDump v1 text file\n"
                      << "  --frames <count>      frame count before exit (default 120)\n"
                      << "  --diagnostics         print frame stats and D3D11 draw coverage\n"
                      << "  --orbit-camera        enable orbit inspection camera controls\n"
                      << "  --auto-orbit          imply --orbit-camera and slowly auto-rotate\n"
                      << "\n"
                      << "Orbit camera controls:\n"
                      << "  Left/Right arrows  rotate yaw\n"
                      << "  A / D              rotate yaw\n"
                      << "  W / S              zoom radius in/out\n"
                      << "  Q / E              lower/raise height\n"
                      << "  R                  reset camera defaults\n"
                      << "\n"
                      << "  --help              print this message\n";
            std::exit(0);
        } else {
            throw std::runtime_error("unknown option: " + arg);
        }
    }
    return options;
}

HWND createShellWindow(HINSTANCE instance, int width, int height) {
    const wchar_t* className = L"BS3DD3D11GameShellWindow";

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = shellWindowProc;
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
                                  L"Blok 13 D3D11 Game Shell",
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

bs3d::RenderFrame loadFrame(const std::string& path) {
    bs3d::RenderFrame frame;
    std::string error;
    if (!bs3d::readRenderFrameDump(path, frame, &error)) {
        throw std::runtime_error("failed to load frame dump " + path + ": " + error);
    }
    return frame;
}

int runShell(const ShellOptions& options) {
    constexpr int width = 1280;
    constexpr int height = 720;

    HINSTANCE instance = GetModuleHandleW(nullptr);
    HWND window = createShellWindow(instance, width, height);

    bs3d::D3D11RendererConfig config;
    config.window = window;
    config.width = width;
    config.height = height;
#if !defined(NDEBUG)
    config.enableDebugLayer = true;
#endif

    bs3d::D3D11Renderer renderer;

    std::string error;
    if (!renderer.initialize(config, &error)) {
        if (IsWindow(window) != FALSE) {
            DestroyWindow(window);
        }
        throw std::runtime_error("D3D11Renderer initialize failed: " + error);
    }
    std::cout << "D3D11Renderer initialized\n";

    bs3d::RenderFrame frame = loadFrame(options.loadFramePath);
    std::cout << "loaded frame from " << options.loadFramePath << '\n';

    float orbitYaw = 0.0f;
    float orbitRadius = 8.0f;
    float orbitHeight = 4.0f;
    float orbitFovy = frame.camera.fovy > 0.0f ? frame.camera.fovy : 60.0f;
    bs3d::Vec3 orbitTarget = {0.0f, 0.0f, 0.0f};
    {
        const auto& t = frame.camera.target;
        if (t.x != 0.0f || t.y != 0.0f || t.z != 0.0f) {
            orbitTarget = t;
        }
    }
    const float autoOrbitSpeed = 0.02f;

    if (options.orbitCamera && options.diagnostics) {
        std::cout << "orbit camera: enabled"
                  << " auto-orbit=" << (options.autoOrbit ? "enabled" : "disabled")
                  << " radius=" << orbitRadius
                  << " height=" << orbitHeight
                  << " fovy=" << orbitFovy << '\n';
    }

    if (options.diagnostics) {
        const auto stats = bs3d::summarizeRenderFrame(frame);
        const auto validation = bs3d::validateRenderFrame(frame);
        std::cout << "loaded frame: primitives=" << stats.totalPrimitives
                  << " debugLines=" << stats.debugLines
                  << " valid=" << (validation.valid ? "true" : "false");
        if (!validation.valid) {
            std::cout << " (" << validation.message << ")";
        }
        std::cout << " opaque=" << stats.opaque
                  << " vehicle=" << stats.vehicle
                  << " decal=" << stats.decal
                  << " glass=" << stats.glass
                  << " translucent=" << stats.translucent
                  << " debug=" << stats.debug << '\n';
    }

    int renderedFrames = 0;
    bool diagnosticsCoveragePrinted = false;
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

        if (options.orbitCamera) {
            if ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0 || (GetAsyncKeyState(0x41) & 0x8000) != 0) {
                orbitYaw -= 0.04f;
            }
            if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0 || (GetAsyncKeyState(0x44) & 0x8000) != 0) {
                orbitYaw += 0.04f;
            }
            if ((GetAsyncKeyState(0x57) & 0x8000) != 0) {
                orbitRadius -= 0.1f;
                if (orbitRadius < 1.0f) orbitRadius = 1.0f;
            }
            if ((GetAsyncKeyState(0x53) & 0x8000) != 0) {
                orbitRadius += 0.1f;
            }
            if ((GetAsyncKeyState(0x51) & 0x8000) != 0) {
                orbitHeight -= 0.1f;
            }
            if ((GetAsyncKeyState(0x45) & 0x8000) != 0) {
                orbitHeight += 0.1f;
            }
            if ((GetAsyncKeyState(0x52) & 0x8000) != 0) {
                orbitYaw = 0.0f;
                orbitRadius = 8.0f;
                orbitHeight = 4.0f;
                orbitFovy = frame.camera.fovy > 0.0f ? frame.camera.fovy : 60.0f;
                const auto& t = frame.camera.target;
                if (t.x != 0.0f || t.y != 0.0f || t.z != 0.0f) {
                    orbitTarget = t;
                } else {
                    orbitTarget = {0.0f, 0.0f, 0.0f};
                }
            }

            if (options.autoOrbit) {
                orbitYaw += autoOrbitSpeed;
            }

            frame.camera.position = {
                orbitTarget.x + std::sin(orbitYaw) * orbitRadius,
                orbitTarget.y + orbitHeight,
                orbitTarget.z - std::cos(orbitYaw) * orbitRadius};
            frame.camera.target = orbitTarget;
            frame.camera.up = {0.0f, 1.0f, 0.0f};
            frame.camera.fovy = orbitFovy;
        }

        renderer.renderFrame(frame);

        if (options.diagnostics && !diagnosticsCoveragePrinted) {
            diagnosticsCoveragePrinted = true;
            const auto& d3d11 = renderer.lastD3D11Stats();
            std::cout << "d3d11 coverage: drawnBoxes=" << d3d11.drawnBoxes
                      << " skipKinds=" << d3d11.skippedUnsupportedKinds
                      << " skipBuckets=" << d3d11.skippedUnsupportedBuckets
                      << " skipPrims=" << d3d11.skippedPrimitives
                      << " drawnLines=" << d3d11.drawnDebugLines
                      << " skipLines=" << d3d11.skippedDebugLines << '\n';
        }

        ++renderedFrames;
    }

    renderer.shutdown();
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
        ShellOptions options = parseOptions(argc, argv);

        if (options.loadFramePath.empty()) {
            std::cerr << "error: --load-frame is required\n"
                      << "Usage: bs3d_d3d11_game_shell --load-frame <path> [--frames <count>]\n"
                      << "Run with --help for more information.\n";
            return 1;
        }

        return runShell(options);
    } catch (const std::exception& ex) {
        std::cerr << "D3D11 game shell failed: " << ex.what() << '\n';
        return 1;
    }
}
