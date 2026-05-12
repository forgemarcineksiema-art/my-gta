# D3D11 mesh/material implementation checklist

Status: LIVE (documentation only)
Created: 2026-05-12
First code pass: not started

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

### 2.1 Scope

| Item | Detail |
|---|---|
| New files | `tests/MeshRegistryTests.cpp`, `tests/MaterialRegistryTests.cpp` |
| New files | `src/render/MeshRegistry.h`, `src/render/MeshRegistry.cpp` (or in `src/game/` — TBD) |
| New files | `src/render/MaterialRegistry.h`, `src/render/MaterialRegistry.cpp` (or in `src/game/` — TBD) |
| CMake | New test target only (no changes to existing targets) |
| Public headers | None (no changes to `include/bs3d/`) |
| D3D11 | None (no `d3d11.h`, `windows.h`, or D3D11-specific code) |
| Runtime behavior | None (GameApp, shell, smoke unchanged) |

### 2.2 MeshRegistry API

```
class MeshRegistry {
    MeshHandle allocate(const std::string& assetId);
    void release(MeshHandle handle);
    bool isValid(MeshHandle handle) const;
    const std::string* assetId(MeshHandle handle) const;
    MeshHandle find(const std::string& assetId) const;
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
- The registry does no I/O, no GPU work, no mesh data storage.

### 2.3 MaterialRegistry API

```
class MaterialRegistry {
    MaterialHandle allocate(const std::string& name);
    void release(MaterialHandle handle);
    bool isValid(MaterialHandle handle) const;
    const std::string* name(MaterialHandle handle) const;
    MaterialHandle find(const std::string& name) const;
    MaterialHandle defaultOpaque() const;
    MaterialHandle defaultAlpha() const;
};
```

Rules:
- `MaterialHandle{0}` is always invalid.
- `defaultOpaque()` and `defaultAlpha()` are pre-allocated at construction (ids 1 and 2).
- User allocations start at id=3.
- Same idempotent-allocation and no-reuse semantics as MeshRegistry.

### 2.4 Test cases

#### MeshRegistryTests.cpp (minimum 6 cases)

| Test | What it verifies |
|---|---|
| `handleZeroIsInvalid` | `MeshHandle{0}` → `isValid` = false, `assetId` = nullptr |
| `allocateReturnsValidHandle` | `allocate("prop_barrel")` → `isValid` = true, `assetId` = "prop_barrel", handle.id >= 2 |
| `duplicateAllocateReturnsSameHandle` | `allocate("a")` twice → same handle, `find("a")` matches |
| `findUnknownReturnsZero` | `find("nonexistent")` → `MeshHandle{0}` |
| `releaseMakesHandleInvalid` | `allocate`, `release`, then `isValid` = false, `assetId` = nullptr |
| `releaseThenReallocateIsNewHandle` | `allocate("a")`, `release`, `allocate("a")` → handle.id != original.id |
| `multipleAllocationsIncreaseId` | Allocate N items → `id` strictly increases |

#### MaterialRegistryTests.cpp (minimum 5 cases)

| Test | What it verifies |
|---|---|
| `defaultsArePreAllocated` | `defaultOpaque()` and `defaultAlpha()` are valid, non-zero, different from each other |
| `handleZeroIsInvalid` | `MaterialHandle{0}` → `isValid` = false |
| `allocateReturnsValidHandle` | `allocate("concrete")` → `isValid` = true, `name` = "concrete", handle.id >= 3 |
| `duplicateAllocateReturnsSameHandle` | Same semantics as MeshRegistry |
| `releaseAndFind` | `allocate`, `release`, `find` returns zero |

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

## 4) Decision points during implementation

These should be resolved during the first code pass (not before):

| Decision | Options | Guidance |
|---|---|---|
| Registry file location | `src/render/` or `src/game/` | Prefer `src/render/` if it compiles without game-layer dependencies. If registries need `WorldAssetRegistry` linkage, use `src/game/`. Keep them in the same directory as each other. |
| Test target | Reuse `bs3d_core_tests` or new `bs3d_render_tests` | Prefer adding to `bs3d_core_tests` if registries link into `bs3d_core`. If they need `bs3d_game_support`, they go in `bs3d_game_support_tests`. |
| Header-only vs compiled | Class size and template use | Both registries are small single-class types. Compiled `.cpp` is fine; inline in header is also acceptable. Match existing project convention. |
| BuiltInUnitCubeMeshId in registry | Should registry know about id=1? | No. `BuiltInUnitCubeMeshId` remains a `RenderFrame.h` constant. The registry starts real handles at id=2 and treats id=1 as "not mine." |

## 5) After stage 1 — what's next

Once Stage 1 is complete and all CI passes:
- Review the implementation against this checklist and the plan doc.
- Update this checklist with lessons learned.
- Proceed to Stage 2 (`D3D11MeshCache` GPU upload).
- Do NOT skip stages or combine Stage 1 + Stage 2 in a single pass.
