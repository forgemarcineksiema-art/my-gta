#pragma once

#include <string>
#include <vector>

namespace bs3d {

struct HudTextLayout {
    std::vector<std::string> objectiveLines;
    std::vector<std::string> controlsLines;
};

bool runtimeDevToolsEnabledByDefault();
std::string buildHudControlHelp(bool playerInVehicle, bool devToolsEnabled);
std::string buildHudDebugHelp();
HudTextLayout layoutHudText(const std::string& objectiveLine, const std::string& controlsText, int screenWidth);

} // namespace bs3d
