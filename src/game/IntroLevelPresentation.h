#pragma once

#include "GameRenderers.h"
#include "IntroLevelBuilder.h"

#include "bs3d/core/DriveRouteGuide.h"
#include "bs3d/core/MissionController.h"

#include <string>

namespace bs3d {

struct IntroObjectiveContext {
    const IntroLevelData* level = nullptr;
    MissionPhase phase = MissionPhase::WaitingForStart;
    Vec3 vehiclePosition{};
    float vehicleYawRadians = 0.0f;
    const DriveRouteGuide* driveRoute = nullptr;
};

class IntroLevelPresentation {
public:
    static ObjectiveMarker currentObjectiveMarker(const IntroObjectiveContext& context);
    static std::string objectiveLineWithDistance(const IntroObjectiveContext& context,
                                                 const std::string& objectiveText,
                                                 Vec3 actorPosition);
    static void drawMarkers(const IntroObjectiveContext& context, float hudPulse, float elapsedSeconds);
    static void drawDistrictPlanDebug(const IntroLevelData& level, float elapsedSeconds);

private:
    static bool activeMarkerFor(const IntroObjectiveContext& context, Vec3 markerPosition);
};

} // namespace bs3d
