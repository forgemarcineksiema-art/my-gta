#pragma once

#include "RuntimeMapEditor.h"
#include "WorldArtTypes.h"
#include "bs3d/core/Types.h"

namespace bs3d {

enum class GizmoAxis { None, X, Y, Z };

enum class GizmoDragState { Idle, Dragging };

class EditorGizmo {
public:
    // Called each fixedUpdate frame with input
    void update(const InputState& input, RuntimeMapEditor& editor, Vec3 cameraPosition, Vec3 cameraTarget,
                const std::vector<WorldObject>& objects);

    // Called each draw frame — builds rays from camera and mouse position
    void processFrame(RuntimeMapEditor& editor, Vec3 cameraPos, Vec3 cameraTarget, float fovY,
                      const std::vector<WorldObject>& objects);

    // Draws translation arrows at object position (only in dev tools)
    void drawSelectedGizmo(Vec3 objectPosition, Vec3 cameraPosition) const;

    void resetDrag();
    GizmoDragState dragState() const;

private:
    GizmoAxis hitTestAxis(Vec3 rayOrigin, Vec3 rayDir, Vec3 gizmoCenter, float gizmoScale) const;
    Vec3 projectDrag(Vec3 rayOrigin, Vec3 rayDir, GizmoAxis axis, Vec3 gizmoCenter,
                     Vec3 cameraPosition) const;

    GizmoAxis grabbedAxis_ = GizmoAxis::None;
    GizmoDragState dragState_ = GizmoDragState::Idle;

    // Stored from update() via raylib, consumed in draw()
    struct { float x = 0.0f; float y = 0.0f; } mouseScreenPos_{};
    bool primaryPressed_ = false;
    bool primaryDown_ = false;
    bool pendingClick_ = false;
};

// Standalone raycast utilities
float raycastAabb(Vec3 origin, Vec3 direction, Vec3 boxCenter, Vec3 boxSize, float maxDistance = 1e6f);
float raycastObb(Vec3 origin, Vec3 direction, Vec3 boxCenter, Vec3 boxSize, float yawRadians,
                 float maxDistance = 1e6f);
float raycastSphere(Vec3 origin, Vec3 direction, Vec3 sphereCenter, float radius, float maxDistance = 1e6f);
const WorldObject* raycastSelectObject(Vec3 rayOrigin, Vec3 rayDir, const std::vector<WorldObject>& objects);

} // namespace bs3d
