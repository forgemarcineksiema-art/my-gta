#include "RenderExtraction.h"
#include "bs3d/game/GameApp.h"

#include "GameRenderers.h"
#include "ChaseAiRuntime.h"
#include "ChaseVehicleRuntime.h"
#include "CharacterPosePreview.h"
#include "D3D11ShadowSidecar.h"
#include "DevTools.h"

#include "MaterialRegistry.h"
#include "MeshRegistry.h"
#include "RenderFrameDump.h"

#include "CpuMeshData.h"
#include "CpuMeshLoader.h"
#include "IntroLevelBuilder.h"
#include "IntroLevelPresentation.h"
#include "MissionOutcomeTrigger.h"
#include "MissionRuntimeBridge.h"
#include "PropSimulationSystem.h"
#include "RaylibInputReader.h"
#include "RaylibPlatform.h"
#include "RuntimeAudio.h"
#include "VehicleExitResolver.h"
#include "VisualBaselineDebug.h"
#include "WorldDataLoader.h"
#include "WorldEventEmitterBridge.h"
#include "WorldObjectInteraction.h"

#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
#include "RuntimeMapEditor.h"
#include "RuntimeMapEditorImGui.h"
#include "EditorGizmo.h"
#include "rlImGui.h"
#endif

#include "bs3d/core/CameraRig.h"
#include "bs3d/core/CharacterAction.h"
#include "bs3d/core/CharacterPhysicalReaction.h"
#include "bs3d/core/CharacterStatusPolicy.h"
#include "bs3d/core/CharacterVaultPolicy.h"
#include "bs3d/core/CharacterWorldHitPolicy.h"
#include "bs3d/core/ChaseSystem.h"
#include "bs3d/core/ControlContext.h"
#include "bs3d/core/DialogueSystem.h"
#include "bs3d/core/DriveRouteGuide.h"
#include "bs3d/core/FixedTimestep.h"
#include "bs3d/core/GameFeedback.h"
#include "bs3d/core/InteractionResolver.h"
#include "bs3d/core/MissionController.h"
#include "bs3d/core/MissionTriggerSystem.h"
#include "bs3d/core/NpcReactionSystem.h"
#include "bs3d/core/ParagonMission.h"
#include "bs3d/core/ParagonMissionSpec.h"
#include "bs3d/core/PlayerPresentation.h"
#include "bs3d/core/PlayerController.h"
#include "bs3d/core/PrzypalSystem.h"
#include "bs3d/core/RenderInterpolation.h"
#include "bs3d/core/Scene.h"
#include "bs3d/core/SaveGame.h"
#include "bs3d/core/StoryState.h"
#include "bs3d/core/VehicleController.h"
#include "bs3d/core/WorldEventLedger.h"
#include "bs3d/core/WorldInteraction.h"
#include "bs3d/core/WorldRewirPressure.h"
#include "bs3d/core/WorldServiceState.h"
#include "bs3d/core/WorldCollision.h"

#include "bs3d/render/NullRenderer.h"
#include "bs3d/render/RenderFrameBuilder.h"
#include "bs3d/render/RenderFrameValidation.h"

#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace bs3d {

namespace {

std::string buildConfigName() {
#if defined(NDEBUG)
    return "Release";
#else
    return "Debug";
#endif
}

std::string buildStampText() {
    return buildConfigName() + " " + __DATE__ + " " + __TIME__;
}

std::string devToolsStatusText() {
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
    return "DevTools ON";
#else
    return "DevTools OFF";
#endif
}

std::string resolvedPathText(const std::string& value) {
    if (value.empty()) {
        return "(unknown)";
    }
    std::error_code error;
    const std::filesystem::path absolute = std::filesystem::absolute(value, error);
    if (error) {
        return value;
    }
    return absolute.string();
}

std::string runtimeWindowTitle(const GameRunOptions& options) {
    std::ostringstream title;
    title << "Blok 13 | " << buildStampText() << " | " << devToolsStatusText() << " | data "
          << resolvedPathText(options.dataRoot);
    title << " | renderer " << bs3d::rendererBackendName(options.rendererBackend) << " | physics "
          << bs3d::physicsBackendName(options.physicsBackend);
    if (!options.executablePath.empty()) {
        title << " | exe " << resolvedPathText(options.executablePath);
    }
    return title.str();
}

constexpr int ScreenWidth = 1024;
constexpr int ScreenHeight = 720;

Vector3 toRay(Vec3 value) {
    return {value.x, value.y, value.z};
}

float radiansToDegrees(float radians) {
    return radians * 180.0f / Pi;
}

Color color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
    return {r, g, b, a};
}

RenderColor renderColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
    return {r, g, b, a};
}

Color toRayColor(RenderColor tint) {
    return {tint.r, tint.g, tint.b, tint.a};
}

RenderCamera toRenderCamera(Camera3D camera) {
    return {{camera.position.x, camera.position.y, camera.position.z},
            {camera.target.x, camera.target.y, camera.target.z},
            {camera.up.x, camera.up.y, camera.up.z},
            camera.fovy,
            camera.projection};
}

bool environmentFlagEnabled(const char* name) {
#if defined(_WIN32)
    char* value = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) {
        return false;
    }
    const bool enabled = length > 1 && value[0] != '0';
    std::free(value);
    return enabled;
#else
    const char* value = std::getenv(name);
    return value != nullptr && value[0] != '\0' && value[0] != '0';
#endif
}

bool hasGameplayTag(const WorldObject& object, const std::string& tag) {
    return std::find(object.gameplayTags.begin(), object.gameplayTags.end(), tag) != object.gameplayTags.end();
}

bool pointInsideObjectXZ(const WorldObject& object, Vec3 position, float padding = 0.0f) {
    const Vec3 offset = position - object.position;
    const float c = std::cos(-object.yawRadians);
    const float s = std::sin(-object.yawRadians);
    const float localX = offset.x * c - offset.z * s;
    const float localZ = offset.x * s + offset.z * c;
    return std::fabs(localX) <= object.scale.x * 0.5f + padding &&
           std::fabs(localZ) <= object.scale.z * 0.5f + padding;
}

float przypalDifficultyFactor(PrzypalBand band) {
    switch (band) {
    case PrzypalBand::Calm:
        return 1.0f;
    case PrzypalBand::Noticed:
        return 1.15f;
    case PrzypalBand::Hot:
        return 1.35f;
    case PrzypalBand::ChaseRisk:
        return 1.55f;
    case PrzypalBand::Meltdown:
        return 1.80f;
    }
    return 1.0f;
}

Vector3 interpolateVector3(Vector3 previous, Vector3 current, float alpha) {
    const Vec3 interpolated = interpolateVec3({previous.x, previous.y, previous.z},
                                              {current.x, current.y, current.z},
                                              alpha);
    return toRay(interpolated);
}

Camera3D interpolateCamera3D(const Camera3D& previous, const Camera3D& current, float alpha) {
    Camera3D camera = current;
    camera.position = interpolateVector3(previous.position, current.position, alpha);
    camera.target = interpolateVector3(previous.target, current.target, alpha);
    camera.up = interpolateVector3(previous.up, current.up, alpha);
    camera.fovy = interpolateFloat(previous.fovy, current.fovy, alpha);
    return camera;
}

std::vector<WorldObject> interpolateWorldObjects(const std::vector<WorldObject>& previous,
                                                 const std::vector<WorldObject>& current,
                                                 float alpha) {
    if (previous.size() != current.size()) {
        return current;
    }

    std::vector<WorldObject> objects = current;
    for (std::size_t index = 0; index < objects.size(); ++index) {
        if (previous[index].id != current[index].id) {
            continue;
        }
        objects[index].position = interpolateVec3(previous[index].position, current[index].position, alpha);
        objects[index].scale = interpolateVec3(previous[index].scale, current[index].scale, alpha);
        objects[index].yawRadians = interpolateYawRadians(previous[index].yawRadians, current[index].yawRadians, alpha);
    }
    return objects;
}

struct RuntimeRenderSnapshot {
    std::vector<WorldObject> worldObjects;
    Vec3 playerPosition{};
    float playerYawRadians = 0.0f;
    VehicleRuntimeState vehicle{};
    Vec3 chaserPosition{};
    float chaserYawRadians = 0.0f;
    Camera3D camera{};
    float cameraYawRadians = 0.0f;
};

RuntimeRenderSnapshot interpolateRenderSnapshot(const RuntimeRenderSnapshot& previous,
                                                const RuntimeRenderSnapshot& current,
                                                float alpha) {
    RuntimeRenderSnapshot snapshot = current;
    snapshot.worldObjects = interpolateWorldObjects(previous.worldObjects, current.worldObjects, alpha);
    snapshot.playerPosition = interpolateVec3(previous.playerPosition, current.playerPosition, alpha);
    snapshot.playerYawRadians = interpolateYawRadians(previous.playerYawRadians, current.playerYawRadians, alpha);
    snapshot.vehicle = interpolateVehicleRuntimeState(previous.vehicle, current.vehicle, alpha);
    snapshot.chaserPosition = interpolateVec3(previous.chaserPosition, current.chaserPosition, alpha);
    snapshot.chaserYawRadians = interpolateYawRadians(previous.chaserYawRadians, current.chaserYawRadians, alpha);
    snapshot.camera = interpolateCamera3D(previous.camera, current.camera, alpha);
    snapshot.cameraYawRadians = interpolateYawRadians(previous.cameraYawRadians, current.cameraYawRadians, alpha);
    return snapshot;
}

struct GameWorldRuntime {
    WorldCollision collision;
    WorldInteraction interactions;
    Scene scene;
    IntroLevelData level;
    WorldDataCatalog dataCatalog;
    WorldAssetRegistry assetRegistry;
    WorldModelCache worldModels;
    PropSimulationSystem propSimulation;
    DriveRouteGuide driveRoute;
};

struct MissionRuntime {
    DialogueSystem dialogue;
    MissionController mission;
    ParagonMission paragonMission;
    MissionTriggerSystem missionTriggers;
    StoryState story;
    MissionRuntimeBridge bridge;

    MissionRuntime() : mission(dialogue), paragonMission(dialogue) {}
};

struct VehicleRuntime {
    VehicleController vehicle;
    ChaseSystem chase;
    Vec3 chaserPosition{20.0f, 0.0f, -24.0f};
    float chaserYaw = 0.0f;
    ChaseVehicleStep lastChaseVehicleStep{};
    ChaseAiRuntimeState chaseAiState{};
    ChaseAiCommand lastChaseAiCommand{};
    Vec3 chaserVelocity{};
};

struct WorldReactionRuntime {
    GameFeedback feedback;
    WorldEventLedger worldEvents;
    WorldEventEmitterCooldowns eventCooldowns;
    WorldEventEmitterBridge eventEmitter;
    PrzypalSystem przypal;
    NpcReactionSystem reactionSystem;
};

struct InputRouter {
    InputState mapToGameplayInput(const RawInputState& raw, const ControlContext& context) const {
        return mapRawInputToInputState(raw, context);
    }
};

struct RenderCoordinator {
    WorldRenderList buildWorldRenderList(const std::vector<WorldObject>& objects,
                                         const WorldAssetRegistry& registry,
                                         Vec3 cameraPosition,
                                         WorldRenderIsolationMode mode) const {
        return bs3d::buildWorldRenderList(
            objects,
            registry,
            cameraPosition,
            defaultWorldPresentationStyle().worldCullDistance,
            mode);
    }

    void drawWorldOpaque(const WorldRenderList& renderList,
                         const WorldAssetRegistry& registry,
                         const WorldModelCache& modelCache,
                         WorldRenderIsolationMode mode) const {
        WorldRenderer::drawWorldOpaque(renderList, registry, modelCache, mode);
    }

    void drawWorldTransparent(const WorldRenderList& renderList,
                              const WorldAssetRegistry& registry,
                              const WorldModelCache& modelCache,
                              WorldRenderIsolationMode mode) const {
        WorldRenderer::drawWorldTransparent(renderList, registry, modelCache, mode);
    }

    void drawHud(const HudSnapshot& snapshot) const {
        HudRenderer::drawHud(snapshot);
    }
};

struct MissionCheckpointSnapshot {
    bool valid = false;
    std::string id;
    MissionPhase phase = MissionPhase::ReachVehicle;
    Vec3 playerPosition{};
    float playerYawRadians = 0.0f;
    bool playerInVehicle = false;
    Vec3 vehiclePosition{};
    float vehicleYawRadians = 0.0f;
    float vehicleCondition = 100.0f;
    Vec3 chaserPosition{};
    float chaserYaw = 0.0f;
    float przypalValue = 0.0f;
    float phaseSeconds = 0.0f;
    StoryState story{};
    ParagonMissionPhase paragonPhase = ParagonMissionPhase::Locked;
    ParagonSolution paragonSolution = ParagonSolution::None;
    bool shopBanActive = false;
    WorldReactionSnapshot worldReaction{};
    PropSimulationSnapshot props{};
    std::vector<std::string> completedLocalRewirFavorIds;
};

struct Runtime {
    GameWorldRuntime gameWorld;
    MissionRuntime missionRuntime;
    VehicleRuntime vehicleRuntime;
    WorldReactionRuntime worldReactionRuntime;
    InputRouter inputRouter;
    RenderCoordinator renderCoordinator;
    IPlatform& platform;
    IInputReader& inputReader;

    DialogueSystem& dialogue;
    MissionController& mission;
    ParagonMission& paragonMission;
    MissionTriggerSystem& missionTriggers;
    StoryState& story;
    PlayerController player;
    PlayerPresentation playerPresentation;
    VehicleController& vehicle;
    ChaseSystem& chase;
    CameraRig cameraRig;
    GameFeedback& feedback;
    WorldEventLedger& worldEvents;
    WorldEventEmitterCooldowns& eventCooldowns;
    WorldEventEmitterBridge& eventEmitter;
    PrzypalSystem& przypal;
    NpcReactionSystem& reactionSystem;
    WorldCollision& collision;
    WorldInteraction& interactions;
    Scene& scene;
    Camera3D camera{};
    IntroLevelData& level;
    WorldAssetRegistry& assetRegistry;
    WorldModelCache& worldModels;
    MissionRuntimeBridge& missionRuntimeBridge;
    PropSimulationSystem& propSimulation;
    DriveRouteGuide& driveRoute;
    VisualBaselineDebug visualBaselineDebug;
    CharacterPosePreview characterPosePreview;
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
    RuntimeMapEditor runtimeEditor;
    RuntimeMapEditorImGui runtimeEditorUi;
    EditorGizmo gizmo;
    bool runtimeEditorOpen = false;
#endif

    Vec3& chaserPosition;
    float& chaserYaw;
    ChaseVehicleStep& lastChaseVehicleStep;
    ChaseAiRuntimeState& chaseAiState;
    ChaseAiCommand& lastChaseAiCommand;
    Vec3& chaserVelocity;
    bool playerInVehicle = false;
    bool debugEnabled = false;
    bool mouseCaptured = true;
    bool invertMouseX = false;
    bool stableCameraMode = true;
    WorldRenderIsolationMode renderIsolationMode = WorldRenderIsolationMode::Full;
    int assetPreviewIndex = 0;
    RuntimeAudio audio;
    float elapsedSeconds = 0.0f;
    float lastDeltaSeconds = 0.0f;
    ReactionIntent lastReaction{};
    std::string lastPrzypalReason = "brak";
    float reactionToastSeconds = 0.0f;
    MissionPhase observedPhase = MissionPhase::WaitingForStart;
    float phaseSeconds = 0.0f;
    float vehicleLockedHintCooldown = 0.0f;
    float interactionCameraSeconds = 0.0f;
    float characterFollowUpDelaySeconds = 0.0f;
    float characterFollowUpDurationSeconds = 0.0f;
    CharacterPoseKind characterFollowUpPose = CharacterPoseKind::Idle;
    float characterNpcBumpCooldownSeconds = 0.0f;
    RawInputState lastRawInput{};
    InputState lastInput{};
    VehicleCollisionResult lastVehicleCollision{};
    MissionCheckpointSnapshot missionCheckpoint{};
    RuntimeRenderSnapshot previousRenderSnapshot{};
    RuntimeRenderSnapshot currentRenderSnapshot{};
    std::string dataRoot = "data";
    std::string savePath = "artifacts/savegame_intro.sav";
    std::string executablePath;
    bool loadSaveOnStart = true;
    bool writeSave = true;
    std::vector<std::string> completedLocalRewirFavorIds;

