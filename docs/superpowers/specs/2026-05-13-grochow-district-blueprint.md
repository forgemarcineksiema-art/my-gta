# Grochow-Inspired District Blueprint: Osiedle Za Torem

Status: DESIGN
Created: 2026-05-13
Owner: world design
Source of truth for: Grochow-scale district layout, zones, assets, gameplay hooks

## 1. Design Brief

### Real-world inspiration (filtered for gameplay)
- Grochow Centrum: high residential blocks (10-19 floors), grouped around Majdanska/Iganska/Kobielska/Rondo Wiatraczna-like urban structure
- Ground-floor services: shops, kebab, bar, workshop, driving school storefronts
- Green courtyards: playgrounds, benches, trzepak, sport courts between blocks
- Large parking areas: surface lots with worn paint, curbs, broken lamps
- Brick garage rows: older estate garages with numbered doors, stains, tool clutter
- Main arterial road: Grochowska-like spine cutting through the district
- Dirty but believable: Warsaw right-bank Praga-Poludnie mood — concrete, plaster stains, chipped curbs, posters, oil marks

### What we build (original IP, not Blok Ekipa)
- Name: Osiedle Za Torem ("The Estate Behind the Tracks")
- Identity: A compressed, fictionalized Grochow-scale district built as 4 interconnected rewir cells
- Tone: Bitter social satire with comedy under pressure from debt, bureaucracy, developers, parking wars
- Player: Returning protagonist with family/housing/debt pressure, dragged into local jobs

### Constraints
- Stylized mid-poly (not low-poly blockout, not photorealistic)
- Readable third-person navigation without HUD dependency
- Safe collision: visual complexity must not trap player or vehicle
- No D3D11 activation
- No rewrite of PlayerMotor, CameraRig, VehicleController, MissionController, SaveGame, WorldCollision, ControlContext
- Current intro mission (Bogus -> Zenon shop -> gruz -> chase -> parking) must keep working
- Incremental build: Ring 0 -> Ring 1 -> Ring 2 -> Ring 3, not a single huge expansion

---

## 2. District Layout

### Map coordinate system
- Origin (0,0,0) at current map center
- Current playable footprint: ~69m X x 60m Z (boundary walls at X=+-34, Z=-26..+26)
- Y=0 ground plane, Y is up, all units in meters

### The 4-rewir cell structure

```
                    +------------------------------------------+
                    |       REWIR 4: INDUSTRIAL EDGE           |
                    |  (rail tracks, backstreet, night jobs)   |
                    |          X: -20..+30  Z: +30..+60        |
                    +------------------+-----------------------+
                    |                  |                        |
 REWIR 3:           |   REWIR 1:       |   REWIR 2:             |
 GARAGE BELT        |   BLOK 13 HOME   |   PAWILONY MARKET      |
 + SIDE STREET      |   + COURTYARD    |   + MAIN ARTERY        |
                    |                  |                        |
 X: -45..-20        |   X: -20..+10    |   X: +10..+40          |
 Z: -35..+30        |   Z: -35..+30    |   Z: -35..+30          |
                    |                  |                        |
                    +------------------+------------------------+
```

### Scale (compressed Rockstar-style)
Full district: ~110m X x 95m Z — roughly 3x the current footprint.
Real Grochow is ~2km across; this is ~1:18 scale compression.
- Building clusters: 10-15m per block footprint
- Roads: 4-8m wide (not 12m real)
- Travel between rewirs: 20-40m walking, 10-20s driving
- Green courtyards: 15-25m across
- Parking lots: 15-20m x 10-15m

### Build sequence (ring-by-ring)

| Ring | Phase | What | Playable Area |
|------|-------|------|---------------|
| Ring 0 (current) | Intro slice | Blok 13, Zenon shop, parking, garage row, road loop | ~69x60m |
| Ring 1a | Neighbor block | Blok 11, courtyard, playground | Expand Z north +10m |
| Ring 1b | Artery + pawilony | Grochowska spine, 3-shop pavilion strip, bus stop | Expand Z south +15m |
| Ring 2 | Garage belt | 3-row garage belt, side street | Expand X west +15m |
| Ring 3 | Industrial edge | Rail tracks, warehouse, backstreet | Expand Z north +30m |


## 3. Zone Catalog (8 zones)

### Zone 1: Blok 13 Home Courtyard ("Podworko Trzynastki")
Footprint: 30m x 25m, center (-10, 0, 12) | Tag: Block | Ring 0 (current)

