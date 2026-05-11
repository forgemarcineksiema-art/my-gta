# Goal Low-Capacity Protocol (5.3-Codex-Spark)

Status: ARCHIVED
Last verified against code: 2026-05-10
Owner: process archive
Source of truth for: historical low-capacity session process only; use `DEVELOPMENT.md` and `docs/current-state.md` for current gates

This protocol is mandatory for `5.3-Codex-Spark` sessions and exists to prevent:

- drift away from Blok 13 visual/game identity,
- speculative broad edits without evidence, rollback path, or validation.

## 0. Session bootstrap (mandatory before edits)

1. Set mode: `GREEN`, `AMBER`, or `RED`.
2. Lock the one-line goal slice (max 25 words).
3. Pick one owner layer only: C++, Python, C#, docs, or tests.
4. Set 4 confidence scores (0-10):
   - scope clarity,
   - data/model compatibility,
   - backward compatibility risk,
   - validation confidence.
5. Capture two anchors:
   - source anchor (`file:line`),
   - gate anchor (command + expected pass signal).
6. Declare one rollback action and one explicit uncertainty.
7. If any bootstrap item is missing, stay in `AMBER` and do not edit files.
8. Create one decision card and log it in `progress.md` (or a linked artifact) before merge.
9. Capture a one-line "source lock" with:
   - one `src/data/tools` anchor,
   - one command anchor from a passing gate.