    explicit Runtime(GameRunOptions options, IPlatform& platform, IInputReader& inputReader)
        : platform(platform),
          inputReader(inputReader),
          dialogue(missionRuntime.dialogue),
          mission(missionRuntime.mission),
          paragonMission(missionRuntime.paragonMission),
          missionTriggers(missionRuntime.missionTriggers),
          story(missionRuntime.story),
          vehicle(vehicleRuntime.vehicle),
          chase(vehicleRuntime.chase),
          feedback(worldReactionRuntime.feedback),
          worldEvents(worldReactionRuntime.worldEvents),
          eventCooldowns(worldReactionRuntime.eventCooldowns),
          eventEmitter(worldReactionRuntime.eventEmitter),
          przypal(worldReactionRuntime.przypal),
          reactionSystem(worldReactionRuntime.reactionSystem),
          collision(gameWorld.collision),
          interactions(gameWorld.interactions),
          scene(gameWorld.scene),
          level(gameWorld.level),
          assetRegistry(gameWorld.assetRegistry),
          worldModels(gameWorld.worldModels),
          missionRuntimeBridge(missionRuntime.bridge),
          propSimulation(gameWorld.propSimulation),
          driveRoute(gameWorld.driveRoute),
          chaserPosition(vehicleRuntime.chaserPosition),
          chaserYaw(vehicleRuntime.chaserYaw),
          lastChaseVehicleStep(vehicleRuntime.lastChaseVehicleStep),
          chaseAiState(vehicleRuntime.chaseAiState),
          lastChaseAiCommand(vehicleRuntime.lastChaseAiCommand),
          chaserVelocity(vehicleRuntime.chaserVelocity) {
        dataRoot = std::move(options.dataRoot);
        savePath = std::move(options.savePath);
        executablePath = std::move(options.executablePath);
        loadSaveOnStart = options.loadSaveOnStart;
        writeSave = options.writeSave;
    }

    void startInteractionCamera(float seconds = 2.2f) {
        if (!playerInVehicle) {
            interactionCameraSeconds = std::max(interactionCameraSeconds, seconds);
        }
    }

    bool explicitInteractionCameraActive() const {
        return !playerInVehicle && interactionCameraSeconds > 0.0f;
    }

    Vec3 vehiclePlatformVelocity() const {
        if (!playerInVehicle) {
            return {};
        }
        const VehicleRuntimeState& state = vehicle.runtimeState();
        return forwardFromYaw(state.yawRadians) * state.speed +
               screenRightFromYaw(state.yawRadians) * state.lateralSlip;
    }

    void setup() {
        const std::filesystem::path dataRootPath(dataRoot);
        const std::filesystem::path assetsRoot = dataRootPath / "assets";

        const AssetManifestLoadResult manifestResult =
            assetRegistry.loadManifest((assetsRoot / "asset_manifest.txt").string());
        for (const std::string& warning : manifestResult.warnings) {
            TraceLog(LOG_WARNING, "Asset manifest warning: %s", warning.c_str());
        }
        if (!manifestResult.loaded) {
            throw std::runtime_error("asset manifest failed to load");
        }

        level = IntroLevelBuilder::build(IntroLevelBuildConfig{}, assetRegistry);

        gameWorld.dataCatalog = loadWorldDataCatalog(dataRootPath.string());
        if (gameWorld.dataCatalog.loaded) {
            const WorldDataApplyResult dataApply = applyWorldDataCatalog(level, gameWorld.dataCatalog);
            const int missionOverrides = applyMissionDataToController(mission, gameWorld.dataCatalog.mission);
            TraceLog(LOG_INFO,
                     "World data catalog applied: mission phases=%d overrides=%d",
                     dataApply.appliedMissionPhases,
                     missionOverrides);
        } else {
            for (const std::string& warning : gameWorld.dataCatalog.warnings) {
                TraceLog(LOG_WARNING, "World data warning: %s", warning.c_str());
            }
        }
        const AssetValidationResult assetValidation = assetRegistry.validateAssets(assetsRoot.string());
        for (const std::string& warning : assetValidation.warnings) {
            TraceLog(LOG_WARNING, "Asset validation warning: %s", warning.c_str());
        }
        if (!assetValidation.ok) {
            throw std::runtime_error("asset manifest references missing files");
        }
        const WorldModelLoadResult modelLoad = worldModels.loadAll(assetRegistry, assetsRoot.string());
        for (const std::string& warning : modelLoad.warnings) {
            TraceLog(LOG_WARNING, "Model load warning: %s", warning.c_str());
        }
        const bool failOnModelWarnings = runtimeDevToolsEnabledByDefault() ||
                                         environmentFlagEnabled("BS3D_FAIL_ON_MODEL_WARNINGS");
        const ModelLoadQualityGate modelGate = evaluateModelLoadQuality(modelLoad, failOnModelWarnings);
        if (!modelGate.ok) {
            throw std::runtime_error(modelGate.message);
        }
        player.reset(level.playerStart, 0.0f);
        resetDriveRoute();
        vehicle.reset(level.vehicleStart, driveRoute.yawFrom(level.vehicleStart));
        chaserPosition = level.initialChaserPosition;
        IntroLevelBuilder::populateWorld(level, scene, collision);
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
        runtimeEditor.attach(level, gameWorld.dataCatalog.editorOverlay.document);
#endif
        propSimulation.addPropsFromWorld(level.objects);
        IntroLevelBuilder::populateInteractions(level, interactions);
        missionTriggers.setTriggers(level.missionTriggers);

        camera.fovy = 55.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        const bool restoredSave = restoreSaveAtStartup();
        cameraRig.reset(cameraTarget());
        updateCamera();
        resetRenderSnapshots();
        captureMissionCheckpoint(restoredSave && !missionCheckpoint.id.empty() ? missionCheckpoint.id : "intro_start");
    }

    void shutdown() {
        worldModels.unload();
        audio.shutdown();
    }

    void resetDriveRoute() {
        driveRoute.reset(level.driveRoute);
    }

    SaveGame buildSaveGame(const std::string& checkpointId) const {
        SaveGame save;
        const PrzypalSnapshot przypalState = przypal.snapshot();
        save.story = story;
        save.mission.phase = mission.phase();
        save.mission.phaseSeconds = phaseSeconds;
        save.mission.checkpointId = checkpointId;
        save.paragonPhase = paragonMission.phase();
        save.playerPosition = player.position();
        save.playerYawRadians = player.yawRadians();
        save.playerInVehicle = playerInVehicle;
        save.vehiclePosition = vehicle.position();
        save.vehicleYawRadians = vehicle.yawRadians();
        save.vehicleCondition = vehicle.condition();
        save.przypalValue = przypalState.value;
        save.przypalDecayDelayRemaining = przypalState.decayDelayRemaining;
        save.przypalBand = przypalState.band;
        save.przypalBandPulseAvailable = przypalState.bandPulseAvailable;
        save.przypalContributors = przypalState.contributors;
        save.worldEvents = worldEvents.recentEvents();
        save.eventCooldowns = eventCooldowns.snapshot();
        save.completedLocalRewirFavorIds = completedLocalRewirFavorIds;
        return save;
    }

    void applySaveGame(const SaveGame& save) {
        story = save.story;
        completedLocalRewirFavorIds = save.completedLocalRewirFavorIds;
        playerInVehicle = save.playerInVehicle;
        player.reset(save.playerPosition, save.playerYawRadians);
        vehicle.reset(save.vehiclePosition, save.vehicleYawRadians);
        vehicle.setCondition(save.vehicleCondition);
        przypal.restore({save.przypalValue,
                         save.przypalDecayDelayRemaining,
                         save.przypalBand,
                         save.przypalBandPulseAvailable,
                         save.przypalContributors});
        worldEvents.restoreRecentEvents(save.worldEvents);
        eventCooldowns.restore(save.eventCooldowns);
        mission.restoreForSave(save.mission.phase, save.mission.phaseSeconds);
        observedPhase = mission.phase();
        phaseSeconds = save.mission.phaseSeconds;
        paragonMission.restoreForSave(save.paragonPhase,
                                      save.story.paragonSolution,
                                      save.story.shopBanActive);
        missionCheckpoint.id = save.mission.checkpointId.empty() ? "loaded_save" : save.mission.checkpointId;
    }

    bool persistCurrentSave(const std::string& checkpointId) const {
        if (!writeSave) {
            return false;
        }

        const std::filesystem::path saveTarget(savePath);
        if (!saveTarget.parent_path().empty()) {
            std::filesystem::create_directories(saveTarget.parent_path());
        }
        if (!saveGameToFile(buildSaveGame(checkpointId), savePath)) {
            TraceLog(LOG_WARNING, "SaveGame write failed: %s", savePath.c_str());
            return false;
        }

        return true;
    }

    bool restoreSaveAtStartup() {
        if (!loadSaveOnStart) {
            return false;
        }

        SaveGame loaded;
        if (!loadSaveGameFromFile(savePath, loaded)) {
            TraceLog(LOG_INFO, "SaveGame load skipped or invalid: %s", savePath.c_str());
            return false;
        }

        applySaveGame(loaded);
        TraceLog(LOG_INFO, "SaveGame restored: %s checkpoint=%s", savePath.c_str(), loaded.mission.checkpointId.c_str());
        return true;
    }

    void captureMissionCheckpoint(const std::string& id) {
        missionCheckpoint.valid = true;
        missionCheckpoint.id = id;
        missionCheckpoint.phase = mission.phase();
        if (missionCheckpoint.phase == MissionPhase::WaitingForStart ||
            missionCheckpoint.phase == MissionPhase::Failed ||
            missionCheckpoint.phase == MissionPhase::Completed) {
            missionCheckpoint.phase = MissionPhase::ReachVehicle;
        }
        missionCheckpoint.playerPosition = player.position();
        missionCheckpoint.playerYawRadians = player.yawRadians();
        missionCheckpoint.playerInVehicle = playerInVehicle;
        missionCheckpoint.vehiclePosition = vehicle.position();
        missionCheckpoint.vehicleYawRadians = vehicle.yawRadians();
        missionCheckpoint.vehicleCondition = vehicle.condition();
        missionCheckpoint.chaserPosition = chaserPosition;
        missionCheckpoint.chaserYaw = chaserYaw;
        missionCheckpoint.przypalValue = przypal.value();
        missionCheckpoint.phaseSeconds = phaseSeconds;
        missionCheckpoint.story = story;
        missionCheckpoint.paragonPhase = paragonMission.phase();
        missionCheckpoint.paragonSolution = paragonMission.solution();
        missionCheckpoint.shopBanActive = paragonMission.shopBanActive();
        missionCheckpoint.worldReaction = captureWorldReactionSnapshot(przypal, worldEvents, eventCooldowns);
        missionCheckpoint.props = propSimulation.snapshot();
        missionCheckpoint.completedLocalRewirFavorIds = completedLocalRewirFavorIds;

        persistCurrentSave(id);
    }

    void restoreMissionCheckpoint() {
        if (!missionCheckpoint.valid) {
            captureMissionCheckpoint("intro_retry_fallback");
        }
        playerInVehicle = missionCheckpoint.playerInVehicle;
        player.reset(missionCheckpoint.playerPosition, missionCheckpoint.playerYawRadians);
        vehicle.reset(missionCheckpoint.vehiclePosition, missionCheckpoint.vehicleYawRadians);
        vehicle.setCondition(missionCheckpoint.vehicleCondition);
        chaserPosition = missionCheckpoint.chaserPosition;
        chaserYaw = missionCheckpoint.chaserYaw;
        story = missionCheckpoint.story;
        paragonMission.restoreForSave(missionCheckpoint.paragonPhase,
                                      missionCheckpoint.paragonSolution,
                                      missionCheckpoint.shopBanActive);
        completedLocalRewirFavorIds = missionCheckpoint.completedLocalRewirFavorIds;
        restoreWorldReactionSnapshot(missionCheckpoint.worldReaction, przypal, worldEvents, eventCooldowns);
        propSimulation.restore(missionCheckpoint.props);
        mission.retryToCheckpoint(missionCheckpoint.phase);
        phaseSeconds = missionCheckpoint.phaseSeconds;
        observedPhase = missionCheckpoint.phase;
    }

    void resetForRetry() {
        resetDriveRoute();
        restoreMissionCheckpoint();
        chaseAiState = {};
        lastChaseAiCommand = {};
        lastChaseVehicleStep = {};
        chaserVelocity = {};
        chase.stop();
        missionTriggers.resetConsumed();
        reactionSystem.clear();
        lastReaction = {};
        lastPrzypalReason = "brak";
        reactionToastSeconds = 0.0f;
        feedback.trigger(FeedbackEvent::MarkerReached);
        cameraRig.reset(cameraTarget());
        resetRenderSnapshots();
    }

    RawInputState readRawInput() const {
        return inputReader.read(mouseCaptured);
    }

    int assetPreviewCount() const {
        return static_cast<int>(assetRegistry.definitions().size());
    }

    const WorldAssetDefinition* selectedAssetPreviewDefinition() const {
        const std::vector<WorldAssetDefinition>& definitions = assetRegistry.definitions();
        if (definitions.empty()) {
            return nullptr;
        }
        const int clampedIndex = std::clamp(assetPreviewIndex, 0, static_cast<int>(definitions.size()) - 1);
        return &definitions[clampedIndex];
    }

    void cycleAssetPreview(int direction) {
        const int count = assetPreviewCount();
        if (count <= 0) {
            assetPreviewIndex = 0;
            return;
        }
        assetPreviewIndex = (assetPreviewIndex + direction) % count;
        if (assetPreviewIndex < 0) {
            assetPreviewIndex += count;
        }
        if (!isAssetPreviewIsolationMode(renderIsolationMode)) {
            renderIsolationMode = WorldRenderIsolationMode::AssetPreview;
            debugEnabled = true;
        }
    }

    std::string assetPreviewMetadata(const WorldAssetDefinition& definition) const {
        std::ostringstream stream;
        stream << definition.assetType << "/" << definition.usage
               << " render=" << definition.renderBucket
               << " collision=" << definition.defaultCollision
               << " origin=" << definition.originPolicy
               << " closed=" << (definition.requiresClosedMesh ? "yes" : "no")
               << " openEdges=" << (definition.allowOpenEdges ? "allowed" : "no")
               << " gameplay=" << (definition.renderInGameplay ? "yes" : "no");
        return stream.str();
    }

    void populateAssetRuntimeStats(DebugSnapshot& snapshot) const {
        snapshot.activeShopAssetId = "(missing)";
        for (const WorldObject& object : level.objects) {
            const WorldAssetDefinition* definition = assetRegistry.find(object.assetId);
            if (object.id == "shop_zenona") {
                snapshot.activeShopAssetId = object.assetId;
            }
            if (definition == nullptr) {
                ++snapshot.opaqueAssetCount;
                continue;
            }
            if (!definition->renderInGameplay ||
                definition->renderBucket == "DebugOnly" ||
                definition->assetType == "DebugOnly") {
                ++snapshot.debugOnlyAssetSkippedCount;
                continue;
            }
            if (definition->renderBucket == "Decal") {
                ++snapshot.decalAssetCount;
            } else if (definition->renderBucket == "Glass") {
                ++snapshot.glassAssetCount;
            } else if (definition->renderBucket == "Translucent") {
                ++snapshot.translucentAssetCount;
            } else if (definition->renderBucket == "Vehicle") {
                ++snapshot.vehicleAssetCount;
            } else {
                ++snapshot.opaqueAssetCount;
            }
        }
    }

