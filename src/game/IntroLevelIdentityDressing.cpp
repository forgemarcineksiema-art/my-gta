#include "IntroLevelDressingSections.h"

#include "IntroLevelDressingUtils.h"

#include <string>
#include <utility>

// Art-direction pivot 2026-05: identity dressing builds the readable landmark
// character of each hero location. Target is stylized mid-poly with:
// - shop front: modelled windows, door, sign, awning, posters, lights, depth
// - block facade: recessed windows, entrance, balconies, stains, pipes, notices
// - garage row: doors with seams, handles, lamps, wear, number plates
// Collision is off for all dressing objects — core layout handles collision.

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
             {6.8f, 0.80f, 0.08f},
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
    // Window frame bars (thin fence_panels around glass edge)
    addTintedDecor("shop_window_left_frame_top",
                   "fence_panel",
                   {level.shopPosition.x - 2.45f, 1.44f, shopFrontZ - 0.05f},
                   {1.55f, 0.025f, 0.025f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target"},
                   objectTint(55, 53, 50));
    addTintedDecor("shop_window_left_frame_bottom",
                   "fence_panel",
                   {level.shopPosition.x - 2.45f, 0.28f, shopFrontZ - 0.04f},
                   {1.60f, 0.030f, 0.030f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target"},
                   objectTint(55, 53, 50));
    addTintedDecor("shop_window_left_frame_L",
                   "fence_panel",
                   {level.shopPosition.x - 3.27f, 0.86f, shopFrontZ - 0.05f},
                   {0.025f, 1.10f, 0.025f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target"},
                   objectTint(55, 53, 50));
    addTintedDecor("shop_window_left_frame_R",
                   "fence_panel",
                   {level.shopPosition.x - 1.63f, 0.86f, shopFrontZ - 0.05f},
                   {0.025f, 1.10f, 0.025f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target"},
                   objectTint(55, 53, 50));
    // Window sill (curb piece below)
    addTintedDecor("shop_window_left_sill",
                   "curb_lowpoly",
                   {level.shopPosition.x - 2.45f, 0.26f, shopFrontZ - 0.02f},
                   {1.70f, 0.06f, 0.10f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target", "artkit_v2"},
                   objectTint(98, 96, 90));
    addDecor("shop_window_right",
             "shop_window",
             {level.shopPosition.x + 2.45f, 0.86f, shopFrontZ - 0.06f},
             {1.65f, 1.15f, 0.08f},
             WorldLocationTag::Shop,
             {"shop_identity", "glass_surface"});
    // Mirror frames for right window
    addTintedDecor("shop_window_right_frame_top",
                   "fence_panel",
                   {level.shopPosition.x + 2.45f, 1.44f, shopFrontZ - 0.05f},
                   {1.55f, 0.025f, 0.025f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target"},
                   objectTint(55, 53, 50));
    addTintedDecor("shop_window_right_frame_bottom",
                   "fence_panel",
                   {level.shopPosition.x + 2.45f, 0.28f, shopFrontZ - 0.04f},
                   {1.60f, 0.030f, 0.030f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target"},
                   objectTint(55, 53, 50));
    addTintedDecor("shop_window_right_frame_L",
                   "fence_panel",
                   {level.shopPosition.x + 1.63f, 0.86f, shopFrontZ - 0.05f},
                   {0.025f, 1.10f, 0.025f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target"},
                   objectTint(55, 53, 50));
    addTintedDecor("shop_window_right_frame_R",
                   "fence_panel",
                   {level.shopPosition.x + 3.27f, 0.86f, shopFrontZ - 0.05f},
                   {0.025f, 1.10f, 0.025f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target"},
                   objectTint(55, 53, 50));
    addTintedDecor("shop_window_right_sill",
                   "curb_lowpoly",
                   {level.shopPosition.x + 2.45f, 0.26f, shopFrontZ - 0.02f},
                   {1.70f, 0.06f, 0.10f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target", "artkit_v2"},
                   objectTint(98, 96, 90));
    addDecor("shop_door_front",
             "shop_door",
             {level.shopPosition.x, 0.0f, shopFrontZ - 0.08f},
             {1.25f, 2.05f, 0.10f},
             WorldLocationTag::Shop,
             {"shop_identity", "entrance"});
    // Door frame verticals
    addTintedDecor("shop_door_frame_left",
                   "bollard_red",
                   {level.shopPosition.x - 0.60f, 1.02f, shopFrontZ - 0.06f},
                   {0.05f, 2.05f, 0.05f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target"},
                   objectTint(52, 50, 46));
    addTintedDecor("shop_door_frame_right",
                   "bollard_red",
                   {level.shopPosition.x + 0.60f, 1.02f, shopFrontZ - 0.06f},
                   {0.05f, 2.05f, 0.05f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target"},
                   objectTint(52, 50, 46));
    // Door frame header
    addTintedDecor("shop_door_frame_top",
                   "fence_panel",
                   {level.shopPosition.x, 2.08f, shopFrontZ - 0.05f},
                   {1.20f, 0.030f, 0.025f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "midpoly_target"},
                   objectTint(52, 50, 46));
    addDecor("shop_security_camera",
             "security_camera",
             {level.shopPosition.x + 3.55f, 2.35f, shopFrontZ - 0.28f},
             {0.36f, 0.24f, 0.25f},
             WorldLocationTag::Shop,
             {"shop_identity", "world_memory"});
    addDecor("shop_graffiti_panel",
             "graffiti_panel",
             {level.shopPosition.x - 3.85f, 0.35f, shopFrontZ - 0.03f},
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
                    {level.shopPosition.x - 2.45f, 1.12f, shopFrontZ - 0.065f},
                    {0.72f, 0.34f, 0.040f},
                    WorldLocationTag::Shop,
                    {"shop_identity", "shop_hero_v2", "shop_cluster", "shop_price_card"},
                    objectTint(235, 222, 154));
    addTintedDecor("shop_price_cards_right",
                    "shop_poster",
                    {level.shopPosition.x + 2.45f, 1.12f, shopFrontZ - 0.065f},
                    {0.72f, 0.34f, 0.040f},
                    WorldLocationTag::Shop,
                    {"shop_identity", "shop_hero_v2", "shop_cluster", "shop_price_card"},
                    objectTint(235, 222, 154));
    addTintedDecor("shop_window_dirt_left",
                    "wall_stain",
                    {level.shopPosition.x - 2.45f, 0.60f, shopFrontZ - 0.080f},
                    {1.30f, 0.28f, 0.035f},
                    WorldLocationTag::Shop,
                    {"shop_identity", "shop_hero_v2", "subtle_decal"},
                    objectTint(73, 66, 54, 118));
    addTintedDecor("shop_window_dirt_right",
                    "wall_stain",
                    {level.shopPosition.x + 2.45f, 0.58f, shopFrontZ - 0.080f},
                    {1.20f, 0.25f, 0.035f},
                    WorldLocationTag::Shop,
                    {"shop_identity", "shop_hero_v2", "subtle_decal"},
                    objectTint(73, 66, 54, 112));
    addTintedDecor("shop_window_repair_patch",
                   "wall_stain",
                   {level.shopPosition.x - 2.45f, 0.58f, shopFrontZ - 0.14f},
                   {1.25f, 0.48f, 0.040f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "lived_in_microdetail", "surface_breakup", "story_dressing"},
                   objectTint(96, 88, 76, 198));
    addTintedDecor("shop_conflict_notice",
                   "notice_board",
                   {level.shopPosition.x - 1.42f, 1.42f, shopFrontZ - 0.16f},
                   {0.48f, 0.40f, 0.035f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "lived_in_microdetail", "surface_breakup", "story_dressing", "readable_notice"},
                   objectTint(218, 192, 144));
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

    addTintedDecor("shop_side_wall_stain_left",
                    "wall_stain",
                    {13.22f, 1.25f, shopFrontZ + 2.90f},
                    {0.04f, 0.42f, 1.60f},
                    WorldLocationTag::Shop,
                    {"shop_identity", "shop_hero_v2", "subtle_decal"},
                    objectTint(72, 65, 54, 106));
    addTintedDecor("shop_side_wall_stain_right",
                    "wall_stain",
                    {22.78f, 1.58f, shopFrontZ + 2.90f},
                    {0.04f, 0.38f, 1.45f},
                    WorldLocationTag::Shop,
                    {"shop_identity", "shop_hero_v2", "subtle_decal"},
                    objectTint(70, 63, 53, 100));
    addTintedDecor("shop_rear_wall_stain",
                    "wall_stain",
                    {19.25f, 0.72f, -14.47f},
                    {1.80f, 0.36f, 0.04f},
                    WorldLocationTag::Shop,
                    {"shop_identity", "shop_hero_v2", "subtle_decal"},
                    objectTint(74, 68, 56, 102));
    addTintedDecor("shop_side_graffiti_right",
                    "graffiti_panel",
                    {22.78f, 0.52f, shopFrontZ + 2.94f},
                    {0.055f, 0.55f, 0.72f},
                    WorldLocationTag::Shop,
                    {"shop_identity", "shop_hero_v2", "lived_in_microdetail", "story_dressing"},
                    objectTint(187, 62, 84));
    addTintedDecor("shop_side_poster_left",
                    "shop_poster",
                    {13.22f, 1.38f, shopFrontZ + 2.90f},
                    {0.035f, 0.72f, 0.56f},
                    WorldLocationTag::Shop,
                    {"shop_identity", "shop_hero_v2", "lived_in_microdetail", "surface_breakup"},
                    objectTint(232, 215, 122));
    addTintedDecor("shop_side_poster_right",
                    "shop_poster",
                    {22.78f, 1.56f, shopFrontZ + 2.90f},
                    {0.035f, 0.62f, 0.48f},
                    WorldLocationTag::Shop,
                    {"shop_identity", "shop_hero_v2", "lived_in_microdetail", "surface_breakup"},
                    objectTint(235, 162, 74));
    addTintedDecor("shop_threshold_floor_worn",
                    "wall_stain",
                    {level.shopPosition.x, 0.07f, shopFrontZ + 0.10f},
                    {1.35f, 0.06f, 0.26f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "lived_in_microdetail", "subtle_decal"},
                   objectTint(55, 50, 42, 138));
    addTintedDecor("shop_wall_bottom_grime",
                   "wall_stain",
                   {16.05f, 0.20f, shopFrontZ + 2.86f},
                   {3.55f, 0.18f, 0.042f},
                   WorldLocationTag::Shop,
                   {"shop_identity", "shop_hero_v2", "subtle_decal"},
                   objectTint(62, 58, 48, 96));

    const float blockFrontZ = 16.0f - 3.58f;
    const float blockLeftWindowX = -20.35f;
    const float blockMidWindowX = -16.0f;
    const float blockRightWindowX = -11.65f;
    const Vec3 blockFrontWindowScale{1.55f, 0.95f, 0.08f};
    for (int floor = 0; floor < 3; ++floor) {
        const float y = 1.72f + static_cast<float>(floor) * 2.70f;
        addDecor("block13_window_left_" + std::to_string(floor),
                 "block_window_strip",
                 {blockLeftWindowX, y, blockFrontZ},
                 blockFrontWindowScale,
                 WorldLocationTag::Block,
                 {"block_facade", "glass_surface"});
        addDecor("block13_window_mid_" + std::to_string(floor),
                 "block_window_strip",
                 {blockMidWindowX, y, blockFrontZ},
                 blockFrontWindowScale,
                 WorldLocationTag::Block,
                 {"block_facade", "glass_surface"});
        addDecor("block13_window_right_" + std::to_string(floor),
                 "block_window_strip",
                 {blockRightWindowX, y, blockFrontZ},
                 blockFrontWindowScale,
                 WorldLocationTag::Block,
                 {"block_facade", "glass_surface"});
        std::vector<std::string> balconyTags{"block_facade", "halina_windows"};
        if (floor == 1) {
            balconyTags.push_back("halina_witness");
            balconyTags.push_back("block_hero_v2");
        }
        addDecor("block13_balcony_" + std::to_string(floor),
                 "balcony_stack",
                 {-11.45f, y - 0.18f, blockFrontZ - 0.42f},
                 {1.65f, 1.0f, 1.0f},
                 WorldLocationTag::Block,
                 std::move(balconyTags));
    }
    addDecor("block13_entrance",
             "block_entrance",
             {-16.1f, 0.0f, blockFrontZ - 0.08f},
             {1.85f, 2.15f, 0.10f},
             WorldLocationTag::Block,
             {"block_facade", "future_interior_hook"});
    addTintedDecor("block13_entry_frame_left",
                   "garage_door_segment",
                   {-17.12f, 0.0f, blockFrontZ - 0.19f},
                   {0.18f, 2.38f, 0.14f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "block_entry_portal"},
                   objectTint(58, 61, 58));
    addTintedDecor("block13_entry_frame_right",
                   "garage_door_segment",
                   {-15.08f, 0.0f, blockFrontZ - 0.19f},
                   {0.18f, 2.38f, 0.14f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "block_entry_portal"},
                   objectTint(58, 61, 58));
    addTintedDecor("block13_entry_frame_top",
                   "garage_door_segment",
                   {-16.10f, 2.14f, blockFrontZ - 0.19f},
                   {2.22f, 0.20f, 0.14f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "block_entry_portal"},
                   objectTint(58, 61, 58));
    addTintedDecor("block13_entry_canopy",
                   "shop_awning",
                   {-16.10f, 2.42f, blockFrontZ - 0.42f},
                   {2.70f, 0.18f, 0.58f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "block_entry_portal"},
                   objectTint(84, 88, 78));
    addTintedDecor("block13_number_plate",
                   "garage_number_plate",
                   {-15.34f, 2.28f, blockFrontZ - 0.50f},
                   {0.42f, 0.28f, 0.045f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "block_entry_portal", "story_dressing"},
                   objectTint(229, 205, 92));
    addTintedDecor("block13_entry_lamp",
                   "garage_number_plate",
                   {-16.88f, 2.28f, blockFrontZ - 0.50f},
                   {0.30f, 0.18f, 0.050f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "block_entry_portal", "story_dressing"},
                   objectTint(240, 218, 128));
    addTintedDecor("block13_threshold_wear",
                   "irregular_dirt_patch",
                   {-16.10f, 0.070f, blockFrontZ - 1.20f},
                   {1.85f, 0.007f, 0.66f},
                   WorldLocationTag::Block,
                   {"block_entry_portal", "threshold_wear", "ground_patch", "story_dressing"},
                   objectTint(64, 58, 48, 120));
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
                   {blockLeftWindowX, 3.35f, blockFrontZ - 0.13f},
                   {1.35f, 0.34f, 0.040f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "front_facade_stain", "subtle_decal"},
                   objectTint(72, 66, 58, 104));
    addTintedDecor("block13_front_stain_right",
                   "wall_stain",
                   {blockRightWindowX, 5.48f, blockFrontZ - 0.13f},
                   {1.10f, 0.30f, 0.040f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "front_facade_stain", "subtle_decal"},
                   objectTint(72, 66, 58, 98));
    addTintedDecor("block13_balcony_clutter_0",
                    "cardboard_stack",
                    {-11.00f, 1.22f, blockFrontZ - 0.40f},
                    {0.34f, 0.24f, 0.22f},
                    WorldLocationTag::Block,
                    {"block_facade", "block_hero_v2", "halina_windows"},
                    objectTint(116, 98, 73));
    addTintedDecor("block13_balcony_clutter_1",
                    "planter_concrete",
                    {-11.92f, 4.10f, blockFrontZ - 0.40f},
                    {0.38f, 0.22f, 0.24f},
                    WorldLocationTag::Block,
                    {"block_facade", "block_hero_v2", "halina_windows", "halina_witness_visual"},
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
    addTintedDecor("block13_halina_curtain_witness",
                   "shop_poster",
                   {-11.80f, 4.38f, blockFrontZ - 0.12f},
                   {0.42f, 0.56f, 0.030f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "halina_windows", "halina_witness_visual"},
                   objectTint(196, 132, 92, 198));
    addTintedDecor("block13_halina_laundry_witness",
                   "shop_poster",
                   {-11.16f, 4.18f, blockFrontZ - 0.36f},
                   {0.74f, 0.08f, 0.42f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "halina_windows", "halina_witness_visual"},
                   objectTint(220, 214, 178, 210));
    addDecor("block13_satellite_dish",
             "satellite_dish",
             {-22.2f, 6.2f, blockFrontZ - 0.22f},
             {0.70f, 0.58f, 0.10f},
             WorldLocationTag::Block,
             {"block_facade", "story_dressing"});
    addTintedDecor("block13_entry_side_stain",
                   "wall_stain",
                   {-14.72f, 0.48f, blockFrontZ - 0.16f},
                   {1.12f, 0.36f, 0.040f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "lived_in_microdetail", "subtle_decal"},
                   objectTint(68, 62, 54, 110));
    addTintedDecor("block13_entry_base_stain",
                   "wall_stain",
                   {-16.10f, 0.22f, blockFrontZ - 0.15f},
                   {2.05f, 0.18f, 0.040f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "subtle_decal"},
                   objectTint(62, 58, 48, 98));
    addTintedDecor("block13_left_graffiti",
                   "graffiti_panel",
                   {-22.40f, 0.52f, blockFrontZ - 0.10f},
                   {0.65f, 0.46f, 0.050f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "lived_in_microdetail", "story_dressing"},
                   objectTint(212, 78, 106));
    addTintedDecor("block13_entry_notice_2",
                   "notice_board",
                   {-17.05f, 1.52f, blockFrontZ - 0.17f},
                   {0.38f, 0.32f, 0.035f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "lived_in_microdetail", "surface_breakup", "story_dressing", "readable_notice", "block_notice"},
                   objectTint(224, 218, 184));
    addTintedDecor("block13_notice_gablotka",
                   "notice_board",
                   {-16.58f, 1.38f, blockFrontZ - 0.18f},
                   {0.52f, 0.42f, 0.038f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "lived_in_microdetail", "surface_breakup", "story_dressing", "readable_notice", "block_notice"},
                   objectTint(238, 233, 200));
    addTintedDecor("block13_balcony_rug_0",
                    "shop_poster",
                    {-10.90f, 1.56f, blockFrontZ - 0.30f},
                    {0.62f, 0.08f, 0.48f},
                    WorldLocationTag::Block,
                    {"block_facade", "block_hero_v2", "lived_in_microdetail", "halina_windows"},
                    objectTint(165, 90, 72, 206));
    addTintedDecor("block13_balcony_rug_1",
                    "shop_poster",
                    {-11.15f, 3.70f, blockFrontZ - 0.30f},
                    {0.56f, 0.08f, 0.44f},
                    WorldLocationTag::Block,
                    {"block_facade", "block_hero_v2", "lived_in_microdetail", "halina_windows"},
                    objectTint(82, 132, 118, 200));
    addTintedDecor("block13_balcony_clutter_2",
                    "cardboard_stack",
                    {-11.02f, 5.78f, blockFrontZ - 0.40f},
                    {0.30f, 0.22f, 0.20f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "halina_windows"},
                   objectTint(126, 108, 82));
    addTintedDecor("block13_window_curtain_2",
                   "shop_poster",
                   {-12.42f, 1.72f, blockFrontZ - 0.10f},
                   {0.40f, 0.48f, 0.030f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "lived_in_microdetail", "surface_breakup", "halina_windows"},
                   objectTint(146, 162, 100, 190));
    addTintedDecor("block13_window_curtain_3",
                   "shop_poster",
                   {-20.30f, 3.86f, blockFrontZ - 0.10f},
                   {0.36f, 0.46f, 0.030f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "lived_in_microdetail", "surface_breakup", "halina_windows"},
                   objectTint(190, 135, 100, 182));
    addTintedDecor("block13_window_curtain_4",
                   "shop_poster",
                   {-12.60f, 5.95f, blockFrontZ - 0.10f},
                   {0.40f, 0.50f, 0.030f},
                   WorldLocationTag::Block,
                   {"block_facade", "block_hero_v2", "lived_in_microdetail", "surface_breakup", "halina_windows"},
                   objectTint(108, 142, 168, 186));

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
    addTintedDecor("garage_wall_grime_mid",
                   "wall_stain",
                   {-18.0f, 0.68f, garageFrontZ - 0.12f},
                   {4.20f, 0.28f, 0.040f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "subtle_decal"},
                   objectTint(62, 60, 54, 92));
    addTintedDecor("garage_wall_grime_left",
                   "wall_stain",
                   {-22.5f, 0.55f, garageFrontZ - 0.12f},
                   {1.85f, 0.22f, 0.040f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "subtle_decal"},
                   objectTint(58, 56, 50, 88));
    addTintedDecor("garage_side_graffiti",
                   "graffiti_panel",
                   {-24.30f, 0.55f, garageFrontZ - 0.14f},
                   {0.58f, 0.48f, 0.050f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "lived_in_microdetail", "story_dressing"},
                   objectTint(195, 85, 62));

    for (int lane = 0; lane < 6; ++lane) {
        addDecor("parking_bay_line_" + std::to_string(lane),
                 "parking_line",
                 {-13.2f + static_cast<float>(lane) * 2.45f, 0.065f, 8.25f},
                 {0.08f, 0.025f, 5.30f},
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
             {-7.0f, 0.065f, 10.96f},
             {14.2f, 0.025f, 0.08f},
             WorldLocationTag::Parking,
             {"parking_paint"});
    addDecor("parking_bollard_shop_0",
             "bollard_red",
             {12.0f, 0.0f, -9.8f},
             {0.25f, 1.05f, 0.25f},
             WorldLocationTag::RoadLoop,
             {"road_readability", "drive_readability"});
    addDecor("parking_bollard_shop_1",
             "bollard_red",
             {15.8f, 0.0f, -9.8f},
             {0.25f, 1.05f, 0.25f},
             WorldLocationTag::RoadLoop,
             {"road_readability", "drive_readability"});
    addDecor("parking_bollard_corner_left",
             "bollard_red",
             {-14.55f, 0.0f, 10.88f},
             {0.25f, 1.05f, 0.25f},
             WorldLocationTag::Parking,
             {"parking_paint", "drive_readability"});
    addDecor("parking_bollard_corner_right",
             "bollard_red",
             {1.5f, 0.0f, 10.88f},
             {0.25f, 1.05f, 0.25f},
             WorldLocationTag::Parking,
             {"parking_paint", "drive_readability"});

    addTintedDecor("trash_green_bin_0",
                   "trash_bin_lowpoly",
                   {8.0f, 0.0f, -4.9f},
                   {0.78f, 1.05f, 0.68f},
                   WorldLocationTag::Trash,
                   {"trash_dressing", "trash_interactable", "trash_cluster", "public_nuisance", "artkit_v2"},
                   objectTint(58, 121, 76));
    addTintedDecor("trash_blue_bin_0",
                   "trash_bin_lowpoly",
                   {9.0f, 0.0f, -4.9f},
                   {0.78f, 1.05f, 0.68f},
                   WorldLocationTag::Trash,
                   {"trash_dressing", "trash_interactable", "trash_cluster", "public_nuisance", "artkit_v2"},
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
    addTintedDecor("trash_cardboard_scatter",
                   "cardboard_stack",
                   {10.58f, 0.0f, -3.65f},
                   {0.55f, 0.38f, 0.42f},
                   WorldLocationTag::Trash,
                   {"trash_dressing", "trash_cluster"},
                   objectTint(172, 138, 76));
    addTintedDecor("trash_shelter_side_stain",
                   "wall_stain",
                   {10.58f, 0.58f, -3.15f},
                   {0.95f, 0.25f, 0.040f},
                   WorldLocationTag::Trash,
                   {"trash_dressing", "trash_cluster", "subtle_decal"},
                   objectTint(72, 66, 55, 126));
    addTintedDecor("trash_shelter_graffiti",
                   "graffiti_panel",
                   {7.60f, 0.48f, -3.15f},
                   {0.58f, 0.40f, 0.050f},
                   WorldLocationTag::Trash,
                   {"trash_dressing", "trash_cluster", "story_dressing"},
                   objectTint(210, 82, 60));

    addDecor("sidewalk_shop_front",
             "sidewalk_slab",
             {18.0f, 0.035f, -22.35f},
             {11.5f, 0.05f, 1.35f},
             WorldLocationTag::Shop,
             {"story_dressing", "drive_readability"});
    addDecor("sidewalk_block_front",
             "sidewalk_slab",
             {-16.0f, 0.035f, 10.75f},
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
             WorldLocationTag::RoadLoop,
             {"route_guidance", "drive_readability"});
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
    addDecor("route_arrow_head_parking_entry",
             "route_arrow_head",
             {level.parkingExitPosition.x - 0.50f, 0.075f, level.parkingExitPosition.z + 0.30f},
             {0.85f, 0.035f, 0.95f},
             WorldLocationTag::Parking,
             {"route_guidance", "drive_readability", "parking_paint"});
    addDecor("route_arrow_head_road_mid",
             "route_arrow_head",
             {level.roadBendPosition.x - 0.60f, 0.075f, level.roadBendPosition.z + 0.72f},
             {0.95f, 0.035f, 1.05f},
             WorldLocationTag::RoadLoop,
             {"route_guidance", "drive_readability"},
             -0.80f);
    addDecor("crosswalk_stripe_parking_exit",
             "crosswalk_stripe",
             {level.parkingExitPosition.x, 0.065f, level.parkingExitPosition.z - 0.65f},
             {0.72f, 0.035f, 1.25f},
             WorldLocationTag::Parking,
             {"route_guidance", "drive_readability"});
    addDecor("crosswalk_stripe_road_bend",
             "crosswalk_stripe",
             {level.roadBendPosition.x + 0.10f, 0.065f, level.roadBendPosition.z + 1.05f},
             {0.72f, 0.035f, 1.25f},
             WorldLocationTag::RoadLoop,
             {"route_guidance", "drive_readability"});
}

} // namespace bs3d::IntroLevelDressingSections
