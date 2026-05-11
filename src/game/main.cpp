#include "bs3d/game/GameApp.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

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

void printHelp(std::ostream& out) {
    out << "Usage: blokowa_satyra [options]\n"
        << "  --data-root <path>       Data catalog root, defaults to data\n"
        << "  --save-path <path>       Save file path\n"
        << "  --renderer <raylib>      Renderer backend (planned: d3d11, not implemented)\n"
        << "  --physics <custom>       Physics backend (planned: physx, not implemented)\n"
        << "  --smoke-frames <count>   Exit after count rendered frames\n"
        << "  --no-load-save           Skip loading save at startup\n"
        << "  --no-save                Disable save writes\n"
        << "  --no-audio               Disable audio setup\n"
        << "  --help                   Show this help\n";
}

} // namespace

int main(int argc, char** argv) {
    bs3d::GameRunOptions options;
    if (argc > 0 && argv[0] != nullptr) {
        options.executablePath = argv[0];
    }
    try {
        for (int index = 1; index < argc; ++index) {
            const std::string arg = argv[index];
            if (arg == "--help") {
                printHelp(std::cout);
                return 0;
            }
            if (arg == "--no-load-save") {
                options.loadSaveOnStart = false;
            } else if (arg == "--no-save") {
                options.writeSave = false;
            } else if (arg == "--no-audio") {
                options.enableAudio = false;
            } else if (arg == "--data-root") {
                options.dataRoot = requireValue(index, argc, argv, arg);
            } else if (arg == "--save-path") {
                options.savePath = requireValue(index, argc, argv, arg);
            } else if (arg == "--renderer") {
                const std::string value = requireValue(index, argc, argv, arg);
                if (value == "raylib") {
                    options.rendererBackend = bs3d::RendererBackendKind::Raylib;
                } else if (value == "d3d11") {
                    throw std::runtime_error("Backend planned but not implemented yet: d3d11");
                } else {
                    throw std::runtime_error("Unknown renderer backend: " + value);
                }
            } else if (arg == "--physics") {
                const std::string value = requireValue(index, argc, argv, arg);
                if (value == "custom") {
                    options.physicsBackend = bs3d::PhysicsBackendKind::Custom;
                } else if (value == "physx") {
                    throw std::runtime_error("Backend planned but not implemented yet: physx");
                } else {
                    throw std::runtime_error("Unknown physics backend: " + value);
                }
            } else if (arg == "--smoke-frames") {
                options.smokeFrames = parseNonNegativeInt(requireValue(index, argc, argv, arg), arg);
            } else {
                throw std::runtime_error("unknown option: " + arg);
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Argument error: " << ex.what() << "\n\n";
        printHelp(std::cerr);
        return 2;
    }

    bs3d::GameApp app;
    try {
        app.run(options);
    } catch (const std::exception& ex) {
        std::cerr << "Game failed: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
