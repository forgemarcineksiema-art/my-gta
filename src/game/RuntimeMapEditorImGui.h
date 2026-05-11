#pragma once

#include "RuntimeMapEditor.h"
#include "WorldAssetRegistry.h"

#include <string>

namespace bs3d {

class RuntimeMapEditorImGui {
public:
    void draw(RuntimeMapEditor& editor, const WorldAssetRegistry& registry, Vec3 placementPosition);

private:
    std::string lastStatus_;
};

} // namespace bs3d
