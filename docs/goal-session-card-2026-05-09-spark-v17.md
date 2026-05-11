# Goal Session Card - 2026-05-09 (Spark v17 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  In each 5.3-Codex-Spark pass, implement one owner layer at a time and verify mission-objective/runtime-data ordering behavior before any next-layer change.
- Owner layer:
  - [x] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor
  - [ ] Docs/procedure
  - [ ] Tests
- Confidence (pre): scope clarity 9/10, data compatibility 9/10, backward compatibility 8/10, validation confidence 10/10
- Rollback action: revert `src/core/MissionController.cpp` to last clean Spark checkpoint and rerun
  `ctest --test-dir build -C Release --output-on-failure`.
- Evidence anchors:
  - source: `src/core/MissionController.cpp`
  - gate: `ctest --test-dir build -C Release --output-on-failure`
- Uncertainty:
  - if any phase has both custom phase dialogue and default NPC lines in same pass, we still keep only one source-of-truth per phase and decide by test coverage.

## Intellectual safety packet

H_true:
- Objective override data should be applied for pre-start objective display and phase transitions; custom phase dialogue should not be hidden by canonical line injection.
- source: `src/core/MissionController.cpp`
- gate: `ctest --test-dir build -C Release --output-on-failure -R "bs3d_core_tests|bs3d_game_support_tests"`

H_false:
- Mission start should always push canonical phase line before any data-driven dialogue and always ignore runtime overrides in objective HUD.
- source: `tests/core_tests.cpp`
- gate: `ctest --test-dir build -C Release --output-on-failure -R bs3d_game_support_tests`

Counter-claim: The safer alternative is not to suppress canonical lines at all; if this causes ordering drift with data-driven lines, missions become unreadable to content authors.

## Decision uncertainty handling

if a confidence score drops below 8/10:
- choose exactly one:
  - `A`: strict minimum
  - `B`: split into two validated steps
  - `C`: defer to next gate
- if two scores drop below 8/10: switch to RED and run no behavior edits.

## Scope and acceptance

- Scope:
  - `src/core/MissionController.cpp`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` passes.
  - `ctest --test-dir build -C Release --output-on-failure`
