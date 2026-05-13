Original prompt: v0.13 Visual Readability + Gameplay HUD pass: clean gameplay/debug HUD, hide long controls, compact objective, restore Polish text/font support, driver pose, decal cleanup, marker/route readability, and camera readability near buildings.

## 2026-05-09

- 5.3-Codex-Spark hardening continuation:
  - mission content parity continuation v36 (C# editor/content proof):
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v36.md](docs/goal-session-card-2026-05-09-spark-v36.md)
    - scope: add baseline `npcReactions` + `cutscenes` to production mission fixture and matching localization keys, then align C# snapshot tests to confirm editor/runtime parity.
    - verified preflight in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
      `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v35.md](docs/goal-session-card-2026-05-09-spark-v35.md)

- 5.3-Codex-Spark hardening continuation:
  - model-switch safeguard v35 (Spark session safety hardening):
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v35.md](docs/goal-session-card-2026-05-09-spark-v35.md)
    - scope: enforce cognitive hardening at the weakest-model layer boundary (docs/procedure only), re-affirm one-owner lock discipline, and lock the no-behavior policy until explicit fresh source+gate evidence exists.
    - verified preflight in Spark pass:
      `python tools\\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v34.md](docs/goal-session-card-2026-05-09-spark-v34.md)

- 5.3-Codex-Spark hardening continuation:
  - model-switch safeguard v34 (Spark session safety hardening):
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v34.md](docs/goal-session-card-2026-05-09-spark-v34.md)
    - scope: hard restart the goal safety loop with explicit evidence pair requirement and branch lock (`A/B/C`) before any behavior edits.
    - verified preflight in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
      [docs/goal-session-card-2026-05-09-spark-v33.md](docs/goal-session-card-2026-05-09-spark-v33.md)

- 5.3-Codex-Spark hardening continuation:
  - model-switch intellectual safeguard v33 (Spark session safety hardening):
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v33.md](docs/goal-session-card-2026-05-09-spark-v33.md)
    - scope: reset the active Spark decision contract to one-owner procedural mode before any behavior edits; explicit anti-drift + rollback requirements.
    - verified preflight in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v32.md](docs/goal-session-card-2026-05-09-spark-v32.md)

- 5.3-Codex-Spark hardening continuation:
  - mission data reliability v32 (Python mission validator slice):
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v32.md](docs/goal-session-card-2026-05-09-spark-v32.md)
    - scope: add Python-side mission schema and catalog consistency validation, plus CTest wiring.
    - verified preflight in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v31.md](docs/goal-session-card-2026-05-09-spark-v31.md)

- 5.3-Codex-Spark hardening continuation:
  - decision/inference hardening v31 (model-switch lock refresh for lower-capacity pass):
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v31.md](docs/goal-session-card-2026-05-09-spark-v31.md)
    - scope: one-pass docs/procedure safeguard refresh and decision protocol reaffirmation; no behavior edits in this pass.
    - verified preflight in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v30.md](docs/goal-session-card-2026-05-09-spark-v30.md)

- 5.3-Codex-Spark hardening continuation:
  - vision and decision stabilization v30 (goal lock refresh for model downgrade):
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v30.md](docs/goal-session-card-2026-05-09-spark-v30.md)
    - scope: decision/guard refresh and sequencing rules for all future Spark passes; no behavior edits in this turn.
    - verified preflight in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v29.md](docs/goal-session-card-2026-05-09-spark-v29.md)

- 5.3-Codex-Spark hardening continuation:
  - phase-safety continuation v29 (editor runtime data roundtrip):
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v29.md](docs/goal-session-card-2026-05-09-spark-v29.md)
    - scope finalized: C# mission-editor dialogue phase roundtrip and fixture phase annotation.
    - verified lock preflight:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v28.md](docs/goal-session-card-2026-05-09-spark-v28.md)

- 5.3-Codex-Spark hardening continuation:
  - cognitive safety hardening v28 (model-switch re-lock):
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v28.md](docs/goal-session-card-2026-05-09-spark-v28.md)
    - scope finalized: docs/procedure hardening only; no behavior edits until this lock is complete.
    - verified preflight in Spark pass:
      `python tools\\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v27.md](docs/goal-session-card-2026-05-09-spark-v27.md)

- 5.3-Codex-Spark hardening continuation:
  - spark-specific cognitive hardening v26 (model-switch lock refresh):
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v26.md](docs/goal-session-card-2026-05-09-spark-v26.md)
    - scope finalized: pre-behavior protocol sanity, no gameplay/code edits in this pass.
    - verified preflight in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v25.md](docs/goal-session-card-2026-05-09-spark-v25.md)

- 5.3-Codex-Spark hardening continuation:
  - cognitive safety hardening v25:
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v25.md](docs/goal-session-card-2026-05-09-spark-v25.md)
    - scope finalized on C# editor consistency and shop catalog read/write readiness.
    - verified in Spark pass:
      `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
      `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj`
      `dotnet build tools\BlokTools\BlokTools.App\BlokTools.App.csproj`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v24.md](docs/goal-session-card-2026-05-09-spark-v24.md)
  - cognitive safety hardening v24:
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v24.md](docs/goal-session-card-2026-05-09-spark-v24.md)
    - scope shifted to C# structured content editor growth (shop/item catalog surface), while remaining one-owner and Spark-safe.
    - verified in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v23.md](docs/goal-session-card-2026-05-09-spark-v23.md)
  - cognitive safety hardening v23:
    - scope tightened for model-drop sessions: one-owner C++ runtime/editor only, explicit source+gate evidence, and identity-drift lock.
    - verified in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v22.md](docs/goal-session-card-2026-05-09-spark-v22.md)
  - cognitive safety hardening v22:
    - added explicit anti-drift lock for model-switch passes where the primary goal is to prevent identity/architecture drift during weaker-model loops; no behavior edits are allowed before this packet is complete.
    - verified in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v21.md](docs/goal-session-card-2026-05-09-spark-v21.md)

