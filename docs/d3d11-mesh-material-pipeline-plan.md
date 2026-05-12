# D3D11 mesh/material pipeline plan

Status: LIVE (Stages 1-4 implemented, Stage 5 in progress)
Created: 2026-05-12
Stage 1: DONE — MeshRegistry + MaterialRegistry data-only registries
Stage 2: DONE — D3D11MeshCache integrated into D3D11Renderer
Stage 3a: DONE — CpuMeshData
Stage 3b: DONE — D3D11GameShell procedural mesh
Stage 4: DONE — RenderFrameDump v2
Stage 5a: DONE — addWorldRenderListMeshCommands extraction helper
Stage 5b: DONE — --renderframe-shadow-meshes wired to GameApp shadow path
Next code pass: Stage 5c — D3D11 sidecar mesh upload for emitted Mesh commands

See also:
- `docs/backend-modernization-grounding.md` — truth hierarchy, protected systems, non-goals
- `docs/d3d11-modernization-milestone.md` — implemented D3D11 subset, supported buckets, coverage
- `docs/d3d11-mesh-material-implementation-checklist.md` — stage 1 readiness gates and implementation steps
- `DEVELOPMENT.md` — daily workflow, D3D11 smoke/verification commands

## 1) Current state (Implemented)

### 1.1 D3D11Renderer draw support

`D3D11Renderer` (`src/render_d3d11/D3D11Renderer.h:51`) currently draws:

| Primitive | Status | Detail |
|---|---|---|
| `RenderPrimitiveKind::Box` | Implemented | All supported buckets (Opaque, Vehicle, Decal, Glass, Translucent, Debug). Uses the per-primitive constant-buffer path with size/transform/tint. Draws 36-index unit cube at scaled/rotated/translated world position. |
| Debug lines | Implemented | Up to 2048 line-pair capacity (`DebugLineVertexCapacity`). Dynamic vertex buffer, line-list topology. |
| `RenderPrimitiveKind::Mesh` | Implemented (cached) | `D3D11MeshCache` holds GPU buffers for each `MeshHandle`. `BuiltInUnitCubeMeshId` (id=1) is uploaded during `initialize()`. Other handles must be explicitly uploaded before rendering. Missing handles → `skippedMissingMeshes`. Shares the Box pipeline (VS/PS/constant buffer, different VB/IB/indexCount). |
| Wireframe overlay | Optional | Second pass with `D3D11_FILL_WIREFRAME` rasterizer on Box + cached Mesh primitives. |

Unsupported (skipped with stat tracking):
- `RenderPrimitiveKind::Sphere`, `CylinderX`, `QuadPanel`
- `RenderPrimitiveKind::Mesh` with uncached handles → counted as `skippedMissingMeshes`
- Buckets: `Sky`, `Ground`, `Hud`

### 1.1a MeshRegistry / MaterialRegistry (Stage 1 — Implemented)

- `MeshRegistry` (`src/render/MeshRegistry.h/.cpp`): data-only handle allocator. `BuiltInUnitCubeMeshId` not owned by registry; first real allocation starts at id=2. `count()` / `empty()` inspection helpers.
- `MaterialRegistry` (`src/render/MaterialRegistry.h/.cpp`): data-only handle allocator with permanent `defaultOpaque` (id=1) and `defaultAlpha` (id=2). User allocations start at id=3.
- 23 tests in `bs3d_render_tests`. No D3D11, no GPU, no asset loading.

### 1.1b D3D11MeshCache (Stage 2 — Implemented)

- `D3D11MeshCache` (`src/render_d3d11/D3D11MeshCache.h/.cpp`): D3D11-private GPU buffer cache keyed by `MeshHandle`.
- `D3D11MeshVertex` (positions-only, matching existing `Vertex` struct) + `D3D11MeshUpload` (vertices + uint16_t indices).
- `D3D11CachedMeshView` non-owning read-only lookup via `find()`.
- Integrated as private `meshCache_` member in `D3D11Renderer`.
- Procedural unit cube uploaded into cache during `initialize()`.
- `renderFrame()` resolves Mesh commands through `meshCache_.find()`.
- 11 GPU-free tests in `bs3d_render_tests`. `--add-test-mesh` shell flag verifies `drawnMeshes=1`.
- No asset loading, no GameApp integration.

