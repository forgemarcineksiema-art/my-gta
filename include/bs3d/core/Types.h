#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace bs3d {

using EntityId = std::uint32_t;

constexpr EntityId InvalidEntity = 0;
constexpr float Pi = 3.14159265358979323846f;

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

inline Vec3 operator+(const Vec3& lhs, const Vec3& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

inline Vec3 operator-(const Vec3& lhs, const Vec3& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

inline Vec3 operator*(const Vec3& value, float scalar) {
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

inline Vec3 operator/(const Vec3& value, float scalar) {
    return {value.x / scalar, value.y / scalar, value.z / scalar};
}

inline float lengthSquaredXZ(const Vec3& value) {
    return value.x * value.x + value.z * value.z;
}

inline float distanceSquaredXZ(const Vec3& lhs, const Vec3& rhs) {
    return lengthSquaredXZ(lhs - rhs);
}

inline float distanceXZ(const Vec3& lhs, const Vec3& rhs) {
    return std::sqrt(distanceSquaredXZ(lhs, rhs));
}

inline Vec3 normalizeXZ(const Vec3& value) {
    const float length = std::sqrt(lengthSquaredXZ(value));
    if (length <= 0.0001f) {
        return {};
    }

    return {value.x / length, 0.0f, value.z / length};
}

inline Vec3 forwardFromYaw(float yawRadians) {
    return {std::sin(yawRadians), 0.0f, std::cos(yawRadians)};
}

inline Vec3 screenRightFromYaw(float yawRadians) {
    return {-std::cos(yawRadians), 0.0f, std::sin(yawRadians)};
}

inline float yawFromDirection(const Vec3& direction) {
    if (lengthSquaredXZ(direction) <= 0.0001f) {
        return 0.0f;
    }

    return std::atan2(direction.x, direction.z);
}

inline float approach(float current, float target, float maxDelta) {
    if (current < target) {
        return std::min(current + maxDelta, target);
    }

    return std::max(current - maxDelta, target);
}

inline float wrapAngle(float angle) {
    while (angle > Pi) {
        angle -= 2.0f * Pi;
    }
    while (angle < -Pi) {
        angle += 2.0f * Pi;
    }
    return angle;
}

struct Transform {
    Vec3 position{};
    float yawRadians = 0.0f;
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct InputState {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool walk = false;
    bool sprint = false;
    bool jumpPressed = false;
    bool accelerate = false;
    bool brake = false;
    bool steerLeft = false;
    bool steerRight = false;
    bool handbrake = false;
    bool vehicleBoost = false;
    bool hornPressed = false;
    bool hornDown = false;
    bool enterExitPressed = false;
    bool primaryActionPressed = false;
    bool secondaryActionPressed = false;
    bool carryActionPressed = false;
    bool pushActionPressed = false;
    bool drinkActionPressed = false;
    bool smokeActionPressed = false;
    bool lowVaultActionPressed = false;
    bool fallActionPressed = false;
    bool interactionLocked = false;
    bool retryPressed = false;
    bool toggleDebugPressed = false;
    bool toggleRuntimeEditorPressed = false;
    bool mouseCaptureTogglePressed = false;
    bool mouseLookActive = true;
    float cameraYawRadians = 0.0f;
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
};

struct EngineContext {
    int screenWidth = 1280;
    int screenHeight = 720;
    float deltaSeconds = 0.0f;
    float elapsedSeconds = 0.0f;
    bool debugEnabled = true;
};

struct GameState {
    bool running = true;
    bool playerInVehicle = false;
    Transform playerTransform{};
    Transform vehicleTransform{};
};

struct DebugLine {
    Vec3 start{};
    Vec3 end{};
    std::string label;
};

struct DebugBox {
    Vec3 center{};
    Vec3 size{};
    std::string label;
};

struct DebugDraw {
    std::vector<DebugLine> lines;
    std::vector<DebugBox> boxes;

    void clear() {
        lines.clear();
        boxes.clear();
    }
};

} // namespace bs3d
