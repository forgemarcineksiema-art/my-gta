#include "RenderExtraction.h"
#include "RenderFrameDump.h"
#include "bs3d/render/IRenderer.h"
#include "bs3d/render/NullRenderer.h"
#include "bs3d/render/RendererFactory.h"
#include "bs3d/render/RenderFrameBuilder.h"
#include "bs3d/render/RenderFrameValidation.h"
#include "bs3d/render/WorldRenderList.h"

#include "bs3d/render/RenderFrame.h"
#include "bs3d/render/RenderTypes.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#if defined(BS3D_HAS_D3D11_RENDERER)
#include "D3D11Renderer.h"
#endif

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void expectNear(float actual, float expected, float epsilon, const std::string& message) {
    const float delta = actual > expected ? actual - expected : expected - actual;
    if (delta > epsilon) {
        std::cerr << "FAIL: " << message << " expected " << expected << " actual " << actual << '\n';
        std::exit(1);
    }
}

class RecordingRenderer : public bs3d::IRenderer {
public:
    const char* backendName() const override {
        return "recording";
    }

    void renderFrame(const bs3d::RenderFrame& frame) override {
        ++renderCalls;
        lastPrimitiveCount = static_cast<int>(frame.primitives.size());
        lastDebugLineCount = static_cast<int>(frame.debugLines.size());
        skyCount = 0;
        groundCount = 0;
        opaqueCount = 0;
        vehicleCount = 0;
        decalCount = 0;
        glassCount = 0;
        translucentCount = 0;
        debugCount = 0;
        hudCount = 0;
        lastBuckets.clear();

        for (const bs3d::RenderPrimitiveCommand& command : frame.primitives) {
            lastBuckets.push_back(command.bucket);
            switch (command.bucket) {
            case bs3d::RenderBucket::Sky:
                ++skyCount;
                break;
            case bs3d::RenderBucket::Ground:
                ++groundCount;
                break;
            case bs3d::RenderBucket::Opaque:
                ++opaqueCount;
                break;
            case bs3d::RenderBucket::Vehicle:
                ++vehicleCount;
                break;
            case bs3d::RenderBucket::Decal:
                ++decalCount;
                break;
            case bs3d::RenderBucket::Glass:
                ++glassCount;
                break;
            case bs3d::RenderBucket::Translucent:
                ++translucentCount;
                break;
            case bs3d::RenderBucket::Debug:
                ++debugCount;
                break;
            case bs3d::RenderBucket::Hud:
                ++hudCount;
                break;
            }
        }
    }

    int renderCalls = 0;
    int lastPrimitiveCount = 0;
    int lastDebugLineCount = 0;
    int skyCount = 0;
    int groundCount = 0;
    int opaqueCount = 0;
    int vehicleCount = 0;
    int decalCount = 0;
    int glassCount = 0;
    int translucentCount = 0;
    int debugCount = 0;
    int hudCount = 0;
    std::vector<bs3d::RenderBucket> lastBuckets;
};

void emptyRenderFrameHasNoCommands() {
    bs3d::RenderCamera camera;
    camera.position = {1.0f, 2.0f, 3.0f};
    bs3d::WorldPresentationStyle style;
    style.groundPlaneSize = 42.0f;

    const bs3d::RenderFrame frame = bs3d::makeEmptyRenderFrame(camera, style);

    expect(frame.primitives.empty(), "empty frame has no primitive commands");
    expect(frame.debugLines.empty(), "empty frame has no debug line commands");
    expectNear(frame.camera.position.x, 1.0f, 0.001f, "empty frame stores camera");
    expectNear(frame.worldStyle.groundPlaneSize, 42.0f, 0.001f, "empty frame stores world style");
}

void fallbackBoxAddsOnePrimitiveCommand() {
    bs3d::RenderFrame frame;
    bs3d::WorldObject object;
    object.id = "test_box";
    object.position = {2.0f, 0.0f, -3.0f};
    object.scale = {2.0f, 3.0f, 4.0f};
    object.yawRadians = 0.75f;

    bs3d::WorldAssetDefinition definition;
    definition.fallbackSize = {1.5f, 2.0f, 0.5f};
    definition.fallbackColor = {10, 20, 30, 220};
    definition.visualOffset = {0.5f, 0.25f, -0.5f};

    bs3d::addWorldFallbackBox(frame, object, definition, bs3d::RenderBucket::Decal);

    expect(frame.primitives.size() == 1, "fallback box adds exactly one primitive command");
    const bs3d::RenderPrimitiveCommand& command = frame.primitives.front();
    expect(command.kind == bs3d::RenderPrimitiveKind::Box, "fallback box command uses Box primitive kind");
    expect(command.bucket == bs3d::RenderBucket::Decal, "fallback box command keeps requested bucket");
    expect(command.sourceId == "test_box", "fallback box command keeps source id");
    expectNear(command.transform.position.x, 2.5f, 0.001f, "fallback box applies visual offset x");
    expectNear(command.transform.position.y, 3.25f, 0.001f, "fallback box centers bottom-origin box on y");
    expectNear(command.transform.position.z, -3.5f, 0.001f, "fallback box applies visual offset z");
    expectNear(command.transform.yawRadians, 0.75f, 0.001f, "fallback box keeps yaw");
    expectNear(command.size.x, 3.0f, 0.001f, "fallback box scales size x");
    expectNear(command.size.y, 6.0f, 0.001f, "fallback box scales size y");
    expectNear(command.size.z, 2.0f, 0.001f, "fallback box scales size z");
    expect(command.tint.r == 10 && command.tint.g == 20 && command.tint.b == 30 && command.tint.a == 220,
           "fallback box adapts definition tint to RenderColor");
}

void fallbackBoxUsesTintOverride() {
    bs3d::RenderFrame frame;
    bs3d::WorldObject object;
    object.hasTintOverride = true;
    object.tintOverride = {100, 110, 120, 130};

    bs3d::WorldAssetDefinition definition;
    definition.fallbackColor = {10, 20, 30, 40};

    bs3d::addWorldFallbackBox(frame, object, definition, bs3d::RenderBucket::Opaque);

    const bs3d::RenderPrimitiveCommand& command = frame.primitives.front();
    expect(command.tint.r == 100 && command.tint.g == 110 && command.tint.b == 120 && command.tint.a == 130,
           "fallback box adapts object tint override to RenderColor");
}

void debugLineAddsOneLineCommand() {
    bs3d::RenderFrame frame;
    bs3d::addDebugLine(frame, {0.0f, 1.0f, 2.0f}, {3.0f, 4.0f, 5.0f}, {6, 7, 8, 9});

    expect(frame.debugLines.size() == 1, "debug line adds exactly one line command");
    const bs3d::RenderLineCommand& command = frame.debugLines.front();
    expectNear(command.start.y, 1.0f, 0.001f, "debug line stores start");
    expectNear(command.end.z, 5.0f, 0.001f, "debug line stores end");
    expect(command.tint.r == 6 && command.tint.a == 9, "debug line stores tint");
}

void bucketAndKindValuesAreStable() {
    expect(static_cast<int>(bs3d::RenderBucket::Sky) == 0, "RenderBucket Sky ordinal is stable");
    expect(static_cast<int>(bs3d::RenderBucket::Opaque) == 2, "RenderBucket Opaque ordinal is stable");
    expect(static_cast<int>(bs3d::RenderBucket::Hud) == 8, "RenderBucket Hud ordinal is stable");
    expect(static_cast<int>(bs3d::RenderPrimitiveKind::Box) == 0, "RenderPrimitiveKind Box ordinal is stable");
    expect(static_cast<int>(bs3d::RenderPrimitiveKind::Mesh) == 4, "RenderPrimitiveKind Mesh ordinal is stable");
    expect(std::string(bs3d::renderBucketName(bs3d::RenderBucket::Glass)) == "Glass",
           "RenderBucket helper returns stable Glass name");
    expect(std::string(bs3d::renderPrimitiveKindName(bs3d::RenderPrimitiveKind::CylinderX)) == "CylinderX",
           "RenderPrimitiveKind helper returns stable CylinderX name");
}

bs3d::WorldObject worldObject(const std::string& id, const std::string& assetId) {
    bs3d::WorldObject object;
    object.id = id;
    object.assetId = assetId;
    return object;
}

bs3d::WorldAssetDefinition assetDefinition(const std::string& id, const std::string& renderBucket) {
    bs3d::WorldAssetDefinition definition;
    definition.id = id;
    definition.renderBucket = renderBucket;
    return definition;
}

