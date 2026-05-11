#include "RenderExtraction.h"

#include "GameRenderers.h"

#include <algorithm>
#include <string>
#include <vector>

namespace bs3d {

namespace {

struct WorldRenderExtractionCommandSource {
    const WorldObject* object = nullptr;
    const WorldAssetDefinition* definition = nullptr;
};

RenderColor toRenderColor(WorldObjectTint tint) {
    return {tint.r, tint.g, tint.b, tint.a};
}

const WorldAssetDefinition* findDefinition(const std::vector<WorldAssetDefinition>& definitions,
                                           const std::string& assetId) {
    const auto found = std::find_if(definitions.begin(), definitions.end(), [&assetId](const WorldAssetDefinition& definition) {
        return definition.id == assetId;
    });
    return found != definitions.end() ? &*found : nullptr;
}

bool tryMapRenderBucket(const WorldAssetDefinition& definition,
                        RenderBucket& bucket,
                        WorldRenderExtractionStats& stats) {
    if (definition.renderBucket == "Opaque") {
        bucket = RenderBucket::Opaque;
        return true;
    }
    if (definition.renderBucket == "Vehicle") {
        bucket = RenderBucket::Vehicle;
        return true;
    }
    if (definition.renderBucket == "Decal") {
        bucket = RenderBucket::Decal;
        return true;
    }
    if (definition.renderBucket == "Glass") {
        bucket = RenderBucket::Glass;
        return true;
    }
    if (definition.renderBucket == "Translucent") {
        bucket = RenderBucket::Translucent;
        return true;
    }
    if (definition.renderBucket == "DebugOnly") {
        ++stats.skippedDebugOnly;
        return false;
    }
    ++stats.skippedUnsupportedBucket;
    return false;
}

bool alreadySeen(const std::vector<const WorldObject*>& seen, const WorldObject* object) {
    return std::find(seen.begin(), seen.end(), object) != seen.end();
}

void addObjectSource(const WorldObject* object,
                     const std::vector<WorldAssetDefinition>& assetDefinitions,
                     std::vector<const WorldObject*>& seen,
                     std::vector<WorldRenderExtractionCommandSource>& opaque,
                     std::vector<WorldRenderExtractionCommandSource>& vehicle,
                     std::vector<WorldRenderExtractionCommandSource>& decal,
                     std::vector<WorldRenderExtractionCommandSource>& glass,
                     std::vector<WorldRenderExtractionCommandSource>& translucent,
                     WorldRenderExtractionStats& stats) {
    if (object == nullptr || alreadySeen(seen, object)) {
        return;
    }
    seen.push_back(object);

    const WorldAssetDefinition* definition = findDefinition(assetDefinitions, object->assetId);
    if (definition == nullptr) {
        ++stats.skippedMissingDefinition;
        return;
    }

    RenderBucket bucket = RenderBucket::Opaque;
    if (!tryMapRenderBucket(*definition, bucket, stats)) {
        return;
    }

    switch (bucket) {
    case RenderBucket::Opaque:
        opaque.push_back({object, definition});
        break;
    case RenderBucket::Vehicle:
        vehicle.push_back({object, definition});
        break;
    case RenderBucket::Decal:
        decal.push_back({object, definition});
        break;
    case RenderBucket::Glass:
        glass.push_back({object, definition});
        break;
    case RenderBucket::Translucent:
        translucent.push_back({object, definition});
        break;
    case RenderBucket::Sky:
    case RenderBucket::Ground:
    case RenderBucket::Debug:
    case RenderBucket::Hud:
        ++stats.skippedUnsupportedBucket;
        break;
    }
}

void addObjectSources(const std::vector<const WorldObject*>& objects,
                      const std::vector<WorldAssetDefinition>& assetDefinitions,
                      std::vector<const WorldObject*>& seen,
                      std::vector<WorldRenderExtractionCommandSource>& opaque,
                      std::vector<WorldRenderExtractionCommandSource>& vehicle,
                      std::vector<WorldRenderExtractionCommandSource>& decal,
                      std::vector<WorldRenderExtractionCommandSource>& glass,
                      std::vector<WorldRenderExtractionCommandSource>& translucent,
                      WorldRenderExtractionStats& stats) {
    for (const WorldObject* object : objects) {
        addObjectSource(object, assetDefinitions, seen, opaque, vehicle, decal, glass, translucent, stats);
    }
}

void appendCommands(RenderFrame& frame,
                    const std::vector<WorldRenderExtractionCommandSource>& sources,
                    RenderBucket bucket,
                    int& bucketCount,
                    WorldRenderExtractionStats& stats) {
    for (const WorldRenderExtractionCommandSource& source : sources) {
        addWorldFallbackBox(frame, *source.object, *source.definition, bucket);
        ++bucketCount;
        ++stats.totalCommands;
    }
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

WorldRenderExtractionStats addWorldRenderListFallbackBoxes(
    RenderFrame& frame,
    const WorldRenderList& renderList,
    const std::vector<WorldAssetDefinition>& assetDefinitions) {
    WorldRenderExtractionStats stats;
    std::vector<const WorldObject*> seen;
    std::vector<WorldRenderExtractionCommandSource> opaque;
    std::vector<WorldRenderExtractionCommandSource> vehicle;
    std::vector<WorldRenderExtractionCommandSource> decal;
    std::vector<WorldRenderExtractionCommandSource> glass;
    std::vector<WorldRenderExtractionCommandSource> translucent;

    addObjectSources(renderList.opaque, assetDefinitions, seen, opaque, vehicle, decal, glass, translucent, stats);
    addObjectSources(renderList.translucent, assetDefinitions, seen, opaque, vehicle, decal, glass, translucent, stats);
    addObjectSources(renderList.glass, assetDefinitions, seen, opaque, vehicle, decal, glass, translucent, stats);
    addObjectSources(renderList.transparent, assetDefinitions, seen, opaque, vehicle, decal, glass, translucent, stats);

    appendCommands(frame, opaque, RenderBucket::Opaque, stats.opaqueCommands, stats);
    appendCommands(frame, vehicle, RenderBucket::Vehicle, stats.vehicleCommands, stats);
    appendCommands(frame, decal, RenderBucket::Decal, stats.decalCommands, stats);
    appendCommands(frame, glass, RenderBucket::Glass, stats.glassCommands, stats);
    appendCommands(frame, translucent, RenderBucket::Translucent, stats.translucentCommands, stats);
    return stats;
}

} // namespace bs3d
