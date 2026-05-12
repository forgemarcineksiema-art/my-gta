#pragma once

#include "CpuMeshData.h"
#include "D3D11MeshCache.h"

namespace bs3d {

D3D11MeshUpload makeD3D11MeshUpload(const CpuMeshData& mesh);

} // namespace bs3d
