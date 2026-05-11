#include "CharacterArtModel.h"

#include <utility>

namespace bs3d {

namespace {

CharacterArtPart part(std::string name,
                      CharacterArtShape shape,
                      CharacterArtMaterial material,
                      CharacterPartRole role,
                      Vec3 position,
                      Vec3 size) {
    CharacterArtPart result;
    result.name = std::move(name);
    result.shape = shape;
    result.material = material;
    result.role = role;
    result.position = position;
    result.size = size;
    return result;
}

CharacterArtModelSpec makeSpec() {
    CharacterArtModelSpec spec;
    spec.visualHeight = 1.84f;
    spec.capsuleRadius = 0.45f;
    spec.capsuleHeight = 1.80f;

    spec.parts = {
        part("Torso_Jacket", CharacterArtShape::Box, CharacterArtMaterial::Jacket, CharacterPartRole::Torso,
             {0.0f, 0.92f, 0.0f}, {0.58f, 0.78f, 0.38f}),
        part("Shoulders", CharacterArtShape::Box, CharacterArtMaterial::Jacket, CharacterPartRole::Torso,
             {0.0f, 1.25f, -0.01f}, {0.70f, 0.22f, 0.42f}),
        part("Back_Hood", CharacterArtShape::Box, CharacterArtMaterial::Accent, CharacterPartRole::Torso,
             {0.0f, 1.26f, -0.25f}, {0.42f, 0.24f, 0.08f}),
        part("Belly_Shadow", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::Torso,
             {0.0f, 0.55f, 0.20f}, {0.44f, 0.08f, 0.04f}),
        part("Neck", CharacterArtShape::Box, CharacterArtMaterial::Skin, CharacterPartRole::Head,
             {0.0f, 1.34f, 0.02f}, {0.18f, 0.16f, 0.16f}),
        part("Head", CharacterArtShape::Sphere, CharacterArtMaterial::Skin, CharacterPartRole::Head,
             {0.0f, 1.57f, 0.02f}, {0.52f, 0.52f, 0.50f}),
        part("Hair_Cap", CharacterArtShape::Box, CharacterArtMaterial::Hair, CharacterPartRole::Head,
             {0.0f, 1.78f, -0.03f}, {0.46f, 0.16f, 0.42f}),
        part("Cap_Brim", CharacterArtShape::Box, CharacterArtMaterial::Hair, CharacterPartRole::Head,
             {0.0f, 1.74f, 0.245f}, {0.34f, 0.055f, 0.18f}),
        part("Ear_L", CharacterArtShape::Box, CharacterArtMaterial::Skin, CharacterPartRole::Head,
             {-0.285f, 1.56f, 0.02f}, {0.045f, 0.12f, 0.075f}),
        part("Ear_R", CharacterArtShape::Box, CharacterArtMaterial::Skin, CharacterPartRole::Head,
             {0.285f, 1.56f, 0.02f}, {0.045f, 0.12f, 0.075f}),
        part("Eye_L", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::Head,
             {-0.095f, 1.60f, 0.275f}, {0.055f, 0.050f, 0.030f}),
        part("Eye_R", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::Head,
             {0.095f, 1.60f, 0.275f}, {0.055f, 0.050f, 0.030f}),
        part("Brow_L", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::Head,
             {-0.095f, 1.655f, 0.292f}, {0.085f, 0.030f, 0.028f}),
        part("Brow_R", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::Head,
             {0.095f, 1.655f, 0.292f}, {0.085f, 0.030f, 0.028f}),
        part("Nose_Mouth_Block", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::Head,
             {0.0f, 1.50f, 0.28f}, {0.20f, 0.12f, 0.08f}),
        part("Mouth_Smirk", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::Head,
             {0.035f, 1.45f, 0.325f}, {0.17f, 0.060f, 0.035f}),
        part("Jacket_Zip", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::Torso,
             {0.0f, 0.94f, 0.215f}, {0.035f, 0.58f, 0.032f}),
        part("Jacket_Collar_L", CharacterArtShape::Box, CharacterArtMaterial::Accent, CharacterPartRole::Torso,
             {-0.115f, 1.31f, 0.12f}, {0.18f, 0.075f, 0.060f}),
        part("Jacket_Collar_R", CharacterArtShape::Box, CharacterArtMaterial::Accent, CharacterPartRole::Torso,
             {0.115f, 1.31f, 0.12f}, {0.18f, 0.075f, 0.060f}),
        part("Jacket_Hem", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::Torso,
             {0.0f, 0.52f, 0.045f}, {0.54f, 0.075f, 0.37f}),
        part("Pocket_Patch_Front", CharacterArtShape::Box, CharacterArtMaterial::Accent, CharacterPartRole::Torso,
             {-0.18f, 0.88f, 0.22f}, {0.13f, 0.14f, 0.032f}),
        part("Arm_L", CharacterArtShape::Box, CharacterArtMaterial::Jacket, CharacterPartRole::LeftArm,
             {-0.33f, 0.96f, 0.0f}, {0.14f, 0.58f, 0.15f}),
        part("Arm_R", CharacterArtShape::Box, CharacterArtMaterial::Jacket, CharacterPartRole::RightArm,
             {0.33f, 0.96f, 0.0f}, {0.14f, 0.58f, 0.15f}),
        part("Elbow_Patch_L", CharacterArtShape::Box, CharacterArtMaterial::Accent, CharacterPartRole::LeftArm,
             {-0.33f, 0.88f, 0.10f}, {0.145f, 0.11f, 0.045f}),
        part("Elbow_Patch_R", CharacterArtShape::Box, CharacterArtMaterial::Accent, CharacterPartRole::RightArm,
             {0.33f, 0.88f, 0.10f}, {0.145f, 0.11f, 0.045f}),
        part("Sleeve_Cuff_L", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::LeftArm,
             {-0.33f, 0.67f, 0.015f}, {0.15f, 0.055f, 0.16f}),
        part("Sleeve_Cuff_R", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::RightArm,
             {0.33f, 0.67f, 0.015f}, {0.15f, 0.055f, 0.16f}),
        part("Hand_L", CharacterArtShape::Sphere, CharacterArtMaterial::Skin, CharacterPartRole::LeftArm,
             {-0.33f, 0.62f, 0.02f}, {0.15f, 0.15f, 0.15f}),
        part("Hand_R", CharacterArtShape::Sphere, CharacterArtMaterial::Skin, CharacterPartRole::RightArm,
             {0.33f, 0.62f, 0.02f}, {0.15f, 0.15f, 0.15f}),
        part("Leg_L", CharacterArtShape::Box, CharacterArtMaterial::Pants, CharacterPartRole::LeftLeg,
             {-0.16f, 0.32f, 0.0f}, {0.16f, 0.48f, 0.18f}),
        part("Leg_R", CharacterArtShape::Box, CharacterArtMaterial::Pants, CharacterPartRole::RightLeg,
             {0.16f, 0.32f, 0.0f}, {0.16f, 0.48f, 0.18f}),
        part("Knee_Patch_L", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::LeftLeg,
             {-0.16f, 0.31f, 0.115f}, {0.15f, 0.10f, 0.040f}),
        part("Knee_Patch_R", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::RightLeg,
             {0.16f, 0.31f, 0.115f}, {0.15f, 0.10f, 0.040f}),
        part("Trouser_Side_Stripe_L", CharacterArtShape::Box, CharacterArtMaterial::Accent, CharacterPartRole::LeftLeg,
             {-0.245f, 0.32f, 0.0f}, {0.035f, 0.39f, 0.16f}),
        part("Trouser_Side_Stripe_R", CharacterArtShape::Box, CharacterArtMaterial::Accent, CharacterPartRole::RightLeg,
             {0.245f, 0.32f, 0.0f}, {0.035f, 0.39f, 0.16f}),
        part("Shoe_L", CharacterArtShape::Box, CharacterArtMaterial::Shoes, CharacterPartRole::LeftFoot,
             {-0.17f, 0.08f, 0.06f}, {0.22f, 0.14f, 0.36f}),
        part("Shoe_R", CharacterArtShape::Box, CharacterArtMaterial::Shoes, CharacterPartRole::RightFoot,
             {0.17f, 0.08f, 0.06f}, {0.22f, 0.14f, 0.36f}),
        part("Shoe_Sole_L", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::LeftFoot,
             {-0.17f, 0.020f, 0.075f}, {0.24f, 0.035f, 0.38f}),
        part("Shoe_Sole_R", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::RightFoot,
             {0.17f, 0.020f, 0.075f}, {0.24f, 0.035f, 0.38f}),
        part("Back_Pocket_Tag", CharacterArtShape::Box, CharacterArtMaterial::Accent, CharacterPartRole::Torso,
             {-0.18f, 0.72f, -0.22f}, {0.12f, 0.12f, 0.04f}),
        part("Back_Block13_Tag", CharacterArtShape::Box, CharacterArtMaterial::Trim, CharacterPartRole::Torso,
             {0.0f, 1.02f, -0.235f}, {0.22f, 0.085f, 0.035f}),
    };

    return spec;
}

CharacterArtPalette makeDefaultPalette() {
    CharacterArtPalette palette;
    palette.skin = {235, 186, 137, 255};
    palette.hair = {33, 62, 148, 255};
    palette.jacket = {52, 86, 158, 255};
    palette.pants = {34, 43, 63, 255};
    palette.shoes = {28, 29, 31, 255};
    palette.trim = {65, 42, 35, 255};
    palette.accent = {28, 56, 118, 255};
    return palette;
}

CharacterArtPalette palette(CharacterArtColor skin,
                            CharacterArtColor hair,
                            CharacterArtColor jacket,
                            CharacterArtColor pants,
                            CharacterArtColor shoes,
                            CharacterArtColor trim,
                            CharacterArtColor accent) {
    CharacterArtPalette result;
    result.skin = skin;
    result.hair = hair;
    result.jacket = jacket;
    result.pants = pants;
    result.shoes = shoes;
    result.trim = trim;
    result.accent = accent;
    return result;
}

CharacterVisualProfile profile(std::string id, std::string label, CharacterArtPalette palette) {
    CharacterVisualProfile result;
    result.id = std::move(id);
    result.label = std::move(label);
    result.palette = palette;
    return result;
}

} // namespace

const CharacterArtModelSpec& characterArtModelSpec() {
    static const CharacterArtModelSpec spec = makeSpec();
    return spec;
}

const CharacterArtPalette& defaultCharacterArtPalette() {
    static const CharacterArtPalette palette = makeDefaultPalette();
    return palette;
}

CharacterArtColor characterArtColorForMaterial(CharacterArtMaterial material, const CharacterArtPalette& palette) {
    switch (material) {
    case CharacterArtMaterial::Skin:
        return palette.skin;
    case CharacterArtMaterial::Hair:
        return palette.hair;
    case CharacterArtMaterial::Jacket:
        return palette.jacket;
    case CharacterArtMaterial::Pants:
        return palette.pants;
    case CharacterArtMaterial::Shoes:
        return palette.shoes;
    case CharacterArtMaterial::Trim:
        return palette.trim;
    case CharacterArtMaterial::Accent:
        return palette.accent;
    }
    return palette.jacket;
}

const std::vector<CharacterVisualProfile>& characterVisualCatalog() {
    static const std::vector<CharacterVisualProfile> catalog = {
        profile("player", "Mlody", defaultCharacterArtPalette()),
        profile("bogus",
                "Bogus",
                palette({238, 196, 146, 255},
                        {72, 54, 42, 255},
                        {218, 156, 89, 255},
                        {78, 64, 48, 255},
                        {38, 35, 31, 255},
                        {64, 45, 32, 255},
                        {238, 211, 79, 255})),
        profile("zenon",
                "Zenon",
                palette({232, 184, 132, 255},
                        {45, 43, 38, 255},
                        {72, 138, 75, 255},
                        {41, 54, 46, 255},
                        {30, 31, 29, 255},
                        {32, 45, 34, 255},
                        {230, 220, 70, 255})),
        profile("lolek",
                "Lolek",
                palette({229, 177, 126, 255},
                        {36, 31, 27, 255},
                        {136, 101, 67, 255},
                        {60, 55, 50, 255},
                        {26, 25, 24, 255},
                        {48, 43, 39, 255},
                        {82, 91, 93, 255})),
        profile("receipt_holder",
                "Typ z paragonem",
                palette({234, 181, 132, 255},
                        {52, 49, 45, 255},
                        {92, 102, 112, 255},
                        {45, 70, 128, 255},
                        {25, 27, 30, 255},
                        {36, 43, 54, 255},
                        {230, 88, 70, 255})),
    };
    return catalog;
}

const CharacterVisualProfile* findCharacterVisualProfile(const std::string& id) {
    for (const CharacterVisualProfile& profile : characterVisualCatalog()) {
        if (profile.id == id) {
            return &profile;
        }
    }
    return nullptr;
}

const CharacterVisualProfile& characterVisualProfileOrDefault(const std::string& id) {
    if (const CharacterVisualProfile* profile = findCharacterVisualProfile(id)) {
        return *profile;
    }
    return characterVisualCatalog().front();
}

} // namespace bs3d