- 5.3-Codex-Spark hardening continuation:
  - model downgrade hardening v21:
    - active lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v21.md](docs/goal-session-card-2026-05-09-spark-v21.md)
    - test-focused slice added for rewir pressure persistence through save/load, relief, long-tail calm transition, and expiry.
    - verified in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
    - evidence:
      - `cmake --build build --config Release`
      - `ctest --test-dir build -C Release -R bs3d_core_tests --output-on-failure`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v20.md](docs/goal-session-card-2026-05-09-spark-v20.md)

- 5.3-Codex-Spark hardening continuation:
  - model downgrade hardening v20:
    - active loop lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v20.md](docs/goal-session-card-2026-05-09-spark-v20.md)
    - formalized a Spark restart lock for this model-switch moment: docs-only intake first, explicit source/goal lock + rollback, AMBER posture by default, and no behavior edits before preflight.
    - verified in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v19.md](docs/goal-session-card-2026-05-09-spark-v19.md)
      and earlier.

- 5.3-Codex-Spark hardening continuation:
  - model downgrade hardening v18:
    - active loop lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v18.md](docs/goal-session-card-2026-05-09-spark-v18.md)
    - converted the model switch into an explicit intellectual lock refresh pass: protocol + shield + checklist + verify loop are now the mandatory entry condition for any Spark behavior pass.
    - verified in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v17.md](docs/goal-session-card-2026-05-09-spark-v17.md)
      and earlier.
- 5.3-Codex-Spark hardening continuation:
  - model downgrade hardening v16:
    - active loop lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v16.md](docs/goal-session-card-2026-05-09-spark-v16.md)
    - added a fresh model-switch lock for 5.3-Codex-Spark: one-owner lock, explicit counterfactual packet, and irreversible-change veto unless gate-backed.
    - added a stricter requirement that cross-layer intent must be converted into an owner-specific lock before behavior edits.
    - verified preflight:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v15.md](docs/goal-session-card-2026-05-09-spark-v15.md)
      and earlier.
- 5.3-Codex-Spark hardening continuation:
  - model downgrade hardening v17:
    - active loop lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v17.md](docs/goal-session-card-2026-05-09-spark-v17.md)
    - changed mission objective/phase-line precedence to keep data-authored phase dialogue above canonical phase filler and make pre-start objective override visible.
    - verified in Spark pass:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
      `ctest --test-dir build -C Release --output-on-failure`
      `ctest --test-dir build -C Release --output-on-failure -R "bs3d_core_tests|bs3d_game_support_tests"`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v16.md](docs/goal-session-card-2026-05-09-spark-v16.md)
      and earlier.