    void handleFrameInput(RawInputState& rawInput) {
        if (rawInput.fullscreenTogglePressed) {
            platform.toggleFullscreen();
        }
        if (runtimeDevToolsEnabledByDefault() && rawInput.invertMouseTogglePressed) {
            invertMouseX = !invertMouseX;
        }
        if (runtimeDevToolsEnabledByDefault() && rawInput.stableCameraTogglePressed) {
            stableCameraMode = !stableCameraMode;
        }
        if (runtimeDevToolsEnabledByDefault() && rawInput.renderIsolationTogglePressed) {
            renderIsolationMode = nextWorldRenderIsolationMode(renderIsolationMode);
            debugEnabled = debugEnabled || isAssetPreviewIsolationMode(renderIsolationMode);
        }
        if (runtimeDevToolsEnabledByDefault() && rawInput.assetPreviewNextPressed) {
            cycleAssetPreview(1);
        }
        if (runtimeDevToolsEnabledByDefault() && rawInput.assetPreviewPreviousPressed) {
            cycleAssetPreview(-1);
        }
        if (rawInput.toggleDebugPressed) {
            debugEnabled = runtimeDevToolsEnabledByDefault() && !debugEnabled;
        }
        if (runtimeDevToolsEnabledByDefault() && rawInput.visualBaselineTogglePressed) {
            visualBaselineDebug.cycle(level.visualBaselines);
        }
        if (runtimeDevToolsEnabledByDefault() && rawInput.posePreviewTogglePressed) {
            characterPosePreview.toggle();
        }
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
        if (runtimeDevToolsEnabledByDefault() && rawInput.toggleRuntimeEditorPressed) {
            runtimeEditorOpen = !runtimeEditorOpen;
            if (runtimeEditorOpen && mouseCaptured) {
                mouseCaptured = false;
                platform.releaseCursor();
            }
            rawInput.mouseLookActive = mouseCaptured;
            rawInput.mouseDeltaX = 0.0f;
            rawInput.mouseDeltaY = 0.0f;
        }
#endif
        if (runtimeDevToolsEnabledByDefault() && characterPosePreview.active()) {
            if (rawInput.posePreviewNextPressed) {
                characterPosePreview.next();
            }
            if (rawInput.posePreviewPreviousPressed) {
                characterPosePreview.previous();
            }
        }

        if (rawInput.mouseCaptureTogglePressed) {
            mouseCaptured = !mouseCaptured;
            if (mouseCaptured) {
                platform.captureCursor();
            } else {
                platform.releaseCursor();
            }
            rawInput.mouseLookActive = mouseCaptured;
            rawInput.mouseDeltaX = 0.0f;
            rawInput.mouseDeltaY = 0.0f;
        }
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
        if (runtimeEditorOpen) {
            rawInput.mouseLookActive = false;
            rawInput.mouseDeltaX = 0.0f;
            rawInput.mouseDeltaY = 0.0f;
        }
#endif
    }

    RuntimeRenderSnapshot captureRenderSnapshot() const {
        RuntimeRenderSnapshot snapshot;
        snapshot.worldObjects = level.objects;
        snapshot.playerPosition = player.position();
        snapshot.playerYawRadians = player.yawRadians();
        snapshot.vehicle = vehicle.runtimeState();
        snapshot.chaserPosition = chaserPosition;
        snapshot.chaserYawRadians = chaserYaw;
        snapshot.camera = camera;
        snapshot.cameraYawRadians = cameraRig.state().cameraYawRadians;
        return snapshot;
    }

    void resetRenderSnapshots() {
        currentRenderSnapshot = captureRenderSnapshot();
        previousRenderSnapshot = currentRenderSnapshot;
    }

    ControlContext currentControlContext() const {
        ControlContextState state;
        state.playerInVehicle = playerInVehicle;
        state.highPrzypal = mission.phase() == MissionPhase::ChaseActive;
        state.carryingObject = propSimulation.carrying();
        return resolveControlContext(state);
    }

    WorldLocationTag resolveLocation(Vec3 position) const {
        WorldLocationTag bestTag = WorldLocationTag::Unknown;
        int bestPriority = -1;
        float bestDistanceSq = 0.0f;

        for (const LocationAnchor& anchor : level.locationAnchors) {
            const float distanceSq = distanceSquaredXZ(position, anchor.position);
            if (distanceSq > anchor.radius * anchor.radius) {
                continue;
            }
            if (anchor.priority > bestPriority ||
                (anchor.priority == bestPriority && distanceSq < bestDistanceSq)) {
                bestTag = anchor.tag;
                bestPriority = anchor.priority;
                bestDistanceSq = distanceSq;
            }
        }

        return bestTag;
    }

    VehicleSurfaceResponse vehicleSurfaceAt(Vec3 position) const {
        for (const WorldObject& object : level.objects) {
            if (!pointInsideObjectXZ(object, position, 0.35f)) {
                continue;
            }
            if (hasGameplayTag(object, "grass_patch") || object.assetId.find("grass") != std::string::npos) {
                return {0.58f, 1.55f, 0.82f, 0.55f};
            }
            if (hasGameplayTag(object, "asphalt_patch") ||
                hasGameplayTag(object, "drive_route") ||
                hasGameplayTag(object, "drive_space") ||
                object.assetId.find("asphalt") != std::string::npos ||
                object.assetId.find("parking_surface") != std::string::npos) {
                return {1.0f, 1.0f, 1.0f, 0.10f};
            }
            if (hasGameplayTag(object, "curb")) {
                return {0.84f, 1.25f, 0.90f, 0.40f};
            }
        }

        switch (resolveLocation(position)) {
        case WorldLocationTag::Shop:
        case WorldLocationTag::Parking:
        case WorldLocationTag::Garage:
        case WorldLocationTag::RoadLoop:
            return {0.96f, 1.04f, 0.98f, 0.12f};
        case WorldLocationTag::Trash:
            return {0.78f, 1.25f, 0.90f, 0.30f};
        case WorldLocationTag::Block:
            return {0.74f, 1.25f, 0.90f, 0.30f};
        case WorldLocationTag::Unknown:
            return {0.68f, 1.35f, 0.86f, 0.35f};
        }
        return {};
    }

    void emitWorldEvent(WorldEventType type, Vec3 position, float intensity, const std::string& source) {
        eventEmitter.emit(worldEvents,
                          przypal,
                          feedback,
                          lastPrzypalReason,
                          type,
                          resolveLocation(position),
                          position,
                          intensity,
                          source);
    }

    void updateWorldSystems(float deltaSeconds) {
        worldEvents.update(deltaSeconds);
        eventCooldowns.update(deltaSeconds);
        przypal.update(deltaSeconds);
        reactionToastSeconds = std::max(0.0f, reactionToastSeconds - deltaSeconds);

        if (przypal.consumeBandPulse()) {
            feedback.trigger(FeedbackEvent::PrzypalNotice);
        }
    }

    bool missionUiBusy() const {
        return dialogue.hasBlockingLine() ||
               mission.phase() == MissionPhase::ChaseActive ||
               mission.phase() == MissionPhase::Failed ||
               mission.phase() == MissionPhase::Completed;
    }

    void updateWorldReactions(float deltaSeconds) {
        const ReactionIntent intent =
            reactionSystem.update(worldEvents, przypal, interactionActorPosition(), missionUiBusy(), deltaSeconds);
        if (!intent.available) {
            return;
        }

        lastReaction = intent;
        reactionToastSeconds = intent.hudOnly ? 2.0f : 1.25f;
        feedback.trigger(intent.feedbackEvent);
        if (!intent.hudOnly) {
            dialogue.push({intent.speaker, intent.text, 2.3f, DialogueChannel::Reaction});
        }
    }

    void applyFeedbackPressure() {
        const float chasePressure = mission.phase() == MissionPhase::ChaseActive ? chase.pressure() : 0.0f;
        const float heatPressure = przypal.normalizedIntensity() * 0.35f;
        feedback.setWorldTension(resolveCameraWorldTension(chasePressure, heatPressure));
    }

    void refreshRuntimeCollision() {
        collision.clearDynamic();
        propSimulation.publishCollision(collision);

        CollisionProfile npcProfile = CollisionProfile::playerBlocker();
        npcProfile.ownerId = 8101;
        collision.addDynamicBox(level.npcPosition + Vec3{0.0f, 0.85f, 0.0f},
                                {0.72f, 1.70f, 0.72f},
                                npcProfile);

        if (!playerInVehicle) {
            VehiclePlayerCollisionConfig vehicleCollision;
            vehicleCollision.platformVelocity = vehiclePlatformVelocity();
            collision.addDynamicVehiclePlayerCollision(vehicle.position(), vehicle.yawRadians(), vehicleCollision);
        }
    }

    void fixedUpdate(const RawInputState& rawInput, float deltaSeconds) {
        previousRenderSnapshot = currentRenderSnapshot;
        lastDeltaSeconds = deltaSeconds;
        elapsedSeconds += deltaSeconds;
        InputState input = inputRouter.mapToGameplayInput(rawInput, currentControlContext());
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
        if (runtimeEditorOpen) {
            input.moveForward = false;
            input.moveBackward = false;
            input.moveLeft = false;
            input.moveRight = false;
            input.walk = false;
            input.sprint = false;
            input.jumpPressed = false;
            input.accelerate = false;
            input.brake = false;
            input.steerLeft = false;
            input.steerRight = false;
            input.handbrake = false;
            input.vehicleBoost = false;
            input.hornPressed = false;
            input.hornDown = false;
            input.enterExitPressed = false;
            input.primaryActionPressed = false;
            input.secondaryActionPressed = false;
            input.carryActionPressed = false;
            input.pushActionPressed = false;
            input.drinkActionPressed = false;
            input.smokeActionPressed = false;
            input.lowVaultActionPressed = false;
            input.fallActionPressed = false;
            input.mouseCaptureTogglePressed = false;
            input.mouseLookActive = false;
            input.mouseDeltaX = 0.0f;
            input.mouseDeltaY = 0.0f;
            const CameraRigState& editorRigState = cameraRig.state();
            gizmo.update(input, runtimeEditor, editorRigState.position, editorRigState.target, level.objects);
        }
#endif
        lastRawInput = rawInput;

        updatePhaseClock(deltaSeconds);
        vehicleLockedHintCooldown = std::max(0.0f, vehicleLockedHintCooldown - deltaSeconds);
        interactionCameraSeconds = std::max(0.0f, interactionCameraSeconds - deltaSeconds);
        characterNpcBumpCooldownSeconds = std::max(0.0f, characterNpcBumpCooldownSeconds - deltaSeconds);

        input.cameraYawRadians = cameraRig.beginFrame(cameraControlInput(input), deltaSeconds);
        lastInput = input;

        if (mission.phase() == MissionPhase::Failed && input.retryPressed) {
            resetForRetry();
        }

        dialogue.update(deltaSeconds);
        feedback.update(deltaSeconds);
        updateWorldSystems(deltaSeconds);
        collision.clearDynamic();
        propSimulation.update(deltaSeconds, collision);
        propSimulation.syncWorldObjects(level.objects);
        refreshRuntimeCollision();
        updateInteraction(rawInput, input);
        updateCharacterActionFollowUp(deltaSeconds);
        updateCharacterActions(input);
        player.setStatus(PlayerStatus::Scared,
                         characterShouldBeScared(mission.phase() == MissionPhase::ChaseActive, przypal.band()));

        if (playerInVehicle) {
            updateVehicle(input, deltaSeconds);
            player.syncToVehicleSeat(vehicle.position(), vehicle.yawRadians());
        } else {
            updatePlayer(input, deltaSeconds);
            handleNpcBodyBump(input);
            propSimulation.updateCarried(player.position(), forwardFromYaw(player.yawRadians()));
            propSimulation.applyPlayerContact(player.position(), player.velocity(), 0.45f);
        }
        if (!playerInVehicle && player.motorState().impactIntensity > 0.05f) {
            feedback.triggerComedyEvent(player.motorState().impactIntensity);
            propSimulation.applyImpulseNear(player.position(),
                                            1.0f,
                                            normalizeXZ(player.velocity()) *
                                                (player.motorState().impactIntensity * 2.0f + 0.5f));
            if (player.motorState().impactIntensity > 0.25f &&
                eventCooldowns.consume(WorldEventType::PublicNuisance, "player_bump", 2.0f)) {
                emitWorldEvent(WorldEventType::PublicNuisance,
                               player.position(),
                               std::clamp(player.motorState().impactIntensity, 0.25f, 1.0f),
                               "player_bump");
            }
        }
        playerPresentation.update(player.motorState(),
                                  deltaSeconds,
                                  feedback.worldTension(),
                                  playerPresentationContext(input));

        updateMissionTriggers();
        updateChase(deltaSeconds);
        updateWorldReactions(deltaSeconds);
        applyFeedbackPressure();
        updateCamera(deltaSeconds);
        currentRenderSnapshot = captureRenderSnapshot();
    }

    void updatePhaseClock(float deltaSeconds) {
        const MissionPhase current = mission.phase();
        if (current != observedPhase) {
            observedPhase = current;
            phaseSeconds = 0.0f;
            if (current == MissionPhase::ReachVehicle ||
                current == MissionPhase::DriveToShop ||
                current == MissionPhase::ChaseActive ||
                current == MissionPhase::ReturnToLot) {
                captureMissionCheckpoint("phase_" + std::to_string(static_cast<int>(current)));
            }
            return;
        }

        phaseSeconds += deltaSeconds;
    }

    Vec3 interactionActorPosition() const {
        return playerInVehicle ? vehicle.position() : player.position();
    }

    bool playerCanReachVehicleDoor() const {
        if (playerInVehicle) {
            return true;
        }
        return std::fabs(player.position().y - vehicle.position().y) <= 0.55f;
    }

