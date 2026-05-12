#include "D3D11MeshUploadAdapter.h"

#include "CpuMeshData.h"

namespace bs3d {

D3D11MeshUpload makeD3D11MeshUpload(const CpuMeshData& mesh) {
    D3D11MeshUpload upload;
    upload.vertices.reserve(mesh.vertices.size());
    for (const CpuMeshVertex& vertex : mesh.vertices) {
        D3D11MeshVertex d3dVertex{};
        d3dVertex.position[0] = vertex.position.x;
        d3dVertex.position[1] = vertex.position.y;
        d3dVertex.position[2] = vertex.position.z;
        upload.vertices.push_back(d3dVertex);
    }
    upload.indices = mesh.indices;
    return upload;
}

} // namespace bs3d
