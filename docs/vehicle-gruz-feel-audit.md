# Vehicle Gruz Feel Audit

Status: CURRENT
Last verified against code: 2026-05-10
Owner: vehicle feel
Source of truth for: current gruz vehicle feel checks and known risks

v0.6.3 is a feel-first pass for one old car. The goal is not Forza physics or a full GTA traffic system. The goal is controlled gruz chaos: the car should sound and look like a tired wreck, while steering, braking, reverse, handbrake, and camera remain understandable.

## Current Contract

- `W` accelerates, `S` brakes first and reverses only after the car is nearly stopped.
- `A/D` steer left/right with stronger low-speed steering and calmer high-speed steering.
- `Space` is handbrake in vehicle mode only.
- `Shift` is desperate gas in vehicle mode: stronger launch, more instability, light wear.
- `H` fires a horn pulse with cooldown and no movement side effects.
- Condition stays in `0..100` and maps to: `Igla`, `Jeszcze pojezdzi`, `Cos stuka`, `Zaraz wybuchnie`, `Zlom`.
- Damage makes the car weaker and smokier, but never randomly immobilizes the player.

## Implemented Hooks

- `VehicleTuningConfig` owns arcade parameters such as acceleration, braking, steering, grip, drift, boost, and damage influence.
- `VehicleRuntimeState` exposes speed, speed ratio, lateral slip, instability, horn pulse/cooldown, boost state, condition, and condition band.
- `applyCollisionImpact(...)` reduces condition, adds bump instability, adds slip, and bleeds speed after a wall impact.
- Steering authority now depends on actual rolling speed, so a parked gruz no longer pivots in place like a debug pawn.
- If gas and brake are held together, brake wins and the car does not snap into reverse.
- Reversing keeps arcade screen direction: `A` remains left-feeling and `D` remains right-feeling instead of inverting like a stricter car sim.
- v0.6.7 feel gate adds recovery/readability checks: handbrake slip should mostly cool down within about 1s, and even a `Zlom` gruz must still accelerate, steer, brake, and boost predictably.
- Vehicle yaw is wrapped to a stable range, with a per-frame turn cap to survive large `dt` spikes.
- Circle collision is swept through substeps, so vehicle-style movement is less likely to tunnel through thin obstacles after a hitch.
- `GameApp` maps vehicle-only boost/horn input, shows a compact Gruz HUD panel, plays generated placeholder sounds, and draws simple smoke/horn feedback.
- `vehicleLightVisualState(...)` gives the gruz readable front/rear light feedback: boost and high rpm make headlights hotter, reverse makes taillights hotter, and heavy condition damage dims the presentation without hiding the car.
- `CameraRig` vehicle mode expands boom/FOV with speed and keeps boom collision active.

## Known Risks

- The car still uses a simple kinematic controller, so wall impacts are readable rather than physically rich.
- Condition damage currently comes from map collision only; props are not destructible yet.
- Audio is generated placeholder tone noise, good enough for feedback but not for final gruz identity.
- There is no traffic, patrol, parking legality, Przypal scoring, or NPC response in this pass.
- Manual tuning is still required: tests prove clamps and state transitions, not that the car is maximally fun.
- The runtime car collision still uses one vehicle radius for the whole body, so the Kombi silhouette and collision shape are only approximate.

## Next Useful Checks

- Tune low-speed steering against the actual parking lot, not only tests.
- During manual QA, test W/S/A/D/Space/Shift again after heavy condition damage; bad condition should add flavor, not make input ambiguous.
- Add destructible or pushable trash/sign props before increasing vehicle damage drama.
- Feed horn, boost, sidewalk driving, and collisions into v0.7 Przypal once that system exists.
- Replace placeholder tones with authored gruz sounds after the sound direction is locked.
