# D3D11 boot spike

Status: standalone Windows-only boot spike.

## What it does

- **Creates a Win32 window**: opens a 1280x720 window titled `Blok 13 D3D11 Boot Spike`.
- **Initializes Direct3D 11**: creates an `ID3D11Device`, `ID3D11DeviceContext`, `IDXGISwapChain`, and render target view.
- **Clears, draws, and presents**: clears the back buffer, draws a hardcoded colored triangle, presents the swapchain, and exits after a fixed frame count.
- **Compiles in-memory HLSL**: compiles a tiny vertex shader and pixel shader with `D3DCompile`, then creates an input layout and immutable vertex buffer.
- **Handles basic shutdown**: processes Windows messages and exits on close, destroy, or Escape.

## What it does not do

- **No production renderer**: this is not `D3D11Renderer`.
- **No runtime integration**: it is not wired into `GameApp`, `IRenderer`, or `RendererFactory`.
- **No RenderFrame drawing**: it does not consume `RenderFrame` commands.
- **No gameplay changes**: the active game runtime remains raylib.
- **No production shader system**: it does not load shaders from disk and does not have a material pipeline.
- **No third-party dependencies**: it uses Win32 and system Direct3D 11 libraries only.

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

Expected logs include window creation, D3D11 device creation, swapchain creation, triangle pipeline creation, rendered frame count, and clean shutdown.
