#include "IntroLevelDressingSections.h"

#include "IntroLevelDressingUtils.h"

#include <string>
#include <utility>

namespace bs3d::IntroLevelDressingSections {

void addIdentityDressing(IntroLevelData& level) {
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
    const float shopFrontZ = level.shopPosition.z - 3.10f;
    addDecor("shop_sign_zenona_front",
             "shop_sign_zenona",
             {level.shopPosition.x, 2.72f, shopFrontZ - 0.04f},
             {6.8f, 0.42f, 0.08f},
             WorldLocationTag::Shop,
             {"shop_identity", "landmark"});
    addDecor("shop_awning_red",
             "shop_awning",
             {level.shopPosition.x, 2.22f, shopFrontZ - 0.22f},
             {7.6f, 0.20f, 0.72f},
             WorldLocationTag::Shop,
             {"shop_identity"});
    addDecor("shop_window_left",
             "shop_window",
             {level.shopPosition.x - 2.45f, 0.86f, shopFrontZ - 0.06f},
             {1.65f, 1.15f, 0.08f},
             WorldLocationTag::Shop,
             {"shop_identity", "glass_surface"});
    addDecor("shop_window_right",
             "shop_window",
             {level.shopPosition.x + 2.45f, 0.86f, shopFrontZ - 0.06f},
             {1.65f, 1.15f, 0.08f},
             WorldLocationTag::Shop,
             {"shop_identity", "glass_surface"});
    addDecor("shop_door_front",
             "shop_door",
             {level.shopPosition.x, 0.0f, shopFrontZ - 0.08f},
             {1.25f, 2.05f, 0.10f},
             WorldLocationTag::Shop,
             {"shop_identity", "entrance"});
    addDecor("shop_security_camera",
             "security_camera",
             {level.shopPosition.x + 3.55f, 2.35f, shopFrontZ - 0.28f},
             {0.36f, 0.24f, 0.55f},
             WorldLocationTag::Shop,
             {"shop_identity", "world_memory"});
    addDecor("shop_graffiti_panel",
             "graffiti_panel",
             {level.shopPosition.x - 3.45f, 0.35f, shopFrontZ - 0.09f},
             {0.80f, 0.48f, 0.06f},
             WorldLocationTag::Shop,
             {"shop_identity", "story_dressing"});
    addTintedDecor("shop_rolling_grille",
                   "garage_door_segment",
                   {level.shopPosition.x, 1.30f, shopFrontZ - 0.12f},
                   {5.75f, 0.72f, 0.045f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "story_dressing"},
                   objectTint(55, 58, 55));
    addTintedDecor("shop_door_handle",
                   "garage_number_plate",
                   {level.shopPosition.x + 0.45f, 1.02f, shopFrontZ - 0.15f},
                   {0.13f, 0.32f, 0.045f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2"},
                   objectTint(218, 188, 96));
    addTintedDecor("shop_price_cards_left",
                   "shop_poster",
                   {level.shopPosition.x - 2.45f, 1.12f, shopFrontZ - 0.115f},
                   {0.72f, 0.34f, 0.040f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "shop_cluster", "shop_price_card"},
                   objectTint(235, 222, 154));
    addTintedDecor("shop_price_cards_right",
                   "shop_poster",
                   {level.shopPosition.x + 2.45f, 1.12f, shopFrontZ - 0.115f},
                   {0.72f, 0.34f, 0.040f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "shop_cluster", "shop_price_card"},
                   objectTint(235, 222, 154));
    addTintedDecor("shop_window_dirt_left",
                   "wall_stain",
                   {level.shopPosition.x - 2.45f, 0.60f, shopFrontZ - 0.13f},
                   {1.30f, 0.28f, 0.035f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "subtle_decal"},
                   objectTint(73, 66, 54, 118));
    addTintedDecor("shop_window_dirt_right",
                   "wall_stain",
                   {level.shopPosition.x + 2.45f, 0.58f, shopFrontZ - 0.13f},
                   {1.20f, 0.25f, 0.035f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "subtle_decal"},
                   objectTint(73, 66, 54, 112));
    addTintedDecor("shop_threshold_worn",
                   "wall_stain",
                   {level.shopPosition.x, 0.12f, shopFrontZ - 0.18f},
                   {1.20f, 0.045f, 0.18f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "lived_in_microdetail", "surface_breakup", "subtle_decal"},
                   objectTint(58, 53, 45, 128));
    addTintedDecor("shop_handwritten_notice",
                   "notice_board",
                   {level.shopPosition.x - 0.48f, 1.48f, shopFrontZ - 0.15f},
                   {0.44f, 0.36f, 0.035f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "lived_in_microdetail", "surface_breakup", "story_dressing", "readable_notice", "shop_notice"},
                   objectTint(236, 229, 190));
    addTintedDecor("shop_window_sticker_left",
                   "shop_poster",
                   {level.shopPosition.x - 2.82f, 1.42f, shopFrontZ - 0.14f},
                   {0.34f, 0.22f, 0.030f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "lived_in_microdetail", "surface_breakup"},
                   objectTint(211, 72, 68, 214));
    addTintedDecor("shop_window_sticker_right",
                   "shop_poster",
                   {level.shopPosition.x + 2.12f, 1.43f, shopFrontZ - 0.14f},
                   {0.30f, 0.20f, 0.030f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "lived_in_microdetail", "surface_breakup"},
                   objectTint(80, 126, 84, 210));

    const float blockFrontZ = 16.0f - 3.58f;
    for (int floor = 0; floor < 3; ++floor) {
        const float y = 1.70f + static_cast<float>(floor) * 2.15f;
        addDecor("block13_window_left_" + std::to_string(floor),
                 "block_window_strip",
                 {-20.1f, y, blockFrontZ},
                 {2.20f, 0.72f, 0.08f},
                 WorldLocationTag::Block,
                 {"block_facade", "glass_surface"});
        addDecor("block13_window_mid_" + std::to_string(floor),
                 "block_window_strip",
                 {-16.4f, y, blockFrontZ},
                 {2.20f, 0.72f, 0.08f},
                 WorldLocationTag::Block,
                 {"block_facade", "glass_surface"});
        addDecor("block13_window_right_" + std::to_string(floor),
                 "block_window_strip",
                 {-12.8f, y, blockFrontZ},
                 {2.20f, 0.72f, 0.08f},
                 WorldLocationTag::Block,
                 {"block_facade", "glass_surface"});
        addDecor("block13_balcony_" + std::to_string(floor),
                 "balcony_stack",
                 {-11.45f, y - 0.18f, blockFrontZ - 0.23f},
                 {1.65f, 0.58f, 0.62f},
                 WorldLocationTag::Block,
                 {"block_facade", "halina_windows"});
    }
    addDecor("block13_entrance",
             "block_entrance",
             {-16.1f, 0.0f, blockFrontZ - 0.08f},
             {1.85f, 2.15f, 0.10f},
             WorldLocationTag::Block,
             {"block_facade", "future_interior_hook"});
    addTintedDecor("block13_intercom_panel",
                   "garage_number_plate",
                   {-15.08f, 1.12f, blockFrontZ - 0.15f},
                   {0.24f, 0.54f, 0.045f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "intercom"},
                   objectTint(58, 63, 60));
    addTintedDecor("block13_mailboxes",
                   "notice_board",
                   {-17.18f, 0.82f, blockFrontZ - 0.15f},
                   {0.72f, 0.58f, 0.050f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "story_dressing"},
                   objectTint(91, 92, 82));
    addTintedDecor("block13_entry_stain",
                   "wall_stain",
                   {-16.1f, 2.05f, blockFrontZ - 0.16f},
                   {1.25f, 0.42f, 0.040f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "front_facade_stain", "subtle_decal"},
                   objectTint(74, 68, 58, 120));
    addTintedDecor("block13_front_stain_left",
                   "wall_stain",
                   {-20.1f, 3.35f, blockFrontZ - 0.13f},
                   {1.35f, 0.34f, 0.040f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "front_facade_stain", "subtle_decal"},
                   objectTint(72, 66, 58, 104));
    addTintedDecor("block13_front_stain_right",
                   "wall_stain",
                   {-12.8f, 5.48f, blockFrontZ - 0.13f},
                   {1.10f, 0.30f, 0.040f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "front_facade_stain", "subtle_decal"},
                   objectTint(72, 66, 58, 98));
    addTintedDecor("block13_balcony_clutter_0",
                   "cardboard_stack",
                   {-11.00f, 1.22f, blockFrontZ - 0.58f},
                   {0.34f, 0.24f, 0.22f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "halina_windows"},
                   objectTint(116, 98, 73));
    addTintedDecor("block13_balcony_clutter_1",
                   "planter_concrete",
                   {-11.92f, 3.35f, blockFrontZ - 0.58f},
                   {0.38f, 0.22f, 0.24f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "halina_windows"},
                   objectTint(78, 96, 66));
    addTintedDecor("block13_entry_notice",
                   "notice_board",
                   {-15.52f, 1.54f, blockFrontZ - 0.16f},
                   {0.42f, 0.34f, 0.035f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "lived_in_microdetail", "surface_breakup", "story_dressing", "readable_notice", "block_notice"},
                   objectTint(218, 212, 176));
    addTintedDecor("block13_repair_patch",
                   "wall_stain",
                   {-13.72f, 2.22f, blockFrontZ - 0.14f},
                   {1.15f, 0.46f, 0.036f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "lived_in_microdetail", "surface_breakup", "front_facade_stain", "subtle_decal"},
                   objectTint(96, 91, 79, 136));
    addTintedDecor("block13_window_curtain_0",
                   "shop_poster",
                   {-20.62f, 1.72f, blockFrontZ - 0.10f},
                   {0.44f, 0.52f, 0.030f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "lived_in_microdetail", "surface_breakup", "halina_windows"},
                   objectTint(180, 122, 94, 188));
    addTintedDecor("block13_window_curtain_1",
                   "shop_poster",
                   {-16.02f, 3.86f, blockFrontZ - 0.10f},
                   {0.38f, 0.48f, 0.030f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "lived_in_microdetail", "surface_breakup", "halina_windows"},
                   objectTint(94, 128, 150, 184));
    addDecor("block13_satellite_dish",
             "satellite_dish",
             {-22.2f, 6.2f, blockFrontZ - 0.22f},
             {0.70f, 0.58f, 0.10f},
             WorldLocationTag::Block,
             {"block_facade", "story_dressing"});

    const float garageFrontZ = 23.0f - 1.72f;
    for (int door = 0; door < 5; ++door) {
        const float x = -23.0f + static_cast<float>(door) * 2.55f;
        addDecor("garage_door_" + std::to_string(door),
                 "garage_door_segment",
                 {x, 0.0f, garageFrontZ - 0.05f},
                 {2.10f, 1.85f, 0.08f},
                 WorldLocationTag::Garage,
                 {"garage_identity", "future_base"});
        addDecor("garage_number_" + std::to_string(door),
                 "garage_number_plate",
                 {x, 1.86f, garageFrontZ - 0.08f},
                 {0.52f, 0.24f, 0.06f},
                 WorldLocationTag::Garage,
                 {"garage_identity"});
    }

    for (int lane = 0; lane < 6; ++lane) {
        addDecor("parking_bay_line_" + std::to_string(lane),
                 "parking_line",
                 {-13.2f + static_cast<float>(lane) * 2.45f, 0.065f, 8.55f},
                 {0.08f, 0.025f, 5.85f},
                 WorldLocationTag::Parking,
                 {"parking_paint"});
    }
    addDecor("parking_front_line",
             "parking_line",
             {-7.0f, 0.065f, 5.58f},
             {14.2f, 0.025f, 0.08f},
             WorldLocationTag::Parking,
             {"parking_paint"});
    addDecor("parking_back_line",
             "parking_line",
             {-7.0f, 0.065f, 11.52f},
             {14.2f, 0.025f, 0.08f},
             WorldLocationTag::Parking,
             {"parking_paint"});
    addDecor("parking_bollard_shop_0",
             "bollard_red",
             {12.0f, 0.0f, -9.8f},
             {0.25f, 1.05f, 0.25f},
             WorldLocationTag::Parking,
             {"parking_paint", "drive_readability"});
    addDecor("parking_bollard_shop_1",
             "bollard_red",
             {15.8f, 0.0f, -9.8f},
             {0.25f, 1.05f, 0.25f},
             WorldLocationTag::Parking,
             {"parking_paint", "drive_readability"});

    addTintedDecor("trash_green_bin_0",
                   "trash_bin_lowpoly",
                   {8.0f, 0.0f, -4.9f},
                   {0.78f, 1.05f, 0.68f},
                   WorldLocationTag::Trash,
                   {"trash_dressing", "trash_cluster", "public_nuisance", "artkit_v2"},
                   objectTint(58, 121, 76));
    addTintedDecor("trash_blue_bin_0",
                   "trash_bin_lowpoly",
                   {9.0f, 0.0f, -4.9f},
                   {0.78f, 1.05f, 0.68f},
                   WorldLocationTag::Trash,
                   {"trash_dressing", "trash_cluster", "public_nuisance", "artkit_v2"},
                   objectTint(57, 99, 143));
    addDecor("trash_cardboard_stack_0",
             "cardboard_stack",
             {10.05f, 0.0f, -4.75f},
             {0.88f, 0.65f, 0.72f},
             WorldLocationTag::Trash,
             {"trash_dressing", "trash_cluster", "collision_toy"});
    addDecor("trash_cardboard_stack_1",
             "cardboard_stack",
             {8.65f, 0.0f, -2.95f},
             {0.70f, 0.48f, 0.55f},
             WorldLocationTag::Trash,
             {"trash_dressing", "trash_cluster"});

    addDecor("sidewalk_shop_front",
             "sidewalk_slab",
             {18.0f, 0.035f, -22.35f},
             {11.5f, 0.05f, 1.35f},
             WorldLocationTag::Shop,
             {"story_dressing", "drive_readability"});
    addDecor("sidewalk_block_front",
             "sidewalk_slab",
             {-16.0f, 0.035f, 11.55f},
             {15.0f, 0.05f, 1.20f},
             WorldLocationTag::Block,
             {"story_dressing"});

    for (int stripe = 0; stripe < 4; ++stripe) {
        addDecor("shop_crosswalk_stripe_" + std::to_string(stripe),
                 "crosswalk_stripe",
                 {15.4f + static_cast<float>(stripe) * 1.25f, 0.065f, level.shopEntrancePosition.z + 0.02f},
                 {0.72f, 0.035f, 1.25f},
                 WorldLocationTag::Shop,
                 {"route_guidance", "drive_readability", "shop_identity"});
    }

    addDecor("route_arrow_head_parking_exit",
             "route_arrow_head",
             {level.parkingExitPosition.x, 0.075f, level.parkingExitPosition.z - 1.08f},
             {1.05f, 0.035f, 1.15f},
             WorldLocationTag::Parking,
             {"route_guidance", "drive_readability", "parking_paint"});
    addDecor("route_arrow_head_parking_lane",
             "route_arrow_head",
             {level.parkingLanePosition.x, 0.075f, level.parkingLanePosition.z - 0.88f},
             {0.95f, 0.035f, 1.05f},
             WorldLocationTag::Parking,
             {"route_guidance", "drive_readability", "parking_paint"});
    addDecor("route_arrow_head_road_bend",
             "route_arrow_head",
             {level.roadBendPosition.x + 0.85f, 0.075f, level.roadBendPosition.z},
             {0.95f, 0.035f, 1.05f},
             WorldLocationTag::RoadLoop,
             {"route_guidance", "drive_readability"},
             1.5708f);
    addDecor("route_arrow_head_shop_road",
             "route_arrow_head",
             {level.shopRoadPosition.x + 1.05f, 0.075f, level.shopRoadPosition.z},
             {1.05f, 0.035f, 1.15f},
             WorldLocationTag::RoadLoop,
             {"route_guidance", "drive_readability"},
             1.5708f);
    addDecor("route_arrow_head_shop_apron",
             "route_arrow_head",
             {level.shopEntrancePosition.x - 1.25f, 0.075f, level.shopEntrancePosition.z + 0.28f},
             {1.05f, 0.035f, 1.15f},
             WorldLocationTag::Shop,
             {"route_guidance", "drive_readability", "shop_identity"},
             1.5708f);
    addDecor("route_arrow_head_main_bend",
             "route_arrow_head",
             {level.roadBendPosition.x + 1.10f, 0.075f, level.roadBendPosition.z - 1.65f},
             {1.10f, 0.035f, 1.20f},
             WorldLocationTag::RoadLoop,
             {"route_guidance", "drive_readability", "main_artery_guidance", "future_expansion"},
             0.78f);
    addDecor("route_arrow_head_main_exit",
             "route_arrow_head",
             {level.shopRoadPosition.x + 0.95f, 0.075f, level.shopRoadPosition.z - 1.65f},
             {1.10f, 0.035f, 1.20f},
             WorldLocationTag::RoadLoop,
             {"route_guidance", "drive_readability", "main_artery_guidance", "future_expansion"},
             0.78f);
    addDecor("route_arrow_head_main_artery",
             "route_arrow_head",
             {22.0f, 0.075f, -29.0f},
             {1.20f, 0.035f, 1.30f},
             WorldLocationTag::RoadLoop,
             {"route_guidance", "drive_readability", "main_artery_guidance", "future_expansion"},
             1.5708f);
}

} // namespace bs3d::IntroLevelDressingSections
