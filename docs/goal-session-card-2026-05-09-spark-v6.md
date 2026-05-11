# Goal Session Card - 2026-05-09 (Spark v6 lock)

## Session lock

- Model: `5.3-Codex-Spark`
- Mode: `AMBER`
- One-sentence goal slice:
  Keep all Spark sessions goal-aligned by forcing explicit evidence, single-layer scope, reversible edits, and test-first verification before any C++/Python/C#/runtime behavior change.
- Owner layer: **procedural guard / decision layer**
- Confidence (pre) - scope clarity 9/10, compatibility 9/10, backward compatibility 10/10, validation confidence 9/10
- Rollback action: abort this pass and follow A/B/C safety branch with no code edits if any hard-stop condition appears; revert any touched files via last-good git checkpoint.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Session evidence:
  - protocol: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - shield: `docs/goal-alignment-shield-2026-05-09.md`

## Decision integrity

- Drift guard: any feature direction that moves toward generic/blocky prototype visual style must be blocked until mapped to an owned rewir visual consequence plan.
- Consistency guard: one subsystem only unless an explicit split plan exists in-session.
- Counter-failure: if two or more confidence categories drop below 8/10, force `RED` and stop implementation.
- A/B/C requirement: under `RED`, choose exactly one of:
  - `A`: strict minimum patch
  - `B`: split into two validated steps
  - `C`: defer to next gate

## Scope and acceptance

- Scope:
  - `docs/goal-alignment-shield-2026-05-09.md`
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-decision-checklist-template.md`
  - `progress.md`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
  - progress references this card.
  - one explicit next-step choice is stated before any behavioral code change.

## Uncertainty

- Uncertainty: whether to force `GREEN` for doc-only optimization edits under Spark or keep all changes in `AMBER`.
