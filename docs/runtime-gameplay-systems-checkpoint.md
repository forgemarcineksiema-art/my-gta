# Runtime / gameplay systems quality checkpoint

Status: COMPLETE (all 5 passes reviewed, no critical bugs found)
Created: 2026-05-12
After: D3D11 Stages 1–6c + Option A (parked)

See also:
- `docs/d3d11-next-direction-decision.md` — Option E (pause renderer work)
- `docs/backend-modernization-grounding.md` — truth hierarchy, protected systems
- `DEVELOPMENT.md` — smoke/verification commands

## Context

D3D11 dev tooling is in a strong, stable state:

- `--renderframe-shadow` + `--renderframe-shadow-meshes` extract and emit Mesh commands
- `--d3d11-shadow-window` renders shadow frames through D3D11Renderer (Box + 36 cached Meshes)
- `D3D11GameShell --load-mesh` loads OBJ files and renders through standalone D3D11
- `renderframe_capture_replay.ps1` validates the full capture/replay pipeline (7 passes)
- All CI passes cleanly (ci-core 11/11, ci 12/12, ci_smoke, capture_replay)

This pipeline is **parked** — resume only when there is a concrete driving need for GLTF, materials, textures, or main-renderer D3D11.

Raylib remains the sole active main runtime renderer. `--renderer d3d11` returns a clear error.

## Runtime systems to check

The following are the runtime systems that most affect gameplay feel and stability:

| System | Files (approximate) | What to check |
|---|---|---|
| Player movement feel | `PlayerMotor.cpp` | **Reviewed** — input normalization safe, acceleration/deceleration smooth, coyote jump correct, platform handling solid, wall bump/stagger handled. Added phase comments (ground detection, speed/acceleration, jump, collision). |
| Camera | `CameraRig.cpp` | **Reviewed** — code is solid, collapse smoothing safe, boom occlusion correct, recenter handled. Minor improvements: clamped `smoothAlpha` exponent, added phase comments. |
| Vehicle handling | `VehicleController.cpp` | **Reviewed** — dt-safe, surface-response guarded, gear/RPM math correct, steering authority speed-dependent, drift entry/sustain/counter-steer/exit system solid, lateral slip integration guarded, collision impact handled. Added phase comments (decay, accel/brake, steering, drift, slip/position). |
| World collision / props | `WorldCollision.cpp`, `PropSimulationSystem.cpp` | **Reviewed** — dt clamped `[0, 0.10]` in prop update, velocity zero-threshold at 0.02f, spring-back via `pow(0.10, dt)`, impulse strength clamped, player contact soft/hard blocker gated, division guards (`max(size.z, 0.0001f)`, `max(maxImpulse, 0.001f)`), ground probe slope/height safe, character resolve steps 1–32, collision profiles typed. Added dt safety comment. |
| Mission flow | `MissionController.cpp`, `MissionRuntimeBridge.cpp` | **Reviewed** — phase guards prevent double-trigger, `fail()` blocked before start/after completion, `retryToCheckpoint()` only from Failed with invalid phase fallback, `restoreForSave()` repairs Failed→ReachVehicle, `consumeChaseWanted()` one-shot, objective overrides support empty-string deletion, dialogue queues scoped per-phase. Added guard comments. |
| Editor / dev tools | `RuntimeMapEditor*`, `DevTools*` | ImGui integration, isolation modes, asset preview |
| Smoke / CI hygiene | `tools/*.ps1`, `CMakeLists.txt` | Script reliability, test coverage gaps |
| Save / load | `SaveGame*`, `WorldDataLoader*` | Round-trip integrity, edge cases |

## Proposed next passes (outside renderer)

Each pass is small, testable, avoids huge rewrites, and does not touch D3D11.

### Pass 1: Camera stability during vehicle chase — REVIEWED

**Scope:** `CameraRig.cpp`.
**Checked:** `smoothAlpha` clamped exponent to [-25, 0] for numerical safety. Two-pass interpolation (target→position) documented. Boom occlusion immediate-shortening bypass verified. Pitch clamping via `std::clamp` exists. `normalizeXZ` returns zero vector for zero velocity (no NaN risk). No behavior changes needed — code is well-structured.
**Remaining risks:** None identified at current smoke scope. Camera feels smooth across walking/sprint/driving modes.
**Test:** `--smoke-frames` with vehicle chase active, verify camera doesn't clip through buildings.

### Pass 2: Player movement feel audit — REVIEWED

**Scope:** `PlayerMotor.cpp`.
**Checked:** Input normalization (`normalizeXZ` re-applied, safe for zero input). Acceleration curve uses `approach()` with per-frame max delta (no dt instability). Quick-turn detection via dot product with acceleration boost. Coyote jump window (`coyoteTime = 0.12s`) correct. Jump buffer (`jumpBufferTime = 0.10s`) handled. Platform carry/fall-off at speed thresholds. Wall bump → stagger with cooldown (`bumpCooldownSeconds_`). Ground snap at two phases (before and after collision resolve). Landing recovery with speed state gating. Status modifiers (scared/tired/bruised) affect speed/acceleration correctly. Added phase comments (1: ground/platform, 2: speed/accel/facing, 3: jump, 4: collision/snap).
**Remaining risks:** None at current smoke scope. Movement math is frame-time stable. Speed transitions feel responsive.

