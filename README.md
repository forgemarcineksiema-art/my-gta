# Blok 13: Rewir

Status: CURRENT
Last verified against code: 2026-05-13
Owner: project overview

Dirty-stylized desktop 3D vertical slice in C++17, CMake, and raylib 6.0.

This README is a project overview. For exact build/run workflow use [DEVELOPMENT.md](DEVELOPMENT.md), and for documentation truth status use [docs/README.md](docs/README.md) plus [docs/current-state.md](docs/current-state.md).

## What is implemented

- One stylized mid-poly block-estate loop map built from authored art-kit model instances: Block 13 facade dressing, Zenon's lived-in shop front, Rysiek's garage row, parking paint, muted route guidance, stylized curbs/lamps/bins, irregular asphalt/grass/dirt patches, fences, bollards, centralized sky/ground/haze presentation styling, and primitive debug/fallback rendering. The current visual-identity pass focuses on making the slice read as a dirty osiedlowy GTA-like rather than a blocky toy prototype — targeting PS2/early-PS3 era readability with stylized mid-poly silhouettes and modeled detail.
- Visual baseline data for start/Bogus, shop, parking, garages, road loop, rear block, plus v2 close-up QA views for Zenon's storefront, gruz wear, and the Block 13 front/intercom, so future art passes can be judged from repeatable angles instead of memory.
- Runtime world authoring now has a JSON data catalog: `data/map_block_loop.json` and `data/mission_driving_errand.json` are loaded at boot and applied before the intro slice starts.
- Dev builds include a Dear ImGui runtime map editor: `F10` opens object selection, transform editing, undo/redo, metadata-searchable manifest asset placement, and overlay save/load via `data/world/block13_editor_overlay.json`.
- The first C# external editor seed lives in `tools/BlokTools`: it loads the current mission/dialogue JSON plus `data/world/object_outcome_catalog.json`, validates editor data against the runtime phase catalog, opens a WPF surface for editing/adding mission steps and dialogue, shows quiet vs Przypal/world-memory outcome consequences, and can bind a step trigger to an object outcome through `outcome:<id>` with guarded `.bak` saves.
- App-layer asset registry validates `data/assets/asset_manifest.txt` at startup, fails fast on missing authored models, and reports model-load warnings.
- GTA V PC-style third-person camera, camera-relative walking, vehicle enter/exit, and simple car driving.
- Code-driven player foundation: input intent, kinematic motor, step/slope ground checks, wall slide, coyote jump, and procedural stylized movement feedback.
- Modular stylized mid-poly character rendering now supports named visual profiles plus v2 silhouette details such as cap brim, brows, cuffs, pockets, trouser stripes, and face/ear landmarks, so key NPCs can share the same readable body model while keeping distinct palettes.
- Camera boom collision shortens the camera against blocking geometry and returns smoothly.
- Walaszek-style feel layer: panic sprint presentation, short bump/stagger reactions, comedy event zoom/shake cooldown, and chase-driven camera tension.
- On-foot feel pass: Ctrl walk, default jog, Shift sprint, quick-turn assist, jump buffer, and closer upper-body camera framing.
- Gruz feel pass: desperate gas, horn cooldown, handbrake slip, light vehicle condition bands, smoke/rattle feedback, speed-aware vehicle camera, and v2 worn-car presentation with dirt, rust, primer, chipped edges, and scuffed bumpers.
- Canon intro slice: Bogus -> Zenon's shop apron/entrance -> Bogus -> Rysiek's gruz -> ShopTrouble -> chase -> return to parking.
- Subtitle-first dialogue, mission objective HUD with distance/arrow, checkpoint retry flow, autosave snapshot, and optional debug overlay.
- Testable core modules for mission flow, dialogue, chase state, player movement, vehicle movement, and collision.
- Contextual control foundation: raylib reads raw buttons, then `ControlContext` maps them to on-foot or vehicle actions.
- Interaction resolver: nearby actions are candidates with input family, radius, priority, and prompt, so `E` and `F` can resolve different things in crowded spots.
- World-object affordances: selected doors, garage segments, Zenon's rolling grille/price cards/notice, the Block 13 intercom/entry notice, the bench, trash dressing, and glass surfaces now expose low-priority `E` prompts with stable outcome ids/patterns; mission JSON can use `outcome:<id>` triggers that runtime resolves when those objects are used. Feedback speaker/line text and noisy `PublicNuisance` consequences can come from `object_outcome_catalog.json`, while C++ keeps fallback affordance rules for bootstrapping and quiet satire affordances stay non-eventful.

## Controls

