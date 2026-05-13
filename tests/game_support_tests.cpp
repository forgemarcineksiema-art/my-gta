#include "IntroLevelBuilder.h"
#include "IntroLevelAuthoring.h"
#include "ChaseAiRuntime.h"
#include "ChaseVehicleRuntime.h"
#include "DevTools.h"
#include "EditorOverlayApply.h"
#include "EditorOverlayCodec.h"
#include "EditorOverlayData.h"
#include "MissionRuntimeBridge.h"
#include "MissionOutcomeTrigger.h"
#include "PropSimulationSystem.h"
#include "RuntimeMapEditor.h"
#include "WorldDataLoader.h"
#include "WorldObjectInteraction.h"
#include "VehicleExitResolver.h"
#include "VehicleArtModel.h"
#include "VisualBaselineDebug.h"
#include "WorldAssetRegistry.h"
#include "WorldGlassRendering.h"
#include "CharacterArtModel.h"
#include "CharacterDebugGeometry.h"
#include "CharacterPosePreview.h"
#include "GameHudLayout.h"
#include "GameRenderers.h"

#include "bs3d/core/DialogueSystem.h"
#include "bs3d/core/GameFeedback.h"
#include "bs3d/core/ChaseSystem.h"
#include "bs3d/core/MissionController.h"
#include "bs3d/core/PlayerMotor.h"
#include "bs3d/core/Scene.h"
#include "bs3d/core/StoryState.h"
#include "bs3d/core/VehicleController.h"
#include "bs3d/core/WorldEventLedger.h"
#include "bs3d/core/WorldServiceState.h"

#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void expectNear(float actual, float expected, float tolerance, const std::string& message) {
    if (std::fabs(actual - expected) > tolerance) {
        throw std::runtime_error(message + " actual=" + std::to_string(actual) +
                                 " expected=" + std::to_string(expected));
    }
}

const bs3d::CharacterArtPart* findCharacterPart(const bs3d::CharacterArtModelSpec& spec, const std::string& name) {
    for (const bs3d::CharacterArtPart& part : spec.parts) {
        if (part.name == name) {
            return &part;
        }
    }
    return nullptr;
}

bool hasTag(const bs3d::WorldObject& object, const std::string& tag) {
    for (const std::string& objectTag : object.gameplayTags) {
        if (objectTag == tag) {
            return true;
        }
    }
    return false;
}

bool hasLayer(const bs3d::WorldObject& object, bs3d::CollisionLayer layer) {
    return bs3d::hasCollisionLayer(object.collisionProfile.layers, layer);
}

int colorDistanceSquared(bs3d::RenderColor a, bs3d::RenderColor b) {
    const int dr = static_cast<int>(a.r) - static_cast<int>(b.r);
    const int dg = static_cast<int>(a.g) - static_cast<int>(b.g);
    const int db = static_cast<int>(a.b) - static_cast<int>(b.b);
    return dr * dr + dg * dg + db * db;
}

int characterColorDistanceSquared(bs3d::CharacterArtColor a, bs3d::CharacterArtColor b) {
    const int dr = static_cast<int>(a.r) - static_cast<int>(b.r);
    const int dg = static_cast<int>(a.g) - static_cast<int>(b.g);
    const int db = static_cast<int>(a.b) - static_cast<int>(b.b);
    return dr * dr + dg * dg + db * db;
}

bool isPlayerSoftCollisionAsset(const bs3d::WorldObject& object) {
    return object.assetId == "street_sign" ||
           object.assetId == "lamp_post_lowpoly" ||
           object.assetId == "bollard_red" ||
           object.assetId == "parking_stop" ||
           object.assetId == "fence_panel" ||
           object.assetId == "cardboard_stack" ||
           object.assetId == "planter_concrete" ||
           (object.assetId == "concrete_barrier" &&
            object.scale.y <= 0.70f &&
            std::max(object.scale.x, object.scale.z) <= 1.75f);
}

bool isFragileSmallPropAsset(const bs3d::WorldObject& object) {
    return object.assetId == "cardboard_stack";
}

bool isDecorativeGroundPatchAsset(const bs3d::WorldObject& object) {
    return object.assetId == "irregular_asphalt_patch" ||
           object.assetId == "irregular_grass_patch" ||
           object.assetId == "irregular_dirt_patch" ||
           object.assetId == "asphalt_patch" ||
           object.assetId == "grass_wear_patch" ||
           object.assetId == "oil_stain";
}

bool isLowConcretePlayerTrapAsset(const bs3d::WorldObject& object) {
    return object.assetId == "planter_concrete" ||
           (object.assetId == "concrete_barrier" &&
            object.scale.y <= 0.70f &&
            std::max(object.scale.x, object.scale.z) <= 1.75f);
}

const bs3d::WorldObject* findObject(const bs3d::IntroLevelData& level, const std::string& id) {
    for (const bs3d::WorldObject& object : level.objects) {
        if (object.id == id) {
            return &object;
        }
    }
    return nullptr;
}

const bs3d::WorldAssetDefinition* requireAsset(const bs3d::WorldAssetRegistry& registry, const std::string& id) {
    const bs3d::WorldAssetDefinition* definition = registry.find(id);
    expect(definition != nullptr, "manifest asset exists: " + id);
    return definition;
}

const bs3d::WorldLandmark* findLandmarkByRole(const bs3d::IntroLevelData& level, const std::string& role) {
    for (const bs3d::WorldLandmark& landmark : level.landmarks) {
        if (landmark.role == role) {
            return &landmark;
        }
    }
    return nullptr;
}

const bs3d::WorldZone* findZoneById(const bs3d::IntroLevelData& level, const std::string& id) {
    for (const bs3d::WorldZone& zone : level.zones) {
        if (zone.id == id) {
            return &zone;
        }
    }
    return nullptr;
}

std::string readTextFile(const std::string& path) {
    const std::vector<std::string> candidates{
        path,
        "../" + path,
        "../../" + path,
    };
    std::ifstream file;
    for (const std::string& candidate : candidates) {
        file.open(candidate);
        if (file.is_open()) {
            break;
        }
        file.clear();
    }
    if (!file.is_open()) {
        return {};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool textContains(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

std::filesystem::path resolveExistingPath(const std::filesystem::path& path) {
    const std::vector<std::filesystem::path> candidates{
        path,
        std::filesystem::path("..") / path,
        std::filesystem::path("..") / ".." / path,
    };
    for (const std::filesystem::path& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return path;
}

int objFaceVertexCount(const std::string& line) {
    if (line.rfind("f ", 0) != 0) {
        return 0;
    }
    std::istringstream stream(line.substr(2));
    int count = 0;
    std::string token;
    while (stream >> token) {
        ++count;
    }
    return count;
}

struct ObjMeshInfo {
    std::vector<bs3d::Vec3> vertices;
    std::vector<std::vector<int>> faces;
    float minY = 0.0f;
    float maxY = 0.0f;
};

int objVertexIndex(const std::string& token) {
    const std::size_t slash = token.find('/');
    return std::stoi(slash == std::string::npos ? token : token.substr(0, slash)) - 1;
}

ObjMeshInfo readObjMeshInfo(const std::string& path) {
    const std::string text = readTextFile(path);
    expect(!text.empty(), "OBJ asset is readable: " + path);

    ObjMeshInfo mesh;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.rfind("v ", 0) == 0) {
            std::istringstream stream(line.substr(2));
            bs3d::Vec3 vertex{};
            stream >> vertex.x >> vertex.y >> vertex.z;
            if (mesh.vertices.empty()) {
                mesh.minY = vertex.y;
                mesh.maxY = vertex.y;
            } else {
                mesh.minY = std::min(mesh.minY, vertex.y);
                mesh.maxY = std::max(mesh.maxY, vertex.y);
            }
            mesh.vertices.push_back(vertex);
        } else if (line.rfind("f ", 0) == 0) {
            std::istringstream stream(line.substr(2));
            std::string token;
            std::vector<int> face;
            while (stream >> token) {
                face.push_back(objVertexIndex(token));
            }
            mesh.faces.push_back(std::move(face));
        }
    }

    expect(!mesh.vertices.empty(), "OBJ asset has vertices: " + path);
    expect(!mesh.faces.empty(), "OBJ asset has faces: " + path);
    return mesh;
}

bs3d::Vec3 faceNormal(const ObjMeshInfo& mesh, const std::vector<int>& face) {
    expect(face.size() >= 3, "OBJ face has at least three vertices");
    const bs3d::Vec3 a = mesh.vertices[face[0]];
    const bs3d::Vec3 b = mesh.vertices[face[1]];
    const bs3d::Vec3 c = mesh.vertices[face[2]];
    const bs3d::Vec3 ab = b - a;
    const bs3d::Vec3 ac = c - a;
    return {
        ab.y * ac.z - ab.z * ac.y,
        ab.z * ac.x - ab.x * ac.z,
        ab.x * ac.y - ab.y * ac.x,
    };
}

bool pointInsideBoxCollision(const bs3d::WorldObject& object, bs3d::Vec3 point, float padding = 0.0f) {
    if (object.collision.kind != bs3d::WorldCollisionShapeKind::Box &&
        object.collision.kind != bs3d::WorldCollisionShapeKind::GroundBox &&
        object.collision.kind != bs3d::WorldCollisionShapeKind::OrientedBox) {
        return false;
    }

    const bs3d::Vec3 center = object.position + object.collision.offset;
    if (object.collision.kind == bs3d::WorldCollisionShapeKind::OrientedBox) {
        const float yaw = -(object.yawRadians + object.collision.yawRadians);
        const float c = std::cos(yaw);
        const float s = std::sin(yaw);
        const bs3d::Vec3 relative = point - center;
        point = {relative.x * c - relative.z * s,
                 relative.y,
                 relative.x * s + relative.z * c};
    } else {
        point = point - center;
    }

    const bs3d::Vec3 half = object.collision.size * 0.5f;
    return std::fabs(point.x) <= half.x + padding &&
           std::fabs(point.y) <= half.y + padding &&
           std::fabs(point.z) <= half.z + padding;
}

bool pointInsideVisualFootprint(const bs3d::WorldObject& object, bs3d::Vec3 point, float padding = 0.0f) {
    return std::fabs(point.x - object.position.x) <= object.scale.x * 0.5f + padding &&
           std::fabs(point.z - object.position.z) <= object.scale.z * 0.5f + padding;
}

bool pointHasDriveSurface(const std::vector<const bs3d::WorldObject*>& driveSurfaces,
                          bs3d::Vec3 point,
                          float padding = 0.0f) {
    for (const bs3d::WorldObject* surface : driveSurfaces) {
        if (pointInsideVisualFootprint(*surface, point, padding)) {
            return true;
        }
    }
    return false;
}

bool visualFootprintsOverlapXZ(const bs3d::WorldObject& lhs,
                               const bs3d::WorldObject& rhs,
                               float separationPadding = 0.0f) {
    const float lhsMinX = lhs.position.x - lhs.scale.x * 0.5f - separationPadding;
    const float lhsMaxX = lhs.position.x + lhs.scale.x * 0.5f + separationPadding;
    const float lhsMinZ = lhs.position.z - lhs.scale.z * 0.5f - separationPadding;
    const float lhsMaxZ = lhs.position.z + lhs.scale.z * 0.5f + separationPadding;
    const float rhsMinX = rhs.position.x - rhs.scale.x * 0.5f;
    const float rhsMaxX = rhs.position.x + rhs.scale.x * 0.5f;
    const float rhsMinZ = rhs.position.z - rhs.scale.z * 0.5f;
    const float rhsMaxZ = rhs.position.z + rhs.scale.z * 0.5f;
    return lhsMinX <= rhsMaxX && lhsMaxX >= rhsMinX &&
           lhsMinZ <= rhsMaxZ && lhsMaxZ >= rhsMinZ;
}

bool segmentIntersectsVisualFootprintXZ(bs3d::Vec3 start,
                                        bs3d::Vec3 end,
                                        const bs3d::WorldObject& object,
                                        float padding = 0.0f) {
    constexpr int Samples = 20;
    for (int i = 0; i <= Samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(Samples);
        const bs3d::Vec3 point = start + (end - start) * t;
        if (pointInsideVisualFootprint(object, point, padding)) {
            return true;
        }
    }
    return false;
}

bool routeSegmentHasDriveSurfaceCoverage(const std::vector<const bs3d::WorldObject*>& driveSurfaces,
                                         bs3d::Vec3 start,
                                         bs3d::Vec3 end,
                                         float sampleSpacing,
                                         float padding,
                                         bs3d::Vec3& uncoveredPoint) {
    const float length = bs3d::distanceXZ(start, end);
    const int samples = std::max(1, static_cast<int>(std::ceil(length / std::max(sampleSpacing, 0.01f))));
    for (int i = 0; i <= samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(samples);
        const bs3d::Vec3 point = start + (end - start) * t;
        if (!pointHasDriveSurface(driveSurfaces, point, padding)) {
            uncoveredPoint = point;
            return false;
        }
    }
    return true;
}

const bs3d::WorldObject* routeSegmentCorridorBlocker(const std::vector<const bs3d::WorldObject*>& blockers,
                                                     bs3d::Vec3 start,
                                                     bs3d::Vec3 end,
                                                     float sampleSpacing,
                                                     float corridorHalfWidth,
                                                     bs3d::Vec3& blockedPoint) {
    const float length = bs3d::distanceXZ(start, end);
    const int samples = std::max(1, static_cast<int>(std::ceil(length / std::max(sampleSpacing, 0.01f))));
    for (int i = 0; i <= samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(samples);
        const bs3d::Vec3 point = start + (end - start) * t;
        for (const bs3d::WorldObject* blocker : blockers) {
            if (pointInsideBoxCollision(*blocker, point, corridorHalfWidth)) {
                blockedPoint = point;
                return blocker;
            }
        }
    }
    return nullptr;
}

void addObjectCollisionForTest(const bs3d::WorldObject& object, bs3d::WorldCollision& collision) {
    const bs3d::Vec3 center = object.position + object.collision.offset;
    switch (object.collision.kind) {
    case bs3d::WorldCollisionShapeKind::Box:
        collision.addBox(center, object.collision.size, object.collisionProfile);
        break;
    case bs3d::WorldCollisionShapeKind::OrientedBox:
        collision.addOrientedBox(center,
                                 object.collision.size,
                                 object.collision.yawRadians + object.yawRadians,
                                 object.collisionProfile);
        break;
    case bs3d::WorldCollisionShapeKind::GroundBox:
        collision.addGroundBox(center,
                               object.collision.size,
                               object.collision.yawRadians + object.yawRadians,
                               object.collisionProfile);
        break;
    case bs3d::WorldCollisionShapeKind::RampZ:
        collision.addRamp(center,
                          object.collision.size,
                          object.collision.startHeight,
                          object.collision.endHeight,
                          object.collisionProfile);
        break;
    case bs3d::WorldCollisionShapeKind::TriggerSphere:
    case bs3d::WorldCollisionShapeKind::TriggerBox:
    case bs3d::WorldCollisionShapeKind::None:
    case bs3d::WorldCollisionShapeKind::Unspecified:
        break;
    }
}

void testHelpersRecognizeOrientedBoxCollisionFootprints() {
    bs3d::WorldObject object;
    object.id = "test_oriented_blocker";
    object.position = {0.0f, 0.0f, 0.0f};
    object.yawRadians = 1.5708f;
    object.collision.kind = bs3d::WorldCollisionShapeKind::OrientedBox;
    object.collision.offset = {0.0f, 1.0f, 0.0f};
    object.collision.size = {2.0f, 2.0f, 6.0f};

    expect(pointInsideBoxCollision(object, {2.6f, 1.0f, 0.0f}, 0.0f),
           "test helper treats points inside an oriented collision footprint as inside");
    expect(!pointInsideBoxCollision(object, {0.0f, 1.0f, 2.6f}, 0.0f),
           "test helper rejects points outside an oriented collision footprint");
}

void introLevelBuilderExportsMissionRuntimeData() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    expect(!level.objects.empty(), "intro level exports authored world objects");
    expect(!level.zones.empty(), "intro level exports gameplay zones");
    expect(!level.landmarks.empty(), "intro level exports landmarks");
    expect(!level.locationAnchors.empty(), "intro level exports location anchors");
    expect(level.missionTriggers.size() == 3, "intro level exports exactly three mission triggers");
    expect(level.driveRoute.size() >= 5, "intro level exports staged drive route points");

    expect(level.missionTriggers[0].id == "shop_walk_intro", "first trigger is on-foot shop visit");
    expect(level.missionTriggers[1].id == "shop_vehicle_intro", "second trigger is vehicle shop visit");
    expect(level.missionTriggers[2].id == "parking_dropoff_intro", "third trigger is parking dropoff");

    expectNear(level.missionTriggers[0].position.x, level.shopEntrancePosition.x, 0.001f,
               "walk shop trigger uses exported shop entrance x");
    expectNear(level.missionTriggers[0].position.z, level.shopEntrancePosition.z, 0.001f,
               "walk shop trigger uses exported shop entrance z");
    expectNear(level.missionTriggers[2].position.x, level.dropoffPosition.x, 0.001f,
               "dropoff trigger uses exported parking x");
    expectNear(level.missionTriggers[2].position.z, level.dropoffPosition.z, 0.001f,
               "dropoff trigger uses exported parking z");
}

void introLevelParkingDropoffAndPaintStayOnParkingLot() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    const bs3d::WorldObject* parkingSurface = findObject(level, "parking_surface");
    expect(parkingSurface != nullptr, "parking surface exists for parking placement contracts");

    const bs3d::MissionTriggerDefinition* dropoffTrigger = nullptr;
    for (const bs3d::MissionTriggerDefinition& trigger : level.missionTriggers) {
        if (trigger.id == "parking_dropoff_intro") {
            dropoffTrigger = &trigger;
            break;
        }
    }

    const bs3d::WorldLandmark* dropoffLandmark = findLandmarkByRole(level, "dropoff");
    expect(dropoffTrigger != nullptr, "parking dropoff trigger is exported");
    expect(dropoffLandmark != nullptr, "parking dropoff landmark is exported");
    if (parkingSurface != nullptr && dropoffTrigger != nullptr && dropoffLandmark != nullptr) {
        expect(pointInsideVisualFootprint(*parkingSurface, level.dropoffPosition, 0.35f),
               "exported dropoff position sits on the authored parking lot");
        expect(pointInsideVisualFootprint(*parkingSurface, dropoffTrigger->position, 0.35f),
               "parking dropoff trigger sits on the authored parking lot");
        expect(pointInsideVisualFootprint(*parkingSurface, dropoffLandmark->position, 0.35f),
               "parking dropoff landmark sits on the authored parking lot");
    }

    int parkingPaintObjects = 0;
    if (parkingSurface != nullptr) {
        for (const bs3d::WorldObject& object : level.objects) {
            if (object.zoneTag != bs3d::WorldLocationTag::Parking || !hasTag(object, "parking_paint")) {
                continue;
            }
            ++parkingPaintObjects;
            expect(pointInsideVisualFootprint(*parkingSurface, object.position, 1.20f),
                   "parking paint/readability object stays on or at the edge of the parking lot: " + object.id);
        }
    }
    expect(parkingPaintObjects >= 10, "parking lot keeps bay paint and edge bollards covered by placement contract");
}

void introLevelExportsCompressedGrochowExpansionSkeleton() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    auto findRewir = [&level](const std::string& id) -> const bs3d::DistrictRewirPlan* {
        for (const bs3d::DistrictRewirPlan& rewir : level.districtRewirs) {
            if (rewir.id == id) {
                return &rewir;
            }
        }
        return nullptr;
    };

    const bs3d::DistrictRewirPlan* block13 = findRewir("block13");
    const bs3d::DistrictRewirPlan* mainArtery = findRewir("main_artery");
    const bs3d::DistrictRewirPlan* pavilions = findRewir("pavilions_market");
    const bs3d::DistrictRewirPlan* garageBelt = findRewir("garage_belt");

    expect(level.districtRewirs.size() >= 4,
           "compressed Grochow skeleton starts with Block 13 plus adjacent future rewirs");
    expect(block13 != nullptr && block13->playableNow && block13->homeBase,
           "Block 13 is explicitly the playable home-base rewir");
    expect(mainArtery != nullptr && !mainArtery->playableNow && mainArtery->drivingSpine,
           "main artery exists as the next driving spine, not generic sprawl");
    expect(pavilions != nullptr && !pavilions->playableNow && pavilions->serviceEconomy,
           "pavilions/market strip exists as the next local economy rewir");
    expect(garageBelt != nullptr && !garageBelt->playableNow && garageBelt->vehicleIdentity,
           "garage belt exists as the next gruz-driving rewir");

    for (const bs3d::DistrictRewirPlan& rewir : level.districtRewirs) {
        expect(!rewir.id.empty(), "district rewir has stable id");
        expect(!rewir.label.empty(), "district rewir has a readable label: " + rewir.id);
        expect(!rewir.role.empty(), "district rewir has an authored role: " + rewir.id);
        expect(!rewir.gameplayJobs.empty(), "district rewir lists gameplay jobs: " + rewir.id);
        expect(rewir.radius >= 10.0f, "district rewir has a meaningful authored footprint: " + rewir.id);
        if (!rewir.playableNow) {
            expect(!rewir.approachRouteLabel.empty(),
                   "future rewir names a readable route approach: " + rewir.id);
            expect(bs3d::distanceXZ(rewir.center, block13->center) >= 18.0f,
                   "future rewir sits outside the current Blok 13 yard: " + rewir.id);
        }
    }
}

void introLevelExportsDistrictExpansionRoutePlans() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    auto findRewir = [&level](const std::string& id) -> const bs3d::DistrictRewirPlan* {
        for (const bs3d::DistrictRewirPlan& rewir : level.districtRewirs) {
            if (rewir.id == id) {
                return &rewir;
            }
        }
        return nullptr;
    };

    auto findRoute = [&level](const std::string& fromId,
                              const std::string& toId) -> const bs3d::DistrictRoutePlan* {
        for (const bs3d::DistrictRoutePlan& route : level.districtRoutePlans) {
            if (route.fromRewirId == fromId && route.toRewirId == toId) {
                return &route;
            }
        }
        return nullptr;
    };

    const bs3d::DistrictRewirPlan* block13 = findRewir("block13");
    expect(block13 != nullptr, "district route plans need Block 13 as the source rewir");

    const bs3d::DistrictRoutePlan* mainArtery = findRoute("block13", "main_artery");
    expect(mainArtery != nullptr, "main artery has an authored expansion route from Block 13");
    expect(mainArtery->vehicleRoute && mainArtery->futureExpansion,
           "main artery route is explicitly a future gruz-driving expansion route");
    expect(mainArtery->points.size() >= 4, "main artery route has enough points to describe a readable drive");

    int futureRewirsWithBlock13Route = 0;
    for (const bs3d::DistrictRewirPlan& rewir : level.districtRewirs) {
        if (rewir.playableNow) {
            continue;
        }
        const bs3d::DistrictRoutePlan* route = findRoute("block13", rewir.id);
        expect(route != nullptr, "future rewir has a route from Block 13: " + rewir.id);
        expect(!route->id.empty(), "district route has a stable id: " + rewir.id);
        expect(route->toRewirId == rewir.id, "district route targets the matching rewir: " + rewir.id);
        expect(route->points.size() >= 3, "district route has authored waypoints: " + rewir.id);
        expect(bs3d::distanceXZ(route->points.front().position, block13->center) <= block13->radius + 8.0f,
               "district route starts inside the Block 13 travel area: " + rewir.id);
        expect(bs3d::distanceXZ(route->points.back().position, rewir.center) <= rewir.radius,
               "district route ends inside the target rewir footprint: " + rewir.id);
        for (std::size_t i = 0; i < route->points.size(); ++i) {
            expect(!route->points[i].label.empty(), "district route waypoint has a label: " + route->id);
            expect(route->points[i].radius >= 2.0f, "district route waypoint has drive radius: " + route->id);
            if (i > 0) {
                expect(bs3d::distanceXZ(route->points[i - 1].position, route->points[i].position) >= 2.5f,
                       "district route waypoints describe real movement: " + route->id);
            }
        }
        ++futureRewirsWithBlock13Route;
    }

    expect(futureRewirsWithBlock13Route >= 3,
           "compressed Grochow starts with at least three future rewirs reachable from Block 13");
}

void introLevelBuildsDistrictPlanDebugOverlay() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    const bs3d::DistrictPlanDebugOverlay overlay = bs3d::buildDistrictPlanDebugOverlay(level);

    expect(overlay.rewirMarkers.size() == level.districtRewirs.size(),
           "district debug overlay exposes every authored rewir");
    expect(overlay.routeSegments.size() >= 8,
           "district debug overlay expands planned routes into visible route segments");
    expect(overlay.futureRewirCount >= 3, "district debug overlay counts future rewirs");
    expect(overlay.vehicleRouteCount >= 3, "district debug overlay counts future gruz routes");

    bool hasPlayableHome = false;
    bool hasMainArtery = false;
    for (const bs3d::DistrictPlanDebugRewir& marker : overlay.rewirMarkers) {
        if (marker.id == "block13") {
            hasPlayableHome = marker.playableNow && marker.homeBase && marker.radius >= 18.0f;
        }
        if (marker.id == "main_artery") {
            hasMainArtery = !marker.playableNow && marker.drivingSpine && marker.radius >= 24.0f;
        }
        expect(!marker.label.empty(), "district debug rewir keeps readable label: " + marker.id);
    }

    bool hasMainArterySegment = false;
    for (const bs3d::DistrictPlanDebugRouteSegment& segment : overlay.routeSegments) {
        hasMainArterySegment = hasMainArterySegment || segment.routeId == "route_block13_main_artery";
        expect(segment.vehicleRoute, "district debug route segment preserves gruz route intent: " + segment.routeId);
        expect(segment.futureExpansion, "district debug route segment preserves future expansion intent: " + segment.routeId);
        expect(bs3d::distanceXZ(segment.from, segment.to) >= 2.5f,
               "district debug route segment spans readable map distance: " + segment.routeId);
    }

    expect(hasPlayableHome, "district debug overlay marks Block 13 as playable home base");
    expect(hasMainArtery, "district debug overlay marks main artery as future driving spine");
    expect(hasMainArterySegment, "district debug overlay includes route segments to the main artery");
}

void introLevelMaterializesMainArteryAsDriveableExpansion() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    const bs3d::DistrictRewirPlan* mainArtery = nullptr;
    for (const bs3d::DistrictRewirPlan& rewir : level.districtRewirs) {
        if (rewir.id == "main_artery") {
            mainArtery = &rewir;
            break;
        }
    }
    expect(mainArtery != nullptr, "main artery rewir exists before materialization checks");

    const bs3d::DistrictRoutePlan* mainRoute = nullptr;
    for (const bs3d::DistrictRoutePlan& route : level.districtRoutePlans) {
        if (route.id == "route_block13_main_artery") {
            mainRoute = &route;
            break;
        }
    }
    expect(mainRoute != nullptr, "main artery route exists before materialization checks");

    std::vector<const bs3d::WorldObject*> driveSurfaces;
    std::vector<const bs3d::WorldObject*> boundaryWalls;
    int mainArterySurfaceCount = 0;
    for (const bs3d::WorldObject& object : level.objects) {
        if (object.assetId == "parking_surface" || object.assetId == "road_asphalt") {
            driveSurfaces.push_back(&object);
        }
        if (hasTag(object, "boundary")) {
            boundaryWalls.push_back(&object);
        }
        if (hasTag(object, "main_artery_surface")) {
            ++mainArterySurfaceCount;
            expect(object.assetId == "road_asphalt", "main artery uses the road asphalt asset");
            expect(hasTag(object, "future_expansion"), "main artery surface is tagged as future expansion");
            expect(pointInsideVisualFootprint(object, mainArtery->center, 0.35f),
                   "main artery surface covers the authored rewir center");
        }
    }

    expect(mainArterySurfaceCount >= 1, "main artery has a visible driveable asphalt surface");
    for (const bs3d::RoutePoint& point : mainRoute->points) {
        bool covered = false;
        for (const bs3d::WorldObject* surface : driveSurfaces) {
            covered = covered || pointInsideVisualFootprint(*surface, point.position, 0.35f);
        }
        expect(covered, "main artery route point has drive surface under it: " + point.label);
    }

    for (std::size_t i = 1; i < mainRoute->points.size(); ++i) {
        for (const bs3d::WorldObject* wall : boundaryWalls) {
            expect(!segmentIntersectsVisualFootprintXZ(mainRoute->points[i - 1].position,
                                                       mainRoute->points[i].position,
                                                       *wall,
                                                       0.25f),
                   "main artery expansion route is not blocked by boundary wall: " + wall->id);
        }
    }

    bool hasMainArteryZone = false;
    for (const bs3d::WorldZone& zone : level.zones) {
        hasMainArteryZone = hasMainArteryZone ||
                            (zone.id == "zone_main_artery" &&
                             zone.tag == bs3d::WorldLocationTag::RoadLoop &&
                             bs3d::distanceXZ(zone.center, mainArtery->center) <= 1.0f &&
                             zone.radius >= 12.0f);
    }
    expect(hasMainArteryZone, "main artery materialization exports a memory/navigation zone");

    const bs3d::WorldLandmark* landmark = findLandmarkByRole(level, "main_artery");
    expect(landmark != nullptr, "main artery materialization exports a landmark");
    if (landmark != nullptr) {
        expect(bs3d::distanceXZ(landmark->position, mainArtery->center) <= 1.0f,
               "main artery landmark is anchored at the rewir center");
    }
}

void introLevelPavilionsMarketAnchorsMatchMaterializedPavilionStrip() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    const bs3d::WorldObject* pavilion = findObject(level, "pavilion_row");
    const bs3d::WorldObject* artery = findObject(level, "main_artery_spine");
    expect(pavilion != nullptr, "pavilion row exists before anchor coherence checks");
    expect(artery != nullptr, "main artery exists before pavilion route checks");

    const bs3d::DistrictRewirPlan* pavilionsRewir = nullptr;
    for (const bs3d::DistrictRewirPlan& rewir : level.districtRewirs) {
        if (rewir.id == "pavilions_market") {
            pavilionsRewir = &rewir;
            break;
        }
    }
    expect(pavilionsRewir != nullptr, "pavilions market rewir exists");
    expect(bs3d::distanceXZ(pavilionsRewir->center, pavilion->position) <= 1.0f,
           "pavilions market rewir center matches the materialized pavilion strip");

    bool hasPavilionZone = false;
    for (const bs3d::WorldZone& zone : level.zones) {
        hasPavilionZone = hasPavilionZone ||
                          (zone.id == "zone_pavilions" &&
                           zone.tag == bs3d::WorldLocationTag::Shop &&
                           bs3d::distanceXZ(zone.center, pavilion->position) <= 1.0f &&
                           zone.radius >= 10.0f);
    }
    expect(hasPavilionZone, "pavilions zone is anchored to the materialized pavilion strip");

    const bs3d::WorldLandmark* landmark = findLandmarkByRole(level, "pavilions");
    expect(landmark != nullptr, "pavilions materialization exports a landmark");
    if (landmark != nullptr) {
        expect(bs3d::distanceXZ(landmark->position, pavilion->position) <= 3.0f,
               "pavilions landmark stays on the pavilion frontage");
    }

    const bs3d::DistrictRoutePlan* route = nullptr;
    for (const bs3d::DistrictRoutePlan& candidate : level.districtRoutePlans) {
        if (candidate.id == "route_block13_pavilions_market") {
            route = &candidate;
            break;
        }
    }
    expect(route != nullptr && !route->points.empty(), "pavilions market has an authored approach route");
    if (route != nullptr && !route->points.empty()) {
        const bs3d::RoutePoint& endpoint = route->points.back();
        expect(bs3d::distanceXZ(endpoint.position, pavilion->position) <= 8.0f,
               "pavilions route ends at the pavilion frontage, not a different yard");
        expect(!pointInsideBoxCollision(*pavilion, endpoint.position, 0.05f),
               "pavilions route endpoint stays outside the pavilion building collision");
        expect(pointInsideVisualFootprint(*artery, endpoint.position, 0.35f),
               "pavilions route endpoint lands on the artery surface in front of the strip");
    }

    const float pavilionNorthEdge = pavilion->position.z + pavilion->scale.z * 0.5f;
    const float arterySouthEdge = artery->position.z - artery->scale.z * 0.5f;
    expect(pavilionNorthEdge <= arterySouthEdge + 0.05f,
           "pavilion building footprint sits beside the artery instead of on the drive surface");
}

void introLevelDistrictAnchorsStayOnMaterializedObjects() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    struct AnchorContract {
        const char* objectId = "";
        const char* landmarkRole = "";
        const char* zoneId = "";
        bs3d::WorldLocationTag zoneTag = bs3d::WorldLocationTag::Unknown;
        float landmarkPadding = 0.0f;
        float zonePadding = 0.0f;
    };

    const AnchorContract contracts[]{
        {"block13_core", "block13", "zone_block13", bs3d::WorldLocationTag::Block, 0.35f, 0.35f},
        {"parking_surface", "parking_lot", "zone_parking", bs3d::WorldLocationTag::Parking, 0.35f, 0.35f},
        {"garage_row_rysiek", "future_base", "zone_garages", bs3d::WorldLocationTag::Garage, 0.35f, 0.35f},
        {"trash_shelter_shop_side", "trash", "zone_trash", bs3d::WorldLocationTag::Trash, 0.35f, 0.35f},
        {"shop_zenona", "shop", "zone_shop", bs3d::WorldLocationTag::Shop, 1.60f, 0.35f},
        {"main_artery_spine", "main_artery", "zone_main_artery", bs3d::WorldLocationTag::RoadLoop, 0.35f, 0.35f},
        {"pavilion_row", "pavilions", "zone_pavilions", bs3d::WorldLocationTag::Shop, 0.35f, 0.35f},
        {"garage_lane", "garage_belt", "zone_garage_belt", bs3d::WorldLocationTag::Garage, 0.35f, 0.35f},
        {"bus_stop", "bus_stop", "zone_bus_stop", bs3d::WorldLocationTag::RoadLoop, 0.35f, 0.35f}};

    for (const AnchorContract& contract : contracts) {
        const bs3d::WorldObject* object = findObject(level, contract.objectId);
        expect(object != nullptr, "anchor contract object exists: " + std::string{contract.objectId});

        const bs3d::WorldLandmark* landmark = findLandmarkByRole(level, contract.landmarkRole);
        expect(landmark != nullptr, "materialized object exports landmark role: " + std::string{contract.landmarkRole});
        if (object != nullptr && landmark != nullptr) {
            expect(pointInsideVisualFootprint(*object, landmark->position, contract.landmarkPadding),
                   "landmark stays on materialized object footprint: " + std::string{contract.landmarkRole});
        }

        const bs3d::WorldZone* zone = findZoneById(level, contract.zoneId);
        expect(zone != nullptr, "materialized object exports navigation/memory zone: " + std::string{contract.zoneId});
        if (object != nullptr && zone != nullptr) {
            expect(zone->tag == contract.zoneTag,
                   "materialized object zone keeps expected location tag: " + std::string{contract.zoneId});
            expect(pointInsideVisualFootprint(*object, zone->center, contract.zonePadding),
                   "zone center stays on materialized object footprint: " + std::string{contract.zoneId});
            expect(zone->radius >= 3.0f, "materialized object zone has readable radius: " + std::string{contract.zoneId});
        }
    }
}

