# Goal Session Card - 2026-05-09 (Spark v22 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  Every behavior-affecting Spark pass must be preceded by a completed cognitive lock packet (source+gate anchors, H-true/H-false, rollback, uncertainty, and one-owner scope) to prevent drift under reduced model capacity.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor
  - [ ] Docs/procedure
  - [x] Tests
- Confidence (pre): scope clarity 10/10, data/model compatibility 10/10, backward compatibility 9/10, validation confidence 9/10
- Rollback action: stop all behavior edits in this pass and replay only this card + `docs/goal-low-capacity-protocol-2026-05-09.md` when any anchor fails or contradiction is confirmed.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - Can this lock become too rigid for rapid iteration, so we overconstrain low-stakes tasks? If so, split passes into smaller cards instead of weakening lock requirements.

## Intellectual safety packet

H_true:
- Under reduced-capacity model conditions, a Spark pass remains stable and goal-aligned only when every behavior change is bounded to one owner layer and justified by both a source anchor and a gate anchor already available in the session plan.
- source: `docs/goal-low-capacity-protocol-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- Without the strict lock, passes can unintentionally combine layers (for example C++, Python, C# in one pass), producing changes that pass quick local checks but weaken Blok 13 identity, map-authoring discipline, or regression intent.
- source: `tools/verify_goal_guard.py`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: This is a minimum-friction safety-first pass, not a bureaucracy pass; if no behavior edits are being made, lock requirements can stay docs-only while preserving the active one-sentence objective.

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
  - `docs/goal-intellectual-safeguards-2026-05-09.md`
  - `docs/goal-decision-checklist-template.md`
  - `tools/verify_goal_guard.py`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

## Immediate no-go rule

- No behavior edits in `src/`, `data/`, `tools/`, `tests/`, or `C#` projects before this card is the active lock in `progress.md`.
- If lock confidence drops below threshold mid-pass, switch to RED and return a one-line A/B/C clarification before any new edit.
