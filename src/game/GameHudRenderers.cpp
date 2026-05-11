#include "GameRenderers.h"
#include "DevTools.h"
#include "GameHudLayout.h"
#include "GameHudPainter.h"
#include "GameHudTextMetrics.h"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

namespace bs3d {

namespace {

Vector3 toRay(Vec3 value) {
    return {value.x, value.y, value.z};
}

float radiansToDegrees(float radians) {
    return radians * 180.0f / Pi;
}

HudColor color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
    return {r, g, b, a};
}

Color toRayColor(RenderColor tint) {
    return {tint.r, tint.g, tint.b, tint.a};
}

Camera3D toRayCamera(RenderCamera camera) {
    Camera3D rayCamera{};
    rayCamera.position = toRay(camera.position);
    rayCamera.target = toRay(camera.target);
    rayCamera.up = toRay(camera.up);
    rayCamera.fovy = camera.fovy;
    rayCamera.projection = camera.projection;
    return rayCamera;
}

HudColor white() {
    return hudColor(255, 255, 255);
}

HudColor black() {
    return hudColor(0, 0, 0);
}

HudColor przypalColor(PrzypalBand band) {
    switch (band) {
    case PrzypalBand::Calm:
        return hudColor(105, 210, 120);
    case PrzypalBand::Noticed:
        return hudColor(238, 211, 79);
    case PrzypalBand::Hot:
        return hudColor(241, 146, 66);
    case PrzypalBand::ChaseRisk:
        return hudColor(230, 88, 70);
    case PrzypalBand::Meltdown:
        return hudColor(205, 45, 62);
    }

    return white();
}

std::vector<int> latinUiCodepoints() {
    std::vector<int> codepoints;
    for (int codepoint = 32; codepoint <= 126; ++codepoint) {
        codepoints.push_back(codepoint);
    }
    const int polish[] = {0x0104, 0x0105, 0x0106, 0x0107, 0x0118, 0x0119, 0x0141, 0x0142,
                          0x0143, 0x0144, 0x00D3, 0x00F3, 0x015A, 0x015B, 0x0179, 0x017A,
                          0x017B, 0x017C};
    codepoints.insert(codepoints.end(), polish, polish + sizeof(polish) / sizeof(polish[0]));
    return codepoints;
}

Font uiFont() {
    static Font font = []() {
        std::vector<int> codepoints = latinUiCodepoints();
        const char* candidates[] = {
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
        };
        for (const char* path : candidates) {
            if (!std::filesystem::exists(path)) {
                continue;
            }
            Font loaded = LoadFontEx(path, 48, codepoints.data(), static_cast<int>(codepoints.size()));
            if (loaded.texture.id != 0) {
                SetTextureFilter(loaded.texture, TEXTURE_FILTER_BILINEAR);
                return loaded;
            }
        }
        return GetFontDefault();
    }();
    return font;
}

int measureUiText(const char* text, int fontSize) {
    return static_cast<int>(std::ceil(MeasureTextEx(uiFont(), text, static_cast<float>(fontSize), 1.0f).x));
}

class RaylibHudTextMetrics final : public HudTextMetrics {
public:
    int measureTextWidth(const std::string& text, int fontSize) const override {
        return measureUiText(text.c_str(), fontSize);
    }
};

const HudTextMetrics& hudTextMetrics() {
    static RaylibHudTextMetrics metrics;
    return metrics;
}

class RaylibHudProjection final : public HudProjection {
public:
    HudScreenPoint projectWorldToScreen(Vec3 worldPosition, RenderCamera camera) const override {
        const Camera3D rayCamera = toRayCamera(camera);
        const Vector2 result = GetWorldToScreen(toRay(worldPosition), rayCamera);
        return {result.x, result.y};
    }
};

const HudProjection& hudProjection() {
    static RaylibHudProjection projection;
    return projection;
}

class RaylibHudDrawSink final : public HudDrawSink {
public:
    void fillRect(int x, int y, int width, int height, HudColor tint) override {
        DrawRectangle(x, y, width, height, toRayColor(tint));
    }

    void strokeRect(int x, int y, int width, int height, HudColor tint) override {
        DrawRectangleLines(x, y, width, height, toRayColor(tint));
    }

