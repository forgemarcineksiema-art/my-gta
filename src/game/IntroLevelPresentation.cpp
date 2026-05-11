#include "IntroLevelPresentation.h"

#include <cmath>

namespace bs3d {

namespace {

RenderColor color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
    return {r, g, b, a};
}

const IntroLevelData& levelOrFallback(const IntroObjectiveContext& context) {
    static const IntroLevelData fallback = IntroLevelBuilder::build();
    return context.level != nullptr ? *context.level : fallback;
}

} // namespace

ObjectiveMarker IntroLevelPresentation::currentObjectiveMarker(const IntroObjectiveContext& context) {
    const IntroLevelData& level = levelOrFallback(context);
    switch (context.phase) {
    case MissionPhase::WaitingForStart:
    case MissionPhase::ReturnToBench:
        return {"Boguś", level.npcPosition, color(238, 211, 79), true};
    case MissionPhase::WalkToShop:
        return {"Sklep Zenona", level.shopEntrancePosition, color(75, 220, 105), true};
    case MissionPhase::DriveToShop:
        if (context.driveRoute != nullptr) {
            if (const RoutePoint* point = context.driveRoute->activePoint()) {
                return {point->label, point->position, color(75, 220, 105), true};
            }
        }
        return {"Sklep Zenona", level.shopEntrancePosition, color(75, 220, 105), true};
    case MissionPhase::ReachVehicle:
        return {"Gruz", context.vehiclePosition, color(221, 82, 66), true};
    case MissionPhase::ReturnToLot:
        return {"Parking", level.dropoffPosition, color(77, 150, 225), true};
    case MissionPhase::ChaseActive:
        return {"Ucieczka",
                context.vehiclePosition + forwardFromYaw(context.vehicleYawRadians) * 12.0f,
                color(230, 88, 70),
                true};
    case MissionPhase::Completed:
    case MissionPhase::Failed:
        return {};
    }

    return {};
}

std::string IntroLevelPresentation::objectiveLineWithDistance(const IntroObjectiveContext& context,
                                                              const std::string& objectiveText,
                                                              Vec3 actorPosition) {
    const ObjectiveMarker marker = currentObjectiveMarker(context);
    if (!marker.active || context.phase == MissionPhase::ChaseActive) {
        return "Cel: " + objectiveText;
    }

    const int meters = static_cast<int>(std::round(distanceXZ(actorPosition, marker.position)));
    return "Cel: " + objectiveText + " (" + std::to_string(meters) + "m)";
}

bool IntroLevelPresentation::activeMarkerFor(const IntroObjectiveContext& context, Vec3 markerPosition) {
    const IntroLevelData& level = levelOrFallback(context);
    if (context.phase == MissionPhase::WaitingForStart || context.phase == MissionPhase::ReturnToBench) {
        return distanceXZ(markerPosition, level.npcPosition) < 0.5f;
    }
    if (context.phase == MissionPhase::WalkToShop) {
        return distanceXZ(markerPosition, level.shopEntrancePosition) < 0.5f;
    }
    if (context.phase == MissionPhase::ReachVehicle) {
        return distanceXZ(markerPosition, context.vehiclePosition) < 0.5f;
    }
    if (context.phase == MissionPhase::DriveToShop) {
        if (context.driveRoute != nullptr) {
            if (const RoutePoint* point = context.driveRoute->activePoint()) {
                return distanceXZ(markerPosition, point->position) < 0.5f;
            }
        }
        return false;
    }
    if (context.phase == MissionPhase::ReturnToLot) {
        return distanceXZ(markerPosition, level.dropoffPosition) < 0.5f;
    }
    return false;
}

} // namespace bs3d
