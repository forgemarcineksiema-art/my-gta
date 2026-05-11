#pragma once

namespace bs3d {

enum class RendererBackendKind {
    Raylib,
};

enum class PhysicsBackendKind {
    Custom,
};

constexpr const char* rendererBackendName(RendererBackendKind kind) {
    switch (kind) {
    case RendererBackendKind::Raylib:
        return "raylib";
    }
    return "unknown";
}

constexpr const char* physicsBackendName(PhysicsBackendKind kind) {
    switch (kind) {
    case PhysicsBackendKind::Custom:
        return "custom";
    }
    return "unknown";
}

} // namespace bs3d

