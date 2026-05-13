# Current State

Status: LIVE
Last verified against code: 2026-05-13
Owner: project workflow

This document is a compact map of what the project currently appears to support. It is not a replacement for code, tests, validators, CMake, run scripts, or actual runtime smoke tests. If this file disagrees with implementation evidence, implementation evidence wins and this file should be updated.

## Operational truth

Current operational truth is:

```txt
code
CMakeLists.txt
CMakePresets.json
tests
validators
run scripts
actual runtime smoke behavior
DEVELOPMENT.md
```

README is an overview. Design docs are direction. Goal/session cards are archive unless explicitly reactivated.

## Current run command

Daily development should use:

```powershell
.\tools\run_dev.ps1
```

Release smoke should use:

```powershell
.\tools\run_release_smoke.ps1
```

Do not launch random `blokowa_satyra.exe` files from Explorer. The run scripts build from presets, resolve multi-config and single-config executable paths, print absolute paths, and pass an explicit source `--data-root`.

## Current build folders

Official preset build folders are:

- `build/debug`
- `build/release`
- `build/dev-tools`
- `build/ci-core`
- `build/ci`

`build-dev` is legacy/manual only and should not be used for normal development.

## Verified gates

As of this pass, the project has CTest-backed or script-backed gates for:

- **Implemented:** core unit tests through `bs3d_core_tests`.
- **Implemented:** game support tests through `bs3d_game_support_tests`.
- **Implemented:** asset validation through `tools/validate_assets.py`.
- **Implemented:** editor overlay validation through `tools/validate_editor_overlay.py`.
- **Implemented:** world contract validation through `tools/validate_world_contract.py`.
- **Implemented:** object outcome validation through `tools/validate_object_outcomes.py`.
- **Implemented:** mission validation through `tools/validate_mission.py`.
- **Implemented:** CI preset build/test/smoke workflow through `cmake --preset ci`, `cmake --build --preset ci`, `ctest --preset ci`, and `tools/ci_smoke.ps1`.
- **Implemented:** dev-tools build and CTest coverage through `cmake --preset dev-tools`, `cmake --build --preset dev-tools`, and `ctest --preset dev-tools`; `tools/ci_verify.ps1` runs this gate after the normal CI preset.

Recent smoke/build validation confirmed:

- `python tools\validate_assets.py data\assets`
- `python tools\validate_world_contract.py data --asset-root data\assets --quiet`
- `cmake --preset ci`
- `cmake --build --preset ci`
- `ctest --preset ci`
- `cmake --preset dev-tools`
- `cmake --build --preset dev-tools`
- `ctest --preset dev-tools`
- `tools\ci_smoke.ps1 -Preset ci -SmokeFrames 3`
- `tools\run_release_smoke.ps1 -SmokeFrames 3`
- `tools\run_dev.ps1 -GameArgs '--no-audio','--no-save','--smoke-frames','1'`

## What works now

- **Implemented:** preset-based build workflow for debug, release, dev-tools, ci-core, and ci.
- **Implemented:** checked-in dev and release smoke run scripts with explicit absolute executable/data-root logging.
- **Implemented:** runtime build identity in window title and debug panel, including DevTools status, data root, and executable path.
- **Implemented:** Python validation gates for assets, world contract, editor overlay, object outcomes, and mission data.
- **Implemented:** C++ core/game-support test targets are registered with CTest.
- **Implemented:** DevTools are isolated behind `BS3D_ENABLE_DEV_TOOLS` and the `dev-tools` preset.
- **Implemented:** the project has a playable intro slice with on-foot movement, vehicle driving, mission flow, interaction prompts, subtitles/objectives, debug HUD, and smoke-test launch paths.

## Partial systems

- **Partial:** world-object affordances and object outcome catalog exist for selected slice objects, not a complete open-world interaction library.
- **Partial:** Przypal/world-memory consequences are present for current low-stakes outcome hooks, but long-term story memory is not final production scope.
- **Partial:** runtime data/editor pipeline exists around JSON, overlay validation, and dev-tools workflow, but the broader authoring loop should still be treated as stabilizing. Map `objects` in `data/map_block_loop.json` are intentionally rejected by the world contract until runtime loading support exists; use editor overlay objects for current authoring.
- **Partial:** asset pipeline has manifest metadata, validation, and runtime registry rules, but it is still a small art-kit pipeline rather than a final content pipeline.
- **Partial:** mission/data-driven flow exists for the current intro slice and validated data, not a full campaign authoring stack.
- **Partial:** vehicle gruz feel is a tuned arcade slice for one current vehicle, not full traffic, law, parking, or systemic vehicle simulation.

## Experimental systems

- **Experimental:** Dear ImGui runtime map editor is dev-tools only.
- **Experimental:** C# `tools/BlokTools` is an external editor seed, not the authoritative production editor.
- **Experimental:** QA/debug modes and visual baseline views are development aids, not player-facing product UI.

## Aspirational direction

- **Aspirational:** dense micro-open-world systemic rewir simulation.
- **Aspirational:** long-term Przypal/world memory with estate-wide consequences.
- **Aspirational:** full episode/season structure.
- **Aspirational:** final art/material/audio pipeline.
- **Aspirational:** broader NPC, traffic, patrol, parking, bureaucracy, and social response systems.

## Known risk

The main risk is documentation overclaiming implementation. Before using any older doc as a task source, check whether there is code, a validator, a CTest, a run script, or a runtime smoke path that proves the claim.

## Next priority

Keep workflow and current-state docs honest while continuing production foundations. If a future task changes runtime behavior, update this file only when the change affects the global project map.
