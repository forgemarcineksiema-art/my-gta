#include "RenderExtraction.h"

namespace bs3d {

namespace {

RenderColor toRenderColor(WorldObjectTint tint) {
    return {tint.r, tint.g, tint.b, tint.a};
}

} // namespace

RenderFrame makeEmptyRenderFrame(RenderCamera camera, WorldPresentationStyle style) {
    RenderFrame frame;
    frame.camera = camera;
    frame.worldStyle = style;
    return frame;
}

void addWorldFallbackBox(RenderFrame& frame,
                         const WorldObject& object,
                         const WorldAssetDefinition& definition,
                         RenderBucket bucket) {
    RenderPrimitiveCommand command;
    command.kind = RenderPrimitiveKind::Box;
    command.bucket = bucket;
    command.transform.position = object.position + definition.visualOffset;
    command.transform.position.y += definition.fallbackSize.y * object.scale.y * 0.5f;
    command.transform.scale = object.scale;
    command.transform.yawRadians = object.yawRadians;
    command.size = {definition.fallbackSize.x * object.scale.x,
                    definition.fallbackSize.y * object.scale.y,
                    definition.fallbackSize.z * object.scale.z};
    command.tint = object.hasTintOverride ? toRenderColor(object.tintOverride) : toRenderColor(definition.fallbackColor);
    command.sourceId = object.id;
    frame.primitives.push_back(command);
}

void addDebugLine(RenderFrame& frame, Vec3 start, Vec3 end, RenderColor tint) {
    frame.debugLines.push_back({start, end, tint});
}

} // namespace bs3d