    void text(const std::string& value, int x, int y, int fontSize, HudColor tint) override {
        DrawTextEx(uiFont(),
                   value.c_str(),
                   {static_cast<float>(x), static_cast<float>(y)},
                   static_cast<float>(fontSize),
                   1.0f,
                   toRayColor(tint));
    }

    void circle(HudPoint center, float radius, HudColor tint) override {
        DrawCircleV({center.x, center.y}, radius, toRayColor(tint));
    }

    void circleLines(int x, int y, float radius, HudColor tint) override {
        DrawCircleLines(x, y, radius, toRayColor(tint));
    }

    void triangle(HudPoint a, HudPoint b, HudPoint c, HudColor tint) override {
        DrawTriangle({a.x, a.y}, {b.x, b.y}, {c.x, c.y}, toRayColor(tint));
    }
};

HudDrawSink& hudDrawSink() {
    static RaylibHudDrawSink sink;
    return sink;
}

void drawUiText(HudDrawSink& sink, const std::string& text, int x, int y, int fontSize, HudColor tint) {
    sink.text(text, x, y, fontSize, tint);
}

void drawUiText(const std::string& text, int x, int y, int fontSize, HudColor tint) {
    drawUiText(hudDrawSink(), text, x, y, fontSize, tint);
}

} // namespace

std::vector<std::string> HudRenderer::wrapTextToWidth(const std::string& text, int fontSize, int maxWidth) {
    return wrapHudTextToWidth(text, fontSize, maxWidth, hudTextMetrics());
}

void HudRenderer::drawDialogueBox(const DialogueSystem& dialogue, int screenWidth, int screenHeight) {
    if (!dialogue.hasLine()) {
        return;
    }

    HudDrawSink& sink = hudDrawSink();
    const DialogueLine& line = dialogue.currentLine();
    const HudDialogueBoxLayout layout = dialogueBoxLayout(line, screenWidth, screenHeight, hudTextMetrics());

    sink.fillRect(layout.rect.x, layout.rect.y, layout.rect.width, layout.rect.height, hudFade(black(), 0.70f));
    sink.strokeRect(layout.rect.x, layout.rect.y, layout.rect.width, layout.rect.height, hudColor(238, 211, 79));
    drawUiText(sink, line.speaker, layout.speakerX, layout.speakerY, 22, hudColor(238, 211, 79));
    for (std::size_t i = 0; i < layout.lines.size(); ++i) {
        drawUiText(sink, layout.lines[i], layout.lineX, layout.firstLineY + static_cast<int>(i) * layout.lineStep, 23, white());
    }
}

void HudRenderer::drawObjectiveCompass(const HudSnapshot& snapshot) {
    if (!snapshot.objectiveMarker.active) {
        return;
    }

    HudDrawSink& sink = hudDrawSink();
    const ObjectiveMarker& marker = snapshot.objectiveMarker;
    const HudColor markerColor = marker.color;
    const HudObjectiveCompassLayout compassLayout = objectiveCompassLayout(snapshot, hudProjection());

    if (compassLayout.offscreen) {
        const HudPoint center{static_cast<float>(snapshot.screenWidth) * 0.5f,
                              static_cast<float>(snapshot.screenHeight) * 0.5f};
        const float angle = std::atan2(compassLayout.clamped.y - center.y, compassLayout.clamped.x - center.x);
        const HudPoint tip{compassLayout.clamped.x + std::cos(angle) * 16.0f,
                           compassLayout.clamped.y + std::sin(angle) * 16.0f};
        const HudPoint left{compassLayout.clamped.x + std::cos(angle + 2.45f) * 14.0f,
                            compassLayout.clamped.y + std::sin(angle + 2.45f) * 14.0f};
        const HudPoint right{compassLayout.clamped.x + std::cos(angle - 2.45f) * 14.0f,
                             compassLayout.clamped.y + std::sin(angle - 2.45f) * 14.0f};
        sink.triangle(tip, left, right, markerColor);
    } else {
        sink.circle({compassLayout.clamped.x, compassLayout.clamped.y},
                    11.0f + snapshot.hudPulse * 4.0f,
                    markerColor);
        sink.circleLines(static_cast<int>(compassLayout.clamped.x),
                         static_cast<int>(compassLayout.clamped.y),
                         16.0f,
                         white());
    }

    const std::string label = marker.label + " " + std::to_string(compassLayout.distanceMeters) + "m";
    const HudObjectiveCompassLabelLayout labelLayout =
        objectiveCompassLabelLayout(snapshot, compassLayout.clamped, label, hudTextMetrics());
    sink.fillRect(labelLayout.rect.x, labelLayout.rect.y, labelLayout.rect.width, labelLayout.rect.height, hudFade(black(), 0.62f));
    sink.strokeRect(labelLayout.rect.x, labelLayout.rect.y, labelLayout.rect.width, labelLayout.rect.height, markerColor);
    drawUiText(sink, label, labelLayout.textX, labelLayout.textY, 18, white());
}

