#pragma once

#include "D3D11BootDevice.h"

namespace d3d11_spike {

struct Vertex {
    float position[3];
    float color[4];
};

void createCubePipeline(D3D11State& state);
void updateTransformConstants(D3D11State& state, int frameIndex, float aspectRatio);

} // namespace d3d11_spike
