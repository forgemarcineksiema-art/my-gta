# D3D11 renderer modernization milestone

Status: LIVE (documentation only)
Last verified against implementation: 2026-05-12

## Current status

### Implemented (standalone/smoke tooling)

| Component | Status | Notes |
|---|---|---|
| `bs3d_d3d11_boot` | Spike | Hardcoded indexed colored cube in standalone Win32 window |
| `D3D11Renderer` | Standalone | Implements `IRenderer`, private D3D11 device/swapchain/shader init, Box + debug-line drawing |
| `bs3d_d3d11_renderer_smoke` | Standalone | Smoke executable exercising D3D11Renderer with procedural frames, RenderFrameBuilder, WorldRenderList extraction, RendererFactory |
| `RenderFrame` / `RenderFrameBuilder` | Backend-neutral | Production-order primitive accumulation, validation, summarization |
| `WorldRenderList` extraction | Backend-neutral | WorldObject → RenderFrameBuilder fallback Box pipeline |
| `NullRenderer` / `RecordingRenderer` | Test | No-op renderers for data-only test paths |
| `RendererFactory` | Gated | Creates `D3D11Renderer` only via `allowExperimentalD3D11Renderer = true` |
| `RenderFrameDump v1` | Capture/replay | Text file serialize/deserialize of camera + Box primitives + debug lines |
| `bs3d_d3d11_game_shell` | Standalone | 1280x720 Win32 window, loads dump file, renders through D3D11Renderer |
| `--renderframe-shadow` | GameApp dev flag | Builds shadow RenderFrame from live WorldRenderList, feeds to NullRenderer |
| `--d3d11-shadow-window` | GameApp dev flag | Opens sidecar Win32 window, renders shadow frame through D3D11Renderer |
| `--d3d11-shadow-diagnostics` | GameApp dev flag | Logs bucket stats, D3D11 coverage, coverage ratios (supportedBoxes, boxCoveragePct, lineCoveragePct) |

### Game shell features

- CLI: `--load-frame`, `--frames`, `--diagnostics`, `--orbit-camera`, `--auto-orbit`, `--wire-boxes`
- Orbit camera: yaw/zoom/height keyboard controls (time-based), auto-orbit rotation
- Hot reload: F5 reloads `RenderFrameDump v1` from disk, resets orbit
- Diagnostics: frame stats, bucket breakdown, D3D11 draw coverage, wireframe mode
- Wireframe overlay: `D3D11_FILL_WIREFRAME` second pass on supported Box primitives

### Smoke / verification scripts

- `.\tools\d3d11_shadow_smoke.ps1 -Preset ci -Build`
- `.\tools\renderframe_capture_replay.ps1 -Preset ci -Build`

### Example commands

```powershell
.\build\ci\Debug\bs3d_d3d11_game_shell.exe --frames 0 --load-frame artifacts\shadow_frame.txt --diagnostics --orbit-camera
.\build\ci\Debug\bs3d_d3d11_game_shell.exe --frames 0 --load-frame artifacts\shadow_frame.txt --diagnostics --orbit-camera --wire-boxes
.\tools\renderframe_capture_replay.ps1 -Preset ci -Build
```

## Explicit non-goals / not done

- `--renderer d3d11` is **not active** in GameApp
- GameApp main renderer is still **raylib**
- No mesh rendering (Mesh, Sphere, CylinderX, QuadPanel)
- No material system or shader-file pipeline
- No texture loading
- No HUD / text / UI rendering through D3D11
- No window resize handling
- No production D3D11 main renderer
- No raylib `IRenderer` adapter (GameApp still uses WorldRenderer/HudRenderer/DebugRenderer directly)
- `RenderFrameDump v1` intentionally skips non-Box primitive kinds on write

## Supported D3D11Renderer subset

- Box primitives in buckets: Opaque, Vehicle, Decal, Glass, Translucent, Debug
- Debug lines (up to 2048 line pairs)
- Minimal alpha blending for Glass/Translucent buckets
- Camera-based view/projection (with fallback fixed camera for default frames)
- Wireframe overlay (`D3D11_FILL_WIREFRAME` rasterizer)

## Unsupported D3D11Renderer subset

- Buckets: Sky, Ground, Hud
- Primitive kinds: Mesh, Sphere, CylinderX, QuadPanel
- Coverage diagnostics clearly separate frame-level stats (all primitives) from draw-level stats (Box + debug lines only)

## Recommended next passes

1. **Mesh/material pipeline planning** — design how `RenderPrimitiveKind::Mesh` commands flow through extraction → RenderFrame → D3D11Renderer. See `docs/d3d11-mesh-material-pipeline-plan.md`.
2. **Explicit dev-only main renderer experiment** — a gated `--renderer d3d11` that replaces raylib rendering in GameApp for a single frame or limited smoke session
3. **Do not** silently activate `--renderer d3d11` without explicit gate, verification scripts, and coverage diagnostics

## References

- `DEVELOPMENT.md` — daily dev workflow and tooling
- `docs/backend-modernization-grounding.md` — full backend modernization grounding document
- `docs/d3d11-mesh-material-pipeline-plan.md` — mesh/material pipeline architecture plan (next step)
- `docs/d3d11-renderer-smoke.md` — D3D11Renderer smoke executable details
