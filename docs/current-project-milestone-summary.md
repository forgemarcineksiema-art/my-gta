# Current project milestone summary

Status: LIVE
Created: 2026-05-12
After: D3D11 Stages 1–6c + Option A, runtime/gameplay checkpoints, tooling hygiene

See also:
- `docs/backend-modernization-grounding.md` — truth hierarchy, protected systems, non-goals
- `docs/runtime-gameplay-systems-checkpoint.md` — focused pass results
- `docs/d3d11-next-direction-decision.md` — renderer next-direction options
- `DEVELOPMENT.md` — smoke/verification commands

## Completed

### D3D11 dev/shadow/tooling pipeline (parked)

| Stage | What | Status |
|---|---|---|
| 1–2 | MeshRegistry + MaterialRegistry + D3D11MeshCache | DONE |
| 3a–3b | CpuMeshData + D3D11GameShell procedural mesh | DONE |
| 4 | RenderFrameDump v2 (Mesh command serialization) | DONE |
| 5a–5d | Mesh extraction, GameApp shadow wiring, sidecar upload, seed selection | DONE |
| 6a–6c | CpuMeshLoader minimal OBJ, D3D11GameShell --load-mesh, modelPath sidecar upload | DONE |
| Option A | Broader WorldAssetRegistry integration (seed limit 16) | DONE |

D3D11 pipeline is **parked** — dev tooling is stable, all smokes pass. Resume only with a concrete driving need for GLTF, materials, textures, or main-renderer D3D11.

### Runtime / gameplay systems checkpoint (complete)

| System | Result |
|---|---|
| Camera stability (`CameraRig.cpp`) | Reviewed — smoothAlpha exponent clamped, phase comments added |
| Player movement (`PlayerMotor.cpp`) | Reviewed — update phases commented, all edge cases safe |
| Vehicle handling (`VehicleController.cpp`) | Reviewed — simulate phases commented, drift/gear math solid |
| Mission flow (`MissionController.cpp`, `MissionRuntimeBridge.cpp`) | Reviewed — phase guards confirmed, save/load consistency verified |
| World collision / props (`WorldCollision.cpp`, `PropSimulationSystem.cpp`) | Reviewed — dt clamped, division guards, collision profiles typed |
| Save / load (`SaveGame.cpp`) | Reviewed — atomic write, validation, version lock, array caps |

### Tooling hygiene (complete)

| Area | Result |
|---|---|
| Smoke / CI hygiene (`tools/*.ps1`) | Reviewed — artifact dir guard added, clear error messages, no duplicate builds |
| Editor / DevTools (`EditorOverlayApply.cpp`, `EditorOverlayCodec.cpp`, `RuntimeMapEditor.cpp`) | Reviewed — phase comments added, null guards confirmed, JSON codec validated |

## Current state

- Raylib is the sole active main runtime renderer
- `--renderer d3d11` returns a clear error — NOT implemented
- `--renderframe-shadow` + `--renderframe-shadow-meshes` extract and emit Mesh commands
- `--d3d11-shadow-window` renders shadow frames through D3D11Renderer (Box + 36 cached Meshes)
- `D3D11GameShell --load-mesh` loads OBJ files and renders through standalone D3D11
- `renderframe_capture_replay.ps1` validates the full capture/replay pipeline (7 passes)
- All CI passes (ci-core 11/11, ci 12/12, ci_smoke, d3d11_shadow_smoke, capture_replay)
- `WorldModelCache` unchanged — raylib `Model` loading is the production path

## Recommended next work

- **Gameplay content / mission expansion**: New mission phases, narrative progression
- **World dressing / interaction polish**: NPC placement, prop variety, visual landmarks
- **Small player/vehicle feel tuning**: Only if manually observed — do not preemptively rewrite
- **No renderer expansion** unless explicitly resumed from the parked state

## Pre-work verification

```powershell
cmake --preset ci-core && cmake --build --preset ci-core && ctest --preset ci-core
cmake --preset ci && cmake --build --preset ci && ctest --preset ci
.\tools\ci_smoke.ps1 -Preset ci
.\tools\renderframe_capture_replay.ps1 -Preset ci -Build -DumpVersion v2
```