void introLevelFlatDriveSurfacesUseSeparatedRenderHeights() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    std::vector<const bs3d::WorldObject*> driveSurfaces;
    std::vector<const bs3d::WorldObject*> groundPatches;
    for (const bs3d::WorldObject& object : level.objects) {
        if (object.assetId == "parking_surface" || object.assetId == "road_asphalt") {
            driveSurfaces.push_back(&object);
        }
        if (hasTag(object, "ground_patch")) {
            groundPatches.push_back(&object);
        }
    }

    for (std::size_t i = 0; i < driveSurfaces.size(); ++i) {
        for (std::size_t j = i + 1; j < driveSurfaces.size(); ++j) {
            const bs3d::WorldObject& lhs = *driveSurfaces[i];
            const bs3d::WorldObject& rhs = *driveSurfaces[j];
            if (!visualFootprintsOverlapXZ(lhs, rhs)) {
                continue;
            }
            expect(std::fabs(lhs.position.y - rhs.position.y) >= 0.004f,
                   "overlapping drive surfaces use separated render heights: " + lhs.id + " / " + rhs.id);
        }
    }

    for (const bs3d::WorldObject* patch : groundPatches) {
        for (const bs3d::WorldObject* surface : driveSurfaces) {
            if (!visualFootprintsOverlapXZ(*patch, *surface)) {
                continue;
            }
            expect(patch->position.y > surface->position.y + 0.020f,
                   "ground patch/decal renders above overlapped drive surface: " + patch->id + " over " + surface->id);
        }
    }
}

void introLevelMainArteryExpansionHasReadableRouteGuidance() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    const bs3d::DistrictRoutePlan* mainRoute = nullptr;
    for (const bs3d::DistrictRoutePlan& route : level.districtRoutePlans) {
        if (route.id == "route_block13_main_artery") {
            mainRoute = &route;
            break;
        }
    }
    expect(mainRoute != nullptr, "main artery route exists before guidance checks");

    std::vector<const bs3d::WorldObject*> guidance;
    for (const bs3d::WorldObject& object : level.objects) {
        if (!hasTag(object, "main_artery_guidance")) {
            continue;
        }
        guidance.push_back(&object);
        expect(object.assetId == "route_arrow_head", "main artery guidance uses arrow-head asset: " + object.id);
        expect(object.zoneTag == bs3d::WorldLocationTag::RoadLoop,
               "main artery guidance belongs to road-loop/main-artery memory: " + object.id);
        expect(object.collision.kind == bs3d::WorldCollisionShapeKind::None,
               "main artery guidance remains visual-only and drive-safe: " + object.id);
        expect(hasTag(object, "future_expansion"), "main artery guidance is tagged as expansion dressing: " + object.id);
    }

    expect(guidance.size() >= 3, "main artery branch has readable world guidance arrows");
    int coveredFuturePoints = 0;
    for (std::size_t i = 2; i < mainRoute->points.size(); ++i) {
        bool covered = false;
        for (const bs3d::WorldObject* marker : guidance) {
            covered = covered || bs3d::distanceXZ(marker->position, mainRoute->points[i].position) <= 3.2f;
        }
        if (covered) {
            ++coveredFuturePoints;
        }
    }
    expect(coveredFuturePoints >= 3,
           "main artery guidance covers the bend, exit, and destination beats of the expansion route");
}

void introLevelMainArteryRouteIsVehicleTraversableByGruz() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::Scene scene;
    bs3d::WorldCollision collision;
    bs3d::IntroLevelBuilder::populateWorld(level, scene, collision);

    bs3d::PropSimulationSystem props;
    props.addPropsFromWorld(level.objects);
    props.publishCollision(collision);

    const bs3d::DistrictRouteTraversalReport report =
        bs3d::inspectDistrictRouteVehicleTraversal(level, "route_block13_main_artery", collision);

    expect(report.foundRoute, "main artery traversal QA finds the authored route");
    expect(report.checkedSegments >= 4, "main artery traversal QA checks every route segment");
    expect(report.clear,
           "main artery traversal reaches every gruz waypoint; blocked segment=" +
               report.blockedSegmentLabel + " resolved=" +
               std::to_string(report.resolvedPosition.x) + "," +
               std::to_string(report.resolvedPosition.z));
}

std::string vehicleRouteBlockerCandidates(const bs3d::IntroLevelData& level,
                                          const std::string& routeId,
                                          const std::string& blockedSegmentLabel) {
    std::string candidates;
    for (const bs3d::WorldObject& object : level.objects) {
        const bool vehicleBlocking =
            hasLayer(object, bs3d::CollisionLayer::WorldStatic) ||
            hasLayer(object, bs3d::CollisionLayer::VehicleBlocker) ||
            hasLayer(object, bs3d::CollisionLayer::DynamicProp);
        if (!vehicleBlocking || object.collision.kind == bs3d::WorldCollisionShapeKind::None ||
            object.collision.kind == bs3d::WorldCollisionShapeKind::Unspecified) {
            continue;
        }

        bs3d::WorldCollision singleObjectCollision;
        singleObjectCollision.addGroundPlane(0.0f);
        addObjectCollisionForTest(object, singleObjectCollision);
        const bs3d::DistrictRouteTraversalReport singleObjectReport =
            bs3d::inspectDistrictRouteVehicleTraversal(level, routeId, singleObjectCollision);
        if (!singleObjectReport.clear &&
            singleObjectReport.blockedSegmentLabel == blockedSegmentLabel) {
            if (!candidates.empty()) {
                candidates += ",";
            }
            candidates += object.id;
        }
    }

    return candidates.empty() ? " candidate_objects=<none>" : " candidate_objects=" + candidates;
}

void introLevelFutureDistrictRoutesAreVehicleTraversableByGruz() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::Scene scene;
    bs3d::WorldCollision collision;
    bs3d::IntroLevelBuilder::populateWorld(level, scene, collision);

    bs3d::PropSimulationSystem props;
    props.addPropsFromWorld(level.objects);
    props.publishCollision(collision);

    std::vector<const bs3d::WorldObject*> driveSurfaces;
    std::vector<const bs3d::WorldObject*> solidBlockers;
    for (const bs3d::WorldObject& object : level.objects) {
        if (object.assetId == "parking_surface" || object.assetId == "road_asphalt") {
            driveSurfaces.push_back(&object);
        }
        if ((object.collision.kind == bs3d::WorldCollisionShapeKind::Box ||
             object.collision.kind == bs3d::WorldCollisionShapeKind::OrientedBox) &&
            !hasTag(object, "boundary")) {
            solidBlockers.push_back(&object);
        }
    }

    const std::vector<std::string> routeIds{
        "route_block13_main_artery",
        "route_block13_pavilions_market",
        "route_block13_garage_belt"};

    for (const std::string& routeId : routeIds) {
        const bs3d::DistrictRoutePlan* route = nullptr;
        for (const bs3d::DistrictRoutePlan& candidate : level.districtRoutePlans) {
            if (candidate.id == routeId) {
                route = &candidate;
                break;
            }
        }
        expect(route != nullptr, "future district route exists for traversal QA: " + routeId);
        expect(route != nullptr && route->vehicleRoute, "future district route is authored for gruz traversal: " + routeId);

        for (std::size_t i = 1; route != nullptr && i < route->points.size(); ++i) {
            const bs3d::RoutePoint& previous = route->points[i - 1];
            const bs3d::RoutePoint& point = route->points[i];
            const bool covered = pointHasDriveSurface(driveSurfaces, point.position, 0.25f);
            expect(covered, "future district route waypoint has drive surface under it: " + routeId + " / " + point.label);

            bs3d::Vec3 uncoveredPoint{};
            const bool segmentCovered = routeSegmentHasDriveSurfaceCoverage(driveSurfaces,
                                                                            previous.position,
                                                                            point.position,
                                                                            0.75f,
                                                                            0.25f,
                                                                            uncoveredPoint);
            expect(segmentCovered,
                   "future district route segment stays on drive surface: " + routeId + " / " +
                       previous.label + " -> " + point.label +
                       " uncovered=" + std::to_string(uncoveredPoint.x) +
                       "," + std::to_string(uncoveredPoint.z));

            for (const bs3d::WorldObject* blocker : solidBlockers) {
                expect(!pointInsideBoxCollision(*blocker, point.position, 0.05f),
                       "future district route waypoint stays outside solid blocking collision: " +
                           routeId + " / " + point.label + " vs " + blocker->id);
            }
        }

        const bs3d::DistrictRouteTraversalReport report =
            bs3d::inspectDistrictRouteVehicleTraversal(level, routeId, collision);
        const std::string blockerCandidates = report.clear
                                                  ? std::string{}
                                                  : vehicleRouteBlockerCandidates(level, routeId, report.blockedSegmentLabel);
        expect(report.foundRoute, "future district traversal QA finds authored route: " + routeId);
        expect(report.checkedSegments >= 2, "future district traversal QA checks route segments: " + routeId);
        expect(report.clear,
               "future district traversal reaches every gruz waypoint: " + routeId +
                   " blocked segment=" + report.blockedSegmentLabel +
                   " resolved=" + std::to_string(report.resolvedPosition.x) +
                   "," + std::to_string(report.resolvedPosition.z) +
                   blockerCandidates);
    }
}

void introLevelVehicleRoutesKeepClearanceFromAuthoredObjectFootprints() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    std::vector<const bs3d::WorldObject*> blockers;
    for (const bs3d::WorldObject& object : level.objects) {
        if (object.assetId == "parking_surface" || object.assetId == "road_asphalt") {
            continue;
        }
        const bool vehicleBlocking =
            hasLayer(object, bs3d::CollisionLayer::WorldStatic) ||
            hasLayer(object, bs3d::CollisionLayer::VehicleBlocker) ||
            hasLayer(object, bs3d::CollisionLayer::DynamicProp);
        const bool boxLike = object.collision.kind == bs3d::WorldCollisionShapeKind::Box ||
                             object.collision.kind == bs3d::WorldCollisionShapeKind::OrientedBox;
        if (vehicleBlocking && boxLike) {
            blockers.push_back(&object);
        }
    }

    const std::vector<std::string> routeIds{
        "route_block13_main_artery",
        "route_block13_pavilions_market",
        "route_block13_garage_belt"};

    for (const std::string& routeId : routeIds) {
        const bs3d::DistrictRoutePlan* route = nullptr;
        for (const bs3d::DistrictRoutePlan& candidate : level.districtRoutePlans) {
            if (candidate.id == routeId) {
                route = &candidate;
                break;
            }
        }
        expect(route != nullptr, "vehicle clearance route exists: " + routeId);

        for (std::size_t i = 1; route != nullptr && i < route->points.size(); ++i) {
            bs3d::Vec3 blockedPoint{};
            const bs3d::WorldObject* blocker = routeSegmentCorridorBlocker(blockers,
                                                                           route->points[i - 1].position,
                                                                           route->points[i].position,
                                                                           0.50f,
                                                                           1.05f,
                                                                           blockedPoint);
            expect(blocker == nullptr,
                   "vehicle route corridor keeps object-footprint clearance: " + routeId + " / " +
                       route->points[i - 1].label + " -> " + route->points[i].label +
                       " blocker=" + (blocker != nullptr ? blocker->id : std::string{"<none>"}) +
                       " at=" + std::to_string(blockedPoint.x) +
                       "," + std::to_string(blockedPoint.z));
        }
    }
}

void editorOverlayDataDefaultsAreSafe() {
    const bs3d::EditorOverlayDocument overlay;
    expect(overlay.schemaVersion == 1, "editor overlay schema starts at version 1");
    expect(overlay.overrides.empty(), "editor overlay starts with no overrides");
    expect(overlay.instances.empty(), "editor overlay starts with no instances");

    bs3d::EditorOverlayObject object;
    object.id = "editor_test_lamp";
    object.assetId = "lamp_post_lowpoly";
    object.position = {1.0f, 0.0f, 2.0f};
    object.scale = {0.18f, 3.1f, 0.18f};
    object.zoneTag = bs3d::WorldLocationTag::RoadLoop;
    object.gameplayTags = {"vertical_readability", "future_expansion"};

    expect(object.id == "editor_test_lamp", "editor overlay object keeps stable id");
    expect(object.assetId == "lamp_post_lowpoly", "editor overlay object keeps manifest asset id");
    expect(object.zoneTag == bs3d::WorldLocationTag::RoadLoop, "editor overlay object keeps zone tag");
    expect(object.gameplayTags.size() == 2, "editor overlay object keeps gameplay tags");
}

void editorOverlayCodecParsesValidOverlay() {
    const std::string text =
        "{"
        "\"schemaVersion\":1,"
        "\"overrides\":[{\"id\":\"sign_no_parking\",\"assetId\":\"street_sign\","
        "\"position\":[-3.65,0.0,5.7],\"yawRadians\":0.25,"
        "\"scale\":[0.58,1.75,0.08],\"zoneTag\":\"Parking\","
        "\"gameplayTags\":[\"vertical_readability\",\"drive_readability\"]}],"
        "\"instances\":[{\"id\":\"editor_main_artery_lamp_0\",\"assetId\":\"lamp_post_lowpoly\","
        "\"position\":[18.0,0.0,-27.0],\"yawRadians\":0.0,"
        "\"scale\":[0.18,3.1,0.18],\"zoneTag\":\"RoadLoop\","
        "\"gameplayTags\":[\"vertical_readability\",\"future_expansion\"]}]"
        "}";

    const bs3d::EditorOverlayLoadResult loaded = bs3d::parseEditorOverlay(text);

    expect(loaded.success, "valid editor overlay parses");
    expect(loaded.document.overrides.size() == 1, "valid overlay has one override");
    expect(loaded.document.instances.size() == 1, "valid overlay has one instance");
    expect(loaded.document.overrides[0].id == "sign_no_parking", "override id parsed");
    expect(loaded.document.overrides[0].zoneTag == bs3d::WorldLocationTag::Parking, "zone tag parsed");
    expectNear(loaded.document.instances[0].position.z, -27.0f, 0.001f, "instance position parsed");
}

void editorOverlayCodecRejectsBadSchema() {
    const bs3d::EditorOverlayLoadResult loaded =
        bs3d::parseEditorOverlay("{\"schemaVersion\":99,\"overrides\":[],\"instances\":[]}");

    expect(!loaded.success, "unsupported editor overlay schema fails");
    expect(!loaded.warnings.empty(), "unsupported schema reports warning");
}

void editorOverlayCodecRejectsTrailingGarbage() {
    const bs3d::EditorOverlayLoadResult loaded =
        bs3d::parseEditorOverlay("{\"schemaVersion\":1,\"overrides\":[],\"instances\":[]} trailing");

    expect(!loaded.success, "editor overlay parser rejects trailing garbage after root object");
    expect(!loaded.warnings.empty(), "trailing garbage reports parse warning");
}

void editorOverlayCodecRoundTripsMinimalDocument() {
    bs3d::EditorOverlayDocument document;
    bs3d::EditorOverlayObject instance;
    instance.id = "editor_test_bin";
    instance.assetId = "trash_bin_lowpoly";
    instance.position = {8.0f, 0.0f, -4.0f};
    instance.scale = {0.78f, 1.05f, 0.68f};
    instance.zoneTag = bs3d::WorldLocationTag::Trash;
    instance.gameplayTags = {"trash_dressing"};
    document.instances.push_back(instance);

    const std::string saved = bs3d::serializeEditorOverlay(document);
    const bs3d::EditorOverlayLoadResult loaded = bs3d::parseEditorOverlay(saved);

    expect(loaded.success, "serialized editor overlay parses back");
    expect(loaded.document.instances.size() == 1, "round-trip keeps instance");
    expect(loaded.document.instances[0].assetId == "trash_bin_lowpoly", "round-trip keeps asset id");
    expect(loaded.document.instances[0].zoneTag == bs3d::WorldLocationTag::Trash, "round-trip keeps zone");
}

void editorOverlayApplyOverridesExistingObject() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::EditorOverlayDocument overlay;
    bs3d::EditorOverlayObject overrideObject;
    overrideObject.id = "sign_no_parking";
    overrideObject.assetId = "street_sign";
    overrideObject.position = {-4.25f, 0.0f, 5.9f};
    overrideObject.scale = {0.58f, 1.75f, 0.08f};
    overrideObject.zoneTag = bs3d::WorldLocationTag::Parking;
    overrideObject.gameplayTags = {"vertical_readability", "drive_readability", "editor_adjusted"};
    overlay.overrides.push_back(overrideObject);

    const bs3d::EditorOverlayApplyResult result = bs3d::applyEditorOverlay(level, overlay);

    const bs3d::WorldObject* object = findObject(level, "sign_no_parking");
    expect(result.appliedOverrides == 1, "editor overlay applies one override");
    expect(object != nullptr, "overridden base object still exists");
    expectNear(object->position.x, -4.25f, 0.001f, "override changes object position");
    expect(hasTag(*object, "editor_adjusted"), "override replaces gameplay tags");
}

void editorOverlayApplyAppendsNewManifestInstance() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    const std::size_t beforeCount = level.objects.size();
    bs3d::EditorOverlayDocument overlay;
    bs3d::EditorOverlayObject instance;
    instance.id = "editor_main_artery_lamp_0";
    instance.assetId = "lamp_post_lowpoly";
    instance.position = {18.0f, 0.0f, -27.0f};
    instance.scale = {0.18f, 3.1f, 0.18f};
    instance.zoneTag = bs3d::WorldLocationTag::RoadLoop;
    instance.gameplayTags = {"vertical_readability", "future_expansion"};
    overlay.instances.push_back(instance);

    const bs3d::EditorOverlayApplyResult result = bs3d::applyEditorOverlay(level, overlay);

    const bs3d::WorldObject* object = findObject(level, "editor_main_artery_lamp_0");
    expect(result.appliedInstances == 1, "editor overlay applies one instance");
    expect(level.objects.size() == beforeCount + 1, "editor overlay appends object");
    expect(object != nullptr && object->assetId == "lamp_post_lowpoly", "editor instance uses manifest asset id");
    expect(object != nullptr && object->collision.kind == bs3d::WorldCollisionShapeKind::None,
           "editor instance defaults to no authored collision before normalization");
}

void editorOverlayApplyRejectsDuplicateInstanceId() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::EditorOverlayDocument overlay;
    bs3d::EditorOverlayObject instance;
    instance.id = "sign_no_parking";
    instance.assetId = "lamp_post_lowpoly";
    instance.scale = {0.18f, 3.1f, 0.18f};
    overlay.instances.push_back(instance);

    const bs3d::EditorOverlayApplyResult result = bs3d::applyEditorOverlay(level, overlay);

    expect(result.appliedInstances == 0, "duplicate editor instance is not applied");
    expect(!result.warnings.empty(), "duplicate editor instance reports warning");
}

void introLevelBuilderExportsDistinctRewirMemoryZones() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    bool hasGarageZone = false;
    bool hasTrashZone = false;
    bool hasGarageAnchor = false;
    bool hasTrashAnchor = false;
    int garageObjectCount = 0;
    int trashObjectCount = 0;

    for (const bs3d::WorldZone& zone : level.zones) {
        hasGarageZone = hasGarageZone || (zone.id == "zone_garages" && zone.tag == bs3d::WorldLocationTag::Garage);
        hasTrashZone = hasTrashZone || (zone.id == "zone_trash" && zone.tag == bs3d::WorldLocationTag::Trash);
    }

    for (const bs3d::LocationAnchor& anchor : level.locationAnchors) {
        hasGarageAnchor = hasGarageAnchor || anchor.tag == bs3d::WorldLocationTag::Garage;
        hasTrashAnchor = hasTrashAnchor || anchor.tag == bs3d::WorldLocationTag::Trash;
    }

    for (const bs3d::WorldObject& object : level.objects) {
        if (hasTag(object, "garage_cluster")) {
            expect(object.zoneTag == bs3d::WorldLocationTag::Garage,
                   "garage-cluster objects use Garage memory tag");
            ++garageObjectCount;
        }
        if (hasTag(object, "trash_cluster")) {
            expect(object.zoneTag == bs3d::WorldLocationTag::Trash,
                   "trash-cluster objects use Trash memory tag");
            ++trashObjectCount;
        }
    }

    expect(hasGarageZone, "intro level exports a distinct garage rewir zone");
    expect(hasTrashZone, "intro level exports a distinct trash rewir zone");
    expect(hasGarageAnchor, "intro level exposes garage as a location anchor");
    expect(hasTrashAnchor, "intro level exposes trash as a location anchor");
    expect(garageObjectCount >= 4, "garage zone has authored tagged dressing");
    expect(trashObjectCount >= 4, "trash zone has authored tagged dressing");
}

void introLevelBuilderExportsParagonMissionInteractionPoints() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::WorldInteraction interactions;
    bs3d::IntroLevelBuilder::populateInteractions(level, interactions);

    bool hasZenon = false;
    bool hasLolek = false;
    bool hasReceiptHolder = false;
    for (const bs3d::InteractionPoint& point : interactions.points()) {
        hasZenon = hasZenon || point.id == "npc_zenon";
        hasLolek = hasLolek || point.id == "npc_lolek";
        hasReceiptHolder = hasReceiptHolder || point.id == "receipt_holder";
    }

    expect(hasZenon, "v0.10 exports Zenon as a real shop interaction point");
    expect(hasLolek, "v0.10 exports Lolek as the paragon clue interaction point");
    expect(hasReceiptHolder, "v0.10 exports the receipt holder for the solution beat");
    expect(bs3d::distanceXZ(level.zenonPosition, level.shopEntrancePosition) < 2.4f,
           "Zenon stands at the shop instead of being a detached marker");
    expect(bs3d::distanceXZ(level.lolekPosition, level.receiptHolderPosition) > 2.5f,
           "Lolek clue and receipt holder are separate readable beats");
}

void introLevelBuilderExportsGarageServiceInteractionPoint() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::WorldInteraction interactions;
    bs3d::IntroLevelBuilder::populateInteractions(level, interactions);

    const bs3d::InteractionPoint* garagePoint = nullptr;
    for (const bs3d::InteractionPoint& point : interactions.points()) {
        if (point.id == "garage_rysiek") {
            garagePoint = &point;
            break;
        }
    }

    expect(garagePoint != nullptr, "intro level exports Rysiek garage service interaction point");
    if (garagePoint != nullptr) {
        expect(garagePoint->prompt == "E: pogadaj z Ryskiem",
               "garage interaction starts with the calm Rysiek prompt");
        expect(bs3d::distanceXZ(garagePoint->position, {-18.0f, 0.0f, 23.0f}) < 0.8f,
               "Rysiek garage interaction is anchored at the garage rewir");
        expect(garagePoint->radius >= 2.4f, "Rysiek garage interaction is reachable on foot");
    }
}

void introLevelBuilderExportsCaretakerServiceInteractionPoint() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::WorldInteraction interactions;
    bs3d::IntroLevelBuilder::populateInteractions(level, interactions);

    const bs3d::InteractionPoint* caretakerPoint = nullptr;
    for (const bs3d::InteractionPoint& point : interactions.points()) {
        if (point.id == "trash_dozorca") {
            caretakerPoint = &point;
            break;
        }
    }

    expect(caretakerPoint != nullptr, "intro level exports caretaker trash service interaction point");
    if (caretakerPoint != nullptr) {
        expect(caretakerPoint->prompt == "E: pogadaj z Dozorca",
               "caretaker interaction starts with the calm prompt");
        expect(bs3d::distanceXZ(caretakerPoint->position, {9.0f, 0.0f, -4.0f}) < 0.8f,
               "caretaker trash interaction is anchored at the trash rewir");
        expect(caretakerPoint->radius >= 2.4f, "caretaker trash interaction is reachable on foot");
    }
}

void introLevelBuilderPopulatesLocalServiceInteractionsFromCatalog() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::WorldInteraction interactions;
    bs3d::IntroLevelBuilder::populateInteractions(level, interactions);

    for (const bs3d::LocalRewirServiceSpec& service : bs3d::localRewirServiceCatalog()) {
        const bs3d::InteractionPoint* point = nullptr;
        for (const bs3d::InteractionPoint& candidate : interactions.points()) {
            if (candidate.id == service.interactionPointId) {
                point = &candidate;
                break;
            }
        }

        expect(point != nullptr, "intro level populates local service point from catalog: " + service.interactionPointId);
        if (point != nullptr) {
            expect(point->prompt == service.normalPrompt,
                   "catalog local service prompt is used for point: " + service.interactionPointId);
            expect(bs3d::distanceXZ(point->position, service.interactionPosition) < 0.01f,
                   "catalog local service position is used for point: " + service.interactionPointId);
            expect(std::fabs(point->radius - service.interactionRadius) < 0.01f,
                   "catalog local service radius is used for point: " + service.interactionPointId);
        }
    }
}

void introLevelBuilderPopulatesLocalRewirFavorInteractionsFromCatalog() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::WorldInteraction interactions;
    bs3d::IntroLevelBuilder::populateInteractions(level, interactions);

    for (const bs3d::LocalRewirFavorSpec& favor : bs3d::localRewirFavorCatalog()) {
        const bs3d::InteractionPoint* point = nullptr;
        for (const bs3d::InteractionPoint& candidate : interactions.points()) {
            if (candidate.id == favor.interactionPointId) {
                point = &candidate;
                break;
            }
        }

        expect(point != nullptr,
               "intro level populates local rewir favor point from catalog: " + favor.interactionPointId);
        if (point != nullptr) {
            expect(point->prompt == favor.prompt,
                   "catalog local rewir favor prompt is used for point: " + favor.interactionPointId);
            expect(bs3d::distanceXZ(point->position, favor.interactionPosition) < 0.01f,
                   "catalog local rewir favor position is used for point: " + favor.interactionPointId);
            expect(std::fabs(point->radius - favor.interactionRadius) < 0.01f,
                   "catalog local rewir favor radius is used for point: " + favor.interactionPointId);
        }
    }
}

void introLevelBuilderExportsParkingServiceInteractionPoint() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::WorldInteraction interactions;
    bs3d::IntroLevelBuilder::populateInteractions(level, interactions);

    const bs3d::InteractionPoint* parkingPoint = nullptr;
    for (const bs3d::InteractionPoint& point : interactions.points()) {
        if (point.id == "parking_parkingowy") {
            parkingPoint = &point;
            break;
        }
    }

    expect(parkingPoint != nullptr, "intro level exports Parkingowy parking service interaction point");
    if (parkingPoint != nullptr) {
        expect(parkingPoint->prompt == "E: pogadaj z Parkingowym",
               "parking interaction starts with the calm Parkingowy prompt");
        expect(bs3d::distanceXZ(parkingPoint->position, {-7.0f, 0.0f, 8.6f}) < 0.8f,
               "Parkingowy interaction is anchored at the parking rewir");
        expect(parkingPoint->radius >= 2.4f, "Parkingowy interaction is reachable on foot");
    }
}

void introLevelBuilderExportsRoadLoopServiceInteractionPoint() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::WorldInteraction interactions;
    bs3d::IntroLevelBuilder::populateInteractions(level, interactions);

    const bs3d::InteractionPoint* roadPoint = nullptr;
    for (const bs3d::InteractionPoint& point : interactions.points()) {
        if (point.id == "road_kierowca") {
            roadPoint = &point;
            break;
        }
    }

    expect(roadPoint != nullptr, "intro level exports Kierowca road-loop service interaction point");
    if (roadPoint != nullptr) {
        expect(roadPoint->prompt == "E: pogadaj z Kierowca",
               "road-loop interaction starts with the calm Kierowca prompt");
        expect(bs3d::distanceXZ(roadPoint->position, {8.0f, 0.0f, -18.0f}) < 0.8f,
               "Kierowca interaction is anchored at the road-loop rewir");
        expect(roadPoint->radius >= 2.4f, "Kierowca interaction is reachable on foot");
    }
}

void assetRegistryLoadsManifestAndReportsMissingAssets() {
    bs3d::WorldAssetRegistry registry;

    const bs3d::AssetManifestLoadResult result = registry.loadManifest("data/assets/asset_manifest.txt");

    expect(result.loaded, "asset manifest loads from data/assets");
    expect(result.entries >= 8, "asset manifest contains starter art-kit entries");
    expect(registry.contains("block13_core"), "manifest contains Block 13 asset");
    expect(registry.contains("shop_zenona"), "manifest contains shop asset");
    expect(registry.contains("garage_row"), "manifest contains garage asset");
    expect(registry.contains("trash_shelter"), "manifest contains trash shelter asset");
    expect(registry.contains("shop_sign_zenona"), "manifest contains readable shop sign asset");
    expect(registry.contains("block_window_strip"), "manifest contains block window identity asset");
    expect(registry.contains("balcony_stack"), "manifest contains balcony identity asset");
    expect(registry.contains("garage_door_segment"), "manifest contains garage door identity asset");
    expect(registry.contains("parking_line"), "manifest contains parking line asset");
    expect(registry.contains("trash_bin_green"), "manifest contains trash bin asset");
    expect(registry.contains("curb_segment"), "manifest contains curb ground-truth asset");
    expect(registry.contains("fence_panel"), "manifest contains estate frame fence asset");
    expect(registry.contains("lamp_post"), "manifest contains vertical lamp post asset");
    expect(registry.contains("route_arrow_head"), "manifest contains readable route arrow head asset");
    expect(registry.contains("wall_stain"), "manifest contains facade grime asset");
    expect(registry.contains("asphalt_patch"), "manifest contains asphalt breakup patch asset");
    expect(registry.contains("grass_wear_patch"), "manifest contains worn grass patch asset");
    expect(registry.contains("oil_stain"), "manifest contains garage oil stain asset");
    expect(registry.contains("curb_lowpoly"), "manifest contains v0.9.5 low-poly curb asset");
    expect(registry.contains("lamp_post_lowpoly"), "manifest contains v0.9.5 low-poly lamp asset");
    expect(registry.contains("trash_bin_lowpoly"), "manifest contains v0.9.5 low-poly trash bin asset");
    expect(registry.contains("garage_row_v2"), "manifest contains v0.9.5 garage hero asset");
    expect(registry.contains("shop_zenona_v2"), "manifest contains v0.9.5 shop hero asset");
    expect(registry.contains("shop_zenona_v3"), "manifest contains v0.9.10 shop hygiene hero asset");
    expect(registry.contains("irregular_asphalt_patch"), "manifest contains v0.9.5 irregular asphalt patch");
    expect(registry.contains("irregular_grass_patch"), "manifest contains v0.9.5 irregular grass patch");
    expect(registry.contains("irregular_dirt_patch"), "manifest contains v0.9.5 irregular dirt patch");
    expect(registry.contains("vehicle_gruz_e36"), "manifest contains runtime-ready gruz coupe-sedan asset");

    const bs3d::WorldAssetDefinition* shop = registry.find("shop_zenona");
    expect(shop != nullptr, "shop definition is findable");
    expect(shop->modelPath.find("models/") == 0, "shop model path is relative to asset root");
    expect(shop->fallbackSize.x > 0.0f && shop->fallbackSize.z > 0.0f, "shop has fallback bounds");

    const bs3d::WorldAssetDefinition* road = registry.find("road_asphalt");
    expect(road != nullptr, "road definition is findable");
    expect(road->originPolicy == "bottom_center", "road declares bottom-center placement origin");
    expect(road->renderBucket == "Opaque", "road declares opaque render bucket");
    expect(road->defaultCollision == "Ground", "road declares ground collision intent");

    const bs3d::WorldAssetDefinition* patch = registry.find("irregular_asphalt_patch");
    expect(patch != nullptr, "irregular asphalt patch definition is findable");
    expect(patch->renderBucket == "Decal", "irregular patch declares decal render bucket");
    expect(patch->defaultCollision == "None", "irregular patch opts out of collision");
    expect(std::find(patch->defaultTags.begin(), patch->defaultTags.end(), "decal") != patch->defaultTags.end(),
           "irregular patch carries a decal tag for authoring tools");

    const bs3d::WorldAssetDefinition* shopV3 = requireAsset(registry, "shop_zenona_v3");
    expect(shopV3->assetType == "SolidBuilding", "shop v3 declares SolidBuilding type");
    expect(shopV3->renderBucket == "Opaque", "shop v3 declares opaque render bucket");
    expect(shopV3->defaultCollision == "SolidWorld", "shop v3 declares solid world collision");
    expect(shopV3->cameraBlocks, "shop v3 declares camera blocking");
    expect(shopV3->requiresClosedMesh, "shop v3 declares closed mesh requirement");

    const bs3d::WorldAssetDefinition* shopV2 = requireAsset(registry, "shop_zenona_v2");
    expect(shopV2->assetType == "DebugOnly", "legacy shop v2 declares DebugOnly type");
    expect(shopV2->renderBucket == "DebugOnly", "legacy shop v2 declares DebugOnly render bucket");
    expect(!shopV2->renderInGameplay, "legacy shop v2 is not gameplay-rendered");

    const bs3d::WorldAssetDefinition* parkingLine = requireAsset(registry, "parking_line");
    expect(parkingLine->assetType == "Decal", "parking line declares Decal type");
    expect(parkingLine->renderBucket == "Decal", "parking line declares decal render bucket");
    expect(parkingLine->defaultCollision == "None", "parking line declares no collision");

    const bs3d::WorldAssetDefinition* shopWindow = requireAsset(registry, "shop_window");
    expect(shopWindow->assetType == "Glass", "shop window declares Glass type");
    expect(shopWindow->renderBucket == "Glass", "shop window declares glass render bucket");

    expect(registry.find("not_a_real_asset") == nullptr, "missing assets return nullptr instead of crashing");
}

void assetRegistryValidatesManifestFilesAndModelLoadingReportsWarnings() {
    bs3d::WorldAssetRegistry registry;
    registry.addDefinition({"missing_runtime_model", "models/does_not_exist.obj", {1.0f, 1.0f, 1.0f}, {255, 255, 255, 255}});

    const bs3d::AssetValidationResult validation = registry.validateAssets("data/assets");
    expect(!validation.ok, "manifest validation fails for missing model files");
    expect(validation.missingAssets.size() == 1, "manifest validation reports missing file count");
    expect(validation.warnings[0].find("missing_runtime_model") != std::string::npos,
           "manifest validation warning names the asset id");

    bs3d::WorldAssetRegistry shipped;
    const bs3d::AssetManifestLoadResult manifest = shipped.loadManifest("data/assets/asset_manifest.txt");
    expect(manifest.loaded, "shipped manifest loads before validation");
    const bs3d::AssetValidationResult shippedValidation = shipped.validateAssets("data/assets");
    expect(shippedValidation.ok, "shipped manifest validates all referenced starter assets");
}

void shippedObjFacesStayWithinRaylibTinyObjLimits() {
    const std::filesystem::path modelRoot =
        resolveExistingPath(std::filesystem::path("data") / "assets" / "models");
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(modelRoot)) {
        if (entry.path().extension() != ".obj") {
            continue;
        }
        const std::string text = readTextFile(entry.path().string());
        std::istringstream lines(text);
        std::string line;
        int lineNumber = 0;
        while (std::getline(lines, line)) {
            ++lineNumber;
            const int vertices = objFaceVertexCount(line);
            if (vertices == 0) {
                continue;
            }
            expect(vertices <= 4,
                   entry.path().filename().string() + " face line " + std::to_string(lineNumber) +
                       " exceeds raylib tinyobj safe polygon size");
        }
    }
}

