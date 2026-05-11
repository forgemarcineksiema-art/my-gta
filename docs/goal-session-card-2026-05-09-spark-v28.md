# Goal Session Card - 2026-05-09 (Spark v28 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:
  Stabilize low-capacity model execution by preventing goal drift and forcing a reversible, evidence-first loop before any behavior change.

- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor tools
  - [x] Docs/procedure
  - [ ] Tests

- Confidence (pre): 9/10, 9/10, 9/10, 7/10
- Rollback action: pause implementation immediately and revert to the last passing lock (`docs/goal-session-card-2026-05-09-spark-v27.md`) and clean workspace state.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty: whether to keep one more "docs-only hardening" step before behavior can resume in the active C# editor slice.

## Intellectual safety packet

H_true:
source: `docs/goal-low-capacity-protocol-2026-05-09.md` (Spark hard-switch requirements section)
gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
source: `tools/BlokTools/BlokTools.Core/MissionDocumentValidator.cs` (ongoing C# slice)
gate: `dotnet run --project tools\\BlokTools\\BlokTools.Tests\\BlokTools.Tests.csproj`

- Counter-claim: the weaker model should be treated as a signal-strength issue only, but skipping explicit locks would silently widen scope drift in this project more than usual.

## Uncertainty branch rule

Model-shift work is in AMBER, so choose one path before any behavior edit:
- `A`: strict minimum docs-only loop completion and lock continuity.
- `B`: split behavior work and add a second intermediate gate between content-model and UI edits.
- `C`: defer all behavior work until `docs` lock is green for two full passes.

Current choice: `A` until guard preflight + lock continuity is re-verified.

## Decision guardrails for this pass

- Keep one-owner scope: `Docs/procedure` only.
- No behavior edits before `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes on the active lock.
- Add an explicit anti-drift line in:
  - [docs/goal-alignment-shield-2026-05-09.md](goal-alignment-shield-2026-05-09.md)
  - [docs/goal-low-capacity-protocol-2026-05-09.md](goal-low-capacity-protocol-2026-05-09.md)
- Keep output lock:
  - `progress.md` points to this card before behavior resumes.
  - `docs/goal-completion-audit-2026-05-09.md` notes active lock migration.

## Acceptance

- One active lock for the new model exists and is the most recent Spark lock.
- Guard preflight runs cleanly with the new lock name:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- No behavior changes happen while this pass is in AMBER.
