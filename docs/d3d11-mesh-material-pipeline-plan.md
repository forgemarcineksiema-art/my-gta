# D3D11 mesh/material pipeline plan

Status: Aspirational (design document)
Created: 2026-05-12
Next code pass: not started

See also:
- `docs/backend-modernization-grounding.md` — truth hierarchy, protected systems, non-goals
- `docs/d3d11-modernization-milestone.md` — implemented D3D11 subset, supported buckets, coverage
- `DEVELOPMENT.md` — daily workflow, D3D11 smoke/verification commands

## 1) Current state (Implemented)

### 1.1 D3D11Renderer draw support

`D3D11Renderer` (`src/render_d3d11/D3D11Renderer.h:51`) currently draws:

| Primitive | Status | Detail |
|---|---|---|
| `RenderPrimitiveKind::Box` | Implemented | All supported buckets (Opaque, Vehicle, Decal, Glass, Translucent, Debug). Uses the per-primitive constant-buffer path with size/transform/tint. Draws 36-index unit cube at scaled/rotated/translated world position. |
| Debug lines | Implemented | Up to 2048 line-pair capacity (`DebugLineVertexCapacity`). Dynamic vertex buffer, line-list topology. |
| `RenderPrimitiveKind::Mesh` | Smoke-only | Only `BuiltInUnitCubeMeshId` (id=1) is recognized. Treated as `Box`-equivalent geometry (shared hardcoded cube VB/IB). No real mesh upload. |
| Wireframe overlay | Optional | Second pass with `D3D11_FILL_WIREFRAME` rasterizer on Box + BuiltInUnitCubeMesh primitives. |

Unsupported (skipped with stat tracking):
- `RenderPrimitiveKind::Sphere`, `CylinderX`, `QuadPanel`
- `RenderPrimitiveKind::Mesh` with any handle other than `BuiltInUnitCubeMeshId` → counted as `skippedMissingMeshes`
- Buckets: `Sky`, `Ground`, `Hud`

### 1.2 RenderFrame data model

`RenderFrame` (`include/bs3d/render/RenderFrame.h:83`) already contains the data fields needed for mesh/material commands:

- `RenderPrimitiveCommand` (line 66) carries `MeshHandle mesh` (line 72) and `MaterialHandle material` (line 73) alongside `RenderPrimitiveKind`.
- `MeshHandle` (line 32) is a zero-struct with `uint32_t id`.
- `MaterialHandle` (line 45) is a zero-struct with `uint32_t id`.
- `TextureHandle` (line 49) is a zero-struct with `uint32_t id`.
- `RenderMaterial` (line 53) has `RenderColor tint` and `TextureHandle texture`.

These handles are reserved in the data model but are **not yet populated by any extraction path** and **not yet consumed by D3D11Renderer beyond `BuiltInUnitCubeMeshId`**.

### 1.3 RenderFrameDump v1

`RenderFrameDump` (`src/render/RenderFrameDump.h:9-11`) intentionally serializes only:
- `RenderCamera`
- `RenderPrimitiveKind::Box` primitives
- Debug lines

Non-Box primitive kinds (Sphere, Mesh, CylinderX, QuadPanel) are **silently skipped on write**. The dump format version is `"RenderFrameDump v1"`. Any Mesh commands added to a RenderFrame in memory will not survive a dump/load round-trip.

### 1.4 GameApp runtime

- GameApp main renderer is **raylib** — no D3D11.
- `--renderer d3d11` remains **inactive** (returns a clear error).
- Shadow extraction (`--renderframe-shadow`) builds `RenderFrame` from live `WorldRenderList` via fallback Box extraction only. No mesh extraction exists.
- Shadow sidecar (`--d3d11-shadow-window`) renders extracted Box + debug lines only.
- `WorldModelCache` (`src/game/WorldAssetRegistry.h:76`) loads raylib `Model` objects directly; no backend-neutral mesh path exists.

### 1.5 Game shell

`D3D11GameShell` (`src/render_d3d11/D3D11GameShell.cpp`) loads `RenderFrameDump v1`, renders through `D3D11Renderer`, and supports `--add-test-mesh` (append `BuiltInUnitCubeMeshId` compound). No real asset loading.

