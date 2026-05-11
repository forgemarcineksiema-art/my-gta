#pragma once

#include "bs3d/core/Types.h"

namespace bs3d {

enum class ControlContext {
    OnFootExploration,
    OnFootPanic,
    VehicleDriving,
    InteractionLocked,
    Combat,
    CarryObject,
    Ragdoll
};

struct ControlContextState {
    bool playerInVehicle = false;
    bool interactionLocked = false;
    bool combatFocus = false;
    bool carryingObject = false;
    bool ragdoll = false;
    bool highPrzypal = false;
};

struct RawInputState {
    bool moveForwardDown = false;
    bool moveBackwardDown = false;
    bool moveLeftDown = false;
    bool moveRightDown = false;
    bool fastDown = false;
    bool carefulDown = false;
    bool dynamicPressed = false;
    bool dynamicDown = false;
    bool usePressed = false;
    bool vehiclePressed = false;
    bool primaryPressed = false;
    bool primaryDown = false;
    bool secondaryPressed = false;
    bool secondaryDown = false;
    bool carryPressed = false;
    bool pushPressed = false;
    bool drinkPressed = false;
    bool smokePressed = false;
    bool lowVaultPressed = false;
    bool fallPressed = false;
    bool hornPressed = false;
    bool hornDown = false;
    bool retryPressed = false;
    bool toggleDebugPressed = false;
    bool toggleRuntimeEditorPressed = false;
    bool mouseCaptureTogglePressed = false;
    bool fullscreenTogglePressed = false;
    bool invertMouseTogglePressed = false;
    bool stableCameraTogglePressed = false;
    bool renderIsolationTogglePressed = false;
    bool assetPreviewNextPressed = false;
    bool assetPreviewPreviousPressed = false;
    bool visualBaselineTogglePressed = false;
    bool posePreviewTogglePressed = false;
    bool posePreviewNextPressed = false;
    bool posePreviewPreviousPressed = false;
    bool mouseLookActive = true;
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
};

ControlContext resolveControlContext(const ControlContextState& state);
void clearRawInputFrameEdges(RawInputState& raw);
InputState mapRawInputToInputState(const RawInputState& raw, ControlContext context);

} // namespace bs3d