void HudRenderer::drawInteractionPrompt(const InteractionPromptSnapshot& snapshot, int screenWidth, int screenHeight) {
    const HudInteractionPromptLayout layout = interactionPromptLayout(snapshot, screenWidth, screenHeight, hudTextMetrics());
    if (!layout.visible) {
        return;
    }

    HudDrawSink& sink = hudDrawSink();
    sink.fillRect(layout.rect.x, layout.rect.y, layout.rect.width, layout.rect.height, hudFade(black(), 0.68f));
    sink.strokeRect(layout.rect.x, layout.rect.y, layout.rect.width, layout.rect.height, hudColor(238, 211, 79));
    for (std::size_t i = 0; i < snapshot.lines.size(); ++i) {
        drawUiText(sink,
                   snapshot.lines[i],
                   layout.lineX,
                   layout.firstLineY + static_cast<int>(i) * layout.lineStep,
                   22,
                   white());
    }
}

void HudRenderer::drawVehicleHud(const VehicleHudSnapshot& snapshot, int screenWidth) {
    if (!snapshot.visible) {
        return;
    }

    const HudPanelRect rect = vehicleHudPanelRect(snapshot, screenWidth);
    const int y = rect.y;
    const int width = rect.width;
    const int x = rect.x;
    const float conditionRatio = std::clamp(snapshot.condition / 100.0f, 0.0f, 1.0f);
    const HudColor conditionColor = conditionRatio > 0.65f ? hudColor(100, 218, 117)
                                  : conditionRatio > 0.35f ? hudColor(238, 211, 79)
                                                           : hudColor(230, 85, 70);
    HudDrawSink& sink = hudDrawSink();

    sink.fillRect(x, y, width, 82, hudFade(black(), 0.58f));
    sink.strokeRect(x, y, width, 82, hudColor(238, 211, 79));
    drawUiText(sink, TextFormat("Gruz: %s", conditionBandName(snapshot.conditionBand)), x + 12, y + 8, 18, white());
    sink.fillRect(x + 12, y + 34, 148, 12, hudFade(white(), 0.16f));
    sink.fillRect(x + 12, y + 34, static_cast<int>(148.0f * conditionRatio), 12, conditionColor);
    sink.strokeRect(x + 12, y + 34, 148, 12, white());

    drawUiText(sink, TextFormat("Stan %.0f%%", snapshot.condition), x + std::min(170, width - 92), y + 31, 16, conditionColor);
    drawUiText(sink, TextFormat("Prędkość %.0f km/h", std::fabs(snapshot.speed) * 3.6f), x + 12, y + 55, 16, white());
}