void shippedModelGeometryMatchesRuntimeAuthoringContract() {
    for (const std::string& path : {
             "data/assets/models/road_asphalt.obj",
             "data/assets/models/parking_surface.obj",
         }) {
        const ObjMeshInfo mesh = readObjMeshInfo(path);
        for (const std::vector<int>& face : mesh.faces) {
            const bs3d::Vec3 normal = faceNormal(mesh, face);
            expect(normal.y > 0.0f, path + " horizontal render face winds upward for Y-up lighting");
        }
    }

    const ObjMeshInfo patch = readObjMeshInfo("data/assets/models/irregular_patch.obj");
    expect(patch.maxY - patch.minY <= 0.05f,
           "irregular patch remains decal-thin instead of becoming meter-high geometry");

    const ObjMeshInfo block = readObjMeshInfo("data/assets/models/block13_core.obj");
    int blockFrontDepthBands = 0;
    bool hasDeepFacadeRelief = false;
    bool hasRaisedFacadeDetail = false;
    bool hasEntryScaleDetail = false;
    std::vector<float> frontDepths;
    for (const bs3d::Vec3& vertex : block.vertices) {
        if (vertex.z >= -0.50f) {
            continue;
        }
        bool newDepth = true;
        for (float depth : frontDepths) {
            if (std::fabs(vertex.z - depth) <= 0.001f) {
                newDepth = false;
                break;
            }
        }
        if (newDepth) {
            frontDepths.push_back(vertex.z);
        }
        hasDeepFacadeRelief = hasDeepFacadeRelief || vertex.z <= -0.62f;
        hasRaisedFacadeDetail = hasRaisedFacadeDetail || vertex.z <= -0.58f;
        hasEntryScaleDetail = hasEntryScaleDetail || (vertex.y <= 0.26f && vertex.z <= -0.58f);
    }
    blockFrontDepthBands = static_cast<int>(frontDepths.size());
    expect(block.faces.size() >= 30, "Block 13 core has modeled facade detail, not only a box plus flat quads");
    expect(blockFrontDepthBands >= 3, "Block 13 front uses several depth layers for windows/frames/entry relief");
    expect(hasDeepFacadeRelief, "Block 13 has protruding facade pieces that read as frames or parapets");
    expect(hasRaisedFacadeDetail, "Block 13 facade has raised geometry beyond the glass/decal plane");
    expect(hasEntryScaleDetail, "Block 13 core includes entry-height relief instead of only upper flat windows");

    const ObjMeshInfo bench = readObjMeshInfo("data/assets/models/bench.obj");
    expectNear(bench.minY, 0.0f, 0.001f, "bench mesh is base-anchored on the ground");
}

void defaultWorldPresentationStyleSupportsReadableGrochowLook() {
    const bs3d::WorldPresentationStyle& style = bs3d::defaultWorldPresentationStyle();

    expect(style.skyColor.a == 255, "presentation sky is opaque for stable clear background");
    expect(style.groundColor.a == 255, "presentation ground is opaque for stable world base");
    expect(style.horizonHazeColor.a > 0 && style.horizonHazeColor.a < 140,
           "presentation haze stays subtle enough for gameplay readability");
    expect(colorDistanceSquared(style.skyColor, style.groundColor) > 3000,
           "presentation sky and ground remain visually separated");
    expect(style.groundPlaneSize >= 90.0f,
           "presentation ground plane covers the expanded Blok 13/Grochow slice");
    expect(style.worldCullDistance >= 145.0f,
           "presentation cull distance leaves room for district-scale landmarks");
}

void worldAtmosphereBandsFollowCameraAndStaySubtle() {
    const bs3d::WorldPresentationStyle& style = bs3d::defaultWorldPresentationStyle();
    const bs3d::Vec3 cameraPosition{11.0f, 4.0f, -7.0f};

    const std::vector<bs3d::WorldAtmosphereBand> bands =
        bs3d::buildWorldAtmosphereBands(cameraPosition, style);

    expect(bands.size() == 2, "presentation atmosphere builds inner and outer haze bands");
    for (const bs3d::WorldAtmosphereBand& band : bands) {
        expectNear(band.center.x, cameraPosition.x, 0.001f, "atmosphere band follows camera x");
        expectNear(band.center.z, cameraPosition.z, 0.001f, "atmosphere band follows camera z");
        expect(band.center.y >= 0.0f && band.center.y <= 0.05f,
               "atmosphere band remains near ground instead of occluding gameplay");
        expect(band.height > 0.0f && band.height <= 0.05f, "atmosphere band stays thin");
        expect(band.radiusBottom > band.radiusTop, "atmosphere band tapers toward the horizon");
        expect(band.radiusBottom <= style.worldCullDistance * 0.43f,
               "atmosphere band remains within world render distance");
        expect(band.color.a > 0 && band.color.a <= style.horizonHazeColor.a,
               "atmosphere band alpha stays subtle");
    }
    expect(bands[0].color.a < bands[1].color.a, "outer haze is stronger than the inner wash");
}

class FixedHudTextMetrics final : public bs3d::HudTextMetrics {
public:
    int measureTextWidth(const std::string& text, int) const override {
        return static_cast<int>(text.size()) * 10;
    }
};

void hudPanelLayoutKeepsGameplayPanelsSeparated() {
    bs3d::VehicleHudSnapshot vehicle;
    vehicle.visible = true;
    vehicle.chaseActive = false;
    bs3d::HudPanelRect vehicleRect = bs3d::vehicleHudPanelRect(vehicle, 1024);
    expect(vehicleRect.x == 688 && vehicleRect.y == 126 && vehicleRect.width == 318 && vehicleRect.height == 82,
           "vehicle HUD keeps right-side default panel rect");

    bs3d::PrzypalHudSnapshot przypal;
    przypal.playerInVehicle = true;
    przypal.reactionToastSeconds = 1.0f;
    bs3d::HudPanelRect przypalRect = bs3d::przypalHudPanelRect(przypal, 1024);
    expect(przypalRect.x == 18 && przypalRect.y == 126 && przypalRect.width == 318 && przypalRect.height == 104,
           "przypal HUD expands on the left when player is in vehicle and reaction toast is visible");

    const bs3d::HudPanelRect chaseRect = bs3d::chaseHudMeterRect(1024);
    expect(chaseRect.x == 772 && chaseRect.y == 104 && chaseRect.width == 220 && chaseRect.height == 24,
           "chase HUD meter stays anchored near the top-right corner");

    bs3d::HudSnapshot failed;
    failed.screenWidth = 1024;
    failed.screenHeight = 720;
    failed.missionPhase = bs3d::MissionPhase::Failed;
    const bs3d::HudMissionBannerLayout failedBanner = bs3d::missionBannerLayout(failed);
    expect(failedBanner.visible &&
               failedBanner.rect.x == 0 &&
               failedBanner.rect.y == 324 &&
               failedBanner.rect.width == 1024 &&
               failedBanner.rect.height == 72 &&
               failedBanner.textX == 322 &&
               failedBanner.textY == 348 &&
               std::string(failedBanner.text) == "MISJA NIEUDANA - naciśnij R",
           "failed mission banner preserves legacy centered layout");

    bs3d::HudSnapshot completed;
    completed.screenWidth = 1024;
    completed.screenHeight = 720;
    completed.missionPhase = bs3d::MissionPhase::Completed;
    completed.flashAlpha = 1.0f;
    const bs3d::HudMissionBannerLayout completedBanner = bs3d::missionBannerLayout(completed);
    expect(completedBanner.visible &&
               completedBanner.textX == 376 &&
               completedBanner.textY == 348 &&
               std::string(completedBanner.text) == "MISJA ZALICZONA",
           "completed mission banner preserves legacy centered layout");
}

void hudMeasuredLayoutUsesBackendNeutralTextMetrics() {
    const FixedHudTextMetrics metrics;

    bs3d::DialogueLine line;
    line.speaker = "Zenon";
    line.text = "hello world";
    const bs3d::HudDialogueBoxLayout dialogueLayout = bs3d::dialogueBoxLayout(line, 800, 600, metrics);
    expect(dialogueLayout.rect.x == 120 &&
               dialogueLayout.rect.y == 486 &&
               dialogueLayout.rect.width == 560 &&
               dialogueLayout.rect.height == 86 &&
               dialogueLayout.speakerX == 146 &&
               dialogueLayout.firstLineY == 528 &&
               dialogueLayout.lines.size() == 1,
           "dialogue box measured layout is deterministic with injected text metrics");

    bs3d::InteractionPromptSnapshot prompt;
    prompt.lines = {"Use", "Talk"};
    const bs3d::HudInteractionPromptLayout promptLayout = bs3d::interactionPromptLayout(prompt, 800, 600, metrics);
    expect(promptLayout.visible &&
               promptLayout.rect.x == 360 &&
               promptLayout.rect.y == 424 &&
               promptLayout.rect.width == 80 &&
               promptLayout.rect.height == 74 &&
               promptLayout.lineX == 380 &&
               promptLayout.firstLineY == 434,
           "interaction prompt measured layout is deterministic with injected text metrics");

    bs3d::HudSnapshot hud;
    hud.screenWidth = 900;
    hud.screenHeight = 600;
    hud.objectiveLine = "Cel: Dostarcz paczkę (120m)";
    const bs3d::HudObjectivePanelLayout objectiveLayout = bs3d::objectivePanelLayout(hud, metrics);
    expect(objectiveLayout.rect.x == 18 &&
               objectiveLayout.rect.y == 18 &&
               objectiveLayout.rect.width == 300 &&
               objectiveLayout.rect.height == 72 &&
               objectiveLayout.distanceX == 260 &&
               objectiveLayout.distanceY == 28 &&
               objectiveLayout.objective.title == "Dostarcz paczkę" &&
               objectiveLayout.objective.distance == "120m",
           "objective panel measured layout is deterministic with injected text metrics");
}

void gitignoreKeepsAuthoredObjAssetsTrackable() {
    const std::string gitignore = readTextFile(".gitignore");
    expect(!gitignore.empty(), "gitignore is readable");
    expect(!textContains(gitignore, "\n*.obj\n") && gitignore.rfind("*.obj", 0) != 0,
           "gitignore does not globally ignore authored .obj assets");
    expect(textContains(gitignore, "!data/assets/models/**/*.obj"),
           "gitignore explicitly keeps authored OBJ models trackable");
}

void raylibContainmentKeepsDataAndAssetHeadersBackendAgnostic() {
    const std::string registryHeader = readTextFile("src/game/WorldAssetRegistry.h");
    const std::string levelBuilderHeader = readTextFile("src/game/IntroLevelBuilder.h");
    const std::string gameplayAuthoring = readTextFile("src/game/IntroLevelGameplayData.cpp");
    const std::string runtimeAudioHeader = readTextFile("src/game/RuntimeAudio.h");
    const std::string gameRenderersHeader = readTextFile("src/game/GameRenderers.h");
    const std::string gameHudLayoutHeader = readTextFile("src/game/GameHudLayout.h");
    const std::string gameHudLayoutSource = readTextFile("src/game/GameHudLayout.cpp");
    const std::string gameHudPainterHeader = readTextFile("src/game/GameHudPainter.h");
    const std::string gameHudPainterSource = readTextFile("src/game/GameHudPainter.cpp");
    const std::string gameHudTextMetricsHeader = readTextFile("src/game/GameHudTextMetrics.h");
    const std::string gameHudTextMetricsSource = readTextFile("src/game/GameHudTextMetrics.cpp");
    const std::string gameHudRendererSource = readTextFile("src/game/GameHudRenderers.cpp");
    const std::string gameRendererSource = readTextFile("src/game/GameRenderers.cpp");
    const std::string introPresentationLogic = readTextFile("src/game/IntroLevelPresentation.cpp");
    const std::string gameApp = readTextFile("src/game/GameApp.cpp");
    const std::string raylibInputHeader = readTextFile("src/game/RaylibInputReader.h");
    const std::string raylibPlatformHeader = readTextFile("src/game/RaylibPlatform.h");
    const std::string cmakeLists = readTextFile("CMakeLists.txt");

    expect(!textContains(registryHeader, "raylib.h"),
           "asset registry public header stays backend-agnostic");
    expect(!textContains(registryHeader, "Color fallbackColor"),
           "asset registry exposes engine-owned color data instead of raylib Color");
    expect(!textContains(levelBuilderHeader, "GameRenderers.h") &&
               !textContains(levelBuilderHeader, "raylib.h"),
           "level data builder does not depend on renderer/raylib headers");
    expect(!textContains(gameplayAuthoring, "raylib.h") &&
               !textContains(gameplayAuthoring, "Color "),
           "level gameplay authoring stays independent from raylib Color");
    expect(!textContains(runtimeAudioHeader, "raylib.h") &&
               !textContains(runtimeAudioHeader, "Sound "),
           "runtime audio public header hides raylib audio handles");
    expect(!textContains(gameRenderersHeader, "raylib.h") &&
               !textContains(gameRenderersHeader, "Camera3D") &&
               !textContains(gameRenderersHeader, " Color "),
           "game renderer public header exposes engine-owned render camera/color types");
    expect(!textContains(gameHudLayoutHeader, "raylib.h") &&
               !textContains(gameHudLayoutSource, "raylib.h") &&
               !textContains(gameHudLayoutSource, "Draw") &&
               !textContains(gameHudLayoutSource, "MeasureText") &&
               !textContains(gameHudLayoutSource, "GetWorldToScreen"),
           "HUD layout helper unit stays independent from raylib drawing and text measurement");
    expect(!textContains(gameHudTextMetricsHeader, "raylib.h") &&
               !textContains(gameHudTextMetricsSource, "raylib.h") &&
               !textContains(gameHudTextMetricsSource, "MeasureText") &&
               !textContains(gameHudTextMetricsSource, "Draw"),
           "HUD text wrapping logic depends on backend-neutral text metrics only");
    expect(!textContains(gameHudPainterHeader, "raylib.h") &&
               !textContains(gameHudPainterHeader, "Camera3D") &&
               !textContains(gameHudPainterHeader, "Vector2") &&
               !textContains(gameHudPainterHeader, "DrawText") &&
               !textContains(gameHudPainterHeader, "DrawRectangle") &&
               !textContains(gameHudPainterSource, "raylib.h") &&
               !textContains(gameHudPainterSource, "DrawText") &&
               !textContains(gameHudPainterSource, "DrawRectangle"),
           "HUD draw command API stays backend-neutral");
    expect(textContains(gameHudRendererSource, "dialogueBoxLayout(") &&
               textContains(gameHudRendererSource, "interactionPromptLayout(") &&
               textContains(gameHudRendererSource, "objectivePanelLayout(") &&
               textContains(gameHudRendererSource, "objectiveCompassLabelLayout("),
           "HUD renderer delegates measured layout decisions to the backend-free layout unit");
    expect(textContains(gameHudRendererSource, "RaylibHudDrawSink") &&
               textContains(gameHudRendererSource, "HudDrawSink& hudDrawSink()"),
           "HUD renderer adapts backend-neutral draw commands to raylib locally");
    expect(textContains(gameHudRendererSource, "objectiveCompassLayout(") &&
               textContains(gameHudRendererSource, "RaylibHudProjection"),
           "HUD renderer delegates objective compass projection to the backend-free layout unit");
    expect(textContains(gameHudLayoutHeader, "HudProjection") &&
               !textContains(gameHudLayoutSource, "GetWorldToScreen"),
           "HUD layout unit exposes backend-neutral projection interface");
    expect(!textContains(gameRendererSource, "HudRenderer::wrapTextToWidth") &&
               !textContains(gameRendererSource, "HudRenderer::drawDialogueBox") &&
               !textContains(gameRendererSource, "measureUiText") &&
               !textContains(gameRendererSource, "DrawTextEx"),
           "world renderer implementation no longer owns HUD text measurement or dialogue drawing");
    expect(!textContains(introPresentationLogic, "raylib.h") &&
               !textContains(introPresentationLogic, "Draw") &&
               !textContains(introPresentationLogic, "BeginBlendMode"),
           "intro presentation objective logic stays separate from raylib drawing");
    expect(textContains(cmakeLists, "IntroLevelPresentationRender.cpp"),
           "intro presentation raylib drawing is isolated in the render implementation unit");
    expect(!textContains(raylibInputHeader, "raylib.h") &&
               textContains(raylibInputHeader, "RawInputState"),
           "raylib input reader header exposes only engine-owned raw input");
    expect(!textContains(gameApp, "IsKeyDown") &&
               !textContains(gameApp, "IsKeyPressed") &&
               !textContains(gameApp, "IsMouseButton") &&
               !textContains(gameApp, "GetMouseDelta"),
           "game app delegates raw keyboard and mouse reads to the raylib input adapter");
    expect(!textContains(raylibPlatformHeader, "raylib.h") &&
               !textContains(raylibPlatformHeader, "KEY_") &&
               !textContains(raylibPlatformHeader, "FLAG_"),
           "raylib platform header exposes engine-owned window lifecycle API");
    expect(!textContains(gameApp, "InitWindow") &&
               !textContains(gameApp, "CloseWindow") &&
               !textContains(gameApp, "WindowShouldClose") &&
               !textContains(gameApp, "BeginDrawing") &&
               !textContains(gameApp, "EndDrawing") &&
               !textContains(gameApp, "SetConfigFlags") &&
               !textContains(gameApp, "SetTargetFPS") &&
               !textContains(gameApp, "SetExitKey") &&
               !textContains(gameApp, "ToggleFullscreen") &&
               !textContains(gameApp, "GetFrameTime") &&
               !textContains(gameApp, "GetFPS"),
           "game app delegates window lifecycle and frame timing to the raylib platform adapter");
    expect(textContains(cmakeLists, "RaylibInputReader.cpp"),
           "raylib input adapter is registered in the game support build");
    expect(textContains(cmakeLists, "RaylibPlatform.cpp"),
           "raylib platform adapter is registered in the game support build");
    expect(textContains(cmakeLists, "GameHudLayout.cpp"),
           "HUD layout helper unit is registered in the game support build");
    expect(textContains(cmakeLists, "GameHudPainter.cpp"),
           "HUD draw command helper unit is registered in the game support build");
    expect(textContains(cmakeLists, "GameHudTextMetrics.cpp"),
           "HUD text metrics helper unit is registered in the game support build");
}

void worldDataLoaderReadsRuntimeWorldAndMissionSchema() {
    const bs3d::WorldDataCatalog catalog = bs3d::loadWorldDataCatalog("data");
    expect(catalog.loaded, "world data catalog loads from data folder at runtime");
    expect(catalog.world.loaded, "map schema is loaded");
    expect(catalog.mission.loaded, "mission schema is loaded");
    expect(catalog.objectOutcomes.loaded, "object outcome catalog is loaded for runtime affordance policy");
    expect(catalog.mission.phases.size() >= 6, "mission schema exposes the full playable vertical-slice phase chain");
    expect(catalog.objectOutcomes.outcomes.size() >= 8, "object outcome catalog exposes authored hooks");
    expect(!catalog.mission.phases[0].objective.empty(), "mission phase objective is data-driven");
    const bs3d::ObjectOutcomeData* shopNoticeOutcome =
        bs3d::findObjectOutcomeData(catalog.objectOutcomes, "shop_notice_read_shop_handwritten_notice");
    expect(shopNoticeOutcome != nullptr && shopNoticeOutcome->worldEvent == std::nullopt,
           "runtime outcome data exposes quiet readable shop notice hooks");
    const bs3d::ObjectOutcomeData* blockNoticeOutcome =
        bs3d::findObjectOutcomeData(catalog.objectOutcomes, "block_notice_read_block13_entry_notice");
    expect(blockNoticeOutcome != nullptr && blockNoticeOutcome->worldEvent == std::nullopt,
           "runtime outcome data exposes quiet readable block notice hooks");
    const bs3d::ObjectOutcomeData* intercomOutcome =
        bs3d::findObjectOutcomeData(catalog.objectOutcomes, "block_intercom_buzzed");
    expect(intercomOutcome != nullptr && intercomOutcome->worldEvent.has_value(),
           "runtime outcome data loads intercom world event metadata");
    if (intercomOutcome != nullptr && intercomOutcome->worldEvent.has_value()) {
        expect(intercomOutcome->speaker == "Domofon",
               "runtime outcome data preserves player-facing speaker");
        expect(textContains(intercomOutcome->line, "caly blok"),
               "runtime outcome data preserves player-facing line");
        expect(intercomOutcome->worldEvent->type == bs3d::WorldEventType::PublicNuisance,
               "runtime outcome data maps PublicNuisance event type");
        expectNear(intercomOutcome->worldEvent->intensity, 0.20f, 0.001f,
                   "runtime outcome data preserves authored intercom intensity");
        expectNear(intercomOutcome->worldEvent->cooldownSeconds, 3.0f, 0.001f,
                   "runtime outcome data preserves authored intercom cooldown");
    }
    expect(catalog.world.districtBounds.size.x >= 82.0f &&
               catalog.world.districtBounds.size.z >= 73.0f,
           "world schema exposes authored bounds for the expanded Blok 13/Grochow footprint");
    expectNear(catalog.world.districtBounds.center.x, -7.0f, 0.001f,
               "world schema district bounds center matches authored boundary wall footprint x");
    expectNear(catalog.world.districtBounds.center.z, -1.5f, 0.001f,
               "world schema district bounds center matches authored boundary wall footprint z");

    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    const bs3d::WorldDataApplyResult applied = bs3d::applyWorldDataCatalog(level, catalog);
    expect(applied.applied, "runtime intro level accepts data catalog overlay");
    expect(applied.appliedMissionPhases >= 6, "mission phase data covers the full playable intro loop");
}

void worldDataMissionDefinesCompletePlayableVerticalSlice() {
    const bs3d::WorldDataCatalog catalog = bs3d::loadWorldDataCatalog("data");
    expect(catalog.loaded && catalog.mission.loaded, "runtime mission catalog loads for vertical slice contract");
    expect(catalog.mission.id == "driving_errand_vertical_slice", "runtime mission keeps vertical slice id");
    expect(catalog.mission.phases.size() == 6, "runtime mission defines six playable beats");

    const std::vector<std::string> expectedPhases{
        "WalkToShop",
        "ReturnToBench",
        "ReachVehicle",
        "DriveToShop",
        "ChaseActive",
        "ReturnToLot"};
    const std::vector<std::string> expectedTriggers{
        "shop_walk_intro",
        "npc_return_bogus",
        "player_enters_vehicle",
        "vehicle_reaches_shop_marker",
        "sustained_escape_distance",
        "vehicle_reaches_dropoff_marker"};

    for (std::size_t i = 0; i < expectedPhases.size(); ++i) {
        expect(catalog.mission.phases[i].phase == expectedPhases[i],
               "vertical slice mission phase order is stable: " + expectedPhases[i]);
        expect(catalog.mission.phases[i].trigger == expectedTriggers[i],
               "vertical slice mission trigger is authored: " + expectedTriggers[i]);
        expect(!catalog.mission.phases[i].objective.empty(),
               "vertical slice mission objective is player-facing: " + expectedPhases[i]);
    }

    bs3d::DialogueSystem dialogue;
    bs3d::MissionController mission(dialogue);
    const int applied = bs3d::applyMissionDataToController(mission, catalog.mission);
    expect(applied == 6, "all vertical slice objectives apply to the mission controller");
    expect(mission.objectiveText() == "Podejdź do Zenona — sprawdź, czy dług jeszcze żyje",
           "waiting objective previews the local-motivation walk-to-shop intent");

    mission.start();
    expect(mission.phase() == bs3d::MissionPhase::WalkToShop, "vertical slice starts with the on-foot shop check");
    expect(mission.objectiveText() == "Podejdź do Zenona — sprawdź, czy dług jeszcze żyje",
           "walk-to-shop objective hints at debt ledger stakes");
    expect(dialogue.currentLine().speaker == "Boguś", "walk-to-shop beat has authored Bogus dialogue");

    mission.onShopVisitedOnFoot();
    expect(mission.phase() == bs3d::MissionPhase::ReturnToBench, "shop beat sends player back to Bogus");
    expect(mission.objectiveText() == "Wróć do Bogusia z wieściami",
           "return-to-bench objective connects the round trip");

    mission.onReturnedToBogus();
    expect(mission.phase() == bs3d::MissionPhase::ReachVehicle, "Bogus return unlocks gruz step");
    expect(mission.objectiveText() == "Wsiądź do gruza",
           "vehicle entry objective uses local vehicle name");

    mission.onPlayerEnteredVehicle();
    expect(mission.phase() == bs3d::MissionPhase::DriveToShop, "vehicle entry starts drive-to-shop step");
    expect(mission.objectiveText() == "Podjedź gruzem pod Zenona",
           "drive-to-shop objective uses vehicle identity in instruction");

    mission.onShopReachedByVehicle();
    expect(mission.phase() == bs3d::MissionPhase::ChaseActive, "shop vehicle trigger starts chase step");
    expect(mission.objectiveText() == "Zgub przypał",
           "chase objective uses local slang");

    mission.onChaseEscaped();
    expect(mission.phase() == bs3d::MissionPhase::ReturnToLot, "escape step sends player back to parking");
    expect(mission.objectiveText() == "Wróć na parking",
           "return-to-lot objective is concise for HUD");

    mission.onDropoffReached();
    expect(mission.phase() == bs3d::MissionPhase::Completed, "dropoff completes the vertical slice");
}

void worldDataLoaderParsesJsonSchemaWithoutFieldOrderCoupling() {
    const std::filesystem::path root = std::filesystem::path("artifacts") / "schema_order_test";
    std::filesystem::create_directories(root);
    {
        std::ofstream map(root / "map_block_loop.json");
        map << "{\n"
            << "  \"spawns\": {\"vehicle\": [4,0,5], \"npc\": [1,0,2], \"player\": [0,0,1]},\n"
            << "  \"id\": \"test_block\",\n"
            << "  \"districtBounds\": {\"size\": [44, 5, 42], \"center\": [2, 0, -3]},\n"
            << "  \"points\": {\"dropoff\": [8,0,9], \"shop\": [6,0,7]}\n"
            << "}\n";
    }
    {
        std::ofstream mission(root / "mission_driving_errand.json");
        mission << "{\n"
                << "  \"title\": \"Order Test\",\n"
                << "  \"id\": \"mission_order_test\",\n"
                << "  \"phases\": [\n"
                << "    {\"trigger\": \"shop_walk_intro\", \"objective\": \"Data objective A\", \"phase\": \"WalkToShop\"},\n"
                << "    {\"objective\": \"Data objective B\", \"phase\": \"DriveToShop\", \"trigger\": \"shop_vehicle_intro\"}\n"
                << "  ],\n"
                << "  \"dialogue\": [\n"
                << "    {\"speaker\": \"Zenon\", \"lineKey\": \"driving.zenon\", \"phase\": \"WalkToShop\"}\n"
                << "  ],\n"
                << "  \"npcReactions\": [\n"
                << "    {\"speaker\": \"Bogus\", \"phase\": \"WalkToShop\", \"durationSeconds\": 2.2, \"lineKey\": \"npc.bogus\"},\n"
                << "    {\"speaker\": \"\", \"phase\": \"DriveToShop\", \"durationSeconds\": 3.0, \"line\": \"Malformed missing speaker should be ignored\"}\n"
                << "  ],\n"
                << "  \"cutscenes\": [\n"
                << "    {\"cutscene\": \"shop_open\", \"speaker\": \"Zenon\", \"phase\": \"DriveToShop\", \"lineKey\": \"cut.shop_open\", \"durationSeconds\": 2.5},\n"
                << "    {\"cutscene\": \"\", \"speaker\": \"Zenon\", \"phase\": \"WalkToShop\", \"text\": \"Malformed missing cutscene id should be ignored\"}\n"
                << "  ]\n"
                << "}\n";
    }
    {
        std::filesystem::create_directories(root / "world");
        std::ofstream localization(root / "world" / "mission_localization_pl.json");
        localization << "{\n"
                    << "  \"schemaVersion\": 1,\n"
                    << "  \"lines\": {\n"
                    << "    \"driving.zenon\": \"Runtime JSON line\",\n"
                    << "    \"npc.bogus\": \"Nie mow za glosno na podworce.\",\n"
                    << "    \"cut.shop_open\": \"Otworz drzwi po kolei.\"\n"
                    << "  }\n"
                    << "}\n";
    }

    const bs3d::WorldDataCatalog catalog = bs3d::loadWorldDataCatalog(root.string());
    expect(catalog.loaded, "loader accepts valid JSON schema from runtime data root");
    expect(catalog.world.id == "test_block", "map id parses independent of object field order");
    expectNear(catalog.world.playerSpawn.z, 1.0f, 0.001f, "nested player spawn parses from map schema");
    expectNear(catalog.world.districtBounds.center.x, 2.0f, 0.001f,
               "authored map bounds center parses independent of object field order");
    expectNear(catalog.world.districtBounds.size.z, 42.0f, 0.001f,
               "authored map bounds size parses independent of object field order");
    expect(catalog.mission.phases.size() == 2, "mission phases parse independent of object field order");
    expect(catalog.mission.phases[0].objective == "Data objective A",
           "mission objective survives reordered phase fields");
    expect(catalog.mission.dialogue.size() == 1, "mission dialogue is loaded as runtime data");
    expect(catalog.mission.dialogue[0].speaker == "Zenon", "mission dialogue speaker parses from JSON");
    expect(catalog.mission.dialogue[0].lineKey == "driving.zenon", "mission dialogue supports lineKey resolution");
    expect(catalog.mission.dialogue[0].text == "Runtime JSON line", "lineKey resolves to runtime text map");
    expect(catalog.mission.npcReactions.size() == 1, "malformed npcReactions are ignored while valid entries parse");
    expect(catalog.mission.npcReactions[0].speaker == "Bogus", "npcReaction speaker parses from JSON");
    expect(catalog.mission.npcReactions[0].lineKey == "npc.bogus", "npcReaction supports lineKey");
    expect(catalog.mission.npcReactions[0].text == "Nie mow za glosno na podworce.",
           "npcReaction lineKey resolves to runtime text map");
    expect(catalog.mission.cutscenes.size() == 1, "malformed cutscenes are ignored while valid entries parse");
    expect(catalog.mission.cutscenes[0].cutscene == "shop_open", "cutscene id parses from JSON");
    expect(catalog.mission.cutscenes[0].phase == "DriveToShop", "cutscene phase parses from JSON");
    expect(catalog.mission.cutscenes[0].text == "Otworz drzwi po kolei.", "cutscene text resolves to runtime text map");
}

void worldDataLoaderAppliesMissionPhaseLinesToMissionController() {
    const std::filesystem::path root = std::filesystem::path("artifacts") / "mission_phase_lines_test";
    std::filesystem::create_directories(root);
    {
        std::ofstream map(root / "map_block_loop.json");
        map << "{\n"
            << "  \"spawns\": {\"vehicle\": [2,0,3], \"npc\": [0,0,1], \"player\": [1,0,2]},\n"
            << "  \"id\": \"mission_lines_test_block\",\n"
            << "  \"points\": {\"dropoff\": [6,0,4], \"shop\": [3,0,2]}\n"
            << "}\n";
    }
    {
        std::ofstream mission(root / "mission_driving_errand.json");
        mission << "{\n"
                << "  \"title\": \"Phase Line Test\",\n"
                << "  \"id\": \"mission_phase_lines\",\n"
                << "  \"steps\": [\n"
                << "    {\"phase\": \"WalkToShop\", \"objective\": \"Cel: Wejdz do Zenona\", \"trigger\": \"shop_walk_intro\"},\n"
                << "    {\"phase\": \"DriveToShop\", \"objective\": \"Cel: Podejrzany ruch\", \"trigger\": \"shop_vehicle_intro\"}\n"
                << "  ],\n"
                << "  \"dialogue\": [\n"
                << "    {\"speaker\": \"Guide\", \"phase\": \"WalkToShop\", \"text\": \"Misja: idz do drzwi\", \"durationSeconds\": 1.2}\n"
                << "  ],\n"
                << "  \"npcReactions\": [\n"
                << "    {\"speaker\": \"Bystra\", \"phase\": \"WalkToShop\", \"text\": \"Uwaga: sluchaj\", \"durationSeconds\": 1.1}\n"
                << "  ],\n"
                << "  \"cutscenes\": [\n"
                << "    {\"cutscene\": \"open_gate\", \"speaker\": \"System\", \"phase\": \"WalkToShop\", \"text\": \"Wejdz cicho\", \"durationSeconds\": 1.3}\n"
                << "  ]\n"
                << "}\n";
    }

    const bs3d::WorldDataCatalog catalog = bs3d::loadWorldDataCatalog(root.string());
    expect(catalog.loaded, "test root mission catalog loads");
    expect(catalog.mission.phases.size() == 2, "test mission has two authored objective phases");

    bs3d::DialogueSystem dialogue;
    bs3d::MissionController mission(dialogue);
    const int applied = bs3d::applyMissionDataToController(mission, catalog.mission);
    expect(applied == 2, "both authored mission objectives are applied to controller");
    expect(mission.objectiveText() == "Cel: Wejdz do Zenona",
           "mission controller resolves phase objective override on start");

    mission.start();
    expect(dialogue.currentLine().speaker == "Guide",
           "phase mission dialogue queues before objective system line");
    expect(dialogue.currentLine().durationSeconds >= 1.19f && dialogue.currentLine().durationSeconds <= 1.21f,
           "phase mission dialogue carries authored duration");

    dialogue.update(dialogue.currentLine().durationSeconds + 0.01f);
    expect(dialogue.currentLine().speaker == "Misja", "objective hint remains queued after phase dialogue");

    dialogue.update(dialogue.currentLine().durationSeconds + 0.01f);
    expect(dialogue.currentLine().speaker == "System", "phase cutscene hint queues after objective");

    dialogue.update(dialogue.currentLine().durationSeconds + 0.01f);
    expect(dialogue.currentLine().speaker == "Bystra", "phase reaction lines queue after higher-priority queues clear");
    expect(dialogue.hasLine(), "phase data lines remain queued in predictable priority order");
}

void objectAffordanceWorldEventsPreferLoadedOutcomeData() {
    bs3d::ObjectOutcomeCatalogData outcomes;
    outcomes.loaded = true;
    outcomes.outcomes.push_back({"block_intercom_buzzed",
                                 "",
                                 "Block intercom",
                                 "Block",
                                 "",
                                 "",
                                 bs3d::ObjectOutcomeWorldEventData{
                                     bs3d::WorldEventType::PublicNuisance,
                                     0.24f,
                                     4.0f}});
    outcomes.outcomes.push_back({"",
                                 "trash_disturbed_*",
                                 "Trash disturbed",
                                 "Trash",
                                 "",
                                 "",
                                 bs3d::ObjectOutcomeWorldEventData{
                                     bs3d::WorldEventType::PublicNuisance,
                                     0.21f,
                                     5.0f}});

    bs3d::WorldObjectInteractionAffordance intercom;
    intercom.outcomeId = "block_intercom_buzzed";
    intercom.position = {1.0f, 0.0f, 2.0f};
    const std::optional<bs3d::WorldObjectInteractionEvent> intercomEvent =
        bs3d::worldEventForObjectAffordance(intercom, &outcomes);
    expect(intercomEvent.has_value(), "loaded outcome data can drive exact affordance event");
    if (intercomEvent.has_value()) {
        expectNear(intercomEvent->intensity, 0.24f, 0.001f,
                   "data-driven affordance event uses catalog intensity over hardcoded fallback");
        expectNear(intercomEvent->cooldownSeconds, 4.0f, 0.001f,
                   "data-driven affordance event uses catalog cooldown over hardcoded fallback");
    }

    bs3d::WorldObjectInteractionAffordance trash;
    trash.outcomeId = "trash_disturbed_trash_green_bin_0";
    const std::optional<bs3d::WorldObjectInteractionEvent> trashEvent =
        bs3d::worldEventForObjectAffordance(trash, &outcomes);
    expect(trashEvent.has_value(), "loaded outcome data can drive pattern affordance event");
    if (trashEvent.has_value()) {
        expectNear(trashEvent->intensity, 0.21f, 0.001f,
                   "data-driven pattern affordance event uses catalog intensity");
        expect(trashEvent->source == trash.outcomeId,
               "data-driven pattern affordance event keeps concrete outcome id as source");
    }
}

