# Player Foundation Quality Gate

Status: CURRENT
Last verified against code: 2026-05-10
Owner: player foundation
Source of truth for: movement and camera quality checks

## Purpose

Movement and camera are not a completed checkbox. They are the foundation every future system touches. The current project has a code-driven player stack, camera boom collision, sprint/jump basics, and comedy feedback, but every new feature must prove it does not make the player feel worse.

Use this file before and after adding gameplay systems.

## Hard Rule

```txt
The character may look dumb.
The controller must stay clear.
```

## Always Protect

- `W` moves in the direction the camera implies.
- `A/D` feel screen-relative and predictable.
- Walk, jog, and sprint are distinct on foot: Ctrl for walk, default WASD for jog, Shift for sprint.
- Camera does not hide the player behind geometry.
- Sprint has immediate value and stays controllable.
- Jump is simple, buffered, and useful, not a platformer tax.
- Walls slide, not glue.
- Curbs/low steps do not require manual jumping.
- Collision comedy is brief and readable.
- UI/camera feedback never makes steering or walking ambiguous.

## Feature Review Checklist

Before merging or calling a feature done, test:

- 5 minutes free roaming without mission;
- 2 minutes alternating Ctrl walk, default jog, and Shift sprint;
- 5 minutes sprinting around block corners;
- 20 sharp direction reversals while jogging and sprinting;
- 10 jump attempts pressed just before landing;
- 10 wall/corner collisions;
- 10 curb/step/ramp crossings;
- 5 enter/exit vehicle cycles;
- 3 chase starts and escapes;
- 3 high-Przypal moments once v0.7 exists.

Pass condition:

```txt
The first complaint should not be camera, controls, or "I do not know where I am moving".
```

## Allowed Control Loss

Short control loss is allowed only when it is a readable reaction:

- light bump/stagger: very short;
- landing dip: visual first, minimal input impact;
- future ragdoll: rare, strong cause, quick recovery;
- interaction lock: clear prompt/context and no direction surprise after release.

Avoid:

- random tripping;
- repeated stunlock;
- camera event during sharp steering;
- hidden collision that punishes the player;
- long animation before control returns.

## Camera Gates

Every new gameplay context should answer:

- Is this open, indoor, vehicle, chase, dialogue, or future combat?
- Does the camera know what target to look at?
- Does auto-align help or fight the player here?
- Can geometry block the player view?
- Is shake/FOV feedback capped?
- Does the camera return smoothly after an event?

## Backlog For Player Feel

- Tune mouse sensitivity and recenter values through playtest.
- Add indoor/stair camera preset.
- Improve vehicle camera distance and speed FOV.
- Improve enter/exit vehicle lock and camera continuity.
- Add better prop/NPC collision reactions.
- Add real animation assets later only after code-driven feel is stable.
- Add ragdoll/knockdown as a rare response, not a constant gag.

## Verification Commands

For code changes:

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For documentation-only changes, build is optional, but scan docs for placeholders and contradictions before calling the pass complete.
