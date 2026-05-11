#pragma once

#include <string>

namespace bs3d {

struct GameRunOptions {
    bool loadSaveOnStart = true;
    bool writeSave = true;
    bool enableAudio = true;
    std::string dataRoot = "data";
    std::string savePath = "artifacts/savegame_intro.sav";
    std::string executablePath;
    int smokeFrames = 0;
};

class GameApp {
public:
    void run();
    void run(const GameRunOptions& options);
};

} // namespace bs3d
