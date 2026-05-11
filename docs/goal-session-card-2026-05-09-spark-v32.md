# Goal Session Card - 2026-05-09 (Spark v32 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal:
  Add a single-pass Python mission validation slice for `data/mission_driving_errand.json` that enforces phase/trigger/cue integrity against runtime-supported phases and existing object-outcome/catalog contracts.
- Owner layer:
  - [ ] C++ runtime/editor
  - [x] Python pipeline/validator
  - [ ] C# data/editor tool
  - [ ] docs/procedure
  - [ ] Tests
- Confidence (pre): 9/10, 9/10, 8/10, 9/10
- Rollback action: revert `tools/validate_mission.py`, `tools/test_validate_mission.py`, and related `CMakeLists.txt` test registrations from this pass.
- Evidence anchors:
  - source: `src/game/WorldDataLoader.cpp` (`missionPhaseFromDataName`)
  - source: `tools/BlokTools.Core/MissionDocumentValidator.cs` (`mission.step.trigger.outcome`)
  - gate: `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty: should the validator require only known `steps` (legacy `phases` rejected) or accept both for compatibility this cycle?

H_true:
  source: `src/game/WorldDataLoader.cpp`
  gate: `python -m pytest tools/test_validate_mission.py`

H_false:
  source: `tools/BlokTools/BlokTools.Core/MissionDocumentValidator.cs`
  gate: `python tools\\verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: keeping mission JSON checks only in C# editor runtime is not enough for CI trust, because build/release automation can bypass editor-only workflows.

Current choice: `A`.

## Decision guardrails for this pass

- Keep strict one owner layer: only Python pipeline/validator and related test wiring.
- Keep one subsystem only: mission content authoring data integrity.
- AMBER branch logic (if uncertainty grows):
  - `A`: strict minimum pass (single validator + unit tests + CTest registration)
  - `B`: split into validator + separate gate docs/tests update pass
  - `C`: defer mission validation and continue with a different owner layer

## Session acceptance

- Run `python tools/verify_goal_guard.py --model 5.3-Codex-Spark` before any behavior edits.
- Keep changes scoped to `tools/` and `CMakeLists.txt` only in this pass.
- Update `progress.md` with the active lock and run:
  - `python tools/validate_mission.py data/mission_driving_errand.json --mission-localization data/world/mission_localization_pl.json --outcome-catalog data/world/object_outcome_catalog.json --runtime-policy src/game/WorldDataLoader.cpp`
  - `python -m pytest tools/test_validate_mission.py`