struct TestWorldRenderFixture {
    bs3d::WorldObject opaque;
    bs3d::WorldObject decal;
    bs3d::WorldObject glass;
    bs3d::WorldObject translucent;
    std::vector<bs3d::WorldAssetDefinition> definitions;
    bs3d::WorldRenderList renderList;

    TestWorldRenderFixture() {
        opaque = worldObject("opaque_object", "opaque_asset");
        decal = worldObject("decal_object", "decal_asset");
        glass = worldObject("glass_object", "glass_asset");
        translucent = worldObject("translucent_object", "translucent_asset");

        definitions = {
            assetDefinition("opaque_asset", "Opaque"),
            assetDefinition("decal_asset", "Decal"),
            assetDefinition("glass_asset", "Glass"),
            assetDefinition("translucent_asset", "Translucent"),
        };

        renderList.transparent.push_back(&translucent);
        renderList.transparent.push_back(&glass);
        renderList.transparent.push_back(&decal);
        renderList.opaque.push_back(&opaque);
    }
};

bs3d::RenderFrame makeExtractedTestFrame() {
    TestWorldRenderFixture fixture;
    bs3d::RenderFrame frame;
    bs3d::addWorldRenderListFallbackBoxes(frame, fixture.renderList, fixture.definitions);
    bs3d::addDebugLine(frame, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {255, 255, 255, 255});
    return frame;
}

bs3d::RenderFrame makeInvalidBucketOrderFrame() {
    bs3d::RenderFrame frame;
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Glass});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Opaque});
    bs3d::addDebugLine(frame, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {255, 255, 255, 255});
    return frame;
}

void emptyWorldRenderListExtractionProducesNoCommands() {
    bs3d::RenderFrame frame;
    bs3d::WorldRenderList renderList;
    const std::vector<bs3d::WorldAssetDefinition> definitions;

    const bs3d::WorldRenderExtractionStats stats =
        bs3d::addWorldRenderListFallbackBoxes(frame, renderList, definitions);

    expect(frame.primitives.empty(), "empty world render list extracts no primitive commands");
    expect(stats.totalCommands == 0, "empty world render list extracts zero total commands");
    expect(stats.skippedMissingDefinition == 0, "empty world render list skips no missing definitions");
}

void worldRenderListExtractionCountsBuckets() {
    bs3d::WorldObject opaque = worldObject("opaque_object", "opaque_asset");
    bs3d::WorldObject decal = worldObject("decal_object", "decal_asset");
    bs3d::WorldObject glass = worldObject("glass_object", "glass_asset");
    bs3d::WorldObject translucent = worldObject("translucent_object", "translucent_asset");
    bs3d::WorldRenderList renderList;
    renderList.opaque.push_back(&opaque);
    renderList.translucent.push_back(&decal);
    renderList.glass.push_back(&glass);
    renderList.transparent.push_back(&decal);
    renderList.transparent.push_back(&glass);
    renderList.transparent.push_back(&translucent);
    const std::vector<bs3d::WorldAssetDefinition> definitions{
        assetDefinition("opaque_asset", "Opaque"),
        assetDefinition("decal_asset", "Decal"),
        assetDefinition("glass_asset", "Glass"),
        assetDefinition("translucent_asset", "Translucent"),
    };

    bs3d::RenderFrame frame;
    const bs3d::WorldRenderExtractionStats stats =
        bs3d::addWorldRenderListFallbackBoxes(frame, renderList, definitions);

    expect(stats.opaqueCommands == 1, "world render extraction counts opaque commands");
    expect(stats.decalCommands == 1, "world render extraction counts decal commands");
    expect(stats.glassCommands == 1, "world render extraction counts glass commands");
    expect(stats.translucentCommands == 1, "world render extraction counts translucent commands");
    expect(stats.totalCommands == 4, "world render extraction counts total commands");
    expect(frame.primitives.size() == 4, "world render extraction emits one command per unique object");
}

void worldRenderListExtractionOrdersBuckets() {
    TestWorldRenderFixture fixture;
    bs3d::RenderFrame frame;
    bs3d::addWorldRenderListFallbackBoxes(frame, fixture.renderList, fixture.definitions);

    expect(frame.primitives.size() == 4, "world render extraction emits ordered command list");
    expect(frame.primitives[0].bucket == bs3d::RenderBucket::Opaque, "opaque commands are extracted first");
    expect(frame.primitives[1].bucket == bs3d::RenderBucket::Decal, "decal commands are extracted after opaque");
    expect(frame.primitives[2].bucket == bs3d::RenderBucket::Glass, "glass commands are extracted after decals");
    expect(frame.primitives[3].bucket == bs3d::RenderBucket::Translucent,
           "translucent commands are extracted after glass");
}

void missingDefinitionsAreSkippedAndCounted() {
    bs3d::WorldObject present = worldObject("present_object", "present_asset");
    bs3d::WorldObject missing = worldObject("missing_object", "missing_asset");
    bs3d::WorldRenderList renderList;
    renderList.opaque.push_back(&present);
    renderList.opaque.push_back(&missing);
    const std::vector<bs3d::WorldAssetDefinition> definitions{
        assetDefinition("present_asset", "Opaque"),
    };

    bs3d::RenderFrame frame;
    const bs3d::WorldRenderExtractionStats stats =
        bs3d::addWorldRenderListFallbackBoxes(frame, renderList, definitions);

    expect(stats.totalCommands == 1, "missing definition extraction keeps present asset command");
    expect(stats.skippedMissingDefinition == 1, "missing definition extraction counts skipped missing asset");
    expect(frame.primitives.size() == 1, "missing definition extraction skips missing asset command");
    expect(frame.primitives.front().sourceId == "present_object", "missing definition extraction keeps present object");
}

void debugOnlyDefinitionsAreSkippedAndCounted() {
    bs3d::WorldObject debug = worldObject("debug_object", "debug_asset");
    bs3d::WorldRenderList renderList;
    renderList.opaque.push_back(&debug);
    const std::vector<bs3d::WorldAssetDefinition> definitions{
        assetDefinition("debug_asset", "DebugOnly"),
    };

    bs3d::RenderFrame frame;
    const bs3d::WorldRenderExtractionStats stats =
        bs3d::addWorldRenderListFallbackBoxes(frame, renderList, definitions);

    expect(frame.primitives.empty(), "debug-only extraction emits no gameplay primitive commands");
    expect(stats.skippedDebugOnly == 1, "debug-only extraction counts skipped debug-only asset");
    expect(stats.totalCommands == 0, "debug-only extraction counts zero emitted commands");
}

void summarizeRenderFrameCountsEmptyFrame() {
    const bs3d::RenderFrame frame;

    const bs3d::RenderFrameStats stats = bs3d::summarizeRenderFrame(frame);

    expect(stats.totalPrimitives == 0, "render frame stats counts zero primitives for empty frame");
    expect(stats.debugLines == 0, "render frame stats counts zero debug lines for empty frame");
    expect(stats.sky == 0, "render frame stats counts zero sky primitives for empty frame");
    expect(stats.hud == 0, "render frame stats counts zero hud primitives for empty frame");
}

void summarizeRenderFrameCountsBucketsAndDebugLines() {
    bs3d::RenderFrame frame;
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Sky});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Ground});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Opaque});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Vehicle});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Decal});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Glass});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Translucent});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Debug});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Hud});
    bs3d::addDebugLine(frame, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {255, 255, 255, 255});
    bs3d::addDebugLine(frame, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {255, 255, 255, 255});

    const bs3d::RenderFrameStats stats = bs3d::summarizeRenderFrame(frame);

    expect(stats.totalPrimitives == 9, "render frame stats counts total primitives");
    expect(stats.debugLines == 2, "render frame stats counts debug lines");
    expect(stats.sky == 1, "render frame stats counts sky bucket");
    expect(stats.ground == 1, "render frame stats counts ground bucket");
    expect(stats.opaque == 1, "render frame stats counts opaque bucket");
    expect(stats.vehicle == 1, "render frame stats counts vehicle bucket");
    expect(stats.decal == 1, "render frame stats counts decal bucket");
    expect(stats.glass == 1, "render frame stats counts glass bucket");
    expect(stats.translucent == 1, "render frame stats counts translucent bucket");
    expect(stats.debug == 1, "render frame stats counts debug bucket");
    expect(stats.hud == 1, "render frame stats counts hud bucket");
}

