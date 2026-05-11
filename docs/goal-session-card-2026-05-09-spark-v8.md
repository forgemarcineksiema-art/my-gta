# Goal Session Card - 2026-05-09 (Spark v8 lock)

## Session lock

- Model: `5.3-Codex-Spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  Keep all low-capacity sessions cognitively safe by forcing explicit evidence-gated decisions with counter-factual checks before any behavior-affecting change.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor tool
  - [x] Docs/procedure
- Confidence (pre) - scope clarity 9/10, compatibility 9/10, backward compatibility 10/10, validation confidence 9/10
- Rollback action: revert this lock to `docs/goal-session-card-2026-05-09-spark-v7.md` and continue with docs-only planning if guardrail checks fail.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - We have no CI integration that can auto-parse and block missing `source:`/`gate:` anchors inside mission/behavior decision cards from external contributors.

## Decision integrity

- Drift lock: every behavioral decision must preserve Blok 13/visual identity direction and link to either tool stack maturity or clear gameplay consequence.
- Consistency lock: no cross-system behavior edits without explicit split plan and explicit gate.
- Counter-claim: `Counter-claim:` The wrong interpretation is that "more frequent edits imply better alignment"; it breaks safety because weak-capacity sessions tend to shortcut evidence in ambiguous calls.

H_true:
- We can keep Spark sessions safe if every session card explicitly records one `source` and one `gate` anchor, one counter-claim, and a hypothesis pair with rollback binding.
- source: `docs/goal-low-capacity-protocol-2026-05-09.md`  
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- We can skip some of these checks and still stay safe because the engine already has many gates.
- source: `tools/verify_goal_guard.py`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

## Scope and acceptance

- Scope:
  - `tools/verify_goal_guard.py`
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-alignment-shield-2026-05-09.md`
  - `docs/goal-decision-checklist-template.md`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
  - `progress.md` references this card.
  - No behavior code changes in this session; docs/validator hardening only.

## One-line decision (if uncertainty stays)

- `C`: keep edits in docs/procedure until we add CI enforcement for source/gate parsing in external decision cards.
