#include "CpuMeshData.h"

namespace bs3d {

bool isValidCpuMeshData(const CpuMeshData& mesh) {
    return !mesh.vertices.empty() && !mesh.indices.empty();
}

CpuMeshData makeCpuMeshUnitCube(const std::string& debugName) {
    CpuMeshData mesh;
    mesh.debugName = debugName;

    mesh.vertices.push_back({{-0.5f, -0.5f, -0.5f}});
    mesh.vertices.push_back({{-0.5f,  0.5f, -0.5f}});
    mesh.vertices.push_back({{ 0.5f,  0.5f, -0.5f}});
    mesh.vertices.push_back({{ 0.5f, -0.5f, -0.5f}});
    mesh.vertices.push_back({{-0.5f, -0.5f,  0.5f}});
    mesh.vertices.push_back({{-0.5f,  0.5f,  0.5f}});
    mesh.vertices.push_back({{ 0.5f,  0.5f,  0.5f}});
    mesh.vertices.push_back({{ 0.5f, -0.5f,  0.5f}});

    const std::uint16_t cubeIndices[] = {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7,
    };
    mesh.indices.assign(std::begin(cubeIndices), std::end(cubeIndices));

    return mesh;
}

CpuMeshData makeCpuMeshTriangle(const std::string& debugName) {
    CpuMeshData mesh;
    mesh.debugName = debugName;

    mesh.vertices.push_back({{0.0f, 0.5f, 0.0f}});
    mesh.vertices.push_back({{-0.5f, -0.5f, 0.0f}});
    mesh.vertices.push_back({{0.5f, -0.5f, 0.0f}});

    mesh.indices.push_back(0);
    mesh.indices.push_back(1);
    mesh.indices.push_back(2);

    return mesh;
}

} // namespace bs3d