void HudRenderer::drawPrzypalHud(const PrzypalHudSnapshot& snapshot, int screenWidth) {
    const HudPanelRect rect = przypalHudPanelRect(snapshot, screenWidth);
    const int y = rect.y;
    const int height = rect.height;
    const int width = rect.width;
    const int x = rect.x;
    const HudColor bandColor = przypalColor(snapshot.band);
    HudDrawSink& sink = hudDrawSink();

    sink.fillRect(x, y, width, height, hudFade(black(), 0.56f + snapshot.hudPulse * 0.06f));
    sink.strokeRect(x, y, width, height, bandColor);
    drawUiText(sink, TextFormat("Przypał: %s", PrzypalSystem::hudLabel(snapshot.band)), x + 12, y + 8, 18, white());
    const int meterWidth = hudMeterWidth(width);
    sink.fillRect(x + 12, y + 34, meterWidth, 12, hudFade(white(), 0.16f));
    sink.fillRect(x + 12, y + 34, static_cast<int>(static_cast<float>(meterWidth) * snapshot.normalizedIntensity), 12, bandColor);
    sink.strokeRect(x + 12, y + 34, meterWidth, 12, white());
    drawUiText(sink, TextFormat("%.0f/100", snapshot.value), x + meterWidth + 22, y + 30, 17, bandColor);
    if (snapshot.band == PrzypalBand::Calm) {
        drawUiText(sink, "Spokojnie", x + 12, y + 51, 15, hudColor(214, 217, 210));
    } else {
        drawUiText(sink, "Ostatnio: hałas na osiedlu", x + 12, y + 51, 15, hudColor(214, 217, 210));
    }

    if (snapshot.reactionToastSeconds > 0.0f && snapshot.reactionAvailable) {
        const std::string reaction = snapshot.reactionSpeaker + ": " + snapshot.reactionText;
        const std::vector<std::string> reactionLines = HudRenderer::wrapTextToWidth(reaction, 15, width - 24);
        for (std::size_t i = 0; i < reactionLines.size() && i < 2; ++i) {
            drawUiText(sink, reactionLines[i], x + 12, y + 76 + static_cast<int>(i) * 17, 15, white());
        }
    }
}

