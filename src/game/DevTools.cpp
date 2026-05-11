#include "DevTools.h"

#include <algorithm>
#include <sstream>

namespace bs3d {

namespace {

std::vector<std::string> wrapWords(const std::string& text, std::size_t maxChars) {
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word;
    std::string line;
    while (words >> word) {
        while (word.size() > maxChars) {
            const std::size_t room = line.empty() ? maxChars : maxChars - line.size() - 1;
            if (room > 3) {
                const std::size_t chunk = std::min(room, word.size());
                if (!line.empty()) {
                    line += ' ';
                }
                line += word.substr(0, chunk);
                word.erase(0, chunk);
                lines.push_back(line);
                line.clear();
            } else {
                if (!line.empty()) {
                    lines.push_back(line);
                    line.clear();
                }
                line = word.substr(0, maxChars);
                word.erase(0, maxChars);
                lines.push_back(line);
                line.clear();
            }
        }
        if (word.empty()) {
            continue;
        }
        if (line.empty()) {
            line = word;
            continue;
        }
        if (line.size() + 1 + word.size() > maxChars) {
            lines.push_back(line);
            line = word;
        } else {
            line += ' ';
            line += word;
        }
    }
    if (!line.empty()) {
        lines.push_back(line);
    }
    return lines;
}

} // namespace

bool runtimeDevToolsEnabledByDefault() {
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
    return true;
#else
    return false;
#endif
}

std::string buildHudControlHelp(bool playerInVehicle, bool devToolsEnabled) {
    std::string text = playerInVehicle
                           ? "Auto: WASD jazda  Shift gaz  Space ręczny  H/LMB klakson  E/F wysiądź  ESC/F2 mysz  F11 ekran"
                           : "Pieszo: Mysz kamera  WASD ruch  Shift sprint  Space skok  E/F akcja  LMB cios  RMB unik  Q/C/X/Z/V/G pozy  ESC/F2 mysz  F11 ekran";
    if (devToolsEnabled) {
        text += "  F1 debug";
    }
    return text;
}

std::string buildHudDebugHelp() {
    return "Debug: F3 mysz X  F5 stable cam  F6 render  F7 asset QA  F8 widok QA  F9 postac QA  F10 editor";
}

HudTextLayout layoutHudText(const std::string& objectiveLine, const std::string& controlsText, int screenWidth) {
    const int safeWidth = std::max(220, screenWidth - 36);
    const std::size_t objectiveChars = static_cast<std::size_t>(std::clamp(safeWidth / 9, 24, 56));
    const std::size_t controlsChars = static_cast<std::size_t>(std::clamp(safeWidth / 8, 28, 76));
    return {wrapWords(objectiveLine, objectiveChars), wrapWords(controlsText, controlsChars)};
}

} // namespace bs3d
