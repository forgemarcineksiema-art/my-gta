#include "VisualBaselineDebug.h"

namespace bs3d {

bool VisualBaselineDebug::active() const {
    return activeIndex_ >= 0;
}

int VisualBaselineDebug::activeIndex() const {
    return activeIndex_;
}

void VisualBaselineDebug::clear() {
    activeIndex_ = -1;
}

void VisualBaselineDebug::cycle(const std::vector<WorldViewpoint>& viewpoints) {
    if (viewpoints.empty()) {
        activeIndex_ = -1;
        return;
    }

    if (activeIndex_ < 0) {
        activeIndex_ = 0;
        return;
    }

    ++activeIndex_;
    if (activeIndex_ >= static_cast<int>(viewpoints.size())) {
        activeIndex_ = -1;
    }
}

const WorldViewpoint* VisualBaselineDebug::activeViewpoint(const std::vector<WorldViewpoint>& viewpoints) const {
    if (activeIndex_ < 0 || activeIndex_ >= static_cast<int>(viewpoints.size())) {
        return nullptr;
    }
    return &viewpoints[static_cast<std::size_t>(activeIndex_)];
}

} // namespace bs3d