void HudRenderer::drawHud(const HudSnapshot& snapshot) {
    const float pulse = snapshot.hudPulse;
    const bool debugHudActive = snapshot.debugEnabled && runtimeDevToolsEnabledByDefault();
    const HudObjectivePanelLayout objectiveLayout = objectivePanelLayout(snapshot, hudTextMetrics());
    HudDrawSink& sink = hudDrawSink();

    sink.fillRect(objectiveLayout.rect.x,
                  objectiveLayout.rect.y,
                  objectiveLayout.rect.width,
                  objectiveLayout.rect.height,
                  color(24, 27, 26, static_cast<unsigned char>(202 + pulse * 28.0f)));
    sink.strokeRect(objectiveLayout.rect.x,
                    objectiveLayout.rect.y,
                    objectiveLayout.rect.width,
                    objectiveLayout.rect.height,
                    color(238, 211, 79));
    drawUiText(sink, "CEL", objectiveLayout.headingX, objectiveLayout.headingY, 14, color(238, 211, 79));
    for (std::size_t i = 0; i < objectiveLayout.titleLines.size(); ++i) {
        drawUiText(sink,
                   objectiveLayout.titleLines[i],
                   objectiveLayout.titleX,
                   objectiveLayout.firstTitleY + static_cast<int>(i) * objectiveLayout.titleLineStep,
                   19,
                   white());
    }
    if (!objectiveLayout.objective.distance.empty()) {
        drawUiText(sink,
                   objectiveLayout.objective.distance,
                   objectiveLayout.distanceX,
                   objectiveLayout.distanceY,
                   18,
                   color(238, 211, 79));
    }

    const bool showControls = snapshot.debugEnabled || snapshot.phaseSeconds < 7.5f;
    int helperY = objectiveLayout.rect.y + objectiveLayout.rect.height + 10;
    if (showControls) {
        const std::string controlsText = buildHudControlHelp(snapshot.playerInVehicle, runtimeDevToolsEnabledByDefault());
        const HudTextLayout textLayout = layoutHudText("", controlsText, std::min(snapshot.screenWidth, 820));
        for (std::size_t i = 0; i < textLayout.controlsLines.size() && i < 2; ++i) {
            drawUiText(sink,
                       textLayout.controlsLines[i],
                       22,
                       helperY + static_cast<int>(i) * 19,
                       16,
                       color(214, 217, 210, snapshot.debugEnabled ? 230 : 178));
        }
        helperY += static_cast<int>(std::min<std::size_t>(textLayout.controlsLines.size(), 2)) * 19 + 4;
    }

    if (debugHudActive && snapshot.visualBaselineActive) {
        const std::string baseline = "Widok QA " + std::to_string(snapshot.visualBaselineIndex + 1) +
                                     "/" + std::to_string(snapshot.visualBaselineCount) + ": " +
                                     snapshot.visualBaselineLabel;
        drawUiText(sink, baseline, 28, helperY, 17, color(80, 190, 255));
        helperY += 20;
    }
    if (debugHudActive && snapshot.characterPosePreviewActive) {
        const std::string posePreview = "Postać QA " + std::to_string(snapshot.characterPosePreviewIndex + 1) +
                                        "/" + std::to_string(snapshot.characterPosePreviewCount) + ": " +
                                        snapshot.characterPosePreviewLabel + "  [/] zmiana";
        drawUiText(sink, posePreview, 28, helperY, 17, color(246, 190, 92));
        helperY += 20;
    }
    if (debugHudActive && snapshot.assetPreviewActive) {
        const std::string assetPreview = "Asset QA " + std::to_string(snapshot.assetPreviewIndex + 1) +
                                         "/" + std::to_string(snapshot.assetPreviewCount) + ": " +
                                         snapshot.assetPreviewLabel + "  F7/Shift+F7 zmiana";
        drawUiText(sink, assetPreview, 28, helperY, 17, color(255, 236, 128));
        helperY += 20;
        drawUiText(sink, snapshot.assetPreviewMetadata, 28, helperY, 17, color(170, 225, 170));
        helperY += 20;
    }
    if (debugHudActive) {
        drawUiText(sink, buildHudDebugHelp(), 28, helperY, 17, color(238, 211, 79));
        helperY += 20;
    }
    if (snapshot.missionPhase == MissionPhase::WalkToShop && snapshot.phaseSeconds > 8.0f) {
        drawUiText(sink, "Podążaj za zieloną strzałką i markerem do sklepu Zenona.", 28, helperY, 17, color(75, 220, 105));
    }

    drawObjectiveCompass(snapshot);
    drawInteractionPrompt(snapshot.interaction, snapshot.screenWidth, snapshot.screenHeight);
    drawVehicleHud(snapshot.vehicle, snapshot.screenWidth);
    drawPrzypalHud(snapshot.przypal, snapshot.screenWidth);
    if (snapshot.dialogue != nullptr) {
        drawDialogueBox(*snapshot.dialogue, snapshot.screenWidth, snapshot.screenHeight);
    }

    if (snapshot.chase.active) {
        const HudPanelRect chaseMeter = chaseHudMeterRect(snapshot.screenWidth);
        sink.fillRect(chaseMeter.x, chaseMeter.y, chaseMeter.width, chaseMeter.height, hudFade(black(), 0.60f));
        sink.fillRect(chaseMeter.x,
                      chaseMeter.y,
                      static_cast<int>(chaseMeter.width * snapshot.chase.escapeProgress),
                      chaseMeter.height,
                      color(80, 185, 105));
        sink.fillRect(chaseMeter.x,
                      chaseMeter.y + 28,
                      static_cast<int>(chaseMeter.width * snapshot.chase.caughtProgress),
                      8,
                      color(230, 74, 64));
        sink.strokeRect(chaseMeter.x, chaseMeter.y, chaseMeter.width, chaseMeter.height, white());
        drawUiText(sink, "zgub pościg", chaseMeter.x + 4, chaseMeter.y - 22, 18, white());
    }

    const HudMissionBannerLayout banner = missionBannerLayout(snapshot);
    if (banner.visible) {
        const bool failed = snapshot.missionPhase == MissionPhase::Failed;
        sink.fillRect(banner.rect.x,
                      banner.rect.y,
                      banner.rect.width,
                      banner.rect.height,
                      hudFade(black(), failed ? 0.75f : 0.68f));
        drawUiText(sink,
                   banner.text,
                   banner.textX,
                   banner.textY,
                   failed ? 26 : 28,
                   failed ? color(245, 90, 80) : color(95, 220, 117));
    }

    if (snapshot.flashAlpha > 0.0f) {
        HudColor flashColor = snapshot.missionPhase == MissionPhase::Failed ? color(185, 35, 28) : color(75, 170, 84);
        sink.fillRect(0,
                      0,
                      snapshot.screenWidth,
                      snapshot.screenHeight,
                      hudFade(flashColor, snapshot.flashAlpha * 0.28f));
    }
}

void DebugRenderer::drawPanelBackground(int x, int y, int width, int height) {
    hudDrawSink().fillRect(x, y, width, height, hudFade(black(), 0.48f));
}