    void addPointInteractionCandidates(InteractionResolver& resolver) const {
        for (const InteractionPoint& point : interactions.points()) {
            if (point.id == "npc_start" && mission.phase() == MissionPhase::WaitingForStart) {
                resolver.addCandidate({point.id,
                                       InteractionAction::StartMission,
                                       InteractionInput::Use,
                                       point.prompt,
                                       point.position,
                                       point.radius,
                                       100});
            } else if (point.id == "npc_start" && mission.phase() == MissionPhase::ReturnToBench) {
                resolver.addCandidate({"npc_return_bogus",
                                       InteractionAction::Talk,
                                       InteractionInput::Use,
                                       "E: wróć do Bogusia",
                                       point.position,
                                       point.radius,
                                       100});
            } else if (point.id == "npc_start" &&
                       mission.phase() == MissionPhase::Completed &&
                       paragonMission.phase() == ParagonMissionPhase::Locked) {
                resolver.addCandidate({"paragon_start_bogus",
                                       InteractionAction::Talk,
                                       InteractionInput::Use,
                                       paragonActorPrompt("bogus", "E: Paragon Grozy"),
                                       point.position,
                                       point.radius,
                                       100,
                                       story.introCompleted});
            } else if (point.id == "npc_zenon" &&
                       paragonMission.phase() == ParagonMissionPhase::TalkToZenon) {
                resolver.addCandidate({"paragon_zenon_accuse",
                                       InteractionAction::Talk,
                                       InteractionInput::Use,
                                       "E: Zenon: pokaz dowody",
                                       point.position,
                                       point.radius,
                                       110});
            } else if (point.id == "npc_zenon" &&
                       paragonMission.phase() == ParagonMissionPhase::ReturnToZenon) {
                resolver.addCandidate({"paragon_return_zenon",
                                       InteractionAction::Talk,
                                       InteractionInput::Use,
                                       paragonMission.shopBanActive() ? "E: oddaj przez okienko" : "E: oddaj paragon Zenonowi",
                                       point.position,
                                       point.radius,
                                       110});
            } else if (point.id == "npc_zenon" &&
                       paragonMission.phase() == ParagonMissionPhase::Completed &&
                       story.paragonCompleted) {
                const ShopServiceState service = currentShopServiceState();
                resolver.addCandidate({"paragon_zenon_post",
                                       InteractionAction::Talk,
                                       InteractionInput::Use,
                                       service.prompt,
                                       point.position,
                                       point.radius,
                                       55});
            } else if (point.id == "npc_zenon" &&
                       mission.phase() == MissionPhase::Completed &&
                       paragonMission.phase() == ParagonMissionPhase::Locked) {
                const ShopServiceState service = currentShopServiceState();
                resolver.addCandidate({"shop_service_zenon",
                                       InteractionAction::Talk,
                                       InteractionInput::Use,
                                       service.prompt,
                                       point.position,
                                       point.radius,
                                       45});
            } else if (const auto service = currentLocalRewirServiceForPoint(point.id);
                       service.has_value() && mission.phase() == MissionPhase::Completed) {
                resolver.addCandidate({service->interactionId,
                                       InteractionAction::Talk,
                                       InteractionInput::Use,
                                       service->prompt,
                                       point.position,
                                       point.radius,
                                       service->priority,
                                       story.introCompleted});
            } else if (const auto favor = currentLocalRewirFavorForPoint(point.id);
                       favor.has_value() && mission.phase() == MissionPhase::Completed) {
                resolver.addCandidate({favor->id,
                                       InteractionAction::Talk,
                                       InteractionInput::Use,
                                       favor->prompt,
                                       point.position,
                                       point.radius,
                                       favor->priority,
                                       story.introCompleted});
            } else if (point.id == "npc_lolek" &&
                       paragonMission.phase() == ParagonMissionPhase::FindReceiptHolder) {
                resolver.addCandidate({"paragon_lolek_clue",
                                       InteractionAction::Talk,
                                       InteractionInput::Use,
                                       paragonActorPrompt("lolek", "E: spytaj Lolka"),
                                       point.position,
                                       point.radius,
                                       95});
            } else if (point.id == "receipt_holder" &&
                       paragonMission.phase() == ParagonMissionPhase::ResolveReceipt) {
                resolver.addCandidate({"paragon_receipt_peaceful",
                                       InteractionAction::Talk,
                                       InteractionInput::Use,
                                       paragonChoicePrompt(ParagonSolution::Peaceful, "E: dogadaj paragon"),
                                       point.position,
                                       point.radius,
                                       100});
                resolver.addCandidate({"paragon_receipt_trick",
                                       InteractionAction::Talk,
                                       InteractionInput::Vehicle,
                                       paragonChoicePrompt(ParagonSolution::Trick, "F: zakombinuj"),
                                       point.position,
                                       point.radius,
                                       95});
            }
        }
    }

    void addVehicleInteractionCandidates(InteractionResolver& resolver) const {
        const bool vehicleUnlocked = story.gruzUnlocked || mission.phase() == MissionPhase::ReachVehicle ||
                                     mission.phase() == MissionPhase::DriveToShop ||
                                     mission.phase() == MissionPhase::ReturnToLot ||
                                     mission.phase() == MissionPhase::Completed ||
                                     mission.phase() == MissionPhase::Failed;
        const bool vehicleInteractionAllowed = playerCanReachVehicleDoor();
        if (!vehicleUnlocked && vehicleInteractionAllowed) {
            resolver.addCandidate({"vehicle_locked",
                                   InteractionAction::Talk,
                                   InteractionInput::Use,
                                   "E: najpierw Zenon i Boguś",
                                   vehicle.position(),
                                   2.6f,
                                   70});
            resolver.addCandidate({"vehicle_locked",
                                   InteractionAction::Talk,
                                   InteractionInput::Vehicle,
                                   "F: najpierw Zenon i Boguś",
                                   vehicle.position(),
                                   2.6f,
                                   70});
        }
        if (vehicleInteractionAllowed) {
            resolver.addCandidate({"player_vehicle",
                                   InteractionAction::EnterVehicle,
                                   InteractionInput::Vehicle,
                                   "F: wsiadz do gruza",
                                   vehicle.position(),
                                   2.4f,
                                   80,
                                   vehicleUnlocked});
            resolver.addCandidate({"player_vehicle",
                                   InteractionAction::EnterVehicle,
                                   InteractionInput::Use,
                                   "E: wsiadz do gruza",
                                   vehicle.position(),
                                   2.4f,
                                   35,
                                   vehicleUnlocked});
        }
    }

    InteractionResolver buildInteractionResolver() const {
        InteractionResolver resolver;
        if (playerInVehicle) {
            const bool canExitVehicle = mission.phase() != MissionPhase::ChaseActive;
            resolver.addCandidate({"vehicle_exit",
                                   InteractionAction::ExitVehicle,
                                   InteractionInput::Use,
                                   "E: wysiadz z auta",
                                   vehicle.position(),
                                   3.0f,
                                   100,
                                   canExitVehicle});
            resolver.addCandidate({"vehicle_exit",
                                   InteractionAction::ExitVehicle,
                                   InteractionInput::Vehicle,
                                   "F: wysiadz z auta",
                                   vehicle.position(),
                                   3.0f,
                                   100,
                                   canExitVehicle});
            return resolver;
        }

        addPointInteractionCandidates(resolver);

        for (const InteractionCandidate& candidate : buildWorldObjectInteractionCandidates(level.objects)) {
            resolver.addCandidate(candidate);
        }

        addVehicleInteractionCandidates(resolver);
        return resolver;
    }

    void executeInteraction(const InteractionResolution& interaction) {
        if (!interaction.available) {
            return;
        }

        switch (interaction.action) {
        case InteractionAction::StartMission:
            playerPresentation.playOneShot(CharacterPoseKind::Interact, 0.42f);
            mission.start();
            startInteractionCamera();
            feedback.trigger(FeedbackEvent::MarkerReached);
            break;
        case InteractionAction::Talk:
            if (interaction.id == "npc_return_bogus" && mission.phase() == MissionPhase::ReturnToBench) {
                playerPresentation.playOneShot(CharacterPoseKind::Talk, 0.85f);
                mission.onReturnedToBogus();
                story.onReturnedToBogus();
                startInteractionCamera();
                feedback.trigger(FeedbackEvent::MarkerReached);
            } else if (interaction.id == "vehicle_locked" && vehicleLockedHintCooldown <= 0.0f) {
                playerPresentation.playOneShot(CharacterPoseKind::Talk, 0.70f);
                dialogue.push({"Misja", "Najpierw ogarnij Zenona i wróć do Bogusia.", 3.2f, DialogueChannel::SystemHint});
                feedback.trigger(FeedbackEvent::MarkerReached);
                vehicleLockedHintCooldown = 2.5f;
            } else if (interaction.id == "paragon_start_bogus") {
                playerPresentation.playOneShot(CharacterPoseKind::Talk, 0.85f);
                paragonMission.start(story.introCompleted);
                startInteractionCamera();
                feedback.trigger(FeedbackEvent::MarkerReached);
            } else if (interaction.id == "paragon_zenon_accuse") {
                playerPresentation.playOneShot(CharacterPoseKind::Talk, 0.85f);
                emitParagonPulse(paragonMission.onZenonAccusesBogus());
                startInteractionCamera();
            } else if (interaction.id == "paragon_lolek_clue") {
                playerPresentation.playOneShot(CharacterPoseKind::Talk, 0.85f);
                paragonMission.onReceiptHolderFound();
                feedback.trigger(FeedbackEvent::MarkerReached);
            } else if (interaction.id == "paragon_receipt_peaceful") {
                playerPresentation.playOneShot(CharacterPoseKind::Interact, 0.58f);
                emitParagonPulse(paragonMission.resolve(ParagonSolution::Peaceful));
            } else if (interaction.id == "paragon_receipt_trick") {
                playerPresentation.playOneShot(CharacterPoseKind::Interact, 0.62f);
                emitParagonPulse(paragonMission.resolve(ParagonSolution::Trick));
            } else if (interaction.id == "paragon_return_zenon") {
                playerPresentation.playOneShot(CharacterPoseKind::Talk, 0.85f);
                const ParagonSolution finishedSolution = paragonMission.solution();
                paragonMission.onReturnedToZenon();
                story.onParagonResolved(finishedSolution);
                feedback.trigger(FeedbackEvent::MissionComplete);
                startInteractionCamera();
            } else if (interaction.id == "paragon_zenon_post") {
                playerPresentation.playOneShot(CharacterPoseKind::Talk, 0.75f);
                const std::string line = paragonMission.zenonShopLineAfterCompletion();
                const ShopServiceState service = currentShopServiceState();
                dialogue.push({"Zenon", line.empty() ? service.line : line, 3.4f, DialogueChannel::MissionCritical});
                startInteractionCamera();
            } else if (interaction.id == "shop_service_zenon") {
                playerPresentation.playOneShot(CharacterPoseKind::Talk, 0.75f);
                const ShopServiceState service = currentShopServiceState();
                dialogue.push({"Zenon", service.line, 3.2f, DialogueChannel::MissionCritical});
                startInteractionCamera();
            } else if (const auto favor = localRewirFavorById(interaction.id);
                       favor.has_value() && !localRewirFavorCompleted(favor->id)) {
                playerPresentation.playOneShot(CharacterPoseKind::Interact, 0.62f);
                dialogue.push({"Rewir", favor->startLine, 2.8f, DialogueChannel::MissionCritical});
                dialogue.push({"Rewir", favor->completeLine, 2.4f, DialogueChannel::SystemHint});
                completeLocalRewirFavor(favor->id);
                applyLocalRewirServiceRelief(worldEvents, interactionActorPosition(), favor->serviceInteractionId);
                persistCurrentSave(missionCheckpoint.id.empty() ? "local_rewir_favor" : missionCheckpoint.id);
                feedback.trigger(FeedbackEvent::MarkerReached);
                startInteractionCamera();
            } else if (const auto service = currentLocalRewirServiceForInteraction(interaction.id);
                       service.has_value()) {
                playerPresentation.playOneShot(CharacterPoseKind::Talk, 0.75f);
                dialogue.push({service->speaker, service->line, 3.2f, DialogueChannel::MissionCritical});
                if (service->access == LocalRewirServiceAccess::Wary) {
                    const LocalRewirServiceReliefResult relief =
                        applyLocalRewirServiceRelief(worldEvents, interactionActorPosition(), interaction.id);
                    if (relief.softenedEvents > 0) {
                        if (!relief.feedbackLine.empty()) {
                            dialogue.push({"Rewir", relief.feedbackLine, 2.2f, DialogueChannel::SystemHint});
                        }
                        feedback.trigger(FeedbackEvent::MarkerReached);
                    }
                }
                startInteractionCamera();
            }
            break;
        case InteractionAction::EnterVehicle:
            if (!playerInVehicle) {
                playerPresentation.playOneShot(CharacterPoseKind::EnterVehicle, 0.58f);
                playerInVehicle = true;
                interactionCameraSeconds = 0.0f;
                resetDriveRoute();
                vehicle.setYaw(driveRoute.yawFrom(vehicle.position()));
                mission.onPlayerEnteredVehicle();
                player.syncToVehicleSeat(vehicle.position(), vehicle.yawRadians());
                cameraRig.reset(cameraTarget());
                feedback.trigger(FeedbackEvent::MarkerReached);
                audio.playStart();
            }
            break;
        case InteractionAction::ExitVehicle:
            if (playerInVehicle && mission.phase() != MissionPhase::ChaseActive) {
                const Vec3 right = screenRightFromYaw(vehicle.yawRadians());
                const VehicleExitResolution exit =
                    resolveVehicleExitPosition(collision, vehicle.position(), vehicle.yawRadians(), right);
                if (!exit.found) {
                    dialogue.push({"Misja", "Nie ma miejsca zeby wysiasc.", 2.0f, DialogueChannel::SystemHint});
                    feedback.trigger(FeedbackEvent::MarkerReached);
                    return;
                }
                player.syncAfterVehicleExit(exit.position, vehicle.yawRadians());
                playerInVehicle = false;
                playerPresentation.playOneShot(CharacterPoseKind::ExitVehicle, 0.52f);
            }
            break;
        case InteractionAction::None:
            break;
        case InteractionAction::UseMarker:
        case InteractionAction::UseDoor:
        case InteractionAction::PickUp: {
            const std::optional<WorldObjectInteractionAffordance> affordance =
                findWorldObjectInteractionAffordance(level.objects, interaction.id);
            if (affordance.has_value()) {
                const WorldObjectInteractionAffordance resolvedAffordance =
                    worldObjectAffordanceWithOutcomeData(*affordance, &gameWorld.dataCatalog.objectOutcomes);
                playerPresentation.playOneShot(CharacterPoseKind::Interact, 0.46f);
                dialogue.push({resolvedAffordance.speaker,
                               resolvedAffordance.line,
                               2.6f,
                               DialogueChannel::SystemHint});
                feedback.trigger(FeedbackEvent::MarkerReached);
                if (const std::optional<WorldObjectInteractionEvent> event =
                        worldEventForObjectAffordance(resolvedAffordance, &gameWorld.dataCatalog.objectOutcomes)) {
                    if (eventCooldowns.consume(event->type, event->source, event->cooldownSeconds)) {
                        emitWorldEvent(event->type, event->position, event->intensity, event->source);
                    }
                }
                handleMissionTriggerResult(missionOutcomeTriggerForCurrentPhase(gameWorld.dataCatalog.mission,
                                                                                mission.phase(),
                                                                                resolvedAffordance.outcomeId,
                                                                                resolvedAffordance.position,
                                                                                resolvedAffordance.radius));
            }
            break;
        }
        }
    }

    void updateInteraction(const RawInputState& rawInput, const InputState& input) {
        if (!input.enterExitPressed) {
            return;
        }

        const InteractionInput requestedInput = rawInput.vehiclePressed && !rawInput.usePressed
                                                    ? InteractionInput::Vehicle
                                                    : InteractionInput::Use;
        const InteractionResolver resolver = buildInteractionResolver();
        executeInteraction(resolver.resolve(interactionActorPosition(), requestedInput));
    }

    void queueCharacterFollowUp(const CharacterActionRequest& request) {
        if (!request.followUpAvailable) {
            characterFollowUpDelaySeconds = 0.0f;
            characterFollowUpDurationSeconds = 0.0f;
            characterFollowUpPose = CharacterPoseKind::Idle;
            return;
        }
        characterFollowUpDelaySeconds = request.followUpDelaySeconds;
        characterFollowUpDurationSeconds = request.followUpDurationSeconds;
        characterFollowUpPose = request.followUpPoseKind;
    }

    void queueCharacterReactionFollowUp(const CharacterReactionApplicationResult& result) {
        if (!result.followUpAvailable) {
            return;
        }
        characterFollowUpDelaySeconds = result.followUpDelaySeconds;
        characterFollowUpDurationSeconds = result.followUpDurationSeconds;
        characterFollowUpPose = result.followUpPoseKind;
    }

    void emitParagonPulse(const ParagonMissionPulse& pulse) {
        if (!pulse.available) {
            return;
        }
        if (eventCooldowns.consume(pulse.type, pulse.source, 2.0f)) {
            emitWorldEvent(pulse.type, level.shopEntrancePosition, pulse.intensity, pulse.source);
        }
        feedback.trigger(pulse.type == WorldEventType::ShopTrouble ? FeedbackEvent::PrzypalNotice
                                                                    : FeedbackEvent::MarkerReached);
    }

    bool tryResolveParagonChaosFromAction() {
        if (paragonMission.phase() != ParagonMissionPhase::ResolveReceipt ||
            distanceXZ(player.position(), level.receiptHolderPosition) > 2.45f) {
            return false;
        }

        emitParagonPulse(paragonMission.resolve(ParagonSolution::Chaos));
        return true;
    }