- `Mouse`: orbit camera
- `WASD`: walk relative to camera or drive
- `Ctrl`: slower walk on foot
- `Shift`: sprint on foot / desperate gas in vehicle
- `Space`: jump on foot / sharp turn and handbrake while driving
- `H` or `LMB`: horn while driving
- `E` or `F`: start mission / enter vehicle / exit vehicle
- `R`: retry after mission failure
- `F1`: toggle debug overlay when built with `BS3D_ENABLE_DEV_TOOLS=ON`
- `F2` or `Esc`: lock/unlock mouse capture for playtesting
- `F8`: cycle visual QA baseline viewpoints when built with `BS3D_ENABLE_DEV_TOOLS=ON`
- `F9`: cycle character pose preview when built with `BS3D_ENABLE_DEV_TOOLS=ON`
- `F10`: toggle Dear ImGui runtime map editor when built with `BS3D_ENABLE_DEV_TOOLS=ON`
- Runtime editor buttons: `Undo`, `Redo`, `Save Overlay`, and asset placement from the manifest.
- `F11`: toggle fullscreen/windowed
- Window close button or `Alt+F4`: quit

## Current feel target

Player movement should be faster than RDR2, heavier than Fortnite, and simpler than a full GTA clone: responsive camera-relative jogging, readable sprinting, forgiving curb/ramp handling, and a camera that keeps the character visible without demanding constant mouse correction.

Comedy lives in presentation and feedback, not in broken input. The character may flail, wobble, and react like an idiot, but the controller should remain readable.

Vehicle feel follows the same rule: the car is a noisy gruz, but the controls are not gruz. Low speed should be easy to place on a parking lot, high speed should get louder and a little nervous, `Space` should make handbrake turns fun, and damage should make the car more pathetic without randomly stopping the player. Presentation should sell that state through smoke, sound, readable lights, and worn-car visual feedback.

Controls follow a small contextual vocabulary: `Shift` means faster or more aggressive, `Space` means dynamic maneuver, `E/F` means use or enter/exit, and mouse movement means looking. New states should add meanings through `ControlContext`, not by scattering key checks through gameplay code.

## Design docs

- [Blok 13: Rewir DNA](docs/blok-ekipa-world-dna.md): canon identity, Przypal, world memory, and development priorities.
- [Production truth contract](docs/production-truth-contract.md): shared world convention and production rules for assets, render, collision, camera, HUD/debug, save/load, validation, and scope.
- [Grochow target vision](docs/superpowers/specs/2026-05-09-grochow-target-vision-design.md): long-term Rockstar-style compressed district target, with Blok 13 as the heart and first quality gate.
- [Runtime editor design](docs/superpowers/specs/2026-05-09-runtime-editor-design.md): Dear ImGui in-game editor, overlay JSON, Python validation, and later C# data-editor boundary.
- [Goal alignment shield](docs/goal-alignment-shield-2026-05-09.md): decision and rollback rules for lower-capacity-model sessions to keep implementation aligned with the long-term vision.
- [Walaszek research notes](docs/walaszek-research-notes.md): tone observations and how to translate the animated work into systems.

When running focused development sessions with lower-capacity AI assistance, use this default decision gate before large changes:

1. Run the goal guard preflight:

   ```powershell
   python tools/verify_goal_guard.py --model 5.3-Codex-Spark
   ```

1. Fill a one-line pass on the [Goal alignment shield](docs/goal-alignment-shield-2026-05-09.md) decision card.
2. Confirm each planned action maps to a concrete test/validator (or manual smoke) before implementation.
3. Prefer the smallest reversible slice when uncertainty appears.
4. For Spark-scale/low-capacity runs, also run the [Low-capacity protocol](docs/goal-low-capacity-protocol-2026-05-09.md) and explicitly log mode (GREEN/AMBER/RED).

- [Low-capacity protocol](docs/goal-low-capacity-protocol-2026-05-09.md): anti-drift session loop for Spark/low-capacity.
- [Humor and tone bible](docs/humor-tone-bible.md): comedy rules for missions, NPC reactions, movement, camera, and dialogue.
- [Goal completion audit](docs/goal-completion-audit-2026-05-09.md): current objective-to-evidence map with explicit remaining gaps for next checkpoints.
- [v0.7 Osiedle pamieta plan](docs/v0.7-osiedle-pamieta-plan.md): Przypal, short-term world memory, and NPC reactions lite.
- [Player foundation quality gate](docs/player-foundation-quality-gate.md): movement/camera checks every future system must pass.
- [Control context foundation](docs/control-context-foundation.md): raw input to contextual action mapping for on-foot, vehicle, and future states.
- [Interaction resolver](docs/interaction-resolver.md): prompt/action priority rules for `E/F` in dense osiedle spaces.
- [Player/camera risk audit](docs/player-camera-risk-audit.md): known movement and camera risks, fixes, tests, and remaining unproven feel issues.
- [Vehicle gruz feel audit](docs/vehicle-gruz-feel-audit.md): current car-feel scope, tests, tuning rules, and known risks.
- [Manual playtest checklist](docs/manual-playtest.md): hands-on smoke checklist for controls, camera, vehicle, chase, and future Przypal checks.

