# Goal Session Card - 2026-05-09 (Spark v9 lock)

## Session lock

- Model: `5.3-Codex-Spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  For `5.3-Codex-Spark`, enforce an intellectual safety packet (`source`/`gate`, `H_true`/`H_false`, rollback, counter-claim, uncertainty) before any behavior-affecting edit.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor tool
  - [x] Docs/procedure
- Confidence (pre) - scope clarity 9/10, compatibility 9/10, backward compatibility 10/10, validation confidence 9/10
- Rollback action: revert this lock to `docs/goal-session-card-2026-05-09-spark-v8.md` and continue in docs-only planning if intellectual guardrails fail.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - We have no automatic gate for semantic "not Roblox" visual identity quality beyond manual captures plus smoke checks.

## Decision integrity

- Drift lock: every behavioral decision must preserve Blok 13/visual identity direction and link to either tool-stack maturity or clear gameplay consequence.
- Consistency lock: no cross-system behavior edits without explicit split plan and explicit gate.
- Counter-claim: `Counter-claim:` The wrong interpretation is that adding more edits automatically improves quality; under low-capacity sessions it raises drift risk unless each edit is anchored to evidence and rollback.

H_true:
- We can keep Spark sessions safe if every session card explicitly records one `source` and one `gate` anchor, one counter-claim, and a hypothesis pair with rollback binding.
- source: `docs/goal-low-capacity-protocol-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- We can skip intellectual guardrails and still stay aligned because this project already has many existing gates.
- source: `tools/verify_goal_guard.py`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

## Scope and acceptance

- Scope:
  - `tools/verify_goal_guard.py`
  - `docs/goal-intellectual-safeguards-2026-05-09.md`
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-alignment-shield-2026-05-09.md`
  - `docs/goal-decision-checklist-template.md`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
  - `progress.md` references this card.
  - No behavior code changes in this session; docs/validator hardening only.

## One-line decision (if uncertainty stays)

- `B`: add any remaining safety layer checks in the guard/doc set as a minimal second step, then continue with only one verified behavior slice.
