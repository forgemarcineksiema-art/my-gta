# Art Direction Pivot: Stylized Mid-Poly Crime/Action

Status: LIVE
Created: 2026-05-13
Owner: art direction
Source of truth for: visual quality target, asset authoring guidelines, style reference

## Reason for Pivot

The previous "low-poly" art direction was correct for the prototype phase but is now limiting. The playable intro area works mechanically, but the visual style reads as a blockout or primitive tech demo rather than a stylized open-world crime/action game.

The target shifts from **"simple low-poly prototype"** to **"stylized mid-poly open-world crime/action"** inspired by:
- The readability and clarity of GTA SA / Bully
- The grounded urban feel of Mafia / GTA IV
- The dirty, lived-in Polish block-estate aesthetic from Walaszek's work

This is NOT a move toward photorealism or AAA fidelity. It is a move toward richer silhouettes, modelled detail, and material identity that still runs on modest hardware.

## Target Visual Quality

### What stays the same
- Non-photorealistic, stylized aesthetic
- Chunky readable silhouettes
- Strong color and palette readability
- Modest performance target (1280x720, 60fps on weaker laptops)
- raylib 6.0 renderer (no D3D11 activation)
- Simple collision that stays safe for the player

### What changes

| Before (low-poly blockout) | After (stylized mid-poly) |
|---|---|
| Flat box buildings | Beveled/chamfered edges, recessed windows/doors |
| Unit-box dressing pieces | Authored models with modelled detail |
| Single flat surfaces | Material identity: concrete, plaster, asphalt, metal, glass |
| Clean toy-like blocks | Environmental dirt: stains, cracks, posters, oil marks |
| No shop frontage depth | Real shop front depth: recessed windows, awning overhang, door frames |
| Generic garage row | Garage doors with seams, handles, lamps, wear |
| Flat sidewalks | Curbs with bevel/chipping, worn paint, cracks |
| Hardware-store palette | Dirty plaster tones, muted concrete, rusted metal, scuffed glass |

### Readability target
The area should read as a recognizable block-estate without HUD markers:
- The shop, block entrance, parking, car, and garage row are identifiable from silhouettes alone
- Early PS3 / late PS2 era readability, but cleaner and with better proportions
- No toy-like flat buildings — every visible surface has some modelled breakup

## Hero Asset Quality Targets

### Player / NPC body
- **Current:** Modular low-poly character with named visual profiles
- **Target:** Clearer torso, head, arms, legs with clothing layers; cap brim, brows, cuffs, pockets, trouser stripes, face/ear landmarks (already partially achieved in v2 profiles)
- **Pipeline:** Keep current procedural character generation; evolve profile definitions toward richer silhouettes
- **Tags:** `midpoly_target`, `character`, `hero_asset`

### Shop front (Zenon's)
- **Current:** `shop_zenona_v3.obj` hero mesh + unit-box dressing (sign, windows, door, awning)
- **Target:** Modelled windows with frames, recessed door with handle/doorframe depth, sign with modelled lettering/relief, awning as geometry with cloth folds, rolling grille with modelled slats, security camera as geometry, poster boards with depth
- **Pipeline:** Move toward `.gltf` for shop front hero asset; current `.obj` is adequate for now
- **Tags:** `building`, `shop`, `hero_asset`, `midpoly_target`

### Block facade (Block 13)
- **Current:** `block13_core.obj` hero mesh + unit-box dressing (windows, balconies, entrance)
- **Target:** Recessed windows with sills, modelled balcony slabs with railings, entrance with doorframe/intercom panel depth, roof parapet detail, pipe runs, spoldzielnia notice boards
- **Pipeline:** Keep `.obj` for core block mass; add modelled dressing pieces (balcony railing, entrance frame, pipe geometry)
- **Tags:** `building`, `block`, `hero_asset`, `midpoly_target`

### Garage row
- **Current:** `garage_row_v2.obj` hero mesh + unit-box dressing (doors, number plates)
- **Target:** Doors with visible seams/hinges, modelled handles, wall-mounted lamps, wear/dirt, individual garage identity (different paint fade per door)
- **Pipeline:** Keep `.obj`; consider `.gltf` for a future garage hero mesh
- **Tags:** `garage`, `hero_asset`, `midpoly_target`

### Parking lot and sidewalk
- **Current:** `parking_surface.obj` + unit-box curbs + flat decals for paint
- **Target:** Curbs with bevels and chipped corners, asphalt with crack/scuff decals, worn parking-stop geometry, oil stain decals (already present but thin)
- **Pipeline:** Current `.obj` curbs/lamp-posts/bins are adequate; add more decal variation for surface breakup
- **Tags:** `midpoly_target`, `surface`, `curb`, `parking`

### Gruz car
- **Current:** `gruz_e36.gltf` procedural vehicle with 42 box/cylinder parts, 13 PBR materials
- **Target:** Chunky readable silhouette with modelled wheel arches, recessed headlight/taillight cavities, bumper definition, trim depth, visible door lines
- **Pipeline:** Current procedural `.gltf` is the most complete asset in the project; future passes add more modelled detail to the generation script
- **Tags:** `vehicle`, `hero`, `midpoly_target`

### Urban props
- **Current:** Unit-box bins, signs, poles, boxes, barriers
- **Target:** Bins with lid detail and ribbing, street signs with post/base geometry, lamp posts with modelled head and bracket, barriers with textured concrete surface
- **Pipeline:** Current `.obj` models (`lamp_post_lowpoly.obj`, `curb_lowpoly.obj`, `trash_bin_lowpoly.obj`) are already beyond unit-box; future props should follow their lead with bevels and modelled detail
- **Tags:** `midpoly_target` on all prop assets

