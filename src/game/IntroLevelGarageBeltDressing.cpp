#include "IntroLevelDressingSections.h"

#include "IntroLevelDressingUtils.h"

#include <string>
#include <utility>

// Ring 2: Garage Belt dressing. Rysiek row is dressed by the identity pass;
// this section adds the two belt rows (-38, -42),
// door segments, number plates, oil drums, tire stacks, signs,
// wall grime, oil stains, lane markings, lamps.
// All dressing objects are no-collision decorative.

namespace bs3d::IntroLevelDressingSections {

namespace {

constexpr float LaneX = -30.75f;
constexpr float Row2X = -38.0f;
constexpr float Row3X = -42.0f;

} // namespace

void addGarageBeltDressing(IntroLevelData& level) {
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

    // === Row 2 (south belt, 3 doors) at (-38, 0, 10) ===
    const float g2Z = 8.5f;
    for (int door = 0; door < 3; ++door) {
        const float x = Row2X - 2.5f + static_cast<float>(door) * 2.5f;
        addDecor("gb2_door_" + std::to_string(door),
                 "garage_door_segment",
                 {x, 0.0f, g2Z},
                 {2.0f, 2.10f, 0.08f},
                 WorldLocationTag::Garage,
                 {"garage_identity", "garage_belt", "midpoly_target"});
        addTintedDecor("gb2_number_" + std::to_string(door),
                       "garage_number_plate",
                       {x, 1.70f, g2Z - 0.03f},
                       {0.48f, 0.22f, 0.06f},
                       WorldLocationTag::Garage,
                       {"garage_identity", "garage_belt", "midpoly_target"},
                       objectTint(232, 218, 72));
    }
    // Broken door (middle of Row 2)
    addTintedDecor("gb2_door_broken",
                   "garage_door_segment",
                   {Row2X, 0.44f, g2Z + 0.03f},
                   {0.95f, 0.85f, 0.075f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "story_dressing", "midpoly_target"},
                   objectTint(66, 64, 58),
                   0.15f);
    addTintedDecor("gb2_prymark",
                   "wall_stain",
                   {Row2X + 0.5f, 0.68f, g2Z - 0.05f},
                   {1.10f, 0.28f, 0.035f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "story_dressing", "lived_in_microdetail"},
                   objectTint(96, 88, 78, 172));

    // === Row 3 (north belt, 4 doors) at (-42, 0, 28) ===
    const float g3Z = 26.5f;
    for (int door = 0; door < 4; ++door) {
        const float x = Row3X - 3.75f + static_cast<float>(door) * 2.5f;
        addDecor("gb3_door_" + std::to_string(door),
                 "garage_door_segment",
                 {x, 0.0f, g3Z},
                 {2.0f, 2.10f, 0.08f},
                 WorldLocationTag::Garage,
                 {"garage_identity", "garage_belt", "midpoly_target"});
        addTintedDecor("gb3_number_" + std::to_string(door),
                       "garage_number_plate",
                       {x, 1.70f, g3Z - 0.03f},
                       {0.48f, 0.22f, 0.06f},
                       WorldLocationTag::Garage,
                       {"garage_identity", "garage_belt", "midpoly_target"},
                       objectTint(232, 218, 72));
    }

    // === Oil drums, tire stacks, tool crates ===
    addTintedDecor("gb_oil_0",
                   "oil_drum_lowpoly",
                   {-16.0f, 0.0f, 22.2f},
                   {0.55f, 0.85f, 0.55f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "garage_cluster", "artkit_v2"},
                   objectTint(52, 54, 50));
    addTintedDecor("gb_oil_1",
                   "oil_drum_lowpoly",
                   {-15.2f, 0.0f, 22.5f},
                   {0.55f, 0.78f, 0.55f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "garage_cluster", "artkit_v2"},
                   objectTint(48, 50, 46));
    addTintedDecor("gb_tire_0",
                   "concrete_barrier",
                   {-14.0f, 0.0f, 22.0f},
                   {0.65f, 0.55f, 0.50f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "garage_cluster"},
                   objectTint(18, 18, 17));
    addTintedDecor("gb_crate_0",
                   "cardboard_stack",
                   {-20.0f, 0.0f, 22.3f},
                   {0.65f, 0.50f, 0.55f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "garage_cluster"},
                   objectTint(118, 102, 78));
    addTintedDecor("gb_tire_1",
                   "concrete_barrier",
                   {-35.0f, 0.0f, 16.0f},
                   {0.72f, 0.62f, 0.52f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "garage_cluster"},
                   objectTint(18, 18, 17));
    addTintedDecor("gb_crate_1",
                   "cardboard_stack",
                   {-37.0f, 0.0f, 19.0f},
                   {0.70f, 0.52f, 0.58f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "garage_cluster"},
                   objectTint(118, 102, 78));
    addTintedDecor("gb_oil_2",
                   "oil_drum_lowpoly",
                   {Row3X + 1.0f, 0.0f, 27.0f},
                   {0.55f, 0.85f, 0.55f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "garage_cluster", "artkit_v2"},
                   objectTint(52, 54, 50));
    addTintedDecor("gb_scrap",
                   "cardboard_stack",
                   {-39.0f, 0.0f, 27.2f},
                   {0.80f, 0.45f, 0.62f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "garage_cluster"},
                   objectTint(92, 78, 62));
    addTintedDecor("gb_tire_2",
                   "concrete_barrier",
                   {Row3X - 2.0f, 0.0f, 25.0f},
                   {0.62f, 0.50f, 0.48f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "garage_cluster"},
                   objectTint(18, 18, 17));

    // === Signs ===
    addDecor("gb_sign_private",
             "street_sign",
             {-27.35f, 0.0f, 12.0f},
             {0.55f, 1.40f, 0.08f},
             WorldLocationTag::Garage,
             {"garage_identity", "garage_belt", "vertical_readability", "midpoly_target"});
    addDecor("gb_sign_workshop",
             "street_sign",
             {-17.0f, 0.0f, 22.8f},
             {0.60f, 1.50f, 0.08f},
             WorldLocationTag::Garage,
             {"garage_identity", "garage_belt", "vertical_readability", "midpoly_target"});

    // === Wall grime ===
    addTintedDecor("gb_grime_2",
                   "wall_stain",
                   {Row2X, 0.58f, g2Z - 0.05f},
                   {4.5f, 0.28f, 0.038f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "subtle_decal", "midpoly_target"},
                   objectTint(64, 62, 56, 96));
    addTintedDecor("gb_grime_3",
                   "wall_stain",
                   {Row3X, 0.55f, g3Z - 0.05f},
                   {5.5f, 0.25f, 0.038f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "subtle_decal", "midpoly_target"},
                   objectTint(64, 62, 56, 92));
    addTintedDecor("gb_rust",
                   "wall_stain",
                   {Row2X + 2.0f, 0.45f, g2Z - 0.07f},
                   {1.60f, 0.18f, 0.035f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "subtle_decal", "midpoly_target"},
                   objectTint(118, 68, 48, 138));

    // === Graffiti ===
    addTintedDecor("gb_graffiti",
                   "graffiti_panel",
                   {-44.0f, 0.50f, 26.3f},
                   {0.62f, 0.48f, 0.050f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "story_dressing", "midpoly_target"},
                   objectTint(185, 72, 88));

    // === Lamps on garage lane ===
    for (int lamp = 0; lamp < 3; ++lamp) {
        const float lz = 8.0f + static_cast<float>(lamp) * 10.0f;
        addDecor("gb_lamp_W_" + std::to_string(lamp),
                 "lamp_post_lowpoly",
                 {LaneX + 1.5f, 0.0f, lz},
                 {0.16f, 2.8f, 0.16f},
                 WorldLocationTag::Garage,
                 {"garage_identity", "garage_belt", "vertical_readability", "artkit_v2"});
        addDecor("gb_lamp_E_" + std::to_string(lamp),
                 "lamp_post_lowpoly",
                 {LaneX - 1.5f, 0.0f, lz},
                 {0.16f, 2.8f, 0.16f},
                 WorldLocationTag::Garage,
                 {"garage_identity", "garage_belt", "vertical_readability", "artkit_v2"});
    }

    // === Lane markings ===
    for (int dash = 0; dash < 10; ++dash) {
        addTintedDecor("gb_dash_" + std::to_string(dash),
                       "parking_line",
                       {LaneX, 0.065f, 5.0f + static_cast<float>(dash) * 2.8f},
                       {0.10f, 0.025f, 1.5f},
                       WorldLocationTag::Garage,
                       {"garage_identity", "garage_belt", "road_paint"},
                       objectTint(215, 208, 142));
    }

    // === Oil stains ===
    auto oil = objectTint(54, 56, 52, 125);
    addTintedDecor("gb_oil_lane0",
                   "oil_stain",
                   {LaneX + 0.8f, 0.082f, 13.0f},
                   {0.75f, 0.008f, 0.90f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "ground_patch", "artkit_v2"}, oil);
    addTintedDecor("gb_oil_lane1",
                   "oil_stain",
                   {LaneX - 1.2f, 0.082f, 22.0f},
                   {0.65f, 0.008f, 0.80f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "ground_patch", "artkit_v2"}, oil);
    addTintedDecor("gb_oil_r2",
                   "oil_stain",
                   {Row2X, 0.082f, 7.5f},
                   {0.70f, 0.008f, 1.10f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "ground_patch", "artkit_v2"}, oil);
    addTintedDecor("gb_oil_r3",
                   "oil_stain",
                   {Row3X, 0.082f, 25.5f},
                   {0.80f, 0.008f, 1.20f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "ground_patch", "artkit_v2"}, oil);

    // === Ground wear ===
    addTintedDecor("gb_wear_lane",
                   "irregular_asphalt_patch",
                   {LaneX, 0.088f, 16.0f},
                   {2.5f, 0.009f, 12.0f},
                   WorldLocationTag::Garage,
                   {"garage_identity", "garage_belt", "ground_patch", "irregular_ground_patch"},
                   objectTint(88, 86, 78, 128));
}

} // namespace bs3d::IntroLevelDressingSections

