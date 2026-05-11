# Goal Session Card - 2026-05-09 (Spark v35 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal:
  Utrzymujemy tożsamość Grochowa (Blok 13 + mikro-otwarty styl) oraz kolejność realizacji narzędzi (C++ runtime editor -> Python pipeline -> C# data editor) przy ścisłym dowodzeniu każdej decyzji przez anchors i rollback.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor tool
  - [x] docs/procedure
  - [ ] tests
- Confidence (pre): 8/10, 8/10, 9/10, 8/10
- Uncertainty: czy jednoczesne porządkowanie celów wizualnych i runtime-editora nie powinno zostać rozdzielone na A/B, czy da się je traktować jako jeden sprawdzalny pass?
- Rollback action: wycofaj ten lock i przywrocic [docs/goal-session-card-2026-05-09-spark-v34.md](docs/goal-session-card-2026-05-09-spark-v34.md) jako aktywny wpis w `progress.md`.
- Evidence anchors:
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - source: `docs/goal-alignment-shield-2026-05-09.md`
  - gate: `python tools\\verify_goal_guard.py --model 5.3-Codex-Spark`

H_true:
  source: `docs/goal-alignment-shield-2026-05-09.md`
  gate: `python tools\\verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
  source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  gate: `python tools\\verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: jeśli ten lock miałby być jedynie formalnością, to ryzykujemy skok do „ładnej próbnki” zamiast lokalnie-spójnego, konsekwentnego buildu Bloku 13.

Current choice: `A`

## Cognitive safety constraints for this model

- this is a hardening-only pass unless a new lock exists for behavior and evidence:
  - docs/procedure-first,
  - no code/data/asset changes in this pass,
  - one decision snapshot in `progress.md`,
  - one executable gate run in the same turn.
- all future behavior edits under this lock must include:
  - one owner layer,
  - one locked objective clause,
  - one source anchor,
  - one gate anchor,
  - one explicit rollback,
  - one explicit counter-factual pair.
- if mode stays AMBER, only choices `A` or `B` are allowed before any behavior step.
- if any confidence score drops below 8/10, force RED and stop implementation.
- if two or more scores are below 8/10, stay RED and force defer.

Branch table:

`A`: Hardening-only pass (recommended)
- keep only docs/procedure hardening,
- run goal guard,
- archive old lock in `progress.md` and activate this lock.

`B`: Harden + split gate (only if owner changes)
- split into two single-owner locks,
- run separate gates and explicit pre/post proof each.

`C`: Defer behavior
- collect two new anchors (one code/data source + one gate) and re-run.

## Session constraints for Spark

- AMBER is default for any pass that can affect behavior.
- Do not touch >1 owner layer in one behavior step.
- On this pass, the only permitted touched layer is `docs/procedure`.
- Any behavior slice now requires:
  - `python tools\\verify_goal_guard.py --model 5.3-Codex-Spark` before edits,
  - one `ctest` or `pytest`/validator gate after edits when possible.

## Session acceptance

- run `python tools\\verify_goal_guard.py --model 5.3-Codex-Spark` before end of turn;
- update `progress.md` to:
  - set active lock reference to `[docs/goal-session-card-2026-05-09-spark-v35.md](docs/goal-session-card-2026-05-09-spark-v35.md)`,
  - archive `[docs/goal-session-card-2026-05-09-spark-v34.md](docs/goal-session-card-2026-05-09-spark-v34.md)`.
- keep full rollback path documented in this file.
