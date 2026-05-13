# Blok 13: Rewir DNA

Status: ASPIRATIONAL
Last verified against code: 2026-05-10
Owner: design direction
Source of truth for: long-term world identity and tone direction, not current implemented state

Legacy filename note: this file used to describe "Blok Ekipa World". The approved canon is now **Blok 13: Rewir**: an original IP direction inspired by Polish block-estate satire, Rockstar-style systemic consequence design, Bully-scale locality, and Saints Row/GTA arcade readability.

## Working Premise

Blok 13: Rewir is a small-scale single-player micro-open-world about one Polish estate under pressure from debt, bureaucracy, developers, parking wars, local hustles, gossip, and social heat.

The game should not try to be "Polish GTA" with a huge city. It should feel like a dense, bitterly funny estate where ordinary problems become absurd conflicts and where stupid player actions leave visible local consequences.

The core fantasy is:

```txt
one small estate
+ returning protagonist with a personal reason to stay
+ fast readable TPP movement and gruz driving
+ Przypal instead of a generic wanted level
+ World Memory that lets the estate comment and react
+ short episode-like missions that feed one bigger war for the rewir
```

## Canon Decision

The project is now its own IP. Do not adapt Blok Ekipa episodes, characters, names, direct dialogue, or scene structures 1:1.

Walaszek/Blok Ekipa remain tone references: rough timing, ugly local absurdity, blunt escalation, strong archetypes, and low-cost visual charm. They are not the production canon.

Current canon:

- title: `Blok 13: Rewir`;
- format: premium single-player micro-open-world;
- tone: bitter social satire with comedy, not pure gag comedy;
- structure: season-like story with episode missions;
- current scope: one dense estate before any district expansion;
- long-term target: a Rockstar-style compressed Grochow-scale district, with Blok 13 as the home base and quality gate;
- tech: C++17 + raylib runtime, Dear ImGui dev editor, Python pipeline/validators, and later C# external data editor;
- quality target: Rockstar discipline, not Rockstar scale.

## Identity

This is not:

- a direct licensed adaptation;
- a realistic city simulator;
- a full GTA V clone;
- RDR2 with Polish jokes;
- a shooter-first sandbox;
- a giant open world;
- an online/live-service game;
- a pure meme comedy with no story spine.

This is:

- one estate as the main character;
- a local story about parking, debt, garages, shop access, gossip, and developer pressure;
- an original cast of useful, memorable idiots with motivations;
- rough, expressive stylized mid-poly 3D used as intentional style;
- movement that may look stupid but must play well;
- short dialogue, quick escalation, ugly local conflicts, and visible consequences.

The design motto is:

```txt
Rockstar structure. Walaszek bite. Bully scale. Saints Row tempo.
It can look crude. It cannot control stupidly.
```

## Rockstar Standard, Not Rockstar Scale

Use [Rockstar Micro Open World Lens](rockstar-micro-open-world-lens.md) as a structural companion to this DNA document.
Use [Grochow Target Vision](superpowers/specs/2026-05-09-grochow-target-vision-design.md) as the long-term scale companion.

This means:

- the estate has memory;
- locations have gameplay roles;
- systems connect through smooth transitions;
- missions are grounded in geography;
- NPCs remember enough to make the player feel seen;
- consequences are visible in UI, dialogue, prompts, patrol pressure, shop state, or small world changes.

This does not mean:

- photorealism;
- hundreds of NPCs;
- giant cinematic scope;
- long cutscenes;
- huge animation lock-in;
- full police simulation in early milestones;
- AAA production budget.

## Creative Pillars

### 1. Gorzka satyra z humorem

The estate should be funny because it is absurd and recognizable at the same time. The joke should often have a sad or angry truth underneath it:

- parking is territory;
- a receipt becomes evidence;
- a shop ban becomes a social sentence;
- a garage becomes political ground;
- a staircase becomes a witness corridor;
- a developer poster becomes a threat.

Humor should sharpen the world, not replace the world.

### 2. Osiedle is the protagonist

Before adding more map, the current estate must become readable and reactive:

- block and staircases;
- shop;
- parking;
- garages;
- trash area;
- bench/loitering spot;
- small pitch/court;
- local den or basement;
- pawn/lombard-like trade point;
- patrol/municipal pressure point.

Every major place should answer:

```txt
What can the player do here?
Who notices?
What changes afterward?
```

### 3. Przypal is narrative pressure

Przypal replaces a generic wanted level. It is social heat: the whole estate starts noticing that the player is becoming a problem.

Sources:

- public nuisance;
- property damage;
- chase seen by NPCs;
- bumping into people;
- stealing or taking objects;
- upsetting shop/NPC routines;
- repeating chaos in the same location.

Outputs:

- HUD/camera tension;
- alarm lines;
- gossip lines;
- local denial of service or changed prompts;
- temporary patrol/chaser pressure;
- future mission variants.

Rule:

```txt
Przypal must explain pressure. It must never steal control.
```

### 4. World Memory makes missions matter

World memory should start small and readable. The player should understand why the world reacts.

Good early memory:

- "you made chaos near the shop";
- "you were chased on the parking loop";
- "you hit props near the block";
- "you annoyed the same source twice";
- "the estate is hot for the next minute".

