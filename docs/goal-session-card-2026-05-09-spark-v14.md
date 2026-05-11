# Goal Session Card - 2026-05-09 (Spark v14 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  Before each behavior edit on 5.3 we run a single-owner pass where mission/editor/data decisions stay within the current owner layer and are backed by one reversible evidence lock.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor
  - [x] Docs/procedure
- Confidence (pre) - scope clarity 9/10, data compatibility 9/10, backward compatibility 10/10, validation confidence 9/10
- Rollback action: restore active loop lock to the previous card and stop behavior edits until a new `apply` ticket exists for this owner layer.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - if our intent-to-behavior mapping is still ambiguous across owner layers, we may be silently drifting from the vision direction.

## Intellectual safety packet (mandatory for Spark)

H_true:
- Safe path: we keep behavior edits small, single-owner, and reversible because every changed file is anchored to one explicit source+gate pair, then validated by unit tests before continuation.
- source: `docs/goal-decision-checklist-template.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- Unsafe path: we apply mission/data/visual changes across multiple owner layers without a fresh card and counterfactual check, then trust intuition to catch regressions.
- source: `docs/goal-intellectual-safeguards-2026-05-09.md`
- gate: `ctest --test-dir build -C Release --output-on-failure`

Counter-claim: The safe path is not “more files changed faster”; under 5.3, speed is preserved best by stronger gates, because weaker capacity increases the value of explicit reversibility and phase boundaries.

## Decision uncertainty handling

- if any confidence category drops below 8/10:
  - choose exactly one:
    - `A`: strict minimum patch
    - `B`: split into two validated steps
    - `C`: defer to next gate
- if two categories drop below 8/10:
  - switch to `RED` and run no behavior edits.
- if ambiguity persists for two turns:
  - force `RED`, choose `C`, and publish a new lock before any behavior path resumes.

## Scope and acceptance

- Scope:
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-intellectual-safeguards-2026-05-09.md`
  - `docs/goal-alignment-shield-2026-05-09.md`
  - `docs/goal-decision-checklist-template.md`
  - `progress.md`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
  - exactly one owner-layer checkbox is selected.
  - one explicit rollback action remains available.
  - one explicit uncertainty and one counter-claim are present.