### 1.2 RenderFrame data model

`RenderFrame` (`include/bs3d/render/RenderFrame.h:83`) already contains the data fields needed for mesh/material commands:

- `RenderPrimitiveCommand` (line 66) carries `MeshHandle mesh` (line 72) and `MaterialHandle material` (line 73) alongside `RenderPrimitiveKind`.
- `MeshHandle` (line 32) is a zero-struct with `uint32_t id`.
- `MaterialHandle` (line 45) is a zero-struct with `uint32_t id`.
- `TextureHandle` (line 49) is a zero-struct with `uint32_t id`.
- `RenderMaterial` (line 53) has `RenderColor tint` and `TextureHandle texture`.

These handles are reserved in the data model. `MeshHandle` is now consumed by `D3D11Renderer` through `D3D11MeshCache` for cached handles. `MaterialHandle` is still not consumed by the renderer (bucket-based fallback only). No extraction path populates mesh/material handles yet (Stage 5 deferred).

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
- `drawnBoxes`, `drawnMeshes` (cached Mesh handles increment this; `--add-test-mesh` shows `drawnMeshes=1`)
- `skippedMissingMeshes` (uncached Mesh handles), `skippedUnsupportedKinds`, `skippedUnsupportedBuckets`
- `boxCoveragePct`, `meshCoveragePct` (mesh coverage reflects only cached uploaded meshes)
- `primitiveCoveragePct`, `lineCoveragePct`

## 2) Architecture proposal

### 2.1 CPU-side mesh registry (backend-neutral) — IMPLEMENTED

`MeshRegistry` lives at `src/render/MeshRegistry.h/.cpp`:

```
class MeshRegistry {
public:
    MeshHandle allocate(const std::string& assetId);
    void release(MeshHandle handle);
    bool isValid(MeshHandle handle) const;
    const std::string* assetId(MeshHandle handle) const;
    MeshHandle find(const std::string& assetId) const;
    std::size_t count() const;
    bool empty() const;
};
```

Implemented behavior:
- **Handle allocation is monotonic** — ids increase by 1, never reused.
- **Handle id 0 is reserved** for invalid (matches `MeshHandle{}` default).
- **id 1** (`BuiltInUnitCubeMeshId`) is NOT owned by the registry — smoke/testing convention only.
- **id 2+** allocated for real assets. First allocation returns id=2.
- Idempotent: duplicate `allocate("foo")` returns same handle.
- `release()` invalidates handle; `count()` excludes released handles.
- `count()` / `empty()`: `count() == 0` and `empty() == true` on fresh registry.
- No I/O, no GPU, no asset loading. Backend-neutral. Linked into `bs3d_render_tests`.

### 2.2 Material handle registry (backend-neutral) — IMPLEMENTED

`MaterialRegistry` lives at `src/render/MaterialRegistry.h/.cpp`:

```
class MaterialRegistry {
public:
    MaterialRegistry();
    MaterialHandle allocate(const std::string& name);
    void release(MaterialHandle handle);
    bool isValid(MaterialHandle handle) const;
    const std::string* name(MaterialHandle handle) const;
    MaterialHandle find(const std::string& name) const;
    MaterialHandle defaultOpaque() const;
    MaterialHandle defaultAlpha() const;
    std::size_t count() const;
    bool empty() const;
};
```

Implemented behavior:
- **Handle 0 = invalid**.
- **`defaultOpaque()` (id=1) and `defaultAlpha()` (id=2)** pre-allocated at construction.
- **Defaults are permanent** — `release(defaultOpaque/defaultAlpha)` is silently ignored.
- User allocations start at id=3. `count()` returns 2 on fresh registry; `empty()` returns `false`.
- Material registry is **data-only** — no shader parameters, no texture slots, no GPU work.
- Real material data (shader references, textures, blend modes) comes in **Stage 6 or later**.

### 2.3 GPU-side D3D11 mesh cache (D3D11-private) — IMPLEMENTED