void objectAffordancesPreferLoadedOutcomeText() {
    bs3d::ObjectOutcomeCatalogData outcomes;
    outcomes.loaded = true;
    outcomes.outcomes.push_back({"block_intercom_buzzed",
                                 "",
                                 "Block intercom",
                                 "Block",
                                 "Domofon z danych",
                                 "Tekst z katalogu outcome zamiast fallbacku C++.",
                                 bs3d::ObjectOutcomeWorldEventData{
                                     bs3d::WorldEventType::PublicNuisance,
                                     0.24f,
                                     4.0f}});
    outcomes.outcomes.push_back({"",
                                 "trash_disturbed_*",
                                 "Trash disturbed",
                                 "Trash",
                                 "Smietnik z danych",
                                 "Patternowy tekst z katalogu outcome.",
                                 std::nullopt});

    bs3d::WorldObjectInteractionAffordance intercom;
    intercom.outcomeId = "block_intercom_buzzed";
    intercom.speaker = "Fallback";
    intercom.line = "Fallback line";
    const bs3d::WorldObjectInteractionAffordance enrichedIntercom =
        bs3d::worldObjectAffordanceWithOutcomeData(intercom, &outcomes);
    expect(enrichedIntercom.speaker == "Domofon z danych",
           "exact outcome data overrides affordance speaker");
    expect(enrichedIntercom.line == "Tekst z katalogu outcome zamiast fallbacku C++.",
           "exact outcome data overrides affordance line");

    bs3d::WorldObjectInteractionAffordance trash;
    trash.outcomeId = "trash_disturbed_trash_green_bin_0";
    trash.speaker = "Fallback";
    trash.line = "Fallback line";
    const bs3d::WorldObjectInteractionAffordance enrichedTrash =
        bs3d::worldObjectAffordanceWithOutcomeData(trash, &outcomes);
    expect(enrichedTrash.speaker == "Smietnik z danych",
           "pattern outcome data overrides affordance speaker");
    expect(enrichedTrash.line == "Patternowy tekst z katalogu outcome.",
           "pattern outcome data overrides affordance line");
}

void worldDataMissionDialogueUsesCanonActors() {
    const bs3d::WorldDataCatalog catalog = bs3d::loadWorldDataCatalog("data");
    expect(!catalog.mission.dialogue.empty(), "runtime mission data exposes dialogue lines");
    for (const bs3d::MissionDialogueData& line : catalog.mission.dialogue) {
        expect(!textContains(line.speaker, "Spejson") && !textContains(line.speaker, "Wojtas"),
               "runtime mission dialogue does not reintroduce old placeholder speakers");
        expect(!line.text.empty(), "runtime mission dialogue line is readable");
    }
}

void runtimeDevToolsAreBuildGatedAndHudHelpIsReleaseSafe() {
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
    expect(bs3d::runtimeDevToolsEnabledByDefault(),
           "dev tools are enabled by default only in explicit dev-tool builds");
#else
    expect(!bs3d::runtimeDevToolsEnabledByDefault(),
           "dev tools are disabled by default for production/release builds");
#endif
    const std::string releaseHelp = bs3d::buildHudControlHelp(false, false);
    expect(!textContains(releaseHelp, "F8") && !textContains(releaseHelp, "F9") && !textContains(releaseHelp, "F3"),
           "release HUD control help hides QA/dev shortcuts");
    const std::string devHelp = bs3d::buildHudControlHelp(true, true);
    expect(textContains(devHelp, "F1 debug") && !textContains(devHelp, "F8") && !textContains(devHelp, "F9"),
           "gameplay HUD only exposes debug toggle in dev builds");
    const std::string debugHelp = bs3d::buildHudDebugHelp();
    expect(textContains(debugHelp, "F5 stable cam") && textContains(debugHelp, "F8 widok QA") &&
               textContains(debugHelp, "F10 editor"),
           "debug HUD exposes QA/dev shortcuts");
}

void hudTextLayoutWrapsObjectiveAndControlsToSafeArea() {
    const std::string longObjective =
        "Cel: bardzo dlugi testowy opis misji z dystansem i konsekwencja ktory musi miescic sie w safe area";
    const bs3d::HudTextLayout layout = bs3d::layoutHudText(longObjective,
                                                           bs3d::buildHudControlHelp(false, false),
                                                           360);
    expect(layout.objectiveLines.size() >= 2, "long objective wraps on narrow screen");
    expect(layout.controlsLines.size() >= 2, "long controls text wraps on narrow screen");
    expect(std::all_of(layout.objectiveLines.begin(), layout.objectiveLines.end(), [](const std::string& line) {
               return line.size() <= 42;
           }),
           "objective lines are clamped to compact HUD width");
}

void hudTextLayoutSplitsLongTokensBeforeTheyLeaveSafeArea() {
    const bs3d::HudTextLayout layout = bs3d::layoutHudText(
        "Cel: SuperHiperDlugieJednoSlowoKtoreNieMaSpacjiINieMozeWyjscPozaHUD",
        "Sterowanie: FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
        320);

    expect(layout.objectiveLines.size() >= 2, "long objective token is split across HUD lines");
    expect(std::all_of(layout.objectiveLines.begin(), layout.objectiveLines.end(), [](const std::string& line) {
               return line.size() <= 34;
           }),
           "split objective token respects narrow safe area");
    expect(std::all_of(layout.controlsLines.begin(), layout.controlsLines.end(), [](const std::string& line) {
               return line.size() <= 40;
           }),
           "split controls token respects narrow safe area");
}

void hudPanelsStayInsideNarrowScreens() {
    const bs3d::HudPanelRect right = bs3d::layoutHudPanel(260, 318, 126, 70, false, 18);
    expect(right.x >= 18, "right HUD panel keeps left safe margin on narrow screens");
    expect(right.x + right.width <= 260 - 18, "right HUD panel clamps inside narrow screen width");
    expect(right.width >= 160, "right HUD panel keeps a readable minimum width");

    const bs3d::HudPanelRect left = bs3d::layoutHudPanel(260, 318, 126, 70, true, 18);
    expect(left.x == 18, "left HUD panel anchors to safe area when vehicle HUD is active");
    expect(left.x + left.width <= 260 - 18, "left HUD panel also clamps inside narrow screen width");
}

void vehicleDriverRenderUsesSeatedPartialBody() {
    const bs3d::Vec3 vehiclePosition{4.0f, 0.0f, 8.0f};
    const bs3d::Vec3 seat = bs3d::vehicleDriverSeatPosition(vehiclePosition, 0.0f);
    expect(seat.y < vehiclePosition.y, "driver render anchor lowers seated pose into cabin");
    expect(bs3d::distanceXZ(seat, vehiclePosition) < 0.35f, "driver render anchor stays inside vehicle cabin footprint");

    const bs3d::CharacterRenderOptions seatedOptions{true};
    expect(bs3d::shouldRenderCharacterPart(bs3d::CharacterPartRole::Head, seatedOptions),
           "seated driver still renders head");
    expect(bs3d::shouldRenderCharacterPart(bs3d::CharacterPartRole::Torso, seatedOptions),
           "seated driver still renders torso");
    expect(!bs3d::shouldRenderCharacterPart(bs3d::CharacterPartRole::LeftLeg, seatedOptions),
           "seated driver hides lower body instead of clipping through car roof/body");
    expect(!bs3d::shouldRenderCharacterPart(bs3d::CharacterPartRole::RightFoot, seatedOptions),
           "seated driver hides feet inside vehicle");
}

void worldRenderListCullsDistantOpaqueAndGlassObjects() {
    bs3d::WorldAssetRegistry registry;
    registry.addDefinition({"block13_core", "models/unit_box.obj", {1.0f, 1.0f, 1.0f}, {255, 255, 255, 255}});
    registry.addDefinition({"shop_window", "models/unit_box.obj", {1.0f, 1.0f, 1.0f}, {103, 145, 156, 118}});

    std::vector<bs3d::WorldObject> objects;
    bs3d::WorldObject nearOpaque;
    nearOpaque.id = "near_wall";
    nearOpaque.assetId = "block13_core";
    nearOpaque.position = {3.0f, 0.0f, 0.0f};
    nearOpaque.scale = {1.0f, 1.0f, 1.0f};
    objects.push_back(nearOpaque);

    bs3d::WorldObject farOpaque = nearOpaque;
    farOpaque.id = "far_wall";
    farOpaque.position = {200.0f, 0.0f, 0.0f};
    objects.push_back(farOpaque);

    bs3d::WorldObject nearGlass = nearOpaque;
    nearGlass.id = "near_glass";
    nearGlass.assetId = "shop_window";
    nearGlass.position = {2.0f, 0.0f, 8.0f};
    nearGlass.gameplayTags.push_back("glass_surface");
    objects.push_back(nearGlass);

    bs3d::WorldObject farGlass = nearGlass;
    farGlass.id = "far_glass";
    farGlass.position = {-180.0f, 0.0f, 0.0f};
    objects.push_back(farGlass);

    const bs3d::WorldRenderList list =
        bs3d::buildWorldRenderList(objects, registry, {0.0f, 2.0f, 0.0f}, 80.0f);

    expect(list.opaque.size() == 1, "render list culls distant opaque objects");
    expect(list.translucent.empty(), "render list keeps opaque pass free of non-alpha world dressing");
    expect(list.glass.size() == 1, "render list culls distant glass before sorting");
    expect(list.transparent.size() == 1, "render list exposes a unified transparent draw order");
    expect(list.culled == 2, "render list reports culled object count for diagnostics");
    expect(list.opaque[0]->id == "near_wall", "render list keeps near opaque object");
    expect(list.glass[0]->id == "near_glass", "render list keeps near glass object");
    expect(list.transparent[0]->id == "near_glass", "unified transparent list keeps near glass object");
}

void worldRenderListSeparatesTranslucentDressingFromOpaquePass() {
    bs3d::WorldAssetRegistry registry;
    registry.addDefinition({"block13_core", "models/unit_box.obj", {1.0f, 1.0f, 1.0f}, {255, 255, 255, 255}});
    registry.addDefinition({"irregular_asphalt_patch", "models/unit_box.obj", {1.0f, 1.0f, 1.0f}, {54, 56, 52, 255}});
    registry.addDefinition({"shop_window", "models/unit_box.obj", {1.0f, 1.0f, 1.0f}, {103, 145, 156, 118}});

    std::vector<bs3d::WorldObject> objects;

    bs3d::WorldObject wall;
    wall.id = "near_wall";
    wall.assetId = "block13_core";
    wall.position = {0.0f, 0.0f, 0.0f};
    objects.push_back(wall);

    bs3d::WorldObject groundPatch = wall;
    groundPatch.id = "asphalt_wear";
    groundPatch.assetId = "irregular_asphalt_patch";
    groundPatch.position = {3.0f, 0.07f, 0.0f};
    groundPatch.hasTintOverride = true;
    groundPatch.tintOverride = {54, 56, 52, 190};
    groundPatch.gameplayTags.push_back("ground_patch");
    objects.push_back(groundPatch);

    bs3d::WorldObject window = wall;
    window.id = "shop_window";
    window.assetId = "shop_window";
    window.position = {6.0f, 0.0f, 0.0f};
    window.gameplayTags.push_back("glass_surface");
    objects.push_back(window);

    const bs3d::WorldRenderList list =
        bs3d::buildWorldRenderList(objects, registry, {0.0f, 2.0f, -2.0f}, 80.0f);

    expect(list.opaque.size() == 1, "alpha-tinted dressing is not rendered as opaque geometry");
    expect(list.translucent.size() == 1, "alpha-tinted dressing is routed through translucent pass");
    expect(list.glass.size() == 1, "glass keeps its dedicated transparent pass");
    expect(list.transparent.size() == 2, "world transparent pass combines glass and alpha dressing");
    expect(list.translucent[0]->id == "asphalt_wear", "ground wear patch remains renderable after pass split");
}

void worldRenderListSortsAllTransparentWorldObjectsTogether() {
    bs3d::WorldAssetRegistry registry;
    registry.addDefinition({"irregular_asphalt_patch", "models/unit_box.obj", {1.0f, 1.0f, 1.0f}, {54, 56, 52, 128}});
    registry.addDefinition({"shop_window", "models/unit_box.obj", {1.0f, 1.0f, 1.0f}, {103, 145, 156, 118}});

    std::vector<bs3d::WorldObject> objects;
    bs3d::WorldObject nearPatch;
    nearPatch.id = "near_patch";
    nearPatch.assetId = "irregular_asphalt_patch";
    nearPatch.position = {0.0f, 0.0f, 2.0f};
    objects.push_back(nearPatch);

    bs3d::WorldObject middleGlass;
    middleGlass.id = "middle_glass";
    middleGlass.assetId = "shop_window";
    middleGlass.position = {0.0f, 0.0f, 6.0f};
    middleGlass.gameplayTags.push_back("glass_surface");
    objects.push_back(middleGlass);

    bs3d::WorldObject farPatch = nearPatch;
    farPatch.id = "far_patch";
    farPatch.position = {0.0f, 0.0f, 10.0f};
    objects.push_back(farPatch);

    const bs3d::WorldRenderList list =
        bs3d::buildWorldRenderList(objects, registry, {0.0f, 1.0f, 0.0f}, 80.0f);

    expect(list.translucent.size() == 2, "test setup has two alpha/decal objects");
    expect(list.glass.size() == 1, "test setup has one glass object");
    expect(list.transparent.size() == 3, "unified transparent list includes glass and decals");
    expect(list.transparent[0]->id == "far_patch" &&
               list.transparent[1]->id == "middle_glass" &&
               list.transparent[2]->id == "near_patch",
           "all world transparent objects are sorted back-to-front together");
}

void worldRenderListUsesManifestAlphaWithoutTintOverride() {
    bs3d::WorldAssetRegistry registry;
    registry.addDefinition({"block13_core", "models/unit_box.obj", {1.0f, 1.0f, 1.0f}, {255, 255, 255, 255}});
    registry.addDefinition({"oil_stain", "models/unit_box.obj", {1.2f, 0.03f, 0.8f}, {38, 39, 36, 145}});

    std::vector<bs3d::WorldObject> objects;
    bs3d::WorldObject wall;
    wall.id = "wall";
    wall.assetId = "block13_core";
    objects.push_back(wall);

    bs3d::WorldObject stain;
    stain.id = "oil";
    stain.assetId = "oil_stain";
    stain.position = {2.0f, 0.02f, 0.0f};
    objects.push_back(stain);

    const bs3d::WorldRenderList list =
        bs3d::buildWorldRenderList(objects, registry, {0.0f, 2.0f, -2.0f}, 80.0f);

    expect(list.opaque.size() == 1, "manifest-alpha dressing does not pollute opaque world pass");
    expect(list.translucent.size() == 1, "fallbackColor alpha routes dressing to translucent pass without tint override");
    expect(list.translucent[0]->id == "oil", "manifest-alpha object remains renderable as translucent dressing");
}

void worldRenderListUsesAssetFallbackBoundsForCulling() {
    bs3d::WorldAssetRegistry registry;
    registry.addDefinition({"wide_wall", "models/unit_box.obj", {20.0f, 2.0f, 2.0f}, {255, 255, 255, 255}});

    bs3d::WorldObject wall;
    wall.id = "wide_edge_wall";
    wall.assetId = "wide_wall";
    wall.position = {84.0f, 0.0f, 0.0f};
    wall.scale = {1.0f, 1.0f, 1.0f};

    const std::vector<bs3d::WorldObject> objects{wall};
    const bs3d::WorldRenderList list =
        bs3d::buildWorldRenderList(objects, registry, {0.0f, 2.0f, 0.0f}, 80.0f);

    expect(list.opaque.size() == 1, "large asset fallback bounds keep edge-visible objects from being culled");
    expect(list.culled == 0, "bounds-aware culling does not discard large visible fallback assets");
}

void memoryHotspotDebugMarkersPreserveRewirIdentityAndScale() {
    bs3d::WorldEventHotspot parking;
    parking.location = bs3d::WorldLocationTag::Parking;
    parking.position = {-7.0f, 0.0f, 8.6f};
    parking.score = 1.0f;
    parking.source = "parking_bump";

    bs3d::WorldEventHotspot road = parking;
    road.location = bs3d::WorldLocationTag::RoadLoop;
    road.position = {0.0f, 0.0f, 0.0f};
    road.score = 6.0f;
    road.source = "road_loop_bump";

    const std::vector<bs3d::MemoryHotspotDebugMarker> markers =
        bs3d::buildMemoryHotspotDebugMarkers({parking, road});

    expect(markers.size() == 2, "hotspot marker builder keeps one marker per hotspot");
    expect(markers[0].location == bs3d::WorldLocationTag::Parking, "parking marker preserves rewir tag");
    expect(markers[0].source == "parking_bump", "parking marker preserves memory source");
    expectNear(markers[0].position.x, -7.0f, 0.001f, "parking marker preserves x position");
    expect(markers[0].radius >= 0.36f, "quiet hotspot still gets a readable marker radius");
    expect(markers[1].location == bs3d::WorldLocationTag::RoadLoop, "road marker preserves rewir tag");
    expect(markers[1].radius > markers[0].radius, "stronger hotspot gets a larger marker radius");
    expect(markers[0].color.r != markers[1].color.r || markers[0].color.g != markers[1].color.g ||
               markers[0].color.b != markers[1].color.b,
           "different rewirs get distinguishable marker colors");
}

void rewirPressureDebugMarkersExposePatrolInterestRadius() {
    bs3d::RewirPressureSnapshot pressure;
    pressure.active = true;
    pressure.patrolInterest = true;
    pressure.level = bs3d::RewirPressureLevel::Watchful;
    pressure.location = bs3d::WorldLocationTag::Parking;
    pressure.position = {-7.0f, 0.0f, 8.6f};
    pressure.score = 1.4f;
    pressure.source = "parking_pressure_memory";

    const std::vector<bs3d::RewirPressureDebugMarker> markers =
        bs3d::buildRewirPressureDebugMarkers(pressure);

    expect(markers.size() == 1, "active patrol pressure produces one QA marker");
    expect(markers[0].level == bs3d::RewirPressureLevel::Watchful, "pressure marker preserves level");
    expect(markers[0].source == "parking_pressure_memory", "pressure marker preserves source");
    expectNear(markers[0].position.x, -7.0f, 0.001f, "pressure marker preserves x position");
    expectNear(markers[0].position.z, 8.6f, 0.001f, "pressure marker preserves z position");
    expectNear(markers[0].radius, 4.4f, 0.001f, "pressure marker radius matches patrol orbit radius");
    expect(markers[0].color.a > 170, "pressure marker is more assertive than passive memory markers");

    pressure.patrolInterest = false;
    expect(bs3d::buildRewirPressureDebugMarkers(pressure).empty(),
           "pressure without patrol interest does not produce a patrol QA marker");
}

void modelLoadWarningsAreFailFastInDevQualityGate() {
    bs3d::WorldModelLoadResult result;
    result.requested = 3;
    result.loaded = 2;
    result.warnings.push_back("asset failed to load meshes: broken_model");

    const bs3d::ModelLoadQualityGate gate = bs3d::evaluateModelLoadQuality(result, true);
    expect(!gate.ok, "dev/CI model quality gate fails on load warnings");
    expect(gate.message.find("broken_model") != std::string::npos,
           "model quality gate includes actionable failing asset");

    const bs3d::ModelLoadQualityGate releaseGate = bs3d::evaluateModelLoadQuality(result, false);
    expect(releaseGate.ok, "release can allow explicit fallback rendering for model load warnings");
}

void propSimulationSyncUsesIndexedLookupForLargeWorld() {
    std::vector<bs3d::WorldObject> objects;
    objects.reserve(420);
    for (int i = 0; i < 420; ++i) {
        bs3d::WorldObject object;
        object.id = "prop_" + std::to_string(i);
        object.collision.kind = bs3d::WorldCollisionShapeKind::Box;
        object.collision.size = {0.5f, 0.5f, 0.5f};
        object.gameplayTags.push_back("dynamic_prop");
        objects.push_back(object);
    }

    bs3d::PropSimulationSystem props;
    props.addPropsFromWorld(objects);
    props.applyImpulseNear({0.0f, 0.0f, 0.0f}, 999.0f, {1.0f, 0.0f, 0.0f});
    props.update(1.0f / 60.0f);
    const bs3d::PropSyncStats stats = props.syncWorldObjects(objects);

    expect(stats.objectsVisited == objects.size(), "sync visits each world object once");
    expect(stats.indexedLookups == objects.size(), "sync uses one indexed lookup per object");
    expect(stats.linearFindFallbacks == 0, "large world sync does not use per-object string find fallback");
}

void chaseAiUsesSensorBlackboardPatrolSearchAndRecoverStates() {
    bs3d::ChaseAiRuntimeState state;
    bs3d::ChaseAiInput input;
    input.pursuerPosition = {0.0f, 0.0f, 0.0f};
    input.targetPosition = {12.0f, 0.0f, 0.0f};
    input.deltaSeconds = 0.1f;
    input.lineOfSightBlocked = true;
    input.witnessedBySensor = false;

    bs3d::ChaseAiCommand command = bs3d::updateChaseAiRuntime(state, input);
    expect(command.mode == bs3d::ChaseAiMode::Patrol, "police AI patrols before perception has target memory");
    expect(!command.lineOfSight, "blocked LOS is reflected in command");

    input.witnessedBySensor = true;
    input.lineOfSightBlocked = false;
    command = bs3d::updateChaseAiRuntime(state, input);
    expect(command.mode == bs3d::ChaseAiMode::Pursue, "sensor acquisition moves AI into chase state");
    expect(state.blackboard.hasKnownTarget, "blackboard stores last known target");

    input.witnessedBySensor = false;
    input.lineOfSightBlocked = true;
    for (int i = 0; i < 12; ++i) {
        command = bs3d::updateChaseAiRuntime(state, input);
    }
    expect(command.mode == bs3d::ChaseAiMode::Search, "lost target moves police into search instead of blind pursuit");
    expect(bs3d::distanceXZ(command.aimPoint, state.blackboard.lastKnownTarget) < 6.0f,
           "search aims around the remembered target position");
}

void chaseAiPatrolUsesRewirPressureInterestWhenTargetIsUnknown() {
    bs3d::ChaseAiRuntimeState state;
    bs3d::ChaseAiInput input;
    input.pursuerPosition = {0.0f, 0.0f, 0.0f};
    input.targetPosition = {24.0f, 0.0f, 0.0f};
    input.deltaSeconds = 0.1f;
    input.elapsedSeconds = 2.0f;
    input.lineOfSightBlocked = true;
    input.witnessedBySensor = false;
    input.hasPatrolInterest = true;
    input.patrolInterestPosition = {-7.0f, 0.0f, 8.6f};
    input.patrolInterestRadius = 4.0f;

    const bs3d::ChaseAiCommand command = bs3d::updateChaseAiRuntime(state, input);

    expect(command.mode == bs3d::ChaseAiMode::Patrol, "unknown target keeps AI in patrol mode");
    expect(bs3d::distanceXZ(command.aimPoint, input.patrolInterestPosition) <= input.patrolInterestRadius + 0.2f,
           "rewir pressure patrols around the hot spot instead of the patrol car");
    expect(bs3d::distanceXZ(command.aimPoint, input.pursuerPosition) > 6.0f,
           "rewir patrol interest moves the aim point away from idle local circling");
}

void introLevelTextDoesNotContainEncodingArtifacts() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    const auto readable = [](const std::string& text) {
        return !textContains(text, "Äą") && !textContains(text, "Ă…") &&
               !textContains(text, "Ă") && !textContains(text, "ďż˝");
    };
    for (const bs3d::WorldLandmark& landmark : level.landmarks) {
        expect(readable(landmark.label), "landmark label is valid UTF-8/ASCII: " + landmark.id);
    }
    for (const bs3d::WorldViewpoint& viewpoint : level.visualBaselines) {
        expect(readable(viewpoint.label), "viewpoint label is valid UTF-8/ASCII: " + viewpoint.id);
    }
}

void vehicleArtModelSpecMatchesRuntimeScaleAndWheelContract() {
    const bs3d::VehicleArtModelSpec& spec = bs3d::vehicleArtModelSpec();
    expect(spec.assetId == "vehicle_gruz_e36", "vehicle art spec has stable asset id");
    expect(spec.modelPath == "models/gruz_e36.gltf", "vehicle art spec points at shipped glTF");
    expect(spec.bounds.x >= 1.75f && spec.bounds.x <= 2.05f, "vehicle art width matches gameplay collision scale");
    expect(spec.bounds.z >= 3.20f && spec.bounds.z <= 3.75f, "vehicle art length matches gameplay collision scale");

    const std::vector<std::string> requiredParts{
        "Car_Body", "Hood", "Roof", "Trunk", "Front_Bumper", "Rear_Bumper",
        "Fender_FL", "Fender_FR", "Fender_RL", "Fender_RR", "Door_L", "Door_R",
        "Windshield", "Rear_Window", "Window_L", "Window_R",
        "Pillar_A_L", "Pillar_A_R", "Pillar_B_L", "Pillar_B_R", "Pillar_C_L", "Pillar_C_R",
        "Headlight_L", "Headlight_R", "Taillight_L", "Taillight_R",
        "Grille", "Mirror_L", "Mirror_R", "Exhaust",
        "Wheel_FL", "Wheel_FR", "Wheel_RL", "Wheel_RR",
        "Rim_FL", "Rim_FR", "Rim_RL", "Rim_RR",
        "Tire_FL", "Tire_FR", "Tire_RL", "Tire_RR",
        "Collider_Body", "Collider_Wheels"};

    for (const std::string& partName : requiredParts) {
        expect(bs3d::findVehicleArtPart(spec, partName) != nullptr,
               "vehicle art spec contains required part: " + partName);
    }

    const bs3d::VehicleArtPart* frontLeft = bs3d::findVehicleArtPart(spec, "Wheel_FL");
    const bs3d::VehicleArtPart* frontRight = bs3d::findVehicleArtPart(spec, "Wheel_FR");
    const bs3d::VehicleArtPart* rearLeft = bs3d::findVehicleArtPart(spec, "Wheel_RL");
    const bs3d::VehicleArtPart* tireLeft = bs3d::findVehicleArtPart(spec, "Tire_FL");
    const bs3d::VehicleArtPart* windshield = bs3d::findVehicleArtPart(spec, "Windshield");
    const bs3d::VehicleArtPart* rearWindow = bs3d::findVehicleArtPart(spec, "Rear_Window");
    const bs3d::VehicleArtPart* roof = bs3d::findVehicleArtPart(spec, "Roof");
    expect(frontLeft != nullptr && frontRight != nullptr && rearLeft != nullptr && tireLeft != nullptr,
           "wheel pivot parts are findable");
    expect(windshield != nullptr && rearWindow != nullptr && roof != nullptr,
           "glass and roof parts are findable for runtime silhouette checks");
    expect(frontLeft->frontWheel && frontRight->frontWheel && !rearLeft->frontWheel,
           "front wheels are marked steerable while rear wheels are not");
    expect(frontLeft->wheelPivot && frontRight->wheelPivot,
           "wheel nodes are authored as axle-center pivots");
    expectNear(std::fabs(frontLeft->position.x), std::fabs(frontRight->position.x), 0.001f,
               "left/right wheel pivots are mirrored across vehicle center");
    expectNear(frontLeft->position.z, tireLeft->position.z, 0.001f,
               "tire mesh shares wheel pivot z for clean spin");
    expectNear(frontLeft->position.y, tireLeft->position.y, 0.001f,
               "tire mesh shares wheel pivot y for clean spin");
    if (windshield->shape == bs3d::VehicleArtShape::QuadPanel) {
        const float windshieldTopZ = (windshield->panelCorners[0].z + windshield->panelCorners[1].z) * 0.5f;
        const float windshieldBottomZ = (windshield->panelCorners[2].z + windshield->panelCorners[3].z) * 0.5f;
        expect(windshieldBottomZ - windshieldTopZ >= 0.35f,
               "windshield panel is sloped through authored corners instead of a flat tabletop slab");
    } else {
        expect(std::fabs(windshield->pitchRadians) >= 0.35f,
               "windshield is sloped instead of a flat tabletop slab");
    }
    if (rearWindow->shape == bs3d::VehicleArtShape::QuadPanel) {
        const float rearTopZ = (rearWindow->panelCorners[0].z + rearWindow->panelCorners[1].z) * 0.5f;
        const float rearBottomZ = (rearWindow->panelCorners[2].z + rearWindow->panelCorners[3].z) * 0.5f;
        expect(rearTopZ - rearBottomZ >= 0.25f,
               "rear window panel is sloped through authored corners instead of a flat tabletop slab");
    } else {
        expect(std::fabs(rearWindow->pitchRadians) >= 0.30f,
               "rear window is sloped instead of a flat tabletop slab");
    }
    expect(roof->position.y <= 1.22f,
           "roof sits low enough to read as a coupe cabin, not a stacked box");
    expect(tireLeft->size.y <= 0.62f && tireLeft->size.z <= 0.62f,
           "tires stay chunky but below monster-truck scale for the body");
}

void vehicleCabinUsesPanelGlassTechnology() {
    const bs3d::VehicleArtModelSpec& spec = bs3d::vehicleArtModelSpec();
    const std::vector<std::string> panelGlassNames{
        "Windshield", "Rear_Window", "Window_L", "Window_R"};
    for (const std::string& partName : panelGlassNames) {
        const bs3d::VehicleArtPart* glass = bs3d::findVehicleArtPart(spec, partName);
        expect(glass != nullptr, "panel glass part is findable: " + partName);
        expect(glass->material == bs3d::VehicleArtMaterial::Glass,
               "panel glass keeps glass material: " + partName);
        expect(glass->shape == bs3d::VehicleArtShape::QuadPanel,
               "cabin glass uses authored quad panel instead of box slab: " + partName);

        const float topY = (glass->panelCorners[0].y + glass->panelCorners[1].y) * 0.5f;
        const float bottomY = (glass->panelCorners[2].y + glass->panelCorners[3].y) * 0.5f;
        expect(topY > bottomY,
               "panel glass corners are ordered with a real top and bottom edge: " + partName);
    }

    const std::vector<std::string> frameNames{
        "Roof_Rail_L", "Roof_Rail_R", "Windshield_Header", "Rear_Window_Header"};
    for (const std::string& partName : frameNames) {
        const bs3d::VehicleArtPart* frame = bs3d::findVehicleArtPart(spec, partName);
        expect(frame != nullptr, "cabin frame connector exists: " + partName);
        expect(frame->material == bs3d::VehicleArtMaterial::DarkTrim ||
                   frame->material == bs3d::VehicleArtMaterial::BodyPaint,
               "cabin frame connector is opaque structural material: " + partName);
    }
}

void vehicleCabinHasCoherentRoofGlassAndPillars() {
    const bs3d::VehicleArtModelSpec& spec = bs3d::vehicleArtModelSpec();
    const bs3d::VehicleArtPart* body = bs3d::findVehicleArtPart(spec, "Car_Body");
    const bs3d::VehicleArtPart* hood = bs3d::findVehicleArtPart(spec, "Hood");
    const bs3d::VehicleArtPart* roof = bs3d::findVehicleArtPart(spec, "Roof");
    const bs3d::VehicleArtPart* windshield = bs3d::findVehicleArtPart(spec, "Windshield");
    const bs3d::VehicleArtPart* rearWindow = bs3d::findVehicleArtPart(spec, "Rear_Window");
    const bs3d::VehicleArtPart* windowLeft = bs3d::findVehicleArtPart(spec, "Window_L");
    const bs3d::VehicleArtPart* windowRight = bs3d::findVehicleArtPart(spec, "Window_R");
    expect(body != nullptr && hood != nullptr && roof != nullptr && windshield != nullptr && rearWindow != nullptr,
           "body, hood, roof and main glass are findable for cabin coherence checks");
    expect(windowLeft != nullptr && windowRight != nullptr,
           "side windows are findable for cabin coherence checks");

    const std::vector<std::string> pillarNames{
        "Pillar_A_L", "Pillar_A_R",
        "Pillar_B_L", "Pillar_B_R",
        "Pillar_C_L", "Pillar_C_R"};
    for (const std::string& pillarName : pillarNames) {
        const bs3d::VehicleArtPart* pillar = bs3d::findVehicleArtPart(spec, pillarName);
        expect(pillar != nullptr, "vehicle cabin has visible support pillar: " + pillarName);
        expect(pillar->material == bs3d::VehicleArtMaterial::DarkTrim ||
                   pillar->material == bs3d::VehicleArtMaterial::BodyPaint,
               "vehicle cabin pillar uses opaque structural material: " + pillarName);
    }

    expect(roof->position.y > windshield->position.y && roof->position.y > rearWindow->position.y,
           "roof sits above windshield and rear glass instead of intersecting them as a flat slab");
    expect(roof->position.z < windshield->position.z && roof->position.z > rearWindow->position.z,
           "roof sits between front and rear glass along the cabin");
    expect(roof->size.x < body->size.x * 0.72f,
           "roof is narrower than the body so the cabin reads as a real upper structure");
    expect(roof->size.z >= 0.66f && roof->size.z <= 0.88f,
           "roof length stays compact and coupe-like instead of becoming a tabletop");
    expect(windshield->position.z > hood->position.z - hood->size.z * 0.55f,
           "windshield visually grows out of the rear of the hood");
    const float windshieldSlopeDepth = windshield->shape == bs3d::VehicleArtShape::QuadPanel
        ? std::fabs(((windshield->panelCorners[2].z + windshield->panelCorners[3].z) * 0.5f) -
                    ((windshield->panelCorners[0].z + windshield->panelCorners[1].z) * 0.5f))
        : std::fabs(windshield->pitchRadians);
    const float rearSlopeDepth = rearWindow->shape == bs3d::VehicleArtShape::QuadPanel
        ? std::fabs(((rearWindow->panelCorners[2].z + rearWindow->panelCorners[3].z) * 0.5f) -
                    ((rearWindow->panelCorners[0].z + rearWindow->panelCorners[1].z) * 0.5f))
        : std::fabs(rearWindow->pitchRadians);
    expect(windshieldSlopeDepth > rearSlopeDepth * 0.85f,
           "front and rear glass slopes are comparable enough to form one cabin silhouette");
    expect(std::fabs(windowLeft->position.x) <= body->size.x * 0.52f &&
               std::fabs(windowRight->position.x) <= body->size.x * 0.52f,
           "side windows sit inside the body width instead of floating outside the doors");
    expect(windowLeft->size.z >= 0.90f && windowRight->size.z >= 0.90f,
           "side windows span the cabin from A-pillar toward C-pillar");
    expect(windowLeft->size.y >= 0.32f && windowRight->size.y >= 0.32f,
           "side glass has enough height to read from gameplay camera");
}

void vehicleLightVisualStateCommunicatesGruzUseAndWear() {
    bs3d::VehicleRuntimeState idle;
    idle.condition = 100.0f;
    idle.rpmNormalized = 0.18f;

    const bs3d::VehicleLightVisualState idleLights = bs3d::vehicleLightVisualState(idle);
    expect(idleLights.headlightColor.a == 255 && idleLights.taillightColor.a == 255,
           "vehicle light visuals stay opaque in the opaque vehicle pass");
    expect(idleLights.headlightGlow >= 0.18f && idleLights.headlightGlow <= 0.55f,
           "idle gruz headlights are visible without looking like high beams");
    expect(idleLights.taillightGlow >= 0.18f && idleLights.taillightGlow <= 0.55f,
           "idle gruz taillights are visible from gameplay camera");

    bs3d::VehicleRuntimeState boosted = idle;
    boosted.rpmNormalized = 0.92f;
    boosted.boostActive = true;
    const bs3d::VehicleLightVisualState boostedLights = bs3d::vehicleLightVisualState(boosted);
    expect(boostedLights.headlightGlow > idleLights.headlightGlow + 0.12f,
           "boost/rpm makes the front lights read hotter");

    bs3d::VehicleRuntimeState reversing = idle;
    reversing.speed = -3.0f;
    const bs3d::VehicleLightVisualState reverseLights = bs3d::vehicleLightVisualState(reversing);
    expect(reverseLights.taillightGlow > idleLights.taillightGlow + 0.20f,
           "reverse gear makes rear lights clearly hotter");

    bs3d::VehicleRuntimeState damaged = boosted;
    damaged.condition = 8.0f;
    const bs3d::VehicleLightVisualState damagedLights = bs3d::vehicleLightVisualState(damaged);
    expect(damagedLights.headlightGlow < boostedLights.headlightGlow,
           "heavy damage dims the light presentation");
    expect(damagedLights.headlightGlow >= 0.16f && damagedLights.taillightGlow >= 0.16f,
           "damaged gruz keeps enough light to remain readable");
    expect(colorDistanceSquared(damagedLights.headlightColor, idleLights.headlightColor) > 16,
           "damage changes the actual rendered headlight tint, not only a debug number");
}

