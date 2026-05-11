# Documentation Map

Status: LIVE
Last verified against code: 2026-05-10
Owner: project workflow

This folder contains different kinds of documents. Do not treat every document here as current implementation truth.

If documentation conflicts with code, tests, validators, CMake presets, checked-in run scripts, or what actually runs in the game, the implementation evidence wins and the document needs a truth-pass update.

## Truth hierarchy

1. **Operational truth:** code, CMake, tests, validators, run scripts, and actual runtime behavior.
2. **Live workflow truth:** `DEVELOPMENT.md`, this file, and `docs/current-state.md`.
3. **Current technical references:** contracts and quality gates that should be kept close to implementation.
4. **Design vision:** tone, world, mission, and long-term target documents.
5. **Archive:** goal/session cards and process notes from earlier work sessions.

## Status labels

- **LIVE:** actively used as a source of truth for workflow or production rules.
- **CURRENT:** intended to describe current systems, but still subordinate to code/tests/validators.
- **PARTIAL:** implemented only for a slice, prototype, or limited path.
- **EXPERIMENTAL:** dev-tools, QA, or unstable workflow; useful but not production-final.
- **ASPIRATIONAL:** long-term direction, tone, target, or design intent; not a promise that the feature exists now.
- **ARCHIVED:** historical process record; do not use to steer implementation unless explicitly reactivated.
- **DEPRECATED:** old workflow or behavior kept only to explain what not to use.

## Live source-of-truth docs

- `../DEVELOPMENT.md`: official build/run workflow and quality gates.
- `docs/README.md`: documentation hierarchy and truth policy.
- `docs/current-state.md`: compact current-state map for implemented, partial, experimental, and aspirational areas.
- `docs/production-truth-contract.md`: production rules for shared world conventions and validation expectations.

## Current technical references

These should be kept close to code and validators:

- `asset-pipeline.md`
- `manual-playtest.md`
- `player-foundation-quality-gate.md`
- `control-context-foundation.md`
- `interaction-resolver.md`
- `vehicle-gruz-feel-audit.md`

## Design vision docs

These describe target feel, tone, world identity, or longer-term product direction. They may be aspirational and must not override implementation evidence:

- `blok-ekipa-world-dna.md`
- `blok-13-story-bible.md`
- `humor-tone-bible.md`
- `season-01-wojna-o-rewir.md`
- `rockstar-micro-open-world-lens.md`
- `walaszek-research-notes.md`
- `superpowers/specs/*`

## Archive/process docs

Goal/session cards and low-capacity process notes are historical context unless a current task explicitly reactivates them:

- `goal-session-card-*`
- `goal-completion-audit-*`
- `goal-alignment-shield-*`
- `goal-low-capacity-protocol-*`
- `goal-intellectual-safeguards-*`
- `goal-decision-checklist-template.md`

## Update rule

When a feature changes, update the smallest live/current document that would otherwise mislead the next developer. Prefer explicit labels such as `Implemented`, `Partial`, `Experimental`, `Aspirational`, and `Deprecated` over broad claims that everything works.
