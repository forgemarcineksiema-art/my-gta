#pragma once

#include "EditorOverlayData.h"
#include "IntroLevelBuilder.h"

#include <string>
#include <vector>

namespace bs3d {

struct EditorOverlayApplyResult {
    int appliedOverrides = 0;
    int appliedInstances = 0;
    std::vector<std::string> warnings;
};

EditorOverlayApplyResult applyEditorOverlay(IntroLevelData& level,
                                            const EditorOverlayDocument& overlay);

} // namespace bs3d
