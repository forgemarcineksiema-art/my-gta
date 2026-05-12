#pragma once

#include "bs3d/render/RenderFrame.h"

#include <cstddef>
#include <string>
#include <unordered_map>

namespace bs3d {

class MaterialRegistry {
public:
    MaterialRegistry();

    MaterialHandle allocate(const std::string& name);
    void release(MaterialHandle handle);
    bool isValid(MaterialHandle handle) const;
    const std::string* name(MaterialHandle handle) const;
    MaterialHandle find(const std::string& name) const;
    MaterialHandle defaultOpaque() const;
    MaterialHandle defaultAlpha() const;

    std::size_t count() const;
    bool empty() const;

private:
    std::uint32_t nextId_ = 3;
    MaterialHandle defaultOpaque_{};
    MaterialHandle defaultAlpha_{};
    std::unordered_map<std::uint32_t, std::string> entriesById_;
    std::unordered_map<std::string, std::uint32_t> idByName_;
};

} // namespace bs3d
