# Development Workflow

Status: LIVE
Last verified against code: 2026-05-12
Owner: project workflow

See also: `docs/d3d11-modernization-milestone.md` for D3D11 renderer modernization status and boundaries.

Do not run random `blokowa_satyra.exe` files from Explorer.

Use the checked-in run scripts so the executable path and data root are explicit.

## Daily dev run

```powershell
.\tools\run_dev.ps1
```

This uses:

- **Preset:** `dev-tools`
- **Expected multi-config exe:** `build/dev-tools/Debug/blokowa_satyra.exe`
- **Fallback single-config exe:** `build/dev-tools/blokowa_satyra.exe`
- **Data root:** `<repo>/data`
- **Default game args:** `--data-root <repo>/data --no-load-save`

The script prints absolute root, binary directory, data root, and executable path before launch.

## Release smoke

```powershell
.\tools\run_release_smoke.ps1
```

This uses:

- **Preset:** `release`
- **Expected multi-config exe:** `build/release/Release/blokowa_satyra.exe`
- **Fallback single-config exe:** `build/release/blokowa_satyra.exe`
- **Data root:** `<repo>/data`
- **Default game args:** `--data-root <repo>/data --no-audio --no-save --no-load-save --smoke-frames 3`

## Quality gates

```powershell
python tools\validate_assets.py data\assets
python tools\validate_world_contract.py data --asset-root data\assets
cmake --preset ci
cmake --build --preset ci
ctest --preset ci
.\tools\ci_smoke.ps1 -Preset ci
```

Or run the combined CI helper:

```powershell
.\tools\ci_verify.ps1 -Preset ci
```

## D3D11 shadow sidecar smoke (dev-only, Windows)

Verifies the D3D11 shadow RenderFrame sidecar: diagnostics (no window) and full sidecar window with D3D11 rendering. Raylib remains the main runtime renderer. Does not activate `--renderer d3d11`.

```powershell
.\tools\d3d11_shadow_smoke.ps1 -Preset ci -Build
```

Without `-Build` the script expects an existing `blokowa_satyra.exe`.

### Capture/replay shadow frame

```powershell
.\build\ci\Debug\blokowa_satyra.exe --smoke-frames 3 --renderframe-shadow --renderframe-shadow-interval 1 --renderframe-shadow-dump artifacts\shadow_frame.txt
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --load-frame artifacts\shadow_frame.txt
```

### RenderFrame capture/replay smoke script

Scripted workflow that captures a live shadow `RenderFrame` from `blokowa_satyra` and replays it through `bs3d_d3d11_renderer_smoke` (both direct and factory paths). Does not activate `--renderer d3d11`.

```powershell
.\tools\renderframe_capture_replay.ps1 -Preset ci -Build
```

Without `-Build` the script expects both executables already built.

Output: `artifacts\shadow_frame.txt` (dump file with `RenderFrameDump v1` header).

### D3D11 game shell (standalone, Windows-only)

Standalone D3D11 main-window shell that loads a `RenderFrameDump v1` file and renders it through `D3D11Renderer` in its own Win32 window (1280x720). This is NOT GameApp integration — it does not use raylib, does not link `bs3d_game_support`, and does not activate `--renderer d3d11`.

```powershell
.\build\ci\Debug\bs3d_d3d11_game_shell.exe --frames 3 --load-frame artifacts\shadow_frame.txt
```

Run with `--diagnostics` to print frame stats and D3D11 draw coverage:

```powershell
.\build\ci\Debug\bs3d_d3d11_game_shell.exe --frames 3 --load-frame artifacts\shadow_frame.txt --diagnostics
```

Run with `--orbit-camera` for an inspection camera that can be moved with keyboard controls:

```powershell
.\build\ci\Debug\bs3d_d3d11_game_shell.exe --frames 120 --load-frame artifacts\shadow_frame.txt --diagnostics --orbit-camera
```

Add `--auto-orbit` (implies `--orbit-camera`) for slow automatic yaw rotation:

```powershell
.\build\ci\Debug\bs3d_d3d11_game_shell.exe --frames 120 --load-frame artifacts\shadow_frame.txt --diagnostics --orbit-camera --auto-orbit
```

**Orbit camera controls (time-based):**
- Left/Right arrows or A/D — rotate yaw (2.4 rad/s)
- W/S — zoom radius in/out (6.0 units/s)
- Q/E — lower/raise height (6.0 units/s)
- R — reset camera defaults (yaw=0, radius=8, height=4)
- F5 — reload dump file from --load-frame
- Auto-orbit speed: 1.2 rad/s

Use `--frames 0` to run until closed (Escape or window X) instead of auto-exiting after a fixed frame count:

Add `--wire-boxes` for a wireframe overlay on Box primitives:

```powershell
.\build\ci\Debug\bs3d_d3d11_game_shell.exe --frames 0 --load-frame artifacts\shadow_frame.txt --diagnostics --orbit-camera --wire-boxes
```

```powershell
.\build\ci\Debug\bs3d_d3d11_game_shell.exe --frames 0 --load-frame artifacts\shadow_frame.txt --diagnostics --orbit-camera
```

Or build and run through the capture/replay script:

```powershell
.\tools\renderframe_capture_replay.ps1 -Preset ci -Build
```

## Build folders

Official preset build folders live under `build/`:

- `build/debug`
- `build/release`
- `build/dev-tools`
- `build/ci-core`
- `build/ci`

`build-dev` is a legacy manual build folder. Do not use it for normal development. After confirming there is nothing local you need inside it, remove it manually with:

```powershell
Remove-Item -Recurse -Force .\build-dev
```

## Runtime identity

The game window title and debug HUD show build identity, DevTools status, executable path, and data root. If these do not point to the expected `build/dev-tools` executable and `<repo>/data`, stop and fix the launch path before debugging gameplay.