### 1.6 WorldAssetRegistry

`WorldAssetRegistry` (`src/game/WorldAssetRegistry.h:61`) manages asset definitions (`WorldAssetDefinition` → `modelPath`, `fallbackSize`, `fallbackColor`, etc.) from `asset_manifest.txt`. `WorldModelCache` loads raylib `Model` instances directly from OBJ/GLTF files. There is no backend-neutral mesh registry or GPU-upload path.

### 1.7 Coverage diagnostics

Current D3D11 coverage stats track:
- `drawnBoxes`, `drawnMeshes` (only BuiltInUnitCubeMeshId increments this)
- `skippedMissingMeshes`, `skippedUnsupportedKinds`, `skippedUnsupportedBuckets`
- `boxCoveragePct`, `meshCoveragePct` (mesh coverage is always 0 since no real meshes are drawn)
- `primitiveCoveragePct`, `lineCoveragePct`

## 2) Architecture proposal

### 2.1 CPU-side mesh registry (backend-neutral)

A new **`MeshRegistry`** class, backend-neutral, owned by the game layer:

```
// Conceptual location: src/game/MeshRegistry.h or include/bs3d/render/MeshRegistry.h
// TBD during implementation — initial pass is data-only and should not expose
// D3D11/Win32/raylib types in public headers.

class MeshRegistry {
public:
    MeshHandle allocate(const std::string& assetId);
    void release(MeshHandle handle);
    bool isValid(MeshHandle handle) const;

    const std::string* assetId(MeshHandle handle) const;

    // Returns MeshHandle{0} if not found.
    MeshHandle find(const std::string& assetId) const;

private:
    std::vector<std::string> assetIds_;
    // indexById_ maps assetId → handle.id
};
```

Key design decisions:
- **Handle allocation is monotonic** — ids increase by 1, never reused within a session.
- **Handle id 0 is reserved** for "invalid/none" (matches current `MeshHandle{}` default).
- **id 1** is already reserved for `BuiltInUnitCubeMeshId` (smoke/testing only).
- **id 2+** are allocated for real assets.
- The registry does **not** own GPU resources — it is a mapping from handle → assetId + metadata.
- The registry does **not** load mesh data — it only registers that asset "foo" → MeshHandle{3}.

### 2.2 Material handle registry (backend-neutral)

A parallel **`MaterialRegistry`**:

```
class MaterialRegistry {
public:
    MaterialHandle allocate(const std::string& name);
    void release(MaterialHandle handle);
    bool isValid(MaterialHandle handle) const;

    const std::string* name(MaterialHandle handle) const;
    MaterialHandle find(const std::string& name) const;
    MaterialHandle defaultOpaque() const;   // pre-registered handle for fallback
    MaterialHandle defaultAlpha() const;    // pre-registered handle for fallback

private:
    std::vector<std::string> names_;
};
```

Key decisions:
- **Handle 0 = invalid**.
- **Pre-register fallback materials** (e.g., id=1 "default_opaque", id=2 "default_alpha") at registry construction.
- Material registry is **data-only** at this stage — no shader parameters, no texture slots.
- Real material data (shader references, texture bindings, blend modes) comes in **Stage 6 or later**.

### 2.3 GPU-side D3D11 mesh cache (D3D11-private)

Inside `D3D11Renderer` (or a private helper):

```
// D3D11-private, not in public headers.
struct D3D11CachedMesh {
    MeshHandle handle;
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;
    UINT indexCount = 0;
    DXGI_FORMAT indexFormat = DXGI_FORMAT_UNKNOWN;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};

class D3D11MeshCache {
public:
    bool uploadMesh(ID3D11Device* device, MeshHandle handle,
                    const float* vertexData, int vertexCount, int vertexStride,
                    const std::uint16_t* indexData, int indexCount);

    void releaseMesh(MeshHandle handle);
    void releaseAll();

    const D3D11CachedMesh* find(MeshHandle handle) const;

private:
    std::unordered_map<uint32_t, D3D11CachedMesh> cache_;
};
```

