#pragma once

#include "bs3d/render/RenderFrame.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct ID3D11Buffer;
struct ID3D11Device;

namespace bs3d {

struct D3D11MeshVertex {
    float position[3];
};

struct D3D11MeshUpload {
    std::vector<D3D11MeshVertex> vertices;
    std::vector<std::uint16_t> indices;
};

struct D3D11CachedMeshView {
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;
    std::uint32_t indexCount = 0;
};

class D3D11MeshCache {
public:
    D3D11MeshCache() = default;
    D3D11MeshCache(const D3D11MeshCache&) = delete;
    D3D11MeshCache& operator=(const D3D11MeshCache&) = delete;
    ~D3D11MeshCache();

    bool upload(ID3D11Device* device, MeshHandle handle, const D3D11MeshUpload& mesh, std::string* error = nullptr);

    bool contains(MeshHandle handle) const;
    bool find(MeshHandle handle, D3D11CachedMeshView& out) const;
    void release(MeshHandle handle);
    void clear();
    std::size_t count() const;

private:
    struct Entry {
        ID3D11Buffer* vertexBuffer = nullptr;
        ID3D11Buffer* indexBuffer = nullptr;
        std::uint32_t indexCount = 0;
    };

    std::unordered_map<std::uint32_t, Entry> entries_;
};

} // namespace bs3d