    std::string paragonActorPrompt(const std::string& actorId, const std::string& fallback) const {
        const ParagonActorSpec* actor = defaultParagonMissionSpec().actor(actorId);
        return actor != nullptr && !actor->prompt.empty() ? actor->prompt : fallback;
    }

    std::string paragonChoicePrompt(ParagonSolution solution, const std::string& fallback) const {
        const ParagonChoiceSpec* choice = defaultParagonMissionSpec().choice(solution);
        return choice != nullptr && !choice->prompt.empty() ? choice->prompt : fallback;
    }

    ShopServiceState currentShopServiceState() const {
        return resolveShopServiceState(worldEvents, story.shopBanActive || paragonMission.shopBanActive());
    }

    std::optional<LocalRewirServiceState> currentLocalRewirServiceForPoint(const std::string& interactionPointId) const {
        return resolveLocalRewirServiceState(worldEvents, interactionActorPosition(), interactionPointId);
    }

    std::optional<LocalRewirServiceState> currentLocalRewirServiceForInteraction(const std::string& interactionId) const {
        return resolveLocalRewirServiceStateForInteraction(worldEvents, interactionActorPosition(), interactionId);
    }

    bool localRewirFavorCompleted(const std::string& id) const {
        return std::find(completedLocalRewirFavorIds.begin(), completedLocalRewirFavorIds.end(), id) !=
               completedLocalRewirFavorIds.end();
    }

    void completeLocalRewirFavor(const std::string& id) {
        if (!localRewirFavorCompleted(id)) {
            completedLocalRewirFavorIds.push_back(id);
        }
    }

    std::optional<LocalRewirFavorState> currentLocalRewirFavorForPoint(const std::string& interactionPointId) const {
        std::optional<LocalRewirFavorState> favor =
            resolveLocalRewirFavorState(worldEvents, interactionActorPosition(), interactionPointId);
        if (!favor.has_value() || localRewirFavorCompleted(favor->id)) {
            return std::nullopt;
        }

        return favor;
    }

    void updateCharacterActionFollowUp(float deltaSeconds) {
        if (characterFollowUpDelaySeconds <= 0.0f) {
            return;
        }
        characterFollowUpDelaySeconds = std::max(0.0f, characterFollowUpDelaySeconds - deltaSeconds);
        if (characterFollowUpDelaySeconds <= 0.0f &&
            characterFollowUpPose != CharacterPoseKind::Idle &&
            !playerPresentation.oneShotActive()) {
            playerPresentation.playOneShot(characterFollowUpPose, characterFollowUpDurationSeconds);
            characterFollowUpPose = CharacterPoseKind::Idle;
            characterFollowUpDurationSeconds = 0.0f;
        }
    }

    void handleNpcBodyBump(const InputState& input) {
        if (characterNpcBumpCooldownSeconds > 0.0f) {
            return;
        }

        Vec3 attemptedDirection = PlayerInputIntent::fromInputState(input).moveDirection;
        if (lengthSquaredXZ(attemptedDirection) <= 0.0001f) {
            attemptedDirection = forwardFromYaw(player.yawRadians());
        }

        constexpr int BogusNpcOwnerId = 8101;
        const CharacterWorldHit hit =
            resolveCharacterNpcBodyHit(player.motorState(), BogusNpcOwnerId, attemptedDirection);
        if (!hit.available) {
            return;
        }

        const CharacterPhysicalReaction reaction = resolveCharacterPhysicalReaction(hit.event);
        const CharacterReactionApplicationResult result =
            applyCharacterPhysicalReaction(player, playerPresentation, reaction);
        if (!result.applied) {
            return;
        }

        queueCharacterReactionFollowUp(result);
        feedback.triggerComedyEvent(result.feedbackIntensity * 0.35f);
        characterNpcBumpCooldownSeconds = 0.85f;
    }

    CharacterVaultResult evaluateLowVault() const {
        CharacterVaultRequest vaultRequest;
        vaultRequest.position = player.position();
        vaultRequest.forward = forwardFromYaw(player.yawRadians());
        return resolveCharacterLowVault(collision, vaultRequest);
    }

    bool handleCarryAction(const CharacterActionRequest& request) {
        if (request.poseKind != CharacterPoseKind::CarryObject) {
            return false;
        }

        if (paragonMission.phase() == ParagonMissionPhase::ResolveReceipt &&
            distanceXZ(player.position(), level.receiptHolderPosition) <= 2.45f) {
            playerPresentation.playOneShot(CharacterPoseKind::CarryObject, 0.62f);
            emitParagonPulse(paragonMission.resolve(ParagonSolution::Trick));
            feedback.triggerComedyEvent(0.18f);
            return true;
        }

        const Vec3 forward = forwardFromYaw(player.yawRadians());
        if (propSimulation.carrying()) {
            propSimulation.dropCarried(player.velocity() + forward * 1.5f);
            playerPresentation.playOneShot(CharacterPoseKind::CarryObject, 0.32f);
            feedback.triggerComedyEvent(0.10f);
            return true;
        }

        const PropCarryResult carry = propSimulation.tryBeginCarry(player.position(), forward, 1.12f);
        if (!carry.started) {
            return true;
        }

        playerPresentation.playOneShot(CharacterPoseKind::CarryObject, request.durationSeconds);
        feedback.triggerComedyEvent(0.08f);
        return true;
    }

    void applyCharacterActionEffects(const CharacterActionRequest& request,
                                     const InputState& input,
                                     const CharacterVaultResult* plannedVault = nullptr) {
        const Vec3 forward = forwardFromYaw(player.yawRadians());
        switch (request.poseKind) {
        case CharacterPoseKind::Punch:
            if (tryResolveParagonChaosFromAction()) {
                feedback.triggerComedyEvent(0.38f);
            }
            propSimulation.applyImpulseNear(player.position() + forward * 0.72f, 1.05f, forward * 2.6f);
            feedback.triggerComedyEvent(0.22f);
            if (eventCooldowns.consume(WorldEventType::PublicNuisance, "player_punch", 1.4f)) {
                emitWorldEvent(WorldEventType::PublicNuisance, player.position(), 0.18f, "player_punch");
            }
            break;
        case CharacterPoseKind::PushObject:
            if (tryResolveParagonChaosFromAction()) {
                feedback.triggerComedyEvent(0.34f);
            }
            propSimulation.applyImpulseNear(player.position() + forward * 0.82f, 1.25f, forward * 3.4f);
            player.applyImpulse(forward * -0.32f);
            feedback.triggerComedyEvent(0.18f);
            break;
        case CharacterPoseKind::Dodge: {
            const Vec3 right = screenRightFromYaw(player.yawRadians());
            const float side = input.moveRight ? 1.0f : (input.moveLeft ? -1.0f : -1.0f);
            player.applyImpulse(right * side * 1.8f + forward * -0.35f);
            break;
        }
        case CharacterPoseKind::LowVault:
        {
            const CharacterVaultResult vault = plannedVault != nullptr ? *plannedVault : evaluateLowVault();
            if (vault.available) {
                player.applyImpulse(vault.takeoffImpulse);
                feedback.triggerComedyEvent(0.08f);
            }
            break;
        }
        case CharacterPoseKind::Fall: {
            CharacterImpactEvent event;
            event.source = CharacterImpactSource::Platform;
            event.impactSpeed = 5.4f;
            event.impactNormal = forward * -1.0f;
            event.grounded = player.isGrounded();
            const CharacterPhysicalReaction reaction = resolveCharacterPhysicalReaction(event);
            const CharacterReactionApplicationResult result =
                applyCharacterPhysicalReaction(player, playerPresentation, reaction);
            queueCharacterReactionFollowUp(result);
            feedback.triggerComedyEvent(result.feedbackIntensity * 0.55f);
            break;
        }
        case CharacterPoseKind::Drink:
        case CharacterPoseKind::Smoke:
        default:
            break;
        }
    }

    void updateCharacterActions(const InputState& input) {
        if (playerInVehicle || dialogue.hasBlockingLine() || mission.phase() == MissionPhase::Failed) {
            return;
        }
        const CharacterActionRequest request = resolveCharacterAction(input);
        if (!request.available) {
            return;
        }
        CharacterActionStartContext startContext;
        CharacterVaultResult vault;
        const bool isLowVault = request.poseKind == CharacterPoseKind::LowVault;
        if (isLowVault) {
            vault = evaluateLowVault();
            startContext.lowVaultAvailable = vault.available;
        }
        if (request.poseKind == CharacterPoseKind::PushObject) {
            startContext.pushTargetAvailable = propSimulation.hasMovablePropNear(player.position(),
                                                                                 forwardFromYaw(player.yawRadians()),
                                                                                 1.25f);
        }
        if (!canStartCharacterAction(request, startContext)) {
            return;
        }
        if (handleCarryAction(request)) {
            return;
        }
        playerPresentation.playOneShot(request.poseKind, request.durationSeconds);
        queueCharacterFollowUp(request);
        applyCharacterActionEffects(request, input, isLowVault ? &vault : nullptr);
    }

    void updatePlayer(const InputState& input, float deltaSeconds) {
        player.update(input, collision, deltaSeconds);
    }

    void updateVehicle(const InputState& input, float deltaSeconds) {
        const bool hornCanFire = input.hornPressed && vehicle.hornCooldown() <= 0.0f;
        const VehicleMoveProposal proposal = vehicle.previewMove(input, deltaSeconds, vehicleSurfaceAt(vehicle.position()));
        if (hornCanFire) {
            audio.playHorn();
            if (eventCooldowns.consume(WorldEventType::PublicNuisance, "horn", 2.0f)) {
                emitWorldEvent(WorldEventType::PublicNuisance, proposal.previousPosition, 0.45f, "horn");
            }
        }

        if (proposal.proposedState.boostActive &&
            eventCooldowns.consume(WorldEventType::PublicNuisance, "boost_noise", 3.0f)) {
            emitWorldEvent(WorldEventType::PublicNuisance, proposal.previousPosition, 0.35f, "boost_noise");
        }

        VehicleCollisionConfig vehicleCollision;
        vehicleCollision.speed = proposal.proposedState.speed;
        vehicleCollision.lateralSlip = proposal.proposedState.lateralSlip;
        WorldCollision driveCollision = collision;
        if (mission.phase() == MissionPhase::ChaseActive) {
            driveCollision.addDynamicVehiclePlayerCollision(chaserPosition, chaserYaw);
        }
        const VehicleCollisionResult resolved = driveCollision.resolveVehicleCapsule(
            proposal.previousPosition,
            proposal.desiredPosition,
            proposal.proposedState.yawRadians,
            vehicleCollision);
        lastVehicleCollision = resolved;
        const bool blockedByWorld = resolved.hit || distanceSquaredXZ(proposal.desiredPosition, resolved.position) > 0.0004f;
        vehicle.applyMoveResolution(proposal, resolved);
        if (mission.phase() == MissionPhase::DriveToShop) {
            driveRoute.update(vehicle.position());
        }
        const float impact = resolved.impactSpeed;
        if (blockedByWorld && impact > 2.0f) {
            const Vec3 impactDirection = normalizeXZ(proposal.desiredPosition - proposal.previousPosition);
            propSimulation.applyImpulseNear(resolved.position,
                                            2.2f,
                                            impactDirection * std::clamp(impact, 1.0f, 9.0f));
            feedback.triggerComedyEvent(std::min(1.0f, impact / vehicle.maxForwardSpeed()));
            audio.playBump();
            if (shouldEmitPropertyDamage(impact) &&
                eventCooldowns.consume(WorldEventType::PropertyDamage, "vehicle_impact", 2.5f)) {
                emitWorldEvent(WorldEventType::PropertyDamage,
                               resolved.position,
                               std::clamp(impact / 12.0f, 0.35f, 1.35f),
                               "vehicle_impact");
            }
        }
    }

    PlayerPresentationContext playerPresentationContext(const InputState& input) const {
        PlayerPresentationContext context;
        context.inVehicle = playerInVehicle;
        context.interactActive = explicitInteractionCameraActive();
        context.talkActive = !context.interactActive && dialogue.hasLine() && dialogue.hasBlockingLine();
        context.standingOnVehicle = !playerInVehicle && player.motorState().ground.ownerId != 0;
        context.carryingObject = propSimulation.carrying();
        context.steerInput = (input.steerRight ? 1.0f : 0.0f) - (input.steerLeft ? 1.0f : 0.0f);
        return context;
    }

    void handleMissionTriggerResult(const MissionTriggerResult& trigger) {
        if (!trigger.triggered) {
            return;
        }

        MissionRuntimeBridgeContext context{
            mission,
            story,
            feedback,
            eventCooldowns,
            vehicle.position(),
            [this](WorldEventType type, Vec3 position, float intensity, const std::string& source) {
                emitWorldEvent(type, position, intensity, source);
            }};
        missionRuntimeBridge.handleTrigger(trigger, context);
    }

    void updateMissionTriggers() {
        const MissionTriggerResult trigger = missionTriggers.update(
            mission.phase(),
            player.position(),
            vehicle.position(),
            playerInVehicle);
        handleMissionTriggerResult(trigger);
    }

    void updateChase(float deltaSeconds) {
        if (mission.consumeChaseWanted()) {
            chaserPosition = level.shopPosition + Vec3{-8.0f, 0.0f, -12.0f};
            chaserYaw = 0.0f;
            chaseAiState = {};
            lastChaseAiCommand = {};
            lastChaseVehicleStep = {};
            chaserVelocity = {};
            chase.start(chaserPosition, 2.0f);
            feedback.trigger(FeedbackEvent::ChaseWarning);
            if (eventCooldowns.consume(WorldEventType::ChaseSeen, "chase_start", 5.0f)) {
                emitWorldEvent(WorldEventType::ChaseSeen, vehicle.position(), 0.80f, "chase_start");
            }
        }

        if (mission.phase() != MissionPhase::ChaseActive) {
            feedback.setWorldTension(0.0f);
            return;
        }

        const Vec3 target = vehicle.position();
        const Vec3 targetVelocity = vehiclePlatformVelocity();
        const RewirPressureSnapshot chaseRewirPressure = resolveRewirPressure(worldEvents, target);
        const Vec3 losTarget = target + Vec3{0.0f, 1.1f, 0.0f};
        const Vec3 losPursuer = chaserPosition + Vec3{0.0f, 1.1f, 0.0f};
        const Vec3 losResolved = collision.resolveCameraBoom(losTarget, losPursuer, 0.18f);
        const bool lineOfSightBlocked = distanceXZ(losResolved, losPursuer) > 0.35f;
        ChaseAiInput chaseAiInput;
        chaseAiInput.pursuerPosition = chaserPosition;
        chaseAiInput.targetPosition = target;
        chaseAiInput.targetVelocity = targetVelocity;
        chaseAiInput.targetYawRadians = vehicle.yawRadians();
        chaseAiInput.deltaSeconds = deltaSeconds;
        chaseAiInput.elapsedSeconds = elapsedSeconds;
        chaseAiInput.collisionHit = lastChaseVehicleStep.collision.hit;
        chaseAiInput.separatedFromTarget = lastChaseVehicleStep.separatedFromTarget;
        const float przypalDifficulty = przypalDifficultyFactor(przypal.band());
        chaseAiInput.witnessedBySensor = distanceXZ(chaserPosition, target) <= 42.0f * przypalDifficulty;
        chaseAiInput.lineOfSightBlocked = lineOfSightBlocked;
        chaseAiInput.hasPatrolInterest = chaseRewirPressure.patrolInterest;
        chaseAiInput.patrolInterestPosition = chaseRewirPressure.position;
        chaseAiInput.patrolInterestRadius = std::clamp(3.0f + chaseRewirPressure.score, 4.0f, 8.0f);
        lastChaseAiCommand = updateChaseAiRuntime(chaseAiState, chaseAiInput);

        const float followSpeed = chase.pursuerFollowSpeed(distanceXZ(target, chaserPosition), vehicle.speedRatio()) *
                                  lastChaseAiCommand.speedScale;
        chase.setDifficulty(przypalDifficulty);
        ChaseVehicleRuntimeConfig chaseVehicleConfig;
        chaseVehicleConfig.minimumCenterSeparation = 3.65f / przypalDifficulty;
        chaseVehicleConfig.hasPursuitTarget = true;
        chaseVehicleConfig.pursuitTarget = lastChaseAiCommand.aimPoint;
        const Vec3 previousChaserPosition = chaserPosition;
        lastChaseVehicleStep = advanceChaseVehicle(chaserPosition,
                                                   chaserYaw,
                                                   target,
                                                   vehicle.yawRadians(),
                                                   followSpeed,
                                                   deltaSeconds,
                                                   collision,
                                                   chaseVehicleConfig);
        chaserPosition = lastChaseVehicleStep.position;
        chaserYaw = lastChaseVehicleStep.yawRadians;
        chaserVelocity = deltaSeconds <= 0.0001f ? Vec3{} : (chaserPosition - previousChaserPosition) / deltaSeconds;

        ChaseUpdateContext chaseContext;
        chaseContext.playerPosition = target;
        chaseContext.playerVelocity = targetVelocity;
        chaseContext.pursuerPosition = chaserPosition;
        chaseContext.pursuerVelocity = chaserVelocity;
        chaseContext.lineOfSight = lastChaseAiCommand.lineOfSight;
        chaseContext.contactRecoverActive = lastChaseAiCommand.contactRecoverActive;
        const ChaseResult result = chase.updateWithContext(chaseContext, deltaSeconds);
        feedback.setWorldTension(chase.pressure());
        if (chase.pressure() > 0.55f &&
            eventCooldowns.consume(WorldEventType::ChaseSeen, "chase_pressure", 5.0f)) {
            emitWorldEvent(WorldEventType::ChaseSeen, target, std::clamp(chase.pressure(), 0.55f, 1.0f), "chase_pressure");
        }
        if (result == ChaseResult::Caught) {
            mission.fail("pościg dopadł auto");
            feedback.trigger(FeedbackEvent::MissionFailed);
            chase.stop();
        } else if (result == ChaseResult::Escaped) {
            mission.onChaseEscaped();
            feedback.trigger(FeedbackEvent::MarkerReached);
            chase.stop();
        }
    }