void extractedWorldRenderListFrameHasValidBucketOrder() {
    const bs3d::RenderFrame frame = makeExtractedTestFrame();

    expect(bs3d::isRenderFrameBucketOrderValid(frame), "extracted world render list frame has valid bucket order");
    const bs3d::RenderFrameValidationResult result = bs3d::validateRenderFrame(frame);
    expect(result.valid, "validate render frame accepts extracted world render list frame");
}

void invalidBucketOrderIsRejected() {
    const bs3d::RenderFrame frame = makeInvalidBucketOrderFrame();

    const bs3d::RenderFrameValidationResult result = bs3d::validateRenderFrame(frame);

    expect(!bs3d::isRenderFrameBucketOrderValid(frame), "invalid bucket order is rejected");
    expect(!result.valid, "validate render frame reports invalid bucket order");
    expect(result.message.find("Opaque") != std::string::npos, "invalid bucket order message names current bucket");
    expect(result.message.find("Glass") != std::string::npos, "invalid bucket order message names previous bucket");
}

void emptyRenderFrameValidationSucceeds() {
    const bs3d::RenderFrame frame;

    const bs3d::RenderFrameValidationResult result = bs3d::validateRenderFrame(frame);

    expect(bs3d::isRenderFrameBucketOrderValid(frame), "empty frame has valid bucket order");
    expect(result.valid, "validate render frame accepts empty frame");
    expect(result.message.empty(), "valid empty frame has no validation message");
}

void recordingRendererConsumesEmptyRenderFrame() {
    RecordingRenderer recordingRenderer;
    bs3d::IRenderer* renderer = &recordingRenderer;
    const bs3d::RenderFrame frame;

    renderer->renderFrame(frame);

    expect(std::string(renderer->backendName()) == "recording", "recording renderer exposes backend name");
    expect(recordingRenderer.renderCalls == 1, "recording renderer consumes an empty frame");
    expect(recordingRenderer.lastPrimitiveCount == 0, "recording renderer records zero primitives");
    expect(recordingRenderer.lastDebugLineCount == 0, "recording renderer records zero debug lines");
    expect(recordingRenderer.lastBuckets.empty(), "recording renderer records no buckets for empty frame");
}

void recordingRendererConsumesExtractedWorldRenderListFrame() {
    TestWorldRenderFixture fixture;
    bs3d::RenderFrame frame;
    bs3d::addWorldRenderListFallbackBoxes(frame, fixture.renderList, fixture.definitions);
    bs3d::addDebugLine(frame, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {255, 255, 255, 255});

    RecordingRenderer recordingRenderer;
    bs3d::IRenderer& renderer = recordingRenderer;
    renderer.renderFrame(frame);

    expect(recordingRenderer.renderCalls == 1, "recording renderer consumes extracted render frame");
    expect(recordingRenderer.lastPrimitiveCount == 4, "recording renderer records extracted primitive count");
    expect(recordingRenderer.lastDebugLineCount == 1, "recording renderer records extracted debug line count");
    expect(recordingRenderer.opaqueCount == 1, "recording renderer counts opaque bucket");
    expect(recordingRenderer.decalCount == 1, "recording renderer counts decal bucket");
    expect(recordingRenderer.glassCount == 1, "recording renderer counts glass bucket");
    expect(recordingRenderer.translucentCount == 1, "recording renderer counts translucent bucket");
    expect(recordingRenderer.lastBuckets.size() == 4, "recording renderer records production bucket order");
    expect(recordingRenderer.lastBuckets[0] == bs3d::RenderBucket::Opaque,
           "recording renderer sees opaque bucket first");
    expect(recordingRenderer.lastBuckets[1] == bs3d::RenderBucket::Decal,
           "recording renderer sees decal bucket second");
    expect(recordingRenderer.lastBuckets[2] == bs3d::RenderBucket::Glass,
           "recording renderer sees glass bucket third");
    expect(recordingRenderer.lastBuckets[3] == bs3d::RenderBucket::Translucent,
           "recording renderer sees translucent bucket fourth");
}

// ---------- RenderFrameBuilder tests ----------

bs3d::RenderPrimitiveCommand makePrimitive(bs3d::RenderBucket bucket, const std::string& sourceId = "") {
    bs3d::RenderPrimitiveCommand command;
    command.bucket = bucket;
    command.sourceId = sourceId;
    return command;
}

void builderEmptyBuildIsValid() {
    bs3d::RenderFrameBuilder builder;
    const bs3d::RenderFrame frame = builder.build();

    expect(frame.primitives.empty(), "empty builder builds frame with no primitives");
    expect(frame.debugLines.empty(), "empty builder builds frame with no debug lines");
    expect(builder.validate().valid, "empty builder validates successfully");
}

void builderStoresCameraAndWorldStyle() {
    bs3d::RenderCamera camera;
    camera.position = {10.0f, 20.0f, 30.0f};
    camera.fovy = 60.0f;

    bs3d::WorldPresentationStyle style;
    style.groundPlaneSize = 99.0f;
    style.worldCullDistance = 200.0f;

    bs3d::RenderFrameBuilder builder;
    builder.setCamera(camera).setWorldStyle(style);
    const bs3d::RenderFrame frame = builder.build();

    expectNear(frame.camera.position.x, 10.0f, 0.001f, "builder stores camera position x");
    expectNear(frame.camera.position.y, 20.0f, 0.001f, "builder stores camera position y");
    expectNear(frame.camera.fovy, 60.0f, 0.001f, "builder stores camera fovy");
    expectNear(frame.worldStyle.groundPlaneSize, 99.0f, 0.001f, "builder stores world style ground plane size");
    expectNear(frame.worldStyle.worldCullDistance, 200.0f, 0.001f, "builder stores world style cull distance");
}

void builderReordersPrimitivesIntoProductionOrder() {
    bs3d::RenderFrameBuilder builder;
    // Add in deliberately scrambled order.
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Hud, "hud_1"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Opaque, "opaque_1"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Sky, "sky_1"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Translucent, "translucent_1"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Vehicle, "vehicle_1"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Ground, "ground_1"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Debug, "debug_1"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Decal, "decal_1"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Glass, "glass_1"));

    const bs3d::RenderFrame frame = builder.build();

    expect(frame.primitives.size() == 9, "builder emits all 9 primitives");
    expect(frame.primitives[0].bucket == bs3d::RenderBucket::Sky, "builder output[0] is Sky");
    expect(frame.primitives[1].bucket == bs3d::RenderBucket::Ground, "builder output[1] is Ground");
    expect(frame.primitives[2].bucket == bs3d::RenderBucket::Opaque, "builder output[2] is Opaque");
    expect(frame.primitives[3].bucket == bs3d::RenderBucket::Vehicle, "builder output[3] is Vehicle");
    expect(frame.primitives[4].bucket == bs3d::RenderBucket::Decal, "builder output[4] is Decal");
    expect(frame.primitives[5].bucket == bs3d::RenderBucket::Glass, "builder output[5] is Glass");
    expect(frame.primitives[6].bucket == bs3d::RenderBucket::Translucent, "builder output[6] is Translucent");
    expect(frame.primitives[7].bucket == bs3d::RenderBucket::Debug, "builder output[7] is Debug");
    expect(frame.primitives[8].bucket == bs3d::RenderBucket::Hud, "builder output[8] is Hud");

    expect(bs3d::isRenderFrameBucketOrderValid(frame), "builder output has valid bucket order");
}

void builderPreservesInsertionOrderInsideBucket() {
    bs3d::RenderFrameBuilder builder;
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Opaque, "first"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Opaque, "second"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Opaque, "third"));

    const bs3d::RenderFrame frame = builder.build();

    expect(frame.primitives.size() == 3, "builder emits 3 opaque primitives");
    expect(frame.primitives[0].sourceId == "first", "builder preserves insertion order: first");
    expect(frame.primitives[1].sourceId == "second", "builder preserves insertion order: second");
    expect(frame.primitives[2].sourceId == "third", "builder preserves insertion order: third");
}