Key decisions:
- **All mesh data arrives as raw float/index arrays** — no file I/O here.
- The cache does **not** own the `MeshRegistry` — it receives handles and data.
- **No textures, no materials, no shaders per mesh** at this stage.
- **Static (`IMMUTABLE`) buffers** for initial pass; dynamic upload can come later.
- The cache is **D3D11Renderer private** — never exposed via `IRenderer` or `RenderFrame`.

### 2.4 MeshHandle allocation/lifetime

```
Timeline:
  Game init → WorldAssetRegistry loads asset_manifest.txt
           → MeshRegistry.allocate(asset.id) for each registered asset
           → handles stored in an index (assetId → MeshHandle)

  Mesh loading → asset loader reads OBJ/GLTF data
              → extracts vertex positions (+ optionally normals)
              → calls D3D11MeshCache.uploadMesh(device, handle, ...)

  Shutdown → D3D11MeshCache.releaseAll()
           → MeshRegistry cleared
```

Handles are **owned by the game layer**. The renderer receives handles in `RenderFrame` primitives and resolves them against its cache. If a mesh is not yet uploaded, the renderer skips the draw call and increments `skippedMissingMeshes` (already tracked).

### 2.5 MaterialHandle allocation/lifetime

```
Timeline:
  Game init → MaterialRegistry created
           → defaultOpaque(), defaultAlpha() pre-allocated

  (Stage 6+) → per-asset materials allocated from asset manifest

  Shutdown → MaterialRegistry cleared
```

Material handles in `RenderPrimitiveCommand.material` default to `MaterialHandle{0}` (invalid). The D3D11 renderer uses fallback material (opaque/alpha based on bucket) when the material handle is invalid or unresolved.

### 2.6 Fallback behavior

**Missing mesh:**
- `RenderPrimitiveCommand.kind == Mesh` with unresolved `MeshHandle` → skip draw, increment `skippedMissingMeshes`.
- `MeshHandle{0}` → always treated as missing.

**Missing material:**
- `MaterialHandle{0}` or unresolvable handle → use bucket-based fallback: opaque blend for Opaque/Vehicle/Debug, alpha blend for Glass/Translucent.

**No geometry data uploaded:**
- Mesh is registered but no GPU buffer exists → same as missing mesh (skip draw).

**Built-in fallback mesh:**
- `BuiltInUnitCubeMeshId` (id=1) remains as a smoke/testing escape hatch.
- The D3D11Renderer's existing hardcoded cube VB/IB satisfies this handle.
- Real meshes (id >= 2) require explicit GPU upload before rendering.

### 2.7 WorldAssetDefinition → MeshHandle mapping

```
WorldAssetRegistry.definitions() → iterate
  for each WorldAssetDefinition with usage="active":
    MeshHandle handle = meshRegistry.allocate(definition.id)
    // Store mapping: definition.id → handle
    // Later: load mesh data from definition.modelPath, upload via D3D11MeshCache

RenderExtraction (shadow path, Stage 5):
  WorldObject.assetId → meshRegistry.find(assetId) → MeshHandle
  → RenderPrimitiveCommand.kind = Mesh, mesh = handle
```

The `assetId` field on `WorldObject` is the natural bridge:
- `WorldObject.assetId` matches `WorldAssetDefinition.id`.
- `WorldAssetDefinition.id` maps to `MeshHandle` via `MeshRegistry.find()`.
- The extraction path populates `RenderPrimitiveCommand.mesh` with the resolved handle.

At Stage 3 (game shell), mesh handles come from code, not from live WorldObjects.

### 2.8 Backend-neutral vs D3D11-private

