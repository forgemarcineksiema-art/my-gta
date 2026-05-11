#pragma once

#include "bs3d/platform/IPlatform.h"

#include <string>

namespace bs3d {

class RaylibPlatform final : public IPlatform {
public:
    void configureWindowFlags() const override;
    void openWindow(const PlatformWindowConfig& config) const override;
    void closeWindow() const override;
    bool shouldClose() const override;
    float frameTimeSeconds() const override;
    int fps() const override;
    void beginFrame() const override;
    void endFrame() const override;
    void toggleFullscreen() const override;
    void captureCursor() const override;
    void releaseCursor() const override;
    void setupDevToolsUi() const override;
    void shutdownDevToolsUi() const override;
};

} // namespace bs3d
