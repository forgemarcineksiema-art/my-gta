# D3D11Renderer smoke

Status: standalone Windows-only smoke executable for the production-facing `D3D11Renderer` initialization and tiny `RenderFrame` Box/debug-line drawing path.

## What it does

- **Creates a Win32 window**: opens a small 640x360 window titled `Blok 13 D3D11Renderer Smoke`.
- **Initializes `D3D11Renderer`**: passes the window handle and dimensions through `D3D11RendererConfig`.
- **Clears and presents**: by default builds an empty `RenderFrame`, calls `renderFrame()` for a fixed frame count, clears the render target, presents, and exits cleanly.
- **Uses a private depth path**: creates a depth texture, depth-stencil view, and depth-stencil state for initialized smoke rendering.
- **Optionally draws one Box**: with `--box`, builds a minimal `RenderFrame` containing one opaque `RenderPrimitiveKind::Box` command and submits it through depth-tested `D3D11Renderer` rendering.
- **Optionally draws two Boxes**: with `--two-boxes`, submits two overlapping tinted Box commands for manual depth verification.
- **Optionally draws debug lines**: with `--debug-lines`, submits basic world-space `RenderFrame.debugLines` as one-pixel/default D3D11 `LINELIST` lines.
- **Optionally uses RenderFrame.camera**: with `--camera`, builds a `RenderFrame` with a non-default `RenderCamera` (eye at `(0, 2, -5)`, target at origin, fovy 60°). This exercises the `D3D11Renderer` camera-based view/projection path instead of the fallback fixed camera.
- **Optionally builds frame through RenderFrameBuilder**: with `--builder-frame`, constructs a `RenderFrame` using `RenderFrameBuilder` (include/bs3d/render/RenderFrameBuilder.h) instead of manual primitive vector construction. The builder-constructed frame includes 2 Box primitives in supported buckets (Opaque, Vehicle), 3 debug lines, and valid production bucket order. This exercises the render-contract path: RenderFrameBuilder -> RenderFrame -> D3D11Renderer smoke. If `--camera` is combined with `--builder-frame`, a non-default `RenderCamera` is used.
- **Optionally builds frame through WorldRenderList extraction**: with `--extraction-frame`, constructs local `WorldObject` instances and `WorldAssetDefinition` definitions, builds a `WorldRenderList`, then passes data through `RenderExtraction::addWorldRenderListFallbackBoxes(RenderFrameBuilder&, ...)` into `RenderFrameBuilder`, finally submitting to `D3D11Renderer`. This exercises: WorldRenderList-style data → RenderExtraction → RenderFrameBuilder → RenderFrame → D3D11Renderer smoke. Includes 6 WorldObjects across Opaque, Vehicle, Decal, Glass, missing-definition, and DebugOnly buckets; missing definitions and DebugOnly are skipped/counted. If `--camera` is combined with `--extraction-frame`, a non-default `RenderCamera` (eye at `(0, 4, -8)`, target at `(2, 0, 1)`, fovy 60°) is used.
- **Optionally creates renderer through RendererFactory**: with `--factory`, creates the `D3D11Renderer` through `RendererFactory::createRenderer()` with `backend = D3D11` and `allowExperimentalD3D11Renderer = true`, verifies `result.ok()` and `backendName() == "d3d11"`, then `dynamic_cast`s to `D3D11Renderer*` to call `initialize()`. This exercises: RendererFactory → D3D11Renderer smoke path. Without `--factory`, the renderer is constructed directly on the stack.
- **Exercises tiny Box subset**: `D3D11Renderer` can draw Box primitives for the Opaque, Vehicle, and Debug buckets with a private in-memory shader, cube vertex/index buffers, dynamic constant buffer, and depth-stencil state.
- **Exercises tiny debug-line subset**: debug lines use a private in-memory shader, input layout, and dynamic vertex buffer, and are currently drawn after boxes through the same depth-tested path.
- **Exercises shutdown**: calls `D3D11Renderer::shutdown()` before closing the window.

## Camera behavior

`D3D11Renderer` computes view/projection matrices from `RenderFrame.camera`:

- **When camera is usable** (fovy > 0, forward vector has non-zero length, up vector has non-zero length): a look-at view matrix is built from `camera.position`, `camera.target`, `camera.up`, and a perspective projection uses `camera.fovy` (degrees).
- **Fallback** (camera looks default/invalid — fovy ≤ 0, position equals target, or up is zero): the renderer uses a fixed camera with a Z-offset translation and 65° vertical FOV. This is the default behavior when `--camera` is not used.

Both Box primitives and debug lines use the same view/projection derived from this logic.

This is smoke-level camera support, not a full camera system. It is not wired into `GameApp` or runtime camera integration.

## What it does not do

- **No runtime integration**: it is not wired into `GameApp`, renderer CLI selection, or game runtime.
- **No full RenderFrame renderer**: it does not draw meshes, spheres, cylinders, quad panels, HUD, or real game render data.
- **No asset path**: it does not load meshes, textures, materials, shader files, or game assets.
- **No material pipeline**: tint is passed directly as a constant color; there is no mesh/texture/material/shader-file pipeline.
- **No raylib dependency**: the target does not link raylib or `bs3d_game_support`.
- **No replacement for the boot spike**: `bs3d_d3d11_boot` remains as the standalone hardcoded cube spike.
- **No full camera system**: camera support is minimal look-at/perspective with fallback; no resize handling, no input-driven camera, no runtime `CameraRig` integration.

## Build

```powershell
cmake --preset ci
cmake --build --preset ci --target bs3d_d3d11_renderer_smoke
```

## Run

```powershell
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --box
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --two-boxes
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --debug-lines
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --box --debug-lines
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --two-boxes --debug-lines
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --box --camera
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --two-boxes --camera
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --debug-lines --camera
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --two-boxes --debug-lines --camera
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --builder-frame
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --builder-frame --camera
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --extraction-frame
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --extraction-frame --camera
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --factory
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --factory --extraction-frame
.\build\ci\Debug\bs3d_d3d11_renderer_smoke.exe --frames 3 --factory --extraction-frame --camera
```

For single-config generators the executable may be directly under `build\ci`.

Expected logs include window creation, `D3D11Renderer` initialization, rendered frame count, and clean shutdown.
