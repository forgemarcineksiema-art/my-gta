#include "MeshRegistry.h"

namespace bs3d {

MeshHandle MeshRegistry::allocate(const std::string& assetId) {
    const auto existing = idByAsset_.find(assetId);
    if (existing != idByAsset_.end()) {
        return MeshHandle{existing->second};
    }
    const std::uint32_t id = nextId_++;
    entriesById_[id] = assetId;
    idByAsset_[assetId] = id;
    return MeshHandle{id};
}

void MeshRegistry::release(MeshHandle handle) {
    if (handle.id == 0) {
        return;
    }
    const auto entry = entriesById_.find(handle.id);
    if (entry == entriesById_.end()) {
        return;
    }
    idByAsset_.erase(entry->second);
    entriesById_.erase(entry);
}

bool MeshRegistry::isValid(MeshHandle handle) const {
    return entriesById_.find(handle.id) != entriesById_.end();
}

const std::string* MeshRegistry::assetId(MeshHandle handle) const {
    const auto entry = entriesById_.find(handle.id);
    if (entry == entriesById_.end()) {
        return nullptr;
    }
    return &entry->second;
}

MeshHandle MeshRegistry::find(const std::string& assetId) const {
    const auto found = idByAsset_.find(assetId);
    if (found == idByAsset_.end()) {
        return MeshHandle{0};
    }
    return MeshHandle{found->second};
}

} // namespace bs3d
