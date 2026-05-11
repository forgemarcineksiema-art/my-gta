#include "D3D11BootWindow.h"
#include "D3D11BootDevice.h"

#include <iostream>
#include <stdexcept>

namespace {

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

} // namespace

namespace d3d11_spike {

HWND createBootWindow(HINSTANCE instance, int width, int height) {
    const wchar_t* className = L"BS3DD3D11BootSpikeWindow";

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = windowProc;
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
        throw std::runtime_error("CreateWindowExW failed with Win32 error " +
                                 formatWin32Error(GetLastError()));
    }

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    std::cout << "window created\n";
    return window;
}

} // namespace d3d11_spike
