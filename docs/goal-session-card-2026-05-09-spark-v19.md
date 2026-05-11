# Goal Session Card - 2026-05-09 (Spark v19 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  In every 5.3-Codex-Spark pass, behavior edits are blocked until the intellectual lock packet is complete and the one-sentence project direction stays unchanged.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor
  - [x] Docs/procedure
  - [ ] Tests
- Confidence (pre): scope clarity 10/10, data compatibility 10/10, backward compatibility 10/10, validation confidence 10/10
- Rollback action: remove this lock card and continue from the last approved `AMBER`/`GREEN` card only after a passing `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`.
- Evidence anchors:
  - source: `docs/goal-alignment-shield-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - if any source/gate anchor is unavailable, or model output confidence drops below 8/10, the only safe option is `AMBER` with strict minimum scope.

## Intellectual safety packet

H_true:
- The downgrade-to-Spark pass is stable only if every behavior change is deferred behind a completed lock packet: objective slice, owner, H_true/H_false with anchors, counter-claim, rollback, and gate.
- source: `docs/goal-low-capacity-protocol-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- If behavior changes start immediately on model change, the goal can drift toward prototype behavior because counter-safeguards were not yet recalibrated.
- source: `tools/verify_goal_guard.py`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: It is not safe to treat model change as a simple performance setting; without a new lock packet, the next pass may preserve none of the anti-drift constraints.

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
