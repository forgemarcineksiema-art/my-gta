# Early Osiedle Reality Audit

Status: HONEST
Created: 2026-05-12
After: reading all world-building source files, mission flow, dressing code, asset manifest, and data overlays

---

## 1. What does the current osiedle actually contain?

### Major locations (core layout + authoring)
| What | Where | Status |
|---|---|---|
| Blok 13 (core) | `{-16, 0, 16}`, 13×9×7 | Hero asset, landmark, interactive facade |
| Blok 13 neighbor | `{5, 0, 17}`, 14×7×6 | **future_district** - visible, not usable |
| Blok 13 side | `{-25, 0, -7}`, 6×5×11 | **future_district** - visible, not usable |
| Sklep Zenona v3 | config-driven position | Hero asset, shop_identity dressing, mission trigger |
| Parking | `{-7, 0, 8.6}`, 15×7.4m | 6 bays, stops, fences, planters, line paint, curbs |
| Garage row (Rysiek) | `{-18, 0, 23}`, 13×2.4×3.2 | **future_base** - 5 doors, identity dressing, non-interactive |
| Trash shelter cluster | `{9, 0, -4}`, 3.5×1.4 | Shelter + 2 colored bins + 2 cardboard stacks + loose cardboard |
| Road loop | 4 segments (S/N/W/E) + parking connector + shop connector | Classic tech-demo driving loop |
| Main artery spine | `{15, 0, -27.4}`, 32×11.2m | **future_expansion** - present visually, dead |
| Boundary walls | 5 walls enclosing the playable box | **boundary** - keeps player in, named "future_rewir_wall_*" |

### Landmarks (7 registered)
- Ławka Bogusia (mission_start)
- Sklep Zenona (shop)
- Gruz Ryska (vehicle)
- Parking (dropoff)
- Garaże Ryska (future_base)
- Główna arteria (main_artery - future)
- Śmietniki (chaos_playground)

### Routes
- 1 drive route (parking exit → lane → road bend → shop road → shop entrance) with 5 route arrow heads
- 3 district route plans (block13→main_artery, block13→pavilions_market, block13→garage_belt - all future)
- 1 crosswalk stripe set near shop
- No walking route defined — only `grass_wear_bogus_path_0` and `grass_wear_bogus_path_1` hint at informal footpath

### NPC anchors (4 interaction points)
- Bogus (bench, `npc_start`)
- Zenon (shop front, `npc_zenon`) — appears only after intro completed
- Lolek (side position, `npc_lolek`) — appears only during Paragon mission
- Receipt holder (`receipt_holder`) — Paragon-only

### Interaction points
- 4 NPC points above
- 3 mission objective markers (shop, dropoff)
- LocalRewirService/LocalRewirFavor catalog points (runtime, condition-gated)
- World object interactions (carry, punch, vault, talk-to-object)

### Mission triggers (3)
- `shop_walk_intro` — WalkToShop → player enters 3.2m radius
- `shop_vehicle_intro` — DriveToShop → vehicle enters 4.2m radius
- `parking_dropoff_intro` — ReturnToLot → vehicle enters 5.0m radius

### Dressing clusters
- **Shop front v2**: sign, awning, 2 windows, door, security camera, graffiti panel, rolling grille, door handle, 2 price cards, 2 window dirt stains, threshold worn patch, handwritten notice, 2 window stickers
- **Block13 front facade**: 3 floors × 3 windows, 3 balconies, entrance, intercom, mailboxes, 3 stains, 2 balcony clutter items, entry notice, repair patch, 2 curtains, satellite dish
- **Block13 rear/side facade**: 3 floors × side windows, rear windows, side stains
- **Garage row**: 5 doors + 5 number plates + tool crate + oil sign + tire stack placeholder
- **Trash**: 2 bins (color-tinted green/blue) + 3 cardboard stacks + 1 loose cardboard
- **Parking**: 6 bay lines + front/back lines + 5 stops + 5 fence panels + 2 planters + 2 bollards (shop edge)
- **Ground truth**: 9 curb segments, 4 lamp posts, 2 street signs, 2 shop posters (wall), 1 shop cardboard, 1 shop planter, 1 block notice stand, 2 block planters, 2 road barriers
- **Ground patches**: 8 asphalt patches, 6 grass wear patches, 1 dirt patch — all `irregular_*` with tinting
- **Sidewalks**: shop front + block front slabs (11.5m + 15m strips)

