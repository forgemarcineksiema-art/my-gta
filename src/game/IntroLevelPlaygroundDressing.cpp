#include "IntroLevelDressingSections.h"

#include "IntroLevelDressingUtils.h"

#include <string>
#include <utility>

// Art-direction pivot 2026-05: playground dressing for Ring 1a. Target is
// stylized mid-poly. Props use existing unit_box.obj/available meshes as
// placeholders until dedicated playground .obj models are authored.
// All objects are no-collision decorative.

namespace bs3d::IntroLevelDressingSections {

namespace {

constexpr float CourtY = 0.068f;

} // namespace

void addPlaygroundDressing(IntroLevelData& level) {
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

    // --- Trzepak (clothes rack, single model 4.5x1.28m) ---
    addDecor("trzepak",
             "trzepak",
             {-6.0f, 0.0f, 11.5f},
             {1.0f, 1.0f, 1.0f},
             WorldLocationTag::Block,
             {"playground", "trzepak", "social_hub", "midpoly_target"});

    // --- Swing set (single model 2.1x2.2x1.2m) ---
    addDecor("swing_set",
             "swing_set",
             {-4.5f, 0.0f, 13.5f},
             {1.0f, 1.0f, 1.0f},
             WorldLocationTag::Block,
             {"playground", "swing", "midpoly_target"});
    // Broken swing seat (story: vandals)
    addTintedDecor("swing_broken",
                   "wall_stain",
                   {-1.0f, 0.06f, 14.5f},
                   {0.45f, 0.05f, 0.18f},
                   WorldLocationTag::Block,
                   {"playground", "swing", "story_dressing", "lived_in_microdetail"},
                   objectTint(142, 78, 44),
                   0.35f);

    // --- Slide (single model 0.8x1.19x2.25m) ---
    addDecor("slide",
             "slide_small",
             {-9.5f, 0.0f, 11.2f},
             {1.0f, 1.0f, 1.0f},
             WorldLocationTag::Block,
             {"playground", "slide", "midpoly_target"});

    // --- Basketball hoop (single model 1.1x3.0x0.5m) ---
    addDecor("hoop",
             "basketball_hoop",
             {-6.0f, 0.0f, 16.6f},
             {1.0f, 1.0f, 1.0f},
             WorldLocationTag::Block,
             {"playground", "hoop", "midpoly_target"});

    // --- Sandpit ---
    addTintedDecor("sandpit_base",
                   "planter_concrete",
                   {-5.8f, 0.01f, 13.0f},
                   {2.5f, 0.04f, 2.5f},
                   WorldLocationTag::Block,
                   {"playground", "sandpit", "midpoly_target"},
                   objectTint(218, 204, 148));
    // Sandpit border (4 low walls)
    addTintedDecor("sandpit_border_N",
                   "curb_lowpoly",
                   {-5.8f, 0.04f, 11.82f},
                   {2.7f, 0.12f, 0.10f},
                   WorldLocationTag::Block,
                   {"playground", "sandpit", "midpoly_target", "artkit_v2"},
                   objectTint(82, 78, 72));
    addTintedDecor("sandpit_border_S",
                   "curb_lowpoly",
                   {-5.8f, 0.04f, 14.18f},
                   {2.7f, 0.12f, 0.10f},
                   WorldLocationTag::Block,
                   {"playground", "sandpit", "midpoly_target", "artkit_v2"},
                   objectTint(82, 78, 72));
    addTintedDecor("sandpit_border_W",
                   "curb_lowpoly",
                   {-7.05f, 0.04f, 13.0f},
                   {0.10f, 0.12f, 2.5f},
                   WorldLocationTag::Block,
                   {"playground", "sandpit", "midpoly_target", "artkit_v2"},
                   objectTint(82, 78, 72));
    addTintedDecor("sandpit_border_E",
                   "curb_lowpoly",
                   {-4.55f, 0.04f, 13.0f},
                   {0.10f, 0.12f, 2.5f},
                   WorldLocationTag::Block,
                   {"playground", "sandpit", "midpoly_target", "artkit_v2"},
                   objectTint(82, 78, 72));

    // --- Lamp posts ---
    addTintedDecor("court_key_line_N",
                   "parking_line",
                   {-6.0f, CourtY, 12.8f},
                   {3.5f, 0.025f, 0.06f},
                   WorldLocationTag::Block,
                   {"playground", "court_marking"},
                   objectTint(232, 218, 158));
    addTintedDecor("court_key_line_S",
                   "parking_line",
                   {-6.0f, CourtY, 15.0f},
                   {3.5f, 0.025f, 0.06f},
                   WorldLocationTag::Block,
                   {"playground", "court_marking"},
                   objectTint(232, 218, 158));
    addTintedDecor("court_center_line",
                   "parking_line",
                   {-6.0f, CourtY, 14.0f},
                   {5.5f, 0.025f, 0.06f},
                   WorldLocationTag::Block,
                   {"playground", "court_marking"},
                   objectTint(232, 218, 158));
    // Court circle (approximated with crosswalk decals)
    addTintedDecor("court_circle_mark",
                   "crosswalk_stripe",
                   {-6.0f, CourtY, 14.0f},
                   {0.80f, 0.030f, 0.06f},
                   WorldLocationTag::Block,
                   {"playground", "court_marking"},
                   objectTint(232, 218, 158));
    addTintedDecor("court_circle_mark_V",
                   "crosswalk_stripe",
                   {-6.0f, CourtY, 14.0f},
                   {0.06f, 0.030f, 0.80f},
                   WorldLocationTag::Block,
                   {"playground", "court_marking"},
                   objectTint(232, 218, 158));

    // --- Lamp posts ---
    addDecor("lamp_playground_N",
             "lamp_post_lowpoly",
             {-6.5f, 0.0f, 11.0f},
             {0.16f, 2.8f, 0.16f},
             WorldLocationTag::Block,
             {"playground", "vertical_readability", "artkit_v2"});
    addDecor("lamp_playground_S",
             "lamp_post_lowpoly",
             {-6.5f, 0.0f, 17.0f},
             {0.16f, 2.8f, 0.16f},
             WorldLocationTag::Block,
             {"playground", "vertical_readability", "artkit_v2"});
    addDecor("lamp_block11_front",
             "lamp_post_lowpoly",
             {5.0f, 0.0f, 14.0f},
             {0.16f, 2.8f, 0.16f},
             WorldLocationTag::Block,
             {"playground", "vertical_readability", "block_cluster", "artkit_v2"});

    // --- Ground wear path (playground to Blok 13) ---
    auto pathTint = objectTint(108, 96, 68, 120);
    addTintedDecor("grass_wear_playground_0",
                   "irregular_grass_patch",
                   {-5.8f, 0.088f, 10.0f},
                   {2.0f, 0.007f, 2.0f},
                   WorldLocationTag::Block,
                   {"playground", "ground_patch", "irregular_ground_patch", "artkit_v2", "footpath_guide"},
                   pathTint);
    addTintedDecor("grass_wear_playground_1",
                   "irregular_grass_patch",
                   {-6.5f, 0.088f, 8.0f},
                   {1.8f, 0.007f, 2.2f},
                   WorldLocationTag::Block,
                   {"playground", "ground_patch", "irregular_ground_patch", "artkit_v2", "footpath_guide"},
                   pathTint,
                   -0.15f);

    // --- Block 11 simple facade dressing (entrance + windows) ---
    const float blk11FrontZ = 20.0f;
    for (int floor = 0; floor < 2; ++floor) {
        const float wy = 2.20f + static_cast<float>(floor) * 2.60f;
        addDecor("blok11_window_left_" + std::to_string(floor),
                 "block_window_strip",
                 {0.5f, wy, blk11FrontZ},
                 {2.20f, 1.20f, 0.08f},
                 WorldLocationTag::Block,
                 {"block_facade", "glass_surface", "artkit_v2"});
        addDecor("blok11_window_right_" + std::to_string(floor),
                 "block_window_strip",
                 {5.5f, wy, blk11FrontZ},
                 {2.20f, 1.20f, 0.08f},
                 WorldLocationTag::Block,
                 {"block_facade", "glass_surface", "artkit_v2"});
        addDecor("blok11_window_mid_" + std::to_string(floor),
                 "block_window_strip",
                 {9.5f, wy, blk11FrontZ},
                 {2.20f, 1.20f, 0.08f},
                 WorldLocationTag::Block,
                 {"block_facade", "glass_surface", "artkit_v2"});
    }
    addDecor("blok11_entrance",
             "garage_number_plate",
             {3.0f, 0.0f, blk11FrontZ - 0.08f},
             {1.85f, 2.15f, 0.10f},
             WorldLocationTag::Block,
             {"block_facade", "artkit_v2"});
    addTintedDecor("blok11_entrance_stain",
                   "wall_stain",
                   {3.0f, 2.0f, blk11FrontZ - 0.14f},
                   {1.30f, 0.40f, 0.035f},
                   WorldLocationTag::Block,
                   {"block_facade", "subtle_decal", "artkit_v2"},
                   objectTint(74, 68, 58, 115));
    addDecor("blok11_satellite",
             "satellite_dish",
             {12.0f, 6.0f, blk11FrontZ - 0.18f},
             {0.65f, 0.52f, 0.10f},
             WorldLocationTag::Block,
             {"block_facade", "artkit_v2"});
    addTintedDecor("blok11_side_graffiti",
                   "graffiti_panel",
                   {-2.5f, 0.50f, 17.0f},
                   {0.60f, 0.45f, 0.050f},
                   WorldLocationTag::Block,
                   {"block_facade", "lived_in_microdetail", "story_dressing", "artkit_v2"},
                   objectTint(178, 72, 96));
    addTintedDecor("blok11_wall_grime",
                   "wall_stain",
                   {0.5f, 0.42f, blk11FrontZ - 0.12f},
                   {5.0f, 0.22f, 0.038f},
                   WorldLocationTag::Block,
                   {"block_facade", "subtle_decal", "artkit_v2"},
                   objectTint(68, 64, 56, 95));
}

} // namespace bs3d::IntroLevelDressingSections