`D3D11MeshCache` lives at `src/render_d3d11/D3D11MeshCache.h/.cpp`, integrated as a private `meshCache_` member in `D3D11Renderer`:

```
struct D3D11MeshVertex {
    float position[3];
};

struct D3D11MeshUpload {
    std::vector<D3D11MeshVertex> vertices;
    std::vector<std::uint16_t> indices;
};

struct D3D11CachedMeshView {
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;
    std::uint32_t indexCount = 0;
};

class D3D11MeshCache {
public:
    bool upload(ID3D11Device* device, MeshHandle handle, const D3D11MeshUpload& mesh, std::string* error = nullptr);
    bool contains(MeshHandle handle) const;
    bool find(MeshHandle handle, D3D11CachedMeshView& out) const;
    void release(MeshHandle handle);
    void clear();
    std::size_t count() const;
};
```

Implemented behavior:
- `upload()` validates: null device → fail, zero handle → fail, empty vertices/indices → fail.
- Creates `IMMUTABLE` `ID3D11Buffer` for vertex and index data.
- Replaces existing handle entry (releases old buffers first).
- `find()` returns non-owning `D3D11CachedMeshView` (read-only VB/IB/indexCount pointers); returns false and resets view on miss.
- `clear()` / destructor release all GPU buffers. `count()` returns number of cached entries.
- **No textures, no materials, no shaders per mesh** at this stage.
- **D3D11Renderer private** — never exposed via `IRenderer` or public headers.

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
| `MeshRegistry` (allocation/lookup) | Backend-neutral | `src/render/MeshRegistry.h/.cpp` |
| `MaterialRegistry` (allocation/lookup) | Backend-neutral | `src/render/MaterialRegistry.h/.cpp` |
| `D3D11MeshCache` (GPU buffers) | D3D11-private | `src/render_d3d11/D3D11MeshCache.h/.cpp` (private helper) |
| `D3D11CachedMeshView` / `D3D11MeshVertex` / `D3D11MeshUpload` | D3D11-private | `src/render_d3d11/D3D11MeshCache.h` (private structs) |
| `WorldAssetRegistry` / `WorldModelCache` | Game layer (currently raylib-coupled) | `src/game/WorldAssetRegistry.h` (existing — to be gradually decoupled) |
| Mesh vertex/index data format | Backend-neutral | `D3D11MeshVertex` (float[3] positions), std::uint16_t indices

### 2.9 What should NOT go into public include/bs3d headers yet

- `D3D11CachedMeshView` or any `ID3D11*` types
- `D3D11MeshCache` class declaration
- `D3D11MeshVertex` / `D3D11MeshUpload` structs
- `MeshRegistry` — lives in `src/render/`, not in `include/bs3d/`
- `MaterialRegistry` — same reasoning
- Any `using` or `#include` that pulls Windows.h, d3d11.h, or raylib.h into public headers

The public `include/bs3d/render/RenderFrame.h` already has everything needed (`MeshHandle`, `MaterialHandle`, `RenderPrimitiveCommand` fields). No public header changes have been required for Stages 1-2.

## 3) Staged implementation plan

### Stage 1 — Backend-neutral mesh/material handle registry skeleton (data-only, tests) — DONE

Implemented. See sections 2.1 and 2.2 above for actual API. 23 tests in `bs3d_render_tests`.

### Stage 2 — D3D11Renderer private GPU mesh cache + integration — DONE

Implemented. See section 2.3 above for actual API. `D3D11MeshCache` integrates into `D3D11Renderer::renderFrame()`. Procedural unit cube uploaded during `initialize()`. `--add-test-mesh` shell flag verifies `drawnMeshes=1`. 11 GPU-free tests in `bs3d_render_tests`.

### Stage 3 — Backend-neutral CpuMeshData + D3D11MeshUpload adapter — DONE

**Stage 3a — data/adapter (DONE):**
`CpuMeshData` (`src/render/CpuMeshData.h/.cpp`), `D3D11MeshUploadAdapter` (`src/render_d3d11/D3D11MeshUploadAdapter.h/.cpp`), and `D3D11Renderer::initialize()` built-in cube upload refactored to use `makeCpuMeshUnitCube()` → `makeD3D11MeshUpload()` → `meshCache_.upload()`.