void builderIncludesDebugLines() {
    bs3d::RenderFrameBuilder builder;
    builder.addDebugLine({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {255, 0, 0, 255});
    builder.addDebugLine(bs3d::RenderLineCommand{{2.0f, 2.0f, 2.0f}, {3.0f, 3.0f, 3.0f}, {0, 255, 0, 255}});

    const bs3d::RenderFrame frame = builder.build();

    expect(frame.debugLines.size() == 2, "builder includes both debug lines");
    expectNear(frame.debugLines[0].start.x, 0.0f, 0.001f, "builder debug line 0 start x");
    expectNear(frame.debugLines[0].end.x, 1.0f, 0.001f, "builder debug line 0 end x");
    expect(frame.debugLines[0].tint.r == 255 && frame.debugLines[0].tint.g == 0,
           "builder debug line 0 tint");
    expectNear(frame.debugLines[1].start.x, 2.0f, 0.001f, "builder debug line 1 start x");
    expect(frame.debugLines[1].tint.g == 255, "builder debug line 1 tint green");
}

void builderStatsMatchSummarize() {
    bs3d::RenderFrameBuilder builder;
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Sky));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Opaque));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Opaque));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Vehicle));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Hud));
    builder.addDebugLine({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {255, 255, 255, 255});

    const bs3d::RenderFrameStats builderStats = builder.stats();
    const bs3d::RenderFrameStats frameStats = bs3d::summarizeRenderFrame(builder.build());

    expect(builderStats.totalPrimitives == frameStats.totalPrimitives,
           "builder.stats() totalPrimitives matches summarizeRenderFrame");
    expect(builderStats.debugLines == frameStats.debugLines,
           "builder.stats() debugLines matches summarizeRenderFrame");
    expect(builderStats.sky == frameStats.sky,
           "builder.stats() sky matches summarizeRenderFrame");
    expect(builderStats.opaque == frameStats.opaque,
           "builder.stats() opaque matches summarizeRenderFrame");
    expect(builderStats.vehicle == frameStats.vehicle,
           "builder.stats() vehicle matches summarizeRenderFrame");
    expect(builderStats.hud == frameStats.hud,
           "builder.stats() hud matches summarizeRenderFrame");
    expect(builderStats.totalPrimitives == 5, "builder.stats() counts 5 total primitives");
    expect(builderStats.debugLines == 1, "builder.stats() counts 1 debug line");
}

void builderValidateSucceedsForBuilderOutput() {
    bs3d::RenderFrameBuilder builder;
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Hud));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Sky));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Opaque));

    const bs3d::RenderFrameValidationResult result = builder.validate();
    expect(result.valid, "builder.validate() succeeds even when primitives were added in scrambled order");
    expect(result.message.empty(), "builder.validate() has no error message");
}

void builderDoesNotRequireBackendTypes() {
    // This test verifies at compile time that RenderFrameBuilder does not pull
    // in any GPU, raylib, DirectX, or platform headers.  If it compiles, it
    // passes.  We also verify that zero-value MeshHandle and MaterialHandle are
    // accepted.
    bs3d::RenderPrimitiveCommand command;
    command.mesh = bs3d::MeshHandle{0};
    command.material = bs3d::MaterialHandle{0};
    command.bucket = bs3d::RenderBucket::Opaque;

    bs3d::RenderFrameBuilder builder;
    builder.addPrimitive(command);
    const bs3d::RenderFrame frame = builder.build();

    expect(frame.primitives.size() == 1, "builder accepts zero-value handles");
    expect(frame.primitives[0].mesh.id == 0, "builder preserves zero mesh handle");
    expect(frame.primitives[0].material.id == 0, "builder preserves zero material handle");
}

void builderDebugLinesDoNotAffectPrimitiveOrdering() {
    bs3d::RenderFrameBuilder builder;
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Hud, "hud"));
    builder.addDebugLine({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {255, 255, 255, 255});
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Sky, "sky"));
    builder.addDebugLine({2.0f, 2.0f, 2.0f}, {3.0f, 3.0f, 3.0f}, {0, 0, 0, 255});

    const bs3d::RenderFrame frame = builder.build();

    expect(frame.primitives.size() == 2, "debug lines do not create extra primitives");
    expect(frame.primitives[0].bucket == bs3d::RenderBucket::Sky,
           "debug lines between primitives do not affect ordering: sky first");
    expect(frame.primitives[1].bucket == bs3d::RenderBucket::Hud,
           "debug lines between primitives do not affect ordering: hud second");
    expect(frame.debugLines.size() == 2, "debug lines are still present");
    expect(bs3d::isRenderFrameBucketOrderValid(frame),
           "frame with interleaved debug lines has valid bucket order");
}

