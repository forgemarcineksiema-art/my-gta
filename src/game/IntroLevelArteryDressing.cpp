#include "IntroLevelDressingSections.h"

#include "IntroLevelDressingUtils.h"

#include <string>
#include <utility>

// Ring 1b: Main Artery + Pawilony dressing. Target is stylized mid-poly.
// 3 pavilion storefronts (Zenon satelite, Kebab "U Grubego", Lombard),
// bus stop, kiosk, sidewalks, street lamps, crosswalk, center line.
// All dressing objects are no-collision decorative.

namespace bs3d::IntroLevelDressingSections {

namespace {

constexpr float ArterySidewalkY = 0.022f;
constexpr float ArteryDecalY = -0.002f;
constexpr float PavZ = -32.0f;
constexpr float ArteryZ = -30.0f;

} // namespace

void addArteryAndPavilionsDressing(IntroLevelData& level) {
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

    // === Sidewalks along artery (both sides) ===
    // North sidewalk (building side)
    for (int seg = 0; seg < 20; ++seg) {
        addDecor("artery_sidewalk_N_" + std::to_string(seg),
                 "sidewalk_slab",
                 {-28.0f + static_cast<float>(seg) * 3.8f, ArterySidewalkY, ArteryZ + 2.5f},
                 {3.6f, 0.05f, 2.0f},
                 WorldLocationTag::RoadLoop,
                 {"artery", "sidewalk", "midpoly_target"});
    }
    // South sidewalk (pavilion side)
    for (int seg = 0; seg < 20; ++seg) {
        addDecor("artery_sidewalk_S_" + std::to_string(seg),
                 "sidewalk_slab",
                 {-28.0f + static_cast<float>(seg) * 3.8f, ArterySidewalkY, ArteryZ - 3.2f},
                 {3.6f, 0.05f, 1.5f},
                 WorldLocationTag::RoadLoop,
                 {"artery", "sidewalk", "midpoly_target"});
    }

    // === Street lamps along artery (every 12m, both sides) ===
    for (int lamp = 0; lamp < 8; ++lamp) {
        const float lx = -28.0f + static_cast<float>(lamp) * 10.5f;
        addDecor("artery_lamp_N_" + std::to_string(lamp),
                 "lamp_post_lowpoly",
                 {lx, 0.0f, ArteryZ + 1.8f},
                 {0.18f, 3.2f, 0.18f},
                 WorldLocationTag::RoadLoop,
                 {"artery", "vertical_readability", "artkit_v2"});
        addDecor("artery_lamp_S_" + std::to_string(lamp),
                 "lamp_post_lowpoly",
                 {lx + 5.0f, 0.0f, ArteryZ - 4.8f},
                 {0.18f, 3.0f, 0.18f},
                 WorldLocationTag::RoadLoop,
                 {"artery", "vertical_readability", "artkit_v2"});
    }

    // === Center line (dashed) ===
    for (int dash = 0; dash < 30; ++dash) {
        addTintedDecor("artery_center_dash_" + std::to_string(dash),
                       "parking_line",
                       {-28.0f + static_cast<float>(dash) * 2.5f, ArteryDecalY, ArteryZ},
                       {1.8f, 0.025f, 0.08f},
                       WorldLocationTag::RoadLoop,
                       {"artery", "road_paint"},
                       objectTint(238, 231, 150));
    }

    // === Crosswalk from pavilions to north side ===
    for (int stripe = 0; stripe < 6; ++stripe) {
        addDecor("artery_crosswalk_" + std::to_string(stripe),
                 "crosswalk_stripe",
                 {20.5f + static_cast<float>(stripe) * 1.2f, 0.065f, ArteryZ + 0.05f},
                 {0.72f, 0.035f, 1.25f},
                 WorldLocationTag::RoadLoop,
                 {"artery", "route_guidance", "drive_readability"});
    }

    // === Route arrows on artery ===
    addDecor("arrow_artery_west",
             "route_arrow_head",
             {-20.0f, 0.075f, ArteryZ - 1.5f},
             {1.20f, 0.035f, 1.30f},
             WorldLocationTag::RoadLoop,
             {"artery", "route_guidance", "drive_readability"});
    addDecor("arrow_artery_east",
             "route_arrow_head",
             {32.0f, 0.075f, ArteryZ - 1.5f},
             {1.20f, 0.035f, 1.30f},
             WorldLocationTag::RoadLoop,
             {"artery", "route_guidance", "drive_readability"},
             3.1416f);

    // === Pavilion storefronts ===
    const float shopFrontZ = PavZ - 2.05f;

    // --- Unit 1: Zenon satellite (left) ---
    const float u1x = 20.8f;
    addTintedDecor("pav1_sign",
                   "shop_sign_zenona",
                   {u1x, 2.95f, shopFrontZ - 0.04f},
                   {3.8f, 0.35f, 0.06f},
                   WorldLocationTag::Shop,
                   {"pavilion", "shop_identity", "landmark", "midpoly_target"},
                   objectTint(232, 211, 69));
    addDecor("pav1_window",
             "shop_window",
             {u1x, 1.10f, shopFrontZ - 0.06f},
             {2.60f, 1.10f, 0.08f},
             WorldLocationTag::Shop,
             {"pavilion", "shop_identity", "glass_surface", "midpoly_target"});
    addDecor("pav1_door",
             "garage_number_plate",
             {u1x + 1.60f, 0.0f, shopFrontZ - 0.08f},
             {1.10f, 2.10f, 0.10f},
             WorldLocationTag::Shop,
             {"pavilion", "shop_identity", "doorway", "midpoly_target"});
    addTintedDecor("pav1_awning",
                   "shop_awning",
                   {u1x, 2.50f, shopFrontZ - 0.22f},
                   {4.4f, 0.16f, 0.60f},
                   WorldLocationTag::Shop,
                   {"pavilion", "shop_identity", "midpoly_target"},
                   objectTint(96, 158, 93));
    addTintedDecor("pav1_poster",
                   "shop_poster",
                   {u1x - 1.10f, 1.55f, shopFrontZ - 0.10f},
                   {0.55f, 0.72f, 0.040f},
                   WorldLocationTag::Shop,
                   {"pavilion", "shop_identity", "midpoly_target"},
                   objectTint(235, 222, 154));

    // --- Unit 2: Kebab "U Grubego" (center) ---
    const float u2x = 25.0f;
    addTintedDecor("pav2_sign",
                   "shop_sign_zenona",
                   {u2x, 2.95f, shopFrontZ - 0.04f},
                   {4.0f, 0.38f, 0.06f},
                   WorldLocationTag::Shop,
                   {"pavilion", "shop_identity", "landmark", "midpoly_target"},
                   objectTint(215, 62, 44));
    addDecor("pav2_window",
             "shop_window",
             {u2x, 1.10f, shopFrontZ - 0.06f},
             {2.80f, 1.10f, 0.08f},
             WorldLocationTag::Shop,
             {"pavilion", "shop_identity", "glass_surface", "midpoly_target"});
    addDecor("pav2_door",
             "garage_number_plate",
             {u2x + 1.70f, 0.0f, shopFrontZ - 0.08f},
             {1.10f, 2.10f, 0.10f},
             WorldLocationTag::Shop,
             {"pavilion", "shop_identity", "doorway", "midpoly_target"});
    addTintedDecor("pav2_awning_red",
                   "shop_awning",
                   {u2x, 2.50f, shopFrontZ - 0.24f},
                   {4.6f, 0.18f, 0.65f},
                   WorldLocationTag::Shop,
                   {"pavilion", "shop_identity", "landmark", "midpoly_target"},
                   objectTint(178, 38, 34));
    addTintedDecor("pav2_kebab_sign",
                   "shop_poster",
                   {u2x - 0.10f, 1.80f, shopFrontZ - 0.12f},
                   {0.80f, 0.45f, 0.040f},
                   WorldLocationTag::Shop,
                   {"pavilion", "shop_identity", "lived_in_microdetail", "midpoly_target"},
                   objectTint(218, 184, 92));
    // Kebab meat spit prop (vertical warm cylinder)
    addTintedDecor("pav2_meat_spit",
                   "bollard_red",
                   {u2x + 0.80f, 0.0f, shopFrontZ + 0.20f},
                   {0.28f, 1.45f, 0.28f},
                   WorldLocationTag::Shop,
                   {"pavilion", "shop_identity", "story_dressing", "midpoly_target"},
                   objectTint(168, 92, 42));
    // Graffiti on kebab wall
    addTintedDecor("pav2_graffiti",
                   "graffiti_panel",
                   {u2x - 2.0f, 0.55f, shopFrontZ - 0.10f},
                   {0.65f, 0.48f, 0.050f},
                   WorldLocationTag::Shop,
                   {"pavilion", "shop_identity", "story_dressing", "midpoly_target"},
                   objectTint(210, 82, 60));

    // --- Unit 3: Lombard / Komis (right) ---
    const float u3x = 29.2f;
    addTintedDecor("pav3_sign",
                   "shop_sign_zenona",
                   {u3x, 2.95f, shopFrontZ - 0.04f},
                   {3.6f, 0.35f, 0.06f},
                   WorldLocationTag::Shop,
                   {"pavilion", "shop_identity", "landmark", "midpoly_target"},
                   objectTint(232, 188, 52));
    addDecor("pav3_window_barred",
             "shop_window",
             {u3x, 1.10f, shopFrontZ - 0.06f},
             {2.40f, 1.10f, 0.08f},
             WorldLocationTag::Shop,
             {"pavilion", "shop_identity", "glass_surface", "midpoly_target"});
    // Barred window effect (dark bars over glass)
    for (int bar = 0; bar < 4; ++bar) {
        addTintedDecor("pav3_bar_" + std::to_string(bar),
                       "fence_panel",
                       {u3x, 0.55f + static_cast<float>(bar) * 0.32f, shopFrontZ - 0.13f},
                       {2.20f, 0.025f, 0.025f},
                       WorldLocationTag::Shop,
                       {"pavilion", "shop_identity", "midpoly_target"},
                       objectTint(52, 50, 46));
    }
    addDecor("pav3_door",
             "garage_number_plate",
             {u3x + 1.50f, 0.0f, shopFrontZ - 0.08f},
             {1.00f, 2.10f, 0.10f},
             WorldLocationTag::Shop,
             {"pavilion", "shop_identity", "doorway", "midpoly_target"});
    addTintedDecor("pav3_gold_trim",
                   "shop_poster",
                   {u3x, 2.52f, shopFrontZ - 0.10f},
                   {3.8f, 0.10f, 0.035f},
                   WorldLocationTag::Shop,
                   {"pavilion", "shop_identity", "midpoly_target"},
                   objectTint(232, 188, 52));
    addTintedDecor("pav3_skup_poster",
                   "shop_poster",
                   {u3x - 0.80f, 1.45f, shopFrontZ - 0.12f},
                   {0.65f, 0.52f, 0.040f},
                   WorldLocationTag::Shop,
                   {"pavilion", "shop_identity", "story_dressing", "midpoly_target"},
                   objectTint(224, 190, 68));

    // === Pavilion rear: bins, cardboard, wall wear ===
    const float rearZ = PavZ + 2.05f;
    addTintedDecor("pav_rear_bin_green",
                   "trash_bin_lowpoly",
                   {u1x - 1.0f, 0.0f, rearZ + 0.30f},
                   {0.70f, 0.95f, 0.60f},
                   WorldLocationTag::Shop,
                   {"pavilion", "trash", "artkit_v2"},
                   objectTint(58, 121, 76));
    addTintedDecor("pav_rear_bin_blue",
                   "trash_bin_lowpoly",
                   {u2x, 0.0f, rearZ + 0.30f},
                   {0.70f, 0.95f, 0.60f},
                   WorldLocationTag::Shop,
                   {"pavilion", "trash", "artkit_v2"},
                   objectTint(57, 99, 143));
    addDecor("pav_rear_cardboard",
             "cardboard_stack",
             {u3x + 1.0f, 0.0f, rearZ + 0.35f},
             {0.70f, 0.50f, 0.55f},
             WorldLocationTag::Shop,
             {"pavilion", "trash"});
    addTintedDecor("pav_rear_stain",
                   "wall_stain",
                   {u1x + 2.0f, 0.55f, rearZ},
                   {2.5f, 0.35f, 0.035f},
                   WorldLocationTag::Shop,
                   {"pavilion", "subtle_decal", "midpoly_target"},
                   objectTint(68, 64, 56, 110));

    // === Bus stop shelter ===
    addDecor("bus_stop",
             "bus_stop_shelter",
             {22.0f, 0.0f, ArteryZ + 0.80f},
             {3.0f, 2.5f, 1.8f},
             WorldLocationTag::RoadLoop,
             {"artery", "bus_stop", "landmark", "midpoly_target"});
    // Bus stop sign
    addDecor("bus_stop_sign",
             "street_sign",
             {23.5f, 0.0f, ArteryZ + 0.5f},
             {0.55f, 1.8f, 0.08f},
             WorldLocationTag::RoadLoop,
             {"artery", "bus_stop", "vertical_readability", "midpoly_target"});

    // === Kiosk Ruchu ===
    addDecor("kiosk",
             "kiosk_ruchu",
             {28.0f, 0.0f, ArteryZ + 2.2f},
             {2.0f, 2.2f, 1.5f},
             WorldLocationTag::Shop,
             {"pavilion", "kiosk", "landmark", "midpoly_target"});
    // Kiosk window
    addDecor("kiosk_window",
             "shop_window",
             {28.0f, 0.95f, ArteryZ + 2.35f},
             {1.5f, 0.85f, 0.06f},
             WorldLocationTag::Shop,
             {"pavilion", "kiosk", "glass_surface", "midpoly_target"});
    addTintedDecor("kiosk_sign",
                   "shop_poster",
                   {28.0f, 2.05f, ArteryZ + 2.35f},
                   {1.8f, 0.28f, 0.030f},
                   WorldLocationTag::Shop,
                   {"pavilion", "kiosk", "midpoly_target"},
                   objectTint(228, 188, 62));

    // === Artery boundary gates (visual markers at ends) ===
    addTintedDecor("artery_gate_west",
                   "bollard_red",
                   {-30.0f, 0.0f, ArteryZ - 2.0f},
                   {0.28f, 1.20f, 0.28f},
                   WorldLocationTag::RoadLoop,
                   {"artery", "boundary_marker"},
                   objectTint(215, 72, 54));
    addTintedDecor("artery_gate_east",
                   "bollard_red",
                   {38.0f, 0.0f, ArteryZ - 2.0f},
                   {0.28f, 1.20f, 0.28f},
                   WorldLocationTag::RoadLoop,
                   {"artery", "boundary_marker"},
                   objectTint(215, 72, 54));

    // === Directional street signs ===
    addDecor("artery_sign_centrum",
             "street_sign",
             {5.0f, 0.0f, ArteryZ + 2.0f},
             {0.60f, 1.6f, 0.08f},
             WorldLocationTag::RoadLoop,
             {"artery", "vertical_readability", "drive_readability", "midpoly_target"});
    addDecor("artery_sign_parking",
             "street_sign",
             {-10.0f, 0.0f, ArteryZ + 2.0f},
             {0.60f, 1.6f, 0.08f},
             WorldLocationTag::RoadLoop,
             {"artery", "vertical_readability", "drive_readability", "midpoly_target"});

    // === Ground wear near pavilions ===
    auto patchTint = objectTint(90, 88, 78, 128);
    addTintedDecor("artery_wear_pav",
                   "irregular_asphalt_patch",
                   {21.0f, 0.088f, ArteryZ - 3.5f},
                   {4.5f, 0.009f, 0.80f},
                   WorldLocationTag::RoadLoop,
                   {"artery", "ground_patch", "irregular_ground_patch"},
                   patchTint);
    addTintedDecor("artery_wear_pav_2",
                   "irregular_grass_patch",
                   {28.0f, 0.088f, ArteryZ + 1.5f},
                   {3.0f, 0.007f, 1.20f},
                   WorldLocationTag::RoadLoop,
                   {"artery", "ground_patch", "irregular_ground_patch"},
                   objectTint(98, 92, 74, 118));
    addTintedDecor("artery_oil_bus_stop",
                   "oil_stain",
                   {18.0f, 0.085f, ArteryZ + 0.50f},
                   {0.85f, 0.008f, 1.10f},
                   WorldLocationTag::RoadLoop,
                   {"artery", "ground_patch", "artkit_v2"},
                   objectTint(54, 56, 52, 118));
}

} // namespace bs3d::IntroLevelDressingSections