void generatedVehicleGltfContainsNamedPartsMaterialsAndColliders() {
    const std::string gltf = readTextFile("data/assets/models/gruz_e36.gltf");
    expect(!gltf.empty(), "generated gruz e36 glTF exists and is readable");

    const std::vector<std::string> requiredNames{
        "Car_Body", "Hood", "Roof", "Trunk", "Front_Bumper", "Rear_Bumper",
        "Fender_FL", "Fender_FR", "Fender_RL", "Fender_RR", "Door_L", "Door_R",
        "Windshield", "Rear_Window", "Window_L", "Window_R",
        "Pillar_A_L", "Pillar_A_R", "Pillar_B_L", "Pillar_B_R", "Pillar_C_L", "Pillar_C_R",
        "Roof_Rail_L", "Roof_Rail_R", "Windshield_Header", "Rear_Window_Header",
        "Headlight_L", "Headlight_R", "Taillight_L", "Taillight_R",
        "Grille", "Mirror_L", "Mirror_R", "Exhaust",
        "Wheel_FL", "Wheel_FR", "Wheel_RL", "Wheel_RR",
        "Rim_FL", "Rim_FR", "Rim_RL", "Rim_RR",
        "Tire_FL", "Tire_FR", "Tire_RL", "Tire_RR",
        "paint_dirty_red", "glass_translucent", "headlight_emissive",
        "taillight_emissive", "rubber_dark", "rim_dull_alloy"};

    for (const std::string& name : requiredNames) {
        expect(textContains(gltf, "\"" + name + "\""), "vehicle glTF contains name/material: " + name);
    }

    expect(textContains(gltf, "\"alphaMode\": \"BLEND\""), "vehicle glTF marks glass/collider transparency as blend");
    expect(textContains(gltf, "\"emissiveFactor\""), "vehicle glTF includes emissive light materials");
    expect(textContains(gltf, "\"axle_center\""), "vehicle glTF annotates wheel pivots at axle centers");
    expect(textContains(gltf, "\"panelQuad\": true"), "vehicle glTF exports authored cabin panels instead of only boxes");
    expect(textContains(gltf, "\"rotation\""), "vehicle glTF exports sloped glass/hood rotations instead of only flat slabs");
    expect(textContains(gltf, "\"NORMAL\""), "vehicle glTF exports normals for runtime lighting");
    expect(!textContains(gltf, "\"collisionProxy\""), "vehicle render glTF excludes collision proxy nodes");
    expect(!textContains(gltf, "\"Collider_Body\"") && !textContains(gltf, "\"Collider_Wheels\""),
           "vehicle render glTF keeps collision shapes in runtime metadata instead of render meshes");
}

void vehicleArtModelV2CommunicatesDirtyRustyGruz() {
    const bs3d::VehicleArtModelSpec& spec = bs3d::vehicleArtModelSpec();

    const std::vector<std::string> requiredWearParts{
        "Dirt_Rocker_L",
        "Dirt_Rocker_R",
        "Rust_Fender_FL",
        "Rust_Fender_RR",
        "Rust_Door_L",
        "Primer_Door_R",
        "Hood_Chipped_Edge",
        "Trunk_Dust_Strip",
        "Bumper_Scuff_Front",
        "Bumper_Scuff_Rear"};

    int dirtyParts = 0;
    int rustParts = 0;
    int paintMismatchParts = 0;
    for (const std::string& partName : requiredWearParts) {
        const bs3d::VehicleArtPart* part = bs3d::findVehicleArtPart(spec, partName);
        expect(part != nullptr, "gruz v2 has authored wear part: " + partName);
        if (part == nullptr) {
            continue;
        }
        dirtyParts += partName.find("Dirt_") != std::string::npos ||
                              partName.find("Dust_") != std::string::npos ||
                              partName.find("Scuff_") != std::string::npos ? 1 : 0;
        rustParts += partName.find("Rust_") != std::string::npos ? 1 : 0;
        paintMismatchParts += partName.find("Primer_") != std::string::npos ? 1 : 0;
        expect(part->render, "gruz wear detail is renderable: " + partName);
    }

    expect(dirtyParts >= 4, "gruz v2 has enough dirt/scuff overlays to stop reading as a clean toy car");
    expect(rustParts >= 3, "gruz v2 has visible rust around vulnerable panels");
    expect(paintMismatchParts >= 1, "gruz v2 has at least one mismatched/primer panel");

    const bs3d::VehicleArtPart* leftRocker = bs3d::findVehicleArtPart(spec, "Dirt_Rocker_L");
    const bs3d::VehicleArtPart* leftDoor = bs3d::findVehicleArtPart(spec, "Door_L");
    expect(leftRocker != nullptr && leftDoor != nullptr, "gruz v2 rocker dirt and door are findable");
    if (leftRocker != nullptr && leftDoor != nullptr) {
        expect(leftRocker->position.y < leftDoor->position.y,
               "rocker dirt sits low where road grime belongs");
        expect(leftRocker->size.z >= leftDoor->size.z * 0.80f,
               "rocker dirt runs along the side instead of being a tiny debug cube");
    }
}

void generatedVehicleGltfContainsGruzV2WearMaterials() {
    const std::string gltf = readTextFile("data/assets/models/gruz_e36.gltf");
    expect(!gltf.empty(), "generated gruz e36 glTF exists and is readable for v2 wear checks");

    const std::vector<std::string> requiredNames{
        "rust_oxidized",
        "road_dirt_matte",
        "primer_mismatched_gray",
        "Dirt_Rocker_L",
        "Dirt_Rocker_R",
        "Rust_Fender_FL",
        "Rust_Fender_RR",
        "Primer_Door_R",
        "Bumper_Scuff_Front"};

    for (const std::string& name : requiredNames) {
        expect(textContains(gltf, "\"" + name + "\""), "vehicle glTF contains v2 wear name/material: " + name);
    }
}

void glassRenderPolicySeparatesTransparentWorldAndVehicleGlass() {
    const bs3d::VehicleArtModelSpec& spec = bs3d::vehicleArtModelSpec();
    expect(bs3d::isVehicleGlassPart(*bs3d::findVehicleArtPart(spec, "Windshield")),
           "windshield is recognized by the glass render policy");
    expect(bs3d::isVehicleGlassPart(*bs3d::findVehicleArtPart(spec, "Rear_Window")),
           "rear window is recognized by the glass render policy");
    expect(!bs3d::isVehicleGlassPart(*bs3d::findVehicleArtPart(spec, "Roof")),
           "opaque roof is not treated as transparent glass");

    bs3d::WorldObject shopWindow;
    shopWindow.assetId = "shop_window";
    expect(bs3d::isWorldGlassObject(shopWindow), "shop window object is rendered by the transparent glass pass");

    bs3d::WorldObject sign;
    sign.assetId = "shop_sign_zenona";
    expect(!bs3d::isWorldGlassObject(sign), "shop sign stays in the opaque pass");

    const bs3d::GlassVisualState daytime = bs3d::worldGlassVisualState(shopWindow, 0.0f);
    const bs3d::GlassVisualState night = bs3d::worldGlassVisualState(shopWindow, 1.0f);
    expect(daytime.renderAfterOpaque && !daytime.blocksUi, "glass is a world pass and never a UI blocker");
    expect(daytime.tint.a > 60 && daytime.tint.a < 190, "day glass keeps controlled transparency");
    expect(night.reflectionIntensity > daytime.reflectionIntensity, "night glass carries stronger stylized reflection");
}

void glassRenderPolicySortsWorldGlassBackToFront() {
    std::vector<bs3d::WorldObject> objects;
    bs3d::WorldObject nearWindow;
    nearWindow.id = "near";
    nearWindow.assetId = "shop_window";
    nearWindow.position = {0.0f, 0.0f, 2.0f};
    objects.push_back(nearWindow);

    bs3d::WorldObject opaqueBetween;
    opaqueBetween.id = "opaque";
    opaqueBetween.assetId = "bench";
    opaqueBetween.position = {0.0f, 0.0f, 12.0f};
    objects.push_back(opaqueBetween);

    bs3d::WorldObject farWindow;
    farWindow.id = "far";
    farWindow.assetId = "block_window_strip";
    farWindow.position = {0.0f, 0.0f, 9.0f};
    objects.push_back(farWindow);

    const std::vector<const bs3d::WorldObject*> sorted =
        bs3d::sortedGlassObjectsBackToFront(objects, {0.0f, 1.0f, 0.0f});

    expect(sorted.size() == 2, "transparent pass includes only glass objects");
    expect(sorted[0]->id == "far" && sorted[1]->id == "near",
           "transparent world glass is drawn back-to-front to avoid obvious ordering artifacts");
}

void glassCrackPolicyIsOptionalAndImpactDriven() {
    expect(bs3d::glassCrackAmountForImpact(2.0f) == 0.0f, "small taps do not crack glass");
    expect(bs3d::glassCrackAmountForImpact(7.0f) > 0.0f, "medium hit can add hairline cracks");
    expect(bs3d::glassCrackAmountForImpact(13.0f) > bs3d::glassCrackAmountForImpact(7.0f),
           "hard hit increases glass crack amount");

    const bs3d::GlassVisualState healthy = bs3d::vehicleGlassVisualState(100.0f, 0.0f);
    const bs3d::GlassVisualState damaged = bs3d::vehicleGlassVisualState(35.0f, 10.0f);
    expect(healthy.crackAmount == 0.0f, "fresh vehicle glass is clean");
    expect(damaged.crackAmount > healthy.crackAmount, "damaged vehicle glass can show controlled cracks");
}

void characterArtModelMatchesControllerFirstContract() {
    const bs3d::CharacterArtModelSpec& spec = bs3d::characterArtModelSpec();

    expect(spec.visualHeight >= 1.75f && spec.visualHeight <= 1.85f,
           "character visual height stays in the 1.75-1.85m controller-ready range");
    expectNear(spec.capsuleRadius, bs3d::PlayerMotorConfig{}.radius, 0.001f,
               "character art contract matches the gameplay capsule radius");
    expect(spec.parts.size() >= 16, "character model is made from named readable parts");
    expect(findCharacterPart(spec, "Head") != nullptr, "character has a named oversized head");
    expect(findCharacterPart(spec, "Torso_Jacket") != nullptr, "character has a named torso/jacket");
    expect(findCharacterPart(spec, "Back_Hood") != nullptr, "character has a readable rear silhouette part");
    expect(findCharacterPart(spec, "Shoe_L") != nullptr && findCharacterPart(spec, "Shoe_R") != nullptr,
           "character has chunky named shoes for TPP readability");

    float minY = 999.0f;
    float maxY = -999.0f;
    float maxAbsX = 0.0f;
    for (const bs3d::CharacterArtPart& part : spec.parts) {
        if (!part.render) {
            continue;
        }
        const float halfX = part.size.x * 0.5f;
        const float halfY = part.size.y * 0.5f;
        minY = std::min(minY, part.position.y - halfY);
        maxY = std::max(maxY, part.position.y + halfY);
        maxAbsX = std::max(maxAbsX, std::fabs(part.position.x) + halfX);
    }

    expect(minY >= -0.001f, "character origin is on the ground between feet");
    expect(maxY <= spec.visualHeight + 0.03f, "character parts do not exceed authored height");
    expect(maxAbsX < spec.capsuleRadius, "character arms/shoulders stay inside the capsule silhouette");
}

void characterArtModelDefinesAnimationRolesForRuntimePose() {
    const bs3d::CharacterArtModelSpec& spec = bs3d::characterArtModelSpec();

    bool hasLeftArm = false;
    bool hasRightArm = false;
    bool hasLeftLeg = false;
    bool hasRightLeg = false;
    bool hasHead = false;
    bool hasTorso = false;
    for (const bs3d::CharacterArtPart& part : spec.parts) {
        hasLeftArm = hasLeftArm || part.role == bs3d::CharacterPartRole::LeftArm;
        hasRightArm = hasRightArm || part.role == bs3d::CharacterPartRole::RightArm;
        hasLeftLeg = hasLeftLeg || part.role == bs3d::CharacterPartRole::LeftLeg;
        hasRightLeg = hasRightLeg || part.role == bs3d::CharacterPartRole::RightLeg;
        hasHead = hasHead || part.role == bs3d::CharacterPartRole::Head;
        hasTorso = hasTorso || part.role == bs3d::CharacterPartRole::Torso;
    }

    expect(hasLeftArm && hasRightArm, "character model has arm roles for walk/panic/talk animation");
    expect(hasLeftLeg && hasRightLeg, "character model has leg roles for locomotion animation");
    expect(hasHead && hasTorso, "character model has head and torso roles for readable TPP pose");
}

void characterArtModelLeavesSafetyMarginInsideGameplayCapsule() {
    const bs3d::CharacterArtModelSpec& spec = bs3d::characterArtModelSpec();

    float maxAbsX = 0.0f;
    float maxAbsZ = 0.0f;
    for (const bs3d::CharacterArtPart& part : spec.parts) {
        if (!part.render) {
            continue;
        }
        maxAbsX = std::max(maxAbsX, std::fabs(part.position.x) + part.size.x * 0.5f);
        maxAbsZ = std::max(maxAbsZ, std::fabs(part.position.z) + part.size.z * 0.5f);
    }

    expect(maxAbsX <= spec.capsuleRadius - 0.035f,
           "hands/arms leave a visible safety margin inside the gameplay capsule");
    expect(maxAbsZ <= spec.capsuleRadius - 0.035f,
           "front/back silhouette leaves a visible safety margin inside the gameplay capsule");
}

void characterArtModelHasReadableOsiedleSilhouetteLandmarks() {
    const bs3d::CharacterArtModelSpec& spec = bs3d::characterArtModelSpec();

    const bs3d::CharacterArtPart* head = findCharacterPart(spec, "Head");
    const bs3d::CharacterArtPart* torso = findCharacterPart(spec, "Torso_Jacket");
    expect(head != nullptr && torso != nullptr, "character has base head and torso for silhouette checks");
    expect(head->size.x >= torso->size.x * 0.80f,
           "head is intentionally oversized enough for the stylized TPP read");

    expect(findCharacterPart(spec, "Eye_L") != nullptr && findCharacterPart(spec, "Eye_R") != nullptr,
           "face has simple readable eyes instead of a blank skin ball");
    expect(findCharacterPart(spec, "Mouth_Smirk") != nullptr,
           "face has a tired/cwany mouth block readable from gameplay camera");
    expect(findCharacterPart(spec, "Jacket_Zip") != nullptr,
           "front torso has a zipper/centerline to make orientation readable");
    expect(findCharacterPart(spec, "Jacket_Collar_L") != nullptr &&
               findCharacterPart(spec, "Jacket_Collar_R") != nullptr,
           "neck/shoulder area has collar pieces for a less toy-block silhouette");
    expect(findCharacterPart(spec, "Elbow_Patch_L") != nullptr &&
               findCharacterPart(spec, "Elbow_Patch_R") != nullptr,
           "arms have elbow landmarks for punch/talk/interact poses");
    expect(findCharacterPart(spec, "Knee_Patch_L") != nullptr &&
               findCharacterPart(spec, "Knee_Patch_R") != nullptr,
           "legs have knee landmarks for walk/jog/sprint read");
    expect(findCharacterPart(spec, "Shoe_Sole_L") != nullptr &&
               findCharacterPart(spec, "Shoe_Sole_R") != nullptr,
           "chunky shoes have separate soles for running readability");
    expect(findCharacterPart(spec, "Back_Block13_Tag") != nullptr,
           "back silhouette has a small non-IP jacket tag for TPP camera identity");
}

void characterArtModelV2BreaksToyBlockSilhouette() {
    const bs3d::CharacterArtModelSpec& spec = bs3d::characterArtModelSpec();

    const std::vector<std::string> requiredV2Parts{
        "Cap_Brim",
        "Ear_L",
        "Ear_R",
        "Brow_L",
        "Brow_R",
        "Jacket_Hem",
        "Sleeve_Cuff_L",
        "Sleeve_Cuff_R",
        "Pocket_Patch_Front",
        "Trouser_Side_Stripe_L",
        "Trouser_Side_Stripe_R"};

    int accentLandmarks = 0;
    for (const std::string& partName : requiredV2Parts) {
        const bs3d::CharacterArtPart* part = findCharacterPart(spec, partName);
        expect(part != nullptr, "character v2 has osiedle silhouette detail: " + partName);
        if (part != nullptr && (part->material == bs3d::CharacterArtMaterial::Accent ||
                                part->material == bs3d::CharacterArtMaterial::Trim)) {
            ++accentLandmarks;
        }
    }

    const bs3d::CharacterArtPart* brim = findCharacterPart(spec, "Cap_Brim");
    const bs3d::CharacterArtPart* head = findCharacterPart(spec, "Head");
    expect(brim != nullptr && head != nullptr, "character v2 brim and head are findable");
    if (brim != nullptr && head != nullptr) {
        expect(brim->position.z > head->position.z + head->size.z * 0.40f,
               "cap brim pushes the face silhouette forward instead of reading as a cube hat");
        expect(brim->size.y <= 0.08f, "cap brim stays thin and non-blocky");
    }

    expect(accentLandmarks >= 8,
           "character v2 uses small trim/accent landmarks to avoid a single toy-block mass");
}

void characterArtPaletteKeepsCustomizableReadableMaterials() {
    const bs3d::CharacterArtPalette& palette = bs3d::defaultCharacterArtPalette();

    const bs3d::CharacterArtColor jacket =
        bs3d::characterArtColorForMaterial(bs3d::CharacterArtMaterial::Jacket, palette);
    const bs3d::CharacterArtColor pants =
        bs3d::characterArtColorForMaterial(bs3d::CharacterArtMaterial::Pants, palette);
    const bs3d::CharacterArtColor skin =
        bs3d::characterArtColorForMaterial(bs3d::CharacterArtMaterial::Skin, palette);

    expect(jacket.a == 255 && pants.a == 255 && skin.a == 255,
           "default character palette is opaque and runtime-renderable");
    expect(jacket.b > jacket.r && jacket.b > jacket.g,
           "default jacket stays a readable strong blue TPP silhouette");
    expect(std::abs(static_cast<int>(jacket.r) - static_cast<int>(pants.r)) > 20 ||
               std::abs(static_cast<int>(jacket.g) - static_cast<int>(pants.g)) > 20 ||
               std::abs(static_cast<int>(jacket.b) - static_cast<int>(pants.b)) > 20,
           "jacket and pants keep enough contrast for customization readability");

    bs3d::CharacterArtPalette custom = palette;
    custom.jacket = {186, 38, 42, 255};
    expect(bs3d::characterArtColorForMaterial(bs3d::CharacterArtMaterial::Jacket, custom).r == 186,
           "custom palette can recolor the jacket without changing model parts");
}

void characterVisualCatalogDefinesReadableNpcProfiles() {
    const std::vector<bs3d::CharacterVisualProfile>& catalog = bs3d::characterVisualCatalog();
    expect(catalog.size() >= 5, "character visual catalog includes player and named NPC profiles");

    const bs3d::CharacterArtPalette& playerPalette = bs3d::defaultCharacterArtPalette();
    const std::vector<std::string> requiredProfiles{
        "player", "bogus", "zenon", "lolek", "receipt_holder"};
    for (const std::string& profileId : requiredProfiles) {
        const bs3d::CharacterVisualProfile* profile = bs3d::findCharacterVisualProfile(profileId);
        expect(profile != nullptr, "character visual catalog contains profile: " + profileId);
        expect(!profile->label.empty(), "character visual profile has a readable label: " + profileId);
        expect(profile->palette.skin.a == 255 &&
                   profile->palette.jacket.a == 255 &&
                   profile->palette.pants.a == 255 &&
                   profile->palette.accent.a == 255,
               "character visual profile uses opaque runtime colors: " + profileId);
    }

    const bs3d::CharacterVisualProfile* bogus = bs3d::findCharacterVisualProfile("bogus");
    const bs3d::CharacterVisualProfile* zenon = bs3d::findCharacterVisualProfile("zenon");
    const bs3d::CharacterVisualProfile* lolek = bs3d::findCharacterVisualProfile("lolek");
    const bs3d::CharacterVisualProfile* receiptHolder = bs3d::findCharacterVisualProfile("receipt_holder");
    expect(bogus != nullptr && zenon != nullptr && lolek != nullptr && receiptHolder != nullptr,
           "named NPC visual profiles are findable for runtime rendering");

    expect(characterColorDistanceSquared(bogus->palette.jacket, playerPalette.jacket) > 1800,
           "Bogus is visually distinct from the player jacket");
    expect(characterColorDistanceSquared(zenon->palette.jacket, lolek->palette.jacket) > 1200,
           "Zenon and Lolek do not collapse into one NPC color read");
    expect(zenon->palette.accent.r > 180 && zenon->palette.accent.g > 170,
           "Zenon keeps a bright shopkeeper accent");
    expect(receiptHolder->palette.accent.r > receiptHolder->palette.accent.g + 60,
           "receipt holder has a warmer trouble accent for mission readability");

    const bs3d::CharacterVisualProfile& fallback = bs3d::characterVisualProfileOrDefault("missing_profile");
    expect(fallback.id == "player", "unknown character visual profiles fall back to player-safe rendering");
}

void characterDebugCapsuleGeometryMatchesControllerContract() {
    const bs3d::CharacterArtModelSpec& spec = bs3d::characterArtModelSpec();
    const bs3d::Vec3 base{2.0f, 0.25f, -3.0f};
    const bs3d::CharacterCapsuleDebugGeometry geometry =
        bs3d::buildCharacterCapsuleDebugGeometry(base, spec.capsuleRadius, spec.capsuleHeight);

    expectNear(geometry.baseCenter.x, base.x, 0.001f, "debug capsule base follows character position x");
    expectNear(geometry.baseCenter.y, base.y, 0.001f, "debug capsule base follows character position y");
    expectNear(geometry.baseCenter.z, base.z, 0.001f, "debug capsule base follows character position z");
    expectNear(geometry.radius, spec.capsuleRadius, 0.001f, "debug capsule radius matches gameplay capsule");
    expectNear(geometry.topCenter.y, base.y + spec.capsuleHeight, 0.001f,
               "debug capsule top matches authored controller height");
    expect(geometry.ringHeights.size() == 3, "debug capsule has foot/body/head rings");
    expect(geometry.verticalSegments == 4, "debug capsule has cardinal vertical guides");
}

void introLevelWorldObjectsAreArtKitReady() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    bool hasBlock = false;
    bool hasShop = false;
    bool hasParking = false;
    bool hasGarages = false;
    bool hasTrash = false;
    bool hasDeveloperSign = false;
    bool hasExplicitNoCollision = false;
    int shopIdentityCount = 0;
    int blockFacadeCount = 0;
    int garageIdentityCount = 0;
    int parkingPaintCount = 0;
    int trashDressingCount = 0;
    int routeGuidanceCount = 0;

    for (const bs3d::WorldObject& object : level.objects) {
        expect(!object.id.empty(), "world object has stable id");
        expect(!object.assetId.empty(), "world object has asset id");
        expect(object.collision.kind != bs3d::WorldCollisionShapeKind::Unspecified,
               "world object declares collision intent");
        hasExplicitNoCollision = hasExplicitNoCollision ||
                                 object.collision.kind == bs3d::WorldCollisionShapeKind::None;
        hasBlock = hasBlock || object.zoneTag == bs3d::WorldLocationTag::Block;
        hasShop = hasShop || object.zoneTag == bs3d::WorldLocationTag::Shop;
        hasParking = hasParking || object.zoneTag == bs3d::WorldLocationTag::Parking;
        hasGarages = hasGarages || object.id.find("garage") != std::string::npos;
        hasTrash = hasTrash || object.id.find("trash") != std::string::npos;
        hasDeveloperSign = hasDeveloperSign || object.id.find("developer") != std::string::npos;
        for (const std::string& tag : object.gameplayTags) {
            shopIdentityCount += tag == "shop_identity" ? 1 : 0;
            blockFacadeCount += tag == "block_facade" ? 1 : 0;
            garageIdentityCount += tag == "garage_identity" ? 1 : 0;
            parkingPaintCount += tag == "parking_paint" ? 1 : 0;
            trashDressingCount += tag == "trash_dressing" ? 1 : 0;
            routeGuidanceCount += tag == "route_guidance" ? 1 : 0;
        }
    }

    expect(hasBlock, "world contains Block13 objects");
    expect(hasShop, "world contains shop objects");
    expect(hasParking, "world contains parking objects");
    expect(hasGarages, "world contains garage objects");
    expect(hasTrash, "world contains trash area objects");
    expect(hasDeveloperSign, "world contains developer pressure dressing");
    expect(hasExplicitNoCollision, "decorative objects can explicitly opt out of collision");
    expect(shopIdentityCount >= 5, "shop has readable authored identity dressing");
    expect(blockFacadeCount >= 8, "block facade has windows/balconies/entrance dressing");
    expect(garageIdentityCount >= 4, "garage row has readable door/number dressing");
    expect(parkingPaintCount >= 8, "parking has painted bays/route readability dressing");
    expect(trashDressingCount >= 4, "trash area has bins/cardboard clutter dressing");
    expect(routeGuidanceCount >= 6, "road loop has visible drive-route guidance dressing");
}

void introLevelVisualIdentityV2DressesZenonAndBlock13HeroRead() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    const std::vector<std::string> requiredObjects{
        "shop_rolling_grille",
        "shop_door_handle",
        "shop_price_cards_left",
        "shop_price_cards_right",
        "shop_window_dirt_left",
        "shop_window_dirt_right",
        "block13_intercom_panel",
        "block13_mailboxes",
        "block13_entry_stain",
        "block13_balcony_clutter_0",
        "block13_balcony_clutter_1"};

    for (const std::string& objectId : requiredObjects) {
        expect(findObject(level, objectId) != nullptr,
               "visual identity v2 authored hero detail exists: " + objectId);
    }

    int shopHeroDetails = 0;
    int blockHeroDetails = 0;
    int subtleFacadeDecals = 0;
    int frontFacadeStains = 0;
    for (const bs3d::WorldObject& object : level.objects) {
        shopHeroDetails += hasTag(object, "shop_hero_v2") ? 1 : 0;
        blockHeroDetails += hasTag(object, "block_hero_v2") ? 1 : 0;
        frontFacadeStains += hasTag(object, "front_facade_stain") ? 1 : 0;
        if (hasTag(object, "subtle_decal")) {
            ++subtleFacadeDecals;
            expect(object.hasTintOverride, "subtle decal has explicit tint: " + object.id);
            expect(object.tintOverride.a <= 150,
                   "subtle decal alpha is muted enough to avoid sticker-like prototype read: " + object.id);
        }
    }

    expect(shopHeroDetails >= 6, "Zenon shop has enough v2 hero details for first-read identity");
    expect(blockHeroDetails >= 6, "Block 13 front has enough v2 domestic facade details");
    expect(frontFacadeStains >= 3, "Block 13 front facade gets subtle stains, not only side/rear grime");
    expect(subtleFacadeDecals >= 5, "v2 facade/window grime is deliberately subtle and art-directed");
}

void introLevelVisualIdentityV2AddsLivedInMicroBreakup() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    const std::vector<std::string> requiredObjects{
        "shop_threshold_worn",
        "shop_handwritten_notice",
        "shop_window_sticker_left",
        "shop_window_sticker_right",
        "block13_entry_notice",
        "block13_repair_patch",
        "block13_window_curtain_0",
        "block13_window_curtain_1"};

    for (const std::string& objectId : requiredObjects) {
        expect(findObject(level, objectId) != nullptr,
               "visual identity v2 lived-in micro detail exists: " + objectId);
    }

    int shopMicroDetails = 0;
    int blockMicroDetails = 0;
    int surfaceBreakup = 0;
    for (const bs3d::WorldObject& object : level.objects) {
        if (!hasTag(object, "lived_in_microdetail")) {
            continue;
        }
        expect(object.collision.kind == bs3d::WorldCollisionShapeKind::None,
               "lived-in micro detail stays decorative/no-collision: " + object.id);
        expect(std::max(object.scale.x, object.scale.y) <= 1.35f,
               "lived-in micro detail is small enough to break up surfaces, not become a blocky prop: " + object.id);
        shopMicroDetails += object.zoneTag == bs3d::WorldLocationTag::Shop ? 1 : 0;
        blockMicroDetails += object.zoneTag == bs3d::WorldLocationTag::Block ? 1 : 0;
        surfaceBreakup += hasTag(object, "surface_breakup") ? 1 : 0;
    }

    expect(shopMicroDetails >= 4, "Zenon storefront has lived-in small-use details, not only large flat panels");
    expect(blockMicroDetails >= 4, "Block 13 front has lived-in small-use details around entry/windows");
    expect(surfaceBreakup >= 6, "micro details deliberately break flat facade surfaces");
}

void worldObjectInteractionsExposeReadableLowPriorityAffordances() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    const std::vector<bs3d::InteractionCandidate> candidates =
        bs3d::buildWorldObjectInteractionCandidates(level.objects);

    bool hasShopDoor = false;
    bool hasGarageDoor = false;
    bool hasBench = false;
    bool hasTrash = false;
    bool hasParkingSign = false;
    int noisyGroundOrRoutePrompts = 0;
    std::unordered_set<std::string> outcomeIds;
    for (const bs3d::InteractionCandidate& candidate : candidates) {
        const std::optional<bs3d::WorldObjectInteractionAffordance> affordance =
            bs3d::findWorldObjectInteractionAffordance(level.objects, candidate.id);
        expect(affordance.has_value(), "object candidate can recover its affordance outcome: " + candidate.id);
        if (affordance.has_value()) {
            expect(!affordance->outcomeId.empty(),
                   "object affordance exposes a stable outcome id for mission hooks: " + candidate.id);
            expect(affordance->location != bs3d::WorldLocationTag::Unknown,
                   "object affordance preserves authored location for mission hooks: " + candidate.id);
            expect(outcomeIds.insert(affordance->outcomeId).second,
                   "object affordance outcome ids are unique: " + affordance->outcomeId);
        }
        expect(candidate.input == bs3d::InteractionInput::Use,
               "world object affordances stay on the E/use input family");
        expect(candidate.priority < 35,
               "world object affordances stay below vehicle and NPC priority");
        expect(candidate.radius >= 1.4f && candidate.radius <= 2.9f,
               "world object affordances use close object-scale radii");

        hasShopDoor = hasShopDoor || candidate.id == "object:shop_door_front";
        hasBench = hasBench || candidate.id == "object:bogus_bench";
        hasTrash = hasTrash || candidate.id.find("trash_") != std::string::npos;
        hasGarageDoor = hasGarageDoor || candidate.id.find("object:garage_door_") == 0;
        hasParkingSign = hasParkingSign || candidate.id == "object:sign_no_parking";
        noisyGroundOrRoutePrompts += candidate.id.find("asphalt_patch") != std::string::npos ? 1 : 0;
        noisyGroundOrRoutePrompts += candidate.id.find("route_arrow") != std::string::npos ? 1 : 0;
    }

    expect(hasShopDoor, "shop door gets a readable object affordance");
    expect(hasGarageDoor, "garage doors get future-service affordances");
    expect(hasBench, "Bogus bench gets a low-priority object affordance");
    expect(hasTrash, "trash dressing gets a small physical affordance");
    expect(hasParkingSign, "parking no-parking sign exposes a readable affordance");
    expect(noisyGroundOrRoutePrompts == 0,
           "ground patches and route guidance do not spam object interaction prompts");

    const std::optional<bs3d::WorldObjectInteractionAffordance> shopDoor =
        bs3d::findWorldObjectInteractionAffordance(level.objects, "object:shop_door_front");
    expect(shopDoor.has_value(), "shop door affordance can be recovered during execution");
    if (shopDoor.has_value()) {
        expect(shopDoor->action == bs3d::InteractionAction::UseDoor,
               "shop door affordance uses the door action");
        expect(shopDoor->outcomeId == "shop_door_checked",
               "shop door affordance has a stable mission outcome id");
        expect(!shopDoor->line.empty() && !shopDoor->speaker.empty(),
               "shop door affordance has player-facing feedback text");
    }
}

void visualIdentityHeroDetailsExposeCorrectObjectAffordances() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    const std::vector<bs3d::InteractionCandidate> candidates =
        bs3d::buildWorldObjectInteractionCandidates(level.objects);

    bool hasShopGrille = false;
    bool hasShopPrices = false;
    bool hasShopNotice = false;
    bool hasBlockNotice = false;
    bool hasIntercom = false;
    bool hasParkingSign = false;
    int wrongGarageShopPrompts = 0;
    std::unordered_set<std::string> candidateIds;
    for (const bs3d::InteractionCandidate& candidate : candidates) {
        candidateIds.insert(candidate.id);
        const std::optional<bs3d::WorldObjectInteractionAffordance> affordance =
            bs3d::findWorldObjectInteractionAffordance(level.objects, candidate.id);
        expect(affordance.has_value(), "v2 hero affordance can recover outcome: " + candidate.id);
        if (!affordance.has_value()) {
            continue;
        }

        hasShopGrille = hasShopGrille || affordance->outcomeId == "shop_rolling_grille_checked";
        hasShopPrices = hasShopPrices || affordance->outcomeId.find("shop_prices_read_") == 0;
        hasShopNotice = hasShopNotice || affordance->outcomeId == "shop_notice_read_shop_handwritten_notice";
        hasBlockNotice = hasBlockNotice || affordance->outcomeId == "block_notice_read_block13_entry_notice";
        hasIntercom = hasIntercom || affordance->outcomeId == "block_intercom_buzzed";
        hasParkingSign = hasParkingSign || affordance->outcomeId == "parking_sign_read_sign_no_parking";
        if (affordance->objectId == "shop_rolling_grille" &&
            affordance->outcomeId.find("garage_door_checked_") == 0) {
            ++wrongGarageShopPrompts;
        }
    }

    expect(candidateIds.find("object:shop_rolling_grille") != candidateIds.end(),
           "Zenon shop rolling grille has a shop-specific affordance");
    expect(hasShopGrille, "shop rolling grille outcome is shop-specific, not garage-specific");
    expect(hasShopPrices, "shop price cards expose a small local satire affordance");
    expect(hasShopNotice, "Zenon handwritten notice exposes a readable quiet affordance");
    expect(hasBlockNotice, "Block 13 entry notice exposes a readable quiet affordance");
    expect(hasIntercom, "Block 13 intercom exposes a domestic facade affordance");
    expect(hasParkingSign, "Parking no-parking sign exposes a readable quiet parking affordance");
    expect(wrongGarageShopPrompts == 0,
           "shop hero dressing that reuses garage art does not become a garage prompt");
}

