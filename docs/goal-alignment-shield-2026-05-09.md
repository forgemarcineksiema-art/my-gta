# Goal Alignment & Quality Shield

This file is the active safeguard for iteration when model strength is lower (`5.3-Codex-Spark`) and we still want the project direction to stay stable.

## Model-transition hardening (mandatory on downgrade)

Whenever execution moves to `5.3-Codex-Spark` or resumes after a context interruption:

- start in `AMBER`,
- produce a new `goal-session-card-*spark*.md` in the same pass,
- run `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`,
- and only then allow behavior edits.

No behavior edit is allowed in the same turn that also redefines:
- the active objective slice,
- owner-layer selection,
- or objective-guard document set.

## Active one-line goal lock

Build a compressed Grochów-style micro-open-world centered on Blok 13 with readable controls/driving, gritty local social consequences, and a tool-first pipeline
in C++ (runtime authoring), Python (pipeline/validation), then C# (structured content tooling), while keeping a strong visual identity over speed-picking abstractions.

## Mandatory Weak-Model Safety Mode (new)

When running on lower-capacity model runs, we execute only one of these modes:

- `GREEN`: high-confidence continuation of an existing, already-approved path.
- `AMBER`: partial confidence; implement a strict minimum slice + gate.
- `RED`: low confidence; do not implement code, only restate options and propose a safer split.

Before every meaningful implementation step, set the mode explicitly:

1. `GREEN` requires no open critical risk and complete evidence mapping.
2. `AMBER` requires decision card with 2+ rollback points or a plan to add one before merge.
3. `RED` requires stopping implementation and returning a one-step refinement choice:
   - `A`: strict minimal implementation now
   - `B`: split work into two validated steps
   - `C`: defer until next major gate is green

Lower-capacity sessions are allowed to ask for "model-neutral arbitration" if ambiguity appears:

- keep existing working systems,
- avoid inventing new abstractions in one jump,
- prefer one conservative change.

## Spark 5.3 Hardening Contract (mandatory while this model is active)

This contract adds an extra stop-check for `5.3-Codex-Spark` runs:

1. Pre-flight lock  
   - write one-sentence goal effect, owner layer, and done criteria before edits,  
   - explicitly set `GREEN`/`AMBER`/`RED`, and  
   - define one rollback point.

2. Evidence pair  
   - each session must cite one source anchor (file/line or command output), and  
   - one gate anchor (test/validator or manual smoke).

3. No-go triggers  
   - if the goal effect or gate anchor is missing, stop and switch to `AMBER`.
   - if ambiguity is unresolved, choose only `A` and keep scope minimal.
   - if two no-go triggers exist at once, switch to `RED`.
   - if the intellectual safety packet is incomplete, require explicit docs from
     [goal-intellectual-safeguards-2026-05-09.md](goal-intellectual-safeguards-2026-05-09.md) first.

4. Execution clamp  
   - one subsystem only per pass,
   - no speculative subsystem jumps,
   - no multi-layer edits without declared split + merge gate.

5. Exit lock  
   - decision card present in `progress.md` or linked doc,
   - one evidence artifact attached,
   - one explicit rollback action present.
   - if any item is missing, keep the thread open until completed.

## Intelligence Safeguards for 5.3-Codex-Spark

For every session started after a model downgrade to 5.3-Spark, we use a fixed decision circuit:

1. **Source-lock first**  
   Before proposing changes, we must cite where we read the current truth:
   one canonical data source (`src`, `data`, `tests`) and one gate source (`validator` or `tests`).

2. **Dual-pass thought**  
   We run two passes before implementation:
   - *Pass 1:* minimal patch proposal.
   - *Pass 2:* risk/rollback check against the current goal and existing systems.

3. **Counter-claim lock**  
   For every change, list one likely failure mode and one direct rollback action.

4. **Drift lock**  
   Any decision that can be interpreted as moving toward "generic" or "blocky prototype" style is blocked unless it is tied to the visual identity plan with testable effect.

5. **Artifact lock**  
   Session cannot end if either of the following is missing:
   - one concrete evidence artifact,
   - one rollback point,
   - one explicit uncertainty flag.

6. **Intellectual lock**  
   Every behavior-affecting pass must complete the packet from
   [goal-intellectual-safeguards-2026-05-09.md](goal-intellectual-safeguards-2026-05-09.md):
   `H_true`/`H_false` with anchors, `Counter-claim`, rollback, and uncertainty.

7. **Consistency lock**  
   If two code paths must be changed for one behavior, update both at once or defer with a clear split plan.

8. **Hypothesis lock**  
   Before behavior changes, state `H_true` and `H_false` with `source:` and `gate:` anchors, then add a `Counter-claim` sentence against the rejected interpretation.

The loop can only close in GREEN once all required locks are explicitly documented in the decision card and reflected in `progress.md`.

## Decision-Loop Lock for Spark

No behavior change is accepted in `AMBER/RED` sessions unless:

1. The session card contains:
   - a locked one-sentence goal slice,
   - explicit source and gate anchors,
   - rollback action,
   - `Counter-claim` sentence.
2. The active decision has exactly one owner layer and one owner of risk.
3. At least one gate command is runnable in the next turn (or pre-existing) and is tied to that source anchor.
4. If confidence drops below `8/10` in any area, a single one-line A/B/C branch is chosen before action.

This requirement is enforced by `tools/verify_goal_guard.py` and surfaced in `progress.md`.

## Canonical Goal Summary

