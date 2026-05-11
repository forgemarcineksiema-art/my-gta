#include "bs3d/core/ControlContext.h"

namespace bs3d {

namespace {

void mapSharedMeta(const RawInputState& raw, InputState& input) {
    input.retryPressed = raw.retryPressed;
    input.toggleDebugPressed = raw.toggleDebugPressed;
    input.toggleRuntimeEditorPressed = raw.toggleRuntimeEditorPressed;
    input.mouseCaptureTogglePressed = raw.mouseCaptureTogglePressed;
    input.mouseLookActive = raw.mouseLookActive;
    input.mouseDeltaX = raw.mouseDeltaX;
    input.mouseDeltaY = raw.mouseDeltaY;
}

void mapOnFootMovement(const RawInputState& raw, InputState& input) {
    input.moveForward = raw.moveForwardDown;
    input.moveBackward = raw.moveBackwardDown;
    input.moveLeft = raw.moveLeftDown;
    input.moveRight = raw.moveRightDown;
    input.walk = raw.carefulDown;
    input.sprint = raw.fastDown;
    input.jumpPressed = raw.dynamicPressed;
}

void mapOnFootActions(const RawInputState& raw, InputState& input) {
    input.primaryActionPressed = raw.primaryPressed;
    input.secondaryActionPressed = raw.secondaryPressed;
    input.carryActionPressed = raw.carryPressed;
    input.pushActionPressed = raw.pushPressed;
    input.drinkActionPressed = raw.drinkPressed;
    input.smokeActionPressed = raw.smokePressed;
    input.lowVaultActionPressed = raw.lowVaultPressed;
    input.fallActionPressed = raw.fallPressed;
}

void mapVehicleMovement(const RawInputState& raw, InputState& input) {
    input.accelerate = raw.moveForwardDown;
    input.brake = raw.moveBackwardDown;
    input.steerLeft = raw.moveLeftDown;
    input.steerRight = raw.moveRightDown;
    input.handbrake = raw.dynamicDown;
    input.vehicleBoost = raw.fastDown;
    input.hornPressed = raw.hornPressed || raw.primaryPressed;
    input.hornDown = raw.hornDown || raw.primaryDown;
}

} // namespace

ControlContext resolveControlContext(const ControlContextState& state) {
    if (state.interactionLocked) {
        return ControlContext::InteractionLocked;
    }
    if (state.ragdoll) {
        return ControlContext::Ragdoll;
    }
    if (state.playerInVehicle) {
        return ControlContext::VehicleDriving;
    }
    if (state.combatFocus) {
        return ControlContext::Combat;
    }
    if (state.carryingObject) {
        return ControlContext::CarryObject;
    }
    if (state.highPrzypal) {
        return ControlContext::OnFootPanic;
    }
    return ControlContext::OnFootExploration;
}

void clearRawInputFrameEdges(RawInputState& raw) {
    raw.dynamicPressed = false;
    raw.usePressed = false;
    raw.vehiclePressed = false;
    raw.primaryPressed = false;
    raw.secondaryPressed = false;
    raw.carryPressed = false;
    raw.pushPressed = false;
    raw.drinkPressed = false;
    raw.smokePressed = false;
    raw.lowVaultPressed = false;
    raw.fallPressed = false;
    raw.hornPressed = false;
    raw.retryPressed = false;
    raw.toggleDebugPressed = false;
    raw.toggleRuntimeEditorPressed = false;
    raw.mouseCaptureTogglePressed = false;
    raw.fullscreenTogglePressed = false;
    raw.invertMouseTogglePressed = false;
    raw.stableCameraTogglePressed = false;
    raw.renderIsolationTogglePressed = false;
    raw.assetPreviewNextPressed = false;
    raw.assetPreviewPreviousPressed = false;
    raw.visualBaselineTogglePressed = false;
    raw.posePreviewTogglePressed = false;
    raw.posePreviewNextPressed = false;
    raw.posePreviewPreviousPressed = false;
    raw.mouseDeltaX = 0.0f;
    raw.mouseDeltaY = 0.0f;
}

InputState mapRawInputToInputState(const RawInputState& raw, ControlContext context) {
    InputState input;
    mapSharedMeta(raw, input);
    input.enterExitPressed = raw.usePressed || raw.vehiclePressed;

    switch (context) {
    case ControlContext::OnFootExploration:
    case ControlContext::OnFootPanic:
        mapOnFootMovement(raw, input);
        mapOnFootActions(raw, input);
        break;
    case ControlContext::VehicleDriving:
        mapVehicleMovement(raw, input);
        break;
    case ControlContext::Combat:
        mapOnFootMovement(raw, input);
        mapOnFootActions(raw, input);
        input.jumpPressed = false;
        break;
    case ControlContext::CarryObject:
        mapOnFootMovement(raw, input);
        mapOnFootActions(raw, input);
        input.walk = true;
        input.sprint = raw.fastDown;
        input.jumpPressed = false;
        break;
    case ControlContext::InteractionLocked:
        input.interactionLocked = true;
        break;
    case ControlContext::Ragdoll:
        break;
    }

    return input;
}

} // namespace bs3d
