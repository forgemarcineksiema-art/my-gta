#include "D3D11MeshCache.h"

#include <d3d11.h>

namespace bs3d {

D3D11MeshCache::~D3D11MeshCache() {
    clear();
}

bool D3D11MeshCache::upload(ID3D11Device* device, MeshHandle handle, const D3D11MeshUpload& mesh, std::string* error) {
    if (device == nullptr) {
        if (error != nullptr) {
            *error = "D3D11MeshCache::upload requires a valid ID3D11Device";
        }
        return false;
    }
    if (handle.id == 0) {
        if (error != nullptr) {
            *error = "D3D11MeshCache::upload requires a non-zero MeshHandle";
        }
        return false;
    }
    if (mesh.vertices.empty()) {
        if (error != nullptr) {
            *error = "D3D11MeshCache::upload requires non-empty vertices";
        }
        return false;
    }
    if (mesh.indices.empty()) {
        if (error != nullptr) {
            *error = "D3D11MeshCache::upload requires non-empty indices";
        }
        return false;
    }

    release(handle);

    Entry entry;

    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = static_cast<UINT>(mesh.vertices.size() * sizeof(D3D11MeshVertex));
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData{};
    vertexData.pSysMem = mesh.vertices.data();

    HRESULT hr = device->CreateBuffer(&vertexBufferDesc, &vertexData, &entry.vertexBuffer);
    if (FAILED(hr)) {
        if (error != nullptr) {
            *error = "D3D11MeshCache::upload CreateBuffer failed for vertex buffer";
        }
        return false;
    }

    D3D11_BUFFER_DESC indexBufferDesc{};
    indexBufferDesc.ByteWidth = static_cast<UINT>(mesh.indices.size() * sizeof(std::uint16_t));
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData{};
    indexData.pSysMem = mesh.indices.data();

    hr = device->CreateBuffer(&indexBufferDesc, &indexData, &entry.indexBuffer);
    if (FAILED(hr)) {
        entry.vertexBuffer->Release();
        entry.vertexBuffer = nullptr;
        if (error != nullptr) {
            *error = "D3D11MeshCache::upload CreateBuffer failed for index buffer";
        }
        return false;
    }

    entry.indexCount = static_cast<std::uint32_t>(mesh.indices.size());
    entries_[handle.id] = entry;
    return true;
}

bool D3D11MeshCache::contains(MeshHandle handle) const {
    return entries_.find(handle.id) != entries_.end();
}

bool D3D11MeshCache::find(MeshHandle handle, D3D11CachedMeshView& out) const {
    out = {};
    const auto found = entries_.find(handle.id);
    if (found == entries_.end()) {
        return false;
    }
    out.vertexBuffer = found->second.vertexBuffer;
    out.indexBuffer = found->second.indexBuffer;
    out.indexCount = found->second.indexCount;
    return true;
}

void D3D11MeshCache::release(MeshHandle handle) {
    const auto found = entries_.find(handle.id);
    if (found == entries_.end()) {
        return;
    }
    if (found->second.vertexBuffer != nullptr) {
        found->second.vertexBuffer->Release();
    }
    if (found->second.indexBuffer != nullptr) {
        found->second.indexBuffer->Release();
    }
    entries_.erase(found);
}

void D3D11MeshCache::clear() {
    for (auto& entry : entries_) {
        if (entry.second.vertexBuffer != nullptr) {
            entry.second.vertexBuffer->Release();
        }
        if (entry.second.indexBuffer != nullptr) {
            entry.second.indexBuffer->Release();
        }
    }
    entries_.clear();
}

std::size_t D3D11MeshCache::count() const {
    return entries_.size();
}

} // namespace bs3d
