#pragma once

#include "bs3d/core/Types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace bs3d {

struct CpuMeshVertex {
    Vec3 position{};
};

struct CpuMeshData {
    std::vector<CpuMeshVertex> vertices;
    std::vector<std::uint16_t> indices;
    std::string debugName;
};

bool isValidCpuMeshData(const CpuMeshData& mesh);

CpuMeshData makeCpuMeshUnitCube(const std::string& debugName);

CpuMeshData makeCpuMeshTriangle(const std::string& debugName);

} // namespace bs3d