Later memory:

- shop refuses or changes prices;
- a patrol route shifts;
- a parking conflict changes;
- an NPC gives worse or better information;
- a prop/scar/graffiti remains after a mission.

### 5. Original cast, clear gameplay roles

No direct canonical adaptation. Build original archetypes:

- returning protagonist: player, personal stake, debt/family/housing pressure;
- bench strategist: early mission giver and local commentary;
- window neighbor: alarm, memory, pressure;
- shopkeeper: economy, service denial, shop quests;
- garage mechanic: gruz repair, upgrades, vehicle missions;
- drunk informant: gossip, rumor, memory after events;
- local officer/patrol: pressure, chase hint, social consequence;
- co-opting bureaucrat: spoldzielnia pressure;
- polished developer: main external threat.

Each NPC needs:

- one obsession;
- one gameplay job;
- two or three useful reactions;
- one way to change after missions.

## Story Spine

Season 1 is **Wojna o Rewir**.

Premise:

```txt
The protagonist returns to Block 13 because of a family/housing/debt problem.
At first he only wants to fix one personal mess.
The estate is already being squeezed by spoldzielnia politics and a developer plan.
Small jobs reveal that the parking, garages, shop, and housing documents are tied together.
By the finale, a stupid parking dispute has become a local war for the rewir.
```

The antagonist pressure should be visible from the first minutes:

- developer posters;
- survey markers around parking/garages;
- spoldzielnia notices;
- residents arguing about "rewitalizacja";
- patrol presence after repeated trouble;
- shopkeeper and mechanic hinting that something is off.

## Mission Shape

Ideal mission format:

```txt
1. Local stupid premise.
2. Personal or estate-level reason it matters.
3. One clear action goal.
4. A complication caused by people/systems being broken.
5. Gameplay using existing systems: walk, drive, interact, carry, chase, Przypal.
6. A punchline.
7. A visible or remembered consequence.
```

Target duration: 3-7 minutes.

The best missions are not random jokes. They feed the same larger war:

- receipt problem becomes paperwork/evidence theme;
- gruz mission introduces garages under threat;
- sofa mission reveals hidden spoldzielnia documents;
- parking mission exposes the real conflict;
- shop ban shows social consequence of Przypal.

## Movement And Camera Are Permanent Quality Gates

The project now has a player foundation: camera-relative movement, sprint, jump, coyote time, wall slide, step/slope checks, camera boom collision, panic sprint presentation, gruz driving, and comedy feedback.

That means the base exists. It does not mean it is finished.

Every feature must protect player feel:

- Przypal cannot hide the player with shake or HUD spam.
- NPC reactions cannot steal control too long.
- Comedy bumps must be rare, short, and readable.
- Missions cannot force awkward camera angles unless deliberately locked.
- Vehicle/chase pressure must help the player read the route, not fight steering.
- Indoor/stair/shop spaces are camera tests, not content excuses.

Player feel stays a quality gate until camera/movement/driving stop being the first complaint.

## Gameplay DNA Mix

### GTA V / GTA SA

Controls and readability: camera-relative movement, quick vehicle enter/exit, simple mission structure, forgiving driving, readable chaos.

### Bully

Scale: local factions, small map, recognizable NPC types, short social conflicts, personal brawls later.

### RDR2

Use selectively: ambient reactions, world rootedness, slower moments between action. Avoid slow animation lock-in and input delay.

### GTA IV

Use for dirty physical comedy: mass, bumps, awkward collisions, vehicle wobble. Do not use it as the main responsiveness model.

### Saints Row

Use for arcade tempo, activity clarity, and absurd mission escalation. Do not import superhero scale or plastic tone.

## What We Avoid

- Large map before one block is alive.
- Photorealism as a goal.
- Full RPG loot/build trees.
- Soulslike combat difficulty.
- Shooter-first design.
- Realistic driving simulation.
- Animation-heavy RDR2 movement delay.
- Random frequent trips that take away control.
- Long cutscenes where short dialogue would work.
- Profanity as a substitute for a joke.
- Online/multiplayer before the solo game is fun.

## Development Priority

1. Keep v0.8.1 readability gate passing: the player must find Zenon's shop, understand why the gruz is locked, and complete the intro loop without external explanation.
2. Continue player foundation polish as a quality gate in every milestone.
3. Build the runtime editor foundation before expanding Grochow much further: C++ Dear ImGui editor, overlay JSON, Python validation, and editor-ready object data.
4. Treat Blok 13 as the first reusable rewir cell for future Grochow-scale expansion.
5. Build the first 5 mission specs around the same estate systems.
6. Improve vehicle/chase identity after Przypal can drive pressure.
7. Increase world density inside the same block before adding new rewirs.
8. Add social sandbox depth: gossip, respect, local denial/assistance.
9. Prototype combat later, after movement, camera, reactions, mission flow, and authoring tools feel stable.

## Known Current Limitations

- The estate is still a readability-focused stylized mid-poly map, not a finished district.
- There is no minimap, save game, economy, combat, traffic, full police, or persistent reputation.
- World Memory is runtime-only and short-term.
- Camera pitch is clamped; camera collision is basic; indoor/stair/shop camera modes are future quality gates.
- The current playable canon is one intro slice, not the full first act.
