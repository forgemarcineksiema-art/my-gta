# Goal Session Card - 2026-05-09 (Spark v31 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal:
  Preserve Blok 13 identity-first iteration while using a bounded, evidence-first loop where each pass is one subsystem, one rollback step, and one gate.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor tool
  - [x] docs/procedure
  - [ ] Tests
- Confidence (pre): 8/10, 8/10, 8/10, 9/10
- Rollback action: revert the newly added spark-lock/decision artifacts and restore `docs/goal-session-card-2026-05-09-spark-v30.md` as the active reference in `progress.md`.
- Evidence anchors:
  - source: `docs/goal-completion-audit-2026-05-09.md`
  - source: `docs/goal-alignment-shield-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty: should the next enforced safety slice be Python mission JSON validation now, or a C# mission-content editor hardening slice first?

H_true:
  source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
  source: `data/mission_driving_errand.json` (single source of truth for current mission flow)
  gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: dropping the one-layer/safety reset rule on every model switch looks faster, but it increases latent drift risk faster than any gameplay gain it brings.

Current choice: `A`.

## Decision guardrails for this pass

- Keep one owner layer only. No behavior edits until a fresh protocol restart is complete and this card is in force.
- No behavior edits while confidence in any score is < 8 in this pass.
- Apply anti-drift hard-stop:
  - no "Roblox/blocky prototype" style decisions without a local consequence or identity signal in project docs.
- Keep `H_true`, `H_false`, counter-claim, rollback and one uncertainty visible for every meaningful decision.
- If this lock is incomplete at any point, revert and stay in `AMBER`.
- Branch logic (if uncertain):
  - `A`: strict minimum pass (documentation/protocol hardening only)
  - `B`: split into two validated slices
  - `C`: defer to next gate before behavior changes

## Session acceptance

- Run `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` before any behavior edits.
- Keep all changes local to docs/procedure in this pass.
- Update `progress.md` with this active lock before continuing.
- Keep the session one-way and reversible: one lock file + one progress reference.