void builderFrameWithMultipleBoxesAndDebugLinesValidates() {
    bs3d::RenderFrameBuilder builder;

    {
        bs3d::RenderPrimitiveCommand opaqueBox = makePrimitive(bs3d::RenderBucket::Opaque, "smoke_opaque");
        opaqueBox.kind = bs3d::RenderPrimitiveKind::Box;
        opaqueBox.transform.position = {0.0f, 0.0f, 0.0f};
        opaqueBox.size = {1.4f, 1.4f, 1.4f};
        opaqueBox.tint = {255, 180, 60, 255};
        builder.addPrimitive(opaqueBox);
    }

    {
        bs3d::RenderPrimitiveCommand vehicleBox = makePrimitive(bs3d::RenderBucket::Vehicle, "smoke_vehicle");
        vehicleBox.kind = bs3d::RenderPrimitiveKind::Box;
        vehicleBox.transform.position = {0.35f, 0.0f, -0.35f};
        vehicleBox.size = {1.2f, 1.2f, 1.2f};
        vehicleBox.tint = {80, 170, 255, 255};
        builder.addPrimitive(vehicleBox);
    }

    builder.addDebugLine({-2.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {255, 40, 40, 255});
    builder.addDebugLine({0.0f, -2.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, {40, 255, 80, 255});
    builder.addDebugLine({0.0f, 0.0f, -2.0f}, {0.0f, 0.0f, 2.0f}, {80, 140, 255, 255});

    const bs3d::RenderFrame frame = builder.build();

    expect(frame.primitives.size() == 2, "builder smoke frame has 2 primitives");
    expect(frame.primitives[0].bucket == bs3d::RenderBucket::Opaque, "builder smoke frame first is Opaque");
    expect(frame.primitives[1].bucket == bs3d::RenderBucket::Vehicle, "builder smoke frame second is Vehicle");
    expect(frame.debugLines.size() == 3, "builder smoke frame has 3 debug lines");

    const auto result = builder.validate();
    expect(result.valid, "builder smoke frame validates successfully");
    expect(result.message.empty(), "builder smoke frame validation has no error message");

    expect(bs3d::isRenderFrameBucketOrderValid(frame), "builder smoke frame has valid bucket order");
}

// ---------- Builder extraction tests ----------

void builderExtractionEmitsExpectedPrimitiveCount() {
    bs3d::WorldObject obj0;
    obj0.id = "test_shop";
    obj0.assetId = "shop_1";
    obj0.position = {0.0f, 0.0f, 0.0f};

    bs3d::WorldObject obj1;
    obj1.id = "test_vehicle";
    obj1.assetId = "vehicle_1";
    obj1.position = {2.5f, 0.0f, 1.5f};

    bs3d::WorldAssetDefinition shopDef;
    shopDef.id = "shop_1";
    shopDef.fallbackSize = {1.0f, 1.0f, 1.0f};
    shopDef.renderBucket = "Opaque";

    bs3d::WorldAssetDefinition vehicleDef;
    vehicleDef.id = "vehicle_1";
    vehicleDef.fallbackSize = {1.0f, 1.0f, 1.0f};
    vehicleDef.renderBucket = "Vehicle";

    std::vector<bs3d::WorldAssetDefinition> definitions = {shopDef, vehicleDef};

    bs3d::WorldRenderList renderList;
    renderList.opaque = {&obj0};
    renderList.transparent = {&obj1};

    bs3d::RenderFrameBuilder builder;
    const auto stats = bs3d::addWorldRenderListFallbackBoxes(builder, renderList, definitions);

    const bs3d::RenderFrame frame = builder.build();

    expect(frame.primitives.size() == 2, "builder extraction emits 2 primitives");
    expect(stats.totalCommands == 2, "builder extraction stats records 2 total commands");
    expect(stats.opaqueCommands == 1, "builder extraction stats records 1 opaque command");
    expect(stats.vehicleCommands == 1, "builder extraction stats records 1 vehicle command");
    expect(stats.skippedMissingDefinition == 0, "builder extraction has no missing definitions");
    expect(stats.skippedDebugOnly == 0, "builder extraction has no DebugOnly skips");
}

void builderExtractionPreservesBucketOrder() {
    bs3d::WorldObject obj0;
    obj0.id = "test_glass";
    obj0.assetId = "glass_1";
    obj0.position = {0.0f, 0.0f, 0.0f};

    bs3d::WorldObject obj1;
    obj1.id = "test_opaque";
    obj1.assetId = "opaque_1";
    obj1.position = {1.0f, 0.0f, 0.0f};

    bs3d::WorldObject obj2;
    obj2.id = "test_vehicle";
    obj2.assetId = "vehicle_1";
    obj2.position = {2.0f, 0.0f, 0.0f};

    bs3d::WorldAssetDefinition glassDef;
    glassDef.id = "glass_1";
    glassDef.renderBucket = "Glass";

    bs3d::WorldAssetDefinition opaqueDef;
    opaqueDef.id = "opaque_1";
    opaqueDef.renderBucket = "Opaque";

    bs3d::WorldAssetDefinition vehicleDef;
    vehicleDef.id = "vehicle_1";
    vehicleDef.renderBucket = "Vehicle";

    std::vector<bs3d::WorldAssetDefinition> definitions = {glassDef, opaqueDef, vehicleDef};

    bs3d::WorldRenderList renderList;
    renderList.opaque = {&obj1};
    renderList.transparent = {&obj0, &obj2};

    bs3d::RenderFrameBuilder builder;
    bs3d::addWorldRenderListFallbackBoxes(builder, renderList, definitions);

    const bs3d::RenderFrame frame = builder.build();

    expect(bs3d::isRenderFrameBucketOrderValid(frame),
           "builder extraction preserves production bucket order");
    expect(frame.primitives[0].bucket == bs3d::RenderBucket::Opaque,
           "builder extraction: Opaque before Vehicle before Glass");
    expect(frame.primitives[1].bucket == bs3d::RenderBucket::Vehicle,
           "builder extraction: Vehicle second");
    expect(frame.primitives[2].bucket == bs3d::RenderBucket::Glass,
           "builder extraction: Glass last");
}

void builderExtractionCountsMissingDefinitions() {
    bs3d::WorldObject obj;
    obj.id = "test_opaque";
    obj.assetId = "opaque_1";
    obj.position = {0.0f, 0.0f, 0.0f};

    bs3d::WorldObject objMissing;
    objMissing.id = "test_missing";
    objMissing.assetId = "nonexistent_1";

    bs3d::WorldAssetDefinition opaqueDef;
    opaqueDef.id = "opaque_1";
    opaqueDef.renderBucket = "Opaque";

    std::vector<bs3d::WorldAssetDefinition> definitions = {opaqueDef};

    bs3d::WorldRenderList renderList;
    renderList.opaque = {&obj, &objMissing};

    bs3d::RenderFrameBuilder builder;
    const auto stats = bs3d::addWorldRenderListFallbackBoxes(builder, renderList, definitions);

    expect(stats.skippedMissingDefinition == 1,
           "builder extraction counts missing definition");
    expect(stats.totalCommands == 1,
           "builder extraction only emits one command for the found definition");
    expect(stats.opaqueCommands == 1,
           "builder extraction opaque command comes from found definition");

    const bs3d::RenderFrame frame = builder.build();
    expect(frame.primitives.size() == 1,
           "builder extraction frame has 1 primitive (missing skipped)");
}

void builderExtractionSkipsDebugOnly() {
    bs3d::WorldObject obj;
    obj.id = "test_opaque";
    obj.assetId = "opaque_1";
    obj.position = {0.0f, 0.0f, 0.0f};

    bs3d::WorldObject objDebug;
    objDebug.id = "test_debug_only";
    objDebug.assetId = "debug_1";

    bs3d::WorldAssetDefinition opaqueDef;
    opaqueDef.id = "opaque_1";
    opaqueDef.renderBucket = "Opaque";

    bs3d::WorldAssetDefinition debugDef;
    debugDef.id = "debug_1";
    debugDef.renderBucket = "DebugOnly";

    std::vector<bs3d::WorldAssetDefinition> definitions = {opaqueDef, debugDef};

    bs3d::WorldRenderList renderList;
    renderList.opaque = {&obj, &objDebug};

    bs3d::RenderFrameBuilder builder;
    const auto stats = bs3d::addWorldRenderListFallbackBoxes(builder, renderList, definitions);

    expect(stats.skippedDebugOnly == 1,
           "builder extraction counts DebugOnly skip");
    expect(stats.totalCommands == 1,
           "builder extraction only emits one command for non-DebugOnly");
    expect(stats.opaqueCommands == 1,
           "builder extraction opaque command not DebugOnly");

    const bs3d::RenderFrame frame = builder.build();
    expect(frame.primitives.size() == 1,
           "builder extraction frame has 1 primitive (DebugOnly skipped)");
}

void builderExtractionSupportsDecalGlassTranslucentBuckets() {
    bs3d::WorldObject objOpaque;
    objOpaque.id = "test_opaque";
    objOpaque.assetId = "opaque_1";
    objOpaque.position = {0.0f, 0.0f, 0.0f};

    bs3d::WorldObject objDecal;
    objDecal.id = "test_decal";
    objDecal.assetId = "decal_1";
    objDecal.position = {1.0f, 0.0f, 0.0f};

    bs3d::WorldObject objGlass;
    objGlass.id = "test_glass";
    objGlass.assetId = "glass_1";
    objGlass.position = {2.0f, 0.0f, 0.0f};

    bs3d::WorldObject objTranslucent;
    objTranslucent.id = "test_translucent";
    objTranslucent.assetId = "translucent_1";
    objTranslucent.position = {3.0f, 0.0f, 0.0f};

    bs3d::WorldAssetDefinition opaqueDef;
    opaqueDef.id = "opaque_1";
    opaqueDef.renderBucket = "Opaque";

    bs3d::WorldAssetDefinition decalDef;
    decalDef.id = "decal_1";
    decalDef.renderBucket = "Decal";

    bs3d::WorldAssetDefinition glassDef;
    glassDef.id = "glass_1";
    glassDef.renderBucket = "Glass";

    bs3d::WorldAssetDefinition translucentDef;
    translucentDef.id = "translucent_1";
    translucentDef.renderBucket = "Translucent";

    std::vector<bs3d::WorldAssetDefinition> definitions = {opaqueDef, decalDef, glassDef, translucentDef};

    bs3d::WorldRenderList renderList;
    renderList.transparent = {&objOpaque, &objDecal, &objGlass, &objTranslucent};

    bs3d::RenderFrameBuilder builder;
    const auto stats = bs3d::addWorldRenderListFallbackBoxes(builder, renderList, definitions);

    expect(stats.totalCommands == 4, "builder extraction counts all 4 buckets");
    expect(stats.opaqueCommands == 1, "builder extraction counts opaque");
    expect(stats.decalCommands == 1, "builder extraction counts decal");
    expect(stats.glassCommands == 1, "builder extraction counts glass");
    expect(stats.translucentCommands == 1, "builder extraction counts translucent");

    const bs3d::RenderFrame frame = builder.build();
    expect(frame.primitives.size() == 4, "builder extraction frame has 4 primitives");
    expect(frame.primitives[0].bucket == bs3d::RenderBucket::Opaque, "production order: Opaque first");
    expect(frame.primitives[1].bucket == bs3d::RenderBucket::Decal, "production order: Decal second");
    expect(frame.primitives[2].bucket == bs3d::RenderBucket::Glass, "production order: Glass third");
    expect(frame.primitives[3].bucket == bs3d::RenderBucket::Translucent, "production order: Translucent fourth");
    expect(bs3d::isRenderFrameBucketOrderValid(frame), "multi-bucket extraction has valid order");
}

// ---------- RenderFrameDump tests ----------

void dumpWriteAndReadRoundTrip() {
    bs3d::RenderFrame original;
    original.camera.position = {1.0f, 2.0f, 3.0f};
    original.camera.target = {4.0f, 5.0f, 6.0f};
    original.camera.fovy = 55.0f;

    {
        bs3d::RenderPrimitiveCommand box;
        box.kind = bs3d::RenderPrimitiveKind::Box;
        box.bucket = bs3d::RenderBucket::Opaque;
        box.transform.position = {0.0f, 0.0f, 0.0f};
        box.size = {1.4f, 1.4f, 1.4f};
        box.tint = {255, 180, 60, 255};
        box.sourceId = "test_box_1";
        original.primitives.push_back(box);
    }
    {
        bs3d::RenderPrimitiveCommand box;
        box.kind = bs3d::RenderPrimitiveKind::Box;
        box.bucket = bs3d::RenderBucket::Vehicle;
        box.transform.position = {1.0f, 0.0f, 2.0f};
        box.size = {0.8f, 0.8f, 0.8f};
        box.tint = {100, 200, 100, 255};
        box.sourceId = "test_box_2";
        original.primitives.push_back(box);
    }

    original.debugLines.push_back({{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {255, 0, 0, 255}});
    original.debugLines.push_back({{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0, 255, 0, 255}});

    const std::string dumpPath = "artifacts/test_dump_roundtrip.txt";
    std::string error;
    expect(bs3d::writeRenderFrameDump(original, dumpPath, &error),
           "writeRenderFrameDump succeeds");
    expect(error.empty(), "writeRenderFrameDump has no error");

    bs3d::RenderFrame loaded;
    expect(bs3d::readRenderFrameDump(dumpPath, loaded, &error),
           "readRenderFrameDump succeeds");
    expect(error.empty(), "readRenderFrameDump has no error");

    expect(loaded.primitives.size() == 2, "dump round-trip preserves 2 primitives");
    expect(loaded.debugLines.size() == 2, "dump round-trip preserves 2 debug lines");

    expectNear(loaded.camera.position.x, 1.0f, 0.001f, "dump round-trip camera pos x");
    expectNear(loaded.camera.fovy, 55.0f, 0.001f, "dump round-trip camera fovy");

    expect(loaded.primitives[0].kind == bs3d::RenderPrimitiveKind::Box,
           "dump round-trip primitive 0 is Box");
    expect(loaded.primitives[0].bucket == bs3d::RenderBucket::Opaque,
           "dump round-trip primitive 0 bucket");
    expect(loaded.primitives[0].sourceId == "test_box_1",
           "dump round-trip primitive 0 sourceId");
    expect(loaded.primitives[0].tint.r == 255 && loaded.primitives[0].tint.g == 180,
           "dump round-trip primitive 0 tint");
    expect(loaded.primitives[1].bucket == bs3d::RenderBucket::Vehicle,
           "dump round-trip primitive 1 bucket");

    expect(loaded.debugLines[0].tint.r == 255, "dump round-trip debugline 0 tint red");
    expect(loaded.debugLines[1].tint.g == 255, "dump round-trip debugline 1 tint green");

    const auto validation = bs3d::validateRenderFrame(loaded);
    expect(validation.valid, "dump round-trip frame validates");

    std::remove(dumpPath.c_str());
}

// ---------- NullRenderer tests ----------

void nullRendererConsumesEmptyRenderFrame() {
    bs3d::NullRenderer renderer;
    const bs3d::RenderFrame frame;

    renderer.renderFrame(frame);

    expect(std::string(renderer.backendName()) == "null", "NullRenderer exposes backend name 'null'");
    expect(renderer.renderCalls() == 1, "NullRenderer records one render call");
    expect(renderer.lastStats().totalPrimitives == 0, "NullRenderer records zero primitives for empty frame");
    expect(renderer.lastStats().debugLines == 0, "NullRenderer records zero debug lines for empty frame");
    expect(renderer.lastFrameValid(), "NullRenderer validates empty frame as valid");
}

void nullRendererConsumesBuilderOutput() {
    bs3d::RenderFrameBuilder builder;
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Sky, "sky"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Opaque, "opaque"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Hud, "hud"));
    builder.addDebugLine({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {255, 255, 255, 255});

    bs3d::NullRenderer renderer;
    renderer.renderFrame(builder.build());

    expect(renderer.renderCalls() == 1, "NullRenderer consumes builder output");
    expect(renderer.lastStats().totalPrimitives == 3, "NullRenderer records 3 primitives from builder");
    expect(renderer.lastStats().debugLines == 1, "NullRenderer records 1 debug line from builder");
    expect(renderer.lastStats().sky == 1, "NullRenderer records sky bucket from builder");
    expect(renderer.lastStats().opaque == 1, "NullRenderer records opaque bucket from builder");
    expect(renderer.lastStats().hud == 1, "NullRenderer records hud bucket from builder");
}

void nullRendererRecordsRenderFrameStats() {
    bs3d::NullRenderer renderer;
    bs3d::RenderFrame frame;
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Opaque});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Sphere, bs3d::RenderBucket::Vehicle});
    bs3d::addDebugLine(frame, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {255, 255, 255, 255});

    renderer.renderFrame(frame);

    expect(renderer.lastStats().totalPrimitives == 2, "NullRenderer stats counts 2 primitives");
    expect(renderer.lastStats().debugLines == 1, "NullRenderer stats counts 1 debug line");
    expect(renderer.lastStats().opaque == 1, "NullRenderer stats counts opaque");
    expect(renderer.lastStats().vehicle == 1, "NullRenderer stats counts vehicle");
}

