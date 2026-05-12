# D3D11 mesh/material implementation checklist

Status: LIVE (Stage 4 complete, Stage 5 planned)
Created: 2026-05-12
Stage 1 status: DONE
Stage 2 status: DONE — D3D11MeshCache + renderer integration.
Stage 3 status: DONE — CpuMeshData + adapter + shell procedural mesh.
Stage 4 status: DONE — RenderFrameDump v2 implemented + CLI/tooling exposed.
Stage 5 status: IN PROGRESS — Stage 5b wired to GameApp shadow path via --renderframe-shadow-meshes.

See also:
- `docs/d3d11-mesh-material-pipeline-plan.md` — full architecture plan
- `docs/backend-modernization-grounding.md` — truth hierarchy, protected systems, non-goals
- `docs/d3d11-modernization-milestone.md` — current implemented subset
- `DEVELOPMENT.md` — daily workflow, smoke commands

## 1) Readiness gates

Before writing any implementation code, verify every gate:

### 1.1 Capture/replay script passes

```powershell
.\tools\renderframe_capture_replay.ps1 -Preset ci -Build
```

Expected: script completes without errors. Both direct and factory smoke paths render successfully.

### 1.2 D3D11 game shell diagnostics pass

```powershell
.\tools\renderframe_capture_replay.ps1 -Preset ci -Build
.\build\ci\Debug\bs3d_d3d11_game_shell.exe --frames 3 --load-frame artifacts\shadow_frame.txt --diagnostics
```

Expected: renders 3 frames, diagnostics show bucket breakdown and D3D11 coverage. No crashes, no error output.

### 1.3 Built-in MeshHandle path works

```powershell
.\build\ci\Debug\bs3d_d3d11_game_shell.exe --frames 3 --load-frame artifacts\shadow_frame.txt --diagnostics --add-test-mesh
```

Expected: `drawnMeshes=1` in diagnostics output. The `BuiltInUnitCubeMeshId` (id=1) path renders.

### 1.4 RenderFrameDump v1 limitations understood

- `RenderFrameDump v1` intentionally skips non-Box primitive kinds on write.
- `Mesh` commands in memory do not survive a dump/load round-trip.
- Test: `dumpSkipsNonBoxPrimitivesOnWrite` and `dumpRoundTripWithBoxAndNonBoxValidatesOnlyBoxSurvives` exist and pass.
- This is expected behavior — not a bug.

### 1.5 --renderer d3d11 remains inactive

- `blokowa_satyra.exe --renderer d3d11` fails with a clear error message.
- No D3D11Renderer created in the GameApp main path.
- Shadow sidecar (`--d3d11-shadow-window`) is the only D3D11 path in GameApp.

### 1.6 CI presets pass

```powershell
cmake --preset ci-core
cmake --build --preset ci-core
ctest --preset ci-core
cmake --preset ci
cmake --build --preset ci
ctest --preset ci
.\tools\ci_smoke.ps1 -Preset ci
```

All gate failures must be resolved before starting implementation. This checklist is not a substitute for failing CI.

## 2) First implementation pass

Stage 1 of `docs/d3d11-mesh-material-pipeline-plan.md`: **MeshRegistry + MaterialRegistry data-only skeleton with unit tests.**

### 2.1 Scope — actual implementation

| Item | Detail |
|---|---|
| Registry files | `src/render/MeshRegistry.h`, `src/render/MeshRegistry.cpp` |
| Registry files | `src/render/MaterialRegistry.h`, `src/render/MaterialRegistry.cpp` |
| Test file | `tests/render_frame_tests.cpp` (tests added inline, no separate test files) |
| CMake | Added `.cpp` sources to `bs3d_render_tests` target (no new library target) |
| Public headers | None changed (`include/bs3d/` untouched) |
| D3D11 | None (no `d3d11.h`, `windows.h`, or D3D11-specific code) |
| Runtime behavior | None (GameApp, shell, smoke unchanged) |

Decision points resolved:
- Registry file location: **`src/render/`** — registries link only to `bs3d_core` (via `RenderFrame.h`), no game-layer dependencies.
- Test target: **`bs3d_render_tests`** — existing render test binary, already includes `src/render/` sources and include paths.
- Implementation style: **compiled `.cpp`** — matching existing pattern (`RenderFrameDump.cpp`, `RendererFactory.cpp`).
- `BuiltInUnitCubeMeshId`: **not owned by `MeshRegistry`** — remains a `RenderFrame.h` constant. Registry starts real handles at id=2.