    CameraRigControlInput cameraControlInput(const InputState& input) const {
        CameraRigControlInput control;
        const CameraFeedbackOutput cameraFeedback = resolvedCameraFeedback();
        control.mode = effectiveCameraMode();
        control.followYawRadians = playerInVehicle ? vehicle.yawRadians() : player.yawRadians();
        control.speedRatio = stableCameraMode ? 0.0f
                                              : playerInVehicle ? vehicle.speedRatio()
                                                                : distanceXZ(player.velocity(), {}) / player.sprintSpeed();
        control.chaseActive = !stableCameraMode && mission.phase() == MissionPhase::ChaseActive;
        control.tension = cameraFeedback.cameraTension;
        control.mouseLookActive = input.mouseLookActive;
        control.invertMouseX = invertMouseX;
        control.mouseDeltaX = input.mouseDeltaX;
        control.mouseDeltaY = input.mouseDeltaY;
        return control;
    }

    CameraRigTarget cameraTarget() const {
        CameraRigTarget target;
        const CameraFeedbackOutput cameraFeedback = resolvedCameraFeedback();
        target.position = playerInVehicle ? vehicle.position() : player.position();
        target.velocity = playerInVehicle ? vehiclePlatformVelocity() : player.velocity();
        target.yawRadians = playerInVehicle ? vehicle.yawRadians() : player.yawRadians();
        target.mode = effectiveCameraMode();
        target.speedRatio = stableCameraMode ? 0.0f
                                             : playerInVehicle ? vehicle.speedRatio()
                                                               : distanceXZ(player.velocity(), {}) / player.sprintSpeed();
        target.chaseActive = !stableCameraMode && mission.phase() == MissionPhase::ChaseActive;
        target.tension = cameraFeedback.cameraTension;
        return target;
    }

    CameraFeedbackOutput resolvedCameraFeedback() const {
        CameraFeedbackInput input;
        input.stableCameraMode = stableCameraMode;
        input.playerInVehicle = playerInVehicle;
        input.chaseActive = mission.phase() == MissionPhase::ChaseActive;
        input.screenShake = feedback.screenShake();
        input.comedyZoom = feedback.comedyZoom();
        input.worldTension = feedback.worldTension();
        input.vehicleInstability = vehicle.instability();
        input.vehicleBoostActive = vehicle.boostActive();
        input.hornPulse = vehicle.hornPulse();
        return resolveCameraFeedback(input);
    }

    CameraRigMode cameraMode() const {
        GameplayCameraModeInput input;
        input.playerInVehicle = playerInVehicle;
        input.explicitInteractionCameraActive = explicitInteractionCameraActive();
        input.dialogueLineActive = dialogue.hasLine();
        input.playerSpeed = distanceXZ(player.velocity(), {});
        input.sprintSpeed = player.sprintSpeed();
        return resolveGameplayCameraMode(input);
    }

    CameraRigMode effectiveCameraMode() const {
        return resolveStableCameraMode(cameraMode(), stableCameraMode);
    }

    void updateCamera(float deltaSeconds = 0.0f) {
        if (isAssetPreviewIsolationMode(renderIsolationMode)) {
            camera.position = {2.15f, 1.35f, 2.35f};
            camera.target = {0.0f, 0.45f, 0.0f};
            camera.up = {0.0f, 1.0f, 0.0f};
            camera.fovy = 45.0f;
            return;
        }
        CameraRigState rigState = cameraRig.update(cameraTarget(), deltaSeconds, &collision);
        if (const WorldViewpoint* viewpoint = visualBaselineDebug.activeViewpoint(level.visualBaselines)) {
            camera.position = toRay(viewpoint->position);
            camera.target = toRay(viewpoint->target);
            camera.up = {0.0f, 1.0f, 0.0f};
            camera.fovy = 55.0f;
            return;
        }
        const CameraFeedbackOutput cameraFeedback = resolvedCameraFeedback();
        const float shake = cameraFeedback.screenShake * 0.18f;
        const Vec3 cameraRight = screenRightFromYaw(yawFromDirection(rigState.target - rigState.position));
        const Vec3 cameraUp{0.0f, 1.0f, 0.0f};
        const Vec3 shakeOffset = cameraRight * (std::sin(elapsedSeconds * 37.0f) * shake) +
                                 cameraUp * (std::sin(elapsedSeconds * 51.0f) * shake * 0.45f);

        camera.target = toRay(rigState.target);
        camera.position = toRay(rigState.position + shakeOffset);
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = std::max(42.0f, rigState.fovY - cameraFeedback.comedyZoom * 5.0f);
    }

    void draw(float alpha) {
        const RuntimeRenderSnapshot renderSnapshot =
            interpolateRenderSnapshot(previousRenderSnapshot, currentRenderSnapshot, alpha);
        const Camera3D renderCamera = renderSnapshot.camera;
        const WorldPresentationStyle& presentation = defaultWorldPresentationStyle();
        ClearBackground(toRayColor(presentation.skyColor));

        BeginMode3D(renderCamera);
        const Vec3 renderCameraPosition{renderCamera.position.x, renderCamera.position.y, renderCamera.position.z};
        const bool assetPreviewMode = isAssetPreviewIsolationMode(renderIsolationMode);
        const bool opaqueOnly = renderIsolationMode == WorldRenderIsolationMode::OpaqueOnly;
        const bool noGlass = renderIsolationMode == WorldRenderIsolationMode::NoGlass;
        const bool worldObjectsOnly = renderIsolationMode == WorldRenderIsolationMode::WorldObjectsOnly ||
                                      renderIsolationMode == WorldRenderIsolationMode::DebugShopShell ||
                                      renderIsolationMode == WorldRenderIsolationMode::DebugShopShellWireframe ||
                                      renderIsolationMode == WorldRenderIsolationMode::DebugShopShellNoCull ||
                                      renderIsolationMode == WorldRenderIsolationMode::DebugShopShellNormals ||
                                      renderIsolationMode == WorldRenderIsolationMode::DebugShopShellBounds ||
                                      renderIsolationMode == WorldRenderIsolationMode::DebugShopDoorWireframe ||
                                      renderIsolationMode == WorldRenderIsolationMode::DebugShopDoorCube ||
                                      assetPreviewMode;
        const WorldRenderList worldRenderList =
            renderCoordinator.buildWorldRenderList(renderSnapshot.worldObjects,
                                                   assetRegistry,
                                                   renderCameraPosition,
                                                   renderIsolationMode);
        if (!worldObjectsOnly) {
            WorldRenderer::drawWorldGround();
            WorldRenderer::drawWorldAtmosphere(renderCameraPosition);
        }
        if (assetPreviewMode) {
            WorldRenderer::drawWorldGround();
            if (const WorldAssetDefinition* definition = selectedAssetPreviewDefinition()) {
                WorldRenderer::drawAssetPreview(*definition, worldModels, renderIsolationMode);
            }
        } else {
            renderCoordinator.drawWorldOpaque(worldRenderList, assetRegistry, worldModels, renderIsolationMode);
        }
        if (!worldObjectsOnly) {
            WorldRenderer::drawVehicleOpaque(renderSnapshot.vehicle, renderColor(170, 55, 45));
            if (mission.phase() == MissionPhase::ChaseActive) {
                VehicleRuntimeState chaserState;
                chaserState.position = renderSnapshot.chaserPosition;
                chaserState.yawRadians = renderSnapshot.chaserYawRadians;
                WorldRenderer::drawVehicleOpaque(chaserState, renderColor(55, 74, 105));
            }
            drawParagonActors();
            if (playerInVehicle) {
                const Vec3 driverSeat =
                    vehicleDriverSeatPosition(renderSnapshot.vehicle.position, renderSnapshot.vehicle.yawRadians);
                WorldRenderer::drawPlayer(driverSeat,
                                          renderSnapshot.vehicle.yawRadians,
                                          playerPresentation.state(),
                                          elapsedSeconds,
                                          {true});
                if (debugEnabled) {
                    WorldRenderer::drawCharacterCapsuleDebug(driverSeat, renderColor(80, 190, 255));
                }
            } else {
                WorldRenderer::drawPlayer(renderSnapshot.playerPosition,
                                          renderSnapshot.playerYawRadians,
                                          playerPresentation.state(),
                                          elapsedSeconds);
                if (debugEnabled) {
                    WorldRenderer::drawCharacterCapsuleDebug(renderSnapshot.playerPosition, renderColor(80, 190, 255));
                }
            }
            if (characterPosePreview.active()) {
                const Vec3 previewRight = screenRightFromYaw(renderSnapshot.cameraYawRadians);
                const Vec3 previewForward = forwardFromYaw(renderSnapshot.cameraYawRadians);
                const Vec3 previewPosition =
                    renderSnapshot.playerPosition + previewRight * 1.55f + previewForward * 0.25f;
                WorldRenderer::drawPlayer(previewPosition,
                                          renderSnapshot.cameraYawRadians,
                                          characterPosePreview.activePreviewState(),
                                          elapsedSeconds);
                if (debugEnabled) {
                    WorldRenderer::drawCharacterCapsuleDebug(previewPosition, renderColor(246, 190, 92));
                }
            }
        }
        if (!opaqueOnly) {
            renderCoordinator.drawWorldTransparent(worldRenderList, assetRegistry, worldModels, renderIsolationMode);
        }
        if (!opaqueOnly && !noGlass && !worldObjectsOnly) {
            WorldRenderer::drawVehicleGlass(renderSnapshot.vehicle);
            WorldRenderer::drawVehicleSmoke(renderSnapshot.vehicle, elapsedSeconds);
            if (mission.phase() == MissionPhase::ChaseActive) {
                VehicleRuntimeState chaserState;
                chaserState.position = renderSnapshot.chaserPosition;
                chaserState.yawRadians = renderSnapshot.chaserYawRadians;
                WorldRenderer::drawVehicleGlass(chaserState);
            }
        }
        if (!opaqueOnly && !worldObjectsOnly) {
            IntroLevelPresentation::drawMarkers(objectiveContext(), feedback.hudPulse(), elapsedSeconds);
            drawParagonMarkers();
        }
        if (debugEnabled && !worldObjectsOnly) {
            const Vec3 pressureProbePosition = playerInVehicle ? vehicle.position() : player.position();
            const RewirPressureSnapshot debugRewirPressure = resolveRewirPressure(worldEvents, pressureProbePosition);
            IntroLevelPresentation::drawDistrictPlanDebug(level, elapsedSeconds);
            WorldRenderer::drawWorldCollisionDebug(level.objects);
            WorldRenderer::drawMemoryHotspotDebugMarkers(
                buildMemoryHotspotDebugMarkers(worldEvents.memoryHotspots()),
                elapsedSeconds);
            WorldRenderer::drawRewirPressureDebugMarkers(
                buildRewirPressureDebugMarkers(debugRewirPressure),
                elapsedSeconds);
        }
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
        if (!debugEnabled && runtimeEditorOpen && runtimeEditorUi.showCollision()) {
            WorldRenderer::drawWorldCollisionDebug(level.objects);
        }
#endif
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
        if (runtimeEditorOpen) {
            gizmo.processFrame(runtimeEditor,
                              {renderSnapshot.camera.position.x, renderSnapshot.camera.position.y,
                               renderSnapshot.camera.position.z},
                              {renderSnapshot.camera.target.x, renderSnapshot.camera.target.y,
                               renderSnapshot.camera.target.z},
                              renderSnapshot.camera.fovy, level.objects);
            if (runtimeEditor.selectedObject() != nullptr) {
                WorldRenderer::drawSelectionHighlight(*runtimeEditor.selectedObject());
                gizmo.drawSelectedGizmo(runtimeEditor.selectedObject()->position,
                                        {renderSnapshot.camera.position.x, renderSnapshot.camera.position.y,
                                         renderSnapshot.camera.position.z});
            }
        }
#endif
        DebugRenderer::drawCameraLines(buildCameraDebugLinesSnapshot());
        EndMode3D();

        drawHud(renderSnapshot);
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
        if (runtimeEditorOpen) {
            const Vec3 placementAnchor = playerInVehicle ? renderSnapshot.vehicle.position : renderSnapshot.playerPosition;
            const Vec3 placementPosition =
                runtimeEditorPlacementPosition(placementAnchor, renderSnapshot.cameraYawRadians);
            rlImGuiBegin();
            runtimeEditorUi.draw(runtimeEditor,
                                 assetRegistry,
                                 placementPosition,
                                 runtimeEditorOverlayPathForDataRoot(dataRoot));
            rlImGuiEnd();
        }
#endif
    }

    IntroObjectiveContext objectiveContext() const {
        return {&level, mission.phase(), vehicle.position(), vehicle.yawRadians(), &driveRoute};
    }

    bool paragonObjectiveActive() const {
        return paragonMission.phase() != ParagonMissionPhase::Locked &&
               paragonMission.phase() != ParagonMissionPhase::Completed &&
               paragonMission.phase() != ParagonMissionPhase::Failed;
    }

    ObjectiveMarker paragonObjectiveMarker() const {
        switch (paragonMission.phase()) {
        case ParagonMissionPhase::TalkToZenon:
        case ParagonMissionPhase::ReturnToZenon:
            return {"Zenon", level.zenonPosition, renderColor(75, 220, 105), true};
        case ParagonMissionPhase::FindReceiptHolder:
            return {"Lolek", level.lolekPosition, renderColor(238, 211, 79), true};
        case ParagonMissionPhase::ResolveReceipt:
            return {"Paragon", level.receiptHolderPosition, renderColor(230, 88, 70), true};
        case ParagonMissionPhase::Locked:
        case ParagonMissionPhase::Completed:
        case ParagonMissionPhase::Failed:
            return {};
        }
        return {};
    }

