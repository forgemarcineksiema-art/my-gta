#include "IntroLevelAuthoring.h"

namespace bs3d {

namespace {

WorldObjectTint color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
    return {r, g, b, a};
}

const WorldObject* findObject(const IntroLevelData& level, const char* id) {
    for (const WorldObject& object : level.objects) {
        if (object.id == id) {
            return &object;
        }
    }
    return nullptr;
}

struct FallbackPropStyle {
    const char* objectId = "";
    WorldObjectTint fill{};
    bool collidable = true;
};

} // namespace

void IntroLevelAuthoring::addGameplayData(IntroLevelData& level) {
    level.zones = {
        {"zone_shop", WorldLocationTag::Shop, level.shopPosition, {9.0f, 3.0f, 9.0f}, 9.0f, 100, WorldZoneShapeKind::Sphere},
        {"zone_parking", WorldLocationTag::Parking, {-7.0f, 0.0f, 8.6f}, {19.0f, 2.0f, 12.0f}, 11.5f, 80, WorldZoneShapeKind::Sphere},
        {"zone_garages", WorldLocationTag::Garage, {-18.0f, 0.0f, 23.0f}, {16.0f, 2.0f, 6.0f}, 8.0f, 90, WorldZoneShapeKind::Sphere},
        {"zone_garage_belt", WorldLocationTag::Garage, {-33.0f, 0.0f, 19.0f}, {30.0f, 3.0f, 24.0f}, 18.0f, 85, WorldZoneShapeKind::Sphere},
        {"zone_trash", WorldLocationTag::Trash, {9.0f, 0.0f, -4.0f}, {7.0f, 2.0f, 5.0f}, 6.0f, 85, WorldZoneShapeKind::Sphere},
        {"zone_road_loop", WorldLocationTag::RoadLoop, {0.0f, 0.0f, 0.0f}, {80.0f, 2.0f, 72.0f}, 42.0f, 60, WorldZoneShapeKind::Sphere},
        {"zone_main_artery", WorldLocationTag::RoadLoop, {5.0f, 0.0f, -30.0f}, {82.0f, 2.0f, 14.0f}, 38.0f, 76, WorldZoneShapeKind::Sphere},
        {"zone_pavilions", WorldLocationTag::Shop, {25.0f, 0.0f, -36.0f}, {18.0f, 3.0f, 8.0f}, 12.0f, 90, WorldZoneShapeKind::Sphere},
        {"zone_block13", WorldLocationTag::Block, {-16.0f, 0.0f, 16.0f}, {18.0f, 3.0f, 13.0f}, 10.0f, 40, WorldZoneShapeKind::Sphere},
        {"zone_block11", WorldLocationTag::Block, {5.0f, 0.0f, 17.0f}, {16.0f, 3.0f, 10.0f}, 9.0f, 35, WorldZoneShapeKind::Sphere},
        {"zone_playground", WorldLocationTag::Block, {-6.0f, 0.0f, 14.0f}, {12.0f, 3.0f, 10.0f}, 7.0f, 50, WorldZoneShapeKind::Sphere}
    };

    level.landmarks = {
        {"lm_bogus_bench", "Ławka Bogusia", level.npcPosition, "mission_start"},
        {"lm_shop_zenona", "Sklep Zenona", level.shopEntrancePosition, "shop"},
        {"lm_gruz", "Gruz Ryska", level.vehicleStart, "vehicle"},
        {"lm_parking", "Parking", level.dropoffPosition, "dropoff"},
        {"lm_garages", "Garaże Ryska", {-18.0f, 0.0f, 23.0f}, "future_base"},
        {"lm_main_artery", "Glowna arteria", {5.0f, 0.0f, -30.0f}, "main_artery"},
        {"lm_trash", "Smietniki", {9.0f, 0.0f, -4.0f}, "chaos_playground"},
        {"lm_block11", "Blok 11", {5.0f, 0.0f, 17.0f}, "block11"},
        {"lm_playground", "Plac zabaw", {-6.0f, 0.0f, 14.0f}, "playground"},
        {"lm_pavilions", "Pawilony", {25.0f, 0.0f, -34.0f}, "pavilions"},
        {"lm_bus_stop", "Przystanek", {22.0f, 0.0f, -29.2f}, "bus_stop"},
        {"lm_garage_belt", "Pasaz Garazowy", {-33.0f, 0.0f, 19.0f}, "garage_belt"}
    };

    level.districtRewirs = {
        {"block13",
         "Blok 13",
         "Home base and first playable rewir for movement, camera, memory, services, and gruz handling.",
         {-16.0f, 0.0f, 16.0f},
         18.0f,
         true,
         true,
         false,
         true,
         true,
         "",
         {"return point", "local services", "world memory lab", "intro mission loop"}},
         {"main_artery",
          "Glowna arteria",
          "Compressed district spine for readable travel, chases, transit noise, and escalation.",
          {5.0f, 0.0f, -30.0f},
          38.0f,
         false,
         false,
         true,
         false,
         false,
         "Droga",
         {"fast rewir travel", "chase pressure", "route signage", "patrol visibility"}},
        {"pavilions_market",
         "Pawilony i bazarek",
         "Local economy strip for gossip, debts, receipts, services, and small hustles.",
         {25.0f, 0.0f, -36.0f},
         18.0f,
         false,
         false,
         false,
         true,
         false,
         "Pod sklep",
         {"shops and bans", "small favors", "gossip spread", "local prices"}},
        {"garage_belt",
         "Pas garazy",
         "Vehicle identity zone for repairs, parts, shady transport, and garage politics.",
         {-34.0f, 0.0f, 28.0f},
         20.0f,
         false,
         false,
         true,
         false,
         true,
         "Garaze",
         {"repair access", "parts errands", "shortcut driving", "garage reputation"}}
    };

    level.districtRoutePlans = {
        {"route_block13_main_artery",
         "block13",
         "main_artery",
         true,
         true,
         {{level.vehicleStart, 3.0f, "Gruz pod blokiem"},
          {level.parkingExitPosition, 3.0f, "Wyjazd"},
          {level.parkingLanePosition, 3.0f, "Alejka"},
          {level.roadBendPosition, 3.2f, "Luk przy drodze"},
          {level.shopRoadPosition, 3.6f, "Wylot"},
          {{8.5f, 0.0f, -25.0f}, 2.0f, "Brama arterii"},
          {{14.5f, 0.0f, -28.0f}, 3.0f, "Pas arterii"},
          {{22.0f, 0.0f, -30.0f}, 4.8f, "Glowna arteria"}}},
        {"route_block13_pavilions_market",
         "block13",
         "pavilions_market",
         true,
         true,
         {{{-10.0f, 0.0f, 10.5f}, 3.0f, "Spod Bloku 13"},
          {level.parkingLanePosition, 3.0f, "Alejka"},
          {level.shopEntrancePosition, 4.2f, "Pod sklep"},
          {{25.0f, 0.0f, -30.0f}, 4.4f, "Pawilony"}}},
        {"route_block13_garage_belt",
         "block13",
         "garage_belt",
         true,
         true,
         {{{-10.0f, 0.0f, 10.5f}, 3.0f, "Spod Bloku 13"},
          {{-18.0f, 0.0f, 23.0f}, 3.4f, "Garaze Ryska"},
          {{-34.0f, 0.0f, 28.0f}, 4.2f, "Pas garazy"}}}
    };

    level.visualBaselines = {
        {"vp_start_bogus", "Start przy ławce Bogusia", {-7.0f, 2.1f, 3.0f}, level.npcPosition},
        {"vp_shop_front", "Front sklepu Zenona", {18.0f, 2.0f, -14.5f}, level.shopEntrancePosition},
        {"vp_shop_zenon_v2", "Zenon hero v2", {18.0f, 2.25f, -15.95f}, {18.0f, 1.25f, -21.20f}},
        {"vp_parking", "Parking i gruz", {-3.5f, 2.2f, 4.6f}, level.vehicleStart},
        {"vp_gruz_wear_v2", "Gruz wear v2", {-5.6f, 1.45f, 5.25f}, level.vehicleStart},
        {"vp_garages", "Garaże Ryska", {-10.5f, 2.0f, 18.4f}, {-18.0f, 1.0f, 23.0f}},
        {"vp_road_loop", "Pętla drogowa", {10.0f, 2.1f, -12.5f}, level.shopRoadPosition},
        {"vp_block13_front_v2", "Blok 13 front v2", {-16.35f, 1.95f, 8.65f}, {-15.08f, 1.12f, 12.42f}},
        {"vp_block_rear", "Tył bloku i zaplecze", {-21.5f, 2.4f, 23.5f}, {-16.0f, 3.0f, 19.5f}},
        {"vp_playground", "Plac zabaw i trzepak", {-8.5f, 2.2f, 7.0f}, {-6.0f, 0.8f, 14.0f}},
        {"vp_pavilions", "Pawilony przy arterii", {14.5f, 2.4f, -25.5f}, {25.0f, 1.2f, -34.0f}},
        {"vp_main_artery", "Główna arteria", {4.0f, 2.6f, -21.0f}, {5.0f, 0.7f, -30.0f}},
        {"vp_garage_belt", "Pas garaży", {-23.5f, 2.4f, 16.0f}, {-30.75f, 0.8f, 19.0f}}
    };

    const FallbackPropStyle fallbackStyles[] = {
        {"block13_core", color(132, 137, 128), true},
        {"block13_neighbor", color(111, 119, 126), true},
        {"block13_side", color(154, 132, 108), true},
        {"shop_zenona", color(96, 158, 93), true},
        {"trash_shelter_shop_side", color(92, 87, 83), true},
        {"low_concrete_barrier", color(89, 91, 88), true},
        {"future_rewir_wall_west", color(72, 80, 70), true},
        {"future_rewir_wall_east", color(72, 80, 70), true},
        {"future_rewir_wall_south", color(72, 80, 70), true},
        {"future_rewir_wall_south_east", color(72, 80, 70), true},
        {"future_rewir_wall_north", color(72, 80, 70), true},
        {"main_artery_spine", color(66, 66, 62), false},
        {"curb_ramp_parking", color(120, 120, 112), false},
        {"shop_side_ramp", color(97, 105, 96), false},
        {"spoldzielnia_notice", color(232, 230, 206), false},
        {"bogus_bench", color(231, 213, 84), false},
        {"developer_survey_marker", color(235, 92, 72), false},
    };

    level.props.clear();
    for (const FallbackPropStyle& style : fallbackStyles) {
        const WorldObject* object = findObject(level, style.objectId);
        if (object == nullptr) {
            continue;
        }
        level.props.push_back({object->position, object->scale, style.fill, style.collidable});
    }

    level.missionTriggers = {
        {"shop_walk_intro",
         MissionPhase::WalkToShop,
         MissionTriggerActor::Player,
         level.shopEntrancePosition,
         3.2f,
         MissionTriggerAction::ShopVisitedOnFoot},
        {"shop_vehicle_intro",
         MissionPhase::DriveToShop,
         MissionTriggerActor::Vehicle,
         level.shopEntrancePosition,
         4.2f,
         MissionTriggerAction::ShopReachedByVehicle},
        {"parking_dropoff_intro",
         MissionPhase::ReturnToLot,
         MissionTriggerActor::Vehicle,
         level.dropoffPosition,
         5.0f,
         MissionTriggerAction::DropoffReached}
    };

    level.locationAnchors = {
        {WorldLocationTag::Shop, level.shopPosition, 9.0f, 100},
        {WorldLocationTag::Parking, {-7.0f, 0.0f, 8.6f}, 11.5f, 80},
        {WorldLocationTag::Garage, {-18.0f, 0.0f, 23.0f}, 8.0f, 90},
        {WorldLocationTag::Trash, {9.0f, 0.0f, -4.0f}, 6.0f, 85},
        {WorldLocationTag::RoadLoop, {0.0f, 0.0f, 0.0f}, 32.0f, 60},
        {WorldLocationTag::RoadLoop, {22.0f, 0.0f, -30.0f}, 14.0f, 76},
        {WorldLocationTag::Block, {-16.0f, 0.0f, 16.0f}, 10.0f, 40},
        {WorldLocationTag::Block, {5.0f, 0.0f, 17.0f}, 10.0f, 40},
        {WorldLocationTag::Block, {-25.0f, 0.0f, -7.0f}, 10.0f, 40}
    };
}

} // namespace bs3d
