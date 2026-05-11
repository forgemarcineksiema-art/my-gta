# Goal Session Card - 2026-05-09 (Spark v13 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  We implement one reversible C++ runtime-scripting slice: mission phase data (`dialogue`, `npcReactions`, `cutscenes`) is loaded into `MissionController` and queued on phase transitions with clear owner-layer separation.
- Owner layer:
  - [ ] Docs/procedure
  - [x] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor
- Confidence (pre) - scope clarity 9/10, data compatibility 8/10, backward compatibility 9/10, validation confidence 9/10
- Rollback action: remove the C++ slice by restoring `src/core/MissionController.cpp` and `src/game/WorldDataLoader.cpp` to the previous git-freeze behavior and revert `tests/core_tests.cpp` + `tests/game_support_tests.cpp` edits.
- Evidence anchors:
  - source: `src/core/MissionController.cpp`, `src/game/WorldDataLoader.cpp`, `tests/core_tests.cpp`, `tests/game_support_tests.cpp`
  - gate: `ctest --test-dir build -C Release -R bs3d_core --output-on-failure` and
    `ctest --test-dir build -C Release -R game_support --output-on-failure`
- Uncertainty:
  - if line source/speaker resolution drifts from `data/world/mission_localization_pl.json` expectations, mission dialogue identity may regress during phase transitions.

## Intellectual safety packet (mandatory for Spark behavior pass)

H_true:
- The safe path is to make only one bounded C++ owner-layer patch: phase-line setters in `MissionController`, application wiring in `WorldDataLoader`, and direct transition-driven queue checks in tests.
- source: `docs/goal-low-capacity-protocol-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
- The unsafe path is to edit C++ mission behavior while simultaneously changing Python/editor tooling or mission data shape, increasing integration risk and making rollback harder.
- source: `docs/goal-alignment-shield-2026-05-09.md`
- gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: Assuming "slice-safe C++ changes" while implicitly changing data-layer behavior is harmless is wrong; without this pass boundary the same pass would blur responsibilities and raise risk in a reduced-capacity run.

## Decision uncertainty handling

- if any confidence category drops below 8/10:
  - choose one:
    - `A`: strict minimum patch
    - `B`: split into two validated steps
    - `C`: defer to next gate
- if two categories drop below 8/10:
  - switch to `RED`, no behavior edits.

## Scope and acceptance

- Scope:
  - `src/core/MissionController.h/.cpp`
  - `src/game/WorldDataLoader.cpp/.h`
  - `tests/core_tests.cpp`
  - `tests/game_support_tests.cpp`
  - `docs/goal-session-card-2026-05-09-spark-v13.md`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` still passes.
  - `progress.md` references this card.
  - one rollback action remains immediate.
  - one explicit uncertainty remains documented.

## Decision output if uncertainty persists

`A`: strict minimum C++ data-wiring only (setters + queue on transition), defer test-assertion expansion until second gate.
