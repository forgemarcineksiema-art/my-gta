#pragma once

#include "WorldArtTypes.h"
#include "WorldAssetRegistry.h"

#include "bs3d/render/RenderFrame.h"
#include "bs3d/render/RenderFrameBuilder.h"

#include <vector>

namespace bs3d {

struct WorldRenderList;

struct WorldRenderExtractionStats {
    int opaqueCommands = 0;
    int vehicleCommands = 0;
    int decalCommands = 0;
    int glassCommands = 0;
    int translucentCommands = 0;
    int skippedMissingDefinition = 0;
    int skippedDebugOnly = 0;
    int skippedUnsupportedBucket = 0;
    int totalCommands = 0;
};

RenderFrame makeEmptyRenderFrame(RenderCamera camera, WorldPresentationStyle style);
void addWorldFallbackBox(RenderFrame& frame,
                         const WorldObject& object,
                         const WorldAssetDefinition& definition,
                         RenderBucket bucket);
void addDebugLine(RenderFrame& frame, Vec3 start, Vec3 end, RenderColor tint);
WorldRenderExtractionStats addWorldRenderListFallbackBoxes(
    RenderFrame& frame,
    const WorldRenderList& renderList,
    const std::vector<WorldAssetDefinition>& assetDefinitions);
WorldRenderExtractionStats addWorldRenderListFallbackBoxes(
    RenderFrameBuilder& builder,
    const WorldRenderList& renderList,
    const std::vector<WorldAssetDefinition>& assetDefinitions);

} // namespace bs3d
