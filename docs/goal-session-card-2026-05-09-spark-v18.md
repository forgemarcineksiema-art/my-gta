# Goal Session Card - 2026-05-09 (Spark v18 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  In each 5.3-Codex-Spark pass, we must run the intellectual safety loop before any behavior changes and keep the same one-sentence objective direction, one-owner scope, and rollback path.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor
  - [x] Docs/procedure
  - [ ] Tests
- Confidence (pre): scope clarity 10/10, data compatibility 10/10, backward compatibility 9/10, validation confidence 10/10
- Rollback action: revert `docs/goal-low-capacity-protocol-2026-05-09.md` and this session card, and continue with only `GREEN`-status behavior cards after a passing
  `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`.
- Evidence anchors:
  - source: `docs/goal-alignment-shield-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - if one of the gate commands for the current pass is unavailable or unstable in this environment, the only safe option is to stay in AMBER and defer implementation.

## Intellectual safety packet

H_true:
- Pre-factum lock upgrades in documentation and protocol files reduce drift risk in low-capacity passes, because every pass must declare an unchanged high-level objective with one owner layer and explicit rollback.
- source: `docs/goal-alignment-shield-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- Locking only behavior code and skipping protocol updates first increases the chance of goal fragmentation, especially after a model downgrade or context reset.
- source: `tools/verify_goal_guard.py`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: The wrong shortcut is treating model downgrade as a warning only in text; without a passing lock packet, the weaker model can satisfy immediate tasks while silently weakening long-term vision.

## Decision uncertainty handling

if a confidence score drops below 8/10:
- choose exactly one:
  - `A`: strict minimum
  - `B`: split into two validated steps
  - `C`: defer to next major gate
- if two scores drop below 8/10: switch to RED and run no behavior edits.

## Scope and acceptance

- Scope:
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-alignment-shield-2026-05-09.md`
  - `docs/goal-decision-checklist-template.md`
  - `docs/goal-intellectual-safeguards-2026-05-09.md`
  - `tools/verify_goal_guard.py`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
