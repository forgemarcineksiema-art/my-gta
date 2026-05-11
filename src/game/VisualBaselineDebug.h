#pragma once

#include "WorldArtTypes.h"

#include <vector>

namespace bs3d {

class VisualBaselineDebug {
public:
    bool active() const;
    int activeIndex() const;
    void clear();
    void cycle(const std::vector<WorldViewpoint>& viewpoints);
    const WorldViewpoint* activeViewpoint(const std::vector<WorldViewpoint>& viewpoints) const;

private:
    int activeIndex_ = -1;
};

} // namespace bs3d
