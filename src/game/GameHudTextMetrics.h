#pragma once

#include <string>
#include <vector>

namespace bs3d {

class HudTextMetrics {
public:
    virtual ~HudTextMetrics() = default;
    virtual int measureTextWidth(const std::string& text, int fontSize) const = 0;
};

std::vector<std::string> wrapHudTextToWidth(const std::string& text,
                                            int fontSize,
                                            int maxWidth,
                                            const HudTextMetrics& metrics);

} // namespace bs3d
