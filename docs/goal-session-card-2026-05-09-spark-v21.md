# Goal Session Card - 2026-05-09 (Spark v21 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:  
  Add a high-signal regression test for the persisted rewir-pressure lifecycle: save/load, local-service relief, long-tail calm, and eventual expiry behavior must remain consistent.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor
  - [ ] Docs/procedure
  - [x] Tests
- Confidence (pre): scope clarity 9/10, data/model compatibility 10/10, backward compatibility 10/10, validation confidence 9/10
- Rollback action: remove this test function and revert this lock entry in `progress.md` if it introduces brittle assumptions; keep snapshot assertions unchanged otherwise.
- Evidence anchors:
  - source: `tests/core_tests.cpp`
  - gate: `ctest --test-dir build -C Release -R bs3d_core_tests --output-on-failure`
- Uncertainty:
  - If pressure scoring rules change (for example threshold or hotspot score formula), this test may need retuning in one place rather than broad behavior edits.

## Intellectual safety packet

H_true:
- Rewir pressure behavior is expected to remain stable when persisted: restored events can still trigger wary service state, can be softened by service relief, and naturally fade to inactivity after timer expiry.
- source: `tests/core_tests.cpp`
- gate: `ctest --test-dir build -C Release -R bs3d_core_tests --output-on-failure`

H_false:
- Without a persistence-aware regression, a save/load or relief refactor could silently break long-tail memory sequencing while preserving superficial test coverage in unrelated systems.
- source: `tests/core_tests.cpp`
- gate: `ctest --test-dir build -C Release -R bs3d_core_tests --output-on-failure`

Counter-claim: This is not only a serialization test; it verifies behavioral continuity of rewir consequences after restore, so it should fail on regressions even when string snapshots still look plausible.

## Decision uncertainty handling

if a confidence score drops below 8/10:
- choose exactly one:
  - `A`: strict minimum
  - `B`: split into two validated steps
  - `C`: defer to next major gate
- if two scores drop below 8/10: switch to RED and run no behavior edits.

## Scope and acceptance

- Scope:
  - `tests/core_tests.cpp`
  - `docs/goal-completion-audit-2026-05-09.md`
  - `docs/goal-session-card-2026-05-09-spark-v20.md`
  - `docs/goal-session-card-2026-05-09-spark-v21.md`
  - `progress.md`
- Acceptance:
  - `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
  - `ctest --test-dir build -C Release -R bs3d_core_tests --output-on-failure`

## Immediate no-go rule

- No runtime/editor code changes in this turn; tests-only edits must be limited to `tests/core_tests.cpp` and directly tied to rewir save-pressure-relief lifecycle behavior.
