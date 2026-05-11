# World Data

Runtime world and mission schemas now load through `WorldDataLoader`.

The current boot path reads `data/map_block_loop.json` and `data/mission_driving_errand.json`, applies spawn/mission metadata to the intro slice, and validates authored asset references before the game starts.
Mission dialogue lines can also be supplied through `data/world/mission_localization_pl.json` when `lineKey` is used in `mission_driving_errand.json`.
