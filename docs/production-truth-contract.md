# Production Truth Contract

Status: LIVE
Last verified against code: 2026-05-10
Owner: production rules
Source of truth for: shared world conventions and validation expectations

This document is the shared production contract for Blok 13. If code, data, tools, tests, or documentation disagree with this file, the disagreement is a bug or a migration task. Use code, tests, validators, and runtime behavior to determine the current implemented state before changing gameplay.

## Purpose

The project must not have separate private interpretations of the world for assets, rendering, collision, camera, missions, HUD, save/load, and tools. Every shipped feature should pass through one explicit contract so a world object means the same thing everywhere.

## World Convention

```txt
Y = up
1 world unit = 1 meter
WorldObject.position = bottom-center ground contact point
yawRadians = rotation around +Y
asset origin = bottom_center unless explicitly declared otherwise
asset minY = 0 for bottom_center gameplay assets
visual offset = render-only correction, not gameplay position
collision center = position + authored offset + height / 2 on Y
```

Authoring rules:

- `WorldObject.position` is not the visual mesh center.
- `WorldObject.yawRadians` rotates visuals, authored collision, triggers, and interaction facing around +Y.
- Asset metadata must declare exceptions instead of relying on local code assumptions.
- `visualOffset` may fix presentation only; collision, triggers, zones, save/load, and mission targets still use the object contract.
- Ground, decal, and walkable meshes must make their height rules explicit through metadata and validation.

## Authoritative Data Flow

```txt
asset_manifest.txt
-> WorldAssetRegistry
-> world map / editor overlay instances
-> runtime render, collision, interaction, camera, HUD, save/load
```

C++ owns systems. Data owns content.

- Asset metadata lives in `data/assets/asset_manifest.txt`.
- Map objects, zones, landmarks, route points, interactions, and overlays live under `data/`.
- Mission text, phases, triggers, and object outcomes are data-driven where the current runtime supports them.
- Runtime code may provide fallback behavior, but the final production path should be visible to validators and tools.

## Render Contract

Production render order is:

```txt
1. sky / background
2. ground / base
3. opaque world
4. opaque dynamic actors and vehicles
5. decals
6. glass / transparent
7. particles / smoke
8. 3D markers
9. 2D HUD
```

Every asset must route through one render bucket:

```txt
Opaque
Decal
Glass
Translucent
DebugOnly
Vehicle
```

Rules:

- `DebugOnly` assets must not render in normal gameplay.
- `Decal` assets must not create gameplay collision and must declare a small `maxHeight`.
- `Glass` assets must use the glass/transparent path and alpha fallback color.
- Vehicle render assets are presentation for vehicles, not the full driving collision contract.

## Collision Contract

Visual mesh is not collision mesh.

A complete production object may need separate authored intents:

```txt
visual mesh
player collision
vehicle collision
camera collision
walkable surface
interaction trigger
world event trigger
```

Current manifest collision intent names include:

```txt
Unspecified
None
Ground
Prop
SolidWorld
DynamicProp
VehicleDynamic
CollisionProxy
```

Rules:

- `SolidBuilding` must block the camera, be opaque, and require closed mesh validation unless deliberately migrated.
- `Decal`, `Glass`, and debug-only assets do not imply player or vehicle blockers.
- Camera blockers are explicit through `cameraBlocks`, not guessed from visual appearance.
- Vehicle body, vehicle interaction, vehicle top/ground handling, and camera collision should remain separate concerns as the vehicle pipeline matures.

## Camera Contract

Camera is a production system, not a derived render offset.

Required modes or behaviors:

```txt
on-foot stable camera
sprint camera tuning
vehicle look-ahead / parking-friendly camera
camera boom shortening against blockers
interaction/dialogue-safe framing
StableCameraMode with no shake, no zoom, no event effects
```

Rules:

- Camera feedback must never make movement direction ambiguous.
- Comedy shake/zoom is presentation only and must be capped or disabled in stable mode.
- Stable camera keeps collision/boom safety but disables sprint, vehicle, chase, and event camera tuning.
- Building and shop approaches must help the player read the interaction, not punish them with clipping.
- Camera collision consumes the same blocker truth as world collision authoring.

## Gameplay Feel Contract

The foundation systems are:

```txt
on-foot movement
camera
enter/exit vehicle
gruz driving
vehicle/player collision
prop collision
E/F interactions
```

Rules:

- `WASD` on foot stays camera-relative.
- `A/D` must remain screen-relative and predictable.
- Vehicle comedy must come from presentation and tuning, not broken control.
- Exit placement must not put the player into blocking collision.
- New systems must pass the player foundation quality gate before being considered done.

## Level Authoring Contract

A world object instance should be expressible as data:

