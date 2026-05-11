# Goal Session Card - 2026-05-09 (Spark v10 lock)

## Session lock

- Model: `5.3-Codex-Spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  Under 5.3-Spark, every pass must first satisfy the intellectual packet (source+gate, H_true/H_false, Counter-claim, rollback, uncertainty) and execute no more than one owner layer.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor tool
  - [x] Docs/procedure
- Confidence (pre) - scope clarity 8/10, compatibility 9/10, backward compatibility 10/10, validation confidence 8/10
- Rollback action: revert this lock to `docs/goal-session-card-2026-05-09-spark-v9.md` and continue with only doc-safe planning if the packet is incomplete.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty:
  - The only remaining risk is that manual visual-identity checks can only be sampled, not mathematically proven; all non-visual assertions stay test-gated.

## Decision integrity

- Drift lock: every pass must preserve Blok 13 and the visual/identity contract, and cannot introduce non-local mechanics without a local consequence hook.
- Consistency lock: no cross-system behavior edits without a declared split, and no speculative layer jumps from one owner layer to another.
- Counter-claim: The wrong move is treating a model downgrade as permission to ship wider edits; in Spark, each edit has to be smaller, reversible, and evidence-bound.

H_true:
- The safer route is to freeze to one owner layer in AMBER, require evidence packets, and gate each pass with a passing preflight before any behavior change.
- source: `docs/goal-low-capacity-protocol-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- The safe route is irrelevant because stronger guardrails are optional; this pass can continue with normal ad-hoc edits and still stay aligned.
- source: `tools/verify_goal_guard.py`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

## Scope and acceptance

- Scope:
  - `docs/goal-alignment-shield-2026-05-09.md`
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-intellectual-safeguards-2026-05-09.md`
  - `docs/goal-decision-checklist-template.md`
  - `docs/goal-completion-audit-2026-05-09.md`
  - `progress.md`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
  - `progress.md` references this card.
  - No behavior code changes in this session; documentation and guardrail hardening only.

## One-line decision (if uncertainty stays)

- `A`: apply only doc/protocol hardening and keep this pass at minimal reversible scope.
