# Goal Session Card - 2026-05-09 (Spark v30 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal slice:
  Keep one high-leverage thread active: Blok 13 must stay a readable, gritty, compressed Grochow district with a stable C++ -> Python -> C# toolchain and persistent local-consequence gameplay.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor tool
  - [x] docs/procedure
  - [ ] Tests
- Confidence (pre): 8/10, 8/10, 7/10, 8/10
- Rollback action: revert only docs/decision packet changes and restore the previous lock reference in `progress.md` before any behavior edits.
- Evidence anchors:
  - source: `docs/goal-completion-audit-2026-05-09.md`
  - gate: `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
- Uncertainty: which layer to prioritize next: runtime edits, Python validators, or C# mission-content tooling breadth?

H_true:
  source: `docs/goal-completion-audit-2026-05-09.md`
  gate: `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`

H_false:
  source: `docs/progress.md`
  gate: `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`

Counter-claim: jumping to the next feature without this lock is tempting, but it increases model drift and weakens our anti-identity guard.

## Decision guardrails for this pass

- Maintain one active owner layer and one subsystem only.
- No behavior edits in AMBER until source lock, packet, rollback action, and one gate are explicitly in place.
- Enforce anti-drift lock:
  - no move toward generic/toy/Roblox-like visuals without a local identity hook and a gate.
- Keep H_true/H_false and risk notes explicit before each significant behavior move.
- If uncertain, choose one branch and stop implementation:
  - `A`: strict minimum patch
  - `B`: split into two validated steps
  - `C`: defer

Current choice: `A`.

## Session acceptance

- Re-run `python tools\verify_goal_guard.py --model 5.3-Codex-Spark` before any behavior edits.
- Keep `docs/goal-completion-audit-2026-05-09.md`, `docs/goal-low-capacity-protocol-2026-05-09.md`, and `docs/goal-intellectual-safeguards-2026-05-09.md` aligned to this lock.
- Continue one-step reversible slices and stop if any anchor disappears or any gate fails.