void DebugRenderer::drawPanel(const DebugSnapshot& snapshot) {
    drawPanelBackground(12, 56, 1000, 58);
    drawPanelBackground(12, 116, 520, 640);
    drawUiText("Build: " + snapshot.buildStamp, 24, 64, 14, color(170, 225, 170));
    drawUiText("Data: " + snapshot.dataRoot, 24, 82, 14, color(170, 225, 170));
    drawUiText("Exe: " + snapshot.executablePath, 24, 100, 14, color(170, 225, 170));
    drawUiText(TextFormat("FPS: %d  dt %.2f ms", snapshot.fps, snapshot.deltaMs),
             24,
             116,
             18,
             color(170, 225, 170));
    drawUiText("Render: " + snapshot.renderModeLabel, 300, 116, 18, color(238, 211, 79));
    drawUiText(TextFormat("Phase: %s", phaseName(snapshot.phase)), 24, 140, 18, white());
    drawUiText("Active shop: " + snapshot.activeShopAssetId, 300, 140, 18, color(238, 211, 79));
    drawUiText(TextFormat("Player: %.1f %.2f %.1f",
                        snapshot.playerPosition.x,
                        snapshot.playerPosition.y,
                        snapshot.playerPosition.z),
             24,
             164,
             18,
             white());
    drawUiText(TextFormat("Assets: O%d D%d G%d T%d V%d skip %d",
                        snapshot.opaqueAssetCount,
                        snapshot.decalAssetCount,
                        snapshot.glassAssetCount,
                        snapshot.translucentAssetCount,
                        snapshot.vehicleAssetCount,
                        snapshot.debugOnlyAssetSkippedCount),
             300,
             164,
             18,
             color(170, 225, 170));
    drawUiText(TextFormat("Speed: %s  XZ %.2f  Vy %.2f",
                        speedStateName(snapshot.speedState),
                        snapshot.playerXzSpeed,
                        snapshot.playerVerticalSpeed),
             24,
             188,
             18,
             white());
    drawUiText(TextFormat("Car: %.1f %.1f speed %.1f",
                        snapshot.carPosition.x,
                        snapshot.carPosition.z,
                        snapshot.vehicleSpeed),
             24,
             212,
             18,
             white());
    drawUiText(TextFormat("Gruz: %.0f%% %s slip %.2f drift %.0f%%",
                        snapshot.vehicleCondition,
                        conditionBandName(snapshot.vehicleConditionBand),
                        snapshot.vehicleSlip,
                        snapshot.vehicleDriftAmount * 100.0f),
             24,
             236,
             18,
             snapshot.vehicleDriftActive ? color(255, 210, 84) : white());
    drawUiText(TextFormat("Gear %d RPM %.0f%% shift %.2f Hit %s %.1f",
                        snapshot.vehicleGear,
                        snapshot.vehicleRpmNormalized * 100.0f,
                        snapshot.vehicleShiftTimerSeconds,
                        vehicleHitZoneName(snapshot.vehicleHitZone),
                        snapshot.vehicleImpactSpeed),
             24,
             260,
             18,
             snapshot.vehicleShiftTimerSeconds > 0.0f ? color(255, 210, 84) : white());
    drawUiText(TextFormat("Boost: %s Horn %.2f DriftA %.1f",
                        snapshot.boostActive ? "yes" : "no",
                        snapshot.hornCooldown,
                        snapshot.vehicleDriftAngleDegrees),
             24,
             284,
             18,
             snapshot.vehicleDriftActive ? color(255, 210, 84) : white());
    drawUiText(TextFormat("Context: %s  In vehicle: %s",
                        cameraModeName(snapshot.cameraMode),
                        snapshot.playerInVehicle ? "yes" : "no"),
             24,
             308,
             18,
             white());
    drawUiText(TextFormat("Mouse: %s X:%s  yaw %.1f pitch %.1f",
                        snapshot.mouseCaptured ? "locked" : "free",
                        snapshot.invertMouseX ? "orbit" : "look",
                        snapshot.cameraYawDegrees,
                        snapshot.cameraPitchDegrees),
             24,
             332,
             18,
             white());
    drawUiText(TextFormat("Boom %.2f/%.2f gap %.2f Ground %s h %.2f own %d",
                        snapshot.boomLength,
                        snapshot.fullBoomLength,
                        snapshot.boomGap,
                        snapshot.grounded ? "yes" : "no",
                        snapshot.groundHeight,
                        snapshot.supportOwnerId),
             24,
             356,
             18,
             white());
    drawUiText(TextFormat("Cam delta target %.2fm pos %.2fm",
                        snapshot.cameraTargetLag,
                        snapshot.cameraPositionLag),
             24,
             380,
             18,
             snapshot.cameraTargetLag > 0.75f ? color(255, 210, 84) : white());
    drawUiText(TextFormat("Shake %.2f Zoom %.2f Tension %.2f Stable %s",
                        snapshot.screenShake,
                        snapshot.comedyZoom,
                        snapshot.worldTension,
                        snapshot.stableCameraMode ? "ON" : "OFF"),
             24,
             404,
             18,
             (snapshot.screenShake > 0.01f || snapshot.comedyZoom > 0.01f || snapshot.worldTension > 0.01f)
                 ? color(255, 210, 84)
                 : white());
    drawUiText(TextFormat("Przypal %.1f %s events %d",
                        snapshot.przypalValue,
                        snapshot.przypalLabel.c_str(),
                        snapshot.recentEventCount),
             24,
             428,
             18,
             przypalColor(snapshot.przypalBand));
    drawUiText("Last heat: " + snapshot.lastPrzypalReason, 24, 452, 18, white());
    drawUiText(TextFormat("Mem B%d S%d P%d G%d T%d R%d | Hot %d Story %d%d%d",
                        snapshot.memoryBlockCount,
                        snapshot.memoryShopCount,
                        snapshot.memoryParkingCount,
                        snapshot.memoryGarageCount,
                        snapshot.memoryTrashCount,
                        snapshot.memoryRoadLoopCount,
                        snapshot.memoryHotspotCount,
                        snapshot.visitedShopOnFoot ? 1 : 0,
                        snapshot.gruzUnlocked ? 1 : 0,
                        snapshot.introCompleted ? 1 : 0),
             24,
             476,
             18,
             white());
    const std::string localServiceName = localServiceDebugName(snapshot.localServiceFirstWaryPoint);
    const std::string localFavorName = localServiceDebugName(snapshot.localFavorFirstActivePoint);
    drawUiText(TextFormat("Rewir %s %s sc %.1f d %.1f p%d | Svc %d/%d %s | Fav %d/%d/%d %s",
                        snapshot.rewirPressureActive ? snapshot.rewirPressureLocation.c_str() : "none",
                        snapshot.rewirPressureLevel.c_str(),
                        snapshot.rewirPressureScore,
                        snapshot.rewirPressureDistance,
                        snapshot.rewirPressurePatrolInterest ? 1 : 0,
                        snapshot.localServiceWary,
                        snapshot.localServiceTotal,
                        localServiceName.c_str(),
                        snapshot.localFavorActive,
                        snapshot.localFavorCompleted,
                        snapshot.localFavorTotal,
                        localFavorName.c_str()),
             24,
             500,
             18,
             (snapshot.rewirPressureActive || snapshot.localServiceWary > 0 || snapshot.localFavorActive > 0)
                 ? color(255, 210, 84)
                 : white());
    drawUiText(TextFormat("Raw WASD: %d%d%d%d  Mdx %.1f Mdy %.1f",
                        snapshot.rawW ? 1 : 0,
                        snapshot.rawA ? 1 : 0,
                        snapshot.rawS ? 1 : 0,
                        snapshot.rawD ? 1 : 0,
                        snapshot.mouseDeltaX,
                        snapshot.mouseDeltaY),
             24,
             524,
             18,
             white());
    drawUiText(TextFormat("Drive: gas %d brake %d L %d R %d steerAuth %.2f",
                        snapshot.accelerate ? 1 : 0,
                        snapshot.brake ? 1 : 0,
                        snapshot.steerLeft ? 1 : 0,
                        snapshot.steerRight ? 1 : 0,
                        snapshot.steeringAuthority),
             24,
             548,
             18,
             white());
    drawUiText(TextFormat("Yaw car %.1f move %.1f visual %.1f err %.1f",
                        snapshot.vehicleYawDegrees,
                        snapshot.cameraYawDegrees,
                        snapshot.visualYawDegrees,
                        snapshot.visualYawErrorDegrees),
             24,
             572,
             18,
             white());
    drawUiText(TextFormat("Route %d %s chaseDist %.1f",
                        snapshot.routeIndex,
                        snapshot.routeLabel.c_str(),
                        snapshot.chaseDistance),
             24,
             596,
             18,
             white());
    drawUiText(TextFormat("ChaseAI %s recover %.2f contact %d",
                        snapshot.chaseAiMode.c_str(),
                        snapshot.chaseRecoverSeconds,
                        snapshot.chaseContactRecover ? 1 : 0),
             24,
             620,
             18,
             snapshot.chaseContactRecover ? color(255, 210, 84) : white());
    if (snapshot.reactionAvailable) {
        drawUiText("Reaction: " + snapshot.reactionSpeaker, 24, 644, 18, white());
    }
    if (snapshot.visualBaselineActive) {
        drawUiText(TextFormat("Visual QA %d/%d: %s",
                            snapshot.visualBaselineIndex + 1,
                            snapshot.visualBaselineCount,
                            snapshot.visualBaselineLabel.c_str()),
                 24,
                 668,
                 18,
                 color(80, 190, 255));
    }
    drawUiText("Input truth: W fwd  S back  A left  D right  MouseX look",
             24,
             692,
             17,
             color(170, 225, 170));
    drawUiText("Diags: WA fwd-left  WD fwd-right  SA back-left  SD back-right",
             24,
             714,
             17,
             color(170, 225, 170));
    drawUiText("Vehicle: A turns left  D turns right",
             24,
             736,
             17,
             color(170, 225, 170));
}

