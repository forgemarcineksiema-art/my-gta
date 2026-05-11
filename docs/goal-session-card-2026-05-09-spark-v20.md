# Goal Session Card - 2026-05-09 (Spark v20 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  Under 5.3-Codex-Spark, no behavior-affecting edits are allowed until this pass has a complete intellectual lock packet, unchanged goal slice, and a fresh gate preflight.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor
  - [x] Docs/procedure
  - [ ] Tests
- Confidence (pre): scope clarity 10/10, data/model compatibility 10/10, backward compatibility 10/10, validation confidence 9/10
- Rollback action: remove this card, revert to the last approved AMBER/GREEN card, and replay only documented gates before resuming behavior edits.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - The highest risk after model downgrade is unforced speculative scope creep into adjacent layers; if scope grows beyond this lock scope, split immediately.

## Intellectual safety packet

H_true:
- Spark-safe operation requires a complete lock packet before behavior edits so the goal remains stable across reduced-capacity inference passes.
- source: `docs/goal-low-capacity-protocol-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- Without a new lock packet, the session can drift into partial, undocumented edits that blur Blok 13 identity and weaken local consequence-first gameplay.
- source: `tools/verify_goal_guard.py`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: This is not "model performance tuning" work; it is control-flow work for goal coherence, so we must treat every objective rewrite as a protocol event.

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
  - `progress.md`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
  - `progress.md` mentions this active session card.

## Immediate no-go rule

- No behavior-affecting file edits (`src/`, `data/`, or runtime/test source under `tests/`) in this turn until a completed lock packet exists with this card linked in `progress.md`.
