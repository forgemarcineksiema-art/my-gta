# Runtime / gameplay systems quality checkpoint

Status: PLANNING
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
| Player movement feel | `PlayerMotor*` | Input response curves, speed states, ground detection |
| Camera | `CameraRig*` | Stability during turns, boom length, collision response |
| Vehicle handling | `VehicleController*` | Steering authority, drift feel, gear shifts, collision impact |
| World collision / props | `WorldCollision*`, `PropSimulation*` | Hit detection accuracy, support platform velocity |
| Mission flow | `MissionRuntimeBridge*`, `MissionOutcomeTrigger*` | Phase transitions, save/load consistency |
| Editor / dev tools | `RuntimeMapEditor*`, `DevTools*` | ImGui integration, isolation modes, asset preview |
| Smoke / CI hygiene | `tools/*.ps1`, `CMakeLists.txt` | Script reliability, test coverage gaps |
| Save / load | `SaveGame*`, `WorldDataLoader*` | Round-trip integrity, edge cases |

## Proposed next passes (outside renderer)

Each pass is small, testable, avoids huge rewrites, and does not touch D3D11.

### Pass 1: Camera stability during vehicle chase

**Scope:** `CameraRig`, chase mode parameters.
**Check:** Smoothing during sharp turns, boom gap recovery, camera collision with world.
**Test:** `--smoke-frames` with vehicle chase active, verify camera doesn't clip through buildings.

### Pass 2: Player movement feel audit

**Scope:** `PlayerMotor` speed curves, ground detection edge cases.
**Check:** Are on-foot speed transitions smooth? Does sprint/brake feel responsive?
**Test:** Smoke frames include on-foot movement, compare against expected feel.

### Pass 3: Vehicle gear shift / drift tuning

**Scope:** `VehicleController` shift timer, drift amount scaling.
**Check:** Does gear display match engine RPM? Is drift entry/exit predictable?
**Test:** Smoke with vehicle active, verify diagnostics show consistent RPM/gear/slip.

### Pass 4: Mission flow consistency

**Scope:** `MissionRuntimeBridge`, phase transitions, save/load round-trip.
**Check:** Does skipping through phases break state? Does save/load restore missions correctly?
**Test:** `--smoke-frames` with save/load cycle, verify mission phase unchanged.

### Pass 5: Prop simulation / world collision edge cases

**Scope:** `PropSimulationSystem`, `WorldCollision` support platform handling.
**Check:** Do props on moving platforms stay attached? Are collision hit zones accurate?
**Test:** Verify prop positions after riding moving platforms across multiple frames.

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