### 2.2 MeshRegistry API (implemented)

```
class MeshRegistry {
    MeshHandle allocate(const std::string& assetId);
    void release(MeshHandle handle);
    bool isValid(MeshHandle handle) const;
    const std::string* assetId(MeshHandle handle) const;
    MeshHandle find(const std::string& assetId) const;
    std::size_t count() const;
    bool empty() const;
};
```

Rules:
- `MeshHandle{0}` is always invalid.
- `BuiltInUnitCubeMeshId` (id=1) is NOT part of the registry — it remains a smoke/testing convention in `RenderFrame.h`.
- First real allocation returns id=2.
- Allocation is idempotent: calling `allocate("foo")` twice returns the same handle.
- `release()` removes the handle but does not reuse the id.
- `assetId()` returns nullptr for invalid handles.
- `find()` returns `MeshHandle{0}` for unknown assetId.
- `count()` returns the number of registered meshes (does not include `BuiltInUnitCubeMeshId`).
- `empty()` returns `true` only when no meshes are registered.
- The registry does no I/O, no GPU work, no mesh data storage.

### 2.3 MaterialRegistry API (implemented)

```
class MaterialRegistry {
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

Rules:
- `MaterialHandle{0}` is always invalid.
- `defaultOpaque()` and `defaultAlpha()` are pre-allocated at construction (ids 1 and 2).
- User allocations start at id=3.
- Same idempotent-allocation and no-reuse semantics as MeshRegistry.
- **Defaults are permanent:** `release(defaultOpaque())` and `release(defaultAlpha())` are silently ignored. Defaults cannot be removed from the registry.
- `count()` returns 2 on construction (defaults); `empty()` returns `false` (defaults prevent emptiness).

### 2.4 Test cases (implemented — 23 tests total)

All tests live in `tests/render_frame_tests.cpp` (not separate files).

#### MeshRegistry tests (13 cases, all passing)

| Test | What it verifies |
|---|---|
| `handleZeroIsInvalid` | `MeshHandle{0}` → `isValid` = false, `assetId` = nullptr |
| `allocateReturnsValidHandle` | `allocate("prop_barrel")` → `isValid` = true, `assetId` = "prop_barrel", handle.id >= 2 |
| `duplicateAllocateReturnsSameHandle` | `allocate("a")` twice → same handle, `find("a")` matches |
| `findUnknownReturnsZero` | `find("nonexistent")` → `MeshHandle{0}` |
| `releaseMakesHandleInvalid` | `allocate`, `release`, then `isValid` = false, `assetId` = nullptr |
| `releaseThenReallocateIsNewHandle` | `allocate("a")`, `release`, `allocate("a")` → handle.id != original.id |
| `multipleAllocationsIncreaseId` | Allocate N items → `id` strictly increases |
| `builtInUnitCubeIdIsNotOwnedByRegistry` | `BuiltInUnitCubeMeshId` (id=1) → `isValid` = false, not findable |
| `releaseUnknownHandleIsSafe` | Releasing handles 0 and 42 does not crash or change registry |
| `findAfterReleaseReturnsZero` | allocate, release, find → `MeshHandle{0}` |
| `countTracksAllocations` | count/empty track allocate/release correctly |
| `releaseUnknownDoesNotChangeCount` | Releasing 0 or unknown handle does not decrement count |

#### MaterialRegistry tests (10 cases, all passing)

| Test | What it verifies |
|---|---|
| `defaultsArePreAllocated` | `defaultOpaque()` and `defaultAlpha()` are valid, non-zero, different |
| `handleZeroIsInvalid` | `MaterialHandle{0}` → `isValid` = false |
| `allocateReturnsValidHandle` | `allocate("concrete")` → `isValid` = true, `name` = "concrete", handle.id >= 3 |
| `duplicateAllocateReturnsSameHandle` | Same semantics as MeshRegistry |
| `releaseAndFind` | `allocate`, `release`, `find` returns zero |
| `releaseThenReallocateIsNewHandle` | Released id not reused |
| `userAllocationsDoNotCollideWithDefaults` | User handle id != default ids |
| `findDefaultsByName` | `find("default_opaque")` / `find("default_alpha")` match `defaultOpaque()` / `defaultAlpha()` |
| `startsWithDefaultsCount` | `count()` = 2, `!empty()` on fresh registry |
| `defaultsAreNotReleased` | `release(defaultOpaque())` / `release(defaultAlpha())` are ignored; count stays 2, defaults remain valid and findable |

### 2.5 Implementation order

1. Write `MeshRegistryTests.cpp` (tests first, confirming the API shape).
2. Write `MeshRegistry.h` / `MeshRegistry.cpp`.
3. Build and run `MeshRegistryTests` — all pass.
4. Write `MaterialRegistryTests.cpp`.
5. Write `MaterialRegistry.h` / `MaterialRegistry.cpp`.
6. Build and run all new tests — all pass.
7. Run full CI to confirm no regressions.
8. Stage 1 complete.

### 2.6 Post-implementation verification

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

## 3) Explicit "do not do yet"

These are **forbidden** during the first implementation pass. Any of these would violate the contract established in `docs/d3d11-mesh-material-pipeline-plan.md`:

- **No real asset loader.** Do not connect `MeshRegistry` to `WorldAssetRegistry` or `asset_manifest.txt`. Handles are allocated from test strings only.
- **No texture upload.** No `ID3D11ShaderResourceView`, no sampler state, no texture paths.
- **No shader file loading.** All shaders remain in-memory HLSL strings in `D3D11Renderer.cpp`.
- **No GameApp main renderer replacement.** `GameApp` continues using raylib.
- **No `--renderer d3d11` activation.** CLI flag continues to fail with a clear error.
- **No D3D11 GPU mesh buffers.** No `ID3D11Buffer` creation for mesh data. No `D3D11MeshCache`.
- **No material data beyond identity tokens.** `MaterialHandle` is just a uint32 — no shader parameters, blend modes, or texture bindings.
- **No RenderFrame extraction changes.** Shadow path continues emitting fallback Boxes only.
- **No RenderFrameDump v2 yet.** v1 serialization behavior is unchanged.
- **No changes to `include/bs3d/` public headers.** `RenderFrame.h` already has `MeshHandle` and `MaterialHandle` — nothing more is needed.
- **No link to `bs3d_game_support` from mesh registry code** if registries live in `src/render/`.
- **No Windows-only coupling** in registry code.

## 4) Stage 1 lessons learned

These were confirmed by implementation:

- **Registries remain data-only.** Both `MeshRegistry` and `MaterialRegistry` are simple index-by-string containers with no I/O, no GPU work, and no dependency beyond `bs3d_core` headers. They compile into `bs3d_render_tests` cleanly.
- **No D3D11/GPU/asset loading added.** The registry code is fully backend-neutral. No `windows.h`, `d3d11.h`, or raylib includes.
- **Test coverage added in `bs3d_render_tests`.** 23 new test cases run alongside existing render tests. No separate test binary was needed — tests were added inline.
- **The "data-only registry first" strategy works.** Having handle allocation semantics solidly tested before any GPU work eliminates an entire class of bugs from future stages.
- **Default material permanence is the right call.** Silently ignoring `release()` on defaultOpaque/defaultAlpha prevents accidental removal of required fallback materials. Tests enforce this.
- **BuiltInUnitCubeMeshId remains outside MeshRegistry.** The registry does not own id=1 and starts real handles at id=2. This keeps the smoke/testing escape hatch separate from production asset management.

## 5) Stage 2 entry criteria

Before starting Stage 2 (`D3D11MeshCache` GPU upload), verify:

- [ ] Full CI passes: `cmake --preset ci-core && cmake --build --preset ci-core && ctest --preset ci-core && cmake --preset ci && cmake --build --preset ci && ctest --preset ci`
- [ ] Capture/replay passes: `.\tools\renderframe_capture_replay.ps1 -Preset ci -Build`
- [ ] No registry regressions (all 23 tests pass)
- [ ] `--renderer d3d11` still inactive (returns clear error)
- [ ] GameApp raylib runtime unchanged
- [ ] No D3D11/GPU/asset loading introduced
- [ ] `.\tools\d3d11_shadow_smoke.ps1 -Preset ci -Build` passes

## 6) After stage 1 — what's next

Stage 1 and cleanup are complete. Stage 2 is DONE (`D3D11MeshCache` integrated into `D3D11Renderer`):

**Stage 2 status (completed):**
- `D3D11MeshCache` integrated as private `meshCache_` member in `D3D11Renderer`.
- Procedural unit cube uploaded during `initialize()` as `MeshHandle{BuiltInUnitCubeMeshId}`.
- `renderFrame()` routes `RenderPrimitiveKind::Mesh` commands through `meshCache_.find()`, binding cached VB/IB/indexCount.
- Box/wireframe/debug-lines paths unchanged.
- Diagnostics: `drawnMeshes=1` with `--add-test-mesh`.

**Stage 3 status (completed):**
- `CpuMeshData` / `CpuMeshVertex` (`src/render/CpuMeshData.h/.cpp`) — backend-neutral CPU mesh data with `isValidCpuMeshData()`, `makeCpuMeshUnitCube()`, `makeCpuMeshTriangle()`.
- `D3D11MeshUploadAdapter` (`src/render_d3d11/D3D11MeshUploadAdapter.h/.cpp`) — converts `CpuMeshData` → `D3D11MeshUpload`.
- `D3D11Renderer::initialize()` built-in unit cube upload uses `makeCpuMeshUnitCube()` → `makeD3D11MeshUpload()` → `meshCache_.upload()`.
- `D3D11Renderer::uploadTestMesh(MeshHandle, CpuMeshData)` shell/tooling method validates, converts, and uploads.
- `D3D11GameShell --add-test-mesh` now uploads a procedural CpuMeshData triangle (handle id=2) via `uploadTestMesh()`, appends a Mesh command with tint `{0, 200, 100}`, position `{2, 2, 0}`.
- Diagnostics: `drawnMeshes=2` (BuiltInUnitCubeMeshId + CpuMeshData triangle).
- 6 GPU-free tests for CpuMeshData + adapter (all passing).
- `--renderer d3d11` still inactive. No asset loading. No GameApp integration.

**Stage 4 status (completed):**
- `RenderFrameDump v2` implemented (`src/render/RenderFrameDump.h/.cpp`).
- `RenderFrameDumpVersion` enum (`V1`, `V2`), `RenderFrameDumpWriteOptions` struct.
- Overloaded `writeRenderFrameDump(frame, path, options, error)`.
- v1 remains default write format and unchanged — still skips Mesh on write.
- v2 header: `"RenderFrameDump v2"`. Writes all primitives with `meshId`/`materialId` tokens.
- Reader accepts both v1 and v2 headers; unknown versions rejected.
- GameApp CLI: `--renderframe-shadow-dump-version <v1|v2>`.
- Capture/replay script: `-DumpVersion v1` (default) or `-DumpVersion v2`.
- **No geometry data, no textures, no material definitions, no asset loading.**
- 5 new dump tests; v1 and v2 capture/replay smoke both pass.
- `--renderer d3d11` remains inactive.

**Stage 4 next pass:**
- Stage 5 — GameApp shadow extraction may emit Mesh commands (when MeshRegistry is available).

**Stage 5 plan (in progress — Stage 5b):**
- Stage 5a (DONE): `addWorldRenderListMeshCommands` data-only helper in `RenderExtraction.h/.cpp`. Emits Mesh commands from `WorldRenderList` via `MeshRegistry`/`MaterialRegistry`. Falls back to Box when mesh missing. Stats: `emittedMeshes`, `meshFallbacks`, `missingDefinitions`. 12 new data-only tests. Not wired to GameApp.
- Stage 5b (DONE): `--renderframe-shadow-meshes` CLI flag wired to GameApp. Implies `--renderframe-shadow`. Creates `MeshRegistry`/`MaterialRegistry` seeded with first 3 assetIds from WorldRenderList. Diagnostics include `emittedMeshes`, `meshFallbacks`, `missingDefinitions`. v1 dump unchanged (skips Mesh), v2 dump preserves Mesh commands. No D3D11 mesh upload, no asset loading. `--renderer d3d11` still inactive.
- Next (Stage 5c): `--renderframe-shadow-meshes`
- `GameApp` creates `MeshRegistry` + `MaterialRegistry` when flag is active.
- `RenderExtraction::addWorldRenderListMeshes()` resolves `WorldObject.assetId` → `MeshHandle`.
- Emits `RenderPrimitiveKind::Mesh` commands for resolved handles; fallback Box for unresolved.
- `WorldRenderExtractionStats` gains `emittedMeshes` counter.
- Shadow diagnostics log `emittedMeshes` / `meshFallbacks`.
- `RenderFrameDump v2` should be used for Mesh capture.
- No real asset loading. No GameApp main renderer changes. `--renderer d3d11` remains inactive.

Do NOT skip stages. See `docs/d3d11-mesh-material-pipeline-plan.md` Section 3 for the full staged plan.
