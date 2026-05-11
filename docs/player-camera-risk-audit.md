# Player / Camera Risk Audit

## Position

We are not 100% certain about player feel. Unit tests can prove important invariants, but they cannot prove that movement "feels good" in a 10 minute playtest. This audit records concrete risks found in code and what protects them now.

## Fixed In This Audit

### 0.6.2 On-foot feel pass

Risk:

- `walkSpeed` existed in config but default on-foot movement only used jog/sprint.
- Direction reversal could preserve too much old velocity and feel like skating.
- Jump input pressed just before landing could be lost.
- Ground snap reused step height too aggressively, so the player could be considered grounded too high above the floor.
- Normal on-foot camera framing was a bit too distant/narrow for close osiedle play.

Fix:

- Added Ctrl walk, default WASD jog, and Shift sprint as distinct speed states.
- Added quick-turn acceleration when movement intent sharply opposes current velocity.
- Added a short jump buffer.
- Separated vertical ground snap from step height so steps and landing forgiveness do not create early fake grounding.
- Moved normal on-foot camera closer, raised its target to upper body, and widened FOV.

Regression tests:

- `playerMotorUsesWalkJogAndSprintSpeedStates`
- `playerMotorQuickTurnsWithoutLongForwardSlide`
- `playerMotorBuffersJumpJustBeforeLanding`
- `cameraRigOnFootNormalUsesCloserReadableFraming`

### 1. On-foot camera auto-recenter fought jogging

Risk:

- Jogging, strafing, or moving backward could trigger auto-recenter after mouse input stopped.
- Because the recenter target was the smoothed character facing yaw, the camera could rotate under the player's input and make `WASD` feel unstable.

Fix:

- On-foot auto-recenter is now limited to sprint mode.
- Normal jogging keeps the player's manual camera intent.
- Sprint has its own shorter recenter delay, so panic/sprint can still get camera help.

Regression tests:

- `cameraRigDoesNotFightJoggingWithOnFootAutoRecenter`
- `cameraRigRecentersOnFootOnlyForSprintPressure`

### 2. Camera boom could stay occluded when an obstacle appeared

Risk:

- The camera collision target was resolved, but final camera position was smoothed from the old position.
- On a sudden wall/close obstacle, the camera could remain behind geometry for several frames.

Fix:

- Boom shortening now applies immediately when collision makes the safe boom much shorter.
- Returning outward remains smoothed.

Regression test:

- `cameraRigDoesNotRemainOccludedWhenBoomObstacleAppears`

### 3. Camera boom minimum distance was too large for close walls

Risk:

- `resolveCameraBoom` forced at least 1m of distance from target.
- If a wall was closer than that, the camera could still resolve behind/inside the obstacle.

Fix:

- Minimum safe boom distance is now tiny, allowing the camera to move almost to the target when a wall is very close.

Regression test:

- `cameraBoomCanResolveVeryCloseObstacles`

### 4. Character collision could tunnel through thin walls on large frames

Risk:

- `resolveCharacter` checked the final destination strongly, but not the whole path.
- A large frame spike could move the player past a thin wall.

Fix:

- Character movement is now resolved in bounded substeps.
- Each substep runs the same step/slope/wall logic.

Regression test:

- `characterCollisionDoesNotTunnelThroughThinWallsOnLargeFrame`

### 5. WASD camera-relative mapping needed fuller coverage

Risk:

- Existing tests covered only a few yaw cases.
- Cardinal camera directions are where inverted or rotated controls become obvious.

Fix:

- Added cardinal yaw tests for `W`, `A`, and `D`.

Regression test:

- `playerInputMapsWasdConsistentlyAcrossCardinalCameraYaws`
- `playerInputMapsDiagonalWasdConsistentlyAcrossCardinalCameraYaws`

### 6. Mouse orbit direction can be misread from boom movement

Risk:

- In third-person orbit, moving the mouse right turns the view direction right, but the physical camera boom can move left around the character.
- This can feel like inverted camera if we debug only the camera position instead of view direction.

Fix:

- Added a regression test for mouse-right increasing view direction to the right.
- Debug overlay now draws camera-forward, camera-right, and player velocity lines.

Regression test:

- `cameraRigMouseRightTurnsViewDirectionRight`

### 7. On-foot camera comfort still had twitch/lag risks

Risk:

- Mouse sensitivity was fast enough that small desktop swipes could overshoot the intended view.
- Sprint auto-align could start too soon after manual mouse look and feel like the camera was pulling against the player.
- On-foot target/position smoothing could lag almost a meter behind sprinting movement, making correct input feel rubbery.

Fix:

- Reduced default mouse sensitivity.
- Increased sprint recenter comfort delay.
- Tightened on-foot target and camera position smoothing while leaving vehicle lag heavier.

Regression tests:

- `cameraRigMouseSensitivityIsControlledForDesktopLook`
- `cameraRigSprintAutoAlignWaitsBeforeHelping`
- `cameraRigOnFootTrackingLagStaysTightAtSprintSpeed`

