#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

namespace d3d11_spike {

HWND createBootWindow(HINSTANCE instance, int width, int height);

} // namespace d3d11_spike