void nullRendererRecordsValidationSuccessForValidFrames() {
    bs3d::RenderFrameBuilder builder;
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Hud));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Sky));

    bs3d::NullRenderer renderer;
    renderer.renderFrame(builder.build());

    expect(renderer.lastFrameValid(), "NullRenderer records validation success for valid builder frame");
    expect(renderer.lastValidation().valid, "NullRenderer lastValidation reports valid");
    expect(renderer.lastValidation().message.empty(), "NullRenderer lastValidation has no error for valid frame");
}

void nullRendererRecordsValidationFailureForInvalidBucketOrder() {
    bs3d::RenderFrame frame;
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Hud});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Sky});

    bs3d::NullRenderer renderer;
    renderer.renderFrame(frame);

    expect(!renderer.lastFrameValid(), "NullRenderer records validation failure for invalid bucket order");
    expect(!renderer.lastValidation().valid, "NullRenderer lastValidation reports invalid");
    expect(renderer.lastValidation().message.find("Sky") != std::string::npos,
           "NullRenderer validation message names the out-of-order bucket");
}

#if defined(BS3D_HAS_D3D11_RENDERER)
void d3d11RendererImplementsIRenderer() {
    static_assert(std::is_base_of<bs3d::IRenderer, bs3d::D3D11Renderer>::value,
                  "D3D11Renderer must implement IRenderer");

    bs3d::D3D11Renderer renderer;
    bs3d::IRenderer* interfaceRenderer = &renderer;

    expect(std::string(interfaceRenderer->backendName()) == "d3d11",
           "D3D11Renderer exposes backend name 'd3d11' through IRenderer");
}

void d3d11RendererConsumesEmptyRenderFrame() {
    bs3d::D3D11Renderer renderer;
    const bs3d::RenderFrame frame;

    renderer.renderFrame(frame);

    expect(!renderer.isInitialized(), "D3D11Renderer starts uninitialized in lightweight tests");
    expect(renderer.renderCalls() == 1, "D3D11Renderer records one render call");
    expect(renderer.lastStats().totalPrimitives == 0, "D3D11Renderer records zero primitives for empty frame");
    expect(renderer.lastStats().debugLines == 0, "D3D11Renderer records zero debug lines for empty frame");
    expect(renderer.lastFrameValid(), "D3D11Renderer validates empty frame as valid");
}

void d3d11RendererConsumesBuilderOutput() {
    bs3d::RenderFrameBuilder builder;
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Sky, "sky"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Opaque, "opaque"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Vehicle, "vehicle"));
    builder.addPrimitive(makePrimitive(bs3d::RenderBucket::Hud, "hud"));
    builder.addDebugLine({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {255, 255, 255, 255});

    bs3d::D3D11Renderer renderer;
    renderer.renderFrame(builder.build());

    expect(renderer.renderCalls() == 1, "D3D11Renderer consumes builder output");
    expect(renderer.lastStats().totalPrimitives == 4, "D3D11Renderer records 4 primitives from builder");
    expect(renderer.lastStats().debugLines == 1, "D3D11Renderer records 1 debug line from builder");
    expect(renderer.lastStats().sky == 1, "D3D11Renderer records sky bucket from builder");
    expect(renderer.lastStats().opaque == 1, "D3D11Renderer records opaque bucket from builder");
    expect(renderer.lastStats().vehicle == 1, "D3D11Renderer records vehicle bucket from builder");
    expect(renderer.lastStats().hud == 1, "D3D11Renderer records hud bucket from builder");
    expect(renderer.lastFrameValid(), "D3D11Renderer validates builder output as valid");
}