## Material and Dirt Identity

### Primary materials (palette/fallback-color based, no textures yet)
| Material | Palette signature | Where used |
|---|---|---|
| Dirty concrete | Warm grey with slight green (132,137,128) | Block facade, walkways |
| Aged plaster | Beige-grey with yellowing (96,158,93 for shop) | Shop walls, block walls |
| Asphalt | Dark neutral grey (43,43,42 road; 68,68,64 parking) | Roads, parking, footpaths |
| Rusted/aged metal | Dark warm grey (70,82,86) | Garage doors, grilles, pipes |
| Dirty glass | Muted blue-grey with alpha (103,145,156,118) | Windows, shop front |
| Worn paint/poster | High-chroma accents (red awning 155,46,42; yellow sign 232,211,69) | Signage, shop identity |
| Bare metal/trim | Cool dark grey (55,58,55) | Rolling grilles, door handles |

### Dirt layers (decals and dressing objects)
- Wall stains: vertical darkening below windows and at building base
- Asphalt cracks: irregular dark patches on road and parking
- Oil stains: dark translucent spots near garages and parking bays
- Worn grass: earth-tone paths tracing pedestrian shortcuts
- Graffiti: colored panels on side walls and garage faces
- Poster/notice wear: torn edges, layering, fading
- Threshold wear: darker patches at entrances from foot traffic

## Pipeline Direction

### Current state (no change required)
- `.obj` for simple/small props: adequate for dressing pieces and blocking geometry
- `.gltf` for hero assets: `gruz_e36.gltf` proves the pipeline works
- All assets declared in `data/assets/asset_manifest.txt` with full metadata
- Python validators (`validate_assets.py`, `validate_world_contract.py`) ensure contract compliance

### Future direction (guidance only, no forced migration)
- Hero identity assets should move toward `.gltf` for richer authored geometry and materials:
  - `shop_zenona_v4.gltf` — shop front with modelled window recesses, door frame, awning geometry
  - `block13_core_v2.gltf` — block with recessed windows, balcony slabs, parapet detail
  - `garage_row_v3.gltf` — garage row with modelled door seams, hinges, lamps
- Simple props keep `.obj` with beveled/chamfered geometry
- Dressing pieces currently using `unit_box.obj` should gain dedicated `.obj` models
- Runtime stays safe and compatible with current validators

## Protected Systems (DO NOT TOUCH)

These gameplay systems are off-limits for the art-direction pivot:
- PlayerMotor
- CameraRig
- VehicleController
- MissionController
- SaveGame
- WorldCollision
- ControlContext

Visual changes must not alter collision behavior. All dressing objects remain collision-free. Core layout objects keep their collision shapes unchanged.

## Implementation Tracker

### Completed in this pass
- [x] Art-direction document created (this file)
- [x] `README.md` updated: low-poly → stylized mid-poly throughout
- [x] `docs/blok-ekipa-world-dna.md` updated: style description evolved
- [x] `docs/walaszek-research-notes.md` updated: 3D style guidance expanded
- [x] `data/map_block_loop.json` style tag updated
- [x] `data/assets/asset_manifest.txt`: `midpoly_target` tags added to all non-legacy assets
- [x] `data/assets/ATTRIBUTION.md`: v0.10 entry added
- [x] `docs/asset-pipeline.md` updated with mid-poly direction
- [x] `progress.md` updated with art-direction reference
- [x] `tools/generate_gruz_e36_gltf.py` docstring updated
- [x] `.obj` asset comments updated: low-poly → stylized mid-poly
- [x] `src/game/IntroLevelCoreLayout.cpp`: midpoly_target tags, art-direction context
- [x] `src/game/IntroLevelDressing.cpp`: art-direction context
- [x] `src/game/IntroLevelIdentityDressing.cpp`: art-direction context
- [x] `src/game/IntroLevelGroundTruthDressing.cpp`: art-direction context

### Future model-authoring work (not in this pass)
- [ ] `shop_zenona_v4` with recessed windows, modelled door frame, awning geometry
- [ ] `block13_core_v2` with recessed windows, balcony slabs, parapet detail
- [ ] `garage_row_v3` with modelled door seams, hinges, lamps
- [ ] Dedicated `.obj` models for currently unit-box-based dressing pieces
- [ ] Gruz car with more modelled detail in procedural generator
- [ ] Character model with clearer torso/arms/legs/clothing layers

## Verification

These gates must pass after any art-direction change:

```powershell
python tools\validate_assets.py data\assets
python tools\validate_world_contract.py data --asset-root data\assets --quiet
python tools\validate_object_outcomes.py data\world\object_outcome_catalog.json --runtime-policy src\game\WorldObjectInteraction.cpp --quiet
cmake --preset ci
cmake --build --preset ci
ctest --preset ci
.\tools\ci_smoke.ps1 -Preset ci
```

## References

- [Blok 13: Rewir DNA](blok-ekipa-world-dna.md) — canon identity
- [Production Truth Contract](production-truth-contract.md) — shared world conventions
- [Asset Pipeline](asset-pipeline.md) — manifest, formats, validation
- [Walaszek Research Notes](walaszek-research-notes.md) — tone reference
- [Early Osiedle Reality Audit](early-osiedle-reality-audit.md) — honest current-state assessment
- [Rockstar Micro Open World Lens](rockstar-micro-open-world-lens.md) — structural companion
- [Grochow Target Vision](superpowers/specs/2026-05-09-grochow-target-vision-design.md) — long-term scale