### Obvious technical/test elements
- `developer_survey_marker` at `{-2.8, 0, 7.6}` — literal dev sign in-world (tagged `developer`)
- `fallbackStyles[]` with 23 colored-box entries — debug rendering for missing meshes
- Road loop geometry — four orthogonal segments forming a rectangle, explicitly a driving test track
- Boundary walls on all 5 sides — classic "box it in" staging
- Two `future_district` blocks present but empty — visual placeholders
- Main artery surface present but tagged `future_expansion` — dead road surface
- `route_arrow_head` painted decals — clearly developer guidance, not natural landmark
- `block13_neighbor` and `block13_side` reuse `block13_core` mesh — identical blocks placed at scale variations

---

## 2. What currently feels like a real place?

**Shop Zenona front (strong)**
The shop front is the most convincingly real cluster in the world. Each element has purpose:
- Sign identifies the business
- Awning provides physical shelter logic
- Windows frame the interior-hint
- Door at center with worn threshold (the `lived_in_microdetail` tag is earned here)
- Security camera suggests neighborhood surveillance culture
- Handwritten notice and price cards imply actual commercial activity
- Window dirt stains + stickers create asymmetry and use
- Graffiti panel on side wall adds edge

The tag system supports this: `shop_hero_v2`, `lived_in_microdetail`, `surface_breakup`, `readable_notice`, `story_dressing`.

**Block13 front facade (good)**
3 floors of windows, balconies with clutter, entrance with intercom and mailboxes, stains, satellite dish, curtains — creates a believable "people live here" surface. The asymmetry of which balconies have clutter vs which don't is good. The repair patch tells a small story of maintenance.

**Trash cluster (adequate)**
The shelter + colored bins (municipal color coding) + cardboard stacks + loose cardboard + grass wear underneath = a coherent functional zone. The `public_nuisance` tag acknowledges neighborhood reality.

**Ground patches (good intent, mixed execution)**
Asphalt repair patches, grass wear paths, dirt under fence — these communicate "this place has been used for years." The grass wear path `grass_wear_bogus_path_0/1` hints at an informal walking route from bench toward parking.

**Dialogue tone (promising)**
The core MissionController dialogue has the right voice:
- "Wróciłeś. Idź do Zenona i sprawdź, czy jeszcze liczy długi."
- "Boguś? Ja go lubię. Najbardziej z daleka od zeszytu długów."
- "To nie złom. To Borex Kombi po przejściach."
- "Gruz pod sklepem? Kamerka wszystko nagrała, kolego."
- "Ja to widzę z okna! Zaraz będzie telefon!"

This is Polish osiedle voice. It's dry, debt-obsessed, petty-surveillance.

---

## 3. What currently feels like a technical test map?

**Road layout (critical)**
The "road loop" is a rectangle: south segment (48×4.2m), north segment (22×3m), west segment (4×44m), east segment (4×47m) — plus parking/shop connector roads. This is a driving-test oval with right-angle turns. No real osiedle in Poland has orthogonal loop roads. The main artery surface sits as a dead rectangle with a `future_expansion` tag. The boundary walls box the entire 68×54m area into a rectangle.

**"Future" placeholders everywhere**
- 2/3 bloks are `future_district` (neighbor + side) — same mesh as core, different scales
- Garage row is `future_base` — visible, dressed, has plates, has tools — but fully non-interactive
- Main artery is `future_expansion` — large road surface that does nothing
- All boundary walls named `future_rewir_wall_*` — the name itself states "not done yet"
- 3/4 district rewir plans have `playableNow: false`

**Empty space between landmarks**
From Bogus bench to shop is roughly 30+ meters of mostly empty ground. The `grass_wear_bogus_path` patches exist but don't form a continuous readable route. Between parking and shop connector road is open dead space. The road loop center is a grass void.

**Developer survey marker**
A literal `developer_sign` object at `{-2.8, 0, 7.6}` tagged `developer`. It's a self-aware joke, but it reinforces "this is a dev build."

**Fallback prop system**
The `FallbackPropStyle` array defines colored boxes for 23 objects — necessary for rendering when meshes fail, but the fact that 23 objects have fallback colors means a lot of the world appeared as debug geometry at some point.

**Route arrows as world decals**
7 `route_arrow_head` decals painted on the ground pointing the driving route. This is dev guidance posing as world art. Crosswalk stripes are similarly planted at exact grid positions.

