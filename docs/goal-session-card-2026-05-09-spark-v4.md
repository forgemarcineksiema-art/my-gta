# Goal Session Card - 2026-05-09 (Spark v4 lock)

## Session lock

- Model: `5.3-Codex-Spark`
- Mode: `AMBER`
- One-sentence goal slice:
  Keep the iteration loop focused on Blok 13 visual/game identity and Rockstar-style compressed Grochow architecture, while executing only one subsystem at a time with explicit evidence gates and reversible edits.
- Owner layer: **docs/procedure**
- Confidence (pre) - scope clarity 8/10, compatibility 9/10, backward compatibility 9/10, validation confidence 8/10
- Rollback action: undo only this session's added goal-governance updates:
  - remove the newest `goal-session-card-2026-05-09-spark-v4.md` if it conflicts with later decisions,
  - revert recent guard wording changes in:
    - `docs/goal-low-capacity-protocol-2026-05-09.md`
    - `docs/goal-alignment-shield-2026-05-09.md`.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md:1`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

## Decision integrity

- Drift guard: any decision that could push toward Roblox/toy-like prototype must fail unless it includes a concrete visual-identity fallback (decal/grime/material/facade/interactions).
- Consistency guard: one subsystem only (C++, Python, C#, or docs/procedure) per Spark pass unless explicitly split.
- Counter-failure: if a proposal cannot be justified by source evidence and one local rollback path, defer implementation and switch to `RED` with option `A/B/C`.

## Scope and acceptance

- Scope:
  - `docs/goal-low-capacity-protocol-2026-05-09.md` (protocol refinement)
  - `docs/goal-alignment-shield-2026-05-09.md` (one-line hardening additions)
  - `docs/goal-decision-checklist-template.md` (model-safety checklist extension)
  - `progress.md` (session traceability line)
- Acceptance:
  - one-sentence goal lock present and unchanged,
  - AMBER mode + rollback + uncertainty + evidence anchors present,
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.

## Uncertainty

- Uncertainty: how strict to enforce the "one subsystem at a time" rule on small UI polish tasks versus core runtime/data tasks.

## Spark choice path

- `A`: strict minimum policy/doc updates only (no runtime/data/tooling behavior changes).
- `B`: split into two validated docs + gates before enabling any behavior edits.
- `C`: defer behavior edits until next decision gate explicitly green-lit in this session card.
