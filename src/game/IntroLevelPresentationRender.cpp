#include "IntroLevelPresentation.h"

#include "VisualBaselineDebug.h"

#include "raylib.h"

#include <cmath>

namespace bs3d {

namespace {

RenderColor color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
    return {r, g, b, a};
}

Color toRayColor(RenderColor tint) {
    return {tint.r, tint.g, tint.b, tint.a};
}

Vector3 toRay(Vec3 value) {
    return {value.x, value.y, value.z};
}

RenderColor districtDebugColor(const DistrictPlanDebugRewir& marker) {
    if (marker.playableNow && marker.homeBase) {
        return color(80, 190, 255, 120);
    }
    if (marker.drivingSpine) {
        return color(230, 88, 70, 118);
    }
    if (marker.vehicleIdentity) {
        return color(241, 146, 66, 118);
    }
    if (marker.serviceEconomy) {
        return color(75, 220, 105, 118);
    }
    return color(214, 217, 210, 100);
}

const IntroLevelData& levelOrFallback(const IntroObjectiveContext& context) {
    static const IntroLevelData fallback = IntroLevelBuilder::build();
    return context.level != nullptr ? *context.level : fallback;
}

} // namespace

void IntroLevelPresentation::drawMarkers(const IntroObjectiveContext& context, float hudPulse, float elapsedSeconds) {
    const IntroLevelData& level = levelOrFallback(context);
    const float pulse = 1.0f + hudPulse * 0.18f + std::sin(elapsedSeconds * 4.0f) * 0.04f;
    const ObjectiveMarker objective = currentObjectiveMarker(context);

    WorldRenderer::drawMarker(level.npcPosition,
                              activeMarkerFor(context, level.npcPosition) ? 0.92f * pulse : 0.38f,
                              color(238, 211, 79, activeMarkerFor(context, level.npcPosition) ? 135 : 62));
    if (context.phase == MissionPhase::ReachVehicle) {
        WorldRenderer::drawMarker(context.vehiclePosition, 1.25f * pulse, color(221, 82, 66, 155));
    }
    WorldRenderer::drawMarker(level.shopEntrancePosition,
                              activeMarkerFor(context, level.shopEntrancePosition) ? 1.25f * pulse : 0.46f,
                              color(75, 220, 105, activeMarkerFor(context, level.shopEntrancePosition) ? 150 : 60));
    WorldRenderer::drawMarker(level.dropoffPosition,
                              activeMarkerFor(context, level.dropoffPosition) ? 1.10f * pulse : 0.42f,
                              color(77, 150, 225, activeMarkerFor(context, level.dropoffPosition) ? 130 : 56));

    CharacterRenderOptions npcOptions;
    npcOptions.palette = &characterVisualProfileOrDefault("bogus").palette;
    WorldRenderer::drawPlayer(level.npcPosition, 0.35f, PlayerPresentationState{}, elapsedSeconds, npcOptions);
    WorldRenderer::drawObjectiveBeacon(objective, pulse, elapsedSeconds);
}

void IntroLevelPresentation::drawDistrictPlanDebug(const IntroLevelData& level, float elapsedSeconds) {
    const DistrictPlanDebugOverlay overlay = buildDistrictPlanDebugOverlay(level);
    const float pulse = 1.0f + std::sin(elapsedSeconds * 2.7f) * 0.035f;

    BeginBlendMode(BLEND_ALPHA);
    for (const DistrictPlanDebugRouteSegment& segment : overlay.routeSegments) {
        const Color tint = segment.vehicleRoute ? toRayColor(color(238, 211, 79, 150))
                                                : toRayColor(color(214, 217, 210, 105));
        const Vec3 from = segment.from + Vec3{0.0f, 0.18f, 0.0f};
        const Vec3 to = segment.to + Vec3{0.0f, 0.18f, 0.0f};
        DrawLine3D(toRay(from), toRay(to), tint);
        DrawSphere(toRay(from), 0.10f, Fade(tint, 0.85f));
        DrawSphere(toRay(to), 0.12f, Fade(tint, 0.85f));
    }

    for (const DistrictPlanDebugRewir& marker : overlay.rewirMarkers) {
        const Color tint = toRayColor(districtDebugColor(marker));
        const float radius = marker.radius * pulse;
        DrawCylinderWires(toRay(marker.center + Vec3{0.0f, 0.11f, 0.0f}), radius, radius, 0.08f, 56, tint);
        DrawCylinderWires(toRay(marker.center + Vec3{0.0f, 0.17f, 0.0f}), radius * 0.72f, radius * 0.72f, 0.06f, 48,
                          Fade(tint, 0.72f));
        DrawSphereWires(toRay(marker.center + Vec3{0.0f, marker.playableNow ? 2.35f : 1.75f, 0.0f}),
                        marker.playableNow ? 0.30f : 0.22f,
                        12,
                        8,
                        Fade(tint, 0.92f));
    }
    EndBlendMode();
}

} // namespace bs3d
