# Goal Session Card - 2026-05-09 (Spark v33 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal:
  W sesji 5.3-Codex-Spark utrzymujemy stały, zweryfikowany pakiet zabezpieczeń intelektualnych, żeby żadna decyzja nie ominęła ścieżki anchorów, kontrfaktów i rollbacku.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor tool
  - [x] docs/procedure
  - [ ] Tests
- Confidence (pre): 9/10, 8/10, 8/10, 9/10
- Rollback action: cofnąć ten lock i przywrócić `docs/goal-session-card-2026-05-09-spark-v32.md` jako aktywny wpis w `progress.md`.
- Evidence anchors:
  - source: `docs/goal-alignment-shield-2026-05-09.md`
  - source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  - gate: `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty: czy zaostrzamy regułę `Source-lock` do poziomu `RED` przy każdej decyzji dotykającej tożsamość wizualną, czy zostawić `AMBER` + explicit roll-forward rollback?

H_true:
  source: `docs/goal-low-capacity-protocol-2026-05-09.md`
  gate: `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
  source: `docs/goal-intellectual-safeguards-2026-05-09.md`
  gate: `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: bez odświeżenia locka na 5.3-Spark tylko przez dokument, kolejne decyzje mogą się rozlać na wiele warstw i stracić ślad rollbacku, więc "wciąż działało u mnie" nie wystarczy.

Current choice: `A`.

## Decision guardrails for this pass

- Keep strict one-owner layer rule for Spark: this pass is `docs/procedure` only.
- Force a fresh evidence packet before any behavior edit:
  1) current one-sentence objective, 2) H_true/H_false with anchors, 3) Counter-claim, 4) rollback action.
- Apply anti-drift hard-stop:
  any tożsamość wizualną/prototypową decyzję musi mieć źródło w `docs/superpowers/specs/2026-05-09-grochow-target-vision-design.md`
  albo odłożyć decyzję do trybu `RED` + branch `C`.
- Confidence floor:
  if any score falls below `8/10`, switch to `RED` immediately and execute exactly one branch from the table below.

AMBER branch logic:
- `A`: hardening-only pass (docs/procedure + verify packet) with no behavior edits.
- `B`: split into "decision packet" then "behavior slice", each with separate lock and guard.
- `C`: pause behavior and defer to next owner-layer gate.

## Session acceptance

- Run `python tools\verify_goal_guard.py --model 5.3-Codex-Spark` before any behavior edits.
- Keep all changes in this pass limited to `docs/` files.
- Update `progress.md`:
  - set active card reference to `docs/goal-session-card-2026-05-09-spark-v33.md`.
  - archive `docs/goal-session-card-2026-05-09-spark-v32.md`.
- Keep decision logs in this pass visibly tied to identity-guard and mission/data safety artifacts.
