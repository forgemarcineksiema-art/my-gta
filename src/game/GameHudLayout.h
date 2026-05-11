#pragma once

#include "GameRenderers.h"
#include "GameHudTextMetrics.h"

#include <string>
#include <vector>

namespace bs3d {

struct ObjectiveHudText {
    std::string title;
    std::string distance;
};

struct HudMissionBannerLayout {
    bool visible = false;
    HudPanelRect rect{};
    int textX = 0;
    int textY = 0;
    const char* text = "";
};

struct HudScreenPoint {
    float x = 0.0f;
    float y = 0.0f;
};

struct HudDialogueBoxLayout {
    HudPanelRect rect{};
    int speakerX = 0;
    int speakerY = 0;
    int lineX = 0;
    int firstLineY = 0;
    int lineStep = 28;
    std::vector<std::string> lines;
};

struct HudInteractionPromptLayout {
    bool visible = false;
    HudPanelRect rect{};
    int lineX = 0;
    int firstLineY = 0;
    int lineStep = 28;
};

struct HudObjectivePanelLayout {
    HudPanelRect rect{};
    int headingX = 0;
    int headingY = 0;
    int titleX = 0;
    int firstTitleY = 0;
    int titleLineStep = 22;
    int distanceX = 0;
    int distanceY = 0;
    ObjectiveHudText objective;
    std::vector<std::string> titleLines;
};

struct HudObjectiveCompassLabelLayout {
    HudPanelRect rect{};
    int textX = 0;
    int textY = 0;
};

class HudProjection {
public:
    virtual ~HudProjection() = default;
    virtual HudScreenPoint projectWorldToScreen(Vec3 worldPosition, RenderCamera camera) const = 0;
};

struct HudObjectiveCompassLayout {
    HudScreenPoint screen{};
    bool offscreen = false;
    HudScreenPoint clamped{};
    int distanceMeters = 0;
};

const char* conditionBandName(VehicleConditionBand band);
const char* vehicleHitZoneName(VehicleHitZone zone);
const char* cameraModeName(CameraRigMode mode);
const char* speedStateName(PlayerSpeedState state);
const char* phaseName(MissionPhase phase);
std::string localServiceDebugName(const std::string& interactionPointId);
ObjectiveHudText compactObjectiveText(const std::string& objectiveLine);
bool rectanglesOverlap(int x, int y, int width, int height, HudPanelRect rect, int padding);
HudPanelRect vehicleHudPanelRect(const VehicleHudSnapshot& snapshot, int screenWidth);
HudPanelRect przypalHudPanelRect(const PrzypalHudSnapshot& snapshot, int screenWidth);
HudPanelRect chaseHudMeterRect(int screenWidth);
HudPanelRect missionBannerRect(int screenWidth, int screenHeight);
HudMissionBannerLayout missionBannerLayout(const HudSnapshot& snapshot);
HudDialogueBoxLayout dialogueBoxLayout(const DialogueLine& line,
                                       int screenWidth,
                                       int screenHeight,
                                       const HudTextMetrics& metrics);
HudInteractionPromptLayout interactionPromptLayout(const InteractionPromptSnapshot& snapshot,
                                                   int screenWidth,
                                                   int screenHeight,
                                                   const HudTextMetrics& metrics);
HudObjectivePanelLayout objectivePanelLayout(const HudSnapshot& snapshot, const HudTextMetrics& metrics);
HudObjectiveCompassLabelLayout objectiveCompassLabelLayout(const HudSnapshot& snapshot,
                                                          HudScreenPoint clamped,
                                                          const std::string& label,
                                                          const HudTextMetrics& metrics);
HudObjectiveCompassLayout objectiveCompassLayout(const HudSnapshot& snapshot, const HudProjection& projection);
int przypalHudHeight(const PrzypalHudSnapshot& snapshot);
int hudMeterWidth(int panelWidth);
int objectiveLabelXOutsidePanel(int x, int y, int width, int height, HudPanelRect rect, int screenWidth);

} // namespace bs3d
