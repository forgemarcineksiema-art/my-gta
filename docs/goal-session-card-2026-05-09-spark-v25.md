# Goal Session Card - 2026-05-09 (Spark v25 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:
  Keep the long game direction stable while continuing the C++/Python/C# three-layer pipeline with one-owner, low-risk edits that produce verifiable editor progress and do not weaken Blok 13 visual/systems identity.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [x] C# data/editor tools
  - [ ] Docs/procedure
  - [ ] Tests
- Confidence (pre): scope clarity 8/10, data/model compatibility 8/10, backward compatibility 8/10, validation confidence 8/10
- Rollback action: stop the pass and restore `docs/goal-session-card-2026-05-09-spark-v24.md` plus revert all edits under `tools/BlokTools` and `data/world/shop_items_catalog.json` from this pass.
- Evidence anchors:
  - source: `docs/goal-alignment-shield-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

## Intellectual safety packet

H_true:
- The current highest-value next move is to complete the C# toolchain slice (shop catalog + editor consistency checks) so mission and economy authoring can scale without touching runtime behavior.
- source: `tools/BlokTools/BlokTools.Core/ShopCatalogEditorSession.cs`
- gate: `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj`

H_false:
- Spending this pass on broad runtime or art direction work first would not improve delivery risk because the current bottleneck is content-production tooling.
- source: `docs/goal-completion-audit-2026-05-09.md`
- gate: `dotnet build tools\BlokTools\BlokTools.App\BlokTools.App.csproj`

Counter-claim: If this pass tries to add both new owner systems at once, it will dilute rollback safety and increase drift risk on a low-capacity model.

Uncertainty:
- We still do not yet have the final economy/rules contract in C++ (`shop data` is editor-facing now, not fully runtime consumed yet), so behavior is intentionally limited to tooling and validation.

## Uncertainty branch rule

if a confidence score drops below 8/10:
- choose exactly one:
  - `A`: strict minimum
  - `B`: split into two validated steps
  - `C`: defer to next major gate
- if two scores drop below 8/10: switch to `RED` and choose `A/B/C` before any behavior edits.

Current choice: `A`: strict minimum.

## Decision guardrails for this pass

- Keep one-owner scope: only C# data/editor tooling.
- Keep anti-drift lock: no identity pass changes that alter the overall look/feel contract without runtime proof.
- Keep source lock:
  - `src/data/tools`: `tools/BlokTools/BlokTools.Core/BlokWorkspaceLoader.cs`, `MainWindow.xaml.cs`
  - gate: `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj`
- Keep output lock:
  - `progress.md` includes this card and scope summary.
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` must pass before further edits.

## Acceptance

- `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj` passes.
- `dotnet build tools\BlokTools\BlokTools.App\BlokTools.App.csproj` passes.
- `BlokTools` runtime load includes:
  - mission editor surface,
  - object outcome surface,
  - shop + shop item surface,
  - and no new issues from `BlokWorkspaceLoader`.
- `progress.md` contains one new entry for this card.