void d3d11RendererRecordsDebugLinesWhenUninitialized() {
    bs3d::RenderFrame frame;
    frame.debugLines.push_back({{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {255, 0, 0, 255}});
    frame.debugLines.push_back({{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0, 255, 0, 255}});

    bs3d::D3D11Renderer renderer;
    renderer.renderFrame(frame);

    expect(!renderer.isInitialized(), "D3D11Renderer remains uninitialized for debug line stats test");
    expect(renderer.renderCalls() == 1, "D3D11Renderer records debug line frame render call");
    expect(renderer.lastStats().totalPrimitives == 0, "D3D11Renderer records zero primitives for debug line frame");
    expect(renderer.lastStats().debugLines == 2, "D3D11Renderer records debug line count while uninitialized");
    expect(renderer.lastFrameValid(), "D3D11Renderer validates debug-line-only frame as valid");
}

void d3d11RendererRecordsValidationFailureForInvalidBucketOrder() {
    bs3d::RenderFrame frame;
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Hud});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Sky});

    bs3d::D3D11Renderer renderer;
    renderer.renderFrame(frame);

    expect(renderer.renderCalls() == 1, "D3D11Renderer records invalid frame render call");
    expect(renderer.lastStats().totalPrimitives == 2, "D3D11Renderer records invalid frame primitive stats");
    expect(!renderer.lastFrameValid(), "D3D11Renderer records validation failure for invalid bucket order");
    expect(!renderer.lastValidation().valid, "D3D11Renderer lastValidation reports invalid");
    expect(renderer.lastValidation().message.find("Sky") != std::string::npos,
           "D3D11Renderer validation message names the out-of-order bucket");
}

void d3d11RendererRejectsInvalidInitConfigWithoutGpu() {
    bs3d::D3D11Renderer renderer;
    bs3d::D3D11RendererConfig config;
    config.width = 640;
    config.height = 360;

    std::string error;
    expect(!renderer.initialize(config, &error), "D3D11Renderer rejects missing HWND");
    expect(!renderer.isInitialized(), "D3D11Renderer remains uninitialized after invalid config");
    expect(error.find("HWND") != std::string::npos, "D3D11Renderer invalid config reports HWND error");

    renderer.shutdown();
    expect(!renderer.isInitialized(), "D3D11Renderer shutdown is safe when uninitialized");
}

void d3d11RendererRecordsStatsWithNonDefaultCameraWhenUninitialized() {
    bs3d::RenderFrame frame;
    frame.camera.position = {0.0f, 2.0f, -5.0f};
    frame.camera.target = {0.0f, 0.0f, 0.0f};
    frame.camera.up = {0.0f, 1.0f, 0.0f};
    frame.camera.fovy = 60.0f;
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Opaque});
    frame.debugLines.push_back({{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {255, 0, 0, 255}});

    bs3d::D3D11Renderer renderer;
    renderer.renderFrame(frame);

    expect(!renderer.isInitialized(), "D3D11Renderer remains uninitialized for camera stats test");
    expect(renderer.renderCalls() == 1, "D3D11Renderer records render call with non-default camera");
    expect(renderer.lastStats().totalPrimitives == 1,
           "D3D11Renderer records primitive count with non-default camera");
    expect(renderer.lastStats().debugLines == 1,
           "D3D11Renderer records debug line count with non-default camera");
    expect(renderer.lastStats().opaque == 1,
           "D3D11Renderer records opaque bucket with non-default camera");
    expect(renderer.lastFrameValid(), "D3D11Renderer validates frame with non-default camera as valid");
}

void d3d11RendererRecordsExpectedStatsForBuilderFrameWhenUninitialized() {
    bs3d::RenderFrameBuilder builder;

    {
        bs3d::RenderPrimitiveCommand opaqueBox = makePrimitive(bs3d::RenderBucket::Opaque, "smoke_opaque");
        opaqueBox.kind = bs3d::RenderPrimitiveKind::Box;
        opaqueBox.size = {1.4f, 1.4f, 1.4f};
        builder.addPrimitive(opaqueBox);
    }
    {
        bs3d::RenderPrimitiveCommand vehicleBox = makePrimitive(bs3d::RenderBucket::Vehicle, "smoke_vehicle");
        vehicleBox.kind = bs3d::RenderPrimitiveKind::Box;
        vehicleBox.size = {1.2f, 1.2f, 1.2f};
        builder.addPrimitive(vehicleBox);
    }

    builder.addDebugLine({-2.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {255, 40, 40, 255});
    builder.addDebugLine({0.0f, -2.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, {40, 255, 80, 255});
    builder.addDebugLine({0.0f, 0.0f, -2.0f}, {0.0f, 0.0f, 2.0f}, {80, 140, 255, 255});

    bs3d::D3D11Renderer renderer;
    renderer.renderFrame(builder.build());

    expect(!renderer.isInitialized(), "D3D11Renderer remains uninitialized for builder frame stats test");
    expect(renderer.renderCalls() == 1, "D3D11Renderer records render call for builder frame");
    expect(renderer.lastStats().totalPrimitives == 2,
           "D3D11Renderer records 2 primitives for builder frame");
    expect(renderer.lastStats().debugLines == 3,
           "D3D11Renderer records 3 debug lines for builder frame");
    expect(renderer.lastStats().opaque == 1,
           "D3D11Renderer records opaque bucket from builder frame");
    expect(renderer.lastStats().vehicle == 1,
           "D3D11Renderer records vehicle bucket from builder frame");
    expect(renderer.lastFrameValid(),
           "D3D11Renderer validates builder smoke frame as valid");
}

void d3d11RendererRecordsExpectedStatsForExtractionFrameWhenUninitialized() {
    bs3d::WorldObject obj0;
    obj0.id = "test_opaque";
    obj0.assetId = "opaque_1";
    obj0.position = {0.0f, 0.0f, 0.0f};

    bs3d::WorldObject obj1;
    obj1.id = "test_vehicle";
    obj1.assetId = "vehicle_1";
    obj1.position = {2.0f, 0.0f, 1.0f};

    bs3d::WorldAssetDefinition opaqueDef;
    opaqueDef.id = "opaque_1";
    opaqueDef.fallbackSize = {1.0f, 1.0f, 1.0f};
    opaqueDef.renderBucket = "Opaque";

    bs3d::WorldAssetDefinition vehicleDef;
    vehicleDef.id = "vehicle_1";
    vehicleDef.fallbackSize = {1.0f, 1.0f, 1.0f};
    vehicleDef.renderBucket = "Vehicle";

    std::vector<bs3d::WorldAssetDefinition> definitions = {opaqueDef, vehicleDef};

    bs3d::WorldRenderList renderList;
    renderList.opaque = {&obj0};
    renderList.transparent = {&obj1};

    bs3d::RenderFrameBuilder builder;
    bs3d::addWorldRenderListFallbackBoxes(builder, renderList, definitions);
    builder.addDebugLine({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {255, 0, 0, 255});

    bs3d::D3D11Renderer renderer;
    renderer.renderFrame(builder.build());

    expect(!renderer.isInitialized(), "D3D11Renderer remains uninitialized for extraction frame stats test");
    expect(renderer.renderCalls() == 1, "D3D11Renderer records render call for extraction frame");
    expect(renderer.lastStats().totalPrimitives == 2,
           "D3D11Renderer records 2 primitives from extraction frame");
    expect(renderer.lastStats().debugLines == 1,
           "D3D11Renderer records 1 debug line from extraction frame");
    expect(renderer.lastStats().opaque == 1,
           "D3D11Renderer records opaque bucket from extraction frame");
    expect(renderer.lastStats().vehicle == 1,
           "D3D11Renderer records vehicle bucket from extraction frame");
    expect(renderer.lastFrameValid(),
           "D3D11Renderer validates extraction frame as valid");
}

void d3d11RendererRecordsStatsForDecalGlassTranslucentWhenUninitialized() {
    bs3d::RenderFrame frame;
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Opaque});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Decal});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Glass});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Translucent});

    bs3d::D3D11Renderer renderer;
    renderer.renderFrame(frame);

    expect(!renderer.isInitialized(), "D3D11Renderer remains uninitialized for new buckets test");
    expect(renderer.renderCalls() == 1, "D3D11Renderer records render call");
    expect(renderer.lastStats().totalPrimitives == 4,
           "D3D11Renderer records all 4 primitives (Opaque, Decal, Glass, Translucent)");
    expect(renderer.lastStats().opaque == 1,
           "D3D11Renderer records opaque bucket");
    expect(renderer.lastStats().decal == 1,
           "D3D11Renderer records decal bucket");
    expect(renderer.lastStats().glass == 1,
           "D3D11Renderer records glass bucket");
    expect(renderer.lastStats().translucent == 1,
           "D3D11Renderer records translucent bucket");
    expect(renderer.lastFrameValid(),
           "D3D11Renderer validates multi-bucket frame as valid");
}

