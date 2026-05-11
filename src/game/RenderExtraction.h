#pragma once

#include "WorldArtTypes.h"
#include "WorldAssetRegistry.h"

#include "bs3d/render/RenderFrame.h"

namespace bs3d {

RenderFrame makeEmptyRenderFrame(RenderCamera camera, WorldPresentationStyle style);
void addWorldFallbackBox(RenderFrame& frame,
                         const WorldObject& object,
                         const WorldAssetDefinition& definition,
                         RenderBucket bucket);
void addDebugLine(RenderFrame& frame, Vec3 start, Vec3 end, RenderColor tint);

} // namespace bs3d
