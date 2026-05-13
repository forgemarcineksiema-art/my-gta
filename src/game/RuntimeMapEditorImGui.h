#pragma once

#include "RuntimeMapEditor.h"
#include "WorldAssetRegistry.h"

#include <cstdint>
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
    std::string lastAssetObjectId_;
    std::string lastTagsObjectId_;
    std::uint64_t lastAssetRevision_ = 0;
    std::uint64_t lastTagsRevision_ = 0;
    bool showCollision_ = false;
};

} // namespace bs3d