**Stage 3b — shell test mesh (DONE):**
`D3D11Renderer::uploadTestMesh()` added as shell/tooling method. `D3D11GameShell --add-test-mesh` now creates a procedural `makeCpuMeshTriangle()`, uploads via `uploadTestMesh(MeshHandle{2})`, and appends a Mesh command. Diagnostics show `drawnMeshes=2` (BuiltInUnitCubeMeshId + CpuMeshData triangle).
- `CpuMeshData` can feed `D3D11MeshCache` through the adapter.
- Shell can render a procedural CPU mesh beyond `BuiltInUnitCubeMeshId`.
- `RenderFrameDump v1` still does not serialize Mesh commands.
- No file I/O, no asset loading, no GameApp integration.

**Next: Stage 4 — RenderFrameDump v2**, or pre-Stage-4 cleanup (see below).

**Goal:** Create a backend-neutral CPU-side mesh data struct that can be converted to `D3D11MeshUpload`, enabling game shell to upload custom procedural meshes beyond `BuiltInUnitCubeMeshId`.

**Scope:**
- Backend-neutral `CpuMeshData` struct (or `CpuMeshVertex` + indices) in `src/render/` or new location.
- Adapter/conversion to `D3D11MeshUpload` (trivial since both use `float position[3]` and `uint16_t` indices).
- `D3D11GameShell` can create a `CpuMeshData` for a simple triangle, convert to `D3D11MeshUpload`, upload to `D3D11MeshCache` via a new `D3D11Renderer` method or direct access.
- New `--mesh-test-triangle` flag or reuse `--mesh-test`.
- Diagnostics show `drawnMeshes=2` (BuiltInUnitCube + custom mesh).
- **No file I/O, no asset loading, no GameApp integration, no RenderFrameDump v2.**

**Verification:**
- `.\build\ci\Debug\bs3d_d3d11_game_shell.exe --frames 3 --load-frame artifacts\shadow_frame.txt --add-test-mesh --mesh-test-triangle`
- `.\tools\renderframe_capture_replay.ps1 -Preset ci -Build` (existing passes still work)

**Runtime behavior:** No change to GameApp. `D3D11GameShell` only.

### Stage 4 — RenderFrameDump v2 serializing Mesh commands — DONE

**Implemented:**
- `RenderFrameDumpVersion` enum (`V1`, `V2`), `RenderFrameDumpWriteOptions` struct in `RenderFrameDump.h`.
- Overloaded `writeRenderFrameDump(frame, path, options, error)`.
- v2 writer: header `"RenderFrameDump v2"`, writes all primitives with `meshId`/`materialId` tokens.
- v2 reader: accepts both v1 and v2 headers; parses `meshId`/`materialId` when present.
- v1 remains default write format; v1 writer unchanged (still skips non-Box primitives).
- GameApp CLI: `--renderframe-shadow-dump-version <v1|v2>`.
- Capture/replay script: `-DumpVersion v1` (default) or `-DumpVersion v2`.
- 5 new dump tests; v1 and v2 capture/replay smoke both pass.
- **No geometry data, no textures, no material definitions, no asset loading.**
- `--renderer d3d11` remains inactive.

**Next: Stage 5 — GameApp shadow extraction Mesh commands.**

### Stage 5 — GameApp shadow Mesh extraction — NEXT

**Goal:** The `--renderframe-shadow` path can optionally emit `RenderPrimitiveKind::Mesh` commands in the shadow `RenderFrame` when a `MeshRegistry` is available.

**This is dev/shadow only.** Main raylib renderer remains unchanged. `--renderer d3d11` remains inactive.

**Gating:**

New explicit dev flag:
- `--renderframe-shadow-meshes` — enables Mesh command emission in the shadow frame.
- Implies `--renderframe-shadow` (not meaningful without it).
- Default remains fallback Box extraction only.
- Invalid/absent MeshRegistry → fallback Box extraction (unchanged behavior).

**Registry behavior (first pass):**

