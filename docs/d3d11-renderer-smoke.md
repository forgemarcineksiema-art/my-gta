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
- **Exercises tiny Box subset**: `D3D11Renderer` can draw Box primitives for the Opaque, Vehicle, and Debug buckets with a private in-memory shader, cube vertex/index buffers, dynamic constant buffer, and depth-stencil state.
- **Exercises tiny debug-line subset**: debug lines use a private in-memory shader, input layout, and dynamic vertex buffer, and are currently drawn after boxes through the same depth-tested path.
- **Exercises shutdown**: calls `D3D11Renderer::shutdown()` before closing the window.

## What it does not do

- **No runtime integration**: it is not wired into `GameApp`, renderer CLI selection, or `RendererFactory`.
- **No full RenderFrame renderer**: it does not draw meshes, spheres, cylinders, quad panels, HUD, or real game render data.
- **No asset path**: it does not load meshes, textures, materials, shader files, or game assets.
- **No material pipeline**: tint is passed directly as a constant color; there is no mesh/texture/material/shader-file pipeline.
- **No raylib dependency**: the target does not link raylib or `bs3d_game_support`.
- **No replacement for the boot spike**: `bs3d_d3d11_boot` remains as the standalone hardcoded cube spike.

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
```

For single-config generators the executable may be directly under `build\ci`.

Expected logs include window creation, `D3D11Renderer` initialization, rendered frame count, and clean shutdown.