| Component | Visibility | Location |
|---|---|---|
| `MeshHandle` / `MaterialHandle` / `RenderPrimitiveCommand.mesh` / `.material` | Backend-neutral, public | `include/bs3d/render/RenderFrame.h` (already exists) |
| `MeshRegistry` (allocation/lookup) | Backend-neutral, game layer | `src/game/` or new `src/render/` |
| `MaterialRegistry` (allocation/lookup) | Backend-neutral, game layer | `src/game/` or new `src/render/` |
| `D3D11MeshCache` (GPU buffers) | D3D11-private | `src/render_d3d11/` (private helper, not exposed) |
| `D3D11CachedMesh` (buffer handles) | D3D11-private | `src/render_d3d11/` (private struct) |
| `WorldAssetRegistry` / `WorldModelCache` | Game layer (currently raylib-coupled) | `src/game/WorldAssetRegistry.h` (existing — to be gradually decoupled) |
| Mesh vertex/index data format | Backend-neutral | Raw float arrays (positions), std::uint16_t indices |

### 2.9 What should NOT go into public include/bs3d headers yet

- `D3D11CachedMesh` or any `ID3D11*` types
- `D3D11MeshCache` class declaration
- `MeshRegistry` — keep in `src/game/` or `src/render/` until the API stabilizes across both D3D11 and raylib consumers
- `MaterialRegistry` — same reasoning
- Any `using` or `#include` that pulls Windows.h, d3d11.h, or raylib.h into public headers

The public `include/bs3d/render/RenderFrame.h` already has everything needed (`MeshHandle`, `MaterialHandle`, `RenderPrimitiveCommand` fields). No public header changes are required for stages 1-5.

## 3) Staged implementation plan

### Stage 1 — Backend-neutral mesh/material handle registry skeleton (data-only, tests)

**Goal:** `MeshRegistry` and `MaterialRegistry` classes exist as testable, data-only types with unit tests.

**Scope:**
- New files (exact location TBD during implementation):
  - `src/render/MeshRegistry.h` / `.cpp` or `src/game/MeshRegistry.h` / `.cpp`
  - `src/render/MaterialRegistry.h` / `.cpp` or `src/game/MaterialRegistry.h` / `.cpp`
- Both classes are header-only or compiled into `bs3d_core`/`bs3d_game_support` (TBD).
- **No GPU code, no D3D11, no Windows.h.**
- Unit tests in `tests/`:
  - Allocate → valid handle (id >= 2 for MeshRegistry, id >= 3 for MaterialRegistry after seeded defaults).
  - Handle 0 → invalid.
  - `find(assetId)` → correct handle.
  - `assetId(handle)` → correct string.
  - `release()` → handle becomes invalid.
  - Duplicate allocation → returns existing handle (idempotent).
  - `defaultOpaque()` / `defaultAlpha()` → non-zero, valid.

**Verification:**
- `cmake --preset ci-core && cmake --build --preset ci-core && ctest --preset ci-core`

**Runtime behavior:** No change. No D3D11, no GameApp modification.

### Stage 2 — D3D11Renderer private GPU mesh cache for one uploaded mesh

**Goal:** `D3D11MeshCache` exists and D3D11Renderer can draw one explicitly uploaded `MeshHandle`.

**Scope:**
- New file: `src/render_d3d11/D3D11MeshCache.h` / `.cpp` (D3D11-private).
- `D3D11MeshCache::uploadMesh()` creates `ID3D11Buffer` (vertex + index, `IMMUTABLE`).
- `D3D11MeshCache::find()` returns `const D3D11CachedMesh*` or nullptr.
- `D3D11Renderer::renderFrame()` changes:
  - When `command.kind == Mesh` and `isBuiltInUnitCubeMesh(command.mesh)` → existing path (unchanged).
  - When `command.kind == Mesh` and mesh is in `D3D11MeshCache` → draw cached mesh using its own VB/IB/indexCount.
  - It shares the same pipeline (vertex shader, pixel shader, constant buffer, blend state) — only the geometry binding changes.
  - `D3D11Renderer::shutdown()` releases the mesh cache.
- `bs3d_d3d11_renderer_smoke` or new test:
  - Upload a small triangle mesh to D3D11MeshCache.
  - Build a RenderFrame with a Mesh command referencing that handle.
  - Verify it draws (counts increment).

**Verification:**
- `.\tools\renderframe_capture_replay.ps1 -Preset ci -Build` (existing smoke still passes)
- New D3D11 smoke test with custom mesh.

