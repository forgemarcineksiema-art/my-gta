# Grochow Target Vision Design

## Summary

`Blok 13: Rewir` should grow from the current estate-scale vertical slice into a Rockstar-style compressed district game inspired by Grochow. The target is not a one-to-one street simulator and not a direct adaptation of any existing show, cast, episode, dialogue, or scene. It is an original IP that uses Grochow as the real cultural and spatial anchor the way Rockstar uses Miami, Los Angeles, or California: recognizable pressure, condensed geography, heightened satire, and gameplay-first structure.

The current Blok 13 estate remains the heart of the project. It is the first playable rewir, the quality gate for movement/camera/driving, and the emotional home base. The long-term map expands outward only after the estate proves it can support readable navigation, Przypal, world memory, mission flow, and free-roam play without external explanation.

## Product Promise

The final game should feel like a dirty, local, social open-world action comedy about a district that remembers what the player did.

The player fantasy:

```txt
 return to Blok 13
+ get dragged into petty local jobs
+ discover district-wide pressure from spoldzielnia, developers, services, and small hustles
+ use walking, gruz driving, interaction, Przypal, and social memory to survive
+ decide what kind of rewir Grochow becomes
```

The map fantasy:

```txt
 Grochow as myth, not survey data
+ Blok 13 as home base
+ a few dense rewir zones instead of a huge empty city
+ each zone has services, witnesses, conflicts, shortcuts, and consequences
+ the world reacts locally before it reacts globally
```

## Rockstar Compression Rule

The target map should be inspired by real Grochow structure, but compressed and renamed as needed for design, satire, legal safety, readability, and production scope. It can keep recognizable patterns: a main artery, blocks and courtyards, shop/pavilion strips, market-like economy, garages, transit pressure, newer developments, municipal paperwork, and industrial edges. It should not require exact real-world street fidelity.

Use the Rockstar method:

- keep the emotional truth of the place;
- reduce travel dead space;
- exaggerate contrasts between old blocks, commerce, offices, and new developments;
- create strong routes for driving and chases;
- make every zone serve missions and systems;
- rename, merge, distort, or invent locations when gameplay needs it.

Avoid the weak version:

- huge map before the current estate is alive;
- empty streets copied for accuracy;
- direct character or episode copying from reference media;
- licensed-location dependency;
- tourist postcard treatment;
- map scale that outruns AI, content, camera, and traversal quality.

## Target District Structure

The long-term Grochow-scale map should be built as a small set of authored rewirs.

### Blok 13

Role: home base, tutorial space, emotional anchor, memory laboratory.

Gameplay jobs:

- player home and recurring mission return point;
- early Przypal and World Memory proof;
- first shop, parking, garage, window-neighbor, and bench-strategist loops;
- quality gate for dense movement, camera, interaction prompts, and gruz driving.

### Main Artery

Role: district spine for travel, chases, signage, transit noise, and escalation.

Gameplay jobs:

- fastest drive route between rewirs;
- route-reading test for mission markers and future minimap/GPS;
- patrol visibility and chase pressure;
- contrast between everyday commute and absurd mission chaos.

### Pavilions And Market Strip

Role: local economy, gossip, small hustles, debts, and social access.

Gameplay jobs:

- shops, pawn/lombard, quick jobs, favors, receipts, bans, bribes;
- compact pedestrian spaces where Przypal spreads through witnesses;
- mission chain that turns tiny trade conflicts into district-level leverage.

### Garage Belt

Role: vehicle identity, storage, repair, upgrades, and shady transport.

Gameplay jobs:

- gruz unlocks and upgrades;
- delivery, towing, parts, races, and chase escapes;
- garage politics tied to developer pressure and parking conflict.

### Spoldzielnia And Local Authority

Role: paperwork antagonist, soft power, fines, notices, and legal absurdity.

Gameplay jobs:

- mission blockers and unlocks through documents, stamps, debts, and favors;
- non-combat pressure that changes access, prices, patrol hints, or dialogue;
- comedy built around procedure rather than generic villainy.

### New Development Zone

Role: polished external pressure and visible future threat.

Gameplay jobs:

- guards, cameras, private security, survey markers, fences, propaganda;
- infiltration, sabotage, delivery, chase, and social-choice missions;
- visual contrast: clean branding over old district displacement.

### Industrial And Rail Edge

Role: night jobs, rough routes, hiding, transport, and finale-scale staging.

Gameplay jobs:

- less witnessed but higher-risk missions;
- gruz driving routes with wider turns and rough surfaces;
- storage, evidence, contraband-like errands, and climactic district conflict.

## Systems Target

### Przypal

Przypal should grow from a local heat meter into a layered social pressure system.

Layers:

- local: nearby witnesses, subtitles, quick reactions, HUD pulse;
- area: shop, parking, garage, market, authority, development zone memory;
- service: prices, bans, repair costs, help, blocked prompts;
- chase: patrol, private security, local pursuers, search/recover states;
- story: mission variants and dialogue that acknowledge repeated behavior.

Rule:

```txt
Przypal explains pressure. It never makes controls worse.
```

### World Memory

World Memory should store selected events that matter to play.

Remember:

- recent public chaos;
- location-specific trouble;
- shop trust;
- garage relationship;
- authority/developer attention;
- mission choices that alter access, dialogue, patrols, props, or prices.