**Current (built):**
- [x] Blok 13 building (13x9x7m at -16,0,16)
- [x] Bogus bench (-10.8, 0, 11.9)
- [x] Spoldzielnia notice board (-20, 0, 7.9)
- [x] 3 floors window/balcony/entrance/intercom/stain/curtain dressing

**Planned (Ring 1a):**
- [ ] Trzepak (clothes-beating rack) — social anchor point, gossip source
- [ ] Small playground: 1 swing, 1 slide, sandpit (3-5m footprint)
- [ ] 3 more benches forming a "laweczka" cluster
- [ ] Concrete planter with overgrown bushes
- [ ] Dog-walking path wear (ground decals)
- [ ] Halina's balcony as formal witness point

**Gameplay hooks:** Mission start (Bogus), witness window (Halina), bench social hub

### Zone 2: Parking Lot ("Parking pod Blokiem")
Footprint: 20m x 15m, center (-7, 0, 8.6) | Tag: Parking | Ring 0 (current)

**Current (built):**
- [x] Parking surface (15x7.4m), 6 bay lines, 5 stops, 5 fence panels, 4 bollards, curbs
- [x] Oil stains, asphalt patches

**Planned (Ring 1b polish):**
- [ ] 1 broken parking stop (story: someone hit it)
- [ ] "Zakaz parkowania" sign (ignored by residents)
- [ ] Parkingowy NPC anchor (catalog service already coded)

**Gameplay hooks:** Vehicle spawn, Parkingowy interaction, Przypal hotspot

### Zone 3: Pawilon Strip ("Pawilony przy Arterii")
Footprint: 25m x 12m, center (25, 0, -28) | Tag: Shop | Ring 1b

**Design (not yet built):**
- Main arterial road: 8m wide asphalt spine, Z=-30 to -35
- 3-shop pavilion row (14x4x3.5m single-storey):
  - Pawilon 1: Sklep Zenona (existing clone)
  - Pawilon 2: Kebab "U Grubego" — red awning, meat-spit prop, graffiti
  - Pawilon 3: Komis / Lombard — barred windows, gold-trim sign, "Skup Zlota" poster
- Crosswalk, bus stop shelter, sidewalk slabs, kiosk Ruchu-like, rear bins
- Parking for 3-4 cars

**New assets:** pavilion_row_3, kiosk_ruchu, bus_stop_shelter

**Gameplay hooks:** Kebab mission, Lombard debt interaction, shop disturbance Przypal

### Zone 4: Garage Row ("Pas Garazy")
Footprint: 30m x 12m, center (-30, 0, 20) | Tag: Garage | Ring 0 -> Ring 2

**Current (built):**
- [x] Rysiek garage row (13x2.4x3.2m, 5 doors, at -18,0,23)