**Runtime behavior:** No change to GameApp. `bs3d_d3d11_renderer_smoke` gains new optional `--mesh-test` flag.

### Stage 3 — D3D11 game shell can load a small test mesh from code

**Goal:** `D3D11GameShell` creates a `MeshRegistry`, allocates a test mesh handle, loads simple vertex data, uploads to D3D11MeshCache, and renders it alongside existing Box primitives.

**Scope:**
- `D3D11GameShell` optionally creates `MeshRegistry` (if `--add-test-mesh` or new flag `--mesh-test <vertex-count>`).
- Mesh data is **hardcoded or procedurally generated** (e.g., a simple triangle or tetrahedron) — no file I/O.
- Shell calls `D3D11MeshCache::uploadMesh()` through a new method on `D3D11Renderer` or by owning its own `D3D11MeshCache`.
- The existing `--add-test-mesh` flag continues to work with `BuiltInUnitCubeMeshId`.
- New `--mesh-test` flag creates a registered MeshHandle with real geometry.

**Verification:**
- `.\build\ci\Debug\bs3d_d3d11_game_shell.exe --frames 3 --load-frame artifacts\shadow_frame.txt --add-test-mesh --mesh-test`
- Diagnostics show `drawnMeshes=2` (BuiltInUnitCubeMesh + custom mesh).

**Runtime behavior:** No change to GameApp.

### Stage 4 — RenderFrameDump v2 may serialize Mesh commands

**Goal:** `RenderFrameDump` format gains support for serializing `RenderPrimitiveKind::Mesh` commands.

**Scope:**
- New dump version: `"RenderFrameDump v2"`.
- v2 serializes all primitive kinds (Box, Mesh, Sphere, CylinderX, QuadPanel), including `MeshHandle.id` and `MaterialHandle.id`.
- v1 remains the default write format until explicitly opted-in. Both v1 and v2 can be read.
- `RenderFrameDump` gains a `setVersion(int)` or `WriteOptions` parameter.
- v2 format decisions:
  - Mesh command line: `Mesh <bucket> <pos> <scale> <yaw> <size> <tint> <meshId> <materialId> <sourceId>`
  - Sphere command: similar but with radius instead of size.
  - CylinderX/QuadPanel: TBD during implementation (may only serialize Mesh initially).
- Tests updated: round-trip for Mesh commands, backward compatibility with v1.

**Verification:**
- `.\tools\renderframe_capture_replay.ps1 -Preset ci -Build` (v1 still works)
- New dump round-trip test with Mesh commands.
- Game shell loads v2 dumps with Mesh commands.

**Runtime behavior:** Existing v1 consumers unaffected. v2 is opt-in for capture.

### Stage 5 — GameApp shadow extraction may emit Mesh commands (only when safe)

**Goal:** The `--renderframe-shadow` path can populate `RenderPrimitiveKind::Mesh` commands in the shadow RenderFrame when `MeshRegistry` is available.

**Scope:**
- `GameApp` optionally creates a `MeshRegistry` when `--renderframe-shadow` is active (gated, dev-only).
- `RenderExtraction` gains a new function or parameter:
  - `addWorldRenderListMeshes(RenderFrameBuilder&, const WorldRenderList&, const MeshRegistry&)`
  - For each `WorldObject` in the render list, resolve `assetId` → `MeshHandle` via `MeshRegistry`.
  - Emit `RenderPrimitiveCommand` with `kind=Mesh`, `mesh=handle`, `material=defaultOpaque()`.
  - **Only emits Mesh commands when `MeshRegistry` is non-null** — fallback Box path remains as-is when registry is absent.
- `--d3d11-shadow-window` D3D11Renderer gains access to the mesh cache so Mesh commands can be drawn.
- **No change to default gameplay render path** — shadow extraction is dev-only.

**Gate:**
- The existing dev flags `--renderframe-shadow`, `--d3d11-shadow-window`, `--d3d11-shadow-diagnostics` remain the only way to exercise this path.
- `--renderer d3d11` remains inactive.

**Verification:**
- `.\tools\d3d11_shadow_smoke.ps1 -Preset ci -Build`
- Shadow diagnostics show `meshCoveragePct > 0` for the first time.
- Existing smoke scripts still pass.

