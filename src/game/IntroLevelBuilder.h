#pragma once

#include "WorldArtTypes.h"

#include "bs3d/core/DriveRouteGuide.h"
#include "bs3d/core/MissionTriggerSystem.h"
#include "bs3d/core/Scene.h"
#include "bs3d/core/WorldCollision.h"
#include "bs3d/core/WorldEventLedger.h"
#include "bs3d/core/WorldInteraction.h"

#include <string>
#include <vector>

namespace bs3d {

class WorldAssetRegistry;

struct LocationAnchor {
    WorldLocationTag tag = WorldLocationTag::Unknown;
    Vec3 position{};
    float radius = 0.0f;
    int priority = 0;
};

struct DistrictRewirPlan {
    std::string id;
    std::string label;
    std::string role;
    Vec3 center{};
    float radius = 0.0f;
    bool playableNow = false;
    bool homeBase = false;
    bool drivingSpine = false;
    bool serviceEconomy = false;
    bool vehicleIdentity = false;
    std::string approachRouteLabel;
    std::vector<std::string> gameplayJobs;
};

struct DistrictRoutePlan {
    std::string id;
    std::string fromRewirId;
    std::string toRewirId;
    bool vehicleRoute = false;
    bool futureExpansion = false;
    std::vector<RoutePoint> points;
};

struct DistrictPlanDebugRewir {
    std::string id;
    std::string label;
    Vec3 center{};
    float radius = 0.0f;
    bool playableNow = false;
    bool homeBase = false;
    bool drivingSpine = false;
    bool serviceEconomy = false;
    bool vehicleIdentity = false;
};

struct DistrictPlanDebugRouteSegment {
    std::string routeId;
    Vec3 from{};
    Vec3 to{};
    bool vehicleRoute = false;
    bool futureExpansion = false;
};

struct DistrictPlanDebugOverlay {
    std::vector<DistrictPlanDebugRewir> rewirMarkers;
    std::vector<DistrictPlanDebugRouteSegment> routeSegments;
    int futureRewirCount = 0;
    int vehicleRouteCount = 0;
};

struct DistrictRouteTraversalReport {
    bool foundRoute = false;
    bool clear = false;
    std::string routeId;
    int checkedSegments = 0;
    std::string blockedSegmentLabel;
    Vec3 blockedFrom{};
    Vec3 blockedTo{};
    Vec3 resolvedPosition{};
};

struct IntroLevelBuildConfig {
    Vec3 playerStart{-12.0f, 0.0f, -7.0f};
    Vec3 npcPosition{-12.0f, 0.0f, -10.0f};
    Vec3 vehicleStart{-2.35f, 0.0f, 7.75f};
    Vec3 parkingExitPosition{-1.0f, 0.0f, 5.0f};
    Vec3 parkingLanePosition{-0.5f, 0.0f, 1.0f};
    Vec3 roadBendPosition{3.0f, 0.0f, -10.0f};
    Vec3 shopRoadPosition{8.0f, 0.0f, -18.0f};
    Vec3 shopEntrancePosition{18.0f, 0.0f, -22.35f};
    Vec3 shopPosition{18.0f, 0.0f, -18.0f};
    Vec3 zenonPosition{17.2f, 0.0f, -21.2f};
    Vec3 lolekPosition{9.0f, 0.0f, -4.2f};
    Vec3 receiptHolderPosition{2.2f, 0.0f, -12.2f};
    Vec3 dropoffPosition{-7.0f, 0.0f, 8.2f};
    Vec3 initialChaserPosition{20.0f, 0.0f, -24.0f};
};

struct IntroLevelData {
    std::vector<WorldProp> props;
    std::vector<WorldObject> objects;
    std::vector<WorldZone> zones;
    std::vector<WorldLandmark> landmarks;
    std::vector<DistrictRewirPlan> districtRewirs;
    std::vector<DistrictRoutePlan> districtRoutePlans;
    std::vector<WorldViewpoint> visualBaselines;
    std::vector<LocationAnchor> locationAnchors;
    std::vector<MissionTriggerDefinition> missionTriggers;
    std::vector<RoutePoint> driveRoute;

    Vec3 playerStart{-12.0f, 0.0f, -7.0f};
    Vec3 npcPosition{-12.0f, 0.0f, -10.0f};
    Vec3 vehicleStart{-2.35f, 0.0f, 7.75f};
    Vec3 parkingExitPosition{-1.0f, 0.0f, 5.0f};
    Vec3 parkingLanePosition{-0.5f, 0.0f, 1.0f};
    Vec3 roadBendPosition{3.0f, 0.0f, -10.0f};
    Vec3 shopRoadPosition{8.0f, 0.0f, -18.0f};
    Vec3 shopEntrancePosition{18.0f, 0.0f, -22.35f};
    Vec3 shopPosition{18.0f, 0.0f, -18.0f};
    Vec3 zenonPosition{17.2f, 0.0f, -21.2f};
    Vec3 lolekPosition{9.0f, 0.0f, -4.2f};
    Vec3 receiptHolderPosition{2.2f, 0.0f, -12.2f};
    Vec3 dropoffPosition{-7.0f, 0.0f, 8.2f};
    Vec3 initialChaserPosition{20.0f, 0.0f, -24.0f};
};

DistrictPlanDebugOverlay buildDistrictPlanDebugOverlay(const IntroLevelData& level);
DistrictRouteTraversalReport inspectDistrictRouteVehicleTraversal(const IntroLevelData& level,
                                                                  const std::string& routeId,
                                                                  const WorldCollision& collision,
                                                                  VehicleCollisionConfig config = {});

class IntroLevelBuilder {
public:
    static IntroLevelData build();
    static IntroLevelData build(const IntroLevelBuildConfig& config);
    static IntroLevelData build(const IntroLevelBuildConfig& config, const WorldAssetRegistry& registry);
    static void populateWorld(const IntroLevelData& level, Scene& scene, WorldCollision& collision);
    static void populateInteractions(const IntroLevelData& level, WorldInteraction& interactions);
};

} // namespace bs3d
