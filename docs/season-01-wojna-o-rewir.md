# Season 01: Wojna o Rewir

## Season Pitch

Mlody returns to Block 13 to fix a family-flat debt and disappear again. Instead, every small job reveals that the estate is being squeezed by spoldzielnia procedure, developer pressure, parking conflict, and local idiots treating tiny power struggles like historic wars.

The season starts with errands. It ends with a fight over who controls the rewir.

## Act Structure

### Act 1: Wracasz i od razu przypal

Purpose:

- introduce movement, camera, interaction, shop, Przypal, and the estate tone;
- show developer pressure in the background from the first minutes;
- make the player understand that even stupid errands matter.

Missions:

1. Powrot na Rewir
2. Paragon Grozy
3. Gruzolot

### Act 2: Osiedle zaczyna cie kojarzyc

Purpose:

- unlock garage/base;
- make World Memory affect shop and gossip;
- turn parking from joke into territory;
- reveal that spoldzielnia/developer pressure is connected to garages and family-flat paperwork.

Missions:

4. Wersalka z Czwartego
5. Kto Zajal Miejsce?
6. Garaz nr 13
7. Lolek Wie, Ale Nie Powie
8. Zakaz Sklepowy

### Act 3: Wojna o parking, czyli o wszystko

Purpose:

- escalate local conflict into public estate dispute;
- force player to use movement, car, chase, Przypal, World Memory, and social choices;
- make the finale feel like a stupid but meaningful civil war over space.

Missions:

9. Spoldzielnia Kontratakuje
10. Dostawa z Lombardu
11. Dzien Bez Przypalu
12. Wojna o Rewir

## Season Systems

### Przypal

Early:

- HUD heat;
- alarm lines;
- shop/parking reactions.

Mid:

- mission dialogue references recent chaos;
- shop or garage can comment on bad behavior;
- stronger patrol/chase pressure.

Late:

- mission variants based on local heat;
- NPCs choose whether to help, mock, or block.

### World Memory

Early:

- recent event ledger only.

Mid:

- location memory: Shop, Parking, Garage, Block.

Late:

- mission consequences:
  - shop ban or discount;
  - garage access;
  - parking conflict state;
  - altered patrol hints;
  - visible small scars.

## First Five Mission Specs

These are the most important missions because they define the game language.

## 01. Powrot na Rewir

One-liner:

```txt
Mlody returns to Block 13 and immediately learns that the estate noticed him before he even unpacked.
```

Starting state:

- player arrives near bus stop / edge of estate;
- developer posters already visible;
- spoldzielnia notice near block;
- Halina comments from window.

Core gameplay:

- walk to Block 13;
- basic camera-relative movement;
- talk to Bogus;
- walk to shop marker;
- first tiny Przypal/World Memory hint.

Systems used:

- PlayerMotor;
- CameraRig;
- InteractionResolver;
- DialogueSystem;
- WorldEventLedger with low-intensity ShopTrouble.

Opening beat:

```txt
Bus leaves smoke.
Mlody stands with one bag.
Block 13 fills the frame.
Halina shouts before the player even gets control.
```

Sample dialogue:

```txt
Mlody: No dobra. Tylko na chwile.
Halina: Na chwile to moj maz wyszedl po srubokret w dziewiecdziesiatym osmym.
```

Objective chain:

- Go to Block 13.
- Talk to Bogus.
- Go to Zenon's shop.

World events:

- `ShopTrouble`, low intensity, source `mission_01_arrival`, when player reaches shop.

Consequence:

- Halina/Bogus can later mention that Mlody went straight to trouble.

Success criteria:

- player understands movement, camera, interaction prompt, and tone in under 3 minutes.

## 02. Paragon Grozy

One-liner:

```txt
A shop receipt becomes proof, debt, accusation, and local myth.
```

Story role:

- introduces Zenon Paragon;
- makes paperwork and evidence a recurring theme;
- links small shop conflict to bigger spoldzielnia/debt pressure.

Core gameplay:

- talk to Zenon;
- search near shop/trash/bench;
- talk to Lolek;
- resolve one small NPC conflict.

Systems used:

- DialogueSystem;
- InteractionResolver;
- PrzypalSystem;
- WorldEventLedger;
- NpcReactionSystem;
- later SimpleCombat or shove can be optional, not required now.

Branch options:

- talk/trade: low Przypal;
- steal/run: PublicNuisance;
- shove: PublicNuisance + possible PropertyDamage;
- pay: no heat but loses money later.

Sample dialogue:

```txt
Zenon: Boguś ma u mnie dlug.
Mlody: Ile?
Zenon: W pieniadzach czy w honorze?
Mlody: W pieniadzach.
Zenon: To jeszcze policzalne.
```

World events:

- `ShopTrouble` for argument;
- `PublicNuisance` for theft/run/shove;
- `PropertyDamage` if player hits shop props later.

Consequence:

- if chaotic, Zenon stores `shop_trust_low`;
- if calm, Zenon stores `shop_trust_ok`;
- Lolek can gossip later.

Success criteria:

- player sees that a small choice affects dialogue/reaction, not only objective completion.

## 03. Gruzolot

One-liner:

```txt
Rysiek gives Mlody the first gruz and insists it is not broken, only emotionally complex.
```

Story role:

- introduces garages and Rysiek;
- shows garages are threatened by redevelopment;
- introduces vehicle controls and gruz identity.

Core gameplay:

- walk to garages;
- enter vehicle;
- drive through parking route;
- honk;
- handbrake turn;
- bump a safe prop;
- return to garage.

Systems used:

