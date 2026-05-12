#pragma once

#include "CpuMeshData.h"

#include <string>

namespace bs3d {

struct CpuMeshLoadResult {
    CpuMeshData mesh;
    bool ok = false;
    std::string error;
};

CpuMeshLoadResult loadCpuMeshFromObjText(const std::string& text, const std::string& debugName);

CpuMeshLoadResult loadCpuMeshFromObjFile(const std::string& path);

} // namespace bs3d
