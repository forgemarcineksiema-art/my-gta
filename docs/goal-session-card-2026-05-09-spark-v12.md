# Goal Session Card - 2026-05-09 (Spark v12 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  Under 5.3-Codex-Spark we only change behavior in tight, reversible slices after a completed Intellectual Safety Packet, one-owner-lock, and a documented rollback gate.
- Owner layer:
  - [x] Docs/procedure
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor
- Confidence (pre) - scope clarity 9/10, data compatibility 9/10, backward compatibility 9/10, validation confidence 9/10
- Rollback action: remove this session lock and restore active status to [docs/goal-session-card-2026-05-09-spark-v11.md](docs/goal-session-card-2026-05-09-spark-v11.md), then continue with only documentation-only decisions until all safeguards are re-established.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md` + `docs/goal-intellectual-safeguards-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - risk that a future behavior edit will drift toward cross-subsystem changes despite owner-layer lock, especially when mission and rendering priorities compete.

## Intellectual safety packet (mandatory each Spark behavior pass)

H_true:
- Safe path: before any behavior change we complete one-sentence goal restatement, one owner-layer lock, and a linked source + gate pair, then execute one reversible step with immediate rollback.
- source: `docs/goal-low-capacity-protocol-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- Unsafe path: we keep momentum by treating confidence statements as optional and implement mixed-layer changes without explicit counterfactual rejection and rollback plan.
- source: `docs/goal-alignment-shield-2026-05-09.md`
- gate: `ctest --test-dir build --output-on-failure`

Counter-claim: Treating model downgrade as a metadata detail is wrong; a Spark pass without a completed packet increases the chance of goal drift and silent integration errors.

## Decision uncertainty handling

- if any confidence category drops below 8/10:
  - choose exactly one option:
    - `A`: strict minimum implementation
    - `B`: split into two validated steps
    - `C`: defer to next gate
- if two categories drop below 8/10:
  - switch to `RED` and enforce only `A/B/C` without behavior edits.

## Decision integrity

- Drift lock: no behavior pass is allowed to weaken "Blok 13 as Warsaw estate identity" without a source anchor proving identity alignment.
- Scope lock: exactly one owner layer per pass; any new owner requires a fresh session card before edits.
- Rollback lock: every behavior pass must contain one immediate inverse/disable path written in the same card.

## Scope and acceptance

- Scope:
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-alignment-shield-2026-05-09.md`
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

`C`: defer behavior edits and keep this session locked to documentation-hardening until we have one fully anchored source file + one gate for each candidate owner layer.
