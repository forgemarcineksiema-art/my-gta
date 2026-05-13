#include "RenderExtraction.h"

#include "MaterialRegistry.h"
#include "MeshRegistry.h"
#include "bs3d/render/WorldRenderList.h"

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

void builderAddFallbackBox(RenderFrameBuilder& builder,
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
    command.size = definition.fallbackSize;
    command.tint = object.hasTintOverride ? toRenderColor(object.tintOverride) : toRenderColor(definition.fallbackColor);
    command.sourceId = object.id;
    builder.addPrimitive(command);
}

void builderAppendCommands(RenderFrameBuilder& builder,
                           const std::vector<WorldRenderExtractionCommandSource>& sources,
                           RenderBucket bucket,
                           int& bucketCount,
                           WorldRenderExtractionStats& stats) {
    for (const WorldRenderExtractionCommandSource& source : sources) {
        builderAddFallbackBox(builder, *source.object, *source.definition, bucket);
        ++bucketCount;
        ++stats.totalCommands;
    }
}

void builderEmitMeshOrFallback(RenderFrameBuilder& builder,
                               const WorldObject& object,
                               const WorldAssetDefinition& definition,
                               RenderBucket bucket,
                               const MeshRegistry& meshRegistry,
                               const MaterialRegistry& materialRegistry,
                               WorldRenderExtractionStats& stats) {
    const auto meshHandle = meshRegistry.find(definition.id);
    if (meshHandle.id != 0) {
        RenderPrimitiveCommand command;
        command.kind = RenderPrimitiveKind::Mesh;
        command.bucket = bucket;
        command.mesh = meshHandle;
        if (bucket == RenderBucket::Glass || bucket == RenderBucket::Translucent) {
            command.material = materialRegistry.defaultAlpha();
        } else {
            command.material = materialRegistry.defaultOpaque();
        }
        command.transform.position = object.position + definition.visualOffset;
        command.transform.position.y += definition.fallbackSize.y * object.scale.y * 0.5f;
        command.transform.scale = object.scale;
        command.transform.yawRadians = object.yawRadians;
        command.size = definition.fallbackSize;
        command.tint = object.hasTintOverride ? toRenderColor(object.tintOverride) : toRenderColor(definition.fallbackColor);
        command.sourceId = definition.id;
        builder.addPrimitive(command);
        ++stats.emittedMeshes;
        ++stats.totalCommands;
    } else {
        builderAddFallbackBox(builder, object, definition, bucket);
        ++stats.meshFallbacks;
        ++stats.totalCommands;
    }
}

