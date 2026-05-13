#include "IntroLevelBuilder.h"

#include "IntroLevelAuthoring.h"
#include "WorldAssetRegistry.h"

#include "bs3d/core/WorldServiceState.h"

#include <algorithm>

namespace bs3d {

namespace {

bool hasTag(const WorldObject& object, const std::string& tag) {
    for (const std::string& objectTag : object.gameplayTags) {
        if (objectTag == tag) {
            return true;
        }
    }
    return false;
}

bool isSmallPhysicalAsset(const WorldObject& object) {
    return object.assetId == "bench" ||
           object.assetId == "parking_stop" ||
           object.assetId == "fence_panel" ||
           object.assetId == "planter_concrete" ||
           object.assetId == "lamp_post_lowpoly" ||
           object.assetId == "street_sign" ||
           object.assetId == "bollard_red" ||
           object.assetId == "trash_bin_lowpoly" ||
           object.assetId == "cardboard_stack" ||
           object.assetId == "concrete_barrier" ||
           object.assetId == "trzepak" ||
           object.assetId == "swing_set" ||
           object.assetId == "slide_small" ||
           object.assetId == "basketball_hoop";
}

bool isDynamicPropAsset(const WorldObject& object) {
    return object.assetId == "trash_bin_lowpoly" ||
           object.assetId == "cardboard_stack" ||
           object.assetId == "bollard_red" ||
           object.assetId == "concrete_barrier";
}

bool isDecorativeGroundPatchAsset(const WorldObject& object, const WorldAssetRegistry* registry) {
    if (registry != nullptr) {
        const WorldAssetDefinition* definition = registry->find(object.assetId);
        if (definition != nullptr) {
            return definition->renderBucket == "Decal" || definition->defaultCollision == "None";
        }
    }
    return object.assetId == "irregular_asphalt_patch" ||
           object.assetId == "irregular_grass_patch" ||
           object.assetId == "irregular_dirt_patch" ||
           object.assetId == "asphalt_patch" ||
           object.assetId == "grass_wear_patch" ||
           object.assetId == "oil_stain";
}

bool isPlayerSoftCollisionAsset(const WorldObject& object) {
    return object.assetId == "street_sign" ||
           object.assetId == "lamp_post_lowpoly" ||
           object.assetId == "bollard_red" ||
           object.assetId == "parking_stop" ||
           object.assetId == "fence_panel" ||
           object.assetId == "cardboard_stack" ||
           object.assetId == "planter_concrete" ||
           (object.assetId == "concrete_barrier" &&
            object.scale.y <= 0.70f &&
            std::max(object.scale.x, object.scale.z) <= 1.75f);
}

void addTagIfMissing(WorldObject& object, const std::string& tag) {
    if (!hasTag(object, tag)) {
        object.gameplayTags.push_back(tag);
    }
}

CollisionProfile softVehiclePropProfile(bool dynamic) {
    CollisionProfile profile = CollisionProfile::vehicleBlocker();
    profile.responseKind = dynamic ? CollisionResponseKind::PropDynamicLite : CollisionResponseKind::PropStatic;
    if (dynamic) {
        profile.layers |= collisionLayerMask(CollisionLayer::DynamicProp);
    }
    profile.blocksCamera = false;
    return profile;
}

bool isPropSimulationOwned(CollisionResponseKind responseKind) {
    return responseKind == CollisionResponseKind::PropStatic ||
           responseKind == CollisionResponseKind::PropFakeDynamic ||
           responseKind == CollisionResponseKind::PropDynamicLite ||
           responseKind == CollisionResponseKind::BreakableLite;
}

void applyAssetCollisionMetadata(WorldObject& object, const WorldAssetRegistry* registry) {
    if (registry == nullptr) {
        return;
    }
    const WorldAssetDefinition* definition = registry->find(object.assetId);
    if (definition == nullptr) {
        return;
    }
    object.collisionProfile.blocksCamera = definition->cameraBlocks;
    if (definition->cameraBlocks) {
        object.collisionProfile.layers |= collisionLayerMask(CollisionLayer::CameraBlocker);
    } else {
        object.collisionProfile.layers &= ~collisionLayerMask(CollisionLayer::CameraBlocker);
    }
}

void normalizeCollisionAuthoring(WorldObject& object, const WorldAssetRegistry* registry) {
    if (object.collision.kind == WorldCollisionShapeKind::Unspecified) {
        object.collision.kind = WorldCollisionShapeKind::None;
    }

    if (isDecorativeGroundPatchAsset(object, registry)) {
        object.collision.kind = WorldCollisionShapeKind::None;
        object.collisionProfile = CollisionProfile::noCollision();
        applyAssetCollisionMetadata(object, registry);
        return;
    }

    const bool physicalByTag = hasTag(object, "collision_toy") ||
                               hasTag(object, "physical_prop") ||
                               hasTag(object, "dynamic_prop") ||
                               hasTag(object, "parking_frame") ||
                               object.id == "bogus_bench";
    const bool smallPhysical = physicalByTag || isSmallPhysicalAsset(object);

    if (object.collision.kind == WorldCollisionShapeKind::None && smallPhysical) {
        object.collision.kind = object.yawRadians == 0.0f ? WorldCollisionShapeKind::Box : WorldCollisionShapeKind::OrientedBox;
        object.collision.offset = {0.0f, object.scale.y * 0.5f, 0.0f};
        object.collision.size = object.scale;
        object.collision.yawRadians = 0.0f;
        addTagIfMissing(object, "physical_prop");
    }

    if (isDynamicPropAsset(object)) {
        addTagIfMissing(object, "dynamic_prop");
    }

    if (isPlayerSoftCollisionAsset(object)) {
        addTagIfMissing(object, "soft_player_collision");
    }

    if (object.assetId == "cardboard_stack") {
        addTagIfMissing(object, "breakable_lite");
    }

    if (object.assetId == "trash_bin_lowpoly") {
        addTagIfMissing(object, "walkable_prop_top");
    }

    if (object.collisionProfile.responseKind != CollisionResponseKind::Unspecified) {
        applyAssetCollisionMetadata(object, registry);
        return;
    }

    switch (object.collision.kind) {
    case WorldCollisionShapeKind::Box:
    case WorldCollisionShapeKind::OrientedBox:
        if (hasTag(object, "soft_player_collision")) {
            object.collisionProfile = softVehiclePropProfile(hasTag(object, "dynamic_prop"));
            if (hasTag(object, "breakable_lite")) {
                object.collisionProfile.responseKind = CollisionResponseKind::BreakableLite;
            }
        } else if (hasTag(object, "dynamic_prop")) {
            object.collisionProfile = CollisionProfile::dynamicProp();
        } else if (object.id == "bogus_bench") {
            object.collisionProfile = CollisionProfile::playerBlocker();
        } else if (smallPhysical) {
            object.collisionProfile = CollisionProfile::dynamicProp();
            object.collisionProfile.responseKind = CollisionResponseKind::PropStatic;
            object.collisionProfile.layers &= ~collisionLayerMask(CollisionLayer::DynamicProp);
        } else {
            object.collisionProfile = CollisionProfile::solidWorld();
        }
        break;
    case WorldCollisionShapeKind::GroundBox:
    case WorldCollisionShapeKind::RampZ:
        object.collisionProfile = CollisionProfile::walkableSurface();
        break;
    case WorldCollisionShapeKind::TriggerSphere:
    case WorldCollisionShapeKind::TriggerBox:
        object.collisionProfile = CollisionProfile::trigger(CollisionLayer::InteractionTrigger);
        break;
    case WorldCollisionShapeKind::None:
    case WorldCollisionShapeKind::Unspecified:
        object.collisionProfile = CollisionProfile::noCollision();
        break;
    }

    applyAssetCollisionMetadata(object, registry);
}

void normalizeCollisionAuthoring(IntroLevelData& level, const WorldAssetRegistry* registry) {
    for (WorldObject& object : level.objects) {
        normalizeCollisionAuthoring(object, registry);
    }
}

} // namespace

IntroLevelData IntroLevelBuilder::build() {
    return build(IntroLevelBuildConfig{});
}

IntroLevelData IntroLevelBuilder::build(const IntroLevelBuildConfig& config) {
    IntroLevelData level;
    level.playerStart = config.playerStart;
    level.npcPosition = config.npcPosition;
    level.vehicleStart = config.vehicleStart;
    level.parkingExitPosition = config.parkingExitPosition;
    level.parkingLanePosition = config.parkingLanePosition;
    level.roadBendPosition = config.roadBendPosition;
    level.shopRoadPosition = config.shopRoadPosition;
    level.shopEntrancePosition = config.shopEntrancePosition;
    level.shopPosition = config.shopPosition;
    level.zenonPosition = config.zenonPosition;
    level.lolekPosition = config.lolekPosition;
    level.receiptHolderPosition = config.receiptHolderPosition;
    level.dropoffPosition = config.dropoffPosition;
    level.initialChaserPosition = config.initialChaserPosition;

    IntroLevelAuthoring::addCoreLayout(level);
    IntroLevelAuthoring::addDressing(level);
    IntroLevelAuthoring::addGameplayData(level);
    normalizeCollisionAuthoring(level, nullptr);
    return level;
}

IntroLevelData IntroLevelBuilder::build(const IntroLevelBuildConfig& config, const WorldAssetRegistry& registry) {
    IntroLevelData level;
    level.playerStart = config.playerStart;
    level.npcPosition = config.npcPosition;
    level.vehicleStart = config.vehicleStart;
    level.parkingExitPosition = config.parkingExitPosition;
    level.parkingLanePosition = config.parkingLanePosition;
    level.roadBendPosition = config.roadBendPosition;
    level.shopRoadPosition = config.shopRoadPosition;
    level.shopEntrancePosition = config.shopEntrancePosition;
    level.shopPosition = config.shopPosition;
    level.zenonPosition = config.zenonPosition;
    level.lolekPosition = config.lolekPosition;
    level.receiptHolderPosition = config.receiptHolderPosition;
    level.dropoffPosition = config.dropoffPosition;
    level.initialChaserPosition = config.initialChaserPosition;

    IntroLevelAuthoring::addCoreLayout(level);
    IntroLevelAuthoring::addDressing(level);
    IntroLevelAuthoring::addGameplayData(level);
    normalizeCollisionAuthoring(level, &registry);
    return level;
}

DistrictPlanDebugOverlay buildDistrictPlanDebugOverlay(const IntroLevelData& level) {
    DistrictPlanDebugOverlay overlay;
    overlay.rewirMarkers.reserve(level.districtRewirs.size());

    for (const DistrictRewirPlan& rewir : level.districtRewirs) {
        DistrictPlanDebugRewir marker;
        marker.id = rewir.id;
        marker.label = rewir.label;
        marker.center = rewir.center;
        marker.radius = rewir.radius;
        marker.playableNow = rewir.playableNow;
        marker.homeBase = rewir.homeBase;
        marker.drivingSpine = rewir.drivingSpine;
        marker.serviceEconomy = rewir.serviceEconomy;
        marker.vehicleIdentity = rewir.vehicleIdentity;
        overlay.rewirMarkers.push_back(marker);

        if (!rewir.playableNow) {
            ++overlay.futureRewirCount;
        }
    }

    for (const DistrictRoutePlan& route : level.districtRoutePlans) {
        if (route.vehicleRoute) {
            ++overlay.vehicleRouteCount;
        }
        if (route.points.size() < 2) {
            continue;
        }

        for (std::size_t i = 1; i < route.points.size(); ++i) {
            DistrictPlanDebugRouteSegment segment;
            segment.routeId = route.id;
            segment.from = route.points[i - 1].position;
            segment.to = route.points[i].position;
            segment.vehicleRoute = route.vehicleRoute;
            segment.futureExpansion = route.futureExpansion;
            overlay.routeSegments.push_back(segment);
        }
    }

    return overlay;
}

DistrictRouteTraversalReport inspectDistrictRouteVehicleTraversal(const IntroLevelData& level,
                                                                  const std::string& routeId,
                                                                  const WorldCollision& collision,
                                                                  VehicleCollisionConfig config) {
    DistrictRouteTraversalReport report;
    report.routeId = routeId;

    const DistrictRoutePlan* targetRoute = nullptr;
    for (const DistrictRoutePlan& route : level.districtRoutePlans) {
        if (route.id == routeId) {
            targetRoute = &route;
            break;
        }
    }

    if (targetRoute == nullptr) {
        return report;
    }

    report.foundRoute = true;
    if (targetRoute->points.size() < 2) {
        return report;
    }

    report.checkedSegments = static_cast<int>(targetRoute->points.size() - 1);
    constexpr float ArrivalPadding = 0.35f;
    constexpr int MaxStepAttemptsPerSegment = 120;

    Vec3 current = targetRoute->points.front().position;
    report.resolvedPosition = current;
    const float stepDistance = std::max(0.45f, config.radius * 0.65f);

    for (std::size_t i = 1; i < targetRoute->points.size(); ++i) {
        const RoutePoint& from = targetRoute->points[i - 1];
        const RoutePoint& to = targetRoute->points[i];
        bool reached = distanceXZ(current, to.position) <= to.radius + ArrivalPadding;

        for (int attempt = 0; attempt < MaxStepAttemptsPerSegment && !reached; ++attempt) {
            const Vec3 toTarget = to.position - current;
            const float remaining = distanceXZ(current, to.position);
            if (remaining <= to.radius + ArrivalPadding || lengthSquaredXZ(toTarget) <= 0.0001f) {
                reached = true;
                break;
            }

            const Vec3 direction = normalizeXZ(toTarget);
            const Vec3 desired = current + direction * std::min(stepDistance, remaining);
            VehicleCollisionConfig stepConfig = config;
            if (lengthSquaredXZ(stepConfig.velocity) <= 0.0001f) {
                stepConfig.velocity = direction * std::max(1.0f, stepConfig.speed);
            }

            const VehicleCollisionResult resolved =
                collision.resolveVehicleCapsule(current, desired, yawFromDirection(direction), stepConfig);
            const float movement = distanceXZ(resolved.position, current);
            current = resolved.position;
            report.resolvedPosition = current;
            reached = distanceXZ(current, to.position) <= to.radius + ArrivalPadding;

            if (!reached && movement <= 0.03f) {
                break;
            }
        }

        if (!reached && report.blockedSegmentLabel.empty()) {
            report.clear = false;
            report.blockedSegmentLabel = from.label + " -> " + to.label;
            report.blockedFrom = from.position;
            report.blockedTo = to.position;
            report.resolvedPosition = current;
            break;
        }
    }

    report.clear = report.blockedSegmentLabel.empty();
    return report;
}

void IntroLevelBuilder::populateWorld(const IntroLevelData& level, Scene& scene, WorldCollision& collision) {
    collision.clear();
    collision.addGroundPlane(0.0f);
    scene.clear();

    for (const WorldObject& object : level.objects) {
        Transform transform;
        transform.position = object.position;
        transform.yawRadians = object.yawRadians;
        transform.scale = object.scale;
        scene.createObject(object.assetId, transform);

        const Vec3 collisionCenter = object.position + object.collision.offset;
        if (isPropSimulationOwned(object.collisionProfile.responseKind)) {
            continue;
        }
        switch (object.collision.kind) {
        case WorldCollisionShapeKind::Box:
            collision.addBox(collisionCenter, object.collision.size, object.collisionProfile);
            break;
        case WorldCollisionShapeKind::OrientedBox:
            collision.addOrientedBox(collisionCenter,
                                     object.collision.size,
                                     object.collision.yawRadians + object.yawRadians,
                                     object.collisionProfile);
            break;
        case WorldCollisionShapeKind::GroundBox:
            collision.addGroundBox(collisionCenter,
                                   object.collision.size,
                                   object.collision.yawRadians + object.yawRadians,
                                   object.collisionProfile);
            break;
        case WorldCollisionShapeKind::RampZ:
            collision.addRamp(collisionCenter,
                              object.collision.size,
                              object.collision.startHeight,
                              object.collision.endHeight,
                              object.collisionProfile);
            break;
        case WorldCollisionShapeKind::TriggerSphere:
        case WorldCollisionShapeKind::TriggerBox:
            break;
        case WorldCollisionShapeKind::None:
        case WorldCollisionShapeKind::Unspecified:
            break;
        }
    }
}

void IntroLevelBuilder::populateInteractions(const IntroLevelData& level, WorldInteraction& interactions) {
    interactions.clear();
    interactions.addPoint({"npc_start", "E: pogadaj z Bogusiem", level.npcPosition, 2.2f});
    interactions.addPoint({"npc_zenon", "E: pogadaj z Zenonem", level.zenonPosition, 2.4f});
    interactions.addPoint({"npc_lolek", "E: pogadaj z Lolkiem", level.lolekPosition, 2.3f});
    interactions.addPoint({"receipt_holder", "E: typ z paragonem", level.receiptHolderPosition, 2.2f});
    for (const LocalRewirServiceSpec& service : localRewirServiceCatalog()) {
        interactions.addPoint({service.interactionPointId,
                               service.normalPrompt,
                               service.interactionPosition,
                               service.interactionRadius});
    }
    for (const LocalRewirFavorSpec& favor : localRewirFavorCatalog()) {
        interactions.addPoint({favor.interactionPointId,
                               favor.prompt,
                               favor.interactionPosition,
                               favor.interactionRadius});
    }
    interactions.addPoint({"shop", "Cel: sklep Zenona", level.shopEntrancePosition, 3.2f});
    interactions.addPoint({"dropoff", "Cel: parking pod blokiem", level.dropoffPosition, 4.8f});
}

} // namespace bs3d
