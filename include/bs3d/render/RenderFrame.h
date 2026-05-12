#pragma once

#include "bs3d/core/Types.h"
#include "bs3d/render/RenderTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace bs3d {

enum class RenderBucket {
    Sky,
    Ground,
    Opaque,
    Vehicle,
    Decal,
    Glass,
    Translucent,
    Debug,
    Hud
};

enum class RenderPrimitiveKind {
    Box,
    Sphere,
    CylinderX,
    QuadPanel,
    Mesh
};

struct MeshHandle {
    std::uint32_t id = 0;
};

/// Temporary built-in mesh convention for D3D11 smoke/testing.
/// MeshHandle.id == BuiltInUnitCubeMeshId maps to a unit cube (same geometry as Box).
/// This is not an asset-registry handle — no real mesh loading is implied.
constexpr std::uint32_t BuiltInUnitCubeMeshId = 1;

inline bool isBuiltInUnitCubeMesh(MeshHandle mesh) {
    return mesh.id == BuiltInUnitCubeMeshId;
}

struct MaterialHandle {
    std::uint32_t id = 0;
};

struct TextureHandle {
    std::uint32_t id = 0;
};

struct RenderMaterial {
    RenderColor tint{};
    TextureHandle texture{};
};

struct RenderTransform {
    Vec3 position{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
    float yawRadians = 0.0f;
    float pitchRadians = 0.0f;
    float rollRadians = 0.0f;
};

struct RenderPrimitiveCommand {
    RenderPrimitiveKind kind = RenderPrimitiveKind::Box;
    RenderBucket bucket = RenderBucket::Opaque;
    RenderTransform transform{};
    Vec3 size{1.0f, 1.0f, 1.0f};
    RenderColor tint{};
    MeshHandle mesh{};
    MaterialHandle material{};
    std::string sourceId;
};

struct RenderLineCommand {
    Vec3 start{};
    Vec3 end{};
    RenderColor tint{};
};

struct RenderFrame {
    RenderCamera camera{};
    WorldPresentationStyle worldStyle{};
    std::vector<RenderPrimitiveCommand> primitives;
    std::vector<RenderLineCommand> debugLines;
};

constexpr const char* renderBucketName(RenderBucket bucket) {
    switch (bucket) {
    case RenderBucket::Sky:
        return "Sky";
    case RenderBucket::Ground:
        return "Ground";
    case RenderBucket::Opaque:
        return "Opaque";
    case RenderBucket::Vehicle:
        return "Vehicle";
    case RenderBucket::Decal:
        return "Decal";
    case RenderBucket::Glass:
        return "Glass";
    case RenderBucket::Translucent:
        return "Translucent";
    case RenderBucket::Debug:
        return "Debug";
    case RenderBucket::Hud:
        return "Hud";
    }
    return "Unknown";
}

constexpr const char* renderPrimitiveKindName(RenderPrimitiveKind kind) {
    switch (kind) {
    case RenderPrimitiveKind::Box:
        return "Box";
    case RenderPrimitiveKind::Sphere:
        return "Sphere";
    case RenderPrimitiveKind::CylinderX:
        return "CylinderX";
    case RenderPrimitiveKind::QuadPanel:
        return "QuadPanel";
    case RenderPrimitiveKind::Mesh:
        return "Mesh";
    }
    return "Unknown";
}

} // namespace bs3d
