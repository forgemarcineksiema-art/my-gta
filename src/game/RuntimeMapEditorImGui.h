#pragma once

#include "RuntimeMapEditor.h"
#include "WorldAssetRegistry.h"

#include <string>

namespace bs3d {

class RuntimeMapEditorImGui {
public:
    void draw(RuntimeMapEditor& editor,
              const WorldAssetRegistry& registry,
              Vec3 placementPosition,
              const std::string& overlayPath);
    bool showCollision() const;

private:
    std::string lastStatus_;
    std::string lastInspectedId_;
    bool showCollision_ = false;
};

} // namespace bs3d
