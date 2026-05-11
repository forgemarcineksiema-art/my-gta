#pragma once

#include "bs3d/backend/BackendKinds.h"

#include <string>

namespace bs3d {

struct GameRunOptions {
    bool loadSaveOnStart = true;
    bool writeSave = true;
    bool enableAudio = true;
    bool renderFrameShadow = false;
    bool d3d11ShadowWindow = false;
    bool d3d11ShadowDiagnostics = false;
    std::string dataRoot = "data";
    std::string savePath = "artifacts/savegame_intro.sav";
    std::string executablePath;
    int smokeFrames = 0;
    RendererBackendKind rendererBackend = RendererBackendKind::Raylib;
    PhysicsBackendKind physicsBackend = PhysicsBackendKind::Custom;
};

class GameApp {
public:
    void run();
    void run(const GameRunOptions& options);
};

} // namespace bs3d
