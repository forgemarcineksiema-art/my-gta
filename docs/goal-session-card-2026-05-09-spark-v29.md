# Goal Session Card - 2026-05-09 (Spark v29 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:
  Repaired the current C# mission editor slice by restoring phase roundtrip for dialogue rows (read, edit, add, save), so mission data stays schema-valid with the phase-aware validator.

- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [x] C# data/editor tools
  - [ ] Docs/procedure
  - [ ] Tests

- Confidence (pre): 9/10, 8/10, 9/10, 7/10
- Rollback action: revert only files under `tools/BlokTools` and `data/mission_driving_errand.json` from this pass.
- Evidence anchors:
  - source: `tools/BlokTools/BlokTools.Core/MissionDocumentValidator.cs` (`mission.dialogue.phase` + `mission.dialogue.phase.unknown`)
  - gate: `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj`
- Uncertainty: whether to set default dialogue phase from selected mission step or runtime-first step for new lines before binding.

## Intellectual safety packet

H_true:
source: `tools/BlokTools/BlokTools.App/MainWindow.xaml.cs`
gate: `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj`

H_false:
source: `tools/BlokTools.App/MainWindow.xaml` (already includes editable `Phase` column for dialogue)
gate: `dotnet build tools\BlokTools\BlokTools.App\BlokTools.App.csproj`

- Counter-claim: phase-aware UI without explicit roundtrip in the backing model is likely to fail validation later than it appears, because missing phase only surfaces at save/validator time.

## Uncertainty branch rule

For this pass in AMBER:
- `A`: strict minimum fix (phase column model + load/save + new-line defaults + minimal fixture update).
- `B`: add additional UI tests or end-to-end validation for row selection behavior first.
- `C`: defer and merge once a broader mission editor UI refactor lands.

Current choice: `A`.

## Decision guardrails for this pass

- Keep one-owner scope: `C# data/editor tools` only.
- Do not alter C++ runtime/renderer systems or Python validation pipeline in this pass.
- Keep anti-drift lock: preserve phase-aware mission workflow instead of loosening schema.
- Maintain output lock:
  - `progress.md` points to this lock before behavior edits.
  - `python tools\verify_goal_guard.py --model 5.3-Codex-Spark` passes after edits.
- Acceptance:
  - Mission dialogue rows expose and persist `Phase` in `MainWindow.xaml.cs`.
  - Existing dialogue fixtures include `phase` for each dialogue line.
  - `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj` passes.