void d3d11RendererDrawCoverageIsZeroWhenUninitialized() {
    bs3d::RenderFrame frame;
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Opaque});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Opaque});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Sphere, bs3d::RenderBucket::Opaque});
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Sky});
    frame.debugLines.push_back({{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {255, 0, 0, 255}});

    bs3d::D3D11Renderer renderer;
    renderer.renderFrame(frame);

    expect(!renderer.isInitialized(), "D3D11Renderer remains uninitialized");
    const auto& d3d11Stats = renderer.lastD3D11Stats();
    expect(d3d11Stats.drawnBoxes == 0, "uninitialized D3D11Renderer draws 0 boxes");
    expect(d3d11Stats.drawnDebugLines == 0, "uninitialized D3D11Renderer draws 0 debug lines");
    expect(d3d11Stats.skippedUnsupportedKinds == 0,
           "uninitialized D3D11Renderer does not iterate primitives for skip counting");
    expect(d3d11Stats.skippedPrimitives == 0,
           "uninitialized D3D11Renderer skips counting when not initialized");
}
#endif

// ---------- RendererFactory tests ----------

void factoryCreatesNullRenderer() {
    bs3d::RendererFactoryRequest request;
    request.useNullRenderer = true;

    bs3d::RendererFactoryResult result = bs3d::createRenderer(request);

    expect(result.ok(), "factory returns ok for NullRenderer request");
    expect(result.renderer != nullptr, "factory returns non-null renderer");
    expect(result.error.empty(), "factory returns no error for NullRenderer request");
    expect(std::string(result.renderer->backendName()) == "null",
           "factory-created renderer has backend name 'null'");
}

void factoryReturnsErrorForRaylibBackend() {
    bs3d::RendererFactoryRequest request;
    request.backend = bs3d::RendererBackendKind::Raylib;
    request.useNullRenderer = false;

    bs3d::RendererFactoryResult result = bs3d::createRenderer(request);

    expect(!result.ok(), "factory returns not-ok for Raylib without null override");
    expect(result.renderer == nullptr, "factory returns null renderer for Raylib request");
    expect(!result.error.empty(), "factory returns error message for Raylib request");
    expect(result.error.find("not implemented") != std::string::npos,
           "factory error mentions 'not implemented'");
    expect(result.error.find("WorldRenderer") != std::string::npos
           || result.error.find("legacy") != std::string::npos,
           "factory error references legacy runtime path");
}

void factoryDoesNotPretendRaylibAdapterExists() {
    bs3d::RendererFactoryRequest request;
    request.backend = bs3d::RendererBackendKind::Raylib;
    request.useNullRenderer = false;

    bs3d::RendererFactoryResult result = bs3d::createRenderer(request);

    expect(result.renderer == nullptr, "factory does not return a fake Raylib IRenderer adapter");
    expect(!result.ok(), "factory does not pretend Raylib IRenderer adapter is available");
}

void factoryRejectsD3D11WithoutOptIn() {
    bs3d::RendererFactoryRequest request;
    request.backend = bs3d::RendererBackendKind::D3D11;
    request.useNullRenderer = false;
    request.allowExperimentalD3D11Renderer = false;

    bs3d::RendererFactoryResult result = bs3d::createRenderer(request);

    expect(!result.ok(), "factory returns not-ok for D3D11 without opt-in");
    expect(result.renderer == nullptr, "factory returns null renderer for D3D11 without opt-in");
    expect(!result.error.empty(), "factory returns error message for D3D11 without opt-in");
    expect(result.error.find("experimental") != std::string::npos,
           "factory error mentions 'experimental' for D3D11 without opt-in");
}

void factoryD3D11BackendNameIsCorrect() {
    const std::string name(bs3d::rendererBackendName(bs3d::RendererBackendKind::D3D11));
    expect(name == "d3d11", "rendererBackendName(D3D11) returns 'd3d11'");
}

#if defined(BS3D_HAS_D3D11_RENDERER)
void factoryCreatesUninitializedD3D11RendererWithOptIn() {
    bs3d::RendererFactoryRequest request;
    request.backend = bs3d::RendererBackendKind::D3D11;
    request.useNullRenderer = false;
    request.allowExperimentalD3D11Renderer = true;

    bs3d::RendererFactoryResult result = bs3d::createRenderer(request);

    expect(result.ok(), "factory returns ok for D3D11 with opt-in on Windows");
    expect(result.renderer != nullptr, "factory returns non-null renderer for D3D11 with opt-in");
    expect(result.error.empty(), "factory returns no error for D3D11 with opt-in");
    expect(std::string(result.renderer->backendName()) == "d3d11",
           "factory-created D3D11 renderer has backend name 'd3d11'");

    bs3d::RenderFrame frame;
    frame.primitives.push_back({bs3d::RenderPrimitiveKind::Box, bs3d::RenderBucket::Opaque});
    frame.debugLines.push_back({{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {255, 0, 0, 255}});
    result.renderer->renderFrame(frame);

    expect(true, "factory-created D3D11 renderer renders frame without initialize (stats-only)");
}
#endif

} // namespace

int main() {
    emptyRenderFrameHasNoCommands();
    fallbackBoxAddsOnePrimitiveCommand();
    fallbackBoxUsesTintOverride();
    debugLineAddsOneLineCommand();
    bucketAndKindValuesAreStable();
    emptyWorldRenderListExtractionProducesNoCommands();
    worldRenderListExtractionCountsBuckets();
    worldRenderListExtractionOrdersBuckets();
    missingDefinitionsAreSkippedAndCounted();
    debugOnlyDefinitionsAreSkippedAndCounted();
    summarizeRenderFrameCountsEmptyFrame();
    summarizeRenderFrameCountsBucketsAndDebugLines();
    extractedWorldRenderListFrameHasValidBucketOrder();
    invalidBucketOrderIsRejected();
    emptyRenderFrameValidationSucceeds();
    recordingRendererConsumesEmptyRenderFrame();
    recordingRendererConsumesExtractedWorldRenderListFrame();
    builderEmptyBuildIsValid();
    builderStoresCameraAndWorldStyle();
    builderReordersPrimitivesIntoProductionOrder();
    builderPreservesInsertionOrderInsideBucket();
    builderIncludesDebugLines();
    builderStatsMatchSummarize();
    builderValidateSucceedsForBuilderOutput();
    builderDoesNotRequireBackendTypes();
    builderDebugLinesDoNotAffectPrimitiveOrdering();
    builderFrameWithMultipleBoxesAndDebugLinesValidates();
    builderExtractionEmitsExpectedPrimitiveCount();
    builderExtractionPreservesBucketOrder();
    builderExtractionCountsMissingDefinitions();
    builderExtractionSkipsDebugOnly();
    builderExtractionSupportsDecalGlassTranslucentBuckets();
    dumpWriteAndReadRoundTrip();
    nullRendererConsumesEmptyRenderFrame();
    nullRendererConsumesBuilderOutput();
    nullRendererRecordsRenderFrameStats();
    nullRendererRecordsValidationSuccessForValidFrames();
    nullRendererRecordsValidationFailureForInvalidBucketOrder();
#if defined(BS3D_HAS_D3D11_RENDERER)
    d3d11RendererImplementsIRenderer();
    d3d11RendererConsumesEmptyRenderFrame();
    d3d11RendererConsumesBuilderOutput();
    d3d11RendererRecordsDebugLinesWhenUninitialized();
    d3d11RendererRecordsValidationFailureForInvalidBucketOrder();
    d3d11RendererRejectsInvalidInitConfigWithoutGpu();
    d3d11RendererRecordsStatsWithNonDefaultCameraWhenUninitialized();
    d3d11RendererRecordsExpectedStatsForBuilderFrameWhenUninitialized();
    d3d11RendererRecordsExpectedStatsForExtractionFrameWhenUninitialized();
    d3d11RendererRecordsStatsForDecalGlassTranslucentWhenUninitialized();
    d3d11RendererDrawCoverageIsZeroWhenUninitialized();
#endif
    factoryCreatesNullRenderer();
    factoryReturnsErrorForRaylibBackend();
    factoryDoesNotPretendRaylibAdapterExists();
    factoryRejectsD3D11WithoutOptIn();
    factoryD3D11BackendNameIsCorrect();
#if defined(BS3D_HAS_D3D11_RENDERER)
    factoryCreatesUninitializedD3D11RendererWithOptIn();
#endif
    std::cout << "render frame tests passed\n";
    return 0;
}