    std::string objectiveLineWithDistance() const {
        if (paragonObjectiveActive()) {
            const ObjectiveMarker marker = paragonObjectiveMarker();
            if (!marker.active) {
                return "Cel: " + paragonMission.objectiveText();
            }
            const int meters = static_cast<int>(std::round(distanceXZ(interactionActorPosition(), marker.position)));
            return "Cel: " + paragonMission.objectiveText() + " (" + std::to_string(meters) + "m)";
        }

        return IntroLevelPresentation::objectiveLineWithDistance(
            objectiveContext(),
            mission.objectiveText(),
            interactionActorPosition());
    }

    void drawSimpleNpc(Vec3 position, const std::string& profileId, float yawRadians = 0.0f) const {
        const CharacterVisualProfile& profile = characterVisualProfileOrDefault(profileId);
        CharacterRenderOptions options;
        options.palette = &profile.palette;
        PlayerPresentationState pose;
        WorldRenderer::drawPlayer(position, yawRadians, pose, elapsedSeconds, options);
    }

    void drawParagonActors() const {
        const bool paragonVisible = story.introCompleted ||
                                    paragonObjectiveActive() ||
                                    story.paragonCompleted;
        if (!paragonVisible) {
            return;
        }

        drawSimpleNpc(level.zenonPosition, "zenon", -0.35f);

        if (paragonMission.phase() == ParagonMissionPhase::FindReceiptHolder ||
            paragonMission.phase() == ParagonMissionPhase::ResolveReceipt ||
            paragonMission.phase() == ParagonMissionPhase::ReturnToZenon ||
            story.paragonCompleted) {
            drawSimpleNpc(level.lolekPosition, "lolek", 0.45f);
        }

        if (paragonMission.phase() == ParagonMissionPhase::ResolveReceipt) {
            drawSimpleNpc(level.receiptHolderPosition, "receipt_holder", 0.15f);
        }
    }

    void drawParagonMarkers() const {
        const float pulse = 1.0f + feedback.hudPulse() * 0.18f + std::sin(elapsedSeconds * 4.0f) * 0.04f;
        if (paragonMission.phase() == ParagonMissionPhase::TalkToZenon ||
            paragonMission.phase() == ParagonMissionPhase::ReturnToZenon) {
            WorldRenderer::drawMarker(level.zenonPosition, 1.05f * pulse, renderColor(75, 220, 105, 170));
        }
        if (paragonMission.phase() == ParagonMissionPhase::FindReceiptHolder) {
            WorldRenderer::drawMarker(level.lolekPosition, 0.92f * pulse, renderColor(238, 211, 79, 165));
        }
        if (paragonMission.phase() == ParagonMissionPhase::ResolveReceipt) {
            WorldRenderer::drawMarker(level.receiptHolderPosition, 0.98f * pulse, renderColor(230, 88, 70, 170));
        }
    }

    void drawHud(const RuntimeRenderSnapshot& renderSnapshot) {
        renderCoordinator.drawHud(buildHudSnapshot(renderSnapshot));
        if (debugEnabled) {
            DebugRenderer::drawPanel(buildDebugSnapshot());
        }
    }

    InteractionPromptSnapshot buildInteractionPromptSnapshot() const {
        const InteractionResolver resolver = buildInteractionResolver();
        const Vec3 actorPosition = interactionActorPosition();
        const InteractionResolution use = resolver.resolve(actorPosition, InteractionInput::Use);
        const InteractionResolution vehicleAction = resolver.resolve(actorPosition, InteractionInput::Vehicle);

        InteractionPromptSnapshot snapshot;
        std::vector<std::string> lines;
        if (use.available) {
            lines.push_back(use.prompt);
        }
        if (vehicleAction.available &&
            (!use.available || vehicleAction.id != use.id || vehicleAction.action != use.action)) {
            lines.push_back(vehicleAction.prompt);
        }
        if (use.available && vehicleAction.available && use.id == vehicleAction.id && use.action == vehicleAction.action) {
            const std::string::size_type colon = use.prompt.find(':');
            const std::string suffix = colon == std::string::npos ? use.prompt : use.prompt.substr(colon + 1);
            lines[0] = "E/F:" + suffix;
        }

        snapshot.lines = lines;
        return snapshot;
    }

    HudSnapshot buildHudSnapshot(const RuntimeRenderSnapshot& renderSnapshot) const {
        HudSnapshot snapshot;
        snapshot.screenWidth = GetScreenWidth();
        snapshot.screenHeight = GetScreenHeight();
        snapshot.playerInVehicle = playerInVehicle;
        snapshot.missionPhase = mission.phase();
        snapshot.phaseSeconds = phaseSeconds;
        snapshot.hudPulse = feedback.hudPulse();
        snapshot.flashAlpha = feedback.flashAlpha();
        snapshot.debugEnabled = debugEnabled;
        snapshot.renderModeLabel = worldRenderIsolationModeName(renderIsolationMode);
        snapshot.objectiveLine = objectiveLineWithDistance();
        snapshot.objectiveMarker = paragonObjectiveActive()
                                       ? paragonObjectiveMarker()
                                       : IntroLevelPresentation::currentObjectiveMarker(objectiveContext());
        snapshot.actorPosition = playerInVehicle ? renderSnapshot.vehicle.position : renderSnapshot.playerPosition;
        snapshot.camera = toRenderCamera(renderSnapshot.camera);
        snapshot.dialogue = &dialogue;
        snapshot.interaction = buildInteractionPromptSnapshot();
        snapshot.chase = {mission.phase() == MissionPhase::ChaseActive, chase.escapeProgress(), chase.caughtProgress()};
        snapshot.vehicle = {playerInVehicle,
                            mission.phase() == MissionPhase::ChaseActive,
                            vehicle.condition(),
                            vehicle.conditionBand(),
                            vehicle.speed(),
                            vehicle.boostActive(),
                            vehicle.hornCooldown(),
                            vehicle.runtimeState().driftActive,
                            vehicle.runtimeState().driftAmount,
                            vehicle.runtimeState().gear,
                            vehicle.runtimeState().rpmNormalized,
                            vehicle.runtimeState().shiftTimerSeconds};
        snapshot.przypal = {playerInVehicle,
                            mission.phase() == MissionPhase::ChaseActive,
                            przypal.normalizedIntensity(),
                            przypal.value(),
                            przypal.band(),
                            lastPrzypalReason,
                            reactionToastSeconds,
                            lastReaction.available,
                            lastReaction.speaker,
                            lastReaction.text,
                            feedback.hudPulse()};
        if (const WorldViewpoint* viewpoint = visualBaselineDebug.activeViewpoint(level.visualBaselines)) {
            snapshot.visualBaselineActive = true;
            snapshot.visualBaselineLabel = viewpoint->label;
            snapshot.visualBaselineIndex = visualBaselineDebug.activeIndex();
            snapshot.visualBaselineCount = static_cast<int>(level.visualBaselines.size());
        }
        if (characterPosePreview.active()) {
            snapshot.characterPosePreviewActive = true;
            snapshot.characterPosePreviewLabel = characterPosePreview.activeLabel();
            snapshot.characterPosePreviewIndex = characterPosePreview.activeIndex();
            snapshot.characterPosePreviewCount = characterPosePreview.count();
        }
        if (isAssetPreviewIsolationMode(renderIsolationMode)) {
            snapshot.assetPreviewActive = true;
            snapshot.assetPreviewIndex = assetPreviewIndex;
            snapshot.assetPreviewCount = assetPreviewCount();
            if (const WorldAssetDefinition* definition = selectedAssetPreviewDefinition()) {
                snapshot.assetPreviewLabel = definition->id;
                snapshot.assetPreviewMetadata = assetPreviewMetadata(*definition);
            }
        }
        return snapshot;
    }

    DebugSnapshot buildDebugSnapshot() const {
        const CameraRigState& rig = cameraRig.state();
        const float targetLag = distanceXZ(rig.desiredTarget, rig.target);
        const float positionLag = distanceXZ(rig.desiredPosition, rig.position);
        const float boomGap = std::max(0.0f, rig.fullBoomLength - rig.boomLength);
        const float visualYawError = std::atan2(std::sin(rig.visualYawRadians - rig.cameraYawRadians),
                                                std::cos(rig.visualYawRadians - rig.cameraYawRadians));
        const RoutePoint* routePoint = driveRoute.activePoint();
        const WorldEventLocationCounts memoryCounts = worldEvents.locationCounts();
        const std::vector<WorldEventHotspot> memoryHotspots = worldEvents.memoryHotspots();
        const RewirPressureSnapshot rewirPressure = resolveRewirPressure(worldEvents, player.position());
        const LocalRewirServiceDigest serviceDigest = buildLocalRewirServiceDigest(worldEvents);
        const LocalRewirFavorDigest favorDigest =
            buildLocalRewirFavorDigest(worldEvents, completedLocalRewirFavorIds);

        DebugSnapshot snapshot;
        snapshot.fps = platform.fps();
        snapshot.deltaMs = lastDeltaSeconds * 1000.0f;
        snapshot.renderModeLabel = worldRenderIsolationModeName(renderIsolationMode);
        populateAssetRuntimeStats(snapshot);
        snapshot.buildStamp = buildStampText() + " | " + devToolsStatusText();
        snapshot.dataRoot = resolvedPathText(dataRoot);
        snapshot.executablePath = resolvedPathText(executablePath);
        snapshot.phase = mission.phase();
        snapshot.playerPosition = player.position();
        snapshot.speedState = player.motorState().speedState;
        snapshot.playerXzSpeed = distanceXZ(player.velocity(), {});
        snapshot.playerVerticalSpeed = player.velocity().y;
        snapshot.carPosition = vehicle.position();
        snapshot.vehicleSpeed = vehicle.speed();
        snapshot.vehicleCondition = vehicle.condition();
        snapshot.vehicleConditionBand = vehicle.conditionBand();
        snapshot.vehicleSlip = vehicle.lateralSlip();
        snapshot.vehicleDriftActive = vehicle.runtimeState().driftActive;
        snapshot.vehicleDriftAmount = vehicle.runtimeState().driftAmount;
        snapshot.vehicleDriftAngleDegrees = radiansToDegrees(vehicle.runtimeState().driftAngleRadians);
        snapshot.vehicleGear = vehicle.runtimeState().gear;
        snapshot.vehicleRpmNormalized = vehicle.runtimeState().rpmNormalized;
        snapshot.vehicleShiftTimerSeconds = vehicle.runtimeState().shiftTimerSeconds;
        snapshot.vehicleHitZone = lastVehicleCollision.hitZone;
        snapshot.vehicleImpactSpeed = lastVehicleCollision.impactSpeed;
        snapshot.vehicleImpactNormal = lastVehicleCollision.impactNormal;
        snapshot.boostActive = vehicle.boostActive();
        snapshot.hornCooldown = vehicle.hornCooldown();
        snapshot.cameraMode = effectiveCameraMode();
        snapshot.stableCameraMode = stableCameraMode;
        snapshot.playerInVehicle = playerInVehicle;
        snapshot.mouseCaptured = mouseCaptured;
        snapshot.invertMouseX = invertMouseX;
        snapshot.cameraYawDegrees = radiansToDegrees(rig.cameraYawRadians);
        snapshot.cameraPitchDegrees = radiansToDegrees(rig.pitchRadians);
        snapshot.boomLength = rig.boomLength;
        snapshot.fullBoomLength = rig.fullBoomLength;
        snapshot.boomGap = boomGap;
        snapshot.grounded = player.isGrounded();
        snapshot.groundHeight = player.motorState().ground.height;
        snapshot.supportOwnerId = player.motorState().ground.ownerId;
        snapshot.supportPlatformVelocity = player.motorState().ground.platformVelocity;
        snapshot.cameraTargetLag = targetLag;
        snapshot.cameraPositionLag = positionLag;
        const CameraFeedbackOutput cameraFeedback = resolvedCameraFeedback();
        snapshot.screenShake = cameraFeedback.screenShake;
        snapshot.comedyZoom = cameraFeedback.comedyZoom;
        snapshot.worldTension = cameraFeedback.cameraTension;
        snapshot.przypalValue = przypal.value();
        snapshot.przypalLabel = PrzypalSystem::hudLabel(przypal.band());
        snapshot.przypalBand = przypal.band();
        snapshot.recentEventCount = static_cast<int>(worldEvents.recentEvents().size());
        snapshot.memoryBlockCount = memoryCounts.block;
        snapshot.memoryShopCount = memoryCounts.shop;
        snapshot.memoryParkingCount = memoryCounts.parking;
        snapshot.memoryGarageCount = memoryCounts.garage;
        snapshot.memoryTrashCount = memoryCounts.trash;
        snapshot.memoryRoadLoopCount = memoryCounts.roadLoop;
        snapshot.memoryHotspotCount = static_cast<int>(memoryHotspots.size());
        snapshot.rewirPressureActive = rewirPressure.active;
        snapshot.rewirPressurePatrolInterest = rewirPressure.patrolInterest;
        snapshot.rewirPressureScore = rewirPressure.score;
        snapshot.rewirPressureDistance = rewirPressure.distanceToPlayer;
        snapshot.rewirPressureLocation = rewirPressureLocationLabel(rewirPressure.location);
        snapshot.rewirPressureLevel = rewirPressureLevelLabel(rewirPressure.level);
        snapshot.rewirPressureSource = rewirPressure.source;
        snapshot.localServiceTotal = serviceDigest.total;
        snapshot.localServiceWary = serviceDigest.wary;
        if (!serviceDigest.waryInteractionPointIds.empty()) {
            snapshot.localServiceFirstWaryPoint = serviceDigest.waryInteractionPointIds.front();
        }
        snapshot.localFavorTotal = favorDigest.total;
        snapshot.localFavorActive = favorDigest.active;
        snapshot.localFavorCompleted = favorDigest.completed;
        snapshot.localFavorFirstActivePoint = favorDigest.firstActiveInteractionPointId;
        snapshot.lastPrzypalReason = lastPrzypalReason;
        snapshot.visitedShopOnFoot = story.visitedShopOnFoot;
        snapshot.gruzUnlocked = story.gruzUnlocked;
        snapshot.introCompleted = story.introCompleted;
        snapshot.rawW = lastRawInput.moveForwardDown;
        snapshot.rawA = lastRawInput.moveLeftDown;
        snapshot.rawS = lastRawInput.moveBackwardDown;
        snapshot.rawD = lastRawInput.moveRightDown;
        snapshot.mouseDeltaX = lastRawInput.mouseDeltaX;
        snapshot.mouseDeltaY = lastRawInput.mouseDeltaY;
        snapshot.accelerate = lastInput.accelerate;
        snapshot.brake = lastInput.brake;
        snapshot.steerLeft = lastInput.steerLeft;
        snapshot.steerRight = lastInput.steerRight;
        snapshot.steeringAuthority = vehicle.runtimeState().steeringAuthority;
        snapshot.vehicleYawDegrees = radiansToDegrees(vehicle.yawRadians());
        snapshot.visualYawDegrees = radiansToDegrees(rig.visualYawRadians);
        snapshot.visualYawErrorDegrees = radiansToDegrees(visualYawError);
        snapshot.routeIndex = driveRoute.activeIndex();
        snapshot.routeLabel = routePoint != nullptr ? routePoint->label : "-";
        snapshot.chaseDistance = distanceXZ(vehicle.position(), chaserPosition);
        snapshot.chaseAiMode = chaseAiModeName(lastChaseAiCommand.mode);
        snapshot.chaseRecoverSeconds = chaseAiState.contactRecoverSeconds;
        snapshot.chaseContactRecover = lastChaseAiCommand.contactRecoverActive;
        snapshot.reactionAvailable = lastReaction.available;
        snapshot.reactionSpeaker = lastReaction.speaker;
        if (const WorldViewpoint* viewpoint = visualBaselineDebug.activeViewpoint(level.visualBaselines)) {
            snapshot.visualBaselineActive = true;
            snapshot.visualBaselineLabel = viewpoint->label;
            snapshot.visualBaselineIndex = visualBaselineDebug.activeIndex();
            snapshot.visualBaselineCount = static_cast<int>(level.visualBaselines.size());
        }
        return snapshot;
    }

