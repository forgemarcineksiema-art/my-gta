# Runtime Editor Design

## Summary

The next tool milestone for `Blok 13: Rewir` is an in-game runtime map and object editor built in C++ with Dear ImGui. Its first job is to make Grochow-scale authoring practical without forcing a full migration away from the current C++ intro-level authoring.

The editor should start by editing existing map objects and adding new object instances from `data/assets/asset_manifest.txt`. It should save a small overlay file beside the current data catalog:

```txt
data/world/block13_editor_overlay.json
```

The base map still comes from C++ authoring. The overlay applies on top of that base map.

## Goal Loop Decision

The active development loop should now treat tooling as a first-class gameplay enabler.

Current priority:

```txt
Build the runtime editor foundation before expanding Grochow much further.
```

Reason:

- Main Artery work already showed that map expansion needs rapid placement, route, collision, and dressing iteration.
- `IntroLevelData` still contains many objects authored directly in C++.
- The long-term Grochow target needs dense authored rewirs, not hand-edited C++ coordinates forever.
- Runtime editing keeps visual placement inside the actual game camera, collision, renderer, and debug context.

This does not replace the long-term target. It supports it: compressed Grochow, Blok 13 as the heart, Przypal, World Memory, gruz driving, readable camera, and local social consequences all need better authoring loops.

## Tooling Stack Decision

Use three layers, in this order:

1. **C++ runtime editor with Dear ImGui**
   - Runs inside the game.
   - Edits map object instances in the real scene.
   - Saves only overlay JSON.
   - Available only in dev builds.

2. **Python pipeline and validators**
   - Validates overlay JSON, asset manifests, transforms, object ids, route safety, and asset hygiene.
   - Remains scriptable for CI and fast local checks.
   - Later expands toward Blender export/add-on workflows.

3. **C# external data editor later**
   - Not part of the first implementation.
   - Future target for missions, dialogue, cutscenes, NPC reactions, shops, items, asset metadata, and localization.
   - Should consume the same data formats and validators created by the runtime editor/pipeline work.

## Non-Goals For V1

- No full map migration from C++ to JSON.
- No mission, dialogue, cutscene, item, shop, or localization editor.
- No terrain sculpting.
- No complex prefab graph.
- No runtime navmesh or traffic authoring.
- No shipping-build editor UI.
- No direct editing of C++ source files.

## Runtime Scope V1

The editor must support:

- toggle editor mode in dev builds, recommended key: `F10`;
- list existing objects from the composed intro level;
- filter objects by id, asset id, zone tag, and gameplay tags;
- select an object from the list;
- select an object from the world if a simple ray/proximity picker is practical in the first pass;
- edit `position`, `yawRadians`, and `scale`;
- edit `assetId` using assets loaded from `data/assets/asset_manifest.txt`;
- edit `zoneTag`;
- edit `gameplayTags` as a comma-separated or token-list field;
- add a new overlay instance from a selected manifest asset;
- duplicate an object into a new overlay instance;
- delete overlay-created instances;
- reset an override for a base C++ object;
- save and load `data/world/block13_editor_overlay.json`;
- show dirty state and save/load errors in the ImGui UI.

## Data Model

The overlay file should distinguish changes to existing C++ objects from new data-authored objects.

Suggested shape:

```json
{
  "schemaVersion": 1,
  "overrides": [
    {
      "id": "sign_no_parking",
      "position": [-3.65, 0.0, 5.7],
      "yawRadians": 0.0,
      "scale": [0.58, 1.75, 0.08],
      "assetId": "street_sign",
      "zoneTag": "Parking",
      "gameplayTags": ["vertical_readability", "drive_readability"]
    }
  ],
  "instances": [
    {
      "id": "editor_main_artery_lamp_0",
      "assetId": "lamp_post_lowpoly",
      "position": [18.0, 0.0, -27.0],
      "yawRadians": 0.0,
      "scale": [0.18, 3.1, 0.18],
      "zoneTag": "RoadLoop",
      "gameplayTags": ["vertical_readability", "future_expansion"]
    }
  ]
}
```

Rules:

- `overrides[].id` must match an existing base object.
- `instances[].id` must not collide with any base object or other overlay instance.
- Empty optional fields are allowed only if the loader has a clear default.
- V1 should avoid delete records for base objects. Hiding base objects can be considered later with an explicit `disabledBaseObjects` array.

