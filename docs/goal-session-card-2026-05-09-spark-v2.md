# Goal Session Card - 2026-05-09 (Spark v2 hardening)

## Session lock

- Model: `5.3-Codex-Spark`
- Mode: `AMBER`
- One-sentence goal slice:
  Add explicit anti-hallucination and contradiction gates to low-capacity iteration so every Spark session keeps the Blok 13: Rewir trajectory and avoids prototype-vibe drift.
- Owner layer: **docs/procedure**
- Confidence (pre) - scope clarity 9/10, compatibility 10/10, backward compatibility 10/10, validation confidence 9/10
- Rollback action: remove the Spark hardening additions from
  `docs/goal-low-capacity-protocol-2026-05-09.md` and revert `docs/goal-alignment-shield-2026-05-09.md` and `docs/goal-decision-checklist-template.md` sections if they raise noise.
- Evidence anchors:
  - source: `docs/goal-alignment-shield-2026-05-09.md:5`
  - gate: `rg -n "anti-hallucination|source lock|counter-claim" docs/goal-low-capacity-protocol-2026-05-09.md`

## Decision integrity

- Drift guard: every behavior-affecting pass must be justified against the one-line goal lock before implementation.
- Consistency guard: only decision-process docs changed; runtime/tooling behavior untouched in this pass.
- Counter-failure: if the new checks slow down work, fall back to a one-line source lock + explicit `AMBER` rationale before edits instead of skipping the gate.

## Scope and acceptance

- Scope:
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-decision-checklist-template.md`
  - `docs/goal-alignment-shield-2026-05-09.md`
  - `progress.md` (single entry)
- Acceptance:
  - one-sentence goal lock present,
  - anti-hallucination/contradiction checks explicitly required,
  - source lock + gate anchor required before merge,
  - rollback path remains explicit and immediate.

## Uncertainty

- Uncertainty: no automatic CI enforcement for this guardrail yet; enforcement is process-driven and depends on disciplined session use.
