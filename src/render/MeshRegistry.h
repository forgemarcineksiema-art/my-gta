#pragma once

#include "bs3d/render/RenderFrame.h"

#include <string>
#include <unordered_map>

namespace bs3d {

class MeshRegistry {
public:
    MeshHandle allocate(const std::string& assetId);
    void release(MeshHandle handle);
    bool isValid(MeshHandle handle) const;
    const std::string* assetId(MeshHandle handle) const;
    MeshHandle find(const std::string& assetId) const;

private:
    std::uint32_t nextId_ = 2;
    std::unordered_map<std::uint32_t, std::string> entriesById_;
    std::unordered_map<std::string, std::uint32_t> idByAsset_;
};

} // namespace bs3d
