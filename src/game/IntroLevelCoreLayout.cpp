#include "IntroLevelAuthoring.h"

namespace bs3d {

namespace {

constexpr float RoadSurfaceY = 0.0f;
constexpr float MainArterySurfaceY = -0.006f;
constexpr float ConnectorSurfaceY = 0.004f;
constexpr float ParkingSurfaceY = 0.010f;

WorldCollisionShape noCollision() {
    return {WorldCollisionShapeKind::None, {}, {}};
}

WorldCollisionShape boxCollisionFromBase(Vec3 size) {
    return {WorldCollisionShapeKind::Box, {0.0f, size.y * 0.5f, 0.0f}, size};
}

WorldCollisionShape boxCollisionCentered(Vec3 size, Vec3 offset = {}) {
    return {WorldCollisionShapeKind::Box, offset, size};
}

WorldCollisionShape groundBoxCollision(Vec3 size, Vec3 offset = {}) {
    return {WorldCollisionShapeKind::GroundBox, offset, size};
}

WorldCollisionShape rampCollision(Vec3 size, float startHeight, float endHeight, Vec3 offset = {}) {
    WorldCollisionShape collision;
    collision.kind = WorldCollisionShapeKind::RampZ;
    collision.offset = offset;
    collision.size = size;
    collision.startHeight = startHeight;
    collision.endHeight = endHeight;
    return collision;
}

} // namespace

void IntroLevelAuthoring::addCoreLayout(IntroLevelData& level) {
    level.driveRoute = {
        {level.parkingExitPosition, 2.9f, "Wyjazd"},
        {level.parkingLanePosition, 3.0f, "Alejka"},
        {level.roadBendPosition, 3.1f, "Zakręt"},
        {level.shopRoadPosition, 3.5f, "Droga"},
        {level.shopEntrancePosition, 4.2f, "Pod sklep"}
    };

    level.objects = {
        {"block13_core",
         "block13_core",
         {-16.0f, 0.0f, 16.0f},
         {13.0f, 9.0f, 7.0f},
         0.0f,
         boxCollisionFromBase({13.0f, 9.0f, 7.0f}),
         WorldLocationTag::Block,
         {"landmark", "halina_windows", "hero_asset", "artkit_v2"}},
        {"block13_neighbor",
         "block13_core",
         {5.0f, 0.0f, 17.0f},
         {14.0f, 7.0f, 6.0f},
         0.0f,
         boxCollisionFromBase({14.0f, 7.0f, 6.0f}),
         WorldLocationTag::Block,
         {"future_district"}},
        {"block13_side",
         "block13_core",
         {-25.0f, 0.0f, -7.0f},
         {6.0f, 5.0f, 11.0f},
         0.0f,
         boxCollisionFromBase({6.0f, 5.0f, 11.0f}),
         WorldLocationTag::Block,
         {"future_district"}},
        {"shop_zenona",
         "shop_zenona_v3",
         level.shopPosition,
         {8.0f, 3.5f, 6.0f},
         0.0f,
         boxCollisionFromBase({8.0f, 3.5f, 6.0f}),
         WorldLocationTag::Shop,
         {"mission", "shop_trouble", "landmark", "hero_asset", "artkit_v2"}},
        {"parking_surface",
         "parking_surface",
         {-7.0f, ParkingSurfaceY, 8.6f},
         {15.0f, 0.08f, 7.4f},
         0.0f,
         groundBoxCollision({15.0f, 0.10f, 7.4f}, {0.0f, -ParkingSurfaceY, 0.0f}),
         WorldLocationTag::Parking,
         {"drive_space"}},
        {"road_loop_south",
         "road_asphalt",
         {3.0f, RoadSurfaceY, -23.2f},
         {48.0f, 0.07f, 4.2f},
         0.0f,
         noCollision(),
         WorldLocationTag::RoadLoop,
         {"drive_route"}},
        {"road_loop_north",
         "road_asphalt",
         {16.0f, RoadSurfaceY, 22.0f},
         {22.0f, 0.07f, 3.0f},
         0.0f,
         noCollision(),
         WorldLocationTag::RoadLoop,
         {"drive_route"}},
        {"road_loop_west",
         "road_asphalt",
         {-31.0f, RoadSurfaceY, 0.0f},
         {4.0f, 0.07f, 44.0f},
         0.0f,
         noCollision(),
         WorldLocationTag::RoadLoop,
         {"drive_route"}},
        {"road_loop_east",
         "road_asphalt",
         {30.0f, RoadSurfaceY, 0.0f},
         {4.0f, 0.07f, 47.0f},
         0.0f,
         noCollision(),
         WorldLocationTag::RoadLoop,
         {"drive_route"}},
        {"road_parking_connector",
         "road_asphalt",
         {-0.2f, ConnectorSurfaceY, -4.0f},
         {7.4f, 0.07f, 22.0f},
         0.0f,
         noCollision(),
         WorldLocationTag::RoadLoop,
         {"drive_route"}},
        {"road_shop_connector",
         "road_asphalt",
         {5.5f, ConnectorSurfaceY, -18.2f},
         {15.0f, 0.07f, 6.0f},
         0.0f,
         noCollision(),
         WorldLocationTag::RoadLoop,
         {"drive_route"}},
        {"main_artery_spine",
         "road_asphalt",
         {15.0f, MainArterySurfaceY, -27.4f},
         {32.0f, 0.07f, 11.2f},
         0.0f,
         noCollision(),
         WorldLocationTag::RoadLoop,
         {"drive_route", "main_artery_surface", "future_expansion"}},
        {"garage_row_rysiek",
         "garage_row_v2",
         {-18.0f, 0.0f, 23.0f},
         {13.0f, 2.4f, 3.2f},
         0.0f,
         boxCollisionFromBase({13.0f, 2.4f, 3.2f}),
         WorldLocationTag::Garage,
         {"garage", "future_base", "rysiek", "hero_asset", "artkit_v2"}},
        {"trash_shelter_shop_side",
         "trash_shelter",
         {9.0f, 0.0f, -4.0f},
         {3.5f, 1.4f, 2.0f},
         0.0f,
         boxCollisionFromBase({3.5f, 1.4f, 2.0f}),
         WorldLocationTag::Trash,
         {"trash", "trash_cluster", "public_nuisance"}},
        {"low_concrete_barrier",
         "concrete_barrier",
         {-2.0f, 0.0f, -17.5f},
         {4.0f, 1.2f, 1.6f},
         0.0f,
         boxCollisionFromBase({4.0f, 1.2f, 1.6f}),
         WorldLocationTag::RoadLoop,
         {"collision_toy"}},
        {"future_rewir_wall_west",
         "boundary_wall",
         {-34.0f, 0.0f, 0.0f},
         {1.0f, 1.2f, 54.0f},
         0.0f,
         boxCollisionFromBase({1.0f, 1.2f, 54.0f}),
         WorldLocationTag::Unknown,
         {"boundary"}},
        {"future_rewir_wall_east",
         "boundary_wall",
         {34.0f, 0.0f, 0.0f},
         {1.0f, 1.2f, 54.0f},
         0.0f,
         boxCollisionFromBase({1.0f, 1.2f, 54.0f}),
         WorldLocationTag::Unknown,
         {"boundary"}},
        {"future_rewir_wall_south",
         "boundary_wall",
         {-19.0f, 0.0f, -26.0f},
         {30.0f, 1.2f, 1.0f},
         0.0f,
         boxCollisionFromBase({30.0f, 1.2f, 1.0f}),
         WorldLocationTag::Unknown,
         {"boundary"}},
        {"future_rewir_wall_south_east",
         "boundary_wall",
         {31.0f, 0.0f, -26.0f},
         {6.0f, 1.2f, 1.0f},
         0.0f,
         boxCollisionFromBase({6.0f, 1.2f, 1.0f}),
         WorldLocationTag::Unknown,
         {"boundary"}},
        {"future_rewir_wall_north",
         "boundary_wall",
         {0.0f, 0.0f, 26.0f},
         {68.0f, 1.2f, 1.0f},
         0.0f,
         boxCollisionFromBase({68.0f, 1.2f, 1.0f}),
         WorldLocationTag::Unknown,
         {"boundary"}},
        {"curb_ramp_parking",
         "curb_ramp",
         {-7.0f, 0.0f, 6.8f},
         {10.0f, 0.22f, 1.2f},
         0.0f,
         groundBoxCollision({10.0f, 0.22f, 1.2f}),
         WorldLocationTag::Parking,
         {"curb"}},
        {"shop_side_ramp",
         "curb_ramp",
         {14.0f, 0.0f, -8.0f},
         {5.0f, 0.08f, 5.0f},
         0.0f,
         rampCollision({5.0f, 0.0f, 5.0f}, 0.0f, 0.55f),
         WorldLocationTag::Shop,
         {"curb"}},
        {"spoldzielnia_notice",
         "notice_board",
         {-20.0f, 0.0f, 7.9f},
         {3.1f, 1.6f, 0.16f},
         0.0f,
         noCollision(),
         WorldLocationTag::Block,
         {"story_dressing"}},
        {"bogus_bench",
         "bench",
         {-10.8f, 0.0f, 11.9f},
         {2.5f, 1.25f, 0.14f},
         0.0f,
         noCollision(),
         WorldLocationTag::Block,
         {"bogus", "interaction"}},
        {"developer_survey_marker",
         "developer_sign",
         {-2.8f, 0.0f, 7.6f},
         {0.32f, 1.25f, 0.32f},
         0.0f,
         noCollision(),
         WorldLocationTag::Parking,
         {"developer", "story_dressing"}}
    };
}

} // namespace bs3d
