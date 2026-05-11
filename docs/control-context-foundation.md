# Control Context Foundation

Status: CURRENT
Last verified against code: 2026-05-10
Owner: input/gameplay architecture
Source of truth for: contextual control vocabulary and mapping rules

The game uses contextual controls: few buttons, clear families of meaning, and different action mapping per gameplay state.

## Vocabulary

- `WASD`: movement family. On foot it is camera-relative movement; in a vehicle it is throttle/brake/steering.
- `Shift`: faster or more aggressive. On foot it is sprint; in a vehicle it is desperate gas.
- `Space`: dynamic maneuver. On foot it is jump; in a vehicle it is handbrake.
- `E/F`: use, enter, exit, acknowledge.
- `H/LMB`: vehicle horn/action family for the current gruz slice.
- `Mouse`: look. On foot it is orbit; in a vehicle it is orbit with recenter.

## Current Contexts

- `OnFootExploration`: camera-relative walk/jog/sprint/jump/interact.
- `OnFootPanic`: same input vocabulary as exploration, reserved for high-Przypal tuning.
- `VehicleDriving`: throttle, brake, steering, boost, handbrake, horn, exit.
- `InteractionLocked`: blocks movement/vehicle controls but keeps `E/F` available for acknowledge/use.
- `Combat`, `CarryObject`, `Ragdoll`: reserved states with conservative mapping placeholders.

## Architecture Rule

Raylib code should read `RawInputState` only. Gameplay meaning should come from:

```txt
RawInputState -> resolveControlContext(...) -> mapRawInputToInputState(...)
```

Do not add new direct key meanings in random runtime code. Add them to the context mapping and cover them with core tests.

## Current Tests

The core suite checks:

- `Space` maps to jump on foot and handbrake in vehicle.
- `Shift` maps to sprint on foot and boost in vehicle.
- hard interaction locks block movement/vehicle controls.
- `F` aliases enter/exit interaction.
- `LMB` aliases horn/action in vehicle, but not on foot.
- context priority: hard lock, ragdoll, vehicle, combat, carry, panic, exploration.