```json
{
  "id": "shop_zenona_instance",
  "assetId": "shop_zenona_v3",
  "position": [18, 0, -18],
  "yaw": 0,
  "scale": [1, 1, 1],
  "zone": "Shop",
  "tags": ["landmark", "shop"]
}
```

A production level is more than visual dressing. It should contain or reference:

```txt
objects
zones
triggers
route points
landmarks
spawn points
interactions
visual baselines
```

A hero location such as Zenon's shop must connect model, collision, camera blocker, interaction trigger, location tag, mission trigger, marker/route target, object outcome, and save-relevant state through stable ids. Shop door and rolling-grille outcomes are low-stakes `PublicNuisance` events with cooldowns. Shop price-card and notice outcomes are quiet local satire and must not emit `worldEvent`.

## HUD and Debug Contract

Gameplay HUD and Debug HUD are separate products.

Gameplay HUD may show:

```txt
objective
interaction prompt
Przypal
subtitle/dialogue
vehicle condition
```

Debug HUD may show:

```txt
build stamp
DevTools status
data root
executable path
camera mode
render isolation mode
collision profile
WorldLocationTag
active WorldEvents
Przypal contributors
mission phase
route point
raw input
```

Rules:

- Player-facing HUD must not expose raw enum names such as `WorldEventType`, `CollisionProfile`, or internal route ids.
- Gameplay HUD may expose only the `F1 debug` entry point to dev diagnostics, not QA shortcut lists or asset metadata.
- Developer builds must make build config, DevTools status, data root, and executable identity visible enough to avoid testing the wrong binary.
- Development runs must prefer checked-in scripts that pass `--data-root` to the source `data/` directory instead of relying on copied build-tree data.
- Debug shortcuts must remain gated behind dev tooling where possible.

## Save/Load Contract

Save state must preserve gameplay truth, not presentation accidents.

Save/load should cover:

```txt
mission phase and checkpoint
player position
vehicle position and condition
completed favors / flags
Przypal state
recent world memory
shop / parking / trash service states
important object flags
```

Rules:

- Invalid save data must not silently coerce bad values into valid gameplay state.
- Save/load uses world contract positions and stable ids.
- Debug-only visual state should not become required production save state.

## Validation and Regression Contract

Protect the contract through validators, tests, and manual gates.

Current gates include:

```powershell
python tools\validate_assets.py data\assets
python tools\validate_world_contract.py data --asset-root data\assets
python tools\validate_editor_overlay.py data\world\block13_editor_overlay.json --asset-root data\assets
python tools\validate_object_outcomes.py data\world\object_outcome_catalog.json --runtime-policy src\game\WorldObjectInteraction.cpp
ctest --preset ci
.\tools\ci_verify.ps1 -Preset ci
```

D3D shadow/capture validation is dev-only and stays explicit:

```powershell
.\tools\d3d11_shadow_smoke.ps1 -Preset ci -Build
.\tools\renderframe_capture_replay.ps1 -Preset ci -Build -DumpVersion v2
```

Regression priorities:

- WASD remains camera-relative.
- Mouse right turns camera right.
- Player does not pass through solid world blockers.
- Camera does not enter buildings that declare blockers.
- Vehicle does not clip through parking blockers.
- Exit vehicle placement avoids blocker overlap.
- World events and Przypal contributors do not spam every frame.
- Save/load restores mission and world memory state.
- Asset validator catches holes, bad origins, wrong render buckets, and unsafe metadata.

## Definition of Done

A production asset is not done until it has:

- valid manifest metadata;
- correct origin and minY contract;
- correct render bucket;
- intended collision/camera/interaction behavior;
- validation pass;
- visual baseline or in-game QA view if it is important.

Zenon's shop v3 is the first hero-location gate. `shop_zenona_v3` must stay active, `SolidBuilding`, `bottom_center`, `Opaque`, `collisionProfile=SolidWorld`, `cameraBlocks=true`, `requiresClosedMesh=true`, `allowOpenEdges=false`, tagged `building,shop,hero_asset`, and rendered in gameplay. Legacy `shop_zenona` and `shop_zenona_v2` must remain `DebugOnly`, non-gameplay, and collision-free. Shop object outcomes must keep door, rolling-grille, price-card, and notice hooks in `object_outcome_catalog.json`.

A production mission step is not done until it has:

- clear objective;
- trigger;
- dialogue or feedback;
- route/marker that does not lie;
- checkpoint/fail/retry behavior where relevant;
- save/load coverage for its phase;
- at least one repeatable manual completion pass.

## Scope Rule

Near-term production scope is intentionally narrow:

```txt
one map
one shop
one gruz
one Bogus
one Zenon
one complete mission
one Przypal loop
one proof that the estate remembers
```

When a new idea competes with this contract, prefer the smallest change that strengthens the playable vertical slice.