void objectAffordancesCanFeedPrzypalAndWorldMemoryWhenNoisy() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    bs3d::ObjectOutcomeCatalogData catalog;
    catalog.loaded = true;
    catalog.outcomes.push_back({"block_intercom_buzzed", "", "", "", "", "",
                                bs3d::ObjectOutcomeWorldEventData{bs3d::WorldEventType::PublicNuisance, 0.20f, 3.0f}});
    catalog.outcomes.push_back({"", "trash_disturbed_*", "", "", "", "",
                                bs3d::ObjectOutcomeWorldEventData{bs3d::WorldEventType::PublicNuisance, 0.18f, 2.5f}});

    const std::optional<bs3d::WorldObjectInteractionAffordance> intercom =
        bs3d::findWorldObjectInteractionAffordance(level.objects, "object:block13_intercom_panel");
    const std::optional<bs3d::WorldObjectInteractionAffordance> prices =
        bs3d::findWorldObjectInteractionAffordance(level.objects, "object:shop_price_cards_left");
    const std::optional<bs3d::WorldObjectInteractionAffordance> shopNotice =
        bs3d::findWorldObjectInteractionAffordance(level.objects, "object:shop_handwritten_notice");
    const std::optional<bs3d::WorldObjectInteractionAffordance> blockNotice =
        bs3d::findWorldObjectInteractionAffordance(level.objects, "object:block13_entry_notice");
    const std::optional<bs3d::WorldObjectInteractionAffordance> trash =
        bs3d::findWorldObjectInteractionAffordance(level.objects, "object:trash_green_bin_0");
    const std::optional<bs3d::WorldObjectInteractionAffordance> parkingSign =
        bs3d::findWorldObjectInteractionAffordance(level.objects, "object:sign_no_parking");

    expect(intercom.has_value() && prices.has_value() && shopNotice.has_value() && blockNotice.has_value() &&
               trash.has_value() && parkingSign.has_value(),
           "v2 object affordances are recoverable for policy/event checks");

    const std::optional<bs3d::WorldObjectInteractionEvent> intercomEvent =
        intercom.has_value() ? bs3d::worldEventForObjectAffordance(*intercom, &catalog) : std::nullopt;
    const std::optional<bs3d::WorldObjectInteractionEvent> priceEvent =
        prices.has_value() ? bs3d::worldEventForObjectAffordance(*prices, &catalog) : std::nullopt;
    const std::optional<bs3d::WorldObjectInteractionEvent> shopNoticeEvent =
        shopNotice.has_value() ? bs3d::worldEventForObjectAffordance(*shopNotice, &catalog) : std::nullopt;
    const std::optional<bs3d::WorldObjectInteractionEvent> blockNoticeEvent =
        blockNotice.has_value() ? bs3d::worldEventForObjectAffordance(*blockNotice, &catalog) : std::nullopt;
    const std::optional<bs3d::WorldObjectInteractionEvent> trashEvent =
        trash.has_value() ? bs3d::worldEventForObjectAffordance(*trash, &catalog) : std::nullopt;
    const std::optional<bs3d::WorldObjectInteractionEvent> parkingSignEvent =
        parkingSign.has_value() ? bs3d::worldEventForObjectAffordance(*parkingSign, &catalog) : std::nullopt;

    expect(intercomEvent.has_value(), "buzzing the Block 13 intercom can feed Przypal/world memory");
    if (intercomEvent.has_value()) {
        expect(intercomEvent->type == bs3d::WorldEventType::PublicNuisance,
               "intercom buzz is a public nuisance, not property damage");
        expect(intercomEvent->source == "block_intercom_buzzed",
               "intercom event source matches stable outcome id");
        expect(intercomEvent->intensity >= 0.12f && intercomEvent->intensity <= 0.28f,
               "intercom event is noticeable but low-stakes");
        expect(intercomEvent->cooldownSeconds >= 2.0f,
               "intercom event has cooldown so E-spam does not farm Przypal");
    }

    expect(trashEvent.has_value(), "disturbing trash can feed local memory as a nuisance");
    if (trashEvent.has_value()) {
        expect(trashEvent->type == bs3d::WorldEventType::PublicNuisance,
               "trash interaction remains nuisance-level social noise");
        expect(trashEvent->source == "trash_disturbed_trash_green_bin_0",
               "trash event source stays per-object for local memory");
    }

    expect(!priceEvent.has_value(),
           "reading shop prices stays quiet local satire and does not create Przypal memory");
    expect(!shopNoticeEvent.has_value(),
           "reading Zenon notice stays quiet local satire and does not create Przypal memory");
    expect(!blockNoticeEvent.has_value(),
           "reading Block 13 notice stays quiet domestic flavor and does not create Przypal memory");
    expect(!parkingSignEvent.has_value(),
           "reading parking sign stays quiet local satire and does not create Przypal/world memory");
}

void introLevelV092HasReadableMissionTargetsAndDriveRoute() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    const bs3d::WorldObject* shop = findObject(level, "shop_zenona");
    expect(shop != nullptr, "v0.9.2 layout keeps a stable shop building object");

    const bs3d::WorldLandmark* shopLandmark = findLandmarkByRole(level, "shop");
    expect(shopLandmark != nullptr, "v0.9.2 exports a shop landmark");
    expect(!pointInsideBoxCollision(*shop, shopLandmark->position, 0.05f),
           "shop landmark sits at the readable entrance/apron, not inside the building collision");

    for (const bs3d::MissionTriggerDefinition& trigger : level.missionTriggers) {
        if (trigger.id == "shop_walk_intro" || trigger.id == "shop_vehicle_intro") {
            expect(!pointInsideBoxCollision(*shop, trigger.position, 0.05f),
                   "shop mission trigger sits outside the shop collision");
        }
    }

    expect(level.driveRoute.size() >= 5, "drive route uses staged waypoints through the road loop");
    expect(level.driveRoute.front().label == "Wyjazd", "first drive route point names the parking exit");
    expect(level.driveRoute.back().label == "Pod sklep", "last drive route point names the shop apron");
    for (const bs3d::RoutePoint& point : level.driveRoute) {
        expect(!pointInsideBoxCollision(*shop, point.position, 0.05f),
               "drive route point does not sit inside shop collision");
        expect(point.radius >= 2.2f, "drive route points are forgiving enough for gruz driving");
    }
}

void introLevelSolidWorldObjectsUseBaseAnchoredCollision() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    const std::vector<std::string> baseAnchoredSolids{
        "block13_core",
        "block13_neighbor",
        "block13_side",
        "shop_zenona",
        "garage_row_rysiek",
        "trash_shelter_shop_side",
        "future_rewir_wall_west",
        "future_rewir_wall_east",
        "future_rewir_wall_south",
        "future_rewir_wall_south_east",
        "future_rewir_wall_north"};

    for (const std::string& id : baseAnchoredSolids) {
        const bs3d::WorldObject* object = findObject(level, id);
        expect(object != nullptr, "base-anchored solid exists: " + id);
        expect(object->collision.kind == bs3d::WorldCollisionShapeKind::Box,
               "base-anchored solid uses box collision: " + id);
        expectNear(object->collision.offset.y,
                   object->collision.size.y * 0.5f,
                   0.001f,
                   "solid world collision is centered above the authored ground anchor: " + id);
    }
}

void introLevelBoundaryWallsShowTheirWholeBlockingFootprint() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    const std::vector<std::string> boundaryWalls{
        "future_rewir_wall_west",
        "future_rewir_wall_east",
        "future_rewir_wall_south",
        "future_rewir_wall_south_east",
        "future_rewir_wall_north"};

    for (const std::string& id : boundaryWalls) {
        const bs3d::WorldObject* wall = findObject(level, id);
        expect(wall != nullptr, "boundary wall exists: " + id);
        expect(wall->collision.kind == bs3d::WorldCollisionShapeKind::Box,
               "boundary wall uses box collision: " + id);
        expect(wall->scale.x + 0.001f >= wall->collision.size.x,
               "boundary wall visible X footprint covers its blocking X footprint: " + id);
        expect(wall->scale.z + 0.001f >= wall->collision.size.z,
               "boundary wall visible Z footprint covers its blocking Z footprint: " + id);
    }
}

void introLevelGarageBeltLaneKeepsSixMeterDriveableScale() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    const bs3d::WorldObject* lane = findObject(level, "garage_lane");
    const bs3d::WorldObject* row2 = findObject(level, "garage_row_belt_2");
    expect(lane != nullptr, "garage belt has a side-street lane surface");
    expect(row2 != nullptr, "garage belt has row 2 for lane clearance checks");

    expect(lane->scale.x >= 5.8f,
           "garage belt side street is authored at roughly six meters for gruz driving");

    const float laneWestEdge = lane->position.x - lane->scale.x * 0.5f;
    const float row2EastCollisionEdge = row2->position.x + row2->collision.size.x * 0.5f;
    expect(laneWestEdge + 0.05f >= row2EastCollisionEdge,
           "garage belt lane does not visually bury itself inside row 2 collision");
}

void introLevelBuildWithManifestEnforcesRuntimeAssetContract() {
    bs3d::WorldAssetRegistry registry;
    const bs3d::AssetManifestLoadResult manifest = registry.loadManifest("data/assets/asset_manifest.txt");
    expect(manifest.loaded, "runtime contract test loads shipped asset manifest");

    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build(bs3d::IntroLevelBuildConfig{}, registry);
    const bs3d::WorldObject* shop = findObject(level, "shop_zenona");
    expect(shop != nullptr, "active shop object exists");
    expect(shop->assetId == "shop_zenona_v3", "active shop uses the v3 hero asset");
    expect(shop->collision.kind == bs3d::WorldCollisionShapeKind::Box, "active shop keeps solid collision");
    expect(shop->collisionProfile.blocksCamera, "active shop collision blocks camera from manifest metadata");
    expect(hasLayer(*shop, bs3d::CollisionLayer::CameraBlocker), "active shop collision has camera blocker layer");

    int metadataNonBlockingObjects = 0;
    for (const bs3d::WorldObject& object : level.objects) {
        const bs3d::WorldAssetDefinition* definition = registry.find(object.assetId);
        if (definition == nullptr) {
            continue;
        }
        const bool noCollisionAsset = definition->defaultCollision == "None" ||
                                      definition->renderBucket == "Decal" ||
                                      definition->renderBucket == "Glass" ||
                                      definition->renderBucket == "DebugOnly" ||
                                      definition->assetType == "DebugOnly" ||
                                      !definition->renderInGameplay;
        if (!noCollisionAsset) {
            continue;
        }
        ++metadataNonBlockingObjects;
        expect(object.collision.kind == bs3d::WorldCollisionShapeKind::None,
               "manifest non-collision asset stays out of WorldCollision authoring: " + object.id);
        expect(object.collisionProfile.responseKind == bs3d::CollisionResponseKind::None,
               "manifest non-collision asset has no collision response: " + object.id);
        expect(!object.collisionProfile.blocksCamera,
               "manifest non-collision asset does not block camera: " + object.id);
        expect(!hasLayer(object, bs3d::CollisionLayer::CameraBlocker),
               "manifest non-collision asset has no camera blocker layer: " + object.id);
    }

    expect(metadataNonBlockingObjects >= 8, "intro level exercises decal/glass/debug-only no-collision metadata");
}

void introLevelPopulateWorldPreservesAuthoredObjectYaw() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::Scene scene;
    bs3d::WorldCollision collision;
    bs3d::IntroLevelBuilder::populateWorld(level, scene, collision);

    expect(scene.objectCount() == level.objects.size(), "scene keeps one transform per authored world object");
    const std::vector<bs3d::SceneObject>& sceneObjects = scene.objects();
    bool checkedRotatedObject = false;
    for (std::size_t i = 0; i < level.objects.size(); ++i) {
        if (std::fabs(level.objects[i].yawRadians) <= 0.001f) {
            continue;
        }
        checkedRotatedObject = true;
        expectNear(sceneObjects[i].transform.yawRadians,
                   level.objects[i].yawRadians,
                   0.001f,
                   "populateWorld forwards authored yaw to scene transform: " + level.objects[i].id);
        break;
    }

    expect(checkedRotatedObject, "intro level contains rotated authored objects for yaw regression coverage");
}

void runtimeWorldRenderingUsesWorldObjectsAsSingleSourceOfTruth() {
    const std::string gameApp = readTextFile("src/game/GameApp.cpp");

    expect(textContains(gameApp, "snapshot.worldObjects = level.objects"),
           "runtime render snapshot sources world geometry from authored WorldObject data");
    expect(textContains(gameApp, "renderCoordinator.buildWorldRenderList(renderSnapshot.worldObjects"),
           "runtime builds a render list from the authored WorldObject snapshot");
    expect(textContains(gameApp, "renderCoordinator.drawWorldOpaque(worldRenderList"),
           "runtime draws opaque world geometry through the WorldObject renderer");
    expect(textContains(gameApp, "renderCoordinator.drawWorldTransparent(worldRenderList"),
           "runtime draws transparent world geometry through the WorldObject renderer");
    expect(!textContains(gameApp, "drawWorldBase("),
           "runtime does not draw legacy WorldProp cubes on top of authored WorldObjects");
    expect(!textContains(gameApp, "level.props"),
           "runtime does not render legacy fallback props in the active draw path");

    const std::string rendererHeader = readTextFile("src/game/GameRenderers.h");
    expect(!textContains(rendererHeader, "drawWorldBase"),
           "renderer API no longer exposes legacy world-base cube drawing");
    expect(!textContains(rendererHeader, "drawWorldObjects("),
           "renderer API does not expose a combined opaque+transparent world draw that bypasses dynamic pass ordering");
}

void runtimeWorldRenderPassOrderKeepsTransparentAfterOpaqueDynamics() {
    const std::string gameApp = readTextFile("src/game/GameApp.cpp");

    const std::size_t ground = gameApp.find("WorldRenderer::drawWorldGround();");
    const std::size_t worldOpaque = gameApp.find("renderCoordinator.drawWorldOpaque(worldRenderList");
    const std::size_t vehicleOpaque = gameApp.find("WorldRenderer::drawVehicleOpaque(renderSnapshot.vehicle");
    const std::size_t actors = gameApp.find("drawParagonActors();");
    const std::size_t player = gameApp.find("WorldRenderer::drawPlayer(renderSnapshot.playerPosition");
    const std::size_t worldTransparent = gameApp.find("renderCoordinator.drawWorldTransparent(worldRenderList");
    const std::size_t vehicleGlass = gameApp.find("WorldRenderer::drawVehicleGlass(renderSnapshot.vehicle");
    const std::size_t vehicleSmoke = gameApp.find("WorldRenderer::drawVehicleSmoke(renderSnapshot.vehicle");
    const std::size_t markers = gameApp.find("IntroLevelPresentation::drawMarkers");
    const std::size_t end3d = gameApp.find("EndMode3D();");
    const std::size_t hud = gameApp.find("drawHud(renderSnapshot);");

    expect(ground != std::string::npos &&
               worldOpaque != std::string::npos &&
               vehicleOpaque != std::string::npos &&
               actors != std::string::npos &&
               player != std::string::npos &&
               worldTransparent != std::string::npos &&
               vehicleGlass != std::string::npos &&
               vehicleSmoke != std::string::npos &&
               markers != std::string::npos &&
               end3d != std::string::npos &&
               hud != std::string::npos,
           "runtime pass order test finds all expected draw calls");
    expect(ground < worldOpaque, "ground renders before world geometry");
    expect(worldOpaque < vehicleOpaque, "opaque world renders before opaque vehicles");
    expect(vehicleOpaque < actors && actors < worldTransparent, "opaque dynamic actors render before world transparent");
    expect(player < worldTransparent, "player renders before world transparent");
    expect(worldTransparent < vehicleGlass, "world transparent renders before vehicle glass");
    expect(vehicleGlass < vehicleSmoke, "vehicle glass renders before vehicle smoke");
    expect(vehicleSmoke < markers, "transparent dynamic effects render before 3D markers");
    expect(markers < end3d && end3d < hud, "3D markers render before the 2D HUD");
}

void worldDataApplyRebuildsDerivedIntroLevelAuthoring() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::WorldDataCatalog catalog;
    catalog.loaded = true;
    catalog.world.loaded = true;
    catalog.mission.loaded = true;
    catalog.world.playerSpawn = {-3.0f, 0.0f, -4.0f};
    catalog.world.vehicleSpawn = {-1.0f, 0.0f, 5.0f};
    catalog.world.npcSpawn = {-5.0f, 0.0f, 2.0f};
    catalog.world.shopPosition = {4.0f, 0.0f, -9.0f};
    catalog.world.dropoffPosition = {-6.0f, 0.0f, 7.0f};
    catalog.mission.phases.push_back({"WalkToShop", "Data objective", "shop_walk_intro"});

    const bs3d::WorldDataApplyResult applied = bs3d::applyWorldDataCatalog(level, catalog);

    expect(applied.applied, "catalog overlay applies to intro level");
    const bs3d::WorldObject* shop = findObject(level, "shop_zenona");
    expect(shop != nullptr, "rebuilt level still exports shop object");
    expectNear(shop->position.x, catalog.world.shopPosition.x, 0.001f,
               "map data shop position drives authored shop geometry x");
    expectNear(shop->position.z, catalog.world.shopPosition.z, 0.001f,
               "map data shop position drives authored shop geometry z");

    const bs3d::WorldLandmark* shopLandmark = findLandmarkByRole(level, "shop");
    expect(shopLandmark != nullptr, "rebuilt level exports shop landmark");
    expectNear(shopLandmark->position.x, level.shopEntrancePosition.x, 0.001f,
               "rebuilt shop landmark follows derived shop entrance x");
    expectNear(level.missionTriggers[0].position.x, level.shopEntrancePosition.x, 0.001f,
               "rebuilt shop trigger follows derived shop entrance x");

    bool foundShopFallbackProp = false;
    for (const bs3d::WorldProp& prop : level.props) {
        if (std::fabs(prop.size.x - 8.0f) > 0.001f ||
            std::fabs(prop.size.y - 3.5f) > 0.001f ||
            std::fabs(prop.size.z - 6.0f) > 0.001f) {
            continue;
        }
        foundShopFallbackProp = true;
        expectNear(prop.center.x, catalog.world.shopPosition.x, 0.001f,
                   "legacy fallback prop is generated from rebuilt shop object x");
        expectNear(prop.center.z, catalog.world.shopPosition.z, 0.001f,
                   "legacy fallback prop is generated from rebuilt shop object z");
    }
    expect(foundShopFallbackProp, "rebuilt level exports a fallback prop for the shop object");
}

void worldDataCatalogAppliesEditorOverlayAfterBaseMap() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::WorldDataCatalog catalog;
    catalog.loaded = true;
    catalog.world.loaded = true;
    catalog.mission.loaded = true;
    catalog.world.playerSpawn = level.playerStart;
    catalog.world.vehicleSpawn = level.vehicleStart;
    catalog.world.npcSpawn = level.npcPosition;
    catalog.world.shopPosition = level.shopPosition;
    catalog.world.dropoffPosition = level.dropoffPosition;
    catalog.mission.phases.push_back({"ReachVehicle", "Enter the car.", "player_enters_vehicle"});
    catalog.editorOverlay.loaded = true;
    catalog.editorOverlay.document.instances.push_back(
        {"editor_catalog_lamp",
         "lamp_post_lowpoly",
         {3.0f, 0.0f, -24.0f},
         {0.18f, 3.1f, 0.18f},
         0.0f,
         bs3d::WorldLocationTag::RoadLoop,
         {"vertical_readability"}});

    const bs3d::WorldDataApplyResult result = bs3d::applyWorldDataCatalog(level, catalog);

    expect(result.applied, "world data catalog still applies");
    expect(result.appliedEditorOverlayInstances == 1, "catalog applies editor overlay instances");
    expect(findObject(level, "editor_catalog_lamp") != nullptr,
           "editor overlay instance exists after catalog apply");
}

void runtimeMapEditorEditsSelectedObjectAndTracksDirtyState() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::RuntimeMapEditor editor;
    editor.attach(level);

    expect(editor.selectObject("sign_no_parking"), "runtime editor selects existing object");
    expect(editor.selectedObjectId() == "sign_no_parking", "runtime editor reports selection");
    editor.setSelectedPosition({-4.0f, 0.0f, 6.0f});

    const bs3d::WorldObject* object = findObject(level, "sign_no_parking");
    expect(object != nullptr, "edited object still exists");
    expectNear(object->position.x, -4.0f, 0.001f, "runtime editor edits selected position");
    expect(editor.dirty(), "runtime editor becomes dirty after edit");
}

void runtimeMapEditorAddsManifestInstance() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::RuntimeMapEditor editor;
    editor.attach(level);

    expect(editor.addInstance("lamp_post_lowpoly", {18.0f, 0.0f, -27.0f}), "runtime editor adds instance");
    const bs3d::WorldObject* object = findObject(level, "editor_lamp_post_lowpoly_0");
    expect(object != nullptr, "runtime editor generates stable instance id");
    expect(object != nullptr && object->assetId == "lamp_post_lowpoly",
           "runtime editor instance keeps selected asset id");
    expect(editor.dirty(), "runtime editor dirty after add instance");
}

void runtimeMapEditorAddsDefinitionInstanceWithManifestTags() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::WorldAssetDefinition asset;
    asset.id = "bench";
    asset.defaultTags = {"prop", "seating"};

    bs3d::RuntimeMapEditor editor;
    editor.attach(level);

    expect(editor.addInstance(asset, {2.0f, 0.0f, -3.0f}),
           "runtime editor adds instance from full manifest definition");
    const bs3d::WorldObject* object = findObject(level, "editor_bench_0");
    expect(object != nullptr, "runtime editor definition instance exists");
    expect(object != nullptr && hasTag(*object, "prop"), "runtime editor copies default manifest prop tag");
    expect(object != nullptr && hasTag(*object, "seating"), "runtime editor copies default manifest seating tag");

    const bs3d::EditorOverlayDocument overlay = editor.buildOverlayDocument();
    expect(overlay.instances.size() == 1, "runtime editor exports tagged definition instance");
    expect(overlay.instances[0].gameplayTags.size() == 2,
           "runtime editor preserves manifest tags in editor overlay output");
}

void runtimeEditorAssetFilterMatchesManifestMetadata() {
    bs3d::WorldAssetDefinition asset;
    asset.id = "irregular_asphalt_patch";
    asset.modelPath = "models/irregular_patch.obj";
    asset.originPolicy = "bottom_center";
    asset.renderBucket = "Decal";
    asset.defaultCollision = "None";
    asset.defaultTags = {"decal", "ground_patch"};

    expect(bs3d::runtimeEditorAssetMatchesFilter(asset, ""),
           "runtime editor asset filter accepts empty filter");
    expect(bs3d::runtimeEditorAssetMatchesFilter(asset, " asphalt "),
           "runtime editor asset filter matches asset id");
    expect(bs3d::runtimeEditorAssetMatchesFilter(asset, "DECAL ground_patch"),
           "runtime editor asset filter is case-insensitive and matches render bucket plus tag");
    expect(bs3d::runtimeEditorAssetMatchesFilter(asset, "ground_patch none"),
           "runtime editor asset filter matches tags and collision intent");
    expect(!bs3d::runtimeEditorAssetMatchesFilter(asset, "vehicle"),
           "runtime editor asset filter rejects unrelated tokens");
}

void runtimeMapEditorComputesPlacementInFrontOfCamera() {
    const bs3d::Vec3 anchor{2.0f, 0.0f, 4.0f};

    const bs3d::Vec3 placement = bs3d::runtimeEditorPlacementPosition(anchor, bs3d::Pi * 0.5f, 3.0f);

    expectNear(placement.x, 5.0f, 0.001f, "runtime editor placement uses camera forward x");
    expectNear(placement.y, 0.0f, 0.001f, "runtime editor placement keeps ground anchor y");
    expectNear(placement.z, 4.0f, 0.001f, "runtime editor placement uses camera forward z");
}

void runtimeEditorOverlayPathUsesDataRoot() {
    const std::string overlayPath = bs3d::runtimeEditorOverlayPathForDataRoot("C:/slice/data");

    expect(overlayPath.find("C:/slice/data") != std::string::npos,
           "runtime editor overlay path starts from configured data root");
    expect(overlayPath.find("world") != std::string::npos &&
               overlayPath.find("block13_editor_overlay.json") != std::string::npos,
           "runtime editor overlay path points at the editor overlay under world data");
}

void runtimeMapEditorEditsSelectedMetadata() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::RuntimeMapEditor editor;
    editor.attach(level);

    expect(editor.selectObject("sign_no_parking"), "runtime editor selects object for metadata edit");
    expect(editor.setSelectedAssetId("lamp_post_lowpoly"), "runtime editor edits selected asset id");
    expect(editor.setSelectedZoneTag(bs3d::WorldLocationTag::Garage), "runtime editor edits selected zone tag");
    expect(editor.setSelectedGameplayTags({"editor_adjusted", "drive_readability"}),
           "runtime editor edits selected gameplay tags");

    const bs3d::WorldObject* object = findObject(level, "sign_no_parking");
    expect(object != nullptr, "metadata-edited object still exists");
    expect(object != nullptr && object->assetId == "lamp_post_lowpoly", "runtime editor changed asset id");
    expect(object != nullptr && object->zoneTag == bs3d::WorldLocationTag::Garage, "runtime editor changed zone");
    expect(object != nullptr && hasTag(*object, "editor_adjusted"), "runtime editor changed gameplay tags");
    expect(editor.dirty(), "runtime editor metadata edits mark dirty");
}

void runtimeMapEditorBuildsOverlayForEditedObjects() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::RuntimeMapEditor editor;
    editor.attach(level);
    editor.selectObject("sign_no_parking");
    editor.setSelectedPosition({-4.0f, 0.0f, 6.0f});
    editor.addInstance("lamp_post_lowpoly", {18.0f, 0.0f, -27.0f});

    const bs3d::EditorOverlayDocument overlay = editor.buildOverlayDocument();

    expect(overlay.overrides.size() == 1, "runtime editor exports one changed base override");
    expect(overlay.instances.size() == 1, "runtime editor exports one editor instance");
    expect(overlay.overrides[0].id == "sign_no_parking", "runtime editor override keeps base id");
    expect(overlay.instances[0].id == "editor_lamp_post_lowpoly_0", "runtime editor instance keeps generated id");
}

void runtimeMapEditorPreservesLoadedBaseOverridesOnSave() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::EditorOverlayDocument loadedOverlay;
    bs3d::EditorOverlayObject loadedOverride;
    loadedOverride.id = "sign_no_parking";
    loadedOverride.assetId = "street_sign";
    loadedOverride.position = {-4.25f, 0.0f, 5.9f};
    loadedOverride.scale = {0.58f, 1.75f, 0.08f};
    loadedOverride.zoneTag = bs3d::WorldLocationTag::Parking;
    loadedOverride.gameplayTags = {"drive_readability", "loaded_override"};
    loadedOverlay.overrides.push_back(loadedOverride);
    bs3d::applyEditorOverlay(level, loadedOverlay);

    bs3d::RuntimeMapEditor editor;
    editor.attach(level, loadedOverlay);
    editor.addInstance("lamp_post_lowpoly", {18.0f, 0.0f, -27.0f});

    const bs3d::EditorOverlayDocument savedOverlay = editor.buildOverlayDocument();

    expect(savedOverlay.overrides.size() == 1, "runtime editor keeps loaded base override during save");
    expect(savedOverlay.overrides[0].id == "sign_no_parking", "runtime editor keeps loaded override id");
    expect(savedOverlay.instances.size() == 1, "runtime editor still exports new instances with loaded overrides");
}

void runtimeMapEditorOnlyAllowsDeletingOverlayInstances() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::RuntimeMapEditor editor;
    editor.attach(level);

    expect(editor.selectObject("sign_no_parking"), "runtime editor selects base object for delete guard");
    expect(!editor.canDeleteSelectedOverlayInstance(), "runtime editor does not allow deleting base objects");

    expect(editor.addInstance("lamp_post_lowpoly", {18.0f, 0.0f, -27.0f}), "runtime editor adds deletable instance");
    expect(editor.canDeleteSelectedOverlayInstance(), "runtime editor allows deleting editor-created instance");
    expect(editor.deleteSelectedOverlayInstance(), "runtime editor deletes selected editor instance");
    expect(findObject(level, "editor_lamp_post_lowpoly_0") == nullptr, "runtime editor removed editor instance");
}

void runtimeMapEditorUndoRedoRestoresBaseEditsAndOverlayTracking() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    const bs3d::WorldObject* originalObject = findObject(level, "sign_no_parking");
    expect(originalObject != nullptr, "runtime editor undo test finds base object");
    const bs3d::Vec3 originalPosition = originalObject->position;

    bs3d::RuntimeMapEditor editor;
    editor.attach(level);
    expect(editor.selectObject("sign_no_parking"), "runtime editor selects object for undo test");
    expect(!editor.canUndo(), "runtime editor starts without undo history");
    expect(editor.setSelectedPosition({-4.0f, 0.0f, 6.0f}), "runtime editor edits position for undo");

    expect(editor.canUndo(), "runtime editor can undo after base edit");
    expect(!editor.canRedo(), "runtime editor redo stack starts empty after new edit");
    expect(editor.buildOverlayDocument().overrides.size() == 1,
           "runtime editor exports edited base object before undo");

    expect(editor.undo(), "runtime editor undoes base edit");
    const bs3d::WorldObject* undoneObject = findObject(level, "sign_no_parking");
    expect(undoneObject != nullptr, "runtime editor undo keeps base object");
    expectNear(undoneObject->position.x, originalPosition.x, 0.001f, "runtime editor undo restores x");
    expectNear(undoneObject->position.z, originalPosition.z, 0.001f, "runtime editor undo restores z");
    expect(editor.buildOverlayDocument().overrides.empty(),
           "runtime editor undo removes unsaved base override tracking");
    expect(editor.canRedo(), "runtime editor can redo after undo");

    expect(editor.redo(), "runtime editor redoes base edit");
    const bs3d::WorldObject* redoneObject = findObject(level, "sign_no_parking");
    expect(redoneObject != nullptr, "runtime editor redo keeps base object");
    expectNear(redoneObject->position.x, -4.0f, 0.001f, "runtime editor redo restores edited x");
    expectNear(redoneObject->position.z, 6.0f, 0.001f, "runtime editor redo restores edited z");
    expect(editor.buildOverlayDocument().overrides.size() == 1,
           "runtime editor redo restores base override tracking");
}

void runtimeMapEditorCommitsSilentDragAsUndoableEdit() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    const bs3d::WorldObject* originalObject = findObject(level, "sign_no_parking");
    expect(originalObject != nullptr, "runtime editor silent drag test finds base object");
    const bs3d::Vec3 originalPosition = originalObject->position;

    bs3d::RuntimeMapEditor editor;
    editor.attach(level);
    expect(editor.selectObject("sign_no_parking"), "runtime editor selects base object for silent drag");

    bs3d::RuntimeMapEditor::HistoryState beforeDrag = editor.captureState();
    expect(editor.setSelectedPositionSilent({-4.0f, 0.0f, 6.0f}), "silent drag updates selected position");
    expect(!editor.canUndo(), "silent drag stays uncommitted while pointer is still down");

    expect(editor.commitCapturedEdit(std::move(beforeDrag)), "runtime editor commits silent drag edit");
    expect(editor.dirty(), "committed silent drag marks editor dirty");
    expect(editor.canUndo(), "committed silent drag becomes undoable");
    expect(editor.buildOverlayDocument().overrides.size() == 1,
           "committed silent drag exports base object override");

    expect(editor.undo(), "runtime editor undoes committed silent drag");
    const bs3d::WorldObject* undoneObject = findObject(level, "sign_no_parking");
    expect(undoneObject != nullptr, "undo keeps dragged object");
    expectNear(undoneObject->position.x, originalPosition.x, 0.001f, "undo restores pre-drag x");
    expectNear(undoneObject->position.z, originalPosition.z, 0.001f, "undo restores pre-drag z");
}

void runtimeMapEditorCommitsSilentDragForEditorInstance() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::WorldObject editorObject;
    editorObject.id = "editor_lamp_manual";
    editorObject.assetId = "lamp_post_lowpoly";
    editorObject.position = {1.0f, 0.0f, 2.0f};
    level.objects.push_back(editorObject);

    bs3d::RuntimeMapEditor editor;
    editor.attach(level);
    expect(editor.selectObject("editor_lamp_manual"), "runtime editor selects editor instance for silent drag");

    bs3d::RuntimeMapEditor::HistoryState beforeDrag = editor.captureState();
    expect(editor.setSelectedPositionSilent({3.0f, 0.0f, 4.0f}), "silent drag updates editor instance");
    expect(editor.commitCapturedEdit(std::move(beforeDrag)), "runtime editor commits editor instance drag");

    expect(editor.dirty(), "committed editor instance drag marks dirty");
    expect(editor.canUndo(), "committed editor instance drag is undoable");
    expect(editor.buildOverlayDocument().instances.size() == 1,
           "committed editor instance drag remains in overlay instances");
}

void runtimeMapEditorUndoRedoRestoresInstanceAddAndDelete() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::RuntimeMapEditor editor;
    editor.attach(level);

    expect(editor.addInstance("lamp_post_lowpoly", {18.0f, 0.0f, -27.0f}),
           "runtime editor adds instance for undo test");
    expect(findObject(level, "editor_lamp_post_lowpoly_0") != nullptr,
           "runtime editor instance exists before undo");

    expect(editor.undo(), "runtime editor undoes added instance");
    expect(findObject(level, "editor_lamp_post_lowpoly_0") == nullptr,
           "runtime editor undo removes added instance");
    expect(editor.buildOverlayDocument().instances.empty(),
           "runtime editor undo removes added instance from overlay output");

    expect(editor.redo(), "runtime editor redoes added instance");
    expect(findObject(level, "editor_lamp_post_lowpoly_0") != nullptr,
           "runtime editor redo restores added instance");
    expect(editor.canDeleteSelectedOverlayInstance(), "runtime editor redo restores selected editor instance");

    expect(editor.deleteSelectedOverlayInstance(), "runtime editor deletes instance for undo test");
    expect(findObject(level, "editor_lamp_post_lowpoly_0") == nullptr,
           "runtime editor delete removes instance before undo");
    expect(editor.undo(), "runtime editor undoes delete instance");
    expect(findObject(level, "editor_lamp_post_lowpoly_0") != nullptr,
           "runtime editor undo restores deleted instance");
    expect(editor.selectedObjectId() == "editor_lamp_post_lowpoly_0",
           "runtime editor undo restores deleted instance selection");
}

void introLevelV092DecorativeDressingIsCameraSafe() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    int cameraSafeDressing = 0;
    int gameplayBlockingDressing = 0;
    int intentionalPhysicalDressing = 0;
    for (const bs3d::WorldObject& object : level.objects) {
        const bool isDressing = hasTag(object, "shop_identity") ||
                                hasTag(object, "block_facade") ||
                                hasTag(object, "garage_identity") ||
                                hasTag(object, "parking_paint") ||
                                hasTag(object, "trash_dressing") ||
                                hasTag(object, "drive_readability");
        if (!isDressing) {
            continue;
        }

        const bool physicalDressing = hasTag(object, "physical_prop") ||
                                      hasTag(object, "dynamic_prop") ||
                                      hasTag(object, "parking_frame") ||
                                      hasTag(object, "collision_toy");
        if (physicalDressing) {
            ++intentionalPhysicalDressing;
            expect(!hasLayer(object, bs3d::CollisionLayer::CameraBlocker),
                   "physical dressing remains camera-safe in v0.9.7");
            continue;
        }

        if (hasTag(object, "camera_safe") &&
            object.collision.kind == bs3d::WorldCollisionShapeKind::None) {
            ++cameraSafeDressing;
        } else {
            ++gameplayBlockingDressing;
        }
    }

    expect(cameraSafeDressing >= 35, "v0.9.2 dressing is mostly camera-safe no-collision geometry");
    expect(intentionalPhysicalDressing >= 8, "v0.9.7 allows selected dressing to become physical props");
    expect(gameplayBlockingDressing == 0, "non-physical decorative readability dressing does not block gameplay or camera");
}