**Runtime behavior:** Main game rendering unchanged. Shadow sidecar renders real mesh geometry (if D3D11MeshCache has uploaded data alongside WorldModelCache).

### Stage 6 — Only later: actual asset loading / materials / textures

**Deferred.** This is the point where:
- Mesh data is extracted from loaded models (OBJ/GLTF) into backend-neutral arrays.
- `WorldModelCache` is replaced or complemented by a backend-neutral mesh loader.
- Texture loading pipeline exists.
- Material system with shader references exists.
- Per-asset `MaterialHandle` allocation from manifest metadata.

No design commitments yet for Stage 6.

## 4) Explicit non-goals

These are explicitly out of scope for Stages 1-5 and must not be implemented yet:

- **No texture loading.** No `ID3D11ShaderResourceView`, no `ID3D11SamplerState`, no texture paths in mesh data.
- **No shader-file pipeline.** All shaders remain in-memory compiled strings in `D3D11Renderer.cpp`. No `.hlsl` or `.fx` file loading.
- **No real material system.** Material handles are identity tokens only. Fallback bucket-based opaque/alpha behavior is sufficient.
- **No GameApp D3D11 main renderer.** `GameApp` continues to use raylib for main rendering. D3D11 remains shadow-sidecar and standalone-shell only.
- **No `--renderer d3d11` activation.** CLI flag continues to fail with clear error.
- **No normals or texture coordinates in D3D11 mesh data.** Vertex data is positions-only (`float[3]`), matching the existing `Vertex` struct in `D3D11Renderer.cpp:48`.
- **No mesh LOD, instancing, or draw-call batching.**
- **No skeletal animation or skinning.**
- **No dynamic mesh updates** (all buffers are `IMMUTABLE` initially).
- **No `IRenderer` mesh API.** Mesh upload remains D3D11Renderer-private. No virtual methods on `IRenderer` for mesh management.
- **No `WorldModelCache` replacement.** Raylib `Model` loading continues for main gameplay. The new pipeline is parallel and dev-only.

## 5) Integration constraints

- **All Stage 1-5 code paths are gated** by existing dev flags (`--renderframe-shadow`, `--d3d11-shadow-window`, `--d3d11-shadow-diagnostics`).
- **Protected systems** (`PlayerMotor`, `CameraRig`, `VehicleController`, `MissionController`, etc.) are never touched.
- **CI presets** (`ci-core`, `ci`) must continue to pass at every stage.
- **No new Windows-only coupling in public headers.** D3D11-private types stay in `src/render_d3d11/`.
- **The `BuiltInUnitCubeMeshId` convention** (id=1) is preserved as a smoke/testing escape hatch. Real mesh handles start at id=2.
- **Coverage diagnostics** (`meshCoveragePct`, `drawnMeshes`) already exist and will naturally reflect progress.

## 6) Verification commands (must continue to work)

```powershell
cmake --preset ci-core
cmake --build --preset ci-core
ctest --preset ci-core
cmake --preset ci
cmake --build --preset ci
ctest --preset ci
.\tools\ci_smoke.ps1 -Preset ci
.\tools\d3d11_shadow_smoke.ps1 -Preset ci -Build
.\tools\renderframe_capture_replay.ps1 -Preset ci -Build
```

## 7) Recommended first code pass after doc

**Stage 1 — `MeshRegistry` and `MaterialRegistry` data-only skeleton with unit tests.**

This is the smallest, safest, most reversible step:
- Two new classes with `allocate`/`release`/`find`/`isValid`.
- No header changes to `include/bs3d/`.
- No CMake changes to existing targets (new test target only).
- No runtime behavior change.
- Tests validate handle allocation semantics.

Concrete suggested deliverable:
- `tests/MeshRegistryTests.cpp` — 6-8 test cases for handle lifecycle.
- `tests/MaterialRegistryTests.cpp` — 4-5 test cases for defaults + allocation.
- Registry implementation files in `src/render/` or `src/game/` (TBD based on link dependencies).
