#pragma once

#include "bs3d/core/CameraRig.h"
#include "bs3d/core/MissionController.h"
#include "bs3d/core/PlayerPresentation.h"
#include "bs3d/core/PrzypalSystem.h"
#include "bs3d/core/DialogueSystem.h"
#include "bs3d/core/Types.h"
#include "bs3d/core/VehicleController.h"
#include "bs3d/core/WorldCollision.h"
#include "bs3d/core/WorldEventLedger.h"
#include "bs3d/core/WorldRewirPressure.h"

#include "CharacterArtModel.h"
#include "WorldAssetRegistry.h"

#include <string>
#include <vector>

namespace bs3d {

using RenderColor = WorldObjectTint;

struct RenderCamera {
    Vec3 position{};
    Vec3 target{};
    Vec3 up{0.0f, 1.0f, 0.0f};
    float fovy = 45.0f;
    int projection = 0;
};

struct WorldPresentationStyle {
    RenderColor skyColor{};
    RenderColor groundColor{};
    RenderColor horizonHazeColor{};
    float groundPlaneSize = 78.0f;
    float worldCullDistance = 140.0f;
};

const WorldPresentationStyle& defaultWorldPresentationStyle();

enum class WorldRenderIsolationMode {
    Full,
    OpaqueOnly,
    NoDecals,
    NoGlass,
    NoLegacyWorld,
    WorldObjectsOnly,
    DebugShopShell,
    DebugShopShellWireframe,
    DebugShopShellNoCull,
    DebugShopShellNormals,
    DebugShopShellBounds,
    DebugShopDoorWireframe,
    DebugShopDoorCube,
    AssetPreview,
    AssetPreviewWireframe,
    AssetPreviewNoCull,
    AssetPreviewNormals,
    AssetPreviewBounds
};

const char* worldRenderIsolationModeName(WorldRenderIsolationMode mode);
WorldRenderIsolationMode nextWorldRenderIsolationMode(WorldRenderIsolationMode mode);
bool isAssetPreviewIsolationMode(WorldRenderIsolationMode mode);

struct WorldAtmosphereBand {
    Vec3 center{};
    float radiusTop = 0.0f;
    float radiusBottom = 0.0f;
    float height = 0.0f;
    RenderColor color{};
};

std::vector<WorldAtmosphereBand> buildWorldAtmosphereBands(Vec3 cameraPosition,
                                                           const WorldPresentationStyle& style);

struct VehicleLightVisualState {
    RenderColor headlightColor{};
    RenderColor taillightColor{};
    float headlightGlow = 0.0f;
    float taillightGlow = 0.0f;
};

VehicleLightVisualState vehicleLightVisualState(const VehicleRuntimeState& state);

struct ObjectiveMarker {
    std::string label;
    Vec3 position{};
    RenderColor color{};
    bool active = false;
};

struct MemoryHotspotDebugMarker {
    WorldLocationTag location = WorldLocationTag::Unknown;
    Vec3 position{};
    float radius = 0.0f;
    float score = 0.0f;
    RenderColor color{};
    std::string source;
};

struct RewirPressureDebugMarker {
    RewirPressureLevel level = RewirPressureLevel::Quiet;
    WorldLocationTag location = WorldLocationTag::Unknown;
    Vec3 position{};
    float radius = 0.0f;
    float score = 0.0f;
    RenderColor color{};
    std::string source;
};

struct VehicleHudSnapshot {
    bool visible = false;
    bool chaseActive = false;
    float condition = 100.0f;
    VehicleConditionBand conditionBand = VehicleConditionBand::Igla;
    float speed = 0.0f;
    bool boostActive = false;
    float hornCooldown = 0.0f;
    bool driftActive = false;
    float driftAmount = 0.0f;
    int gear = 1;
    float rpmNormalized = 0.0f;
    float shiftTimerSeconds = 0.0f;
};

struct PrzypalHudSnapshot {
    bool playerInVehicle = false;
    bool chaseActive = false;
    float normalizedIntensity = 0.0f;
    float value = 0.0f;
    PrzypalBand band = PrzypalBand::Calm;
    std::string lastReason;
    float reactionToastSeconds = 0.0f;
    bool reactionAvailable = false;
    std::string reactionSpeaker;
    std::string reactionText;
    float hudPulse = 0.0f;
};

struct ChaseHudSnapshot {
    bool active = false;
    float escapeProgress = 0.0f;
    float caughtProgress = 0.0f;
};

struct InteractionPromptSnapshot {
    std::vector<std::string> lines;
};

struct HudSnapshot {
    int screenWidth = 0;
    int screenHeight = 0;
    bool playerInVehicle = false;
    MissionPhase missionPhase = MissionPhase::WaitingForStart;
    float phaseSeconds = 0.0f;
    float hudPulse = 0.0f;
    float flashAlpha = 0.0f;
    bool debugEnabled = false;
    std::string objectiveLine;
    ObjectiveMarker objectiveMarker;
    Vec3 actorPosition{};
    RenderCamera camera{};
    const DialogueSystem* dialogue = nullptr;
    VehicleHudSnapshot vehicle;
    PrzypalHudSnapshot przypal;
    ChaseHudSnapshot chase;
    InteractionPromptSnapshot interaction;
    bool visualBaselineActive = false;
    std::string visualBaselineLabel;
    int visualBaselineIndex = -1;
    int visualBaselineCount = 0;
    bool characterPosePreviewActive = false;
    std::string characterPosePreviewLabel;
    int characterPosePreviewIndex = -1;
    int characterPosePreviewCount = 0;
    std::string renderModeLabel;
    bool assetPreviewActive = false;
    std::string assetPreviewLabel;
    std::string assetPreviewMetadata;
    int assetPreviewIndex = -1;
    int assetPreviewCount = 0;
};

struct DebugSnapshot {
    int fps = 0;
    float deltaMs = 0.0f;
    MissionPhase phase = MissionPhase::WaitingForStart;
    Vec3 playerPosition{};
    PlayerSpeedState speedState = PlayerSpeedState::Idle;
    float playerXzSpeed = 0.0f;
    float playerVerticalSpeed = 0.0f;
    Vec3 carPosition{};
    float vehicleSpeed = 0.0f;
    float vehicleCondition = 100.0f;
    VehicleConditionBand vehicleConditionBand = VehicleConditionBand::Igla;
    float vehicleSlip = 0.0f;
    bool vehicleDriftActive = false;
    float vehicleDriftAmount = 0.0f;
    float vehicleDriftAngleDegrees = 0.0f;
    int vehicleGear = 1;
    float vehicleRpmNormalized = 0.0f;
    float vehicleShiftTimerSeconds = 0.0f;
    VehicleHitZone vehicleHitZone = VehicleHitZone::None;
    float vehicleImpactSpeed = 0.0f;
    Vec3 vehicleImpactNormal{};
    bool boostActive = false;
    float hornCooldown = 0.0f;
    CameraRigMode cameraMode = CameraRigMode::OnFootNormal;
    bool stableCameraMode = true;
    bool playerInVehicle = false;
    bool mouseCaptured = false;
    bool invertMouseX = false;
    float cameraYawDegrees = 0.0f;
    float cameraPitchDegrees = 0.0f;
    float boomLength = 0.0f;
    float fullBoomLength = 0.0f;
    float boomGap = 0.0f;
    bool grounded = false;
    float groundHeight = 0.0f;
    int supportOwnerId = 0;
    Vec3 supportPlatformVelocity{};
    float cameraTargetLag = 0.0f;
    float cameraPositionLag = 0.0f;
    float screenShake = 0.0f;
    float comedyZoom = 0.0f;
    float worldTension = 0.0f;
    float przypalValue = 0.0f;
    std::string przypalLabel;
    PrzypalBand przypalBand = PrzypalBand::Calm;
    int recentEventCount = 0;
    int memoryBlockCount = 0;
    int memoryShopCount = 0;
    int memoryParkingCount = 0;
    int memoryGarageCount = 0;
    int memoryTrashCount = 0;
    int memoryRoadLoopCount = 0;
    int memoryHotspotCount = 0;
    bool rewirPressureActive = false;
    bool rewirPressurePatrolInterest = false;
    float rewirPressureScore = 0.0f;
    float rewirPressureDistance = 0.0f;
    std::string rewirPressureLocation;
    std::string rewirPressureLevel;
    std::string rewirPressureSource;
    int localServiceTotal = 0;
    int localServiceWary = 0;
    std::string localServiceFirstWaryPoint;
    int localFavorTotal = 0;
    int localFavorActive = 0;
    int localFavorCompleted = 0;
    std::string localFavorFirstActivePoint;
    std::string lastPrzypalReason;
    bool visitedShopOnFoot = false;
    bool gruzUnlocked = false;
    bool introCompleted = false;
    bool rawW = false;
    bool rawA = false;
    bool rawS = false;
    bool rawD = false;
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    bool accelerate = false;
    bool brake = false;
    bool steerLeft = false;
    bool steerRight = false;
    float steeringAuthority = 0.0f;
    float vehicleYawDegrees = 0.0f;
    float visualYawDegrees = 0.0f;
    float visualYawErrorDegrees = 0.0f;
    int routeIndex = 0;
    std::string routeLabel;
    float chaseDistance = 0.0f;
    std::string chaseAiMode;
    float chaseRecoverSeconds = 0.0f;
    bool chaseContactRecover = false;
    bool reactionAvailable = false;
    std::string reactionSpeaker;
    bool visualBaselineActive = false;
    std::string visualBaselineLabel;
    int visualBaselineIndex = -1;
    int visualBaselineCount = 0;
    std::string renderModeLabel;
    std::string activeShopAssetId;
    std::string nearestObjectId;
    std::string nearestObjectAssetId;
    std::string nearestObjectZone;
    std::string nearestObjectCollision;
    std::string nearestObjectTags;
    float nearestObjectDistance = 0.0f;
    int opaqueAssetCount = 0;
    int decalAssetCount = 0;
    int glassAssetCount = 0;
    int translucentAssetCount = 0;
    int vehicleAssetCount = 0;
    int debugOnlyAssetSkippedCount = 0;
    std::string buildStamp;
    std::string dataRoot;
    std::string executablePath;
};

struct CameraDebugLinesSnapshot {
    bool debugEnabled = false;
    bool playerInVehicle = false;
    CameraRigState rig{};
    Vec3 playerPosition{};
    Vec3 playerVelocity{};
    Vec3 vehiclePosition{};
    float playerCameraYawRadians = 0.0f;
    float vehicleYawRadians = 0.0f;
    Vec3 vehicleCollisionCircles[3]{};
};

struct HudPanelRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

HudPanelRect layoutHudPanel(int screenWidth,
                            int preferredWidth,
                            int y,
                            int height,
                            bool alignLeft = false,
                            int margin = 18);

struct CharacterRenderOptions {
    bool hideLowerBody = false;
    const CharacterArtPalette* palette = nullptr;
};

bool shouldRenderCharacterPart(CharacterPartRole role, CharacterRenderOptions options);
Vec3 vehicleDriverSeatPosition(Vec3 vehiclePosition, float vehicleYawRadians);

struct WorldRenderList {
    std::vector<const WorldObject*> opaque;
    std::vector<const WorldObject*> translucent;
    std::vector<const WorldObject*> glass;
    std::vector<const WorldObject*> transparent;
    int culled = 0;
};

WorldRenderList buildWorldRenderList(const std::vector<WorldObject>& objects,
                                     const WorldAssetRegistry& registry,
                                     Vec3 cameraPosition,
                                     float maxDistance,
                                     WorldRenderIsolationMode mode = WorldRenderIsolationMode::Full);
std::vector<MemoryHotspotDebugMarker> buildMemoryHotspotDebugMarkers(
    const std::vector<WorldEventHotspot>& hotspots);
std::vector<RewirPressureDebugMarker> buildRewirPressureDebugMarkers(const RewirPressureSnapshot& pressure);

class WorldRenderer {
public:
    static void drawWorldGround();
    static void drawWorldAtmosphere(Vec3 cameraPosition);
    static void drawWorldOpaque(const WorldRenderList& renderList,
                                const WorldAssetRegistry& registry,
                                const WorldModelCache& modelCache,
                                WorldRenderIsolationMode mode = WorldRenderIsolationMode::Full);
    static void drawWorldTransparent(const WorldRenderList& renderList,
                                     const WorldAssetRegistry& registry,
                                     const WorldModelCache& modelCache,
                                     WorldRenderIsolationMode mode = WorldRenderIsolationMode::Full);
    static void drawAssetPreview(const WorldAssetDefinition& definition,
                                 const WorldModelCache& modelCache,
                                 WorldRenderIsolationMode mode);
    static void drawWorldCollisionDebug(const std::vector<WorldObject>& objects);
    static void drawMarker(Vec3 position, float radius, RenderColor markerColor);
    static void drawMemoryHotspotDebugMarkers(const std::vector<MemoryHotspotDebugMarker>& markers,
                                              float elapsedSeconds);
    static void drawRewirPressureDebugMarkers(const std::vector<RewirPressureDebugMarker>& markers,
                                              float elapsedSeconds);
    static void drawObjectiveBeacon(const ObjectiveMarker& marker, float pulse, float elapsedSeconds);
    static void drawPlayer(Vec3 position,
                           float yawRadians,
                           const PlayerPresentationState& pose,
                           float elapsedSeconds,
                           CharacterRenderOptions options = {});
    static void drawCharacterCapsuleDebug(Vec3 position, RenderColor tint);
    static void drawVehicle(Vec3 position, float yawRadians, RenderColor bodyColor);
    static void drawVehicle(const VehicleRuntimeState& state, RenderColor bodyColor);
    static void drawVehicleOpaque(const VehicleRuntimeState& state, RenderColor bodyColor);
    static void drawVehicleGlass(const VehicleRuntimeState& state);
    static void drawVehicleSmoke(const VehicleController& vehicle, float elapsedSeconds);
    static void drawVehicleSmoke(const VehicleRuntimeState& state, float elapsedSeconds);
};

class HudRenderer {
public:
    static std::vector<std::string> wrapTextToWidth(const std::string& text, int fontSize, int maxWidth);
    static void drawDialogueBox(const DialogueSystem& dialogue, int screenWidth, int screenHeight);
    static void drawHud(const HudSnapshot& snapshot);
    static void drawObjectiveCompass(const HudSnapshot& snapshot);
    static void drawInteractionPrompt(const InteractionPromptSnapshot& snapshot, int screenWidth, int screenHeight);
    static void drawPrzypalHud(const PrzypalHudSnapshot& snapshot, int screenWidth);
    static void drawVehicleHud(const VehicleHudSnapshot& snapshot, int screenWidth);
};

class DebugRenderer {
public:
    static void drawPanelBackground(int x, int y, int width, int height);
    static void drawPanel(const DebugSnapshot& snapshot);
    static void drawCameraLines(const CameraDebugLinesSnapshot& snapshot);
};

} // namespace bs3d