void introLevelGlassIsExplicitlyNonBlockingDressing() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    int glassCount = 0;
    int taggedGlassCount = 0;
    for (const bs3d::WorldObject& object : level.objects) {
        if (!bs3d::isWorldGlassObject(object)) {
            continue;
        }
        ++glassCount;
        taggedGlassCount += hasTag(object, "glass_surface") ? 1 : 0;
        expect(object.collision.kind == bs3d::WorldCollisionShapeKind::None,
               "glass dressing does not block player/car/camera: " + object.id);
        expect(object.collisionProfile.responseKind == bs3d::CollisionResponseKind::None,
               "glass dressing has explicit no-collision profile: " + object.id);
    }

    expect(glassCount >= 12, "intro level has enough authored vehicle/building-adjacent glass surfaces");
    expect(taggedGlassCount >= 8, "major authored glass surfaces carry glass_surface tags for renderer/debug");
}

void introLevelV093AddsDensityAndGroundTruth() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    int curbCount = 0;
    int parkingFrameCount = 0;
    int blockSideFacadeCount = 0;
    int osiedleClutterCount = 0;
    int verticalReadabilityCount = 0;
    int routeArrowHeadCount = 0;

    for (const bs3d::WorldObject& object : level.objects) {
        curbCount += hasTag(object, "curb_ground_truth") ? 1 : 0;
        parkingFrameCount += hasTag(object, "parking_frame") ? 1 : 0;
        blockSideFacadeCount += hasTag(object, "block_side_facade") ? 1 : 0;
        osiedleClutterCount += hasTag(object, "osiedle_clutter") ? 1 : 0;
        verticalReadabilityCount += hasTag(object, "vertical_readability") ? 1 : 0;
        routeArrowHeadCount += object.assetId == "route_arrow_head" ? 1 : 0;
    }

    expect(curbCount >= 14, "v0.9.3 adds visible curbs to separate road, sidewalk, grass, and parking");
    expect(parkingFrameCount >= 8, "v0.9.3 frames the parking lot with readable edges/clutter");
    expect(blockSideFacadeCount >= 10, "v0.9.3 dresses side/back block facades so they are not blank slabs");
    expect(osiedleClutterCount >= 18, "v0.9.3 adds dense osiedle clutter between major landmarks");
    expect(verticalReadabilityCount >= 8, "v0.9.3 adds vertical readability objects for scale and orientation");
    expect(routeArrowHeadCount >= 4, "v0.9.3 uses arrow-head shapes, not only yellow rectangle route marks");
}

void introLevelV094BalancesScaleAndMaterialCohesion() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    int oversizedCurbs = 0;
    int rectangularRouteBoards = 0;
    int groundPatchCount = 0;
    int asphaltPatchCount = 0;
    int grassPatchCount = 0;
    int shopClusterCount = 0;
    int garageClusterCount = 0;
    int blockClusterCount = 0;
    int trashClusterCount = 0;

    for (const bs3d::WorldObject& object : level.objects) {
        const float curbThickness = std::min(object.scale.x, object.scale.z);
        if (hasTag(object, "curb_ground_truth") &&
            (object.scale.y > 0.105f || curbThickness > 0.145f)) {
            ++oversizedCurbs;
        }
        rectangularRouteBoards += object.assetId == "road_arrow" ? 1 : 0;
        groundPatchCount += hasTag(object, "ground_patch") ? 1 : 0;
        asphaltPatchCount += hasTag(object, "asphalt_patch") ? 1 : 0;
        grassPatchCount += hasTag(object, "grass_patch") ? 1 : 0;
        shopClusterCount += hasTag(object, "shop_cluster") ? 1 : 0;
        garageClusterCount += hasTag(object, "garage_cluster") ? 1 : 0;
        blockClusterCount += hasTag(object, "block_cluster") ? 1 : 0;
        trashClusterCount += hasTag(object, "trash_cluster") ? 1 : 0;
    }

    expect(oversizedCurbs == 0, "v0.9.4 curbs are low/thin enough to read as curbs, not rails");
    expect(rectangularRouteBoards == 0, "v0.9.4 removes yellow rectangular route boards");
    expect(groundPatchCount >= 14, "v0.9.4 breaks up flat asphalt/grass/chodnik surfaces");
    expect(asphaltPatchCount >= 6, "v0.9.4 adds darker asphalt patches");
    expect(grassPatchCount >= 6, "v0.9.4 adds worn grass patches");
    expect(shopClusterCount >= 5, "v0.9.4 clusters detail around the shop");
    expect(garageClusterCount >= 4, "v0.9.4 clusters detail around Rysiek garages");
    expect(blockClusterCount >= 4, "v0.9.4 clusters detail around Block 13");
    expect(trashClusterCount >= 4, "v0.9.4 clusters detail around trash area");
}

void introLevelV095AddsArtKitQualityBaseline() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    int heroAssetCount = 0;
    int artKitV2Count = 0;
    int irregularGroundPatchCount = 0;
    int tintedObjectCount = 0;
    int lowPolyPrimitiveReplacementCount = 0;

    for (const bs3d::WorldObject& object : level.objects) {
        heroAssetCount += hasTag(object, "hero_asset") ? 1 : 0;
        artKitV2Count += hasTag(object, "artkit_v2") ? 1 : 0;
        irregularGroundPatchCount += hasTag(object, "irregular_ground_patch") ? 1 : 0;
        tintedObjectCount += object.hasTintOverride ? 1 : 0;
        lowPolyPrimitiveReplacementCount += object.assetId == "curb_lowpoly" ||
                                            object.assetId == "lamp_post_lowpoly" ||
                                            object.assetId == "trash_bin_lowpoly" ? 1 : 0;
    }

    expect(heroAssetCount >= 3, "v0.9.5 promotes shop, block, and garage into hero assets");
    expect(artKitV2Count >= 18, "v0.9.5 marks improved art-kit objects for visual audit");
    expect(irregularGroundPatchCount >= 14, "v0.9.5 replaces carpet-like ground rectangles with irregular patches");
    expect(tintedObjectCount >= 8, "v0.9.5 supports per-object tint/material variants");
    expect(lowPolyPrimitiveReplacementCount >= 12, "v0.9.5 replaces repeated unit-box street props with low-poly assets");
    expect(level.visualBaselines.size() >= 6, "v0.9.5 exports visual baseline viewpoints");

    const std::vector<std::string> requiredViewpoints{
        "vp_start_bogus",
        "vp_shop_front",
        "vp_parking",
        "vp_garages",
        "vp_road_loop",
        "vp_block_rear"};
    for (const std::string& id : requiredViewpoints) {
        bool found = false;
        for (const bs3d::WorldViewpoint& viewpoint : level.visualBaselines) {
            found = found || viewpoint.id == id;
        }
        expect(found, "v0.9.5 visual baseline exists: " + id);
    }
}

void visualIdentityV2HasDedicatedQABaselineViewpoints() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    const auto findViewpoint = [&](const std::string& id) -> const bs3d::WorldViewpoint* {
        for (const bs3d::WorldViewpoint& viewpoint : level.visualBaselines) {
            if (viewpoint.id == id) {
                return &viewpoint;
            }
        }
        return nullptr;
    };

    const bs3d::WorldViewpoint* shopHero = findViewpoint("vp_shop_zenon_v2");
    const bs3d::WorldViewpoint* blockFront = findViewpoint("vp_block13_front_v2");
    const bs3d::WorldViewpoint* gruzWear = findViewpoint("vp_gruz_wear_v2");

    expect(shopHero != nullptr, "visual identity v2 has a dedicated Zenon hero QA viewpoint");
    expect(blockFront != nullptr, "visual identity v2 has a dedicated Block 13 front QA viewpoint");
    expect(gruzWear != nullptr, "visual identity v2 has a dedicated gruz wear QA viewpoint");

    const bs3d::WorldObject* grille = findObject(level, "shop_rolling_grille");
    const bs3d::WorldObject* intercom = findObject(level, "block13_intercom_panel");
    expect(grille != nullptr && intercom != nullptr, "v2 hero objects exist for QA viewpoint targeting");
    if (shopHero != nullptr && grille != nullptr) {
        expect(bs3d::distanceXZ(shopHero->target, grille->position) <= 1.2f,
               "Zenon v2 QA viewpoint targets the rolling grille/hero frontage");
        expect(bs3d::distanceXZ(shopHero->position, shopHero->target) >= 5.0f,
               "Zenon v2 QA viewpoint frames the whole storefront instead of clipping detail");
    }
    if (blockFront != nullptr && intercom != nullptr) {
        expect(bs3d::distanceXZ(blockFront->target, intercom->position) <= 1.6f,
               "Block 13 v2 QA viewpoint targets the intercom/entry front");
        expect(blockFront->position.z < blockFront->target.z,
               "Block 13 v2 QA viewpoint looks from the front sidewalk");
    }
    if (gruzWear != nullptr) {
        expect(bs3d::distanceXZ(gruzWear->target, level.vehicleStart) <= 1.0f,
               "gruz v2 QA viewpoint targets the parked car");
        expect(bs3d::distanceXZ(gruzWear->position, level.vehicleStart) <= 6.5f,
               "gruz v2 QA viewpoint is close enough to inspect rust and dirt");
    }

    expect(level.visualBaselines.size() >= 9,
           "visual baseline cycle includes original coverage plus v2 identity close-ups");
}

void expandedRewirBaselinesCoverMaterializedWorldRings() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    const auto findViewpoint = [&](const std::string& id) -> const bs3d::WorldViewpoint* {
        for (const bs3d::WorldViewpoint& viewpoint : level.visualBaselines) {
            if (viewpoint.id == id) {
                return &viewpoint;
            }
        }
        return nullptr;
    };

    const bs3d::WorldObject* playground = findObject(level, "playground_court");
    const bs3d::WorldObject* pavilion = findObject(level, "pavilion_row");
    const bs3d::WorldObject* artery = findObject(level, "main_artery_spine");
    const bs3d::WorldObject* garageLane = findObject(level, "garage_lane");
    expect(playground != nullptr, "playground court exists before baseline checks");
    expect(pavilion != nullptr, "pavilion row exists before baseline checks");
    expect(artery != nullptr, "main artery exists before baseline checks");
    expect(garageLane != nullptr, "garage belt lane exists before baseline checks");

    const bs3d::WorldViewpoint* playgroundView = findViewpoint("vp_playground");
    const bs3d::WorldViewpoint* pavilionView = findViewpoint("vp_pavilions");
    const bs3d::WorldViewpoint* arteryView = findViewpoint("vp_main_artery");
    const bs3d::WorldViewpoint* garageBeltView = findViewpoint("vp_garage_belt");

    expect(playgroundView != nullptr, "visual baseline covers materialized playground");
    expect(pavilionView != nullptr, "visual baseline covers materialized pavilions");
    expect(arteryView != nullptr, "visual baseline covers materialized main artery");
    expect(garageBeltView != nullptr, "visual baseline covers materialized garage belt");

    if (playgroundView != nullptr) {
        expect(bs3d::distanceXZ(playgroundView->target, playground->position) <= 2.0f,
               "playground baseline targets the authored playground footprint");
    }
    if (pavilionView != nullptr) {
        expect(bs3d::distanceXZ(pavilionView->target, pavilion->position) <= 2.0f,
               "pavilion baseline targets the authored pavilion row");
    }
    if (arteryView != nullptr) {
        expect(pointInsideVisualFootprint(*artery, arteryView->target, 0.5f),
               "main artery baseline target sits on the authored artery surface");
    }
    if (garageBeltView != nullptr) {
        expect(pointInsideVisualFootprint(*garageLane, garageBeltView->target, 0.5f),
               "garage belt baseline target sits on the side-street lane");
    }

    expect(level.visualBaselines.size() >= 13,
           "visual baseline cycle covers the expanded rewir materialization, not only Ring 0");
}

void introLevelV098GroundPatchesReadAsMaterialsNotBlackArtifacts() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    int checkedGroundPatches = 0;
    for (const bs3d::WorldObject& object : level.objects) {
        if (!hasTag(object, "ground_patch")) {
            continue;
        }

        ++checkedGroundPatches;
        expect(object.hasTintOverride, "ground patches use explicit art-directed tint/material data");
        expect(object.tintOverride.a < 255, "ground patches are decals in the translucent render pass");
        const int luminance = (static_cast<int>(object.tintOverride.r) +
                               static_cast<int>(object.tintOverride.g) +
                               static_cast<int>(object.tintOverride.b)) /
                              3;
        expect(luminance >= 52, "ground patch tint is not a near-black visual artifact: " + object.id);
        expect(object.scale.y <= 0.02f, "ground patch stays thin and does not read as a raised block: " + object.id);
    }

    expect(checkedGroundPatches >= 14, "intro level keeps enough ground material breakup after decal tuning");
}

void introLevelV099DriveSurfacesDoNotClipBuildingsAndCoverRoute() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    std::vector<const bs3d::WorldObject*> driveSurfaces;
    std::vector<const bs3d::WorldObject*> solidBuildings;
    for (const bs3d::WorldObject& object : level.objects) {
        if (object.assetId == "parking_surface" || object.assetId == "road_asphalt") {
            driveSurfaces.push_back(&object);
        }
        if (object.assetId == "block13_core" ||
            object.assetId == "shop_zenona_v3" ||
            object.assetId == "garage_row_v2" ||
            object.assetId == "trash_shelter") {
            solidBuildings.push_back(&object);
        }
    }

    expect(driveSurfaces.size() >= 7, "level has visible asphalt/parking surfaces for the route, not only paint lines");
    for (const bs3d::WorldObject* surface : driveSurfaces) {
        for (const bs3d::WorldObject* building : solidBuildings) {
            expect(!visualFootprintsOverlapXZ(*surface, *building, -0.04f),
                   "drive surface does not clip through building footprint: " +
                       surface->id + " vs " + building->id);
        }
    }

    for (const bs3d::RoutePoint& point : level.driveRoute) {
        bool covered = false;
        for (const bs3d::WorldObject* surface : driveSurfaces) {
            covered = covered || pointInsideVisualFootprint(*surface, point.position, 0.20f);
        }
        expect(covered, "drive route point has visible asphalt/parking under it: " + point.label);
    }

    const bs3d::WorldObject* block = findObject(level, "block13_core");
    expect(block != nullptr, "Block 13 exists for balcony placement validation");
    const float blockFrontZ = block->position.z - block->scale.z * 0.5f;
    for (const bs3d::WorldObject& object : level.objects) {
        if (object.assetId != "balcony_stack") {
            continue;
        }
        expect(object.position.z + object.scale.z * 0.5f <= blockFrontZ + 0.01f,
               "balcony stack sits outside the block face instead of penetrating the wall: " + object.id);
    }
}

void introLevelV096AuthoringSectionsCanBuildTheSameLevel() {
    bs3d::IntroLevelData sectioned;
    bs3d::IntroLevelAuthoring::addCoreLayout(sectioned);
    const std::size_t coreObjectCount = sectioned.objects.size();
    expect(coreObjectCount >= 12, "v0.9.6 core layout section exports major playable masses");
    expect(sectioned.driveRoute.size() >= 5, "v0.9.6 core layout owns drive route points");
    expect(sectioned.missionTriggers.empty(), "v0.9.6 core layout does not own mission triggers");

    bs3d::IntroLevelAuthoring::addDressing(sectioned);
    expect(sectioned.objects.size() > coreObjectCount + 70, "v0.9.6 dressing section adds art-kit detail separately");
    expect(sectioned.zones.empty(), "v0.9.6 dressing section does not own gameplay zones");

    bs3d::IntroLevelAuthoring::addGameplayData(sectioned);
    expect(sectioned.zones.size() >= 5, "v0.9.6 gameplay section exports zones");
    expect(sectioned.landmarks.size() >= 6, "v0.9.6 gameplay section exports landmarks");
    expect(sectioned.visualBaselines.size() >= 6, "v0.9.6 gameplay section exports visual baselines");
    expect(sectioned.missionTriggers.size() == 3, "v0.9.6 gameplay section exports intro triggers");
    expect(!sectioned.props.empty(), "v0.9.6 gameplay section keeps legacy fallback props");

    const bs3d::IntroLevelData built = bs3d::IntroLevelBuilder::build();
    expect(built.objects.size() == sectioned.objects.size(), "builder delegates to the same authoring sections");
    expect(built.zones.size() == sectioned.zones.size(), "builder keeps zone output from authoring sections");
    expect(built.visualBaselines.size() == sectioned.visualBaselines.size(),
           "builder keeps visual baseline output from authoring sections");
}

void introLevelV097UsesExplicitCollisionProfilesForPhysicalProps() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    int physicalProps = 0;
    int cameraSafePhysicalProps = 0;
    int dynamicProps = 0;
    int walkableSurfaces = 0;
    int triggerLikeObjects = 0;

    for (const bs3d::WorldObject& object : level.objects) {
        expect(object.collisionProfile.responseKind != bs3d::CollisionResponseKind::Unspecified,
               "every authored world object has an explicit collision response kind");

        const bool markedPhysical = hasTag(object, "collision_toy") ||
                                    hasTag(object, "physical_prop") ||
                                    hasTag(object, "dynamic_prop") ||
                                    (hasTag(object, "parking_frame") &&
                                     !isDecorativeGroundPatchAsset(object)) ||
                                    object.id == "bogus_bench";
        if (markedPhysical) {
            ++physicalProps;
            expect(object.collision.kind != bs3d::WorldCollisionShapeKind::None,
                   "physical prop has a blocking collision shape: " + object.id);
            if (isPlayerSoftCollisionAsset(object)) {
                expect(hasTag(object, "soft_player_collision"),
                       "thin readability props are explicitly marked as soft for player collision: " + object.id);
                expect(!hasLayer(object, bs3d::CollisionLayer::PlayerBlocker),
                       "thin readability props do not hard-block the player: " + object.id);
            } else {
                expect(hasLayer(object, bs3d::CollisionLayer::PlayerBlocker),
                       "physical prop blocks player queries: " + object.id);
            }
            expect(hasLayer(object, bs3d::CollisionLayer::VehicleBlocker) || object.id == "bogus_bench",
                   "physical prop blocks vehicle unless it is a bench-only obstacle: " + object.id);
            if (hasTag(object, "camera_safe")) {
                ++cameraSafePhysicalProps;
                expect(!hasLayer(object, bs3d::CollisionLayer::CameraBlocker),
                       "small physical props stay camera-safe: " + object.id);
            }
        }

        if (hasTag(object, "dynamic_prop")) {
            ++dynamicProps;
            expect(hasLayer(object, bs3d::CollisionLayer::DynamicProp),
                   "dynamic prop exposes DynamicProp layer: " + object.id);
        }

        if (hasLayer(object, bs3d::CollisionLayer::WalkableSurface)) {
            ++walkableSurfaces;
            expect(object.collision.kind == bs3d::WorldCollisionShapeKind::GroundBox ||
                       object.collision.kind == bs3d::WorldCollisionShapeKind::RampZ,
                   "walkable authored surfaces use ground/ramp shapes");
        }

        if (hasLayer(object, bs3d::CollisionLayer::InteractionTrigger) ||
            hasLayer(object, bs3d::CollisionLayer::WorldEventTrigger)) {
            ++triggerLikeObjects;
            expect(object.collisionProfile.isTrigger, "trigger layer objects are non-solid triggers");
        }
    }

    expect(physicalProps >= 14, "v0.9.7 promotes enough benches/bins/bollards/parking props to physical collision");
    expect(cameraSafePhysicalProps >= 8, "v0.9.7 keeps small physical props from fighting the camera");
    expect(dynamicProps >= 4, "v0.9.7 marks first bins/cardboard/barriers as controlled dynamic props");
    expect(walkableSurfaces >= 3, "v0.9.7 keeps authored walkable surfaces explicit");
    expect(triggerLikeObjects == 0, "v0.9.7 does not accidentally mix visual objects with trigger blockers yet");
}

void introLevelThinReadabilityPropsDoNotCagePlayerNearCars() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    int softPlayerProps = 0;
    for (const bs3d::WorldObject& object : level.objects) {
        if (!isPlayerSoftCollisionAsset(object)) {
            continue;
        }

        ++softPlayerProps;
        expect(hasTag(object, "soft_player_collision"),
               "thin vertical/parking prop declares soft player collision: " + object.id);
        expect(!hasLayer(object, bs3d::CollisionLayer::PlayerBlocker),
               "thin vertical/parking prop cannot trap player movement: " + object.id);
        expect(hasLayer(object, bs3d::CollisionLayer::VehicleBlocker),
               "thin vertical/parking prop still blocks or reacts to cars: " + object.id);
        expect(!hasLayer(object, bs3d::CollisionLayer::CameraBlocker),
               "thin vertical/parking prop remains camera-safe: " + object.id);
    }

    expect(softPlayerProps >= 8, "level has enough thin readability props covered by soft collision policy");
}

void introLevelSmallDynamicPropPolicyMatchesGameplayIntent() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    int cardboardProps = 0;
    int trashBins = 0;
    for (const bs3d::WorldObject& object : level.objects) {
        if (isFragileSmallPropAsset(object)) {
            ++cardboardProps;
            expect(hasTag(object, "breakable_lite"),
                   "cardboard is authored as breakable-lite instead of a platform: " + object.id);
            expect(hasTag(object, "soft_player_collision"),
                   "cardboard is soft for player collision: " + object.id);
            expect(!hasLayer(object, bs3d::CollisionLayer::PlayerBlocker),
                   "cardboard does not hard-block or trap player movement: " + object.id);
            expect(hasLayer(object, bs3d::CollisionLayer::VehicleBlocker),
                   "cardboard still reacts to vehicles: " + object.id);
        }

        if (object.assetId == "trash_bin_lowpoly") {
            ++trashBins;
            expect(hasTag(object, "walkable_prop_top"),
                   "trash bin exposes an authored walkable top: " + object.id);
            expect(hasLayer(object, bs3d::CollisionLayer::PlayerBlocker),
                   "trash bin keeps body collision for player: " + object.id);
            expect(hasLayer(object, bs3d::CollisionLayer::VehicleBlocker),
                   "trash bin keeps vehicle collision: " + object.id);
        }
    }

    expect(cardboardProps >= 3, "level has multiple cardboard props covered by fragile policy");
    expect(trashBins >= 2, "level has trash bins covered by walkable dynamic policy");
}

void introLevelLowConcreteDressingDoesNotTrapPlayer() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();

    int lowTrapProps = 0;
    for (const bs3d::WorldObject& object : level.objects) {
        if (!isLowConcretePlayerTrapAsset(object)) {
            continue;
        }

        ++lowTrapProps;
        expect(hasTag(object, "soft_player_collision"),
               "low concrete/planter dressing is explicitly soft for player: " + object.id);
        expect(!hasLayer(object, bs3d::CollisionLayer::PlayerBlocker),
               "low concrete/planter dressing cannot trap the player: " + object.id);
        expect(hasLayer(object, bs3d::CollisionLayer::VehicleBlocker),
               "low concrete/planter dressing still blocks or reacts to vehicles: " + object.id);
        expect(!hasLayer(object, bs3d::CollisionLayer::CameraBlocker),
               "low concrete/planter dressing stays camera-safe: " + object.id);
    }

    expect(lowTrapProps >= 5, "level has low concrete/planter props covered by trap-safe policy");
}

