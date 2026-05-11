# BlokTools

`BlokTools` is the first C# external data-editor surface for `Blok 13: Rewir`.

Current scope:

- load `data/mission_driving_errand.json`;
- validate mission step and dialogue basics;
- validate mission phase names against the runtime-supported phase catalog;
- edit mission objectives, triggers, dialogue speakers, dialogue text, and line durations;
- add new mission steps and dialogue lines;
- suggest the next missing runtime-supported phase when adding a mission step;
- bind a selected mission step trigger to a selected object outcome as `outcome:<id>`;
- block saves when an edited `outcome:<id>` trigger does not exist in `data/world/object_outcome_catalog.json`;
- validate workspace snapshots with the same mission/outcome catalog pairing used by the WPF editor;
- author object-outcome mission triggers that the C++ runtime can resolve when the matching world object is used;
- save mission JSON only after validation passes;
- write a `.bak` copy before replacing the mission file;
- load `data/world/object_outcome_catalog.json`;
- show object outcome hooks in a WPF window for mission/dialog authoring context, including whether the hook is quiet or emits a world-memory/Przypał event with intensity and cooldown.

The in-game C++ Dear ImGui editor remains the spatial map/object editor, while BlokTools grows toward missions, dialogue, cutscene beats, NPC reactions, items, shops, asset metadata, and localization.

## Build and test

```powershell
dotnet run --project tools\BlokTools\BlokTools.Tests\BlokTools.Tests.csproj
dotnet build tools\BlokTools\BlokTools.App\BlokTools.App.csproj
```

The main project verifier also builds the WPF app:

```powershell
.\tools\ci_verify.ps1 -Preset ci
```

Run the app:

```powershell
dotnet run --project tools\BlokTools\BlokTools.App\BlokTools.App.csproj
```
