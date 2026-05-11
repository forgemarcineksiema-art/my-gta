# Goal Session Card - 2026-05-09 (Spark v23 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  Keep all Spark passes constrained to one owner layer and one subsystem, preserve Blok 13's dirty Grochow identity over prototype-like appearance, and advance the C++ -> Python -> C# authoring pipeline only with source+gate evidence.
- Owner layer:
  - [x] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# content/editor tools
  - [ ] Docs/procedure
  - [ ] Tests
- Confidence (pre): scope clarity 8/10, data/model compatibility 8/10, backward compatibility 8/10, validation confidence 8/10
- Rollback action: restore the previous `active lock` card, revert all files touched in this pass, and re-run `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md:13`
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

## Intellectual safety packet

H_true:
- `Runtime editor + pipeline decisions` are the highest-value path now because the visual and interaction foundations are already defined and already partially validated in C++.
- source: `docs/superpowers/specs/2026-05-09-runtime-editor-design.md`
- gate: `python tools/verify_editor_overlay.py`

H_false:
- This session could drift into content-side convenience work (C# or Python) too early and dilute core iteration quality, while hidden identity regressions appear in the map editor/interaction layer.
- source: `docs/goal-completion-audit-2026-05-09.md`
- gate: `ctest --test-dir build -C Release -R bs3d_core_tests --output-on-failure`

Counter-claim: The plausible "quick C#/Python slice" would be faster, but it is wrong now because it would bypass the C++/overlay lockpoint that already governs authored world quality and identity.

Uncertainty:
- We still do not have a hard automated visual-drift detector; if a pass cannot include at least one human-validated visual check, we should choose `B` and split into a minimum/verification pass.

## Decision uncertainty handling

if a confidence score drops below 8/10:
- choose exactly one:
  - `A`: strict minimum
  - `B`: split into two validated steps
  - `C`: defer to next major gate
- if two scores drop below 8/10: switch to `RED` and choose `A/B/C` before any behavior edits.

## Immediate no-go rule

- No behavior edits in `src/`, `data/`, `tools/`, `tests/`, or `C#` projects before this card exists, is the active lock, and the guard preflight has passed.
- If anchor or scope is unclear, stop and return to checklist-only mode.

## Scope and acceptance

- Scope:
  - `docs/goal-low-capacity-protocol-2026-05-09.md`
  - `docs/goal-intellectual-safeguards-2026-05-09.md`
  - `docs/goal-alignment-shield-2026-05-09.md`
  - `tools/verify_goal_guard.py`
- Acceptance:
  - one-sentence objective slice unchanged for the pass
  - one checked owner layer
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes
