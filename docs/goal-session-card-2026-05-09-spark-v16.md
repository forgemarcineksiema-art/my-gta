# Goal Session Card - 2026-05-09 (Spark v16 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  In each 5.3-Codex-Spark pass, we only accept one owner-layer intent, one explicit objective sentence, and a complete H_true/H_false evidence packet before any behavior edit.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor
  - [x] Docs/procedure
- Confidence (pre) - scope clarity 9/10, data compatibility 9/10, backward compatibility 8/10, validation confidence 9/10
- Rollback action: discard this card and restore the previous active v15 card, then run a new model-safe split in a new card.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - if the same pass requires both C++ behavior and C#/Python data tooling in one commit, scope must split immediately and switch to `RED`.

## Intellectual safety packet (mandatory for Spark)

H_true:
- Safe path: keep Spark work constrained to one owner layer, run the model guard card first, then execute exactly one reversible micro-pass with a passing objective gate.
- source: `docs/goal-low-capacity-protocol-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- Unsafe path: begin behavior edits after only informal alignment comments, then patch cross-layer systems in one turn and hope gates catch side effects later.
- source: `docs/goal-alignment-shield-2026-05-09.md`
- gate: `ctest --test-dir build --output-on-failure`

Counter-claim: For weak-model sessions, speed is not a quality metric; evidence density is, and only one owner layer keeps that evidence checkable.

## Decision uncertainty handling

- if any confidence category drops below 8/10:
  - choose exactly one:
    - `A`: strict minimum
    - `B`: split into two validated steps
    - `C`: defer to next gate
- if two categories drop below 8/10:
  - switch to `RED` and run no behavior edits.
- if ambiguity persists for two turns:
  - force `RED`, choose `C`, and publish a split plan in docs before behavior resumes.

## Scope and acceptance

- Scope:
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-intellectual-safeguards-2026-05-09.md`
  - `docs/goal-alignment-shield-2026-05-09.md`
  - `docs/goal-decision-checklist-template.md`
  - `progress.md`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
  - `progress.md` references the current Spark session card.
  - one explicit rollback action remains available.
  - one explicit uncertainty and one counter-claim are present.
  - no behavior files are changed while only docs/procedure is owner.

## Decision output if uncertainty persists

`C`: publish a docs-first split and start a new Spark vNN+1 lock before touching behavior.