- VehicleController;
- CameraRig Vehicle;
- GameFeedback;
- PrzypalSystem;
- WorldEventLedger;
- NpcReactionSystem.

Sample dialogue:

```txt
Rysiek: To nie zlom. To projekt.
Mlody: To ma przeglad?
Rysiek: Mialo. W czasach bardziej uczciwych.
```

World events:

- `PublicNuisance` on horn;
- `PublicNuisance` on boost noise;
- `PropertyDamage` on strong collision;
- `ChaseSeen` disabled unless the mission variant explicitly starts chase pressure.

Consequence:

- vehicle unlocked;
- garages become active location;
- Halina can comment on the gruz;
- parking Przypal can start to matter.

Success criteria:

- driving tutorial is funny but not punishing;
- player learns gruz controls without feeling the camera fights them.

## 04. Wersalka z Czwartego

One-liner:

```txt
A sofa-moving favor turns into a paperwork leak and property-damage comedy.
```

Story role:

- introduces carrying/large-object fantasy later;
- reveals spoldzielnia papers hidden in a stupid object;
- connects domestic comedy to season conflict.

Core gameplay first version:

- without full carry system yet: marker-based escort/slow movement prototype;
- move between block/staircase/parking;
- avoid collisions and Przypal triggers;
- optionally transport item with gruz later.

Systems used:

- PlayerMotor slow/interaction lock later;
- CameraRig Interaction/Indoor later;
- WorldEventLedger;
- PrzypalSystem;
- DialogueSystem.

Sample dialogue:

```txt
Mlody: Czemu to wazy jak trup?
Bogus: Rodzinna wersalka. Trzy pokolenia depresji w srodku.
```

Twist:

```txt
Documents fall out of the sofa.
They mention garage redevelopment and family-flat debt.
The sofa stops being a random job and becomes evidence.
```

World events:

- `PublicNuisance` for wall hits/noise;
- `PropertyDamage` if sofa damages car/shop/props;
- `ShopTrouble` only if route crosses shop zone.

Consequence:

- player obtains clue `redevelopment_docs_seen`;
- Halina can mention wall damage;
- Rysiek/Bogus begin treating garages as threatened.

Success criteria:

- first mission where a stupid premise reveals season plot.

## 05. Kto Zajal Miejsce?

One-liner:

```txt
Two neighbors fight over one parking spot and accidentally expose the whole estate's power structure.
```

Story role:

- makes parking the core symbol of territory;
- introduces conflicting NPC interests;
- points directly at developer/spoldzielnia plan.

Core gameplay:

- inspect disputed parking spot;
- talk to two neighbors;
- choose method:
  - persuade;
  - move car;
  - use gruz;
  - fake sign/paperwork;
  - create chaos;
- resolve with consequence.

Systems used:

- VehicleController;
- InteractionResolver;
- PrzypalSystem;
- WorldEventLedger;
- NpcReactionSystem;
- future WorldState flags.

Sample dialogue:

```txt
Neighbor 1: To moje miejsce. Staje tu od 2004.
Neighbor 2: A ja od wczoraj, ale mam blizej do klatki.
Mlody: Czyli mam rozwiazac konflikt?
Neighbor 1: Nie. Masz sprawic, zebym wygral.
```

World events:

- `PublicNuisance` for arguing/noise;
- `PropertyDamage` for car impact;
- `ChaseSeen` if patrol/chaser is triggered later;
- location memory `Parking`.

Consequence:

- `parking_side_a`, `parking_side_b`, or `parking_chaos`;
- one NPC greets/insults player later;
- patrol hint can become more likely near parking;
- developer survey markers become more visible after mission.

Success criteria:

- player understands the season is not about one joke, but about control of space.

## Later Mission List

06. Garaz nr 13

- unlocks base/garage;
- introduces repair/upgrades;
- requires spoldzielnia key or workaround.

07. Lolek Wie, Ale Nie Powie

- info mission;
- gather comfort items for Lolek;
- reveals developer/spoldzielnia link.

08. Zakaz Sklepowy

- uses shop memory;
- if chaotic earlier, Zenon bans or restricts player;
- player can repair or worsen relationship.

09. Spoldzielnia Kontratakuje

- paperwork/infiltration mission;
- introduces Prezes Kobylecki directly.

10. Dostawa z Lombardu

- vehicle delivery;
- gruz condition and Przypal matter.

11. Dzien Bez Przypalu

- ironic stealth/low-heat mission;
- everything tries to provoke chaos.

12. Wojna o Rewir

- finale;
- collect people/documents;
- public meeting collapses;
- chase through parking/garages;
- choice over documents and future rewir state.

## Production Notes

- Do not implement all 12 missions at once.
- First implement the first five as specs and then one playable mission slice.
- Every new mission should reuse existing systems before adding mechanics.
- Carrying/furniture physics can be faked first; do not build a full physics system just for mission 04.
- World Memory flags should be visible and few.
- Dialogue should be subtitle-first and short.

## Current Playable Intro Slice

The current runtime slice is a compact hybrid of:

```txt
Powrot na Rewir
+ Gruzolot
+ early ShopTrouble
```

Reason:

- uses existing movement;
- uses existing vehicle;
- uses v0.7 Przypal/World Memory;
- uses v0.8.1 objective arrows, readable shop markers, fullscreen/cursor comfort, and shorter HUD text;
- introduces the new canon without needing carrying, combat, indoor camera, or a larger map.

Manual acceptance for this slice:

- player can find Zenon's shop without being told where it is;
- gruz lock reads as mission sequence, not broken interaction;
- intro loop completes as Bogus -> Zenon -> Bogus -> gruz -> shop -> chase -> parking.