### 8. v0.6.7 feel gate needed explicit acceptance thresholds

Risk:

- "Feels better" was too vague to protect controls across future Przypal/NPC work.
- Debug overlay showed camera yaw and boom, but not enough to diagnose target lag, pitch, or current movement state.
- Vehicle handbrake/damage behavior had tests for activation, but not recovery/readability after the chaos moment.

Fix:

- Added feel-gate tests for sprint stop distance, wall slide recovery, camera target lag debug data, camera boom recovery, handbrake slip recovery, and damaged gruz steering/boost readability.
- `CameraRigState` now exposes `desiredTarget` and `fullBoomLength` for tests and debug overlay.
- Debug overlay now shows camera mode, yaw, pitch, boom/full boom, target lag, and player speed state.
- Manual checklist now has numeric pass/fail thresholds for target lag, stop distance, bump lock, boom recovery, and vehicle recovery.

Regression tests:

- `feelGatePlayerStopsWithinShortReadableDistanceAfterSprint`
- `feelGatePlayerSlidesAlongWallWithoutLongStaggerLock`
- `feelGateCameraExposesTargetLagForDebugOverlay`
- `feelGateCameraBoomReturnsOutSmoothlyAfterWallClears`
- `feelGateVehicleHandbrakeSlipRecoversAfterRelease`
- `feelGateDamagedVehicleBoostAndSteeringRemainReadable`

### 9. v0.7 world reactions could accidentally degrade feel

Risk:

- Przypal/world-memory events could be emitted every frame from an active event, turning one crash or chase into runaway heat.
- Reaction subtitles could cover mission/chase/fail UI and make the player miss critical state.
- Przypal feedback could accidentally change input mapping, camera yaw, movement direction, or vehicle steering.

Fix:

- `WorldEventLedger` is memory only; heat rises only from the one-shot `WorldEventAddResult` returned by `ledger.add()`.
- Repeated nearby events merge with heat damping and a stack cap, while emitter cooldowns block `ChaseSeen`, `PropertyDamage`, horn/noise, and shop spam.
- `NpcReactionSystem` chooses reactions with global/source cooldowns, line variants, and mission-UI busy handling.
- Runtime Przypal affects HUD/feedback/camera pressure only; it does not mutate `InputState`, `PlayerMotor`, or camera control yaw.

Regression tests:

- `activeWorldEventDoesNotAddPrzypalAgainWithoutNewPulse`
- `worldEventLedgerMergesNearbyRepeatsWithDampenedHeatAndStackCap`
- `worldEventEmitterCooldownsBlockChaseSeenSpamAndFilterTinyDamage`
- `npcReactionSystemPicksDeterministicVariantsWithoutImmediateRepeat`
- `missionUiBusyBlocksLowPriorityGossipButKeepsLedgerMemory`
- `worldReactionsDoNotMutateInputMovementOrCameraState`

### 10. v0.8.1 intro readability could block the whole mission loop

Risk:

- The shop existed, but the player had only a weak world marker and could miss it.
- The gruz was intentionally locked until the player returned from the shop to Bogus, but the silent lock looked like a bug.
- `Esc` used raylib's default quit behavior, which made cursor testing frustrating.
- Floating world labels competed with subtitles and made the screen feel noisy.

Fix:

- Added a screen-space objective arrow/label with distance for Bogus, Zenon's shop, gruz, and parking.
- Added a clear locked-vehicle prompt/line before the gruz is unlocked.
- Disabled `Esc` as quit; `Esc/F2` now toggle mouse capture, and `F11` toggles fullscreen.
- Replaced floating dressing labels with stronger in-world primitive signage and beacons.
- Shortened objective text and lengthened canon intro dialogue.

Regression tests:

- `missionObjectiveTextIsShortCanonAndPlaceholderFree`
- `missionCanonDialogueUsesReadableDurationsAndNoOldSpeakers`

## Still Not Proven

These are not solved just because tests pass:

- camera height/distance taste;
- sprint FOV taste;
- indoor/stair camera behavior;
- vehicle collision sweep on large frames;
- vehicle chase camera under long play;
- gamepad support;
- animation/readability of facing and feet;
- real NPC/prop collision comedy;
- whether players understand controls after 30 seconds.
- whether the debug direction lines match player perception during a real playtest.
- whether the new objective arrow is enough without a minimap.
- whether fullscreen behaves well on every monitor/DPI setup.

## Next Debug Cycle If Controls Still Feel Wrong

1. Record exact playtest complaint: walking, sprint, turning, camera orbit, camera recenter, wall, vehicle, or chase.
2. Reproduce with a minimal scenario.
3. Add a core regression test for the invariant if possible.
4. Make one behavior change only.
5. Run `cmake --build build --config Release`.
6. Run `ctest --test-dir build -C Release --output-on-failure`.
7. Do the manual checklist before calling the pass better.