- 5.3-Codex-Spark hardening continuation:
  - model downgrade hardening v14:
    - active loop lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v14.md](docs/goal-session-card-2026-05-09-spark-v14.md)
    - added a dedicated intellectual safety pass for model downgrade continuation with explicit A/B/C handling and ambiguity lockback.
    - preserved the existing active behavior scope as a one-owner C++/runtime-data guardrail.
    - verified preflight:
      `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v13.md](docs/goal-session-card-2026-05-09-spark-v13.md)
      and earlier.
- 5.3-Codex-Spark hardening continuation:
  - model downgrade hardening v13:
    - active loop lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v13.md](docs/goal-session-card-2026-05-09-spark-v13.md)
    - narrowed this pass to one-owner C++ mission-data behavior wiring:
      phase-linked dialogue/npc reaction/cutscene line storage + phase-transition queue.
    - verified preflight:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v12.md](docs/goal-session-card-2026-05-09-spark-v12.md)
      and earlier.
- 5.3-Codex-Spark hardening continuation:
  - model downgrade hardening v12:
    - active loop lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v12.md](docs/goal-session-card-2026-05-09-spark-v12.md)
    - added a stricter Spark behavior-pass requirement: mandatory counterfactual packet + one-owner-layer only + immediate rollback path + explicit identity-drift lock before any multi-system pass.
    - verified preflight:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v11.md](docs/goal-session-card-2026-05-09-spark-v11.md)
      and earlier.
- 5.3-Spark hardening continuation:
  - model downgrade hardening v11:
    - active loop lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v11.md](docs/goal-session-card-2026-05-09-spark-v11.md)
    - objective-pass rule reinforced: no behavior edits without completed intellectual packet + one-owner layer + A/B/C gating on confidence dips.
    - verified preflight:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - archived previous lock:
    [docs/goal-session-card-2026-05-09-spark-v10.md](docs/goal-session-card-2026-05-09-spark-v10.md)
      and earlier.
- 5.3-Spark downgrade hardening continuation:
  - model downgrade hardening v10:
    - active loop lock moved to:
      [docs/goal-session-card-2026-05-09-spark-v10.md](docs/goal-session-card-2026-05-09-spark-v10.md)
    - tightened objective-pass rule: one-owner layer only + required intellectual packet on every Spark pass.
    - verified preflight:
      `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`
  - added the dedicated intellectual safeguard addendum:
    [docs/goal-intellectual-safeguards-2026-05-09.md](docs/goal-intellectual-safeguards-2026-05-09.md)
  - updated `tools/verify_goal_guard.py` to require that safeguard document and packet-style locks before Spark sessions are considered valid.
  - archived previous active lock:
    [docs/goal-session-card-2026-05-09-spark-v9.md](docs/goal-session-card-2026-05-09-spark-v9.md)
- Model downgrade hardening pass for 5.3-Codex-Spark:
  - introduced stronger counterfactual lock (`H_true`/`H_false`) and single-owner enforcement in docs plus goal-guard script:
    [docs/goal-session-card-2026-05-09-spark-v8.md](docs/goal-session-card-2026-05-09-spark-v8.md)
  - moved the active lock reference to:
    [docs/goal-session-card-2026-05-09-spark-v8.md](docs/goal-session-card-2026-05-09-spark-v8.md)
- Model downgrade hardening pass for 5.3-Codex-Spark:
  - added a current spark session lock card:
    [docs/goal-session-card-2026-05-09-spark-v3.md](docs/goal-session-card-2026-05-09-spark-v3.md)
    (AMBER, explicit rollback, and one-pass acceptance gates).
  - re-affirmed that weak-model edits require explicit evidence + gate sequence before implementation:
    `python tools/verify_goal_guard.py --model 5.3-Codex-Spark`.
  - added Spark v4 governance lock with stricter subsystem and rollback rules:
    [docs/goal-session-card-2026-05-09-spark-v4.md](docs/goal-session-card-2026-05-09-spark-v4.md)
  - moved the active run card to:
    [docs/goal-session-card-2026-05-09-spark-v5.md](docs/goal-session-card-2026-05-09-spark-v5.md)
  - model downgrade stabilization pass for Spark (5.3-Codex-Spark) now logged as:
    [docs/goal-session-card-2026-05-09-spark-v6.md](docs/goal-session-card-2026-05-09-spark-v6.md)
  - strengthened Spark cognitive safety lock after latest downgrade request:
    [docs/goal-session-card-2026-05-09-spark-v7.md](docs/goal-session-card-2026-05-09-spark-v7.md)
- Mission editor C# data surface expansion:
  - added `npcReactions` and `cutscenes` editing flow in BlokTools editor (model, session, UI bindings, save path),
  - added validation for phase/speaker/text/duration in both new mission sections,
  - added tests for editor/session/validator/cross-loader coverage:
    `MissionDocumentValidationAllowsNpcReactionLineKey`,
    `MissionDocumentValidationAllowsCutsceneLineKey`,
    `MissionEditorSessionAddsNpcReactionAndCutscene`,
    plus snapshot surface assertions.
  - verified:
    - `dotnet run --project tools/BlokTools/BlokTools.Tests/BlokTools.Tests.csproj`
    - `dotnet build tools/BlokTools/BlokTools.App/BlokTools.App.csproj`
- Continued the Spark-hardening pass for low-capacity AI runs:
  - finalized explicit bootstrap, scoring, anchor, and hard-stop rules in
    [docs/goal-low-capacity-protocol-2026-05-09.md](docs/goal-low-capacity-protocol-2026-05-09.md).
  - added a session-specific decision card at
    [docs/goal-session-card-2026-05-09-spark-v8.md](docs/goal-session-card-2026-05-09-spark-v8.md),
    including rollback action, uncertainty, and acceptance criteria.
  - verified no runtime code touched in this step by checking the modified scope in docs only, then kept changes reversible.
  - spot-checked protocol gates:
    - `python tools\validate_editor_overlay.py data\world\block13_editor_overlay.json --asset-root data\assets`
    - `python tools\validate_object_outcomes.py data\world\object_outcome_catalog.json --runtime-policy src\game\WorldObjectInteraction.cpp`
    - `python -m pytest tools\test_validate_object_outcomes.py`
    - `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj`
- Tightened Spark safeguards after the active model downgrade to 5.3-Codex-Spark:
  - added a mandatory hardening contract section in
    [docs/goal-alignment-shield-2026-05-09.md](docs/goal-alignment-shield-2026-05-09.md),
    - added Spark-confidence triggers + AMBER/RED behavior in
    [docs/goal-low-capacity-protocol-2026-05-09.md](docs/goal-low-capacity-protocol-2026-05-09.md),
  - documented decision-card continuity requirement for ending any Spark session.
- Added goal safety guardrails for weaker model sessions: GREEN/AMBER/RED mode + confidence checks in
  [docs/goal-alignment-shield-2026-05-09.md](docs/goal-alignment-shield-2026-05-09.md), session pre-checks in
  [docs/goal-decision-checklist-template.md](docs/goal-decision-checklist-template.md), and a dedicated low-capacity protocol in
  [docs/goal-low-capacity-protocol-2026-05-09.md](docs/goal-low-capacity-protocol-2026-05-09.md).
- Tightened Spark-safe decisioning (5.3 mode) with Source-lock, Dual-pass, Drift-lock, Counter-claim lock, and explicit rollback/evidence requirements:
  [docs/goal-alignment-shield-2026-05-09.md](docs/goal-alignment-shield-2026-05-09.md),
  [docs/goal-low-capacity-protocol-2026-05-09.md](docs/goal-low-capacity-protocol-2026-05-09.md),
  [docs/goal-decision-checklist-template.md](docs/goal-decision-checklist-template.md).
- Added another low-capacity hardening pass for Spark:
  - one-line source-lock and counter-claim requirement in
    [docs/goal-low-capacity-protocol-2026-05-09.md](docs/goal-low-capacity-protocol-2026-05-09.md),
  - explicit anti-hallucination block with required contradiction check before behavior changes.
- Logged dedicated Spark hardening session card:
  [docs/goal-session-card-2026-05-09-spark-v2.md](docs/goal-session-card-2026-05-09-spark-v2.md).
- Added a machine-checkable goal guard preflight for Spark:
  - new [tools/verify_goal_guard.py](tools/verify_goal_guard.py),
  - optional Spark preflight flag in [tools/ci_verify.ps1](tools/ci_verify.ps1) (`-VerifyGoalLock`),
  - and explicit references in [docs/goal-low-capacity-protocol-2026-05-09.md](docs/goal-low-capacity-protocol-2026-05-09.md),
    [docs/goal-alignment-shield-2026-05-09.md](docs/goal-alignment-shield-2026-05-09.md),
    and [README.md](README.md).
- Verified the new guard currently passes:
  - `python tools\verify_goal_guard.py --model 5.3-Codex-Spark`.
- Repaired a Spark-related regression introduced during test hardening:
  - fixed escaped-quote corruption in `tests/game_support_tests.cpp::worldDataLoaderParsesJsonSchemaWithoutFieldOrderCoupling`,
  - rebuilt `Release` test artifacts,
  - revalidated with:
    - `cmake --build build --config Release`
    - `ctest --test-dir build -C Release -R game_support --output-on-failure`
  - `python tools\verify_goal_guard.py --model 5.3-Codex-Spark` still passes.

- Expanded readable object affordance coverage on base map clutter:
  `sign_no_parking` now exposes `parking_sign_read_sign_no_parking`,
  dressing now tags it as `parking_sign`, and `data/world/object_outcome_catalog.json` carries a matching no-noise catalog entry.
  Added support-test coverage for affordance presence, stable outcome id, and quiet behavior.
- Verified after rebuild:
  - `cmake --build build --config Release`
  - `python tools\validate_object_outcomes.py data\world\object_outcome_catalog.json --runtime-policy src\game\WorldObjectInteraction.cpp`
  - `python -m pytest tools\test_validate_object_outcomes.py`
  - `ctest --test-dir build -C Release -R game_support --output-on-failure`

- Started C# localization groundwork for mission dialogue in the editor:
  - added `lineKey` support to mission dialogue lines in `tools/BlokTools/BlokTools.Core/MissionDocument.cs`,
  - added schema fallback validation (`text` OR `lineKey`) in `tools/BlokTools/BlokTools.Core/MissionDocumentValidator.cs`,
  - added `LineKey` editing and persistence in `tools/BlokTools/BlokTools.App/MainWindow.xaml` and `tools/BlokTools/BlokTools.App/MainWindow.xaml.cs`,
  - added `MissionDocumentValidationAllowsDialogueLineKey` and snapshot resiliency tests in `tools/BlokTools/BlokTools.Tests/Program.cs`.
  - verified:
    - `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj`
    - `dotnet build tools\BlokTools\BlokTools.App\BlokTools.App.csproj`

- Closed the runtime side of the first localization-key slice:
  - `src/game/WorldDataLoader.h` and `src/game/WorldDataLoader.cpp` now load `data/world/mission_localization_pl.json`,
    add `lineKey` to `MissionDialogueData`, and resolve text from `missionLocalization` when `text/line` is absent.
  - `tests/game_support_tests.cpp` now verifies `lineKey` parsing and localized text resolution in
    `worldDataLoaderParsesJsonSchemaWithoutFieldOrderCoupling`.
  - validation/gates run:
    - `cmake --build build --config Release`
    - `ctest --test-dir build -C Release -R game_support --output-on-failure`
    - `ctest --test-dir build -C Release -R bs3d_core --output-on-failure`
    - `python tools\validate_editor_overlay.py data\world\block13_editor_overlay.json --asset-root data\assets`
    - `python -m pytest tools\test_validate_object_outcomes.py`

- Reprioritized the goal loop around the "not Roblox" visual identity feedback: added a first `Visual Identity V2` pass with non-toy character silhouette details, gruz rust/dirt/primer/scuff overlays in both runtime spec and generated glTF, and denser Zenon/Block 13 hero facade dressing with muted decal alpha tests.
- Connected that visual-identity pass to gameplay hooks: Zenon's rolling grille, shop price cards, and the Block 13 intercom now expose object affordances and catalogued outcome ids/patterns for mission authoring, while tests prevent shop grille art from being mistaken for garage doors.
- Fed selected object affordances into Przypal/world memory: noisy interactions such as buzzing the Block 13 intercom, disturbing trash, or testing Zenon's rolling grille now emit cooldown-limited `PublicNuisance` events, while quiet satire affordances like reading shop prices remain non-eventful.
- Exposed those object-affordance consequences in the C# data surface: `object_outcome_catalog.json` now carries optional `worldEvent` metadata, BlokTools validates event type/intensity/cooldown, and snapshot/editor rows show whether a hook is quiet or feeds Przypal/world memory.
- Added a Python data/runtime consistency gate for object outcomes: `tools/validate_object_outcomes.py` compares catalogued `worldEvent` metadata with `WorldObjectInteraction.cpp` policy, has unit coverage for drift/bad metadata, and is now part of CTest.
- Extended the object-outcome validator to cover quiet runtime affordance outcomes too: it now extracts `worldObjectInteractionAffordance` outcome ids/patterns and fails if C++ exposes an object prompt that `object_outcome_catalog.json` and BlokTools cannot see.
- Made the object-outcome consistency gate bidirectional for quiet hooks: stale catalog outcomes now fail validation if no runtime affordance or event policy can ever emit them, preventing BlokTools from offering mission triggers that cannot fire in-game.
- Required player-facing text for object outcomes across Python and BlokTools validation: every catalog hook now needs `speaker` and `line`, matching the runtime data-enrichment path and preventing empty interaction feedback from entering mission/dialog authoring.
- Moved runtime object-affordance event selection toward data-driven behavior: `WorldDataLoader` now loads `object_outcome_catalog.json` event metadata into `WorldDataCatalog`, and `GameApp` resolves noisy affordance events from that catalog before falling back to the built-in policy.
- Moved runtime object-affordance feedback text into the same data path: `WorldDataLoader` now preserves catalogued `speaker`/`line`, and `GameApp` enriches fallback C++ affordances from `object_outcome_catalog.json` before showing player feedback or resolving mission outcomes.
- Added dedicated visual QA baselines for the v2 art pass: `vp_shop_zenon_v2`, `vp_gruz_wear_v2`, and `vp_block13_front_v2` frame the exact storefront, worn gruz, and intercom/front-facade details that need repeated "not Roblox" inspection.
- Added lived-in micro breakup to the visual identity pass: Zenon's storefront now has a worn threshold, handwritten notice, and window stickers, while Block 13 gets entry notices, a facade repair patch, and curtain color behind windows. Support tests require these details to stay small, decorative, and surface-breaking rather than becoming blocky props.
- Turned selected lived-in microdetails into quiet playable affordances: the handwritten Zenon notice and Block 13 entry notice now expose stable `shop_notice_read_*` / `block_notice_read_*` outcomes, appear in BlokTools as quiet hooks, and stay out of Przypal/world-memory event policy.
- Shifted the goal loop back toward map and driving scale: `IntroLevelData` now carries a compressed-Grochow district skeleton with Blok 13 as the playable home base plus authored future Main Artery, Pavilions/Market, and Garage Belt rewirs, each with a role, footprint, route approach, and gameplay jobs for later expansion.
- Added district expansion route plans from Blok 13 into each authored future rewir, including a gruz-driving spine to the Main Artery plus branches toward Pavilions/Market and the Garage Belt. Tests now require stable route ids, readable waypoint labels, meaningful drive radii, and endpoints inside the target rewir footprints.
- Added a district-plan debug overlay path: `buildDistrictPlanDebugOverlay()` derives QA markers and route segments from the authored Grochow rewir/route data, and the in-game debug view now draws future rewir footprints plus gruz-route branches so map-scale work is inspectable during playtests.
- Materialized the first Main Artery slice as a real driveable expansion: added asphalt under the authored route endpoint, opened the south boundary into a route gate while keeping boundary blockers on both sides, and exported a Main Artery zone/landmark/anchor so memory, navigation, and debug tooling can target it.
- Added readable in-world route guidance for the Main Artery branch: three no-collision arrow-head markers now cover the bend, gate, and destination beats, with tests requiring `main_artery_guidance`/`future_expansion` tags and route-point coverage.
- Added gruz traversal QA for district routes: `inspectDistrictRouteVehicleTraversal()` now simulates vehicle-capsule progress through authored waypoint radii against world and prop collision, and the Main Artery branch is tested as actually traversable rather than only planned on the debug map.
- Captured the longer-term target in `docs/superpowers/specs/2026-05-09-grochow-target-vision-design.md`: a Rockstar-style compressed Grochow-scale district, with Blok 13 preserved as the heart, current quality gate, and first reusable rewir cell. Linked it from `README.md`, `docs/blok-ekipa-world-dna.md`, and `docs/rockstar-micro-open-world-lens.md`.
- Updated the active goal-loop decision toward tooling before more map sprawl: `docs/superpowers/specs/2026-05-09-runtime-editor-design.md` now defines a Dear ImGui C++ runtime editor, `data/world/block13_editor_overlay.json` overlay authoring, Python validation/pipeline responsibilities, and the later C# data-editor boundary.
- Added an explicit **goal-alignment shield** for lower-capacity model sessions: a concrete decision filter for C++ runtime/editor/Python/C# boundaries, anti-drift rules, traceability requirements, and option-based fallback (A/B/C) for risky changes. Added:
  - [docs/goal-alignment-shield-2026-05-09.md](docs/goal-alignment-shield-2026-05-09.md)
- Added a **goal-decision checklist template** for larger iterations:
  - [docs/goal-decision-checklist-template.md](docs/goal-decision-checklist-template.md)
- Added a first-pass **completion audit artifact** to keep the current objective measurable:
  - [docs/goal-completion-audit-2026-05-09.md](docs/goal-completion-audit-2026-05-09.md)
- Updated that artifact into an explicit status matrix (`done/partial/missing`) for fast checkpoint validation.
- Started `v0.15 Runtime Editor Foundation`: added overlay data/codec/apply integration, `data/world/block13_editor_overlay.json`, Python overlay validation in CTest, a testable `RuntimeMapEditor` model, dev-only Dear ImGui/rlImGui wiring, `F10` editor toggle, object list/inspector, asset manifest instance placement, and save/export back to overlay JSON.
- Improved runtime editor placement flow: asset-panel additions now spawn in front of the current camera/player anchor instead of at world zero, and the placement helper is covered by support tests.
- Added a safe runtime-editor delete affordance: the inspector now exposes `Delete Selected` only for editor-created `editor_...` instances, with tests proving base authored objects cannot be deleted through that path.
- Hardened runtime-editor save semantics: dev editor attach now receives the loaded overlay document and preserves existing base-object overrides when saving new instance edits, preventing accidental loss of prior `block13_editor_overlay.json` authoring.
- Added runtime-editor undo/redo: the editor now snapshots object state, selection, generated instance ids, and base-override tracking for transform/metadata edits, manifest instance placement, and safe deletes. Dear ImGui exposes `Undo`/`Redo`, and support tests cover base edit overlay tracking plus add/delete instance recovery.
- Hardened the Python asset pipeline: `tools/validate_assets.py` now enforces known manifest metadata keys/values, duplicate/whitespace tag hygiene, glTF 2.x buffers, and byte-length sanity. Added validator unit tests, registered them in CTest, and documented the current OBJ/glTF/material workflow in `docs/asset-pipeline.md`.
- Connected manifest metadata back into runtime authoring: Dear ImGui asset placement now passes the full `WorldAssetDefinition` into `RuntimeMapEditor`, so new editor instances inherit `tags=` metadata into overlay-persisted gameplay tags. Support tests prove tagged manifest instances round-trip into editor overlay output.
- Improved runtime-editor asset browsing: the asset panel now uses a test-covered multi-token, case-insensitive filter across id, model path, origin, render bucket, collision intent, and manifest tags, and shows render/collision/tag metadata plus hover details before placement.
- Started a presentation/art-direction foundation pass after reprioritizing graphics, gruz, character models, assets, object interactions, and missions: `WorldPresentationStyle` now centralizes sky color, ground color, subtle haze color, ground-plane scale, and world cull distance. Runtime drawing uses that style, and support tests guard readable sky/ground separation plus district-scale render/ground coverage. Art direction now targets stylized mid-poly readability (early PS3/late PS2 era) with beveled/chamfered edges, modelled detail, and material identity — see `docs/art-direction-pivot-stylized-mid-poly.md`.
- Put the new haze style into the actual render loop: `buildWorldAtmosphereBands()` creates thin camera-following haze bands from `WorldPresentationStyle`, `WorldRenderer::drawWorldAtmosphere()` draws them after the ground pass, and tests keep the effect subtle, near-ground, tapered, and within render distance.
- Added the first focused gruz-presentation pass: `vehicleLightVisualState()` now derives headlight/taillight tint and glow from rpm, boost, reverse speed, drift, and condition, and the vehicle renderer uses those colors on authored light parts. Support tests keep boost/reverse readable and heavy damage dimmed but still visible.
- Started the NPC presentation pass: `CharacterVisualProfile`/`characterVisualCatalog()` now define distinct player, Bogus, Zenon, Lolek, and receipt-holder palettes; the renderer can take a character palette, and the intro/paragon NPCs now use the modular low-poly character model instead of simple marker primitives. Tests guard named profiles, opacity, fallback behavior, and distinct mission-readable accents.
- Added the first object-interaction affordance layer: `WorldObjectInteraction` turns selected authored objects into low-priority resolver candidates for `E` prompts, covering shop doors, garage doors, Bogus's bench, trash dressing, and glass surfaces. Runtime use now plays a small interact pose and feedback line, while tests ensure ground patches/route arrows do not spam prompts and object priority stays below NPC/vehicle interactions.
- Extended object affordances with stable `outcomeId` and authored `WorldLocationTag` so future mission/dialog/cutscene data can react to object use without parsing prompt text. Tests require non-empty unique outcomes and a recoverable affordance for each object candidate.
- Started the C# external data-editor track: `tools/BlokTools` now has a test-covered core loader/validator for the current mission JSON and `data/world/object_outcome_catalog.json`, plus a WPF surface that edits/adds mission steps and dialogue, binds a selected step trigger to object hooks as `outcome:<id>`, and saves only after validation while keeping object outcome hooks visible for future mission/dialog/cutscene wiring. Runtime now resolves matching object affordance outcomes back into mission trigger results for supported intro phases.
- Hardened the C# mission editor against data/runtime drift: `MissionPhaseCatalog` now lists runtime-supported phase names, validation blocks unsupported phase ids, and the WPF `Add Step` command uses the next missing supported phase instead of inventing a JSON phase the C++ runtime would silently ignore.
- Tightened BlokTools workspace validation: `BlokWorkspaceLoader` now validates mission `outcome:<id>` triggers against the loaded object outcome catalog, and tests require the current workspace snapshot to be issue-free while synthetic missing-outcome data is rejected.
- Started `v0.14 Rewir Memory Seed` by splitting garage and trash-area memory from generic parking/road-loop tags. Added `WorldLocationTag::Garage` and `WorldLocationTag::Trash`, distinct intro-level zones/anchors, authored object tags for garage/trash dressing, and tests proving garage/trash events stay queryable separately for future rewir systems.
- Added `WorldEventLedger::queryByLocationAndSource` so future service/NPC consequences can ask what a specific rewir remembers from a stable source without scanning the whole ledger manually. Covered active matching, wrong-source filtering, and expiry in core tests.
- Added the first visible service-memory consequence through `WorldServiceState`: Zenon's prompt/line now shifts from normal service to wary service after active shop trouble memory, and to window-only service after `paragon_chaos` or a story shop ban. Covered normal, wary, cooled-down, chaos, and persistent-ban states in core tests.
- Added a debug/QA memory digest for rewir work: `WorldEventLedger::locationCounts()` now reports active memory per location, core tests cover expiry and per-rewir separation, and the debug HUD shows compact `Mem B/S/P/G/T/R` counts during playtests.
- Added `WorldEventLedger::memoryHotspots()` as the first reusable QA/tooling API for strongest active memory per rewir. Hotspots preserve event identity, source, score, position, and stable rewir order; the debug HUD now shows the active hotspot count beside `Mem B/S/P/G/T/R`.
- Added debug-world visualization for memory hotspots: `buildMemoryHotspotDebugMarkers()` converts active hotspot data into colored, score-scaled 3D markers, and debug render now draws them above remembered trouble spots for faster QA of rewir memory.
- Added first non-shop rewir social consequences: fresh garage or trash-area alarm memory now beats generic window gossip and produces local `Garazowy`/`Dozorca` reaction lines, with tests proving both rewir-specific witnesses are selected.
- Completed the first pass of rewir-specific alarm witnesses by adding parking and road-loop reactions: parking damage now routes to `Parkingowy`, road-loop damage to `Kierowca`, both above generic window gossip and covered by core tests.
- Connected memory hotspots back into social logic: aged active non-shop rewir hotspots above the minimum weight now keep local witness reactions above generic gossip, so parking/garage/trash/road-loop memory remains socially local after the first alarm beat while weak old noise still falls back to `Zul` gossip.
- Added `WorldRewirPressure` as a testable local-pressure snapshot for future patrols and micro-consequences: strong nearby parking/garage/trash/road-loop memory now resolves to watchful/alert rewir pressure, weak remote or shop-only memory is ignored, and the debug HUD shows the active rewir, level, score, distance, patrol-interest bit, and source.
- Gave rewir pressure its first gameplay beat: lingering local pressure preserves the remembered event id and can surface a lower-priority `PatrolHint` after the local witness has already reacted, keeping the first response local while letting the wider osiedle start circling the hot spot.
- Made rewir pressure spatial for pursuit support: `ChaseAiInput` now accepts a patrol-interest point/radius, patrol mode orbits that hot spot when the target is unknown, and the mission chase runtime feeds it from `WorldRewirPressure` so active memory can steer patrol behavior around the remembered rewir.
- Added debug-world QA markers for rewir patrol interest: active `WorldRewirPressure` now builds an assertive patrol-radius marker and debug render draws it over the hot spot, making the full memory-to-patrol loop visible during playtests.
- Added the first garage service consequence from rewir pressure: Rysiek now has a post-intro garage interaction whose prompt and line shift into a wary mode while active garage pressure is hot, then cool back when the memory expires.
- Expanded pressure-driven civil consequences to the trash rewir: Dozorca now has a post-intro trash-area interaction whose prompt and line change while active trash pressure is hot, proving the pattern works beyond Rysiek's garage.
- Turned pressure-driven civil consequences into a small service catalog: garage and trash interactions now resolve through `LocalRewirServiceSpec`/`LocalRewirServiceState`, so future Grochow rewir services can be added as data-backed specs instead of one-off `GameApp` branches.
- Moved local-service interaction authoring into that catalog as well: each `LocalRewirServiceSpec` now owns its point position and radius, and `IntroLevelBuilder::populateInteractions()` emits those points from the catalog instead of duplicating Rysiek/Dozorca placement by hand.
- Added the first new service through the completed catalog path: `Parkingowy` now has a post-intro parking interaction whose prompt and line shift under active parking pressure, with no new runtime branches needed.
- Completed the current pressure-service set for non-shop rewir pressure by adding `Kierowca` as a road-loop catalog service. Road-loop pressure now changes his prompt/line through the same catalog path, tying gruz-route trouble back into civil consequences.
- Added a coverage gate between pressure and civil consequences: `WorldRewirPressure` now exposes its pressure-enabled locations and core tests require every such rewir to have a local service spec, preventing future pressure systems from shipping without a visible social consequence.
- Added a debug digest for local rewir services: `buildLocalRewirServiceDigest()` reports total/wary pressure-driven services and the wary interaction point ids, while the debug HUD now shows compact `Svc wary/total` state beside the active rewir pressure line.
- Persisted active world memory into save games: `SaveGame` now round-trips active `WorldEventLedger` events, `GameApp` restores them on load, and tests prove a saved garage pressure memory reactivates the local Rysiek service consequence after deserialization.
- Extended save-game world-reaction persistence beyond raw memory: `SaveGame` now round-trips Przypal decay delay, band, pending pulse, contributors, and emitter cooldowns, so loading a save preserves the heat rhythm and prevents immediate repeated event spam.
- Added the first active rewir-relief loop: talking to a wary local service now softens only that service's own rewir memory, immediately returning the service to calm mode while leaving other hot rewirs intact. The generic `applyLocalRewirServiceRelief()` path is covered by tests and is hooked into local-service talk interactions.
- Made rewir relief visible to the player: successful local-service relief now emits a short `Rewir` system hint such as `Garaz odpuszcza.`, while no-op repeat relief produces no stale feedback.
- Added a data-backed rewir favor catalog as the next step toward active local mini-tasks: every local pressure service now has a nearby favor spec with stable ids, prompt, start and completion lines, and a resolver by service interaction id.
- Activated the first local rewir favors: favor points now populate from the catalog, appear only when their own rewir is under hot pressure, complete once per runtime session, and reuse service relief to calm the matching local memory.
- Persisted completed local rewir favors into save games and retry checkpoints: completed favor ids now round-trip through `SaveGame`, validate against malformed payloads, restore into `GameApp`, and write the active slot immediately after a favor completes.
- Tightened rewir favor QA: save validation now rejects completed favor ids that are not in the authored catalog, `buildLocalRewirFavorDigest()` reports active/completed/total favor state, and the debug HUD shows compact `Fav active/completed/total` context beside local pressure.
- Started pass by locating HUD (`src/game/GameHudRenderers.cpp`), marker presentation (`src/game/IntroLevelPresentation.cpp`, `src/game/GameRenderers.cpp`), driver pose (`src/game/GameRenderers.cpp`), mission text data (`data/mission_driving_errand.json`), and decal dressing (`src/game/IntroLevelGroundTruthDressing.cpp`).
- Implemented compact objective HUD, time-limited controls hint, simplified vehicle HUD, player-facing Przypał text, UTF-8 UI font loading from Windows fonts, Polish mission text, lower driver seat pose, subtler world markers, staged Polish route labels, close-wall camera lift, and thinner/lower-alpha ground decals.
- Verified with `cmake --build build --config Release`, `ctest --test-dir build -C Release --output-on-failure`, and `.\build\Release\blokowa_satyra.exe --data-root data --no-audio --no-save --no-load-save --smoke-frames 5`.
- Visual screenshot artifact: `artifacts/v0.13-hud-start.png`. The Codex terminal overlapped part of the capture, but it confirmed the compact objective panel and caught the remaining `Bogus` marker label, which was changed to `Boguś`.
