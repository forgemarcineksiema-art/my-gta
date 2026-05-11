#include "GameHudLayout.h"

#include <algorithm>

namespace bs3d {

namespace {

std::string stripPrefix(std::string text, const std::string& prefix) {
    if (text.rfind(prefix, 0) == 0) {
        return text.substr(prefix.size());
    }
    return text;
}

} // namespace

const char* conditionBandName(VehicleConditionBand band) {
    switch (band) {
    case VehicleConditionBand::Igla:
        return "Igła";
    case VehicleConditionBand::JeszczePojezdzi:
        return "Jeszcze pojeździ";
    case VehicleConditionBand::CosStuka:
        return "Coś stuka";
    case VehicleConditionBand::ZarazWybuchnieAleNieDzis:
        return "Zaraz wybuchnie";
    case VehicleConditionBand::Zlom:
        return "Złom";
    }

    return "Nie wiadomo";
}

const char* vehicleHitZoneName(VehicleHitZone zone) {
    switch (zone) {
    case VehicleHitZone::None:
        return "none";
    case VehicleHitZone::Front:
        return "front";
    case VehicleHitZone::Rear:
        return "rear";
    case VehicleHitZone::LeftSide:
        return "left";
    case VehicleHitZone::RightSide:
        return "right";
    case VehicleHitZone::Center:
        return "center";
    }

    return "?";
}

const char* cameraModeName(CameraRigMode mode) {
    switch (mode) {
    case CameraRigMode::OnFootNormal:
        return "OnFoot";
    case CameraRigMode::OnFootSprint:
        return "Sprint";
    case CameraRigMode::Interaction:
        return "Interact";
    case CameraRigMode::Vehicle:
        return "Vehicle";
    }

    return "Unknown";
}

const char* speedStateName(PlayerSpeedState state) {
    switch (state) {
    case PlayerSpeedState::Idle:
        return "Idle";
    case PlayerSpeedState::Walk:
        return "Walk";
    case PlayerSpeedState::Jog:
        return "Jog";
    case PlayerSpeedState::Sprint:
        return "Sprint";
    case PlayerSpeedState::Airborne:
        return "Airborne";
    case PlayerSpeedState::Landing:
        return "Landing";
    }

    return "Unknown";
}

const char* phaseName(MissionPhase phase) {
    switch (phase) {
    case MissionPhase::WaitingForStart:
        return "WaitingForStart";
    case MissionPhase::WalkToShop:
        return "WalkToShop";
    case MissionPhase::ReturnToBench:
        return "ReturnToBench";
    case MissionPhase::ReachVehicle:
        return "ReachVehicle";
    case MissionPhase::DriveToShop:
        return "DriveToShop";
    case MissionPhase::ChaseActive:
        return "ChaseActive";
    case MissionPhase::ReturnToLot:
        return "ReturnToLot";
    case MissionPhase::Completed:
        return "Completed";
    case MissionPhase::Failed:
        return "Failed";
    }

    return "Unknown";
}

std::string localServiceDebugName(const std::string& interactionPointId) {
    if (interactionPointId.empty()) {
        return "-";
    }

    const std::size_t separator = interactionPointId.find('_');
    if (separator == std::string::npos || separator == 0) {
        return interactionPointId;
    }

    return interactionPointId.substr(0, separator);
}

ObjectiveHudText compactObjectiveText(const std::string& objectiveLine) {
    ObjectiveHudText text;
    text.title = stripPrefix(objectiveLine, "Cel: ");
    const std::size_t open = text.title.rfind('(');
    const std::size_t close = text.title.rfind(')');
    if (open != std::string::npos && close == text.title.size() - 1 && open < close) {
        text.distance = text.title.substr(open + 1, close - open - 1);
        while (!text.title.empty() && text.title.back() == ' ') {
            text.title.pop_back();
        }
        text.title = text.title.substr(0, open);
        while (!text.title.empty() && text.title.back() == ' ') {
            text.title.pop_back();
        }
    }
    return text;
}

bool rectanglesOverlap(int x, int y, int width, int height, HudPanelRect rect, int padding) {
    return x < rect.x + rect.width + padding &&
           x + width > rect.x - padding &&
           y < rect.y + rect.height + padding &&
           y + height > rect.y - padding;
}

HudPanelRect layoutHudPanel(int screenWidth,
                            int preferredWidth,
                            int y,
                            int height,
                            bool alignLeft,
                            int margin) {
    const int safeMargin = std::max(4, margin);
    const int maxWidth = std::max(160, screenWidth - safeMargin * 2);
    const int width = std::clamp(preferredWidth, 160, maxWidth);
    const int x = alignLeft
                      ? safeMargin
                      : std::max(safeMargin, screenWidth - width - safeMargin);
    return {x, y, width, height};
}

int przypalHudHeight(const PrzypalHudSnapshot& snapshot) {
    return snapshot.reactionToastSeconds > 0.0f ? 104 : 70;
}

HudPanelRect vehicleHudPanelRect(const VehicleHudSnapshot& snapshot, int screenWidth) {
    const int y = snapshot.chaseActive ? 162 : 126;
    return layoutHudPanel(screenWidth, 318, y, 82, false, 18);
}

HudPanelRect przypalHudPanelRect(const PrzypalHudSnapshot& snapshot, int screenWidth) {
    const int y = snapshot.chaseActive ? 162 : 126;
    return layoutHudPanel(screenWidth, 318, y, przypalHudHeight(snapshot), snapshot.playerInVehicle, 18);
}

HudPanelRect chaseHudMeterRect(int screenWidth) {
    constexpr int width = 220;
    return {screenWidth - width - 32, 104, width, 24};
}

HudPanelRect missionBannerRect(int screenWidth, int screenHeight) {
    return {0, screenHeight / 2 - 36, screenWidth, 72};
}

HudMissionBannerLayout missionBannerLayout(const HudSnapshot& snapshot) {
    HudMissionBannerLayout layout;
    if (snapshot.missionPhase == MissionPhase::Failed) {
        layout.visible = true;
        layout.rect = missionBannerRect(snapshot.screenWidth, snapshot.screenHeight);
        layout.textX = snapshot.screenWidth / 2 - 190;
        layout.textY = snapshot.screenHeight / 2 - 12;
        layout.text = "MISJA NIEUDANA - naciśnij R";
    } else if (snapshot.missionPhase == MissionPhase::Completed && snapshot.flashAlpha > 0.0f) {
        layout.visible = true;
        layout.rect = missionBannerRect(snapshot.screenWidth, snapshot.screenHeight);
        layout.textX = snapshot.screenWidth / 2 - 136;
        layout.textY = snapshot.screenHeight / 2 - 12;
        layout.text = "MISJA ZALICZONA";
    }
    return layout;
}

HudDialogueBoxLayout dialogueBoxLayout(const DialogueLine& line,
                                       int screenWidth,
                                       int screenHeight,
                                       const HudTextMetrics& metrics) {
    HudDialogueBoxLayout layout;
    const int boxX = 120;
    const int boxWidth = screenWidth - 240;
    const int textWidth = boxWidth - 52;
    layout.lines = wrapHudTextToWidth(line.text, 23, textWidth, metrics);
    const int boxHeight = 58 + static_cast<int>(layout.lines.size()) * 28;
    const int boxY = screenHeight - boxHeight - 28;
    layout.rect = {boxX, boxY, boxWidth, boxHeight};
    layout.speakerX = boxX + 26;
    layout.speakerY = boxY + 12;
    layout.lineX = boxX + 26;
    layout.firstLineY = boxY + 42;
    return layout;
}

HudInteractionPromptLayout interactionPromptLayout(const InteractionPromptSnapshot& snapshot,
                                                   int screenWidth,
                                                   int screenHeight,
                                                   const HudTextMetrics& metrics) {
    HudInteractionPromptLayout layout;
    if (snapshot.lines.empty()) {
        return layout;
    }

    int measuredWidth = 0;
    for (const std::string& line : snapshot.lines) {
        measuredWidth = std::max(measuredWidth, metrics.measureTextWidth(line, 22));
    }
    const int width = measuredWidth + 40;
    const int x = screenWidth / 2 - width / 2;
    const int y = screenHeight - 176;
    const int height = 18 + static_cast<int>(snapshot.lines.size()) * 28;
    layout.visible = true;
    layout.rect = {x, y, width, height};
    layout.lineX = x + 20;
    layout.firstLineY = y + 10;
    return layout;
}

HudObjectivePanelLayout objectivePanelLayout(const HudSnapshot& snapshot, const HudTextMetrics& metrics) {
    HudObjectivePanelLayout layout;
    layout.objective = compactObjectiveText(snapshot.objectiveLine);
    const int objectiveWidth = std::clamp(snapshot.screenWidth / 3, 270, 390);
    layout.titleLines = wrapHudTextToWidth(layout.objective.title, 19, objectiveWidth - 64, metrics);
    const int objectiveHeight = std::max(72, 42 + static_cast<int>(layout.titleLines.size()) * 22);
    layout.rect = {18, 18, objectiveWidth, objectiveHeight};
    layout.headingX = 32;
    layout.headingY = 28;
    layout.titleX = 32;
    layout.firstTitleY = 50;
    layout.distanceY = 28;
    if (!layout.objective.distance.empty()) {
        const int distanceWidth = metrics.measureTextWidth(layout.objective.distance, 18);
        layout.distanceX = layout.rect.x + layout.rect.width - distanceWidth - 18;
    }
    return layout;
}

HudObjectiveCompassLayout objectiveCompassLayout(const HudSnapshot& snapshot, const HudProjection& projection) {
    HudObjectiveCompassLayout layout;
    const ObjectiveMarker& marker = snapshot.objectiveMarker;
    const Vec3 worldPos = marker.position + Vec3{0.0f, 2.8f, 0.0f};
    layout.screen = projection.projectWorldToScreen(worldPos, snapshot.camera);

    const Vec3 cameraPosition = snapshot.camera.position;
    const Vec3 cameraTarget = snapshot.camera.target;
    const Vec3 cameraForward = cameraTarget - cameraPosition;
    const Vec3 toTarget = marker.position - cameraPosition;
    const float facingDot = cameraForward.x * toTarget.x + cameraForward.y * toTarget.y + cameraForward.z * toTarget.z;
    if (facingDot < 0.0f) {
        layout.screen.x = static_cast<float>(snapshot.screenWidth) - layout.screen.x;
        layout.screen.y = static_cast<float>(snapshot.screenHeight) - layout.screen.y;
    }

    const float minX = 36.0f;
    const float maxX = static_cast<float>(snapshot.screenWidth - 36);
    const float minY = 122.0f;
    const float maxY = static_cast<float>(snapshot.screenHeight - 158);
    layout.offscreen = layout.screen.x < minX || layout.screen.x > maxX ||
                       layout.screen.y < minY || layout.screen.y > maxY;
    layout.clamped = {std::clamp(layout.screen.x, minX, maxX), std::clamp(layout.screen.y, minY, maxY)};
    layout.distanceMeters = static_cast<int>(std::round(distanceXZ(snapshot.actorPosition, marker.position)));
    return layout;
}

HudObjectiveCompassLabelLayout objectiveCompassLabelLayout(const HudSnapshot& snapshot,
                                                          HudScreenPoint clamped,
                                                          const std::string& label,
                                                          const HudTextMetrics& metrics) {
    HudObjectiveCompassLabelLayout layout;
    const int textWidth = metrics.measureTextWidth(label, 18);
    const int labelWidth = textWidth + 16;
    const int labelHeight = 26;
    int labelX = static_cast<int>(std::clamp(clamped.x - static_cast<float>(textWidth) * 0.5f - 8.0f,
                                             8.0f,
                                             std::max(8.0f, static_cast<float>(snapshot.screenWidth - labelWidth))));
    const int labelY = static_cast<int>(std::clamp(clamped.y + 18.0f,
                                                   104.0f,
                                                   static_cast<float>(snapshot.screenHeight - 140)));
    labelX = objectiveLabelXOutsidePanel(labelX,
                                         labelY,
                                         labelWidth,
                                         labelHeight,
                                         przypalHudPanelRect(snapshot.przypal, snapshot.screenWidth),
                                         snapshot.screenWidth);
    if (snapshot.vehicle.visible) {
        labelX = objectiveLabelXOutsidePanel(labelX,
                                             labelY,
                                             labelWidth,
                                             labelHeight,
                                             vehicleHudPanelRect(snapshot.vehicle, snapshot.screenWidth),
                                             snapshot.screenWidth);
    }
    layout.rect = {labelX, labelY, labelWidth, labelHeight};
    layout.textX = labelX + 8;
    layout.textY = labelY + 5;
    return layout;
}

int hudMeterWidth(int panelWidth) {
    return std::max(92, std::min(168, panelWidth - 138));
}

int objectiveLabelXOutsidePanel(int x, int y, int width, int height, HudPanelRect rect, int screenWidth) {
    constexpr int padding = 10;
    if (!rectanglesOverlap(x, y, width, height, rect, padding)) {
        return x;
    }

    const int minX = 8;
    const int maxX = std::max(minX, screenWidth - width - 8);
    const int leftCandidate = rect.x - width - padding;
    const int rightCandidate = rect.x + rect.width + padding;
    if (rect.x >= screenWidth / 2 && leftCandidate >= minX) {
        return leftCandidate;
    }
    if (rightCandidate <= maxX) {
        return rightCandidate;
    }
    if (leftCandidate >= minX) {
        return leftCandidate;
    }
    return std::clamp(x, minX, maxX);
}

} // namespace bs3d
