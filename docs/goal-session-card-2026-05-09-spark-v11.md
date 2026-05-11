# Goal Session Card - 2026-05-09 (Spark v11 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  Under 5.3-Spark, we only execute reversible, evidence-anchored, one-owner-layer moves, and pause to choose A/B/C whenever confidence drops below 8/10.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor tool
  - [x] Docs/procedure
- Confidence (pre) - scope clarity 8/10, data compatibility 8/10, backward compatibility 9/10, validation confidence 8/10
- Rollback action: remove this session card and restore the previously active card (`docs/goal-session-card-2026-05-09-spark-v10.md`), then continue only with explicit documentation updates.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - The main risk is over-constraining on small uncertainty deltas; manual review remains necessary for visual-identity assertions that cannot be fully unit-tested.

## Intellectual safety packet (mandatory each Spark behavior pass)

H_true:
- The safe route is strict owner-layer exclusivity with packet-first decisioning: `source:` and `gate:` evidence before any behavior code change, plus one rollback action.
- source: `docs/goal-low-capacity-protocol-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- The safe route is optional because confidence looks high, so we can continue with broad multi-layer edits and infer quality from assumptions.
- source: `docs/goal-intellectual-safeguards-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: Treating a capacity downgrade as a formality is a false-negative move; we block wide behavior edits until packet checks are done in this pass.

- Direct rollback action: revert this card's scope back to no implementation and re-run the same pass using design/docs-only changes only.
- Decision uncertainty handling:
  - if any confidence category drops below 8/10:
    - choose exactly one:
      - `A`: strict minimum implementation
      - `B`: split into two validated steps
      - `C`: defer to next gate

## Decision integrity

- Drift lock: the game cannot drift toward "Roblox-like prototypicality" if this pass lacks a local consequence hook and vision alignment check.
- Consistency lock: no cross-subsystem behavior changes without explicit split in a new decision card.
- Scope lock: this card covers this pass only; any code ownership change requires a new card.

## Scope and acceptance

- Scope:
  - `docs/goal-alignment-shield-2026-05-09.md`
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-intellectual-safeguards-2026-05-09.md`
  - `docs/goal-decision-checklist-template.md`
  - `docs/goal-completion-audit-2026-05-09.md`
  - `progress.md`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
  - `progress.md` references this card.
  - one rollback action remains immediate.
  - one explicit uncertainty remains documented.

## Decision output if uncertainty persists

`A`: docs/procedure hardening only, no behavior changes, add one more evidence lock for the same owner layer, then retry.
