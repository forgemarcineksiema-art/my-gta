# Goal Session Card - 2026-05-09 (Spark)

## Session lock

- Model: `5.3-Codex-Spark`
- Mode: `GREEN`
- One-sentence goal slice:
  Lock the objective in a repeatable way for weaker-context sessions so future changes stay aligned with Blok 13: Rewir and cannot drift into generic toy-prototype work.
- Owner layer: **docs/procedure**
- Confidence (pre) - scope clarity 9/10, compatibility 9/10, backward compatibility 10/10, validation confidence 9/10
- Rollback action: remove `[docs/goal-low-capacity-protocol-2026-05-09.md](C:/Users/Marcin/Documents/gta/docs/goal-low-capacity-protocol-2026-05-09.md)` and restore prior wording.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md:1`
  - gate: `rg -n "mode|confidence|safety|AMBER|RED|decision card" docs/goal-low-capacity-protocol-2026-05-09.md`

## Decision integrity

- Drift guard: every change is constrained to goal-state language, process rules, and evidence mapping; no gameplay/content style changes.
- Consistency guard: C++/Python/C# boundary is unchanged; only process docs were touched.
- Counter-failure: if the protocol becomes noisy and unused, the team can silently drift during low-capacity sessions; this is mitigated by adding mandatory anchors and hard stop checks.

## Scope and acceptance

- Scope:
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `progress.md` (single-session evidence entry)
- Acceptance:
  - one-sentence goal, mode, and anchors are declared,
  - protocol includes bootstrap/anchors/exit criteria for Spark,
  - progress contains a session card reference.

## Uncertainty

- Uncertainty: we still do not have an automated CI check that enforces this protocol at session boundaries.
