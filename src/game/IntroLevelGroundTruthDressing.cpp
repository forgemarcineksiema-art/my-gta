#include "IntroLevelDressingSections.h"

#include "IntroLevelDressingUtils.h"

#include <string>
#include <utility>

namespace bs3d::IntroLevelDressingSections {

namespace {

constexpr float GroundPatchRenderY = 0.085f;

} // namespace

void addGroundTruthAndClutter(IntroLevelData& level) {
    auto objectTint = [](unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
        return IntroLevelDressingUtils::objectTint(r, g, b, a);
    };
    auto addDecor = [&](std::string id,
                        std::string assetId,
                        Vec3 position,
                        Vec3 scale,
                        WorldLocationTag tag,
                        std::vector<std::string> gameplayTags,
                        float yawRadians = 0.0f) {
        IntroLevelDressingUtils::addDecor(level,
                                          std::move(id),
                                          std::move(assetId),
                                          position,
                                          scale,
                                          tag,
                                          std::move(gameplayTags),
                                          yawRadians);
    };

    auto addTintedDecor = [&](std::string id,
                              std::string assetId,
                              Vec3 position,
                              Vec3 scale,
                              WorldLocationTag tag,
                              std::vector<std::string> gameplayTags,
                              WorldObjectTint tintOverride,
                              float yawRadians = 0.0f) {
        IntroLevelDressingUtils::addTintedDecor(level,
                                                std::move(id),
                                                std::move(assetId),
                                                position,
                                                scale,
                                                tag,
                                                std::move(gameplayTags),
                                                tintOverride,
                                                yawRadians);
    };
    auto addCurb = [&](std::string id, Vec3 position, Vec3 scale, WorldLocationTag tag, float yawRadians = 0.0f) {
        addDecor(std::move(id),
                 "curb_lowpoly",
                 position,
                 scale,
                 tag,
                 {"curb_ground_truth", "drive_readability", "artkit_v2"},
                 yawRadians);
    };

    addCurb("curb_parking_front", {-7.0f, 0.052f, 4.82f}, {16.2f, 0.08f, 0.10f}, WorldLocationTag::Parking);
    addCurb("curb_parking_back", {-7.0f, 0.052f, 12.28f}, {16.2f, 0.08f, 0.10f}, WorldLocationTag::Parking);
    addCurb("curb_parking_left", {-15.30f, 0.052f, 8.55f}, {0.10f, 0.08f, 7.55f}, WorldLocationTag::Parking);
    addCurb("curb_parking_right", {1.35f, 0.052f, 8.55f}, {0.10f, 0.08f, 7.55f}, WorldLocationTag::Parking);
    addCurb("curb_shop_apron_front", {18.0f, 0.055f, -23.20f}, {12.2f, 0.08f, 0.10f}, WorldLocationTag::Shop);
    addCurb("curb_shop_apron_back", {18.0f, 0.055f, -21.55f}, {12.2f, 0.08f, 0.10f}, WorldLocationTag::Shop);
    addCurb("curb_shop_apron_left", {11.82f, 0.055f, -22.35f}, {0.10f, 0.08f, 1.78f}, WorldLocationTag::Shop);
    addCurb("curb_shop_apron_right", {24.18f, 0.055f, -22.35f}, {0.10f, 0.08f, 1.78f}, WorldLocationTag::Shop);
    addCurb("curb_block_walk_front", {-16.0f, 0.055f, 10.88f}, {15.6f, 0.08f, 0.10f}, WorldLocationTag::Block);
    addCurb("curb_block_walk_back", {-16.0f, 0.055f, 12.18f}, {15.6f, 0.08f, 0.10f}, WorldLocationTag::Block);
    addCurb("curb_road_south_inner", {3.0f, 0.050f, -20.95f}, {48.5f, 0.075f, 0.095f}, WorldLocationTag::RoadLoop);
    addCurb("curb_road_south_outer", {3.0f, 0.050f, -25.35f}, {48.5f, 0.075f, 0.095f}, WorldLocationTag::RoadLoop);
    addCurb("curb_road_west_inner", {-28.80f, 0.050f, 0.0f}, {0.095f, 0.075f, 42.5f}, WorldLocationTag::RoadLoop);
    addCurb("curb_road_east_inner", {27.80f, 0.050f, 0.0f}, {0.095f, 0.075f, 45.5f}, WorldLocationTag::RoadLoop);
    addCurb("curb_garage_lane", {-18.0f, 0.052f, 20.95f}, {14.4f, 0.08f, 0.10f}, WorldLocationTag::Garage);

    auto addParkingFrame = [&](std::string id, std::string assetId, Vec3 position, Vec3 scale, float yawRadians = 0.0f) {
        addDecor(std::move(id),
                 std::move(assetId),
                 position,
                 scale,
                 WorldLocationTag::Parking,
                 {"parking_frame", "osiedle_clutter", "drive_readability"},
                 yawRadians);
    };

    for (int stop = 0; stop < 5; ++stop) {
        addParkingFrame("parking_stop_" + std::to_string(stop),
                        "parking_stop",
                        {-12.4f + static_cast<float>(stop) * 2.45f, 0.06f, 11.64f},
                        {1.10f, 0.16f, 0.30f});
    }
    for (int fence = 0; fence < 5; ++fence) {
        addParkingFrame("parking_fence_back_" + std::to_string(fence),
                        "fence_panel",
                        {-13.6f + static_cast<float>(fence) * 3.2f, 0.0f, 12.38f},
                        {2.45f, 0.82f, 0.10f});
    }
    addParkingFrame("parking_planter_left", "planter_concrete", {-15.80f, 0.0f, 5.10f}, {0.95f, 0.52f, 0.62f});
    addParkingFrame("parking_planter_right", "planter_concrete", {1.85f, 0.0f, 5.10f}, {0.95f, 0.52f, 0.62f});

    auto addFacade = [&](std::string id, std::string assetId, Vec3 position, Vec3 scale, float yawRadians = 0.0f) {
        addDecor(std::move(id),
                 std::move(assetId),
                 position,
                 scale,
                 WorldLocationTag::Block,
                 {"block_side_facade", "story_dressing"},
                 yawRadians);
    };

    for (int floor = 0; floor < 3; ++floor) {
        const float y = 1.55f + static_cast<float>(floor) * 2.15f;
        addFacade("block13_side_windows_w_" + std::to_string(floor),
                  "block_window_strip",
                  {-22.58f, y, 16.5f},
                  {0.08f, 0.62f, 2.25f});
        level.objects.back().gameplayTags.push_back("glass_surface");
        addFacade("block13_rear_windows_" + std::to_string(floor),
                  "block_window_strip",
                  {-16.0f, y, 19.58f},
                  {2.35f, 0.62f, 0.08f});
        level.objects.back().gameplayTags.push_back("glass_surface");
        addFacade("block13_side_stain_w_" + std::to_string(floor),
                  "wall_stain",
                  {-22.61f, y + 0.78f, 14.0f},
                  {0.05f, 0.50f, 1.85f});
        addFacade("block13_rear_stain_" + std::to_string(floor),
                  "wall_stain",
                  {-12.6f, y + 0.72f, 19.61f},
                  {1.45f, 0.46f, 0.05f});
    }

    const float shopFrontZ = level.shopPosition.z - 3.10f;

    auto addClutter = [&](std::string id,
                          std::string assetId,
                          Vec3 position,
                          Vec3 scale,
                          WorldLocationTag tag,
                          std::vector<std::string> tags,
                          float yawRadians = 0.0f) {
        tags.push_back("osiedle_clutter");
        addDecor(std::move(id), std::move(assetId), position, scale, tag, std::move(tags), yawRadians);
    };

    addClutter("lamp_block_corner", "lamp_post_lowpoly", {-9.2f, 0.0f, 8.7f}, {0.18f, 3.2f, 0.18f}, WorldLocationTag::Block, {"vertical_readability", "block_cluster", "artkit_v2"});
    addClutter("lamp_parking_left", "lamp_post_lowpoly", {-16.9f, 0.0f, 5.45f}, {0.18f, 3.1f, 0.18f}, WorldLocationTag::Parking, {"vertical_readability", "artkit_v2"});
    addClutter("lamp_parking_right", "lamp_post_lowpoly", {2.8f, 0.0f, 11.95f}, {0.18f, 3.1f, 0.18f}, WorldLocationTag::Parking, {"vertical_readability", "artkit_v2"});
    addClutter("lamp_shop_corner", "lamp_post_lowpoly", {24.9f, 0.0f, -22.9f}, {0.18f, 3.0f, 0.18f}, WorldLocationTag::Shop, {"vertical_readability", "artkit_v2"});
    addClutter("sign_no_parking", "street_sign", {-3.65f, 0.0f, 5.7f}, {0.58f, 1.75f, 0.08f}, WorldLocationTag::Parking, {"vertical_readability", "drive_readability", "parking_sign"});
    addClutter("sign_shop_delivery", "street_sign", {12.6f, 0.0f, -23.55f}, {0.62f, 1.55f, 0.08f}, WorldLocationTag::Shop, {"vertical_readability", "shop_identity", "shop_cluster"});
    addClutter("shop_poster_left", "shop_poster", {14.7f, 0.52f, shopFrontZ - 0.10f}, {0.55f, 0.78f, 0.05f}, WorldLocationTag::Shop, {"shop_identity", "shop_cluster"});
    addClutter("shop_poster_right", "shop_poster", {21.2f, 0.52f, shopFrontZ - 0.10f}, {0.55f, 0.78f, 0.05f}, WorldLocationTag::Shop, {"shop_identity", "shop_cluster"});
    addClutter("shop_cardboard_by_door", "cardboard_stack", {20.6f, 0.0f, -23.02f}, {0.55f, 0.42f, 0.45f}, WorldLocationTag::Shop, {"shop_identity", "shop_cluster"});
    addClutter("shop_planter_corner", "planter_concrete", {13.0f, 0.0f, -22.92f}, {0.82f, 0.42f, 0.55f}, WorldLocationTag::Shop, {"shop_identity", "shop_cluster"});
    addClutter("block_notice_stand", "notice_board", {-18.7f, 0.0f, 10.65f}, {1.45f, 1.05f, 0.10f}, WorldLocationTag::Block, {"vertical_readability", "story_dressing", "block_cluster"});
    addClutter("block_planter_0", "planter_concrete", {-13.4f, 0.0f, 10.72f}, {1.05f, 0.46f, 0.58f}, WorldLocationTag::Block, {"story_dressing", "block_cluster"});
    addClutter("block_planter_1", "planter_concrete", {-21.7f, 0.0f, 10.72f}, {1.05f, 0.46f, 0.58f}, WorldLocationTag::Block, {"story_dressing", "block_cluster"});
    addClutter("garage_tool_crate", "cardboard_stack", {-12.0f, 0.0f, 20.45f}, {0.80f, 0.60f, 0.62f}, WorldLocationTag::Garage, {"garage_identity", "garage_cluster"});
    addClutter("garage_oil_sign", "street_sign", {-24.1f, 0.0f, 20.75f}, {0.50f, 1.20f, 0.08f}, WorldLocationTag::Garage, {"garage_identity", "vertical_readability", "garage_cluster"});
    addClutter("garage_tire_stack_placeholder", "concrete_barrier", {-14.2f, 0.0f, 20.50f}, {0.72f, 0.55f, 0.52f}, WorldLocationTag::Garage, {"garage_identity", "garage_cluster"});
    addClutter("trash_loose_cardboard", "cardboard_stack", {11.2f, 0.0f, -3.85f}, {0.60f, 0.38f, 0.50f}, WorldLocationTag::Trash, {"trash_dressing", "trash_cluster"});
    addClutter("road_small_barrier_0", "concrete_barrier", {5.2f, 0.0f, -14.9f}, {1.50f, 0.55f, 0.42f}, WorldLocationTag::RoadLoop, {"drive_readability"});
    addClutter("road_small_barrier_1", "concrete_barrier", {13.5f, 0.0f, -18.8f}, {1.50f, 0.55f, 0.42f}, WorldLocationTag::RoadLoop, {"drive_readability"});

    auto addGroundPatch = [&](std::string id,
                              std::string assetId,
                              Vec3 position,
                              Vec3 scale,
                              WorldLocationTag tag,
                              std::vector<std::string> tags,
                              float yawRadians = 0.0f) {
        tags.push_back("ground_patch");
        if (assetId.find("irregular_") == 0) {
            tags.push_back("irregular_ground_patch");
            tags.push_back("artkit_v2");
            WorldObjectTint patchTint = objectTint(90, 92, 82);
            if (assetId == "irregular_asphalt_patch") {
                patchTint = objectTint(68, 70, 66, 128);
            } else if (assetId == "irregular_grass_patch") {
                patchTint = objectTint(82, 105, 73, 118);
            } else if (assetId == "irregular_dirt_patch") {
                patchTint = objectTint(104, 88, 67, 126);
            }
            addTintedDecor(std::move(id), std::move(assetId), position, scale, tag, std::move(tags), patchTint, yawRadians);
            return;
        }
        addDecor(std::move(id), std::move(assetId), position, scale, tag, std::move(tags), yawRadians);
    };

    addGroundPatch("asphalt_patch_parking_0", "irregular_asphalt_patch", {-12.2f, GroundPatchRenderY, 7.2f}, {3.2f, 0.009f, 1.25f}, WorldLocationTag::Parking, {"asphalt_patch", "parking_frame"});
    addGroundPatch("asphalt_patch_parking_1", "irregular_asphalt_patch", {-5.3f, GroundPatchRenderY, 10.8f}, {2.8f, 0.009f, 1.10f}, WorldLocationTag::Parking, {"asphalt_patch", "parking_frame"}, 0.25f);
    addGroundPatch("asphalt_patch_shop_0", "irregular_asphalt_patch", {17.2f, GroundPatchRenderY, -21.95f}, {3.6f, 0.009f, 0.85f}, WorldLocationTag::Shop, {"asphalt_patch", "shop_cluster"});
    addGroundPatch("asphalt_patch_shop_1", "irregular_asphalt_patch", {21.5f, GroundPatchRenderY, -22.85f}, {2.1f, 0.009f, 0.75f}, WorldLocationTag::Shop, {"asphalt_patch", "shop_cluster"}, -0.18f);
    addGroundPatch("asphalt_patch_road_south_0", "irregular_asphalt_patch", {1.0f, GroundPatchRenderY, -22.8f}, {5.2f, 0.009f, 1.10f}, WorldLocationTag::RoadLoop, {"asphalt_patch"}, 0.12f);
    addGroundPatch("asphalt_patch_road_south_1", "irregular_asphalt_patch", {10.7f, GroundPatchRenderY, -23.6f}, {4.6f, 0.009f, 0.95f}, WorldLocationTag::RoadLoop, {"asphalt_patch"}, -0.22f);
    addGroundPatch("asphalt_patch_road_west_0", "irregular_asphalt_patch", {-31.0f, GroundPatchRenderY, -2.0f}, {1.0f, 0.009f, 4.4f}, WorldLocationTag::RoadLoop, {"asphalt_patch"});
    addGroundPatch("garage_oil_stain_0", "irregular_asphalt_patch", {-17.7f, GroundPatchRenderY, 20.55f}, {2.2f, 0.008f, 0.95f}, WorldLocationTag::Garage, {"asphalt_patch", "garage_cluster"}, 0.12f);

    addGroundPatch("grass_wear_bogus_path_0", "irregular_grass_patch", {-12.1f, GroundPatchRenderY, 4.8f}, {2.2f, 0.007f, 4.4f}, WorldLocationTag::Block, {"grass_patch", "block_cluster"}, -0.15f);
    addGroundPatch("grass_wear_bogus_path_1", "irregular_grass_patch", {-9.4f, GroundPatchRenderY, 8.0f}, {1.6f, 0.007f, 2.8f}, WorldLocationTag::Block, {"grass_patch", "block_cluster"}, 0.25f);
    addGroundPatch("grass_wear_shop_shortcut", "irregular_grass_patch", {10.7f, GroundPatchRenderY, -12.0f}, {2.0f, 0.007f, 4.2f}, WorldLocationTag::RoadLoop, {"grass_patch"}, 0.35f);
    addGroundPatch("grass_wear_trash_0", "irregular_grass_patch", {8.8f, GroundPatchRenderY, -2.9f}, {2.8f, 0.007f, 1.4f}, WorldLocationTag::Trash, {"grass_patch", "trash_cluster"});
    addGroundPatch("grass_wear_garage_0", "irregular_grass_patch", {-18.0f, GroundPatchRenderY, 19.2f}, {4.2f, 0.007f, 1.25f}, WorldLocationTag::Garage, {"grass_patch", "garage_cluster"});
    addGroundPatch("grass_wear_block_side", "irregular_grass_patch", {-23.2f, GroundPatchRenderY, 8.0f}, {1.7f, 0.007f, 4.5f}, WorldLocationTag::Block, {"grass_patch", "block_cluster"});
    addGroundPatch("dirt_patch_under_fence", "irregular_dirt_patch", {-7.0f, GroundPatchRenderY, 12.25f}, {7.5f, 0.007f, 0.75f}, WorldLocationTag::Parking, {"grass_patch", "parking_frame"});
}

} // namespace bs3d::IntroLevelDressingSections