void DebugRenderer::drawCameraLines(const CameraDebugLinesSnapshot& snapshot) {
    if (!snapshot.debugEnabled) {
        return;
    }

    const CameraRigState& rig = snapshot.rig;
    DrawLine3D(toRay(rig.target), toRay(rig.desiredPosition), toRayColor(color(238, 211, 79)));
    DrawLine3D(toRay(rig.target), toRay(rig.position), toRayColor(color(95, 220, 117)));
    DrawLine3D(toRay(rig.target), toRay(rig.desiredTarget), toRayColor(color(80, 190, 255)));
    DrawSphere(toRay(rig.target), 0.12f, toRayColor(color(238, 211, 79)));
    DrawSphere(toRay(rig.desiredTarget), 0.08f, toRayColor(color(80, 190, 255)));

    if (snapshot.playerInVehicle) {
        const Vec3 origin = snapshot.vehiclePosition + Vec3{0.0f, 0.35f, 0.0f};
        const Vec3 forward = forwardFromYaw(snapshot.vehicleYawRadians);
        const Vec3 right = screenRightFromYaw(snapshot.vehicleYawRadians);
        DrawLine3D(toRay(origin), toRay(origin + forward * 4.0f), toRayColor(color(255, 236, 128)));
        DrawSphere(toRay(origin + forward * 4.0f), 0.18f, toRayColor(color(255, 236, 128)));
        DrawLine3D(toRay(origin), toRay(origin + right * 1.5f), toRayColor(color(255, 110, 110)));
        DrawSphere(toRay(snapshot.vehicleCollisionCircles[0] + Vec3{0.0f, 0.20f, 0.0f}), 0.18f, toRayColor(color(255, 236, 128)));
        DrawSphere(toRay(snapshot.vehicleCollisionCircles[1] + Vec3{0.0f, 0.20f, 0.0f}), 0.14f, toRayColor(color(80, 190, 255)));
        DrawSphere(toRay(snapshot.vehicleCollisionCircles[2] + Vec3{0.0f, 0.20f, 0.0f}), 0.18f, toRayColor(color(255, 110, 110)));
    } else {
        const Vec3 origin = snapshot.playerPosition + Vec3{0.0f, 0.18f, 0.0f};
        const Vec3 cameraForward = forwardFromYaw(snapshot.playerCameraYawRadians);
        const Vec3 cameraRight = screenRightFromYaw(snapshot.playerCameraYawRadians);
        DrawLine3D(toRay(origin), toRay(origin + cameraForward * 2.0f), toRayColor(color(80, 190, 255)));
        DrawLine3D(toRay(origin), toRay(origin + cameraRight * 1.35f), toRayColor(color(255, 110, 110)));
        DrawLine3D(toRay(origin), toRay(origin + snapshot.playerVelocity * 0.35f), toRayColor(color(255, 255, 255)));
    }
}

} // namespace bs3d
