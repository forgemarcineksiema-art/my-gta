#pragma once

#include <string>

namespace bs3d {

struct RaylibWindowConfig {
    int width = 0;
    int height = 0;
    std::string title;
    int targetFps = 60;
    bool disableExitKey = true;
    bool captureCursor = true;
};

class RaylibPlatform {
public:
    void configureWindowFlags() const;
    void openWindow(const RaylibWindowConfig& config) const;
    void closeWindow() const;
    bool shouldClose() const;
    float frameTimeSeconds() const;
    int fps() const;
    void beginFrame() const;
    void endFrame() const;
    void toggleFullscreen() const;
    void captureCursor() const;
    void releaseCursor() const;
    void setupDevToolsUi() const;
    void shutdownDevToolsUi() const;
};

} // namespace bs3d
