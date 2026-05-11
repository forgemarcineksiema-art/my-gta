# Goal Decision Checklist Template

Use this for every larger cycle (mission chunk, tooling task, balance pass, art pass).

## 0) Model safety precheck (required for weak model sessions)

- Ambiguity check:
  - [ ] I can name exactly one objective clause this task advances.
  - [ ] I can name one validator/test or one manual smoke that will verify it.
- [ ] I can explain failure path and rollback trigger.
- [ ] I can list one file/line that defines current truth, and one gate output that must pass.
- Cognitive safeguard check:
  - [ ] I can state the evidence source (file + line or command + output) I relied on.
- [ ] I ran a dual-pass pass/fail check (initial proposal and rollback risk pass).
- [ ] I recorded one uncertainty I did not resolve yet.
- [ ] I stated one explicit counter-claim (most likely wrong interpretation) and why it is rejected.
- [ ] I recorded a hypothesis pair:
  - `H_true:` with one `source:` and one `gate:` anchor.
  - `H_false:` with one `source:` and one `gate:` anchor.
- Scope check:
  - [ ] This is not introducing a completely new concept.
  - [ ] No unrelated system is being touched.
  - [ ] File edits stay within declared scope.

## 1) Decision card

- Task name:
- Why this now:
- What objective clause it serves:
- Owner layer:
  - [ ] exactly one of the owner checkboxes is selected.
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [ ] C# data/editor tool
  - [ ] Design/docs/procedure
- Expected impact:
  - Gameplay:
  - Visual/identity:
  - Social systems:
  - Tooling:
- Mode:
  - [ ] GREEN (clear path, gates available)
  - [ ] AMBER (uncertainty, split + validate)
  - [ ] RED (no implementation; refine scope)
- Reversible? (yes/no):
- Risk level (0-3):
  - [ ] 0: no new risk
  - [ ] 1: localized risk, easy rollback
  - [ ] 2: cross-system risk, test needed
  - [ ] 3: architectural risk, staged rollout required
- Confidence score (pre): /10
- Confidence score (post-review): /10
- Decision integrity:
  - [ ] Drift guard: no "Roblox/shape-first toy" direction without local consequence hook.
  - [ ] Consistency guard: dependent systems updated together or deferred with explicit split.
  - [ ] One-sentence counter-failure:
  - [ ] Source lock: file+line and gate anchor are pinned in notes.
- Choice if uncertain:
  - [ ] A strict minimum delta
  - [ ] B split into two steps + validate midway
  - [ ] C defer until next gate
- One direct rollback action:

## 2) Deliverable mapping (prompt -> artifact)

- Requirement:
- Concrete artifact(s):
  - File(s):
  - Runtime/system touched:
- Evidence:
  - Test/gate:
  - Manual smoke:
  - Screenshot/log:
- Acceptance criteria:
  - [ ] passes
  - [ ] not needed

## 3) Alignment gates

- Vision check:
  - supports Blok 13 as heart
  - supports compressed Grochow scale path
  - preserves readable driving/controls/camera direction
  - preserves satirical social-pressure framing
  - avoids non-local/global abstractions
- Traceability:
  - progress.md entry added:
  - design/docs updated:
  - decision rationale linked to [docs/goal-alignment-shield-2026-05-09.md](docs/goal-alignment-shield-2026-05-09.md):

## 4) Completion note

- Changed file list:
- Validation commands used:
- Open issues:
- Follow-up (if any):