## Load And Apply Flow

Startup flow:

```txt
IntroLevelBuilder::build()
-> WorldDataCatalog applies existing map/mission data
-> EditorOverlayLoader loads data/world/block13_editor_overlay.json if present
-> overrides patch matching WorldObject records by id
-> instances append new WorldObject records
-> normalize collision authoring
-> scene/collision/interaction systems populate from composed level
```

The overlay apply step should live near the existing data catalog path, not inside rendering code.

The editor should edit the in-memory composed `IntroLevelData`. Saving writes only the overlay representation.

## Module Boundaries

Recommended C++ modules:

- `EditorOverlayData`: POD structs for overlay overrides and instances.
- `EditorOverlayCodec`: load/save JSON and schema-version handling.
- `EditorOverlayApply`: apply overlay to `IntroLevelData`, build overlay from edited state.
- `RuntimeMapEditor`: editor state, selection, dirty flag, object commands.
- `RuntimeMapEditorImGui`: Dear ImGui rendering and user input only.

Dear ImGui should not own gameplay data. It should call editor commands.

## Dear ImGui Integration

Build behavior:

- Include Dear ImGui only when `BS3D_ENABLE_DEV_TOOLS=ON`.
- Normal game builds should not create editor windows or require ImGui symbols.
- Prefer a small, pinned dependency path through CMake. Use a raylib ImGui backend such as rlImGui if it fits the build cleanly; otherwise keep the backend wrapper isolated.

Input behavior:

- When editor mode is closed, the game controls behave normally.
- When editor mode is open and ImGui wants mouse/keyboard, camera capture and gameplay input should pause or ignore those inputs.
- The editor should expose a clear mouse-capture state so playtesting can continue without restarting the game.

UI layout V1:

- left panel: object list and filters;
- right panel: inspector for selected object;
- bottom/status area: save/load state, validation warnings, selected object id;
- asset picker modal or panel for adding instances.

## Python Validation

Add a validator for `data/world/block13_editor_overlay.json`.

It should check:

- JSON parses successfully;
- `schemaVersion` is supported;
- ids are non-empty and unique where required;
- override ids exist in the base map export or a generated object-id list;
- instance ids do not collide with base ids;
- `assetId` values exist in `data/assets/asset_manifest.txt`;
- numeric values are finite;
- scale values are positive;
- `zoneTag` values are known;
- gameplay tags are strings;
- base object deletion is not attempted in V1.

This validator should be callable by CTest after the first implementation is in place.

## Testing Strategy

C++ tests:

- overlay loader parses a valid overlay;
- invalid overlay reports warnings or failure without crashing;
- applying an override changes an existing object transform;
- applying a new instance appends a `WorldObject`;
- duplicate ids are rejected;
- unknown asset ids are rejected or reported;
- overlay-created instances participate in normal object rendering/collision authoring paths.

Python tests or CLI checks:

- valid overlay passes;
- duplicate ids fail;
- unknown asset ids fail;
- non-finite transforms fail;
- invalid zone tags fail.

Manual smoke:

- build dev tools;
- launch game;
- open editor with `F10`;
- select an existing object;
- change position/scale;
- add an asset from manifest;
- save overlay;
- restart game;
- confirm changes load.

## Implementation Order

1. Data structs and tests for overlay parsing/apply.
2. Loader integration so overlay affects the built level.
3. Python validator and CTest hook.
4. Dear ImGui dependency and dev-only build wiring.
5. Minimal editor UI: object list, inspector, save/load.
6. Add-instance workflow from asset manifest.
7. World selection and quality-of-life controls.

## Acceptance Criteria

The runtime editor foundation is ready when:

- dev build can open a Dear ImGui editor in-game;
- existing C++ objects can be overridden by id;
- new object instances can be added from the manifest;
- overlay saves to and loads from `data/world/block13_editor_overlay.json`;
- normal non-dev build remains free of editor UI;
- validator catches malformed overlay data;
- C++ tests prove load/apply behavior;
- full build, CTest, and smoke run pass.

## Later C# Editor Boundary

The later C# editor should not replace the runtime editor. It should handle structured content better suited to forms and tables:

- missions;
- dialogue;
- cutscene beats;
- NPC reaction catalogs;
- item and shop data;
- asset metadata;
- localization.

The C# editor should write data formats validated by the same Python pipeline. The runtime editor remains the tool for spatial authoring inside the actual game world.
