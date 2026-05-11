# Goal Session Card - 2026-05-09 (Spark v7 lock)

## Session lock

- Model: `5.3-Codex-Spark`
- Mode: `AMBER`
- One-sentence goal slice:
  Keep all weak-model runs goal-aligned by requiring explicit evidence, anti-hallucination counter-claims, and single-layer, reversible edits before any behavior change.
- Owner layer: **docs/procedure**
- Confidence (pre) - scope clarity 9/10, compatibility 9/10, backward compatibility 10/10, validation confidence 9/10
- Rollback action: stop behavior edits, restore the previous session card (`docs/goal-session-card-2026-05-09-spark-v6.md`) and resume no-file-change planning.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - We still do not have a true CI-enforced hard stop that requires per-session A/B/C decisions.

## Decision integrity

- Drift lock: every change must preserve Blok 13 visual/game identity and tooling sequence; no map content/style additions that are not mapped to a goal-driven consequence path.
- Consistency lock: one owner layer only (this session = docs/procedure), no behavior code edits.
- Counter-claim: the strongest wrong interpretation is that "guardrails are satisfied if one doc says so" (wrong because weak model runs frequently overfit without explicit source+gate evidence).
- Source lock: `docs/goal-low-capacity-protocol-2026-05-09.md` + `docs/goal-decision-checklist-template.md`.
- Gate anchor: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`.
- Fallback contract: because score remains high, keep one branch open:
  - `C`: defer additional automation until one CI-level hard-stop is added.
- Reversible next step:
  - if a stronger hard-stop is approved, add an optional `verify_goal_guard.py` mode token check and continue with docs only.

## Scope and acceptance

- Scope:
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-alignment-shield-2026-05-09.md`
  - `tools/verify_goal_guard.py`
  - `progress.md`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
  - progress references this card.

## Uncertainty and next step

- Uncertainty: whether we should move this card format from text-only to a machine-readable YAML/JSON block in future sessions.
- Next decision: keep docs/procedure lock now; only extend guardrail automation after this pass is verified green by the existing preflight.
