#include "EditorGizmo.h"

#include "bs3d/core/Types.h"

#include <algorithm>
#include <cmath>

// raylib 3D drawing is only available in dev tools builds
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
#include "raylib.h"
#include "rlgl.h"
#endif

namespace bs3d {

namespace {

float infinity() { return 1e10f; }

inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline float lengthVec(Vec3 v) { return std::sqrt(dot(v, v)); }
inline Vec3 normalize(Vec3 v) {
    const float len = lengthVec(v);
    if (len < 0.00001f) return {0, 0, 0};
    return {v.x / len, v.y / len, v.z / len};
}
inline float distance(Vec3 a, Vec3 b) { return lengthVec(a - b); }

Vec3 rotateY(Vec3 value, float yawRadians) {
    const float c = std::cos(yawRadians);
    const float s = std::sin(yawRadians);
    return {value.x * c - value.z * s, value.y, value.x * s + value.z * c};
}

} // namespace

// --- Raycast utilities ---

float raycastAabb(Vec3 origin, Vec3 direction, Vec3 boxCenter, Vec3 boxSize, float maxDistance) {
    const Vec3 half = boxSize * 0.5f;
    const Vec3 boxMin = boxCenter - half;
    const Vec3 boxMax = boxCenter + half;
    float tMin = 0.0f;
    float tMax = maxDistance;

    for (int axis = 0; axis < 3; ++axis) {
        const float start = (&origin.x)[axis];
        const float dir = (&direction.x)[axis];
        const float aMin = (&boxMin.x)[axis];
        const float aMax = (&boxMax.x)[axis];

        if (std::fabs(dir) <= 0.0001f) {
            if (start < aMin || start > aMax) {
                return infinity();
            }
            continue;
        }

        float t1 = (aMin - start) / dir;
        float t2 = (aMax - start) / dir;
        if (t1 > t2) {
            std::swap(t1, t2);
        }
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) {
            return infinity();
        }
    }
    return tMin >= 0.0f ? tMin : infinity();
}

float raycastObb(Vec3 origin, Vec3 direction, Vec3 boxCenter, Vec3 boxSize, float yawRadians, float maxDistance) {
    const Vec3 localOrigin = rotateY(origin - boxCenter, -yawRadians);
    const Vec3 localDir = rotateY(direction, -yawRadians);
    return raycastAabb(localOrigin, localDir, {0, 0, 0}, boxSize, maxDistance);
}

float raycastSphere(Vec3 origin, Vec3 direction, Vec3 sphereCenter, float radius, float maxDistance) {
    const Vec3 oc = origin - sphereCenter;
    const float a = dot(direction, direction);
    const float b = 2.0f * dot(oc, direction);
    const float c = dot(oc, oc) - radius * radius;
    const float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) {
        return infinity();
    }
    const float sqrtDisc = std::sqrt(disc);
    const float t0 = (-b - sqrtDisc) / (2.0f * a);
    if (t0 >= 0.0f && t0 <= maxDistance) {
        return t0;
    }
    const float t1 = (-b + sqrtDisc) / (2.0f * a);
    if (t1 >= 0.0f && t1 <= maxDistance) {
        return t1;
    }
    return infinity();
}

const WorldObject* raycastSelectObject(Vec3 rayOrigin, Vec3 rayDir, const std::vector<WorldObject>& objects) {
    float bestT = infinity();
    const WorldObject* best = nullptr;

    for (const WorldObject& object : objects) {
        if (object.collision.kind == WorldCollisionShapeKind::None ||
            object.collision.kind == WorldCollisionShapeKind::Unspecified) {
            continue;
        }

        const Vec3 center = object.position + object.collision.offset;
        float t = infinity();

        switch (object.collision.kind) {
        case WorldCollisionShapeKind::Box:
        case WorldCollisionShapeKind::GroundBox:
        case WorldCollisionShapeKind::TriggerBox:
            t = raycastAabb(rayOrigin, rayDir, center, object.collision.size);
            break;
        case WorldCollisionShapeKind::OrientedBox:
            t = raycastObb(rayOrigin, rayDir, center, object.collision.size,
                           object.collision.yawRadians + object.yawRadians);
            break;
        case WorldCollisionShapeKind::TriggerSphere:
            t = raycastSphere(rayOrigin, rayDir, center, object.collision.size.x);
            break;
        case WorldCollisionShapeKind::RampZ:
            t = raycastAabb(rayOrigin, rayDir, center,
                            {object.collision.size.x,
                             std::max(object.collision.startHeight, object.collision.endHeight),
                             object.collision.size.z});
            break;
        default:
            break;
        }

        if (t < bestT) {
            bestT = t;
            best = &object;
        }
    }
    return best;
}

