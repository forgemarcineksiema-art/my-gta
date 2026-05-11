#include "RaylibInputReader.h"

#include "raylib.h"

namespace bs3d {

RawInputState RaylibInputReader::read(bool mouseCaptured) const {
    RawInputState raw;
    raw.moveForwardDown = IsKeyDown(KEY_W);
    raw.moveBackwardDown = IsKeyDown(KEY_S);
    raw.moveLeftDown = IsKeyDown(KEY_A);
    raw.moveRightDown = IsKeyDown(KEY_D);
    raw.carefulDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    raw.fastDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    raw.dynamicPressed = IsKeyPressed(KEY_SPACE);
    raw.dynamicDown = IsKeyDown(KEY_SPACE);
    raw.usePressed = IsKeyPressed(KEY_E);
    raw.vehiclePressed = IsKeyPressed(KEY_F);
    raw.primaryPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    raw.primaryDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    raw.secondaryPressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    raw.secondaryDown = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    raw.carryPressed = IsKeyPressed(KEY_C);
    raw.pushPressed = IsKeyPressed(KEY_X);
    raw.drinkPressed = IsKeyPressed(KEY_Z);
    raw.smokePressed = IsKeyPressed(KEY_V);
    raw.lowVaultPressed = IsKeyPressed(KEY_Q);
    raw.fallPressed = IsKeyPressed(KEY_G);
    raw.hornPressed = IsKeyPressed(KEY_H);
    raw.hornDown = IsKeyDown(KEY_H);
    raw.retryPressed = IsKeyPressed(KEY_R);
    raw.toggleDebugPressed = IsKeyPressed(KEY_F1);
    raw.toggleRuntimeEditorPressed = IsKeyPressed(KEY_F10);
    raw.mouseCaptureTogglePressed = IsKeyPressed(KEY_F2) || IsKeyPressed(KEY_ESCAPE);
    raw.fullscreenTogglePressed = IsKeyPressed(KEY_F11);
    raw.invertMouseTogglePressed = IsKeyPressed(KEY_F3);
    raw.stableCameraTogglePressed = IsKeyPressed(KEY_F5);
    raw.renderIsolationTogglePressed = IsKeyPressed(KEY_F6);
    raw.assetPreviewNextPressed = IsKeyPressed(KEY_F7) &&
                                  !(IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
    raw.assetPreviewPreviousPressed = IsKeyPressed(KEY_F7) &&
                                      (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
    raw.visualBaselineTogglePressed = IsKeyPressed(KEY_F8);
    raw.posePreviewTogglePressed = IsKeyPressed(KEY_F9);
    raw.posePreviewNextPressed = IsKeyPressed(KEY_RIGHT_BRACKET);
    raw.posePreviewPreviousPressed = IsKeyPressed(KEY_LEFT_BRACKET);
    raw.mouseLookActive = mouseCaptured;
    const Vector2 mouseDelta = GetMouseDelta();
    raw.mouseDeltaX = mouseCaptured ? mouseDelta.x : 0.0f;
    raw.mouseDeltaY = mouseCaptured ? mouseDelta.y : 0.0f;
    return raw;
}

RawInputState readRaylibRawInput(bool mouseCaptured) {
    return RaylibInputReader{}.read(mouseCaptured);
}

} // namespace bs3d
