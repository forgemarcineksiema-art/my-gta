# Goal Session Card - 2026-05-09 (Spark v27 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:
  Push one concrete C# tooling gain: make mission dialogue entries first-class phase-scoped data and keep the runtime contract consistent with authored mission flow.

- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [x] C# data/editor tools
  - [ ] Docs/procedure
  - [ ] Tests

- Confidence (pre): scope clarity 8/10, compatibility 8/10, backward compatibility 8/10, validation confidence 8/10
- Rollback action: stop and restore previous working state, dropping all changes under `tools/BlokTools` and `data/mission_driving_errand.json` from this pass.
- Evidence anchors:
  - source: `docs/goal-completion-audit-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

## Intellectual safety packet

H_true:
- Requiring dialogue phase everywhere in mission data is the highest-value C#-layer slice because it eliminates silent runtime drops while unblocking phase-targeted writing (hints/cues become actually usable).
- source: `tools/BlokTools/BlokTools.Core/MissionDocumentValidator.cs`
- gate: `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj`

H_false:
- We can postpone phase enforcement because existing dialogues are still readable as-is, but missing phase currently prevents phase-specific mission flow and causes inconsistent behavior under future content growth.
- source: `data/mission_driving_errand.json`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: Phase-scoping dialog in the editor adds one field now and prevents a larger class of runtime/content bugs later.

Uncertainty:
- Existing local runtime behavior for phase-filtered dialogs is already in place, but we do not yet have a separate test proving those phase-bound lines are visible at runtime; we continue to gate by existing content loader/validation tests.

## Uncertainty branch rule

if a confidence score drops below 8/10:
- choose exactly one:
  - `A`: strict minimum
  - `B`: split into two validated steps
  - `C`: defer to next major gate
- if two scores drop below 8/10: switch to `RED` and choose `A/B/C` before any behavior edits.

Current choice: `A`: strict minimum.

## Decision guardrails for this pass

- Keep one-owner scope: only C# data/editor tooling and mission data fixture.
- Keep anti-drift lock: no changes in runtime behavior logic or model-facing assets until this slice is validated.
- Keep source lock:
  - `tools/BlokTools/BlokTools.Core/MissionDocumentValidator.cs`
  - `tools/BlokTools/BlokTools.Core/MissionDocument.cs`
  - `tools/BlokTools/BlokTools.Core/MissionEditorSession.cs`
  - `tools/BlokTools/BlokTools.App/MainWindow.xaml`
  - `tools/BlokTools/BlokTools.App/MainWindow.xaml.cs`
  - `tools/BlokTools/BlokTools.Tests/Program.cs`
  - `data/mission_driving_errand.json`
- Keep output lock:
  - `progress.md` points to this card.
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
  - `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj` passes.

## Acceptance

- Mission dialogue lines in editor/test fixtures are phase-scoped and phase-aware edits are persisted.
- Validator rejects dialogue lines missing `phase` or using unknown mission phase.
- `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj` passes.
- `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
