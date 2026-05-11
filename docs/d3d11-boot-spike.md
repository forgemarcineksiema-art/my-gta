# D3D11 boot spike

Status: standalone Windows-only boot spike.

## What it does

- **Creates a Win32 window**: opens a 1280x720 window titled `Blok 13 D3D11 Boot Spike`.
- **Initializes Direct3D 11**: creates an `ID3D11Device`, `ID3D11DeviceContext`, `IDXGISwapChain`, and render target view.
- **Clears, transforms, draws, and presents**: clears the back buffer and depth buffer, updates a tiny constant-buffer model/view/projection transform, draws a hardcoded indexed colored cube with depth testing enabled, presents the swapchain, and exits after a fixed frame count.
- **Compiles in-memory HLSL**: compiles a tiny vertex shader and pixel shader with `D3DCompile`, then creates an input layout, immutable vertex buffer, immutable index buffer, and dynamic constant buffer.
- **Handles basic shutdown**: processes Windows messages and exits on close, destroy, or Escape.

## What it does not do

- **No production renderer**: this is not `D3D11Renderer`.
- **Separate from the renderer skeleton**: production-facing `D3D11Renderer` lives under `src/render_d3d11/` and currently only records `RenderFrame` stats/validation without window, GPU, or drawing.
- **No runtime integration**: it is not wired into `GameApp`, `IRenderer`, or `RendererFactory`.
- **No RenderFrame drawing**: it does not consume `RenderFrame` commands.
- **No gameplay changes**: the active game runtime remains raylib.
- **No production shader system**: it does not load shaders from disk and does not have a material pipeline.
- **No mesh loading or textures**: the draw path is still a hardcoded indexed cube only, with no mesh loader, texture loader, material pipeline, or shader files on disk.
- **No third-party dependencies**: it uses Win32 and system Direct3D 11 libraries only.

## Source structure

The spike code is split into small local modules under `src/d3d11_spike/`:

| File | Responsibility |
|---|---|
| `D3D11BootMain.cpp` | `main()`, option parsing, high-level `runBootSpike()` orchestration and render loop |
| `D3D11BootWindow.h/.cpp` | Win32 window creation, window proc |
| `D3D11BootDevice.h/.cpp` | D3D11 device/context/swapchain/RTV creation, depth buffer/depth-stencil state, viewport, COM helpers |
| `D3D11BootPipeline.h/.cpp` | Shader compilation, cube vertex/index buffer, input layout, constant buffer, transform update |

All types remain local to the spike — no D3D11 types are exposed through public engine headers in `include/bs3d/`.

## Build

Configure and build the normal CI preset:

```powershell
cmake --preset ci
cmake --build --preset ci --target bs3d_d3d11_boot
```

The target is only defined on Windows. It links `d3d11`, `dxgi`, and `d3dcompiler`, and does not link raylib or `bs3d_game_support`.

## Run

For a short smoke run:

```powershell
.\build\ci\bs3d_d3d11_boot.exe --frames 3
```

For multi-config generators, the executable may be under a configuration folder:

```powershell
.\build\ci\Debug\bs3d_d3d11_boot.exe --frames 3
.\build\ci\Release\bs3d_d3d11_boot.exe --frames 3
```

The default frame count is 120:

```powershell
.\build\ci\bs3d_d3d11_boot.exe
```

Expected logs include window creation, D3D11 device creation, swapchain creation, depth buffer creation, indexed cube pipeline creation, constant buffer creation, rendered frame count, and clean shutdown.