- `MeshRegistry` maps `WorldObject.assetId` → `MeshHandle`.
  - `WorldObject.assetId` matches `WorldAssetDefinition.id`.
  - `MeshRegistry.find(definition.id)` returns `MeshHandle` or `MeshHandle{0}`.
- `MaterialRegistry` uses `defaultOpaque()` / `defaultAlpha()` only (no per-asset materials yet).
- First pass may map a **small subset** of assetIds to procedural/test handles — not every WorldObject needs a Mesh.
- No real asset (OBJ/GLTF) loading in Stage 5. Meshes must be pre-uploaded into `D3D11MeshCache` beforehand.

**Extraction design:**

New function (additive to existing fallback Box path):
```
int addWorldRenderListMeshes(RenderFrameBuilder& builder,
                             const WorldRenderList& renderList,
                             const std::vector<WorldAssetDefinition>& assetDefinitions,
                             const MeshRegistry& meshRegistry,
                             const MaterialRegistry& materialRegistry);
```

Behavior:
- Iterates `WorldRenderList` buckets (Opaque, Vehicle, Decal, Glass, Translucent) in production order.
- For each `WorldObject`, resolves `assetId` via `MeshRegistry.find()`.
- If resolved: emits `RenderPrimitiveCommand` with `kind=Mesh`, `mesh=handle`, `material=materialRegistry.defaultOpaque()` (or `defaultAlpha()` for Glass/Translucent buckets).
- If NOT resolved: **falls back to existing Box extraction** (uses `WorldAssetDefinition.fallbackSize`/`fallbackColor`).
- Returns `emittedMeshCount` for diagnostics.
- `MeshRegistry` is passed by const-ref; if not available (caller passes empty/default registry), emits only fallback Boxes.

**Diagnostics:**

- `WorldRenderExtractionStats` gains `emittedMeshes` counter.
- Shadow diagnostics log: `emittedMeshes`, `meshFallbacks` (objects where mesh was unresolved → Box fallback).
- D3D11 sidecar diagnostics already track `drawnMeshes` and `meshCoveragePct`.
- `RenderFrameDump v2` should be used when capturing shadow frames with Mesh commands (v1 would skip them).

**Explicit non-goals (Stage 5):**

- No real model/OBJ/GLTF loading.
- No texture/material pipeline (MaterialRegistry is data-only).
- No GameApp main D3D11 renderer.
- No `--renderer d3d11` activation.
- No replacement of `WorldRenderer`/`HudRenderer`/`DebugRenderer`.
- No mesh auto-upload from `WorldModelCache` — meshes must be explicitly uploaded before extraction runs.
- No `WorldRenderList` changes — extraction reads existing lists, does not modify them.

**First Stage 5 code pass:**

1. Add `--renderframe-shadow-meshes` CLI flag to `GameRunOptions` and `main.cpp`.
2. In `GameApp`, create `MeshRegistry` and `MaterialRegistry` when the flag is active.
3. Add `addWorldRenderListMeshes()` to `RenderExtraction.h/.cpp`.
4. Wire into the shadow frame build path gated by the flag.
5. Add `emittedMeshes` to `WorldRenderExtractionStats` and shadow diagnostics.
6. Test: `--renderframe-shadow --renderframe-shadow-meshes --renderframe-shadow-dump artifacts\shadow_v2.txt --renderframe-shadow-dump-version v2` with pre-uploaded test handles.

**Verification:**
- `.\tools\d3d11_shadow_smoke.ps1 -Preset ci -Build`
- Shadow diagnostics show `emittedMeshes > 0` and `meshCoveragePct > 0`.
- Existing smoke scripts still pass.
- `--renderer d3d11` still rejected.

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

## 7) Recommended next code pass

**Stage 3 — Backend-neutral `CpuMeshData` + `D3D11MeshUpload` adapter.**

- Create a backend-neutral `CpuMeshVertex` / `CpuMeshData` representation in `src/render/`.
- Provide conversion to `D3D11MeshUpload` (trivial — same position-only layout).
- Extend `D3D11GameShell` with `--mesh-test-triangle` to upload a custom triangle through `D3D11MeshCache`.
- Diagnostics show `drawnMeshes=2`.
- No asset loading. No GameApp integration. No RenderFrameDump v2.