**Planned (Ring 2):**
- [ ] Garage Belt Row 2: 3-door row, (-30, 0, 10), 8x2.2x3m
- [ ] Garage Belt Row 3: 4-door row, (-35, 0, 28), 10x2.2x3m
- [ ] Side street: 6m wide N-S asphalt, garage lane markings
- [ ] Oil drums, tire stacks, tool crates
- [ ] Broken garage door (#3 Row 2 — attempted theft)
- [ ] "Teren Prywatny" sign, faded number plates

**New assets:** garage_row_v3

**Gameplay hooks:** Rysiek service, vehicle repair, garage disturbance Przypal

### Zone 5: Main Arterial Road ("Grochowska")
Footprint: 80m x 12m corridor, Z: -32 to -28 | Tag: RoadLoop | Ring 1b

**Current (partially built):**
- [x] Main artery surface (32x11.2m at 15,-0.006,-27.4)

**Planned (Ring 1b):**
- [ ] Extend to full 80m length (X: -30 to +40)
- [ ] 2 lanes + center line (dashed white decals)
- [ ] Sidewalks both sides (2m wide)
- [ ] Street lamps every 12m (both sides)
- [ ] Bus stop, crosswalk, route signage
- [ ] Boundary gates at west/east ends

**Gameplay hooks:** Fast route between rewirs, patrol path, chase escalation, Kierowca witness

### Zone 6: Trash / Service Courtyard ("Smietnik / Zaplecze")
Footprint: 10m x 8m, center (9, 0, -4) | Tag: Trash | Ring 0 (current)

**Current (built):**
- [x] Trash shelter, 2 colored bins, 2 cardboard stacks, fence, grass wear, stains

**Planned (Ring 1b polish):**
- [ ] Overflow trash bags, broken pallet, shopping cart prop
- [ ] Dozorca shed (small utility building)

**Gameplay hooks:** Dozorca interaction (catalog service), trash disturbance Przypal

### Zone 7: Playground / Sports Area ("Plac Zabaw / Orlik")
Footprint: 20m x 15m, center (3, 0, 15) | Tag: Block | Ring 1a

**Design (between Blok 13 and neighbor block):**
- Trzepak (3-bar rack)
- 2 swings, small slide, sandpit (2x2m, 0.15m below ground)
- 3 benches, small basketball half-court (8x5m)
- Graffiti on court surface, broken swing (vandals)
- Concrete ping-pong table

**New assets:** trzepak, swing_set, slide_small, basketball_hoop

**Gameplay hooks:** NPC hangout (youth/mothers/drunks), playground witness, fix-swing side mission

### Zone 8: Industrial / Backstreet Edge ("Za Torem")
Footprint: 40m x 20m, center (5, 0, 45) | Tag: new Industrial | Ring 3

**Design:**
- Rail track section (embankment + tracks + crossing)
- Abandoned warehouse (12x5x8m, SolidWorld)
- Gravel ground, chain-link fence, graffiti walls
- Industrial lighting (tall lamps), storage containers (2-3)
- Concrete barriers, pallets, oil drums, scrap metal pile

**New assets:** warehouse_small, rail_track_segment, container_metal, scrap_pile, chain_link_fence

**Gameplay hooks:** Night missions, hiding spots, finale staging, chase escape routes, night guard witness

---

## 4. Top-Down Blockout Map Plan

```
                     Z=+60  +--------------------------------------------+
                            |           REWIR 4: ZA TOREM               |
                     Z=+50  |   +----------+      === rail ===          |
                            |   | Warehouse|   +------+  +------+       |
                     Z=+40  |   | 12x5x8   |   |Cont.1|  |Cont.2|       |
                            |   +----------+   +------+  |------+       |
      REWIR 3:        Z=+30 +--------------------------------------------+
      GARAGE BELT           |  +---------+  +---- BLOCK 11 ---+          |
      +----------+    Z=+26 |--|GarRow 3 |  |   (neighbor)    | ==bound==|
      |GarRow 3  |          |  | 4 doors |  |   14x7x6        |          |
      |at -35,28 |    Z=+20 |  +---------+  +-----------------+          |
      +----------+          |       GARAGE LANE    PLAYGROUND            |
      | side st. |          |  +---------+    +---swings---+             |
      +----------+    Z=+16 |  |GarRow 1 |    | trzepak    |  BLOK 13   |
      |GarRow 2  |          |  |(Rysiek) |    | ==court==  |  13x9x7    |
      | 3 doors  |    Z=+10 |  +---------+    +------------+ at -16,16  |
      +----------+          +--------------------------------------------+
                            |                                    Z=+26   |
                     Z=0    |  +- PARKING -+     +---- BLOCK ------+     |
                            |  | 6 bays    |     |     SIDE        |     |
                            |  |gruz spawn |     |    6x5x11       |     |
                            |  +-----------+     |  at -25,-7      |     |
                     Z=-10  |                    +-----------------+     |
                            |        ROAD LOOP (tech oval)               |
      REWIR 2:              |  +-------------------------------+         |
      PAWILONY         Z=-18|  |  SKLEP ZENONA 8x3.5x6         |         |
      + ARTERY              |  |  at 18,-18                    |         |
                     Z=-22  +--+-----------------SHOP APRON----+---------+
                            |  |                                |         |
                     Z=-28  |==+======= MAIN ARTERY 8m ========+=========|
                            |  |  Grochowska spine Z=-30..-35  |         |
                     Z=-30  +--+-------------------------------+---------+
                            |  | +--PAWILONY--+                  |        |
                     Z=-32  |  | |Kebab  Lomb |   bus stop       |        |
                            |  | |    Zenon   |  ==crosswalk==   |        |
                     Z=-35  |  | +------------+                  |        |
                            |  +---------------------------------+        |
                     Z=-35  +--------------------------------------------+
                            X=-45    X=-20       X=0    X=+10    X=+40

                     SCALE: 1 character = ~2.5m
                     Total: ~110m X x 95m Z
```

### Blockout dimensions summary

| Element | Position (X,Y,Z) | Size (X,Y,Z) | Zone | Ring |
|---------|-------------------|--------------|------|------|
| Blok 13 | -16, 0, 16 | 13x9x7 | Block | 0 |
| Blok 11 (neighbor) | 5, 0, 17 | 14x7x6 | Block | 1a |
| Block side | -25, 0, -7 | 6x5x11 | Block | 0 |
| Playground court | 0, 0, 14 | 10x5m | Block/Playground | 1a |
| Parking | -7, 0.01, 8.6 | 15x7.4m | Parking | 0 |
| Zenon shop | 18, 0, -18 | 8x3.5x6 | Shop | 0 |
| Main artery spine | 5, -0.006, -30 | 80x11.2m | RoadLoop | 1b |
| Pavilion row | 25, 0, -32 | 14x3.5x4 | Shop/Pavilions | 1b |
| Bus stop | 22, 0, -32 | 3x2.5x2m | RoadLoop | 1b |
| Kiosk Ruchu | 28, 0, -32 | 2x2.2x1.5 | Shop | 1b |
| Garage Row 1 (Rysiek) | -18, 0, 23 | 13x2.4x3.2 | Garage | 0 |
| Garage Row 2 | -30, 0, 10 | 8x2.2x3m | Garage | 2 |
| Garage Row 3 | -35, 0, 28 | 10x2.2x3m | Garage | 2 |
| Garage lane | -30, 0, 19 | 6x48m | Garage | 2 |
| Trash courtyard | 9, 0, -4 | 7x5m | Trash | 0 |
| Warehouse | 5, 0, 50 | 12x5x8 | Industrial | 3 |
| Rail tracks | 0, 0, 45 | 20x0.3x2m | Industrial | 3 |


## 5. Asset Production List

### Existing assets (56 manifest entries — all reusable)

All current assets remain the backbone. Key categories:

| Category | Count | Examples |
|----------|-------|----------|
| Building bodies | 3 | block13_core, shop_zenona_v3, garage_row_v2 |
| Surfaces | 6 | parking_surface, road_asphalt, curb_lowpoly, curb_ramp, sidewalk_slab |
| Props (hero) | 6 | bench, notice_board, balcony_stack, trash_shelter, concrete_barrier, boundary_wall |
| Props (dressing) | 12 | bollard_red, fence_panel, lamp_post_lowpoly, street_sign, parking_stop, planter_concrete, graffiti_panel, wall_stain, shop_poster, cardboard_stack, trash_bin_lowpoly, security_camera |
| Props (box-based) | 16 | shop_sign, shop_window, shop_door, shop_awning, block_entrance, satellite_dish, garage_door_segment, garage_number_plate, trash_bins, block_window_strip, developer_sign |
| Decals | 9 | parking_line, road_arrow, crosswalk_stripe, route_arrow_head, asphalt/dirt/grass/oil patches, irregular patches |
| Glass | 2 | shop_window, block_window_strip |
| Vehicles | 1 | vehicle_gruz_e36 (mid-poly v2, 102 meshes) |

### New mid-poly assets (by priority)

#### Priority 0 — Blockout (use unit_box.obj as placeholder initially)

| Asset ID | Description | Type | Size (m) | Render | Collision |
|----------|-------------|------|----------|--------|-----------|
| pavilion_row_3 | 3-unit single-storey pavilion | .obj | 14x3.5x4 | Opaque | SolidWorld |
| kiosk_ruchu | Newspaper kiosk | .obj | 2x2.2x1.5 | Opaque | Prop |
| bus_stop_shelter | Glass + metal shelter | .obj | 3x2.5x2 | Opaque frame | None |
| trzepak | 3-bar clothes rack | .obj | 3x1.2x1.8 | Opaque | Prop |
| swing_set | 2-swing frame | .obj | 3x2.2x1.5 | Opaque | Prop |
| slide_small | Children's slide | .obj | 1.5x1.8x2.5 | Opaque | Prop |
| basketball_hoop | Backboard + hoop + pole | .obj | 1.5x3x0.3 | Opaque | Prop |

#### Priority 1 — Hero upgrades

| Asset ID | Description | Type | Size | Tags |
|----------|-------------|------|------|------|
| garage_row_v3 | Doors with seams, hinges, lamps, wear | .obj/.gltf | 13x2.4x3.2 | garage, hero_asset, midpoly_target |
| shop_zenona_v4 | Window recesses, door frame, awning geometry | .gltf | 8x3.5x6 | building, shop, hero_asset, midpoly_target |
| block13_core_v2 | Recessed windows, balcony slabs, parapet detail | .gltf | 13x9x7 | building, block, hero_asset, midpoly_target |

#### Priority 2 — Industrial (Ring 3)

| Asset ID | Description | Type | Size |
|----------|-------------|------|------|
| warehouse_small | Abandoned warehouse | .obj | 12x5x8 |
| rail_track_segment | Rail + sleepers | .obj | 4x0.3x2 |
| container_metal | Storage container | .obj | 6x2.5x2.4 |
| scrap_pile | Metal/wood scrap pile | .obj | 3x1.5x3 |
| chain_link_fence | Industrial fence panel | .obj | 3x2x0.08 |

---

## 6. Gameplay Hook Map

### Mission starts and NPC anchors

| ID | Position | NPC | Phase | Ring |
|----|----------|-----|-------|------|
| start_bogus | -12, 0, -10 | Bogus | Intro mission | 0 (existing) |
| start_zenon | 18, 0, -22.35 | Zenon | Debt ledger | 0 (existing) |
| start_rysiek | -18, 0, 21.2 | Rysiek | Vehicle repair | 2 (future) |
| start_gruby | 22, 0, -32 | Gruby | Kebab/gossip | 1b (future) |
| start_lombard | 28, 0, -32 | Jacek | Debt/pawn | 1b (future) |
| start_dozorca | 9, 0, -4 | Dozorca | Trash tasks | 0 (coded, enrich) |

### Vehicle spawns

| ID | Position | Ring |
|----|----------|------|
| gruz_rysiek | -7, 0, 9.4 | 0 (existing) |
| gruz_pawilons | 20, 0, -33 | 1b |
| gruz_garage_belt | -28, 0, 12 | 2 |

### Patrol / police loop (clockwise)

```
Artery eastbound (30,-30) -> east wall (35,0) -> north wall (30,22)
-> block north (-5,24) -> garage lane north (-30,20) -> garage lane south (-30,5)
-> parking approach (-20,5) -> road bend (-5,-10) -> artery mid (5,-30) -> loop
```

Secondary (night): Industrial edge -> warehouse -> rail crossing -> garage belt -> courtyard -> trash -> pavilion rear -> artery

### Witness windows (Przypal triggers)

| ID | Position | NPC | Reaction |
|----|----------|-----|----------|
| witness_halina | -11.45, 3.5, 12.0 | Halina | Window gossip, alarm |
| witness_zenon_cam | 21.55, 2.35, -21.28 | Zenon | Camera evidence, shop ban |
| witness_gruby | 22, 0, -32 | Gruby | Street gossip, heat |
| witness_rysiek | -18, 0, 21 | Rysiek | Vehicle memory |
| witness_dozorca | 9, 0, -4 | Dozorca | Nuisance reports |
| witness_parkingowy | -4, 0, 6 | Parkingowy | Damage reports |
| witness_kierowca | 5, 0, -30 | Kierowca | Road-loop memory |

### Przypal hotspot zones

| Zone | Trigger | Heat level | Consequence |
|------|---------|------------|-------------|
| Parking | Property damage, hit-and-run | Medium | Parkingowy shifts to wary |
| Shop front | Broken window, disturbance | High | Shop ban, Zenon goes cold |
| Garage row | Break-in, vehicle theft | High | Rysiek denies service |
| Trash | Public nuisance, fire | Low | Dozorca reports |
| Playground | Disturbance near children | Very High | Immediate patrol |
| Industrial | Trespassing, hiding | Medium | Night guard alerts |
| Main artery | Speeding, chase, crash | Medium-High | Kierowca witnesses, patrol route shift |

### Side mission / favor seeds

| ID | Zone | NPC | Short description |
|----|------|-----|-------------------|
| find_basement_key | Block | Bogus | Find key to piwnica (basement), unlock storage |
| fix_swing | Playground | Matka | Repair broken swing for local respect |
| take_out_trash | Trash | Dozorca | Move bins before collection (timed) |
| move_gruz | Garage | Rysiek | Reposition car between garage bays |
| recover_debt | Lombard | Jacek | Retrieve pawned item from debtor |
| kebab_run | Kebab | Gruby | Deliver food order across rewir |

---

## 7. Production Rules

### Visual quality
- All new assets must target stylized mid-poly: beveled/chamfered edges, recessed openings, modelled detail
- No flat primitive blockout geometry for hero assets
- Dressing pieces can use unit_box.obj as placeholder initially, tagged midpoly_target
- Follow material identity guidelines from art-direction-pivot-stylized-mid-poly.md

### Collision safety
- Visual dressing objects: zero collision (noCollision) — use addDecor/addTintedDecor
- Building bodies: SolidWorld collision with boxCollisionFromBase — simple boxes only
- Road surfaces: Ground or no collision
- Camera blockers: explicit via cameraBlocks manifest metadata
- No visual complexity should create gameplay traps

### Navigation readability
- Landmarks must be identifiable from silhouettes without HUD
- Route guidance via decal arrows, curb lines, street signs
- Witness windows visible from gameplay paths
- Crosswalks at logical pedestrian crossing points

### Build compatibility
- Current manifest format unchanged: id|modelPath|fallbackSize|fallbackColor|metadata
- New assets registered in asset_manifest.txt
- New dressing in IntroLevelDressingSections namespace
- Ring 0 (Blok 13 intro area) must remain fully playable without degradation
- Each ring is additive only — never breaks previous ring

### Current mission protection
- Bogus bench, Zenon shop, gruz spawn, parking dropoff, road loop must stay where they are
- Mission triggers (shop_walk_intro, shop_vehicle_intro, parking_dropoff_intro) unchanged
- Drive route (parking exit -> lane -> bend -> shop road -> shop entrance) preserved
- NPC anchors (Bogus, Zenon) at current positions

### Performance budget
- 1280x720 at 60fps target maintained
- New meshes: ~15-20 P0 assets at ~30-100 vertices each (currently manageable)
- Decal count: +20-30 per new rewir (within existing flat_decal budget)
- No texture pipeline yet — all palette/fallback-color based

---

## 8. Implementation Roadmap

### Phase 0: Blueprint approval (CURRENT)
- [x] District layout designed
- [x] Zone catalog defined (8 zones)
- [x] Blockout map positions specified
- [x] Asset production list prioritized
- [x] Gameplay hooks mapped
- [ ] Review and approve blueprint

### Phase 1: Ring 1a — Neighbor Block + Playground
1. Move north boundary wall from Z=+26 to Z=+35
2. Activate block13_neighbor (already placed at 5,0,17) from future_district to active
3. Add playground zone objects (trzepak, swings, slide, court, benches) using unit_box.obj placeholders
4. Add playground zone definition and landmark
5. Add playground ground patches (asphalt court, sandpit decal, grass wear)
6. Add new lamp_posts around playground
7. Register new zone in game data
8. Validate: walk from Blok 13 to playground, confirm Bogus mission still works

### Phase 2: Ring 1b — Main Artery + Pawilony
1. Move south boundary wall from Z=-26 to Z=-38
2. Extend main_artery_spine (already at 15,-0.006,-27.4) from 32m to 80m
3. Add pavilion_row_3 at (25, 0, -32)
4. Dress pavilion storefronts (signs, windows, doors, awning, posters, graffiti)
5. Add bus_stop_shelter, kiosk_ruchu, sidewalk, street lamps along artery
6. Add crosswalk decals, center-line decals, route arrows
7. Register pavilion zone, landmarks, NPC anchors (Gruby, Jacek)
8. Extend patrol route into new area
9. Validate: drive from parking to pavilions, confirm old shop Zenona still reachable

### Phase 3: Ring 2 — Garage Belt
1. Move west boundary wall from X=-34 to X=-48
2. Add Garage Row 2 at (-30, 0, 10) and Garage Row 3 at (-35, 0, 28)
3. Extend garage lane asphalt between rows
4. Dress garage rows (doors, number plates, tool props, oil stains, wall grime)
5. Register new garage zone expansions, NPC anchor (neighbor mechanic)
6. Add side street connection to main road loop
7. Validate: drive from parking through garage belt, confirm collisions safe

### Phase 4: Ring 3 — Industrial Edge
1. Extend north boundary to Z=+60
2. Add warehouse, rail tracks, containers, industrial props
3. Add gravel/industrial ground surfaces
4. Register industrial zone, night guard NPC, patrol extension
5. Validate: drive to industrial area, confirm no collision traps

---

## 9. Verification

```powershell
# Per-ring validation (same gates as current)
python tools\validate_assets.py data\assets
python tools\validate_world_contract.py data --asset-root data\assets --quiet
cmake --preset ci && cmake --build --preset ci && ctest --preset ci
.\tools\ci_smoke.ps1 -Preset ci
```

### Manual smoke per ring
1. Start game fresh
2. Walk Bogus bench -> Zenon shop (confirm path readable)
3. Enter vehicle, drive route loop (confirm no collision traps)
4. Complete intro mission (confirm all phases trigger)
5. Explore new ring content (confirm zone identity readable)
6. Check Przypal reactions in new zones (confirm witness/window hooks)