void builderAppendMeshCommands(RenderFrameBuilder& builder,
                               const std::vector<WorldRenderExtractionCommandSource>& sources,
                               RenderBucket bucket,
                               int& bucketCount,
                               WorldRenderExtractionStats& stats,
                               const MeshRegistry& meshRegistry,
                               const MaterialRegistry& materialRegistry) {
    for (const WorldRenderExtractionCommandSource& source : sources) {
        builderEmitMeshOrFallback(builder, *source.object, *source.definition,
                                  bucket, meshRegistry, materialRegistry, stats);
        ++bucketCount;
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
    command.size = definition.fallbackSize;
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

WorldRenderExtractionStats addWorldRenderListFallbackBoxes(
    RenderFrameBuilder& builder,
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

    builderAppendCommands(builder, opaque, RenderBucket::Opaque, stats.opaqueCommands, stats);
    builderAppendCommands(builder, vehicle, RenderBucket::Vehicle, stats.vehicleCommands, stats);
    builderAppendCommands(builder, decal, RenderBucket::Decal, stats.decalCommands, stats);
    builderAppendCommands(builder, glass, RenderBucket::Glass, stats.glassCommands, stats);
    builderAppendCommands(builder, translucent, RenderBucket::Translucent, stats.translucentCommands, stats);
    return stats;
}

WorldRenderExtractionStats addWorldRenderListMeshCommands(
    RenderFrameBuilder& builder,
    const WorldRenderList& renderList,
    const std::vector<WorldAssetDefinition>& definitions,
    const MeshRegistry& meshRegistry,
    const MaterialRegistry& materialRegistry) {
    WorldRenderExtractionStats stats;
    std::vector<const WorldObject*> seen;
    std::vector<WorldRenderExtractionCommandSource> opaque;
    std::vector<WorldRenderExtractionCommandSource> vehicle;
    std::vector<WorldRenderExtractionCommandSource> decal;
    std::vector<WorldRenderExtractionCommandSource> glass;
    std::vector<WorldRenderExtractionCommandSource> translucent;

    auto collectSources = [&](const std::vector<const WorldObject*>& objects) {
        for (const WorldObject* obj : objects) {
            if (obj == nullptr || alreadySeen(seen, obj)) {
                continue;
            }
            seen.push_back(obj);

            const WorldAssetDefinition* def = findDefinition(definitions, obj->assetId);
            if (def == nullptr) {
                ++stats.missingDefinitions;
                continue;
            }

            RenderBucket bucket = RenderBucket::Opaque;
            if (!tryMapRenderBucket(*def, bucket, stats)) {
                continue;
            }

            switch (bucket) {
            case RenderBucket::Opaque:
                opaque.push_back({obj, def});
                break;
            case RenderBucket::Vehicle:
                vehicle.push_back({obj, def});
                break;
            case RenderBucket::Decal:
                decal.push_back({obj, def});
                break;
            case RenderBucket::Glass:
                glass.push_back({obj, def});
                break;
            case RenderBucket::Translucent:
                translucent.push_back({obj, def});
                break;
            default:
                break;
            }
        }
    };

    collectSources(renderList.opaque);
    collectSources(renderList.translucent);
    collectSources(renderList.glass);
    collectSources(renderList.transparent);

    builderAppendMeshCommands(builder, opaque, RenderBucket::Opaque, stats.opaqueCommands,
                              stats, meshRegistry, materialRegistry);
    builderAppendMeshCommands(builder, vehicle, RenderBucket::Vehicle, stats.vehicleCommands,
                              stats, meshRegistry, materialRegistry);
    builderAppendMeshCommands(builder, decal, RenderBucket::Decal, stats.decalCommands,
                              stats, meshRegistry, materialRegistry);
    builderAppendMeshCommands(builder, glass, RenderBucket::Glass, stats.glassCommands,
                              stats, meshRegistry, materialRegistry);
    builderAppendMeshCommands(builder, translucent, RenderBucket::Translucent, stats.translucentCommands,
                              stats, meshRegistry, materialRegistry);
    return stats;
}

std::vector<std::string> selectShadowMeshSeedAssetIds(
    const WorldRenderList& renderList,
    const std::vector<WorldAssetDefinition>& definitions,
    int maxCount) {
    std::vector<std::string> result;
    if (maxCount <= 0) {
        return result;
    }

    using BucketSources = std::vector<const WorldObject*>;
    BucketSources opaque;
    BucketSources vehicle;
    BucketSources decal;
    BucketSources glass;
    BucketSources translucent;

    WorldRenderExtractionStats stats;

    // Collect objects from all render list fields, deduplicate, categorize by bucket.
    std::vector<const WorldObject*> seen;

    auto collectFrom = [&](const std::vector<const WorldObject*>& objects) {
        for (const WorldObject* obj : objects) {
            if (obj == nullptr || alreadySeen(seen, obj)) {
                continue;
            }
            seen.push_back(obj);

            const WorldAssetDefinition* def = findDefinition(definitions, obj->assetId);
            if (def == nullptr) {
                continue;
            }

            RenderBucket bucket = RenderBucket::Opaque;
            if (!tryMapRenderBucket(*def, bucket, stats)) {
                continue;
            }

            switch (bucket) {
            case RenderBucket::Opaque:   opaque.push_back(obj); break;
            case RenderBucket::Vehicle:  vehicle.push_back(obj); break;
            case RenderBucket::Decal:    decal.push_back(obj); break;
            case RenderBucket::Glass:    glass.push_back(obj); break;
            case RenderBucket::Translucent: translucent.push_back(obj); break;
            default: break;
            }
        }
    };

    collectFrom(renderList.opaque);
    collectFrom(renderList.translucent);
    collectFrom(renderList.glass);
    collectFrom(renderList.transparent);

    auto emitFromBucket = [&](const BucketSources& sources) {
        for (const WorldObject* obj : sources) {
            if (static_cast<int>(result.size()) >= maxCount) {
                return;
            }
            const auto& assetId = obj->assetId;
            if (std::find(result.begin(), result.end(), assetId) == result.end()) {
                result.push_back(assetId);
            }
        }
    };

    emitFromBucket(opaque);
    emitFromBucket(vehicle);
    emitFromBucket(decal);
    emitFromBucket(glass);
    emitFromBucket(translucent);

    return result;
}

std::vector<std::string> selectShadowMeshSeedAssetIdsFromDefinitions(
    const WorldRenderList& renderList,
    const std::vector<WorldAssetDefinition>& definitions,
    int maxCount) {
    std::vector<std::string> result;
    if (maxCount <= 0) {
        return result;
    }

    using BucketSources = std::vector<const WorldObject*>;
    BucketSources opaque;
    BucketSources vehicle;
    BucketSources decal;
    BucketSources glass;
    BucketSources translucent;

    WorldRenderExtractionStats stats;
    std::vector<const WorldObject*> seen;

    auto collectFrom = [&](const std::vector<const WorldObject*>& objects) {
        for (const WorldObject* obj : objects) {
            if (obj == nullptr || alreadySeen(seen, obj)) {
                continue;
            }
            seen.push_back(obj);

            const WorldAssetDefinition* def = findDefinition(definitions, obj->assetId);
            if (def == nullptr) {
                continue;
            }

            if (!def->renderInGameplay) {
                continue;
            }

            RenderBucket bucket = RenderBucket::Opaque;
            if (!tryMapRenderBucket(*def, bucket, stats)) {
                continue;
            }

            switch (bucket) {
            case RenderBucket::Opaque:   opaque.push_back(obj); break;
            case RenderBucket::Vehicle:  vehicle.push_back(obj); break;
            case RenderBucket::Decal:    decal.push_back(obj); break;
            case RenderBucket::Glass:    glass.push_back(obj); break;
            case RenderBucket::Translucent: translucent.push_back(obj); break;
            default: break;
            }
        }
    };

    collectFrom(renderList.opaque);
    collectFrom(renderList.translucent);
    collectFrom(renderList.glass);
    collectFrom(renderList.transparent);

    auto emitFromBucket = [&](const BucketSources& sources) {
        // First pass: emit assetIds with non-empty modelPath
        for (const WorldObject* obj : sources) {
            if (static_cast<int>(result.size()) >= maxCount) {
                return;
            }
            const auto& assetId = obj->assetId;
            if (std::find(result.begin(), result.end(), assetId) != result.end()) {
                continue;
            }
            const WorldAssetDefinition* def = findDefinition(definitions, assetId);
            if (def != nullptr && !def->modelPath.empty()) {
                result.push_back(assetId);
            }
        }

        // Second pass: emit remaining assetIds (no modelPath, use procedural fallback)
        for (const WorldObject* obj : sources) {
            if (static_cast<int>(result.size()) >= maxCount) {
                return;
            }
            const auto& assetId = obj->assetId;
            if (std::find(result.begin(), result.end(), assetId) != result.end()) {
                continue;
            }
            result.push_back(assetId);
        }
    };

    emitFromBucket(opaque);
    emitFromBucket(vehicle);
    emitFromBucket(decal);
    emitFromBucket(glass);
    emitFromBucket(translucent);

    return result;
}

} // namespace bs3d
