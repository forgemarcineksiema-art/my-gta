# Goal Session Card - 2026-05-09 (Spark v15 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  In each 5.3-Codex-Spark pass, keep one owner layer, one reversible micro-slice, and a complete H_true/H_false evidence packet before any behavior edit.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor
  - [x] Docs/procedure
- Confidence (pre) - scope clarity 9/10, data compatibility 9/10, backward compatibility 9/10, validation confidence 8/10
- Rollback action: revert the loop lock to the previous active card and stop edits until a new owner-specific lock exists with fresh sources + gates.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - if the next objective requires touching both runtime and content layers, the active owner is wrong and the pass must switch to `C` or `RED`.

## Intellectual safety packet (mandatory for Spark)

H_true:
- Safe path: plan and execute exactly one owner-layer behavior slice at a time, using only declared scope, then validate before expanding.
- source: `docs/goal-low-capacity-protocol-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- Unsafe path: edit C++ / Python / C# together without a dedicated lock and separate rollback plan, hoping validation later will catch drift.
- source: `docs/goal-alignment-shield-2026-05-09.md`
- gate: `ctest --test-dir build --output-on-failure`

Counter-claim: The safe route is not "do more things now"; for Spark, it is the same as doing less with stronger evidence, because fewer parallel moves produce fewer untrackable regressions.

## Decision uncertainty handling

- if any confidence category drops below 8/10:
  - choose exactly one:
    - `A`: strict minimum
    - `B`: split into two validated steps
    - `C`: defer to next gate
- if two categories drop below 8/10:
  - switch to `RED` and run no behavior edits.
- if ambiguity persists for two turns:
  - force `RED`, choose `C`, and publish a new lock before any behavior work resumes.

## Scope and acceptance

- Scope:
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-intellectual-safeguards-2026-05-09.md`
  - `docs/goal-alignment-shield-2026-05-09.md`
  - `docs/goal-decision-checklist-template.md`
  - `docs/goal-session-card-2026-05-09-spark-v14.md`
  - `progress.md`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
  - one explicit rollback action remains available.
  - one explicit uncertainty and one counter-claim are present.
  - no behavior files are changed while only docs/procedure is owner.

## Decision output if uncertainty persists

`C`: publish a split plan in docs first, assign one clear owner layer in the next lock, and continue only after a new packet.
