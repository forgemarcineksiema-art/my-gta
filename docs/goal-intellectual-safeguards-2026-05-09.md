# Goal Intellectual Safeguards (5.3-Codex-Spark)

This document defines the minimal cognitive-safe loop required for any substantial `5.3-Codex-Spark` pass.

## 1) Objective-lock requirement

Every pass starts with a single sentence:
- what exact objective move is being made now,
- why this is the next highest-value move,
- and which subsystem is responsible.

No other objective sentence is allowed in the same pass.

## 2) Two-sided proof lock

Before any behavior-affecting decision:
1. State `H_true` (what we believe should happen now) with:
   - one file anchor (`file:line`),
   - one gate anchor (`command => expected result`).
2. State `H_false` (the likely wrong interpretation) with:
   - one file anchor,
   - one gate anchor.
3. Add one explicit **Counterfactual** line:
   - why `H_false` is plausible,
   - and why it is rejected after validation.
4. Resolve one with direct evidence before editing behavior.

If either hypothesis cannot be anchored, do not edit behavior.

## 3) Anti-speculation rule

- No broad architecture jump.
- No new cross-subsystem path without a matching runtime/data/tooling owner and matching gate.
- No "I think it fits" edits: every change must have at least one checkable anchor or command gate.

## 4) Contrarian lock

Each behavior change must include:
- one `Counter-claim` sentence (one line, one rejected but plausible interpretation),
- one direct rollback action tied to that decision.

If rollback is not obvious, split into a safe minimum-first plan first.

## 5) Uncertainty budget

- When confidence in any of the four scores (scope, compatibility, compatibility-risk, validation) drops below `8/10`, stop and execute exactly one A/B/C branch:
  - `A`: strict minimum,
  - `B`: validated split,
  - `C`: defer.
- Two or more scores below `8/10` force `RED`.

## 6) Dual-lock evidence stack (mandatory under Spark)

Before any behavior-affecting change, each candidate pass must list both:

- Structural anchor: a source statement in the owning layer file (`src`, `data`, `tools`, or tests) and the owning layer decision card.
- Temporal anchor: a gate command run in the same pass (or already passed this cycle) with its expected outcome.

If either anchor is missing, do not implement: either close the pass with `C` or run the missing anchor first.

## 7) One-owner integrity check

Every behavior pass must state exactly one active owner and must not reference another owner layer in an implementation branch. If a second owner appears, the pass must:

- switch to `RED` or
- split into two explicit `A/B` validated slices, each with its own lock.

## 6) Exit lock

Session can only end when all are present:
- one decision card in `progress.md`,
- one current source anchor and one gate anchor,
- one rollback action,
- one explicit uncertainty line,
- and a passing `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`.

This keeps the goal direction stable even with reduced model capacity.

## 7) Spark model-shift hard restart

When switching to `5.3-Codex-Spark` from any stronger model:

- create a fresh `goal-session-card-*spark*.md` lock before touching behavior,
- re-run `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`,
- keep mode at least `AMBER` until all anchors are explicitly present,
- apply one-owner integrity strictly (one checkbox in exactly one owner layer),
- add one explicit anti-drift sentence:
  - no action may reduce local-world identity into generic prototype language without a local consequence hook.