**Grid-aligned placement**
Virtually all core objects use grid-coordinate positions (blocks at multiples of 5 or 8, roads at exact boundaries, parking at X=-7). This yields clean collision and math, but zero organic irregularity.

**No interior**
Blok13 has an entrance, intercom, mailboxes — but the entrance is a black plane. Shop has windows and a door — but the door leads nowhere. There is zero interior space, zero threshold crossing.

---

## 4. What is missing for the first area to feel like "osiedle"?

### Lived-in local identity
- No personal items visible (single shoe? shopping bag? kid's toy? car cover? broken umbrella on balcony?)
- No seasonal or temporal markers (życzenia świąteczne still up in May? faded banner? election poster?)
- No evidence of different residents — 3 balconies have clutter, but the building has many more windows
- No dog-related anything (famous feature of Polish osiedle)

### Readable landmark hierarchy
Right now only the shop sign and blok serve as "where am I" anchors. Missing:
- A large, unique tree or overgrown bush
- A mural, large graffiti, or distinct wall color on one building face
- An obvious meeting point (not just Bogus bench — a "pod spożywczakiem" or "przy trzepaku" or "na schodkach" type landmark)
- A broken/odd element visible from multiple angles (crooked lamp post, leaning fence section, abandoned cart)

### Believable walking/driving paths
- No sidewalk from block to shop
- Bogus bench → shop route has no continuous visual thread — you walk on undefined ground
- No shortcut through the fence, no beaten path behind the blok
- The driving route works mechanically (arrows, lane widths, triggers) but doesn't feel like navigating a real street — it's a track
- No pedestrian crossing logic — where would a babcia cross?

### NPC/social anchors
- Only Bogus is visually present at world start
- Zenon and Lolek spawn only after intro completion — the world is empty before that
- No ambient NPCs — no one walking a dog, standing at the shop corner, working on a car near garages, smoking by the trash
- No evidence of NPC daily rhythm (morning bread run, afternoon bench sitting, evening trash take-out)

### Local conflict hooks
- World events system exists (`PublicNuisance`, `ShopTrouble`, `PropertyDamage`, `PrzypalNotice`) but no physical hooks anchor them
- No visual evidence of ongoing tension: no broken bottle from last night, no "zakaz parkowania" sign clearly recently ignored, no passive-aggressive note taped to entrance
- The chase is the only overt conflict, and it triggers mechanically (radius entry) not socially

### Clutter with purpose
The dressing code is thorough but follows symmetrical templates:
- Every parking bay has a stop
- Every floor has 3 windows
- Every garage door has a number plate
- Every facade stain is placed per-floor

Real osiedle clutter is uneven — one parking bay has a broken stop, one balcony has 5 flower pots and another has none, the trash shelter overflows on Tuesday but not Wednesday.

### Mission motivation
- The player is told what to do, not shown why to care
- Bogus and Zenon are names without faces — no pre-mission establishment of who they are
- The debt ("zeszyt długów") is mentioned but never shown, never has stakes
- The chase trigger is absurd ("camera recorded everything") but absurdity without context is just noise

### Scene composition
- All structures are axis-aligned — no rotated block, no angled shop, no irregular lot shape
- Everything fits in a flat XZ plane with slight Y offsets for road/parking — no terrain variation, no sloped approach, no sunken area
- The boundary walls reinforce the "this is a box" feeling — they're visible from most positions

---

## 5. Why do the first 1-2 mission flows currently not work as gameplay/narrative?

### Phase-by-phase honesty

**Phase 1: WaitingForStart**
- Player spawns near Bogus bench, sees "E: pogadaj z Bogusiem"
- What happens: Press E, mission starts, Bogus says "go check Zenon's debt ledger"
- What's missing: The player has no idea who Bogus is, who Zenon is, what debt, why they care. The interaction is a button press that starts a chain of objectives.

**Phase 2: WalkToShop**
- Player walks ~30m across undefined ground to shop marker
- One NPC reaction line possible (Halina: "idź jak człowiek")
- What's missing: Nothing happens during transit. No small discovery, no world clue, no micro-story. The walk is dead air.

**Phase 3: ReturnToBench**
- Player reaches shop, Zenon says "I like Bogus, from as far away from the debt ledger as possible" — then told "return to Bogus"
- What's missing: The player just spent 30 seconds walking to the shop, received one line of dialogue, and is now told to walk back. The shop visit has no interaction — you don't enter, don't see Zenon, don't do anything. This creates a "why did I walk here" feeling.

**Phase 4: ReachVehicle**
- Bogus says "shop's alive, plan works, go get Rysiek's gruz"
- What's missing: What plan? The player has done nothing except walk. "Plan działa" implies the player accomplished something, but they haven't.

**Phase 5: DriveToShop**
- Rysiek says "this isn't scrap, it's Borex Kombi with history, drive it to the shop"
- Player drives the same route they just walked, to the same destination they just visited
- What's missing: This is the second visit to the shop. The destination hasn't changed, no new content awaits. The driving exists because the game needs a driving segment, not because the story demands it.

**Phase 6: ChaseActive**
- Arriving at shop triggers chase: Zenon says "camera recorded everything," Halina says "I see it from the window, the phone call is coming"
- What's missing: The chase has potential but the trigger motivation is weak. A grainy camera and one window-watcher suddenly escalate to a full vehicle pursuit. There's no identity to the pursuer — who is chasing you? Some random car? A neighbor? The police? The "przypał" system is sophisticated (5 bands, escalation) but the narrative hook is thin.
- The chase is a mechanic test: can you outrun an AI pursuer? It works for that, but doesn't feel like an osiedle social consequence.

**Phase 7: ReturnToLot**
- If player escapes chase, told "return to parking"
- What's missing: After a chase, returning to parking is anticlimactic. The trigger radius check means you just drive back and park. There's no payoff — no character reaction, no change in the world, no new information.

**Phase 8: Completed**
- "Odcinek zaliczony. Rewir już cię kojarzy."
- What's missing: The mission ends with a system message saying "the rewir knows you now" — but nothing in the world has changed to reflect this.

### Separated analysis

| Aspect | Verdict |
|---|---|
| **Mechanics test flow** | Works: walk → interact → walk → enter vehicle → drive → escape chase → park. All systems function. |
| **Actual player motivation** | Missing: The player has no personal stake. They are a courier for people they don't know. |
| **World storytelling** | Missing: The world doesn't change during the mission. No door opens, no NPC moves, nothing appears or disappears. |
| **Objective clarity** | Adequate: The UI tells you what to do. The physical route has arrows. The triggers have radius feedback. |
| **Comedy/absurdity** | Present but thin: Halina's window surveillance, Rysiek's car pride, Zenon's debt ledger. The tone is right but the situations are too sparse — comedy needs setup → payoff rhythm that the current linear chain doesn't provide. |
| **Local stakes** | Missing: There's no reason this happens in THIS osiedle vs any other. No neighborhood reputation, no local history, no personal relationship to defend or damage. |

---

## 6. What should NOT be built yet?

| Don't build | Why |
|---|---|
| **New big mission arc** | The current 6-phase flow barely works as narrative. Adding more phases before fixing the core motivation would create more content the player has no reason to engage with. |
| **GLTF/material pipeline** | The asset system works for current needs (OBJ + colored boxes). 55/56 assets use OBJ. The one GLTF (gruz_e36) already renders. Material quality won't fix world emptiness. |
| **Main D3D11 renderer** | Explicitly parked per milestone summary. Verification passes. Resume only with concrete driving need. |
| **Huge map expansion** | The current 68×54m box doesn't feel like a place yet. Expanding the box before the content works would create a 2x larger empty space. |
| **Random prop spam** | Dressing code already places ~150 objects. More objects placed without purpose will make the world feel cluttered, not lived-in. |
| **New NPCs** | Adding more NPCs before the existing ones have clear roles, schedules, or social relationships would dilute rather than enrich. Bogus, Zenon, Rysiek, Halina, Lolek are enough characters for the current footprint. |
| **Interiors** | Before the exterior feels like a real neighborhood, don't build what's inside buildings. Interiors are a separate scope with different needs. |
| **Main artery / pavilions / garage belt** | All tagged `playableNow: false` for good reason. The first rewir (Block13) isn't finished. |

---

## 7. Recommended 3 small next improvements

### Improvement 1: Make Bogus bench → shop walk feel physically readable

**What**: Connect the two existing grass-wear patches (`grass_wear_bogus_path_0` and `grass_wear_bogus_path_1`) into a continuous informal "deptak" — a beaten path worn into the grass by years of residents walking the same shortcut. Add exactly one midpoint marker: a bent street sign, a notable graffiti stain on the blok side wall visible from the path, or a single dead tree/stump.

**Why**: The player's first autonomous action is walking from bench to shop. Right now this is just crossing empty space toward a marker. A readable footpath says "people live here and walk this direction." The midpoint marker gives the player something to notice on their second pass. This creates orientation without debug UI.

**Size**: Add ~3-4 ground patches + 1 midpoint object. No new asset needed (existing `irregular_grass_patch` + `wall_stain` or `street_sign` or `planter_concrete`).

**Test**: Walk from Bogus bench to shop. Can you follow the path visually without looking at the marker HUD? Does a midpoint object catch your eye from 15m out?

### Improvement 2: Reframe first mission objective text with local motivation

**What**: Change the WaitingForStart objective from generic "Pogadaj z Bogusiem" to something that gives the player a small personal reason. Change the WalkToShop objective from "Idź do sklepu Zenona" to something that hints at neighborhood stakes. The underlying mechanics don't change — only the text the player sees.

**Why**: The current objective text is mechanically accurate ("go here") but narratively empty. A small reframe in the MissionController default objective strings gives the player a reason to care before any dialogue fires. Combined with the existing Bogus dialogue ("check if he still counts debts"), the player can piece together: "I have a debt situation → Zenon holds the ledger → I should check."

**Size**: Change ~4 string constants in `MissionController.cpp:268-290`. No new systems, no data file changes.

**Test**: Start game fresh. Read the objective. Does it make you curious about what happens at the shop? Does it connect to the Bogus dialogue that fires after?

### Improvement 3: Add one visible social conflict hint near the shop

**What**: Place a single world object that tells a story without dialogue. Examples: a `wall_stain` tinted like fresh repair plaster on one shop window corner (someone threw something), a `shop_poster` torn in half (someone angry at Zenon), or a `cardboard_stack` next to the shop door with a passive-aggressive note ("Zenon — oddaj klucz od piwnicy. — Halina").

**Why**: The shop is the mission's most visited destination (walk phase + drive phase). Right now it's dressed but static. One visible piece of unresolved tension gives the player something to notice on approach, creates curiosity ("what happened here?"), and establishes that this neighborhood has history before the player arrived. It connects Halina's "I'm watching from the window" line to something physical.

**Size**: Add 1 object + tint in `IntroLevelGroundTruthDressing.cpp` or `IntroLevelIdentityDressing.cpp`. No new asset type needed. Could also add a `pushObjectiveLine` call when the player first approaches the shop if interaction is desired, but physical-only is sufficient.

**Test**: Walk to shop. Does something catch your eye that doesn't look "clean"? Does it make you wonder who did it and why? Does it connect to dialogue you've heard or will hear?

---

## Verification

```
git diff --check
```

*(Run after audit document is committed — no code changes were made.)*

---

## Summary

| Question | Short answer |
|---|---|
| What does the osiedle contain? | 3 blocks (1 playable), 1 shop, 1 parking, 5 garage doors, 1 trash cluster, a rectangular road loop, boundary walls, ~150 dressing objects, 4 NPC anchors, a 3-trigger mission flow, a chase system, and a Paragon post-mission |
| What feels real? | Shop front dressing, block facade microdetails, trash cluster, dialogue tone |
| What feels like a test map? | Road loop geometry, orthogonal placement, future_district blocks, dead main artery, boundary box, developer sign, route arrows |
| What's missing? | Lived-in identity, readable landmark hierarchy, continuous footpaths, ambient NPC presence, visible conflict, uneven clutter, player motivation, scene composition |
| Why don't missions work? | The flow is a marker-to-marker fetch chain with no personal stakes, no world change, sparse comedy, and a chase trigger that escalates from nothing |
| What not to build? | New mission arcs, GLTF pipeline, D3D11 renderer, map expansion, random props, new NPCs, interiors, future districts |
| Top 3 improvements? | (1) Continuous readable footpath bench→shop, (2) Reframed objective texts with local motivation, (3) One visible social conflict near the shop |

**Explicit statement: No code or content has been changed by this audit.** The audit is analysis-only. Implementation should follow after review.

---

## 8. Implementation (2026-05-12): First 3 audit fixes applied

### Fix 1: Continuous footpath Bogus bench → Zenon shop

**What was done:**
- Added 4 new `irregular_grass_patch` objects (`grass_wear_bogus_path_2` through `_5`) extending the existing two patches south and southeast toward the shop, forming a readable ~35m informal walking route.
- Each patch uses subtle scale variation (1.9–2.2m width, 2.8–3.4m length) and slight yaw rotation (−0.10 to +0.28 rad) to avoid looking like a clean debug line.
- Added one midpoint landmark: `footpath_weathered_planter` — a tinted `planter_concrete` object at `{-5.5, 0, -1.4}` that serves as a visible orientation anchor roughly halfway along the path.
- All patches and the planter use no collision (decorative only) and follow the existing `addGroundPatch`/`addTintedDecor` patterns.

**Files changed:** `src/game/IntroLevelGroundTruthDressing.cpp`

### Fix 2: Reframed early objective text

**What was done:**
- Changed 4 mission objective strings in `MissionController::objectiveText()`:
  - `WaitingForStart`: "Pogadaj z Bogusiem" → "Spytaj Bogusia o zeszyt dlugow"
  - `WalkToShop`: "Idz do sklepu Zenona" → "Sprawdz u Zenona, czy dlug nadal zyje"
  - `ReturnToBench`: "Wroc do Bogusia" → "Wroc do Bogusia z wiesciami"
  - `DriveToShop`: "Podjedz pod sklep" → "Podjedz gruzem pod Zenona"
- Each new text hints at the debt ledger, local stakes, or vehicle identity rather than giving a bare "go here" instruction.
- No mission phases, triggers, or mechanics were changed.

**Files changed:** `src/core/MissionController.cpp`

### Fix 3: Social-conflict hint near the shop

**What was done:**
- Added `shop_window_repair_patch` — a tinted `wall_stain` at the shop front, positioned below the left window, with a light plaster/concrete tint (122, 118, 108, 172 alpha) that stands out from the building base color.
- Tagged with `lived_in_microdetail`, `surface_breakup`, `story_dressing` to match the existing dressing classification.
- The patch suggests something was thrown at the shop window and hastily repaired — visible on the player's first approach, connecting to Halina's window surveillance and the neighborhood tension without requiring any dialogue.

**Files changed:** `src/game/IntroLevelIdentityDressing.cpp`

### What remains unsolved (not addressed in this pass)
- Road loop geometry is still a tech-demo rectangle
- Future_district blocks still exist as visual placeholders
- No ambient NPCs or social rhythms
- Mission flow is still a linear fetch chain (only motivation text improved)
- No terrain variation or organic scene composition
- No interior spaces
- D3D11 remains parked and untouched

### Manual verification steps
1. Start fresh near Bogus bench (no save loaded).
2. Without relying on the HUD marker, check if the grass-wear path creates a visible walking direction toward the shop (south/southeast).
3. Walk the path — confirm the weathered planter midpoint marker is visible as a landmark during transit.
4. Check the starting HUD objective — confirm it says "Spytaj Bogusia o zeszyt dlugow" (not "Pogadaj z Bogusiem").
5. Talk to Bogus, start mission, check WalkToShop objective says "Sprawdz u Zenona, czy dlug nadal zyje".
6. Walk to the shop — confirm the repair patch below the left window is visible and distinguishable from building color.
7. Confirm: no new mission arc, no D3D11 changes, no renderer changes, no map expansion.
8. Confirm: `--renderer d3d11` still returns error (not activated).

---

## 9. Implementation correction after screenshots (2026-05-12)

### Objective source mismatch found and fixed

**Root cause:** The visible HUD objective text was being overridden by `data/mission_driving_errand.json` via `WorldDataLoader.cpp:618` → `setObjectiveOverride()`, which takes precedence over `MissionController::objectiveText()` defaults. Updating only the C++ switch statement in the prior implementation pass did not change what the player actually sees.

**Fix:**
- Updated all 6 phase objectives in `data/mission_driving_errand.json` to match the desired local-motivation tone.
- Updated test expectations in `tests/game_support_tests.cpp` to match the new strings.

**Exact visible objective text after fix:**

| Phase | Old (visible before) | New (visible after) |
|---|---|---|
| WaitingForStart / WalkToShop | "Sprawdź sklep Zenona na piechotę." | "Podejdź do Zenona — sprawdź, czy dług jeszcze żyje" |
| ReturnToBench | "Wróć do Bogusia pod blokiem." | "Wróć do Bogusia z wieściami" |
| ReachVehicle | "Wsiądź do auta na parkingu." | "Wsiądź do gruza" |
| DriveToShop | "Podejdź pod sklep Zenona." | "Podjedź gruzem pod Zenona" |
| ChaseActive | "Zgub osiedlowy pościg." | "Zgub przypał" |
| ReturnToLot | "Wróć na parking pod blokiem." | "Wróć na parking" |

### Footpath readability correction

**Root cause:** The old path patches (path_2 through shop_shortcut) used the `addGroundPatch` auto-tint system which applied a green-ish tint (R=82, G=105, B=73, A=118) that blended into the grass instead of standing out. The patches also formed a nearly horizontal line (Z range: +2.2 to -12.0) that did not visually angle toward the shop (Z=-22.35), so the path read as random scuffs rather than a directional shortcut.

**Fix:**
- Patches 2–5 and shop_shortcut now use `addTintedDecor` with a warm earth tint (R=108, G=96, B=68, A=132) — noticeably warmer and darker than the surrounding grass.
- Patch positions redesigned to form a clear southeast curve from bench area toward shop:
  - path_2: `{-9.5, _, -5.5}` — heading south from bench
  - path_3: `{-5.0, _, -9.0}` — starting southeast diagonal
  - path_4: `{-0.5, _, -13.0}` — midpoint
  - path_5: `{4.5, _, -16.5}` — past midpoint
  - shop_shortcut: `{11.0, _, -20.0}` — approaching shop
- Patches are slightly larger (2.3–2.5m wide, 3.8–4.0m long) with controlled 0.3–0.9m overlap for continuity.
- Path_0 and path_1 (blok → bench area wear) retained as-is.
- All patches tagged `footpath_guide` for potential future HUD path hinting.
- Collision remains off (decorative only).

### Midpoint landmark correction

**What changed:**
- `footpath_weathered_planter` moved from `{-5.5, _, -1.4}` to `{-0.5, _, -13.2}` — now placed directly along the corrected footpath at the visual midpoint between bench and shop.
- Scale increased from `{0.65, 0.38, 0.48}` to `{1.18, 0.54, 0.78}` — nearly 2× volume, visible from 10–15m.
- Tint darkened from `{98, 102, 94}` to `{74, 78, 70}` — stronger visual contrast against grass.
- Tagged `landmark` to match the landmark system expectations.

### Shop conflict hint correction

**What changed:**
- `shop_window_repair_patch` scale increased from `{1.05, 0.36, 0.038}` to `{1.25, 0.48, 0.040}` — wider and taller.
- Tint darkened and made more opaque: from `{122, 118, 108, 172}` to `{96, 88, 76, 198}` — reads as a distinctly patched area against the shop wall base color.
- Added `shop_conflict_notice` — a tinted `notice_board` positioned at eye level near the patched window, using a warm paper-yellow tint `{218, 192, 144}` that suggests a handwritten or passive-aggressive announcement. Reinforces the "something happened here" read without requiring dialogue.

### Remaining issues honestly unchanged

- Road loop is still a rectangular tech-demo track.
- World is still sparse — no terrain variation, no ambient clutter rhythm.
- Future districts (neighbor block, side block, main artery) are still `future_district` / `future_expansion` placeholders.
- No ambient NPCs or social rhythms — only Bogus is visible at world start.
- Mission flow is still a fetch-chain walk → walk-back → drive → chase → return.
- No interiors, no threshold crossing.
- D3D11 renderer remains parked — no `--renderer d3d11` activation.
- No new mission arc, no random prop spam, no map expansion.

**Verification commands:**
```
git diff --check
cmake --preset ci-core && cmake --build --preset ci-core && ctest --preset ci-core
cmake --preset ci && cmake --build --preset ci && ctest --preset ci
.\tools\ci_smoke.ps1 -Preset ci
.\tools\renderframe_capture_replay.ps1 -Preset ci -Build -DumpVersion v2
```

### Files changed

| File | What |
|---|---|
| `data/mission_driving_errand.json` | All 6 phase objectives replaced with local-motivation text |
| `tests/game_support_tests.cpp` | Expected objective strings updated to match new JSON data |
| `src/game/IntroLevelGroundTruthDressing.cpp` | Footpath patches 2–5 + shop_shortcut redesigned with warm tint and correct curve; midpoint planter enlarged, moved, darkened |
| `src/game/IntroLevelIdentityDressing.cpp` | Shop repair patch made larger/more-opaque; added conflict notice board |
| `docs/early-osiedle-reality-audit.md` | This section added |
