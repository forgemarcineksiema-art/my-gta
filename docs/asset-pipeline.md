# Blok 13 Asset Pipeline

Status: CURRENT
Last verified against code: 2026-05-10
Owner: asset pipeline
Source of truth for: manifest metadata expectations and asset validation workflow

This project currently uses a small, runtime-safe art kit rather than a full AAA material pipeline. The goal is to keep asset authoring fast while making every shipped object predictable in the game, the runtime editor, and CI.

Asset authoring follows the shared [Production Truth Contract](production-truth-contract.md). In short: `Y` is up, units are meters, `WorldObject.position` is the bottom-center ground contact point, and manifest metadata is the source of truth for origin, render bucket, collision intent, camera blocking, and gameplay visibility.

## Current Formats

- `.obj` is the fast prototype format for simple static meshes. The runtime recolors these models from `fallbackColor` in `data/assets/asset_manifest.txt`.
- `.gltf` is the preferred direction for hero assets and future Blender exports because it can carry authored materials. `vehicle_gruz_e36` is the current example.
- `.bin` files are treated as glTF companion buffers and must be referenced by a `.gltf`.
- Texture files are not a mainline workflow yet. Current materials are mostly palette/fallback-color based.

## Manifest Contract

Every asset is declared in `data/assets/asset_manifest.txt`:

```text
id|modelPath|fallbackSize|fallbackColor|metadata
```

Metadata is optional for simple legacy/fallback entries, but production assets should declare their intent explicitly. The Python validator and runtime registry currently understand:

- `origin=bottom_center`
- `type=SolidBuilding`, `type=SolidProp`, `type=DynamicProp`, `type=Decal`, `type=Glass`, `type=Marker`, `type=DebugOnly`, `type=VehicleRender`, or `type=CollisionProxy`
- `usage=active`, `usage=legacy`, or `usage=debug`
- `render=Opaque`, `render=Decal`, `render=Glass`, `render=Translucent`, `render=DebugOnly`, or `render=Vehicle`
- `collision=Unspecified`, `collision=None`, `collision=Ground`, `collision=Prop`, `collision=SolidWorld`, `collision=DynamicProp`, `collision=VehicleDynamic`, or `collision=CollisionProxy`
- `collisionProfile=...` as the preferred alias when an entry is declaring a gameplay collision profile
- `cameraBlocks=true/false`
- `requiresClosedMesh=true/false`
- `allowOpenEdges=true/false`
- `allowFloating=true/false`
- `renderInGameplay=true/false`
- `maxHeight=<meters>`
- `visualOffset=x,y,z`
- `tags=a,b,c`

Runtime objects then place manifest assets as instances with position, scale, yaw, zone tag, gameplay tags, and collision intent.

The runtime editor uses the full manifest definition when placing assets from its asset panel. New editor-created instances inherit `tags=` metadata into their `gameplayTags`, and those tags are written back to `data/world/block13_editor_overlay.json`.

The editor asset filter searches across asset id, model path, origin, render bucket, collision intent, and tags. Multi-word filters require every token to match, so searches such as `translucent decal`, `ground none`, or `bench prop` are useful while dressing Grochow.

## Validation

Run the asset validator before relying on new or changed assets:

```powershell
python tools\validate_assets.py data\assets
```

CTest runs the same validator in quiet mode and also runs unit tests for the validator itself.

The validator checks:

- manifest format and duplicate asset ids;
- missing model files;
- positive finite fallback sizes;
- alpha assets declaring `render=Translucent`;
- known metadata keys and values;
- duplicate or whitespace tags;
- `.obj` vertex/face presence and project-specific geometry rules;
- `.gltf` 2.x metadata, required buffers, and mesh primitives with `POSITION` and `NORMAL`.

## Direction

Keep `.obj` for quick blocking and tiny reusable props. Move important Grochow identity assets toward Blender-authored `.gltf`: shop fronts, block entrances, garage rows, signage, larger street pieces, and eventually character/vehicle variants.

The intended toolchain is:

```text
Blender -> glTF/bin -> asset_manifest.txt -> Python validation -> runtime editor placement -> overlay JSON
```

Python owns preflight validation and future Blender exporter/add-on checks. C++ owns runtime loading, rendering, collision interpretation, and in-game placement.
