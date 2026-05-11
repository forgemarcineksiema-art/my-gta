#include "RenderExtraction.h"

#include "bs3d/render/RenderFrame.h"
#include "bs3d/render/RenderTypes.h"

#include <cstdlib>
#include <iostream>
#include <string>

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

} // namespace

int main() {
    emptyRenderFrameHasNoCommands();
    fallbackBoxAddsOnePrimitiveCommand();
    fallbackBoxUsesTintOverride();
    debugLineAddsOneLineCommand();
    bucketAndKindValuesAreStable();
    std::cout << "render frame tests passed\n";
    return 0;
}