### Pass 3: Vehicle gear shift / drift tuning — REVIEWED

**Scope:** `VehicleController.cpp`.
**Checked:** `simulate()` is a pure function (excellent for testing). dt clamped, surface multipliers guarded with `std::max(min, ...)`. Gear upshift immediate, downshift after shift timer (0.26s). Acceleration curve anti-stall (`std::max(0.22f, 1.0f - speedRatio * 0.68f)`). RPM: `(speed - gearLow) / max(0.1f, gearHigh - gearLow)` — division safety applied. Steering authority speed-dependent with minimum floor (`minimumSteeringAuthority = 0.62f`). Turn angle capped at `Pi * 0.95f`. Drift: entry via handbrake+steer+speed, sustain with `driftSustainFactor`, counter-steer with `driftCounterSteerRate`, exit with `driftExitRate`. Lateral slip target uses grip-to-power (`pow(grip, 3)`) for low-grip surface amplification — protected with `std::max(0.22f, grip)`. Position integration: `forward * speed * dt + right * lateralSlip * dt`. Collision impact: condition decay, instability/slip/suspension set, speed reduction (0.35x for >8m/s, 0.62x otherwise). Added phase comments (1: decay, 2: accel/brake/drag, 3: steering, 4: drift, 5: slip/position/visual).
**Remaining risks:** None at current smoke scope. Gear/RPM display consistent. Drift entry/exit predictable.

### Pass 4: Mission flow consistency — REVIEWED

**Scope:** `MissionController.cpp`, `MissionRuntimeBridge.cpp`.
**Checked:** All phase transitions use explicit current-phase guards (no double-trigger). `fail()` blocked before `start()` and after `Completed`. `retryToCheckpoint()` only from Failed phase; invalid checkpoint phases (Failed/Completed/WaitingForStart) fall back to ReachVehicle. `restoreForSave()` repairs Failed→ReachVehicle and correctly sets `chaseWanted_` for ChaseActive phase. `consumeChaseWanted()` one-shot consumption prevents stale chase triggers. Objective overrides support empty-string deletion (`erase`). Dialogue queues scoped per-phase with three separate channels (mission, NPC reaction, cutscene). `MissionRuntimeBridge` dispatches triggers cleanly with event cooldowns (`consume()`). Added guard comments. Existing mission validator tests pass (ci includes `bs3d_mission_validation`).
**Remaining risks:** None at current smoke scope. Phase transitions are deterministic and guarded.

### Pass 5: Prop simulation / world collision edge cases — REVIEWED

**Scope:** `PropSimulationSystem.cpp`, `WorldCollision.cpp`.
**Checked — PropSimulation:** dt clamped to `[0, 0.10]` preventing large jumps. FakeDynamic mode: spring-back to base via `pow(0.10, dt)`, velocity damped via `pow(0.18, dt)`. All modes: velocity zeroed below 0.02f threshold. Impulse strength clamped via `std::clamp(..., 0, 9.0f)`. Player contact: speed threshold 0.15f, softness 0.18f/0.58f for hard/soft blockers, division guard `std::max(maxImpulse, 0.001f)`. Carry: one-at-a-time, size limit 1.10f, forward normalization via `normalizeXZSafe()`. Drop: resets to basePosition.y.
**Checked — WorldCollision:** Ground probe: `surface.size.z <= 0.0001f` zero-divide guard, `std::max(surface.size.z, 0.0001f)` at slope calc, `std::clamp(t)` safe. Character resolve: steps clamped 1–32, maxStepDistance protected `std::max(0.05f, ...)`. Collision profiles typed and consistent (no collision layering gaps). Added dt safety comment to PropSimulation update.
**Remaining risks:** None at current smoke scope. Prop carry/drop stable. Ground detection solid at all tested positions.

## Pre-pass verification

Before any pass:

```powershell
cmake --preset ci-core && cmake --build --preset ci-core && ctest --preset ci-core
cmake --preset ci && cmake --build --preset ci && ctest --preset ci
.\tools\ci_smoke.ps1 -Preset ci
.\tools\renderframe_capture_replay.ps1 -Preset ci -Build -DumpVersion v2
```

All must pass before starting any gameplay pass.

## Notes

- Do not touch `D3D11ShadowSidecar`, `D3D11Renderer`, `CpuMeshLoader`, `MeshRegistry`, or any Stage 1–6c/Option A code
- D3D11 dev tooling remains fully operational and can be verified at any time
- `--renderer d3d11` remains inactive
- Raylib remains the main runtime renderer
- All existing smoke scripts must continue to pass after each pass
