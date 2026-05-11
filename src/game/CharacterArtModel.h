#pragma once

#include "bs3d/core/Types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace bs3d {

enum class CharacterArtShape {
    Box,
    Sphere
};

enum class CharacterArtMaterial {
    Skin,
    Hair,
    Jacket,
    Pants,
    Shoes,
    Trim,
    Accent
};

enum class CharacterPartRole {
    Static,
    Torso,
    Head,
    LeftArm,
    RightArm,
    LeftLeg,
    RightLeg,
    LeftFoot,
    RightFoot
};

struct CharacterArtPart {
    std::string name;
    CharacterArtShape shape = CharacterArtShape::Box;
    CharacterArtMaterial material = CharacterArtMaterial::Jacket;
    CharacterPartRole role = CharacterPartRole::Static;
    Vec3 position{};
    Vec3 size{1.0f, 1.0f, 1.0f};
    bool render = true;
};

struct CharacterArtModelSpec {
    float visualHeight = 1.80f;
    float capsuleRadius = 0.45f;
    float capsuleHeight = 1.80f;
    std::vector<CharacterArtPart> parts;
};

struct CharacterArtColor {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

struct CharacterArtPalette {
    CharacterArtColor skin{};
    CharacterArtColor hair{};
    CharacterArtColor jacket{};
    CharacterArtColor pants{};
    CharacterArtColor shoes{};
    CharacterArtColor trim{};
    CharacterArtColor accent{};
};

struct CharacterVisualProfile {
    std::string id;
    std::string label;
    CharacterArtPalette palette{};
};

const CharacterArtModelSpec& characterArtModelSpec();
const CharacterArtPalette& defaultCharacterArtPalette();
CharacterArtColor characterArtColorForMaterial(CharacterArtMaterial material, const CharacterArtPalette& palette);
const std::vector<CharacterVisualProfile>& characterVisualCatalog();
const CharacterVisualProfile* findCharacterVisualProfile(const std::string& id);
const CharacterVisualProfile& characterVisualProfileOrDefault(const std::string& id);

} // namespace bs3d