void propSimulationMovesOnlyControlledDynamicPropsAndPublishesCollision() {
    bs3d::PropSimulationSystem props;
    props.addProp({"trash_bin",
                   {0.0f, 0.0f, 0.0f},
                   {0.8f, 1.0f, 0.7f},
                   0.0f,
                   bs3d::PropPhysicsBehavior::DynamicLite,
                   bs3d::CollisionProfile::dynamicProp(),
                   true});
    bs3d::CollisionProfile cardboardProfile = bs3d::CollisionProfile::vehicleBlocker();
    cardboardProfile.layers |= bs3d::collisionLayerMask(bs3d::CollisionLayer::DynamicProp);
    cardboardProfile.responseKind = bs3d::CollisionResponseKind::BreakableLite;
    props.addProp({"cardboard",
                   {2.0f, 0.0f, 0.0f},
                   {0.6f, 0.42f, 0.5f},
                   0.0f,
                   bs3d::PropPhysicsBehavior::BreakableLite,
                   cardboardProfile,
                   false});
    props.addProp({"bench",
                   {4.0f, 0.0f, 0.0f},
                   {2.0f, 0.7f, 0.5f},
                   0.0f,
                   bs3d::PropPhysicsBehavior::Static,
                   bs3d::CollisionProfile::playerBlocker()});

    expect(props.count() == 3, "prop simulation stores controlled props");
    const bool hitDynamic = props.applyImpulseNear({0.2f, 0.0f, 0.0f}, 1.0f, {3.0f, 0.0f, 0.0f});
    const bool hitCardboard = props.applyPlayerContact({2.2f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, 0.45f);
    const bool hitStatic = props.applyImpulseNear({4.0f, 0.0f, 0.0f}, 1.0f, {3.0f, 0.0f, 0.0f});
    expect(hitDynamic, "dynamic prop accepts impulse near its body");
    expect(hitCardboard, "small cardboard accepts player contact impulse instead of trapping player");
    expect(!hitStatic, "static prop ignores movement impulse");

    props.update(0.5f);
    const bs3d::PropSimulationState* bin = props.find("trash_bin");
    const bs3d::PropSimulationState* cardboard = props.find("cardboard");
    const bs3d::PropSimulationState* bench = props.find("bench");
    expect(bin != nullptr && bench != nullptr, "prop states are findable by id");
    expect(bin->position.x > 0.1f, "dynamic prop moves after impulse");
    expect(cardboard != nullptr && cardboard->position.x > 2.1f, "cardboard moves after player contact");
    expectNear(bench->position.x, 4.0f, 0.001f, "static prop stays put");

    bs3d::WorldCollision collision;
    collision.clear();
    collision.addGroundPlane(0.0f);
    props.publishCollision(collision);
    expect(collision.isCircleBlocked(bin->position, 0.45f, bs3d::CollisionMasks::Vehicle),
           "dynamic prop publishes vehicle-blocking runtime collision");
    expect(collision.isCircleBlocked(bin->position, 0.45f, bs3d::CollisionMasks::Player),
           "trash bin body blocks player at ground height");
    const bs3d::GroundHit binTop = collision.probeGround(bin->position + bs3d::Vec3{0.0f, 1.4f, 0.0f}, 1.0f, 38.0f);
    expect(binTop.hit && binTop.height > 0.8f, "trash bin publishes a walkable top surface");
    expect(!collision.isCircleBlocked(cardboard->position, 0.45f, bs3d::CollisionMasks::Player),
           "cardboard runtime collision stays soft for player");
    const bs3d::GroundHit cardboardTop =
        collision.probeGround(cardboard->position + bs3d::Vec3{0.0f, 1.0f, 0.0f}, 1.0f, 38.0f);
    expect(!cardboardTop.hit || cardboardTop.height < 0.1f, "cardboard does not become a walkable platform");
    expect(collision.isCircleBlocked(bench->position, 0.45f, bs3d::CollisionMasks::Player),
           "static prop publishes player-blocking runtime collision");
}

void propSimulationCanCarryOnlyControlledMovableProps() {
    bs3d::PropSimulationSystem props;
    props.addProp({"cardboard",
                   {0.0f, 0.0f, 0.75f},
                   {0.55f, 0.40f, 0.45f},
                   0.0f,
                   bs3d::PropPhysicsBehavior::BreakableLite,
                   bs3d::CollisionProfile::dynamicProp(),
                   false});
    props.addProp({"bench",
                   {0.0f, 0.0f, 1.65f},
                   {2.0f, 0.7f, 0.5f},
                   0.0f,
                   bs3d::PropPhysicsBehavior::Static,
                   bs3d::CollisionProfile::playerBlocker(),
                   false});

    const bs3d::PropCarryResult picked =
        props.tryBeginCarry({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 1.2f);
    expect(picked.started, "carry starts on a nearby movable prop");
    expect(picked.propId == "cardboard", "carry chooses the movable prop in reach, not the static bench");
    expect(props.carrying(), "prop simulation reports active carry state");

    props.updateCarried({1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
    const bs3d::PropSimulationState* carried = props.find("cardboard");
    expect(carried != nullptr, "carried prop remains findable");
    expect(carried->position.x > 1.65f && carried->position.y > 0.25f,
           "carried prop follows a hands-forward hold position");

    const bs3d::PropCarryResult second =
        props.tryBeginCarry({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 1.2f);
    expect(!second.started, "cannot start carrying a second prop while already carrying one");

    props.dropCarried({2.4f, 0.0f, 0.0f});
    expect(!props.carrying(), "drop releases the carry state");
    carried = props.find("cardboard");
    expect(carried != nullptr && carried->velocity.x > 1.0f,
           "dropped prop keeps a small throw/release velocity");
    expect(carried != nullptr && std::fabs(carried->position.y) < 0.001f,
           "dropped prop snaps back to its authored ground/base height instead of floating at hand height");
}

void propSimulationStopsDynamicPropsAgainstStaticWorld() {
    bs3d::WorldCollision collision;
    collision.clear();
    collision.addGroundPlane(0.0f);
    collision.addBox({1.15f, 0.6f, 0.0f},
                     {0.20f, 1.2f, 2.0f},
                     bs3d::CollisionProfile::solidWorld());

    bs3d::PropSimulationSystem props;
    props.addProp({"crate",
                   {0.0f, 0.0f, 0.0f},
                   {0.45f, 0.45f, 0.45f},
                   0.0f,
                   bs3d::PropPhysicsBehavior::DynamicLite,
                   bs3d::CollisionProfile::dynamicProp(),
                   false});

    props.applyImpulseNear({0.0f, 0.0f, 0.0f}, 1.0f, {8.0f, 0.0f, 0.0f});
    for (int i = 0; i < 12; ++i) {
        props.update(0.10f, collision);
    }

    const bs3d::PropSimulationState* crate = props.find("crate");
    expect(crate != nullptr, "dynamic prop remains tracked after world collision update");
    expect(crate->position.x < 0.65f, "dynamic prop stops before entering the static wall");
    expect(std::fabs(crate->velocity.x) < 0.001f, "dynamic prop loses horizontal velocity when blocked by world");
}

void populateWorldLeavesPhysicalPropCollisionToPropSimulation() {
    bs3d::IntroLevelData level;
    bs3d::CollisionProfile profile = bs3d::CollisionProfile::playerBlocker();
    profile.responseKind = bs3d::CollisionResponseKind::PropStatic;
    level.objects.push_back({"test_bench",
                             "bench",
                             {0.0f, 0.0f, 0.0f},
                             {2.0f, 0.7f, 0.5f},
                             0.0f,
                             {bs3d::WorldCollisionShapeKind::Box, {0.0f, 0.35f, 0.0f}, {2.0f, 0.7f, 0.5f}},
                             bs3d::WorldLocationTag::Block,
                             {"physical_prop"},
                             false,
                             {},
                             profile});

    bs3d::Scene scene;
    bs3d::WorldCollision collision;
    bs3d::IntroLevelBuilder::populateWorld(level, scene, collision);

    expect(scene.objectCount() == 1, "populateWorld still creates a scene object for physical props");
    expect(!collision.isCircleBlocked({0.0f, 0.0f, 0.0f}, 0.45f, bs3d::CollisionMasks::Player),
           "populateWorld does not publish static collision for prop-simulation-owned props");

    bs3d::PropSimulationSystem props;
    props.addPropsFromWorld(level.objects);
    props.publishCollision(collision);
    expect(collision.isCircleBlocked({0.0f, 0.0f, 0.0f}, 0.45f, bs3d::CollisionMasks::Player),
           "PropSimulation publishes the prop collision as the single runtime owner");
}

void propSimulationRejectsStaticCarryTargets() {
    bs3d::PropSimulationSystem props;
    props.addProp({"bench",
                   {0.0f, 0.0f, 0.75f},
                   {2.0f, 0.7f, 0.5f},
                   0.0f,
                   bs3d::PropPhysicsBehavior::Static,
                   bs3d::CollisionProfile::playerBlocker(),
                   false});

    const bs3d::PropCarryResult result =
        props.tryBeginCarry({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 1.2f);
    expect(!result.started, "carry rejects static props even if they are nearby");
    expect(!props.carrying(), "static carry rejection leaves no active carry state");
}

void propSimulationQueriesMovablePushTargetsWithoutMutatingThem() {
    bs3d::PropSimulationSystem props;
    props.addProp({"cardboard",
                   {0.0f, 0.0f, 0.85f},
                   {0.55f, 0.40f, 0.45f},
                   0.0f,
                   bs3d::PropPhysicsBehavior::BreakableLite,
                   bs3d::CollisionProfile::dynamicProp(),
                   false});
    props.addProp({"bench",
                   {0.0f, 0.0f, 0.45f},
                   {2.0f, 0.7f, 0.5f},
                   0.0f,
                   bs3d::PropPhysicsBehavior::Static,
                   bs3d::CollisionProfile::playerBlocker(),
                   false});

    const bs3d::PropSimulationState* before = props.find("cardboard");
    const float beforeX = before != nullptr ? before->position.x : -100.0f;
    const bool hasTarget = props.hasMovablePropNear({0.0f, 0.0f, 0.0f},
                                                    {0.0f, 0.0f, 1.0f},
                                                    1.15f);
    expect(hasTarget, "push query sees the movable prop ahead");
    before = props.find("cardboard");
    expect(before != nullptr && std::fabs(before->position.x - beforeX) < 0.001f,
           "push target query does not move the prop");

    expect(!props.hasMovablePropNear({0.0f, 0.0f, 0.0f},
                                     {0.0f, 0.0f, -1.0f},
                                     1.15f),
           "push query rejects props behind the character");

    bs3d::PropSimulationSystem staticOnly;
    staticOnly.addProp({"bench",
                        {0.0f, 0.0f, 0.45f},
                        {2.0f, 0.7f, 0.5f},
                        0.0f,
                        bs3d::PropPhysicsBehavior::Static,
                        bs3d::CollisionProfile::playerBlocker(),
                        false});
    expect(!staticOnly.hasMovablePropNear({0.0f, 0.0f, 0.0f},
                                          {0.0f, 0.0f, 1.0f},
                                          1.15f),
           "push query rejects static props");
}

void playerCanMoveOnIntroVehicleRoofWithoutWorldOverlapTrap() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::Scene scene;
    bs3d::WorldCollision collision;
    bs3d::IntroLevelBuilder::populateWorld(level, scene, collision);

    bs3d::PropSimulationSystem props;
    props.addPropsFromWorld(level.objects);
    collision.clearDynamic();
    props.publishCollision(collision);

    const float vehicleYaw = level.driveRoute.empty()
                                 ? 0.0f
                                 : bs3d::yawFromDirection(level.driveRoute.front().position - level.vehicleStart);
    collision.addDynamicVehiclePlayerCollision(level.vehicleStart, vehicleYaw);

    bs3d::PlayerMotor motor;
    motor.reset(level.vehicleStart + bs3d::Vec3{0.0f, 1.85f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent idle;
    for (int i = 0; i < 90; ++i) {
        motor.update(idle, collision, 1.0f / 60.0f);
    }
    expect(motor.state().grounded, "player lands on intro vehicle roof in authored level");

    const bs3d::Vec3 directions[] = {
        bs3d::Vec3{1.0f, 0.0f, 0.0f},
        bs3d::Vec3{-1.0f, 0.0f, 0.0f},
        bs3d::Vec3{0.0f, 0.0f, 1.0f},
        bs3d::Vec3{0.0f, 0.0f, -1.0f},
    };

    bool foundEscapeDirection = false;
    for (const bs3d::Vec3& direction : directions) {
        bs3d::PlayerMotor probe = motor;
        bs3d::PlayerInputIntent move;
        move.moveDirection = direction;
        const bs3d::Vec3 start = probe.state().position;
        for (int i = 0; i < 30; ++i) {
            probe.update(move, collision, 1.0f / 60.0f);
        }
        if (bs3d::distanceXZ(probe.state().position, start) > 0.45f) {
            foundEscapeDirection = true;
        }
    }

    expect(foundEscapeDirection,
           "authored vehicle parking spot does not pin player on roof against nearby world blockers");
}

void introVehicleCanDriveOutOfAuthoredParkingSpot() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::Scene scene;
    bs3d::WorldCollision collision;
    bs3d::IntroLevelBuilder::populateWorld(level, scene, collision);

    bs3d::DriveRouteGuide route;
    route.reset(level.driveRoute);

    bs3d::VehicleController staticOnlyVehicle;
    staticOnlyVehicle.reset(level.vehicleStart, route.yawFrom(level.vehicleStart));
    bs3d::InputState gas;
    gas.accelerate = true;
    {
        const bs3d::Vec3 previous = staticOnlyVehicle.position();
        staticOnlyVehicle.update(gas, 1.0f / 60.0f);
        const bs3d::VehicleCollisionResult resolved =
            collision.resolveVehicleCapsule(previous,
                                            staticOnlyVehicle.position(),
                                            staticOnlyVehicle.yawRadians(),
                                            bs3d::VehicleCollisionConfig{});
        if (resolved.hit) {
            std::string blocker = "unknown";
            for (const bs3d::WorldObject& object : level.objects) {
                if ((object.collisionProfile.layers & bs3d::CollisionMasks::Vehicle) == 0) {
                    continue;
                }
                bs3d::WorldCollision single;
                single.clear();
                single.addGroundPlane(0.0f);
                addObjectCollisionForTest(object, single);
                const bs3d::VehicleCollisionResult singleResult =
                    single.resolveVehicleCapsule(previous,
                                                 staticOnlyVehicle.position(),
                                                 staticOnlyVehicle.yawRadians(),
                                                 bs3d::VehicleCollisionConfig{});
                if (singleResult.hit) {
                    blocker = object.id;
                    break;
                }
            }
            throw std::runtime_error("intro vehicle start is blocked by static world collision: " + blocker);
        }
    }

    bs3d::PropSimulationSystem props;
    props.addPropsFromWorld(level.objects);
    props.publishCollision(collision);

    bs3d::VehicleController vehicle;
    vehicle.reset(level.vehicleStart, route.yawFrom(level.vehicleStart));

    const bs3d::Vec3 start = vehicle.position();
    bool blockedEveryFrame = true;

    for (int i = 0; i < 90; ++i) {
        const bs3d::Vec3 previous = vehicle.position();
        vehicle.update(gas, 1.0f / 60.0f);
        const bs3d::Vec3 simulated = vehicle.position();
        const bs3d::VehicleCollisionResult resolved =
            collision.resolveVehicleCapsule(previous,
                                            simulated,
                                            vehicle.yawRadians(),
                                            bs3d::VehicleCollisionConfig{});
        if (!resolved.hit && bs3d::distanceSquaredXZ(simulated, resolved.position) <= 0.0004f) {
            blockedEveryFrame = false;
        }
        vehicle.setPosition(resolved.position);
    }

    expect(!blockedEveryFrame, "intro vehicle is not blocked by authored collision every frame");
    expect(bs3d::distanceXZ(vehicle.position(), start) > 1.5f,
           "holding W moves the intro gruz out of its authored parking spot");
}

void vehicleExitResolverAvoidsBlockedPreferredSide() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({-1.75f, 0.55f, 0.0f},
                     {0.45f, 1.1f, 4.2f},
                     bs3d::CollisionProfile::solidWorld());

    const bs3d::Vec3 vehiclePosition{};
    const float vehicleYaw = 0.0f;
    const bs3d::VehicleExitResolution exit =
        bs3d::resolveVehicleExitPosition(collision,
                                         vehiclePosition,
                                         vehicleYaw,
                                         bs3d::screenRightFromYaw(vehicleYaw));

    expect(exit.found, "vehicle exit resolver finds another side when preferred side is blocked");
    expect(exit.position.x > 1.35f, "vehicle exit resolver chooses the free opposite side");

    bs3d::WorldCollision validation = collision;
    validation.addDynamicVehiclePlayerCollision(vehiclePosition, vehicleYaw);
    bs3d::CharacterCollisionConfig config;
    const bs3d::CharacterCollisionResult result =
        validation.resolveCharacter(exit.position, exit.position, config);
    expect(!result.hitWall && !result.blockedByStep && !result.blockedBySlope,
           "resolved vehicle exit position is not inside car or world blockers");
}

void chaseVehicleRuntimeKeepsPursuerOutOfTargetVehicleBody() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::ChaseVehicleRuntimeConfig config;
    config.minimumCenterSeparation = 3.65f;
    const bs3d::Vec3 target{0.0f, 0.0f, 0.0f};
    const bs3d::ChaseVehicleStep step =
        bs3d::advanceChaseVehicle({0.0f, 0.0f, -7.0f},
                                  0.0f,
                                  target,
                                  0.0f,
                                  14.0f,
                                  1.0f,
                                  collision,
                                  config);

    expect(step.distanceToTarget >= config.minimumCenterSeparation - 0.05f,
           "chase vehicle stops before embedding into target gruz body");
    expect(step.separatedFromTarget || step.collision.hit,
           "chase vehicle reports target separation or vehicle collision when it reaches the gruz");

    bs3d::WorldCollision validation;
    validation.addGroundPlane(0.0f);
    validation.addDynamicVehiclePlayerCollision(target, 0.0f);
    const bs3d::VehicleCollisionResult overlap =
        validation.resolveVehicleCapsule(step.position,
                                         step.position,
                                         step.yawRadians,
                                         bs3d::VehicleCollisionConfig{});
    expect(!overlap.hit, "resolved chase vehicle is not inside the target vehicle collision");
}

void chaseVehicleRuntimeResolvesPursuerAgainstWorldBlockers() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({0.0f, 1.0f, -2.5f},
                     {5.0f, 2.0f, 0.45f},
                     bs3d::CollisionProfile::solidWorld());

    const bs3d::ChaseVehicleStep step =
        bs3d::advanceChaseVehicle({0.0f, 0.0f, -6.0f},
                                  0.0f,
                                  {0.0f, 0.0f, 4.0f},
                                  0.0f,
                                  8.0f,
                                  1.0f,
                                  collision);

    expect(step.position.z < -2.5f,
           "chase vehicle remains before the blocker after collision resolution z=" +
               std::to_string(step.position.z));
}

void npcBodyLiteBlocksPlayerButKeepsInteractionRange() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::CollisionProfile npcProfile = bs3d::CollisionProfile::playerBlocker();
    npcProfile.ownerId = 8101;
    const bs3d::Vec3 npcPosition{0.0f, 0.0f, 0.0f};
    collision.addDynamicBox(npcPosition + bs3d::Vec3{0.0f, 0.85f, 0.0f},
                            {0.72f, 1.70f, 0.72f},
                            npcProfile);

    bs3d::CharacterCollisionConfig config;
    const bs3d::CharacterCollisionResult blocked =
        collision.resolveCharacter({1.3f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, config);
    expect(blocked.hitWall, "NPC body lite blocks player overlap at ground height");
    expect(bs3d::distanceXZ(blocked.position, npcPosition) < 2.2f,
           "NPC body still leaves player inside normal interaction prompt range");
}

void chaseAiTargetsRearOffsetInsteadOfVehicleCenter() {
    bs3d::ChaseAiRuntimeState state;
    bs3d::ChaseAiInput input;
    input.pursuerPosition = {0.0f, 0.0f, -12.0f};
    input.targetPosition = {0.0f, 0.0f, 0.0f};
    input.targetYawRadians = 0.0f;
    input.elapsedSeconds = 0.1f;

    const bs3d::ChaseAiCommand command = bs3d::updateChaseAiRuntime(state, input);

    expect(command.mode == bs3d::ChaseAiMode::Pursue, "chase AI starts in pursue mode");
    expect(bs3d::distanceXZ(command.aimPoint, input.targetPosition) > 3.0f,
           "chase AI aims behind/around the gruz instead of its center");
    expect(command.aimPoint.z < input.targetPosition.z - 3.0f,
           "chase AI aims at a rear offset so the patrol follows instead of spearing the vehicle center");
    expect(std::fabs(command.aimPoint.x - input.targetPosition.x) > 1.0f,
           "chase AI includes a side offset for imperfect parking-lot pressure");
}

void chaseAiContactRecoveryBacksOffAfterVehicleContact() {
    bs3d::ChaseAiRuntimeState state;
    bs3d::ChaseAiInput input;
    input.pursuerPosition = {0.0f, 0.0f, -2.8f};
    input.targetPosition = {0.0f, 0.0f, 0.0f};
    input.targetYawRadians = 0.0f;
    input.collisionHit = true;
    input.deltaSeconds = 1.0f / 60.0f;

    const bs3d::ChaseAiCommand command = bs3d::updateChaseAiRuntime(state, input);

    expect(command.mode == bs3d::ChaseAiMode::ContactRecover, "chase AI enters contact recovery after collision");
    expect(command.contactRecoverActive, "contact recovery is exposed to caught logic");
    expect(!command.lineOfSight, "contact recovery suppresses clean caught line-of-sight");
    expect(command.speedScale < 0.6f, "contact recovery slows the patrol before it re-acquires");
    expect(command.aimPoint.z < input.pursuerPosition.z - 2.5f,
           "contact recovery aims away from the target vehicle instead of pushing through it");
}

void introChaseAiCanBeEscapedBySustainedCleanDriving() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::ChaseSystem chase;
    bs3d::ChaseAiRuntimeState aiState;
    bs3d::ChaseVehicleStep lastStep;
    bs3d::Vec3 playerVehicle{0.0f, 0.0f, 0.0f};
    bs3d::Vec3 chaser{0.0f, 0.0f, -12.0f};
    bs3d::Vec3 chaserVelocity{};
    float playerYaw = 0.0f;
    float chaserYaw = 0.0f;

    chase.start(chaser, 0.0f);
    bs3d::ChaseResult result = bs3d::ChaseResult::Running;
    constexpr float Dt = 1.0f / 60.0f;
    for (int i = 0; i < 420 && result == bs3d::ChaseResult::Running; ++i) {
        const float elapsed = static_cast<float>(i) * Dt;
        const bs3d::Vec3 playerVelocity = bs3d::forwardFromYaw(playerYaw) * 13.5f;
        playerVehicle = playerVehicle + playerVelocity * Dt;

        bs3d::ChaseAiInput input;
        input.pursuerPosition = chaser;
        input.targetPosition = playerVehicle;
        input.targetVelocity = playerVelocity;
        input.targetYawRadians = playerYaw;
        input.deltaSeconds = Dt;
        input.elapsedSeconds = elapsed;
        input.collisionHit = lastStep.collision.hit;
        input.separatedFromTarget = lastStep.separatedFromTarget;
        const bs3d::ChaseAiCommand command = bs3d::updateChaseAiRuntime(aiState, input);

        bs3d::ChaseVehicleRuntimeConfig config;
        config.hasPursuitTarget = true;
        config.pursuitTarget = command.aimPoint;
        const float followSpeed = chase.pursuerFollowSpeed(bs3d::distanceXZ(playerVehicle, chaser), 0.86f) *
                                  command.speedScale;
        const bs3d::Vec3 previousChaser = chaser;
        lastStep = bs3d::advanceChaseVehicle(chaser,
                                             chaserYaw,
                                             playerVehicle,
                                             playerYaw,
                                             followSpeed,
                                             Dt,
                                             collision,
                                             config);
        chaser = lastStep.position;
        chaserYaw = lastStep.yawRadians;
        chaserVelocity = (chaser - previousChaser) / Dt;

        bs3d::ChaseUpdateContext chaseContext;
        chaseContext.playerPosition = playerVehicle;
        chaseContext.playerVelocity = playerVelocity;
        chaseContext.pursuerPosition = chaser;
        chaseContext.pursuerVelocity = chaserVelocity;
        chaseContext.lineOfSight = command.lineOfSight;
        chaseContext.contactRecoverActive = command.contactRecoverActive;
        result = chase.updateWithContext(chaseContext, Dt);
    }

    expect(result == bs3d::ChaseResult::Escaped,
           "intro chase AI is passable when the player keeps clean speed and opens a gap");
}

void visualBaselineDebugCyclesThroughAuthoredViewpoints() {
    const bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::VisualBaselineDebug debug;

    expect(!debug.active(), "visual baseline debug starts inactive");
    expect(debug.activeViewpoint(level.visualBaselines) == nullptr, "inactive debug view has no camera override");

    debug.cycle(level.visualBaselines);
    expect(debug.active(), "first cycle enables first baseline view");
    expect(debug.activeIndex() == 0, "first cycle selects index zero");
    const bs3d::WorldViewpoint* first = debug.activeViewpoint(level.visualBaselines);
    expect(first != nullptr && first->id == "vp_start_bogus", "first baseline is start/Bogus view");

    for (std::size_t i = 1; i < level.visualBaselines.size(); ++i) {
        debug.cycle(level.visualBaselines);
    }
    expect(debug.active(), "last authored baseline is still active before wrap");
    expect(debug.activeIndex() == static_cast<int>(level.visualBaselines.size() - 1),
           "cycle reaches the final authored baseline");

    debug.cycle(level.visualBaselines);
    expect(!debug.active(), "cycle after final baseline returns to live gameplay camera");
    expect(debug.activeIndex() == -1, "inactive baseline index is -1");
}

void characterPosePreviewCyclesCatalogWithoutMutatingGameplay() {
    bs3d::CharacterPosePreview preview;
    expect(!preview.active(), "character pose preview starts inactive");
    expect(preview.count() == static_cast<int>(bs3d::characterPoseCatalog().size()),
           "character pose preview covers the runtime pose catalog");

    preview.toggle();
    expect(preview.active(), "character pose preview toggles on");
    expect(preview.activePose() == bs3d::characterPoseCatalog().front(),
           "first preview pose is the first catalog pose");

    preview.next();
    expect(preview.activeIndex() == 1, "preview next advances active index");
    preview.previous();
    expect(preview.activeIndex() == 0, "preview previous returns to first index");

    preview.toggle();
    expect(!preview.active(), "character pose preview toggles off without changing gameplay state");
}

void characterPosePreviewBuildsPoseSpecificState() {
    bs3d::CharacterPosePreview preview;
    const bs3d::PlayerPresentationState punch = preview.previewStateFor(bs3d::CharacterPoseKind::Punch);
    expect(punch.poseKind == bs3d::CharacterPoseKind::Punch, "preview state preserves selected pose");
    expect(punch.punchAmount > 0.9f, "punch preview drives punch channel");
    expect(punch.dodgeAmount == 0.0f, "punch preview does not drive dodge channel");

    const bs3d::PlayerPresentationState standVehicle = preview.previewStateFor(bs3d::CharacterPoseKind::StandVehicle);
    expect(standVehicle.balanceAmount > 0.9f, "stand-vehicle preview drives balance channel");

    const bs3d::PlayerPresentationState panic = preview.previewStateFor(bs3d::CharacterPoseKind::PanicSprint);
    expect(panic.panicFlail > 0.9f, "panic sprint preview drives panic flail");
    expect(panic.locomotionAmount > 0.7f, "panic sprint preview still looks like locomotion");
}

void missionRuntimeBridgeMapsShopWalkTriggerToStoryAndEvent() {
    bs3d::DialogueSystem dialogue;
    bs3d::MissionController mission(dialogue);
    bs3d::StoryState story;
    bs3d::GameFeedback feedback;
    bs3d::WorldEventEmitterCooldowns cooldowns;
    bs3d::MissionRuntimeBridge bridge;

    mission.start();

    bs3d::WorldEventType emittedType = bs3d::WorldEventType::PublicNuisance;
    bs3d::Vec3 emittedPosition{};
    float emittedIntensity = 0.0f;
    std::string emittedSource;
    int emissionCount = 0;

    bs3d::MissionRuntimeBridgeContext context{
        mission,
        story,
        feedback,
        cooldowns,
        bs3d::Vec3{4.0f, 0.0f, 5.0f},
        [&](bs3d::WorldEventType type, bs3d::Vec3 position, float intensity, const std::string& source) {
            emittedType = type;
            emittedPosition = position;
            emittedIntensity = intensity;
            emittedSource = source;
            ++emissionCount;
        }};

    const bs3d::MissionTriggerResult trigger{
        true,
        "shop_walk_intro",
        bs3d::MissionTriggerAction::ShopVisitedOnFoot,
        bs3d::Vec3{18.0f, 0.0f, -18.0f},
        4.8f};

    const bs3d::MissionRuntimeBridgeResult result = bridge.handleTrigger(trigger, context);

    expect(result.handled, "shop walk trigger is handled");
    expect(mission.phase() == bs3d::MissionPhase::ReturnToBench, "shop walk trigger advances mission");
    expect(story.visitedShopOnFoot && story.shopTroubleSeen, "shop walk trigger updates story flags");
    expect(feedback.hudPulse() > 0.0f, "shop walk trigger pulses HUD feedback");
    expect(emissionCount == 1, "shop walk trigger emits one world event");
    expect(emittedType == bs3d::WorldEventType::ShopTrouble, "shop walk trigger emits ShopTrouble");
    expectNear(emittedPosition.x, trigger.position.x, 0.001f, "shop walk event uses trigger position x");
    expectNear(emittedIntensity, 0.30f, 0.001f, "shop walk event uses low intensity");
    expect(emittedSource == "shop_walk_intro", "shop walk event source is stable");
}

void missionRuntimeBridgeMapsVehicleAndDropoffTriggers() {
    bs3d::DialogueSystem dialogue;
    bs3d::MissionController mission(dialogue);
    bs3d::StoryState story;
    bs3d::GameFeedback feedback;
    bs3d::WorldEventEmitterCooldowns cooldowns;
    bs3d::MissionRuntimeBridge bridge;

    mission.start();
    mission.onShopVisitedOnFoot();
    mission.onReturnedToBogus();
    story.onReturnedToBogus();
    mission.onPlayerEnteredVehicle();

    int emissionCount = 0;
    std::string emittedSource;
    float emittedIntensity = 0.0f;
    const bs3d::Vec3 vehiclePosition{17.0f, 0.0f, -17.0f};
    bs3d::MissionRuntimeBridgeContext context{
        mission,
        story,
        feedback,
        cooldowns,
        vehiclePosition,
        [&](bs3d::WorldEventType, bs3d::Vec3, float intensity, const std::string& source) {
            emittedIntensity = intensity;
            emittedSource = source;
            ++emissionCount;
        }};

    const bs3d::MissionRuntimeBridgeResult vehicleResult = bridge.handleTrigger(
        {true,
         "shop_vehicle_intro",
         bs3d::MissionTriggerAction::ShopReachedByVehicle,
         bs3d::Vec3{18.0f, 0.0f, -18.0f},
         5.0f},
        context);

    expect(vehicleResult.handled, "vehicle shop trigger is handled");
    expect(mission.phase() == bs3d::MissionPhase::ChaseActive, "vehicle shop trigger starts chase phase");
    expect(mission.consumeChaseWanted(), "vehicle shop trigger sets chase wanted flag");
    expect(emissionCount == 1, "vehicle shop trigger emits one event");
    expectNear(emittedIntensity, 0.85f, 0.001f, "vehicle shop trigger uses strong intensity");
    expect(emittedSource == "shop_mission", "vehicle shop event source is stable");

    mission.onChaseEscaped();
    const bs3d::MissionRuntimeBridgeResult dropoffResult = bridge.handleTrigger(
        {true,
         "parking_dropoff_intro",
         bs3d::MissionTriggerAction::DropoffReached,
         bs3d::Vec3{-12.0f, 0.0f, -10.0f},
         5.0f},
        context);

    expect(dropoffResult.handled, "dropoff trigger is handled");
    expect(dropoffResult.completed, "dropoff trigger reports completion");
    expect(mission.phase() == bs3d::MissionPhase::Completed, "dropoff trigger completes mission");
    expect(story.introCompleted, "dropoff trigger updates story completion");
}

void missionOutcomeTriggerResolvesDataAuthoredObjectOutcome() {
    bs3d::MissionData missionData;
    missionData.loaded = true;
    missionData.phases.push_back({"WalkToShop", "Sprawdz drzwi Zenona.", "outcome:shop_door_checked"});

    const bs3d::MissionTriggerResult trigger = bs3d::missionOutcomeTriggerForCurrentPhase(
        missionData,
        bs3d::MissionPhase::WalkToShop,
        "shop_door_checked",
        bs3d::Vec3{18.0f, 0.0f, -18.0f},
        2.2f);

    expect(trigger.triggered, "data-authored object outcome resolves to mission trigger");
    expect(trigger.id == "outcome:shop_door_checked", "outcome trigger keeps data-authored trigger id");
    expect(trigger.action == bs3d::MissionTriggerAction::ShopVisitedOnFoot,
           "WalkToShop outcome maps to existing shop visit mission action");
    expectNear(trigger.position.x, 18.0f, 0.001f, "outcome trigger preserves object position x");
    expectNear(trigger.radius, 2.2f, 0.001f, "outcome trigger preserves object radius");

    const bs3d::MissionTriggerResult wrongOutcome = bs3d::missionOutcomeTriggerForCurrentPhase(
        missionData,
        bs3d::MissionPhase::WalkToShop,
        "garage_door_checked_garage_door_0",
        bs3d::Vec3{},
        1.0f);
    expect(!wrongOutcome.triggered, "unmatched outcome does not trigger mission data");
}

void objectOutcomeCatalogResolvesWorldEventsForEnrichedOutcomes() {
    using namespace bs3d;
    ObjectOutcomeCatalogData catalog;
    catalog.loaded = true;

    ObjectOutcomeData shopDoor;
    shopDoor.id = "shop_door_checked";
    shopDoor.worldEvent = ObjectOutcomeWorldEventData{WorldEventType::PublicNuisance, 0.12f, 4.0f};
    catalog.outcomes.push_back(shopDoor);

    ObjectOutcomeData garageDoor;
    garageDoor.idPattern = "garage_door_checked_*";
    garageDoor.worldEvent = ObjectOutcomeWorldEventData{WorldEventType::PublicNuisance, 0.16f, 3.0f};
    catalog.outcomes.push_back(garageDoor);

    ObjectOutcomeData glassPeek;
    glassPeek.idPattern = "glass_peeked_*";
    glassPeek.worldEvent = ObjectOutcomeWorldEventData{WorldEventType::ShopTrouble, 0.10f, 5.0f};
    catalog.outcomes.push_back(glassPeek);

    ObjectOutcomeData quietRead;
    quietRead.idPattern = "parking_sign_read_*";
    catalog.outcomes.push_back(quietRead);

    WorldObjectInteractionAffordance shopDoorAffordance;
    shopDoorAffordance.outcomeId = "shop_door_checked";
    shopDoorAffordance.position = {18.0f, 0.0f, -18.0f};
    const auto shopEvent = worldEventForObjectAffordance(shopDoorAffordance, &catalog);
    expect(shopEvent.has_value(), "catalog resolves worldEvent for shop door");
    expect(shopEvent->type == WorldEventType::PublicNuisance, "shop door event is PublicNuisance");
    expectNear(shopEvent->intensity, 0.12f, 0.001f, "shop door event intensity from catalog");
    expectNear(shopEvent->cooldownSeconds, 4.0f, 0.001f, "shop door event cooldown from catalog");

    WorldObjectInteractionAffordance garageAffordance;
    garageAffordance.outcomeId = "garage_door_checked_garage_door_0";
    garageAffordance.position = {};
    const auto garageEvent = worldEventForObjectAffordance(garageAffordance, &catalog);
    expect(garageEvent.has_value(), "catalog resolves worldEvent for garage door via pattern");
    expect(garageEvent->type == WorldEventType::PublicNuisance, "garage door event is PublicNuisance");

    WorldObjectInteractionAffordance glassAffordance;
    glassAffordance.outcomeId = "glass_peeked_window_shop_0";
    glassAffordance.position = {};
    const auto glassEvent = worldEventForObjectAffordance(glassAffordance, &catalog);
    expect(glassEvent.has_value(), "catalog resolves worldEvent for glass peek via pattern");
    expect(glassEvent->type == WorldEventType::ShopTrouble, "glass peek event is ShopTrouble");

    WorldObjectInteractionAffordance quietAffordance;
    quietAffordance.outcomeId = "parking_sign_read_sign_0";
    quietAffordance.position = {};
    const auto quietEvent = worldEventForObjectAffordance(quietAffordance, &catalog);
    expect(!quietEvent.has_value(), "catalog returns no worldEvent for quiet outcome without one");

    const auto nullCatalogEvent = worldEventForObjectAffordance(shopDoorAffordance, nullptr);
    expect(!nullCatalogEvent.has_value(), "null catalog returns no worldEvent");
}

} // namespace

int main() {
    try {
        assetRegistryLoadsManifestAndReportsMissingAssets();
        assetRegistryValidatesManifestFilesAndModelLoadingReportsWarnings();
        shippedObjFacesStayWithinRaylibTinyObjLimits();
        shippedModelGeometryMatchesRuntimeAuthoringContract();
        defaultWorldPresentationStyleSupportsReadableGrochowLook();
        worldAtmosphereBandsFollowCameraAndStaySubtle();
        hudPanelLayoutKeepsGameplayPanelsSeparated();
        hudMeasuredLayoutUsesBackendNeutralTextMetrics();
        gitignoreKeepsAuthoredObjAssetsTrackable();
        raylibContainmentKeepsDataAndAssetHeadersBackendAgnostic();
        worldDataLoaderReadsRuntimeWorldAndMissionSchema();
        worldDataMissionDefinesCompletePlayableVerticalSlice();
        worldDataLoaderParsesJsonSchemaWithoutFieldOrderCoupling();
        worldDataLoaderAppliesMissionPhaseLinesToMissionController();
        objectAffordanceWorldEventsPreferLoadedOutcomeData();
        objectAffordancesPreferLoadedOutcomeText();
        worldDataMissionDialogueUsesCanonActors();
        runtimeDevToolsAreBuildGatedAndHudHelpIsReleaseSafe();
        hudTextLayoutWrapsObjectiveAndControlsToSafeArea();
        hudTextLayoutSplitsLongTokensBeforeTheyLeaveSafeArea();
        hudPanelsStayInsideNarrowScreens();
        vehicleDriverRenderUsesSeatedPartialBody();
        worldRenderListCullsDistantOpaqueAndGlassObjects();
        worldRenderListSeparatesTranslucentDressingFromOpaquePass();
        worldRenderListSortsAllTransparentWorldObjectsTogether();
        worldRenderListUsesManifestAlphaWithoutTintOverride();
        worldRenderListUsesAssetFallbackBoundsForCulling();
        memoryHotspotDebugMarkersPreserveRewirIdentityAndScale();
        rewirPressureDebugMarkersExposePatrolInterestRadius();
        modelLoadWarningsAreFailFastInDevQualityGate();
        propSimulationSyncUsesIndexedLookupForLargeWorld();
        chaseAiUsesSensorBlackboardPatrolSearchAndRecoverStates();
        chaseAiPatrolUsesRewirPressureInterestWhenTargetIsUnknown();
        introLevelTextDoesNotContainEncodingArtifacts();
        characterArtModelMatchesControllerFirstContract();
        characterArtModelDefinesAnimationRolesForRuntimePose();
        characterArtModelLeavesSafetyMarginInsideGameplayCapsule();
        characterArtModelHasReadableOsiedleSilhouetteLandmarks();
        characterArtModelV2BreaksToyBlockSilhouette();
        characterArtPaletteKeepsCustomizableReadableMaterials();
        characterVisualCatalogDefinesReadableNpcProfiles();
        characterDebugCapsuleGeometryMatchesControllerContract();
        vehicleArtModelSpecMatchesRuntimeScaleAndWheelContract();
        vehicleCabinUsesPanelGlassTechnology();
        vehicleCabinHasCoherentRoofGlassAndPillars();
        vehicleLightVisualStateCommunicatesGruzUseAndWear();
        generatedVehicleGltfContainsNamedPartsMaterialsAndColliders();
        vehicleArtModelV2CommunicatesDirtyRustyGruz();
        generatedVehicleGltfContainsGruzV2WearMaterials();
        glassRenderPolicySeparatesTransparentWorldAndVehicleGlass();
        glassRenderPolicySortsWorldGlassBackToFront();
        glassCrackPolicyIsOptionalAndImpactDriven();
        testHelpersRecognizeOrientedBoxCollisionFootprints();
        introLevelBuilderExportsMissionRuntimeData();
        introLevelParkingDropoffAndPaintStayOnParkingLot();
        introLevelExportsCompressedGrochowExpansionSkeleton();
        introLevelExportsDistrictExpansionRoutePlans();
        introLevelBuildsDistrictPlanDebugOverlay();
        introLevelMaterializesMainArteryAsDriveableExpansion();
        introLevelPavilionsMarketAnchorsMatchMaterializedPavilionStrip();
        introLevelDistrictAnchorsStayOnMaterializedObjects();
        introLevelFlatDriveSurfacesUseSeparatedRenderHeights();
        introLevelMainArteryExpansionHasReadableRouteGuidance();
        introLevelMainArteryRouteIsVehicleTraversableByGruz();
        introLevelFutureDistrictRoutesAreVehicleTraversableByGruz();
        introLevelVehicleRoutesKeepClearanceFromAuthoredObjectFootprints();
        editorOverlayDataDefaultsAreSafe();
        editorOverlayCodecParsesValidOverlay();
        editorOverlayCodecRejectsBadSchema();
        editorOverlayCodecRejectsTrailingGarbage();
        editorOverlayCodecRoundTripsMinimalDocument();
        editorOverlayApplyOverridesExistingObject();
        editorOverlayApplyAppendsNewManifestInstance();
        editorOverlayApplyRejectsDuplicateInstanceId();
        introLevelBuilderExportsDistinctRewirMemoryZones();
        introLevelBuilderExportsParagonMissionInteractionPoints();
        introLevelBuilderExportsGarageServiceInteractionPoint();
        introLevelBuilderExportsCaretakerServiceInteractionPoint();
        introLevelBuilderPopulatesLocalServiceInteractionsFromCatalog();
        introLevelBuilderPopulatesLocalRewirFavorInteractionsFromCatalog();
        introLevelBuilderExportsParkingServiceInteractionPoint();
        introLevelBuilderExportsRoadLoopServiceInteractionPoint();
        introLevelWorldObjectsAreArtKitReady();
        introLevelVisualIdentityV2DressesZenonAndBlock13HeroRead();
        introLevelVisualIdentityV2AddsLivedInMicroBreakup();
        worldObjectInteractionsExposeReadableLowPriorityAffordances();
        visualIdentityHeroDetailsExposeCorrectObjectAffordances();
        objectAffordancesCanFeedPrzypalAndWorldMemoryWhenNoisy();
        introLevelV092HasReadableMissionTargetsAndDriveRoute();
        introLevelSolidWorldObjectsUseBaseAnchoredCollision();
        introLevelBoundaryWallsShowTheirWholeBlockingFootprint();
        introLevelGarageBeltLaneKeepsSixMeterDriveableScale();
        introLevelBuildWithManifestEnforcesRuntimeAssetContract();
        introLevelPopulateWorldPreservesAuthoredObjectYaw();
        runtimeWorldRenderingUsesWorldObjectsAsSingleSourceOfTruth();
        runtimeWorldRenderPassOrderKeepsTransparentAfterOpaqueDynamics();
        worldDataApplyRebuildsDerivedIntroLevelAuthoring();
        worldDataCatalogAppliesEditorOverlayAfterBaseMap();
        runtimeMapEditorEditsSelectedObjectAndTracksDirtyState();
        runtimeMapEditorAddsManifestInstance();
        runtimeMapEditorAddsDefinitionInstanceWithManifestTags();
        runtimeEditorAssetFilterMatchesManifestMetadata();
        runtimeMapEditorComputesPlacementInFrontOfCamera();
        runtimeEditorOverlayPathUsesDataRoot();
        runtimeMapEditorEditsSelectedMetadata();
        runtimeMapEditorBuildsOverlayForEditedObjects();
        runtimeMapEditorPreservesLoadedBaseOverridesOnSave();
        runtimeMapEditorOnlyAllowsDeletingOverlayInstances();
        runtimeMapEditorUndoRedoRestoresBaseEditsAndOverlayTracking();
        runtimeMapEditorCommitsSilentDragAsUndoableEdit();
        runtimeMapEditorCommitsSilentDragForEditorInstance();
        runtimeMapEditorUndoRedoRestoresInstanceAddAndDelete();
        introLevelV092DecorativeDressingIsCameraSafe();
        introLevelGlassIsExplicitlyNonBlockingDressing();
        introLevelV093AddsDensityAndGroundTruth();
        introLevelV094BalancesScaleAndMaterialCohesion();
        introLevelV095AddsArtKitQualityBaseline();
        visualIdentityV2HasDedicatedQABaselineViewpoints();
        expandedRewirBaselinesCoverMaterializedWorldRings();
        introLevelV098GroundPatchesReadAsMaterialsNotBlackArtifacts();
        introLevelV099DriveSurfacesDoNotClipBuildingsAndCoverRoute();
        introLevelV096AuthoringSectionsCanBuildTheSameLevel();
        introLevelV097UsesExplicitCollisionProfilesForPhysicalProps();
        introLevelThinReadabilityPropsDoNotCagePlayerNearCars();
        introLevelSmallDynamicPropPolicyMatchesGameplayIntent();
        introLevelLowConcreteDressingDoesNotTrapPlayer();
        propSimulationMovesOnlyControlledDynamicPropsAndPublishesCollision();
        propSimulationCanCarryOnlyControlledMovableProps();
        propSimulationStopsDynamicPropsAgainstStaticWorld();
        populateWorldLeavesPhysicalPropCollisionToPropSimulation();
        propSimulationRejectsStaticCarryTargets();
        propSimulationQueriesMovablePushTargetsWithoutMutatingThem();
        playerCanMoveOnIntroVehicleRoofWithoutWorldOverlapTrap();
        introVehicleCanDriveOutOfAuthoredParkingSpot();
        vehicleExitResolverAvoidsBlockedPreferredSide();
        chaseVehicleRuntimeKeepsPursuerOutOfTargetVehicleBody();
        chaseVehicleRuntimeResolvesPursuerAgainstWorldBlockers();
        npcBodyLiteBlocksPlayerButKeepsInteractionRange();
        chaseAiTargetsRearOffsetInsteadOfVehicleCenter();
        chaseAiContactRecoveryBacksOffAfterVehicleContact();
        introChaseAiCanBeEscapedBySustainedCleanDriving();
        visualBaselineDebugCyclesThroughAuthoredViewpoints();
        characterPosePreviewCyclesCatalogWithoutMutatingGameplay();
        characterPosePreviewBuildsPoseSpecificState();
        missionRuntimeBridgeMapsShopWalkTriggerToStoryAndEvent();
        missionRuntimeBridgeMapsVehicleAndDropoffTriggers();
        missionOutcomeTriggerResolvesDataAuthoredObjectOutcome();
        objectOutcomeCatalogResolvesWorldEventsForEnrichedOutcomes();
    } catch (const std::exception& ex) {
        std::cerr << "Test failed: " << ex.what() << '\n';
        return 1;
    }

    std::cout << "Game support tests passed\n";
    return 0;
}