Do not remember every tiny action. Remember the actions that can return as gameplay.

### Driving

Gruz driving should be one of the game's signatures.

Targets:

- few memorable vehicle classes instead of a huge car list;
- readable arcade handling with pathetic personality;
- condition bands, repairs, bad noises, smoke, rattles, horn comedy;
- handbrake and desperate gas as fun tools;
- chase routes designed around district shortcuts, parking lots, and side roads.

### Combat And Physical Chaos

The game should avoid becoming shooter-first.

Core conflict tools:

- shoves, grabs, improvised objects, short scuffles, escapes;
- vehicle pressure and environmental comedy;
- rare weapons only when story stakes justify them;
- fast recovery and readable camera during any physical joke.

### Economy

Economy should stay local and slightly desperate.

Use money for:

- repair, fuel, fines, small bribes, parts, clothes, shop items, mission fees;
- debt and paperwork pressure;
- choices between solving cleanly, cheaply, or stupidly.

Avoid RPG loot bloat.

### Audio And Dialogue

Audio should sell Grochow as a living district.

Targets:

- short subtitle-first dialogue;
- window comments, shop lines, garage noise, transit bed, distant traffic, stairwell echo;
- gruz radio and local ads;
- repeated-line cooldowns and variants;
- jokes that reveal social pressure instead of random meme noise.

## Campaign Shape

The long-term campaign can be larger than the current Season 01 outline, but it should keep mission density and local consequence.

Recommended shape:

- Act 1: return to Blok 13, learn movement, shop, garage, Przypal;
- Act 2: expand into market/pavilions and garage belt, introduce district services and economy;
- Act 3: spoldzielnia and developer pressure connect the rewirs;
- Act 4: district-wide conflict over access, parking, garages, housing, and who controls the story of Grochow;
- finale: the player chooses or causes the future state of the rewir.

Mission length target remains short: most missions should be 3-8 minutes with strong transitions between walk, drive, dialogue, chase, interaction, and consequence.

## Development Strategy

The project should not jump directly to a full Grochow map. It should expand in rings.

### Current Tooling Priority

Before expanding the map much further, the project should build the runtime editor foundation described in [Runtime Editor Design](2026-05-09-runtime-editor-design.md).

Decision:

```txt
C++ runtime editor with Dear ImGui
+ overlay JSON beside current C++ authoring
+ Python validation and asset pipeline
+ later C# data editor for missions/dialogue/catalogs
```

This is now part of the active goal loop. New Grochow rewir work should prefer editor-ready data paths over more hardcoded C++ placement.

### Ring 0: Current Vertical Slice

Make the existing Blok 13 loop readable, reactive, and fun in free roam.

Required proof:

- movement/camera/driving remain pleasant;
- Przypal has understandable causes;
- World Memory creates at least a few visible consequences;
- intro mission works without external explanation;
- compact HUD and Polish text are stable.

### Ring 1: Blok 13 As District Seed

Add one adjacent rewir slice without changing the whole map tech at once.

Candidate: pavilions/market strip or garage belt extension.

Required proof:

- data-driven location tags;
- route guidance across two rewirs;
- location-specific Przypal;
- one mission that starts in Blok 13 and resolves in the adjacent zone;
- free-roam consequence returns later.

### Ring 2: Rewir Network

Connect 3-4 rewirs through driving routes, services, and Przypal layers.

Required proof:

- service memory across shop/garage/authority;
- patrol/security pressure differs by zone;
- story missions reuse the same world systems;
- performance and camera remain stable.

### Ring 3: Compressed Grochow

Author the final district network with strong landmarks, mission chains, and persistent outcomes.

Required proof:

- every major zone has a job;
- travel is fun;
- world state changes are visible;
- the ending reflects player relationships and district pressure.

## Next Best Milestone

The next high-value milestone should not be "make all of Grochow." After the Rewir Memory Seed work, the next tool milestone is:

```txt
v0.15 Runtime Editor Foundation
```

Goal:

- make current Blok 13 and Main Artery object placement editable in-game;
- keep the C++ authored base map intact while adding `data/world/block13_editor_overlay.json`;
- make new asset instances addable from the manifest;
- add Python validation so editor output can enter the build/test loop;
- prepare the data boundary for later C# mission/dialogue/catalog tooling.

Candidate deliverables:

- overlay data structs and loader/apply path;
- Dear ImGui dev-only editor shell;
- object list, asset picker, transform inspector, save/load;
- Python overlay validator;
- tests proving override and new-instance behavior;
- smoke run proving editor changes do not break the normal game.

## Acceptance Criteria

The target vision is aligned when future work can answer yes to these questions:

- Does this protect `Blok 13` as the heart instead of abandoning it for scale?
- Does this move toward compressed Grochow rather than generic city sprawl?
- Does this create a gameplay consequence, not only a joke?
- Does Przypal explain why pressure rises?
- Does World Memory make at least one later moment feel earned?
- Does the feature preserve readable controls, camera, and HUD?
- Does the implementation create reusable structure for later rewirs?

## Open Decisions

- Final public-facing map name: literal Grochow, fictionalized Grochow, or fully fictional district with Grochow DNA.
- Exact number of final rewirs.
- Whether transit remains ambient world dressing or becomes a traversal/service system.
- Whether the finale has explicit faction endings or more subtle district-state outcomes.
