# D3D11Renderer smoke

Status: standalone Windows-only smoke executable for the production-facing `D3D11Renderer` initialization path.

## What it does

- **Creates a Win32 window**: opens a small 640x360 window titled `Blok 13 D3D11Renderer Smoke`.
- **Initializes `D3D11Renderer`**: passes the window handle and dimensions through `D3D11RendererConfig`.
- **Clears and presents**: builds an empty `RenderFrame`, calls `renderFrame()` for a fixed frame count, clears the render target, presents, and exits cleanly.
- **Exercises shutdown**: calls `D3D11Renderer::shutdown()` before closing the window.

## What it does not do

- **No runtime integration**: it is not wired into `GameApp`, renderer CLI selection, or `RendererFactory`.
- **No RenderFrame primitive drawing**: it does not draw boxes, meshes, debug lines, HUD, or real game render data.
- **No asset path**: it does not load meshes, textures, materials, shader files, or game assets.
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
```

For single-config generators the executable may be directly under `build\ci`.

Expected logs include window creation, `D3D11Renderer` initialization, rendered frame count, and clean shutdown.
