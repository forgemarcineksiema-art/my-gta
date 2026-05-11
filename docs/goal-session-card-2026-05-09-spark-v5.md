# Goal Session Card - 2026-05-09 (Spark v5 lock)

## Session lock

- Model: `5.3-Codex-Spark`
- Mode: `AMBER`
- One-sentence goal slice:
  Extend C# mission tooling to include authoring paths for NPC reactions and cutscenes end-to-end in editor UI, validation, and tests without changing runtime semantics this pass.
- Owner layer: **C# data/editor tool**
- Confidence (pre) - scope clarity 9/10, compatibility 9/10, backward compatibility 9/10, validation confidence 9/10
- Rollback action: revert mission editor C# changes from this pass:
  - `tools/BlokTools/BlokTools.Core/MissionDocumentValidator.cs`
  - `tools/BlokTools/BlokTools.Core/MissionDocumentStore.cs`
  - `tools/BlokTools/BlokTools.Core/MissionEditorSession.cs`
  - `tools/BlokTools/BlokTools.Core/BlokWorkspaceLoader.cs`
  - `tools/BlokTools/BlokTools.App/MainWindow.xaml`
  - `tools/BlokTools/BlokTools.App/MainWindow.xaml.cs`
  - `tools/BlokTools/BlokTools.Tests/Program.cs`
- Evidence anchors:
  - source: `tools/BlokTools/BlokTools.Core/MissionDocumentValidator.cs`
  - gate: `dotnet run --project tools/BlokTools/BlokTools.Tests/BlokTools.Tests.csproj`

## Decision integrity

- Drift guard: no behavior-change without editor-driven production relevance.
- Consistency guard: one subsystem only (C#) this pass; runtime parser is deferred.
- Counter-failure: if edits appear unsupported by tests in the same pass, stop and switch to `RED`.

## Scope and acceptance

- Scope:
  - `tools/BlokTools/BlokTools.Core/MissionDocumentValidator.cs`
  - `tools/BlokTools/BlokTools.Core/MissionDocumentStore.cs`
  - `tools/BlokTools/BlokTools.Core/MissionEditorSession.cs`
  - `tools/BlokTools/BlokTools.Core/BlokWorkspaceLoader.cs`
  - `tools/BlokTools/BlokTools.App/MainWindow.xaml`
  - `tools/BlokTools/BlokTools.App/MainWindow.xaml.cs`
  - `tools/BlokTools/BlokTools.Tests/Program.cs`
- Acceptance:
  - new validation + session + UI + snapshot + tests pass for `npcReactions` and `cutscenes`,
  - goal lock + evidence remains in `progress.md`.

## Uncertainty

- Uncertainty: whether initial phase defaults for added NPC/cutscene rows should be first step phase or first free runtime phase.
