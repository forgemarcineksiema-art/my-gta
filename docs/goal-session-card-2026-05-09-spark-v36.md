# Goal Session Card - 2026-05-09 (Spark v36 lock)

## Session lock

- Model: `5.3-codex-spark`
- Mode: `AMBER`
- One-sentence goal:
  Finalizujemy pierwszy gameplayowy profil misji przez wypełnienie jej runtime-pracujących struktur (`npcReactions`, `cutscenes`) oraz lokalizacją, bez otwierania nowego owner-a.
- Owner layer:
  - [ ] C++ runtime/editor
  - [ ] Python pipeline/validator
  - [x] C# data/editor tool
  - [ ] docs/procedure
  - [ ] tests
- Confidence (pre): 8/10, 8/10, 9/10, 8/10
- Uncertainty: czy do tej misji dołożyć minimum jedną paczkę `npcReactions` i `cutscenes` już teraz, czy zacząć od `npcReactions` z osobnego kroku i rozszerzyć cutsceny później.
- Rollback action: wycofaj ten lock i przywrocic [docs/goal-session-card-2026-05-09-spark-v35.md](docs/goal-session-card-2026-05-09-spark-v35.md) jako aktywny wpis w `progress.md`.
- Evidence anchors:
  - source: `tools/BlokTools/BlokTools.Core/MissionDocument.cs`
  - source: `src/game/WorldDataLoader.cpp`
  - gate: `dotnet run --project tools\\BlokTools\\BlokTools.Tests\\BlokTools.Tests.csproj`

H_true:
  source: `tools/BlokTools.Core\\MissionDocument.cs`
  gate: `dotnet run --project tools\\BlokTools\\BlokTools.Tests\\BlokTools.Tests.csproj`

H_false:
  source: `src/game/WorldDataLoader.cpp`
  gate: `dotnet run --project tools\\BlokTools\\BlokTools.Tests\\BlokTools.Tests.csproj`

Counter-claim: jeśli do tej pory produkcyjna misja nie ma faktycznie `npcReactions` i `cutscenes`, to „obsługiwanie ich w UI” pozostaje martwą funkcją bez gameplayowego zwrotu.

Current choice: `A`

## Behavior constraints for this pass

- one owner layer only: C# data/editor + mission fixture.
- one-source lock + one gate per slice.
- no C++ or Python layer edits in this pass.
- if validator/test shows regressions, revert only files touched in this slice.

Branch table:

`A`: Minimal hardening slice (recommended)
- add one mission-level `npcReactions` and one `cutscenes` block to `data/mission_driving_errand.json`,
- add matching localization keys in `data/world/mission_localization_pl.json`,
- update tests that currently assert legacy fixture shape,
- run BlokTools test gate.

`B`: Two-step split (if encoding/reliability concerns arise)
- Step 1: add npcReactions + localization, verify with tests.
- Step 2: add cutscenes + localization, verify with tests.

`C`: Defer
- collect one more runtime smoke check from C++ build/gate first.

## Session acceptance

- run:
  - `dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj`
- update `progress.md` to set active lock to this card and archive v35.