// --- Gizmo implementation ---

GizmoDragState EditorGizmo::dragState() const { return dragState_; }

void EditorGizmo::resetDrag() {
    grabbedAxis_ = GizmoAxis::None;
    dragState_ = GizmoDragState::Idle;
}

static float gizmoScaleFn(Vec3 objectPos, Vec3 cameraPos) {
    return std::clamp(distance(objectPos, cameraPos) * 0.15f, 0.12f, 2.5f);
}

void EditorGizmo::update(const InputState& input, RuntimeMapEditor& editor, Vec3 cameraPosition, Vec3 cameraTarget,
                          const std::vector<WorldObject>& objects) {
    (void)input;
    (void)cameraTarget;
    (void)objects;
    (void)cameraPosition;

#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
    mouseScreenPos_ = GetMousePosition();
    primaryPressed_ = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    primaryDown_ = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    if (dragState_ == GizmoDragState::Dragging) {
        if (!primaryDown_) {
            if (const WorldObject* sel = editor.selectedObject()) {
                editor.setSelectedPosition(sel->position);
            }
            resetDrag();
        }
        return;
    }

    if (!primaryPressed_) {
        return;
    }

    pendingClick_ = true;
#endif
}

void EditorGizmo::processFrame(RuntimeMapEditor& editor, Vec3 cameraPos, Vec3 cameraTarget, float fovY,
                                const std::vector<WorldObject>& objects) {
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
    // Build Camera3D for GetScreenToWorldRay
    Camera3D cam;
    cam.position = {cameraPos.x, cameraPos.y, cameraPos.z};
    cam.target = {cameraTarget.x, cameraTarget.y, cameraTarget.z};
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = fovY;
    cam.projection = CAMERA_PERSPECTIVE;

    const Ray ray = GetScreenToWorldRay({mouseScreenPos_.x, mouseScreenPos_.y}, cam);
    const Vec3 rayOrigin = {ray.position.x, ray.position.y, ray.position.z};
    const Vec3 rayDir = {ray.direction.x, ray.direction.y, ray.direction.z};

    // Handle drag continuation
    if (dragState_ == GizmoDragState::Dragging) {
        if (!primaryDown_) {
            if (const WorldObject* sel = editor.selectedObject()) {
                editor.setSelectedPosition(sel->position);
            }
            resetDrag();
            return;
        }
        // Continue drag
        WorldObject* sel = editor.selectedObject();
        if (sel == nullptr || grabbedAxis_ == GizmoAxis::None) {
            resetDrag();
            return;
        }
        const Vec3 newCenter = projectDrag(rayOrigin, rayDir, grabbedAxis_, sel->position, cameraPos);
        sel->position = newCenter;
        editor.markSelectedBaseObjectEdited();
        return;
    }

    // Handle click
    if (!pendingClick_) {
        return;
    }
    pendingClick_ = false;

    // Try gizmo axis grab first
    if (editor.selectedObject() != nullptr) {
        const Vec3 selCenter = editor.selectedObject()->position;
        const float gs = gizmoScaleFn(selCenter, cameraPos);
        const GizmoAxis hit = hitTestAxis(rayOrigin, rayDir, selCenter, gs);
        if (hit != GizmoAxis::None) {
            grabbedAxis_ = hit;
            dragState_ = GizmoDragState::Dragging;
            return;
        }
    }

    // Try object selection
    const WorldObject* hit = raycastSelectObject(rayOrigin, rayDir, objects);
    if (hit != nullptr) {
        editor.selectObject(hit->id);
    }
#endif
}

