# Goal Session Card - 2026-05-09 (Spark v26 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:
  Keep the long game direction stable after a model downgrade by enforcing evidence-first edits, one-owner scope, and explicit rollback before any game-system behavior changes.

- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor tools
  - [x] Docs/procedure
  - [ ] Tests

- Confidence (pre): scope clarity 8/10, compatibility 8/10, backward compatibility 8/10, validation confidence 8/10
- Rollback action: stop immediately and restore `docs/goal-session-card-2026-05-09-spark-v25.md`; revert any undocumented edits outside the declared owner layer for this session.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

## Intellectual safety packet

H_true:
- The safest immediate move is to re-affirm a Spark-specific lock before any behavior edit so weaker-context jumps cannot silently steer the project back into unbounded prototype-style work.
- source: `docs/goal-intellectual-safeguards-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- We can skip the lock refresh because previous Spark cards are "good enough"; this increases the chance of drift and accidental cross-layer edits when switching from higher-capacity models.
- source: `docs/goal-session-card-2026-05-09-spark-v25.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: A fresh lock does not slow us if it blocks only one additional command, and it prevents expensive rollback later.

Uncertainty:
- We still do not yet have an automatic "no code edits without lock" CI gate; until that exists, we rely on manual session discipline and script verification.

## Uncertainty branch rule

if a confidence score drops below 8/10:
- choose exactly one:
  - `A`: strict minimum
  - `B`: split into two validated steps
  - `C`: defer to next major gate
- if two scores drop below 8/10: switch to `RED` and choose `A/B/C` before any behavior edits.

Current choice: `A`: strict minimum.

## Decision guardrails for this pass

- Keep one-owner scope: `Docs/procedure` only during this lock refresh.
- Keep anti-drift lock: no gameplay/runtime/art changes before the lock packet is fully present and `green` or `amber` checks pass.
- Keep source lock:
  - `docs/goal-intellectual-safeguards-2026-05-09.md`
  - `docs/goal-alignment-shield-2026-05-09.md`
- Keep output lock:
  - `progress.md` contains the v26 card and explicit active-lock note.
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes before non-document edits.

## Acceptance

- `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
- `progress.md` contains one new entry for this card.
- no behavior edits are performed before this lock refresh is complete.
