# Interaction Resolver

Status: CURRENT
Last verified against code: 2026-05-10
Owner: gameplay interactions
Source of truth for: current `E/F` interaction priority model

The resolver decides what `E/F` means when several things are close together. This matters for a dense block map where an NPC, a car, a door, and a mission marker can all overlap.

## Current Rules

- Interactions are represented as `InteractionCandidate`.
- Each candidate declares an action, input family, prompt, radius, priority, and enabled flag.
- `InteractionInput::Use` is the `E` family.
- `InteractionInput::Vehicle` is the `F` family.
- Highest priority wins.
- If priority ties, the nearest candidate wins.
- Disabled, out-of-range, wrong-input, or `None` candidates are ignored.

## Current Slice

- `E` near the starting NPC starts the mission.
- `F` near the car enters the car even if an `E` interaction is also nearby.
- If only the car is relevant, `E/F` can both enter or exit.
- Exit is disabled during the chase, matching the existing mission rule.
- Selected authored world objects now add low-priority `E` candidates through `WorldObjectInteraction`: shop doors, garage doors, Bogus's bench, trash dressing, and glass surfaces.
- Each object affordance carries a stable `outcomeId` plus authored location. Mission JSON and BlokTools can bind a step trigger to `outcome:<id>`, and runtime object use resolves that data trigger without parsing prompt text.
- Object candidates are intentionally lower priority than NPCs and vehicle actions, so they add texture without stealing mission prompts.

## Why This Exists

Do not solve future interactions by adding more local `if` checks in `GameApp`. Add candidates, give them clear priority, and cover the rule in core tests.

Good future examples:

- `E`: talk to NPC.
- `F`: enter vehicle.
- `E`: open door if no higher-priority NPC/object is active.
- `E`: pick up item if it is the highlighted candidate.
- `F`: reserved for vehicle/special interaction family.