void EditorGizmo::drawSelectedGizmo(Vec3 objectPosition, Vec3 cameraPosition) const {
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
    const float gs = gizmoScaleFn(objectPosition, cameraPosition);
    const float shaftLen = 0.65f * gs;
    const float shaftRadius = 0.025f * gs;
    const float coneLen = 0.15f * gs;
    const float coneRadius = 0.055f * gs;

    // Center cube
    const float cc = 0.04f * gs;
    DrawCubeV({objectPosition.x - cc, objectPosition.y - cc, objectPosition.z - cc},
              {cc * 2.0f, cc * 2.0f, cc * 2.0f}, WHITE);

    // X axis — red, rotate Y→X
    rlPushMatrix();
    rlTranslatef(objectPosition.x, objectPosition.y, objectPosition.z);
    rlRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
    DrawCylinderEx({0, 0, 0}, {0, shaftLen, 0}, shaftRadius, shaftRadius, 12, RED);
    DrawCone({0, shaftLen, 0}, coneRadius, 0.0f, coneLen, 12, RED);
    rlPopMatrix();

    // Y axis — green, already Y-up
    rlPushMatrix();
    rlTranslatef(objectPosition.x, objectPosition.y, objectPosition.z);
    DrawCylinderEx({0, 0, 0}, {0, shaftLen, 0}, shaftRadius, shaftRadius, 12, GREEN);
    DrawCone({0, shaftLen, 0}, coneRadius, 0.0f, coneLen, 12, GREEN);
    rlPopMatrix();

    // Z axis — blue, rotate Y→Z
    rlPushMatrix();
    rlTranslatef(objectPosition.x, objectPosition.y, objectPosition.z);
    rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    DrawCylinderEx({0, 0, 0}, {0, shaftLen, 0}, shaftRadius, shaftRadius, 12, BLUE);
    DrawCone({0, shaftLen, 0}, coneRadius, 0.0f, coneLen, 12, BLUE);
    rlPopMatrix();
#endif
}

GizmoAxis EditorGizmo::hitTestAxis(Vec3 rayOrigin, Vec3 rayDir, Vec3 gizmoCenter, float gs) const {
    const float shaftLen = 0.65f * gs;
    const float hitRadius = 0.10f * gs;

    const Vec3 xEnd = gizmoCenter + Vec3{shaftLen, 0, 0};
    const Vec3 yEnd = gizmoCenter + Vec3{0, shaftLen, 0};
    const Vec3 zEnd = gizmoCenter + Vec3{0, 0, shaftLen};

    float best = hitRadius;
    GizmoAxis result = GizmoAxis::None;

    auto check = [&](GizmoAxis ax, const Vec3& a, const Vec3& b) {
        const Vec3 w = rayOrigin - a;
        const Vec3 u = b - a;
        const Vec3 v = rayDir;
        const float uu = dot(u, u);
        if (uu < 0.00001f) return;
        const float uv = dot(u, v);
        const float vv = dot(v, v);
        const float wu = dot(w, u);
        const float wv = dot(w, v);
        const float denom = uu * vv - uv * uv;
        if (std::fabs(denom) < 0.0001f) return;
        const float t = (uu * wv - uv * wu) / denom;
        const float s = (uv * wv - vv * wu) / denom;
        if (t < 0.0f || s < 0.0f || s > 1.0f) return;
        const Vec3 cp = rayOrigin + v * t;
        const Vec3 sp = a + u * s;
        const float d = distance(cp, sp);
        if (d < best) {
            best = d;
            result = ax;
        }
    };

    check(GizmoAxis::X, gizmoCenter, xEnd);
    check(GizmoAxis::Y, gizmoCenter, yEnd);
    check(GizmoAxis::Z, gizmoCenter, zEnd);

    return result;
}

Vec3 EditorGizmo::projectDrag(Vec3 rayOrigin, Vec3 rayDir, GizmoAxis axis, Vec3 gizmoCenter, Vec3 cameraPos) const {
    const Vec3 axisDir = axis == GizmoAxis::X   ? Vec3{1, 0, 0}
                         : axis == GizmoAxis::Y ? Vec3{0, 1, 0}
                                                : Vec3{0, 0, 1};

    const Vec3 toCamera = normalize(cameraPos - gizmoCenter);
    const Vec3 perp = normalize(cross(axisDir, cross(toCamera, axisDir)));
    const Vec3 planeN = normalize(cross(perp, axisDir));

    if (planeN.x == 0.0f && planeN.y == 0.0f && planeN.z == 0.0f) {
        return gizmoCenter;
    }

    const float denom = dot(rayDir, planeN);
    if (std::fabs(denom) < 0.0001f) {
        return gizmoCenter;
    }

    const float t = dot(gizmoCenter - rayOrigin, planeN) / denom;
    if (t < 0.0f) {
        return gizmoCenter;
    }

    const Vec3 worldPoint = rayOrigin + rayDir * t;
    const float proj = dot(worldPoint - gizmoCenter, axisDir);
    return gizmoCenter + axisDir * proj;
}

} // namespace bs3d
