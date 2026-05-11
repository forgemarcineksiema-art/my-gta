# Goal Session Card - 2026-05-09 (Spark v34 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal:
  Utrzymujemy spojnosc celu gry i decyzji przy modelu 5.3-Codex-Spark: zadna decyzja nie przechodzi bez source lock, gate, kontrfaktow i jednoznacznego rollbacku.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor tool
  - [x] docs/procedure
  - [ ] Tests
- Confidence (pre): 9/10, 8/10, 9/10, 9/10
- Uncertainty: czy kazde zachowanie systemowe w tym cyklu wymaga podwojnego gate (cel + implementacja), czy wystarczy jeden gate do fazy decyzji i jeden do fazy domkniecia?
- Rollback action: wycofac ten lock i przywrocic `docs/goal-session-card-2026-05-09-spark-v33.md` jako aktywny wpis w `progress.md`.
- Evidence anchors:
  - source: `docs/goal-alignment-shield-2026-05-09.md`
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`

H_true:
  source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  gate: `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
  source: `docs/goal-intellectual-safeguards-2026-05-09.md`
  gate: `ctest --test-dir build -C Release --output-on-failure`

Counter-claim: bez odswiezonego locka v34 latwo wejsc w drobne zmiany bez pelnego proofu, co przy niskiej mocy modelu zwieksza ryzyko dryftu w strone ogolnego prototypu.

Current choice: `A`

## Session constraints for Spark

- AMBER is the default for behavior-impact changes: do minimum scope and gate every step.
- This turn is decision/guarding only (docs/procedure + verify packet). No behavior edits are allowed.
- If any confidence score drops below 8/10 before implementation, stay in `AMBER` and execute only choice `A` or `B`.
- If two scores are below 8/10, switch immediately to `RED`.
- Every future behavior step must have:
  - exactly one owner layer,
  - one source anchor,
  - one gate anchor,
  - one explicit rollback tied to the same source/gate pair,
  - and one counter-claim.

## Branch table

`A`: Hardening-only pass (recommended for this session)
  - keep only docs/procedure updates,
  - run `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`,
  - append completion proof to `progress.md`.

`B`: Decision + behavior split
  - separate lock + evidence packet for decision slice,
  - then separate lock + evidence packet for behavior slice.

`C`: Defer
  - no edits, only collect stronger anchors/tests and re-enter with `A` or `B`.

## Session acceptance

- run `python tools\verify_goal_guard.py --model 5.3-Codex-Spark` before any behavior edits;
- keep all modifications in this pass in `docs/` only;
- update `progress.md`:
  - set active card reference to `docs/goal-session-card-2026-05-09-spark-v34.md`,
  - archive `docs/goal-session-card-2026-05-09-spark-v33.md`.
- if this lock is abandoned, restore previous lock by exact inverse edit in `progress.md`.