    CameraDebugLinesSnapshot buildCameraDebugLinesSnapshot() const {
        CameraDebugLinesSnapshot snapshot;
        snapshot.debugEnabled = debugEnabled;
        snapshot.playerInVehicle = playerInVehicle;
        snapshot.rig = cameraRig.state();
        snapshot.playerPosition = player.position();
        snapshot.playerVelocity = player.velocity();
        snapshot.vehiclePosition = vehicle.position();
        snapshot.playerCameraYawRadians = cameraRig.state().cameraYawRadians;
        snapshot.vehicleYawRadians = vehicle.yawRadians();

        VehicleCollisionConfig vehicleCollision;
        const VehicleCollisionResult probe = collision.resolveVehicleCapsule(
            vehicle.position(),
            vehicle.position(),
            vehicle.yawRadians(),
            vehicleCollision);
        snapshot.vehicleCollisionCircles[0] = probe.circles[0];
        snapshot.vehicleCollisionCircles[1] = probe.circles[1];
        snapshot.vehicleCollisionCircles[2] = probe.circles[2];
        return snapshot;
    }
};

} // namespace

void GameApp::run() {
    run({});
}

void GameApp::run(const GameRunOptions& options) {
    RaylibPlatform platform;
    RaylibInputReader inputReader;
    bool windowReady = false;
    bool runtimeReady = false;
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
    bool imguiReady = false;
#endif
    Runtime runtime(options, platform, inputReader);
    platform.configureWindowFlags();
    try {
        platform.openWindow({ScreenWidth, ScreenHeight, runtimeWindowTitle(options), 60, true, true});
        windowReady = true;
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
        platform.setupDevToolsUi();
        imguiReady = true;
#endif

        if (options.enableAudio) {
            runtime.audio.setup();
        }
        runtime.setup();
        runtimeReady = true;
        FixedStepClock simulationClock({1.0f / 60.0f, 0.10f, 5});
        int renderedFrames = 0;
        int shadowFramesBuilt = 0;
        int shadowFramesSubmitted = 0;
        bool sidecarCloseLogged = false;
        bool shadowFrameDumped = false;

        D3D11ShadowSidecar sidecar;
        if (options.d3d11ShadowWindow) {
            if (sidecar.initialize()) {
                TraceLog(LOG_INFO, "D3D11 shadow sidecar initialized");
            } else {
                TraceLog(LOG_WARNING, "D3D11 shadow sidecar init failed: %s", sidecar.lastError());
            }
        }

        MeshRegistry shadowMeshRegistry;
        MaterialRegistry shadowMaterialRegistry;
        bool shadowMeshRegistrySeeded = false;
        bool shadowSidecarTestMeshesUploaded = false;
        std::vector<MeshHandle> shadowSeededMeshHandles;

        while (!platform.shouldClose()) {
            RawInputState frameRawInput = runtime.readRawInput();
            runtime.handleFrameInput(frameRawInput);

            const FixedStepFrame frame = simulationClock.advance(platform.frameTimeSeconds());
            for (int step = 0; step < frame.steps; ++step) {
                RawInputState stepRawInput = frameRawInput;
                if (step > 0) {
                    clearRawInputFrameEdges(stepRawInput);
                }
                runtime.fixedUpdate(stepRawInput, frame.fixedDeltaSeconds);
            }

            platform.beginFrame();
            runtime.draw(frame.alpha);
            platform.endFrame();

            ++renderedFrames;

            if (options.renderFrameShadow && (renderedFrames == 1 || renderedFrames % options.renderFrameShadowInterval == 0)) {
                const RuntimeRenderSnapshot& renderSnapshot = runtime.currentRenderSnapshot;
                const Vec3 cameraPos{renderSnapshot.camera.position.x,
                                     renderSnapshot.camera.position.y,
                                     renderSnapshot.camera.position.z};

                WorldRenderList renderList = runtime.renderCoordinator.buildWorldRenderList(
                    renderSnapshot.worldObjects,
                    runtime.assetRegistry,
                    cameraPos,
                    WorldRenderIsolationMode::Full);

                RenderFrameBuilder builder;
                builder.setCamera(toRenderCamera(renderSnapshot.camera));

                const auto& assetDefs = runtime.assetRegistry.definitions();

                WorldRenderExtractionStats extractionStats;
                if (options.renderFrameShadowMeshes) {
                    if (!shadowMeshRegistrySeeded) {
                        shadowMeshRegistrySeeded = true;
                        constexpr int ShadowMeshSeedLimit = 16;
                        const auto seedIds = selectShadowMeshSeedAssetIdsFromDefinitions(renderList, assetDefs, ShadowMeshSeedLimit);
                        for (const auto& assetId : seedIds) {
                            const auto handle = shadowMeshRegistry.allocate(assetId);
                            shadowSeededMeshHandles.push_back(handle);
                        }
                        if (options.d3d11ShadowDiagnostics) {
                            std::string seedList;
                            for (const auto& id : seedIds) {
                                if (!seedList.empty()) seedList += ", ";
                                seedList += id;
                            }
                            TraceLog(LOG_INFO, "RenderFrame shadow diag: seeded %d mesh assetIds: %s",
                                     static_cast<int>(seedIds.size()), seedList.c_str());
                        }
                    }

                    if (!shadowSidecarTestMeshesUploaded && sidecar.isInitialized()) {
                        shadowSidecarTestMeshesUploaded = true;
                        int loadedMeshFiles = 0;
                        int proceduralFallbackUploads = 0;
                        int meshLoadFailures = 0;

                        const std::filesystem::path assetsRoot =
                            std::filesystem::path(options.dataRoot) / "assets";

                        for (const auto& handle : shadowSeededMeshHandles) {
                            const std::string* assetId = shadowMeshRegistry.assetId(handle);
                            const WorldAssetDefinition* def =
                                assetId != nullptr ? runtime.assetRegistry.find(*assetId) : nullptr;

                            CpuMeshData meshData;
                            bool useLoadedMesh = false;

                            if (def != nullptr && !def->modelPath.empty()) {
                                const std::filesystem::path resolvedPath = assetsRoot / def->modelPath;
                                const auto loadResult = loadCpuMeshFromObjFile(resolvedPath.string());
                                if (loadResult.ok) {
                                    meshData = loadResult.mesh;
                                    useLoadedMesh = true;
                                    ++loadedMeshFiles;
                                } else {
                                    ++meshLoadFailures;
                                    TraceLog(LOG_WARNING,
                                             "D3D11 shadow sidecar: load failed for %s (%s): %s",
                                             def->id.c_str(), resolvedPath.string().c_str(), loadResult.error.c_str());
                                }
                            }

                            if (!useLoadedMesh) {
                                meshData = makeCpuMeshUnitCube("shadow_" + std::to_string(handle.id));
                            }

                            std::string uploadError;
                            if (sidecar.uploadTestMesh(handle, meshData, &uploadError)) {
                                if (!useLoadedMesh) {
                                    ++proceduralFallbackUploads;
                                }
                            } else {
                                TraceLog(LOG_WARNING, "D3D11 shadow sidecar: mesh upload failed for id=%u: %s",
                                         handle.id, uploadError.c_str());
                            }
                        }

                        const int totalUploaded = loadedMeshFiles + proceduralFallbackUploads;
                        if (totalUploaded > 0) {
                            TraceLog(LOG_INFO, "D3D11 shadow sidecar: seedCount=%d uploaded %d mesh handles"
                                     " (loadedMeshFiles=%d proceduralFallback=%d loadFailures=%d)",
                                     static_cast<int>(shadowSeededMeshHandles.size()),
                                     totalUploaded, loadedMeshFiles, proceduralFallbackUploads, meshLoadFailures);
                        }
                    }

                    extractionStats = addWorldRenderListMeshCommands(
                        builder, renderList, assetDefs,
                        shadowMeshRegistry, shadowMaterialRegistry);
                } else {
                    extractionStats = addWorldRenderListFallbackBoxes(builder, renderList, assetDefs);
                }

                builder.addDebugLine({0.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f}, renderColor(255, 0, 0));
                builder.addDebugLine({0.0f, 0.0f, 0.0f}, {0.0f, 3.0f, 0.0f}, renderColor(0, 255, 0));
                builder.addDebugLine({cameraPos.x, cameraPos.y, cameraPos.z},
                                     {renderSnapshot.camera.target.x,
                                      renderSnapshot.camera.target.y,
                                      renderSnapshot.camera.target.z},
                                     renderColor(128, 128, 255));

                RenderFrame shadowFrame = builder.build();

                if (!shadowFrameDumped && !options.renderFrameShadowDumpPath.empty()) {
                    shadowFrameDumped = true;
                    RenderFrameDumpWriteOptions dumpOpts;
                    dumpOpts.version = options.renderFrameShadowDumpVersion == 2
                                           ? RenderFrameDumpVersion::V2
                                           : RenderFrameDumpVersion::V1;
                    std::string dumpError;
                    if (writeRenderFrameDump(shadowFrame, options.renderFrameShadowDumpPath, dumpOpts, &dumpError)) {
                        TraceLog(LOG_INFO, "RenderFrame shadow: dumped to %s version=%s",
                                 options.renderFrameShadowDumpPath.c_str(),
                                 dumpOpts.version == RenderFrameDumpVersion::V2 ? "v2" : "v1");
                    } else {
                        TraceLog(LOG_WARNING, "RenderFrame shadow dump failed: %s", dumpError.c_str());
                    }
                }

                RenderFrameValidationResult validation = validateRenderFrame(shadowFrame);
                RenderFrameStats stats = summarizeRenderFrame(shadowFrame);

                NullRenderer nullRenderer;
                nullRenderer.renderFrame(shadowFrame);

                ++shadowFramesBuilt;

                TraceLog(LOG_INFO,
                         "RenderFrame shadow: primitives=%d debugLines=%d valid=%s nullCalls=%d"
                         " emittedMeshes=%d meshFallbacks=%d missingDefs=%d",
                         stats.totalPrimitives,
                         stats.debugLines,
                         validation.valid ? "true" : "false",
                         nullRenderer.renderCalls(),
                         extractionStats.emittedMeshes,
                         extractionStats.meshFallbacks,
                         extractionStats.missingDefinitions);

                if (sidecar.isInitialized()) {
                    sidecar.submit(shadowFrame);
                    ++shadowFramesSubmitted;
                }

                if (options.d3d11ShadowDiagnostics) {
                    const auto& d3d11Stats = sidecar.lastD3D11Stats();
                    int supportedBoxes = 0;
                    int supportedMeshes = 0;
                    for (const auto& prim : shadowFrame.primitives) {
                        const bool supportedBucket =
                            prim.bucket == RenderBucket::Opaque ||
                            prim.bucket == RenderBucket::Vehicle ||
                            prim.bucket == RenderBucket::Decal ||
                            prim.bucket == RenderBucket::Glass ||
                            prim.bucket == RenderBucket::Translucent ||
                            prim.bucket == RenderBucket::Debug;
                        if (!supportedBucket) {
                            continue;
                        }
                        if (prim.kind == RenderPrimitiveKind::Box) {
                            ++supportedBoxes;
                        } else if (prim.kind == RenderPrimitiveKind::Mesh) {
                            ++supportedMeshes;
                        }
                    }
                    const int supportedPrimitives = supportedBoxes + supportedMeshes;
                    const int boxCoveragePct = supportedBoxes > 0
                        ? (d3d11Stats.drawnBoxes * 100) / supportedBoxes
                        : 100;
                    const int meshCoveragePct = supportedMeshes > 0
                        ? (d3d11Stats.drawnMeshes * 100) / supportedMeshes
                        : 100;
                    const int primitiveCoveragePct = supportedPrimitives > 0
                        ? ((d3d11Stats.drawnBoxes + d3d11Stats.drawnMeshes) * 100) / supportedPrimitives
                        : 100;
                    const int lineCoveragePct = stats.debugLines > 0
                        ? (d3d11Stats.drawnDebugLines * 100) / stats.debugLines
                        : 100;
                    TraceLog(LOG_INFO,
                             "RenderFrame shadow diag: built=%d submitted=%d prims=%d lines=%d valid=%d "
                             "opaque=%d vehicle=%d decal=%d glass=%d transl=%d debug=%d "
                             "sidecarInit=%d sidecarCalls=%d sidecarValid=%d "
                              "drawnBoxes=%d drawnMeshes=%d skipMissMesh=%d skipKinds=%d skipBuckets=%d skipPrims=%d "
                              "drawnLines=%d skipLines=%d "
                              "supportedBoxes=%d supportedMeshes=%d supportedPrims=%d "
                              "boxCoveragePct=%d meshCoveragePct=%d primitiveCoveragePct=%d lineCoveragePct=%d "
                              "meshEmitted=%d meshFallbacks=%d meshMissingDefs=%d",
                              shadowFramesBuilt,
                              shadowFramesSubmitted,
                              stats.totalPrimitives,
                              stats.debugLines,
                              validation.valid ? 1 : 0,
                              stats.opaque,
                              stats.vehicle,
                              stats.decal,
                              stats.glass,
                              stats.translucent,
                              stats.debug,
                              sidecar.isInitialized() ? 1 : 0,
                              sidecar.isInitialized() ? sidecar.renderCalls() : 0,
                              sidecar.isInitialized() ? (sidecar.lastFrameValid() ? 1 : 0) : 1,
                              d3d11Stats.drawnBoxes,
                              d3d11Stats.drawnMeshes,
                              d3d11Stats.skippedMissingMeshes,
                              d3d11Stats.skippedUnsupportedKinds,
                              d3d11Stats.skippedUnsupportedBuckets,
                              d3d11Stats.skippedPrimitives,
                              d3d11Stats.drawnDebugLines,
                              d3d11Stats.skippedDebugLines,
                              supportedBoxes,
                              supportedMeshes,
                              supportedPrimitives,
                              boxCoveragePct,
                              meshCoveragePct,
                              primitiveCoveragePct,
                              lineCoveragePct,
                              extractionStats.emittedMeshes,
                              extractionStats.meshFallbacks,
                              extractionStats.missingDefinitions);
                    if (!sidecar.isInitialized() && options.d3d11ShadowWindow) {
                        TraceLog(LOG_WARNING, "RenderFrame shadow diag: sidecar error: %s", sidecar.lastError());
                    }
                }
            }

            if (sidecar.isInitialized()) {
                sidecar.pumpMessages();
            }

            if (sidecar.isCloseRequested() && !sidecarCloseLogged) {
                sidecarCloseLogged = true;
                sidecar.shutdown();
                TraceLog(LOG_INFO, "D3D11 shadow sidecar closed; continuing raylib runtime");
            }

            if (options.smokeFrames > 0 && renderedFrames >= options.smokeFrames) {
                break;
            }
        }

        sidecar.shutdown();
        runtime.shutdown();
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
        if (imguiReady) {
            platform.shutdownDevToolsUi();
            imguiReady = false;
        }
#endif
        if (windowReady) {
            platform.closeWindow();
        }
    } catch (const std::exception& ex) {
        TraceLog(LOG_ERROR, "GameApp boot/runtime failure: %s", ex.what());
        if (runtimeReady) {
            runtime.shutdown();
        } else {
            runtime.shutdown();
        }
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
        if (imguiReady) {
            platform.shutdownDevToolsUi();
            imguiReady = false;
        }
#endif
        if (windowReady) {
            platform.closeWindow();
        }
        throw;
    }
}

} // namespace bs3d
