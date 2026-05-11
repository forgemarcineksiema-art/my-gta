#pragma once

#include <string>

namespace bs3d {

struct PlatformWindowConfig {
    int width = 0;
    int height = 0;
    std::string title;
    int targetFps = 60;
    bool disableExitKey = true;
    bool captureCursor = true;
};

class IPlatform {
public:
    virtual ~IPlatform() = default;

    virtual void configureWindowFlags() const = 0;
    virtual void openWindow(const PlatformWindowConfig& config) const = 0;
    virtual void closeWindow() const = 0;
    virtual bool shouldClose() const = 0;
    virtual float frameTimeSeconds() const = 0;
    virtual int fps() const = 0;
    virtual void beginFrame() const = 0;
    virtual void endFrame() const = 0;
    virtual void toggleFullscreen() const = 0;
    virtual void captureCursor() const = 0;
    virtual void releaseCursor() const = 0;
    virtual void setupDevToolsUi() const = 0;
    virtual void shutdownDevToolsUi() const = 0;
};

} // namespace bs3d