10. Run the goal guard preflight:
   - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`.
11. Write one explicit counter-claim sentence (most likely wrong interpretation and why).
12. Write one counterfactual pair:
   - `H_true`: what we believe should currently happen.
   - `H_false`: what we fear is happening instead.
   - Each must include a `source:` and `gate:` anchor.
13. If the model token has changed, restart this protocol by:
    - creating a new `goal-session-card-*spark*.md` with a fresh `One-sentence goal slice`,
    - rerunning `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`,
    - and keeping no behavior edits in the same pass until the new card and evidence packet are complete.
14. Default state on restart:
    - set mode to `AMBER` by default for the first turn on the new model.
    - if any confidence score is below `8/10`, stay in `AMBER` and pick exactly one branch: `A`, `B`, or `C`.

## 1. Risk-to-mode rule

- Any confidence score `< 8` raises mode to at least `AMBER`.
- Two or more scores `< 8` force `RED`.
- In `RED`, do not implement: only choose `A/B/C` options and scope a safer follow-up step.

- Anti-drift hard-stop (Spark sessions): any decision that increases prototype-like readability risk (blocky/toy/Roblox-like direction) is blocked unless it is explicitly tied to a signed visual identity artifact in `docs/superpowers/specs/2026-05-09-grochow-target-vision-design.md` and one measurable gate/test.

## 1b. Spark uncertainty protocol

- If confidence is below `8/10` in any category, ask exactly one option and do only one:
  - `A`: strict minimum implementation
  - `B`: split into two validated steps
  - `C`: defer to next major gate
- If two or more categories are below `8/10`, force `RED` and stop implementation.

## 2. Before implementation

1. Restate the sentence from bootstrap unchanged.
2. Fill the model safety precheck in
   [goal-decision-checklist-template.md](docs/goal-decision-checklist-template.md).
3. Reconfirm mode and evidence anchors.
4. Confirm one validator/test or manual smoke is already prepared.
5. Confirm rollback action is immediate/reversible.
6. Run the inference clamp:
   - define one plausible local contradiction,
   - reject or defer it with explicit reason tied to canonical goal.
7. Confirm subsystem scope:
   - one subsystem = acceptable in `GREEN/AMBER`;
   - two unrelated subsystems = forced `AMBER` with explicit split.

## 3. During implementation

- If any anchor disappears (command output changes, merge conflict, failing gate), switch to `AMBER`.
- If any required gate goes red, stop edits, restore last-good behavior, and only return after a scoped split plan.
- Keep edits inside declared scope only.
- Prefer schema/data edits over architecture rewrites.
- If any runtime behavior is undocumented, pause and mark deferred TODO.
- Re-check decision integrity against
  [goal-decision-checklist-template.md](docs/goal-decision-checklist-template.md) before finalizing.

## 4. Anti-hallucination check (Spark mandatory)

- Before changing behavior, answer:
  - Which existing file proves this is needed now?
  - Which specific file proves this does not already exist?
- If any answer is uncertain, move to `RED` and choose one:
  - `A`: strict minimum patch,
  - `B`: two-step validated split,
  - `C`: defer.
- Keep this check visible in the decision card as a short note.

## 5. Exit condition

Session can end only if:

- one-sentence goal is still present and unchanged from bootstrap,
- mode is justified with evidence (or still `AMBER` with no code changes),
- at least one objective gate artifact exists,
- one rollback action remains available,
- one explicit uncertainty is documented.
- one decision card is present in `progress.md` or linked file.

If any condition fails, continue on a smaller, reversible step.

## 6. Weekly anchor checks

When possible, keep these checks in evidence:

- Use the current quality gates in `DEVELOPMENT.md`.
- Use `docs/current-state.md` for the current build folders and smoke workflow.

## 7. Spark hard-stop rules

- If the proposed change touches >1 owner layer, set mode `RED` and split the work before any edit.
- If no rollback point is defined in under 1 hour of work, abort implementation and switch to the A/B/C choice path.
- If no gate evidence is available for behavior edits, do not edit behavior; documentation-only safe path only.
- If the session repeats the same uncertainty twice, move to `RED` and ask for a one-line decision:
  - `A`: minimal constrained edit
  - `B`: two-step validated split
  - `C`: defer to next gate

## 8. Intellectual Safety Loop (minimum for Spark changes)

- Before any behavior-affecting decision:
  1. Write two concrete hypotheses:
     - `H_true`: what we believe is happening now,
     - `H_false`: likely opposite that would break goal alignment.
  2. For each hypothesis cite one file anchor and one gate/test anchor.
  3. Record one direct rollback action tied to the same anchor.
- If either hypothesis cannot be linked to files/tests, do not edit behavior.
- If the same hypothesis is repeated in two consecutive turns, move to `RED` and apply `C` (defer) unless a single new gate changes confidence above 8.
- Before returning, the card must include a sentence beginning with:
  - `Counter-claim:` (why the rejected interpretation was wrong, in one line).
- This loop is mandatory even for docs-only sessions when objective scope is redefined.

## 9. Owner-layer exclusivity guard

- Session decision must mark exactly one owner layer as active (C++ / Python / C# / docs / tests), and all implementation tasks must stay within that layer.
- If scope changes, force a new card and repeat bootstrap before touching code.

## 10. Intellectual Integrity Addendum

- Every behavior-affecting cycle must include a compact **Intellectual Safety Packet**:
  - one locked objective sentence,
  - explicit `H_true` and `H_false` (each with `source:` and `gate:`),
  - one `Counter-claim` sentence,
  - one direct rollback action,
  - one uncertainty line,
  - one owner-layer declaration with exactly one checked box.
- If any field of that packet is missing:
  - set mode to `AMBER`,
  - pick one of `A/B/C`,
  - and keep edits to a reversible minimum.
- The packet must be cross-referenced from:
  - [docs/goal-intellectual-safeguards-2026-05-09.md](goal-intellectual-safeguards-2026-05-09.md).

## 11. Spark v12 behavior envelope

- A behavior pass must stay inside exactly one owner layer and one subsystem.
- The active session card must be the most recent `goal-session-card-*spark*.md`; no exceptions.
- In `AMBER`, behavior edits must be either:
  - `A`: strict minimum patch, or
  - `B`: two-step split with an extra gate between steps.
- If a behavior pass touches identity, mission/dialog, or object-interaction behavior, it must include:
  - one source anchor in code/data/asset layer,
  - one gate anchor in the same pass.

This protocol is the hardening layer for weak-model iteration.
