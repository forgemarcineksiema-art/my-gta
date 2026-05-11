#include "RaylibPlatform.h"

#include "raylib.h"

#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
#include "rlImGui.h"
#endif

namespace bs3d {

void RaylibPlatform::configureWindowFlags() const {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
}

void RaylibPlatform::openWindow(const PlatformWindowConfig& config) const {
    InitWindow(config.width, config.height, config.title.c_str());
    if (config.disableExitKey) {
        SetExitKey(KEY_NULL);
    }
    if (config.captureCursor) {
        DisableCursor();
    } else {
        EnableCursor();
    }
    SetTargetFPS(config.targetFps);
}

void RaylibPlatform::closeWindow() const {
    CloseWindow();
}

bool RaylibPlatform::shouldClose() const {
    return WindowShouldClose();
}

float RaylibPlatform::frameTimeSeconds() const {
    return GetFrameTime();
}

int RaylibPlatform::fps() const {
    return GetFPS();
}

void RaylibPlatform::beginFrame() const {
    BeginDrawing();
}

void RaylibPlatform::endFrame() const {
    EndDrawing();
}

void RaylibPlatform::toggleFullscreen() const {
    ToggleFullscreen();
}

void RaylibPlatform::captureCursor() const {
    DisableCursor();
}

void RaylibPlatform::releaseCursor() const {
    EnableCursor();
}

void RaylibPlatform::setupDevToolsUi() const {
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
    rlImGuiSetup(true);
#endif
}

void RaylibPlatform::shutdownDevToolsUi() const {
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
    rlImGuiShutdown();
#endif
}

} // namespace bs3d
