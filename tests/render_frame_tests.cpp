#include "RenderExtraction.h"
#include "bs3d/render/IRenderer.h"
#include "bs3d/render/RenderFrameValidation.h"
#include "bs3d/render/WorldRenderList.h"

#include "bs3d/render/RenderFrame.h"
#include "bs3d/render/RenderTypes.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

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
    std::cout << "render frame tests passed\n";
    return 0;
}