- `Blok 13: Rewir` is a compact but dense osiedlowy micro-open-world with a Rockstar-style structural discipline.
- Blok 13 remains the playable heart and quality gate; later expansion is ring-based (`Main Artery`, `Pavilions/Market`, `Garage Belt`).
- Core pillars are: readable movement/camera, gruz-like driving, Przypał, short World Memory, local satire/social pressure, and authored social-geographic consequences.
- Tooling stack order is fixed: C++ runtime editor first, Python pipeline/validators second, C# data editor later.
- Visual identity must read as dirty, lived-in Grochow, not as a generic toy/blocky prototype.
- Originality is composed from known game systems in a new local context, not via wholesale invention from scratch.

## Decision Filter (must pass before any significant implementation step)

1. **Vision Fit**
   - Does the change directly support the canonical pillars above?
   - If a feature is stylistically close to GTA/Uncharted/Mafia/Bully/etc., it must be grounded in the estate/Polish block context.
   - No `Roblox-like`, full-city, or non-local abstraction is acceptable if it weakens estate identity.

2. **System Coupling Fit**
   - Does the change belong to the correct tool layer?
     - Spatial/runtime adjustments -> C++ runtime editor path first.
     - Data schema/pipeline/validation -> Python pipeline path.
     - Structured mission/dialogue/cutscene editing -> C# editor path.
   - Reject single-file hacks that bypass existing loaders/validators.

3. **Signal-to-noise Fit**
   - One change should create one meaningful gameplay/readability win.
   - No speculative overbuild outside active bottleneck (e.g. large map sprawl before editor and gates are stable).
   - If uncertain, make a minimal reversible slice and validate via existing gates.

4. **Traceability and Rollback**
   - Every implemented change must have:
     - evidence line in changelog/progress entry,
     - at least one validator/gate or test path that can fail before merge,
     - a clear rollback point (or inverse action).
   - If no rollback point exists, delay or split the task.

5. **Model Reliability**
   - Run a quick confidence review (1-10) on:
     - scope clarity,
     - data/asset compatibility,
     - risk of breaking existing gates.
   - If score < 8 in any category, force mode to `AMBER`.
   - If 2+ categories are below 8, force mode `RED` and do not ship code changes.

## Lightweight Decision Card (use for each session)

- Why this now?  
- What part of the goal it advances.  
- Which layer owns it (C++/Python/C#).  
- Acceptance criteria and one concrete verification artifact (file, gate, or manual smoke).  
- Risks introduced + how to detect/catch them.  

If confidence is below `8/10`, ask one model-agnostic option choice:

- `A`: strict implementation now (smallest possible delta)
- `B`: split into 2 steps and validate halfway
- `C`: defer and implement after another gate reaches green

Use this companion artifact for each planned chunk: [Goal Decision Checklist Template](docs/goal-decision-checklist-template.md).

## Anti-Overreach Rules (to prevent hallucination-driven expansion)

- Any patch larger than one subsystem without a passing gate is not allowed.
- If an improvement is not in any of: C++ runtime/editor, Python validation, C# data tooling, mission/content, or visual polish, defer it.
- Never add a new content system before it has at least one documented reason in `Blok 13: Rewir` canon and one test/validator.
- Refrain from new mechanics that are "nice ideas" without player-readable consequence (`Przypał`, `World Memory`, or local NPC reaction).

## Session Exit Gate (mandatory)

Before ending a session, append:

- one decision card (or explicit non-action reasoning),
- one evidence file that shows gate status (test log, validator output, screenshot, or short notes with command output),
- one rollback step ready for immediate reversion.

If any of these three are missing, continue work on the same thread instead of ending on partial state.

For `5.3-Codex-Spark` sessions, run this preflight before starting implementation:

```powershell
python tools/verify_goal_guard.py --model 5.3-Codex-Spark
```

## Anti-Drift Rules (for weaker model runs)

- No creative jump unless grounded by:
  - an existing gameplay system or
  - an explicit, local narrative need in Blok 13.
- No `new idea` should replace an existing stable path if a known analog exists in current docs/specs.
- No "finalize-vibe-first" changes without a map of consequences (Przypał and/or Memory).
- No direct mention of direct real-brand adaptation in core canon.

### Spark decision floor

- `AMBER` is the default for any pass that can change behavior, assets, or file outputs.
- `RED` is required when:
  - source or gate evidence is missing,
  - two or more confidence scores are below 8/10,
  - or scope includes more than one tool layer without an explicit split plan.
- Under `RED`, only one of:
  - `A`: strict minimum patch
  - `B`: split into two validated steps
  - `C`: defer to next gate
- Under `GREEN`, still require:
  - one unchanged source anchor,
  - one passing gate anchor.

## What must always be checked first

- `README.md` (current implementation summary and active priorities),
- `docs/blok-ekipa-world-dna.md` (canon and non-goals),
- `docs/superpowers/specs/2026-05-09-runtime-editor-design.md` and
- `docs/superpowers/specs/2026-05-09-grochow-target-vision-design.md` (tooling and expansion path).

Then map changes into gates:
- object/mission affordances -> `tools/validate_object_outcomes.py`
- overlay edits -> `tools/validate_editor_overlay.py`
- assets/manifest -> `tools/validate_assets.py` and docs in `docs/asset-pipeline.md`
- core game/runtime behavior -> existing CTest suite and local smoke/manual checks.

---

This shield is not a process burden; it is a way to keep each step fast, useful, and aligned even when model capacity is reduced.
