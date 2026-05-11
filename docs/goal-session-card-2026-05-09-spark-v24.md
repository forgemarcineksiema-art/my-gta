# Goal Session Card - 2026-05-09 (Spark v24 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  Advance the C# content-editor layer for structured data by adding a first-class shop/item catalog surface with shared Python-checkable validation shape, while preserving one-layer work scope and the dirty-block estate identity.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [x] C# data/editor tools
  - [ ] Docs/procedure
  - [ ] Tests
- Confidence (pre): scope clarity 8/10, data/model compatibility 8/10, backward compatibility 8/10, validation confidence 8/10
- Rollback action: stop the pass and restore `docs/goal-session-card-2026-05-09-spark-v23.md` + revert all edits in `tools/BlokTools` and `data/world/shop_items_catalog.json` from this pass.
- Evidence anchors:
  - source: `docs/goal-completion-audit-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

## Intellectual safety packet

H_true:
- A small C#-first addition for shops/items is the right next pass because the mission/content pipeline is already connected and we need an editor baseline for future mission/shop/cutscene/narrative work.
- source: `docs/superpowers/specs/2026-05-09-runtime-editor-design.md`
- gate: `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj`

H_false:
- Delaying shop/item editor work and staying in runtime systems now would leave runtime/motion polish unbalanced with missing content production bandwidth, slowing meaningful content growth.
- source: `docs/goal-completion-audit-2026-05-09.md`
- gate: `dotnet run --project tools\BlokTools\BlokTools.App\BlokTools.App.csproj`

Counter-claim: The alternative to shipping a C# shop/item editor now is to postpone content tooling, but that creates a bottleneck for missions and economy content despite stable low-level systems.

Uncertainty:
- I am not yet fully sure what the final shop runtime contract should be; this pass only creates editor-facing catalog structure and validation, not economy/effect integration.

## Decision uncertainty handling

if a confidence score drops below 8/10:
- choose exactly one:
  - `A`: strict minimum
  - `B`: split into two validated steps
  - `C`: defer to next major gate
- if two scores drop below 8/10: switch to `RED` and choose `A/B/C` before any behavior edits.

Current choice: `A`: strict minimum.

## Plan and acceptance

- Scope:
  - `tools/BlokTools` (C# editor/editorial core + tests + WPF snapshot surface),
  - `data/world/shop_items_catalog.json`.
- Acceptance:
  - one checked owner layer,
  - `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj` passes,
  - new shop/item catalog is readable in `BlokTools` snapshot,
  - one new artifact in `progress.md`.
