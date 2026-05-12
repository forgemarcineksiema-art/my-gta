# D3D11 renderer modernization — next direction decision

Status: DECIDED (Option A implemented, now paused via Option E)
Created: 2026-05-12
After: Stages 1–6c complete

See also:
- `docs/d3d11-mesh-material-pipeline-plan.md` — full architecture plan
- `docs/d3d11-mesh-material-implementation-checklist.md` — stage-by-stage implementation status
- `docs/backend-modernization-grounding.md` — truth hierarchy, non-goals
- `DEVELOPMENT.md` — verification commands

## Completed state

Stages 1–6c are fully implemented:

| Stage | What | Status |
|---|---|---|
| 1 | MeshRegistry + MaterialRegistry data-only registries | DONE |
| 2 | D3D11MeshCache integrated into D3D11Renderer | DONE |
| 3a | CpuMeshData + D3D11MeshUploadAdapter | DONE |
| 3b | D3D11GameShell procedural mesh (--add-test-mesh) | DONE |
| 4 | RenderFrameDump v2 (Mesh command serialization) | DONE |
| 5a | addWorldRenderListMeshCommands extraction helper | DONE |
| 5b | --renderframe-shadow-meshes wired to GameApp | DONE |
| 5c | D3D11 sidecar procedural mesh upload | DONE |
| 5d | selectShadowMeshSeedAssetIds deterministic helper | DONE |
| 6a | CpuMeshLoader minimal OBJ parser | DONE |
| 6b | D3D11GameShell --load-mesh <path> | DONE |
| 6c | Dev-only sidecar modelPath upload via assetsRoot | DONE |

**Current boundaries:**
- Raylib remains the only active main runtime renderer
- `--renderer d3d11` returns a clear error — NOT implemented
- D3D11 is dev/shadow/shell tooling only
- Minimal OBJ loading exists through `CpuMeshLoader`
- `modelPath` sidecar mesh upload exists behind `--renderframe-shadow-meshes` + `--d3d11-shadow-window`
- No textures, no materials, no GLTF, no animation
- `WorldModelCache` unchanged — raylib `Model` loading is the production path

## Option A — Broader WorldAssetRegistry integration — IMPLEMENTED

**Scope:**
- Populate `MeshRegistry` for more/all active asset definitions
- Still dev/shadow only — no GameApp main renderer change
- Still no material/texture system

**Implemented (after Stages 6a-6c):**
- `selectShadowMeshSeedAssetIdsFromDefinitions` helper extends the Stage 5d seed selector
- Prefers assetIds with non-empty `modelPath` so real OBJ meshes are prioritized
- Skips `renderInGameplay=false`, skips missing definitions
- Seeding limit raised from 3 to 16 (`ShadowMeshSeedLimit`)
- Seed selection is a one-time shadow snapshot seed, not dynamic runtime asset streaming
- Sample smoke diagnostics: `seedCount=16 loadedMeshFiles=16 drawnMeshes=36` (up from drawnMeshes=5)
- No GLTF support, no textures/materials, no `--renderer d3d11` activation
- Seeding is dev/shadow-only — no impact on raylib runtime or gameplay
- No `WorldModelCache` changes, no GLTF, no material system

## Option B — GLTF support

**Scope:**
- Add GLTF parsing to `CpuMeshLoader`
- Positions-first minimal subset (no animation/materials/textures at first)
- Reuse existing OBJ infrastructure (same `CpuMeshData` output, same upload path)

**Pros:**
- Unlocks real-world asset formats (most modern models are GLTF)
- Natural complement to Option A
- Can be staged: positions-only first, then expand

**Cons:**
- Higher implementation complexity than OBJ
- GLTF spec is large; even "minimal" subset requires careful scoping
- Risk of scope creep into materials/textures/animation
- External dependency risk (GLTF parsing library or custom parser)

## Option C — Material/texture pipeline planning

**Scope:**
- Design `RenderMaterial` / `MaterialHandle` system
- Plan texture loading pipeline (`TextureHandle`, `ID3D11ShaderResourceView`)
- No immediate implementation — planning only

**Pros:**
- True visual fidelity improvement
- Natural destination for any asset pipeline

**Cons:**
- **Highest scope risk** — materials imply textures, shaders, asset format decisions
- Opens door to full rendering pipeline rebuild
- Premature without clear asset pipeline foundation
- Would require decisions on shader authoring, PBR, texture formats

## Option D — RenderFrameDump v3 / geometry embedding

**Scope:**
- Embed mesh geometry in dump files
- Enable standalone repro without asset files
- Useful for renderer debugging

**Pros:**
- Makes dumps self-contained
- Easier debugging/repro for renderer issues
- Could accelerate future renderer work

**Cons:**
- Risk of dump becoming an asset format
- Duplicates geometry data (already in OBJ/GLTF files)
- Large file sizes with embedded geometry
- Premature without clear debugging need

## Option E — Pause renderer work

**Scope:**
- Return to gameplay/world/camera/vehicles/tools
- Maintain existing D3D11 dev tooling (all smokes still pass)
- Resume renderer work later when there is a concrete driving need

**Pros:**
- **Reduces renderer rabbit-hole risk** — avoids over-engineering a shadow pipeline
- Gameplay improvements have more immediate user impact
- Existing D3D11 dev infrastructure is already useful and self-contained
- Can resume later from a well-documented state

**Cons:**
- D3D11 pipeline left in "dev tooling" state indefinitely
- May lose momentum on renderer modernization
- Asset pipeline gap remains

## Recommendation

**Option A (broader integration) or Option E (pause) are the safest next directions.**

- **Option A** makes sense if there is a near-term need to see more real assets in the D3D11 sidecar.
- **Option E** makes sense if the priority should shift back to gameplay features.

**Do not recommend jumping to Options B/C/D yet** — GLTF, materials, textures, and dump v3 are higher-complexity efforts that should be driven by a concrete, well-defined need. All can be planned later from the current documented state.

**Current status:** Option A implemented, D3D11 pipeline parked via Option E. See `docs/runtime-gameplay-systems-checkpoint.md` for next gameplay-focused passes.

## Next prompt suggestions

Based on the chosen option:

**If Option A:**
> "Implement broader MeshRegistry integration: register all active WorldAssetDefinitions in the shadow MeshRegistry, keep existing deterministic seed order, do not change raylib runtime, do not activate --renderer d3d11."

**If Option E:**
> "Pause D3D11 renderer work. Fix/improve [choose gameplay area: vehicle handling / camera / mission flow / world dressing / tools]."
