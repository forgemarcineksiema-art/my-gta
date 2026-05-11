# Rockstar Micro Open World Lens

## Purpose

This document is a design lens, not a promise to build a Rockstar-scale game. It captures what is useful from Rockstar-style world design for Blok 13: Rewir:

```txt
one small estate
+ dense systemic consequences
+ readable routines
+ strong camera/control polish
+ missions that use the same world systems
```

The goal is not "Polish GTA". The goal is a compact satirical osiedle where ordinary places behave like systems: parking, shop, staircase, trash area, garage, patrol route, window neighbors, local informants, and one miserable car.

Long-term, this lens expands through the [Grochow Target Vision](superpowers/specs/2026-05-09-grochow-target-vision-design.md): Blok 13 stays the first quality gate and home base, then the same systemic density expands into a Rockstar-style compressed Grochow-scale district. Scale must come from connected rewirs, not empty map size.

## Core Thought

If Rockstar designed this premise, the estate would be the main character.

The player should learn the space through repeated use:

- where the shopkeeper stands;
- where the local drunk gives information;
- where the patrol tends to turn;
- which staircase can hide the player;
- which parking spot starts trouble;
- where the camera gets tight;
- where a chase can be lost.

The map can stay small if every location has a job in gameplay.

## What To Borrow

### 1. Osiedle As A System

Every important place should answer three questions:

- What can the player do here?
- Who notices it?
- What changes after it happens?

Examples:

- Shop: buying, arguing, banned service, gossip, camera/patrol attention.
- Parking: vehicle access, conflict over spots, collisions, Przypal, chases.
- Staircase: hiding, NPC shortcuts, indoor camera stress test, escape route.
- Trash area: cheap chaos, prop collisions, witness reactions, local memory.
- Garage: vehicle repair, gruz upgrades, shady jobs, storage.

### 2. System Stitching

The magic is not one big system. It is transitions between small systems:

```txt
walk to NPC
-> short dialogue
-> enter car
-> driving dialogue or subtitle
-> small complication
-> chase or escape
-> consequence saved in world memory
-> later NPC line references it
```

This is more important than adding a bigger map.

### 3. Layered Przypal

Przypal should grow from a simple readable meter into layered pressure over time.

Early layers:

- Local Przypal: nearby NPCs react, shout, avoid, gossip.
- Area Przypal: shop/parking/staircase remembers recent trouble.
- Chase Przypal: a local pursuer or patrol pressure starts.

Later layers:

- Faction/social Przypal: some groups like or dislike what happened.
- Service consequences: shop refuses, garage charges more, NPC gives worse info.
- World state: boarded window, moved patrol, changed parking conflict, new graffiti.

Do not hide these consequences. The player should understand why the world changed.

### 4. NPC Memory Without Heavy Simulation

Rockstar-style detail does not mean full AI life simulation in our scope.

Use a small event ledger first:

```txt
event type
location tag
involved NPC/archetype
severity
time remaining
one follow-up line or consequence
```

This is enough for:

- "you made chaos near the shop";
- "you hit a parked car on the parking lot";
- "you were chased near the block";
- "you annoyed the same NPC twice";
- "this area is hot for a short time".

The player does not need a giant invisible reputation matrix yet.

### 5. Missions Use The Estate

Good mission shape:

```txt
small stupid premise
-> known local place
-> one system complication
-> existing movement/vehicle/chase/interact mechanic
-> short consequence remembered by osiedle
```

Bad mission shape:

```txt
marker says go here
-> unrelated objective
-> reset world after completion
```

The mission should teach the player something about the estate.

## What Not To Borrow

Do not copy Rockstar production weight:

- huge city;
- long cinematic cutscenes;
- dozens of bespoke mission scripts;
- slow RDR2-style interaction locks;
- hyper-detailed NPC schedules before the base game is fun;
- mission failure for harmless creativity;
- realism that makes controls worse.

The project needs Rockstar questions, not Rockstar budget.

## Relationship To Bitter Osiedle Satire

Walaszek-like reference provides temperament, not canon:

- blunt local absurdity;
- ugly small conflicts;
- short punchy dialogue;
- fools making bad decisions;
- escalation from stupid premises.

Rockstar lens provides structure:

- the place remembers;
- systems connect;
- consequences are visible;
- camera and controls are polished;
- missions are grounded in local geography.

Together:

```txt
Bitter osiedle satire
+ Rockstar structure
+ Bully scale
+ Saints Row tempo
+ GTA IV/V physical readability
```

## v0.7 Implications

v0.7 should not become a full city simulation. It should prove the first version of "osiedle remembers".

Recommended v0.7 target:

- `PrzypalSystem`: local heat value with decay and event reasons.
- `WorldMemory`: small recent-event ledger.
- `NpcReactionSystem`: short reaction lines based on memory and Przypal.
- Location tags for current map: shop, parking, block, road loop, trash/props.
- HUD/debug showing why Przypal changed.
- One or two visible consequences: changed prompt, gossip line, stronger chase pressure, or temporary shop/parking reaction.

Non-goals for v0.7:

- full police system;
- full daily schedules;
- persistent faction economy;
- social media/radio simulation;
- new district;
- complex branching mission chain.

## Quality Bar

Before expanding the map, the current estate should pass these questions:

- Can the player have fun walking or driving for 10 minutes without a mission?
- Does the camera help in the exact places where the estate is tight?
- Does causing chaos near the shop feel different from causing chaos on the road?
- Can an NPC line make the player remember what they did?
- Does Przypal explain pressure without stealing control?
- Does a parking/trash/shop event create a small story?

If yes, the project has the right micro-open-world direction.
