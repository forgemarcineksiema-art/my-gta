#include "MaterialRegistry.h"

namespace bs3d {

MaterialRegistry::MaterialRegistry() {
    entriesById_[1] = "default_opaque";
    idByName_["default_opaque"] = 1;
    defaultOpaque_ = MaterialHandle{1};

    entriesById_[2] = "default_alpha";
    idByName_["default_alpha"] = 2;
    defaultAlpha_ = MaterialHandle{2};
}

MaterialHandle MaterialRegistry::allocate(const std::string& materialName) {
    const auto existing = idByName_.find(materialName);
    if (existing != idByName_.end()) {
        return MaterialHandle{existing->second};
    }
    const std::uint32_t id = nextId_++;
    entriesById_[id] = materialName;
    idByName_[materialName] = id;
    return MaterialHandle{id};
}

void MaterialRegistry::release(MaterialHandle handle) {
    if (handle.id == 0) {
        return;
    }
    const auto entry = entriesById_.find(handle.id);
    if (entry == entriesById_.end()) {
        return;
    }
    idByName_.erase(entry->second);
    entriesById_.erase(entry);
}

bool MaterialRegistry::isValid(MaterialHandle handle) const {
    return entriesById_.find(handle.id) != entriesById_.end();
}

const std::string* MaterialRegistry::name(MaterialHandle handle) const {
    const auto entry = entriesById_.find(handle.id);
    if (entry == entriesById_.end()) {
        return nullptr;
    }
    return &entry->second;
}

MaterialHandle MaterialRegistry::find(const std::string& materialName) const {
    const auto found = idByName_.find(materialName);
    if (found == idByName_.end()) {
        return MaterialHandle{0};
    }
    return MaterialHandle{found->second};
}

MaterialHandle MaterialRegistry::defaultOpaque() const {
    return defaultOpaque_;
}

MaterialHandle MaterialRegistry::defaultAlpha() const {
    return defaultAlpha_;
}

} // namespace bs3d