## Build

Install a C++17 compiler and CMake. For daily development, do not launch random executables from Explorer; use the checked-in workflow in [DEVELOPMENT.md](DEVELOPMENT.md).

Daily dev run:

```powershell
.\tools\run_dev.ps1
```

This builds preset `dev-tools`, launches the matching `blokowa_satyra.exe`, and passes an absolute `--data-root <repo>\data`.

Release smoke:

```powershell
.\tools\run_release_smoke.ps1
```

Manual preset commands:

```powershell
cmake --preset dev-tools
cmake --build --preset dev-tools

cmake --preset release
cmake --build --preset release

cmake --preset ci
cmake --build --preset ci
ctest --preset ci
```

The combined CI helper also builds and tests the `dev-tools` preset:

```powershell
.\tools\ci_verify.ps1 -Preset ci
```

The game executable is `blokowa_satyra`. CMake fetches raylib 6.0 automatically.
Game builds copy `data/` next to the executable after build; for development launches, prefer the scripts above because they always pass the source `data/` folder through `--data-root <path>`.

Developer QA shortcuts are hidden in normal builds. They are enabled by the `dev-tools` preset. The old manual `build-dev` folder is legacy; use `build/dev-tools` instead.

Runtime editor output is saved to `data/world/block13_editor_overlay.json`. Validate it with:

```powershell
python tools\validate_editor_overlay.py data\world\block13_editor_overlay.json --asset-root data\assets
```

Object outcome hooks and their optional Przypal/world-memory consequences are validated against runtime policy with:

```powershell
python tools\validate_object_outcomes.py data\world\object_outcome_catalog.json --runtime-policy src\game\WorldObjectInteraction.cpp
```

This gate checks both noisy world-event metadata and quiet runtime affordance outcomes in both directions, so C++ object prompts stay visible to BlokTools and mission authoring, while stale catalog hooks cannot point at interactions the runtime will never emit. Every outcome also needs player-facing `speaker` and `line` text because runtime object feedback is data-enriched from this catalog.

Asset manifests and model files are documented in `docs/asset-pipeline.md`. Validate shipped asset hygiene with:

```powershell
python tools\validate_assets.py data\assets
```

The shared world-production contract is documented in `docs/production-truth-contract.md`. Validate manifest/map/overlay consistency with:

```powershell
python tools\validate_world_contract.py data --asset-root data\assets
```

The C# data-editor seed can be checked with:

```powershell
dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj
dotnet build tools\BlokTools\BlokTools.App\BlokTools.App.csproj
```

Run the editor surface with:

```powershell
dotnet run --project tools\BlokTools\BlokTools.App\BlokTools.App.csproj
```

To build only the core tests without fetching raylib:

```powershell
cmake -S . -B build-tests -DBS3D_BUILD_GAME=OFF
cmake --build build-tests --config Release
ctest --test-dir build-tests --output-on-failure
```

Preset shortcuts are also available:

```powershell
cmake --preset ci-core
cmake --build --preset ci-core
ctest --preset ci-core

cmake --preset ci
cmake --build --preset ci
ctest --preset ci
.\tools\ci_smoke.ps1 -Preset ci
```

## Performance target

The game opens at 1280x720, supports `F11` fullscreen toggle, and simulates gameplay through a clamped 60 Hz fixed-step accumulator. The art direction targets stylized mid-poly readability inspired by early PS3/late PS2 era aesthetics — chunky readable silhouettes with modeled detail, beveled edges, and material identity — optimized to run on weaker laptops while still reading as a specific block-estate place.

## Known current limitations

- No minimap, economy, combat, traffic, or larger district yet.
- The current police/chase runtime now has sensor memory, patrol/search/pursue/recover states, and LOS input, but it is still a compact intro-slice AI rather than city-scale traffic police.
- The current map is still a first art-kit rewir, not a finished estate layout or full open world; JSON data loading makes iteration easier, but final materials, textures, foliage, signs, and authored props are still future work.
- Camera pitch is clamped and camera collision is basic; indoor/stair/shop camera modes are still future work.
- World Memory is still short-term in simulation; story/progress state is persisted through the current autosave snapshot.
- Intro mission content is one playable slice, not the full first five-mission arc.
