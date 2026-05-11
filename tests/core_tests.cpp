#include "bs3d/core/ChaseSystem.h"
#include "bs3d/core/CameraRig.h"
#include "bs3d/core/CharacterAction.h"
#include "bs3d/core/CharacterPhysicalReaction.h"
#include "bs3d/core/CharacterStatusPolicy.h"
#include "bs3d/core/CharacterVaultPolicy.h"
#include "bs3d/core/CharacterWorldHitPolicy.h"
#include "bs3d/core/ControlContext.h"
#include "bs3d/core/DialogueSystem.h"
#include "bs3d/core/DriveRouteGuide.h"
#include "bs3d/core/GameFeedback.h"
#include "bs3d/core/InteractionResolver.h"
#include "bs3d/core/MissionController.h"
#include "bs3d/core/MissionTriggerSystem.h"
#include "bs3d/core/NpcReactionSystem.h"
#include "bs3d/core/ParagonMission.h"
#include "bs3d/core/ParagonMissionSpec.h"
#include "bs3d/core/PlayerMotor.h"
#include "bs3d/core/PlayerPresentation.h"
#include "bs3d/core/PlayerController.h"
#include "bs3d/core/PrzypalSystem.h"
#include "bs3d/core/RenderInterpolation.h"
#include "bs3d/core/Scene.h"
#include "bs3d/core/FixedTimestep.h"
#include "bs3d/core/SaveGame.h"
#include "bs3d/core/StoryState.h"
#include "bs3d/core/VehicleController.h"
#include "bs3d/core/WorldEventLedger.h"
#include "bs3d/core/WorldInteraction.h"
#include "bs3d/core/WorldRewirPressure.h"
#include "bs3d/core/WorldServiceState.h"
#include "bs3d/core/WorldCollision.h"

#include <cmath>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void expectNear(float actual, float expected, float epsilon, const std::string& message) {
    if (std::fabs(actual - expected) > epsilon) {
        throw std::runtime_error(message + " expected " + std::to_string(expected) +
                                 " got " + std::to_string(actual));
    }
}

void missionAdvancesThroughDrivingErrand() {
    bs3d::DialogueSystem dialogue;
    bs3d::MissionController mission(dialogue);

    expect(mission.phase() == bs3d::MissionPhase::WaitingForStart, "mission starts idle");

    mission.start();
    mission.start();
    expect(mission.phase() == bs3d::MissionPhase::WalkToShop, "intro starts by sending player to Zenon");
    expect(dialogue.currentLine().speaker == "Boguś", "canon character line owns the mission subtitle first");
    expect(mission.objectiveText().find("Zenona") != std::string::npos, "first objective names Zenon shop visit");

    mission.onShopVisitedOnFoot();
    expect(mission.phase() == bs3d::MissionPhase::ReturnToBench, "on-foot shop visit sends player back to Bogus");
    expect(dialogue.queuedLineCount() > 0, "shop visit queues canon dialogue");

    mission.onReturnedToBogus();
    expect(mission.phase() == bs3d::MissionPhase::ReachVehicle, "Bogus return unlocks gruz objective");

    mission.onPlayerEnteredVehicle();
    expect(mission.phase() == bs3d::MissionPhase::DriveToShop, "entering car advances mission");

    mission.onShopReachedByVehicle();
    expect(mission.phase() == bs3d::MissionPhase::ChaseActive, "shop starts chase");
    expect(mission.chaseWanted(), "mission asks chase system to start");

    mission.onChaseEscaped();
    expect(mission.phase() == bs3d::MissionPhase::ReturnToLot, "escaping chase unlocks return");

    mission.onDropoffReached();
    expect(mission.phase() == bs3d::MissionPhase::Completed, "dropoff completes mission");

    mission.fail("Rozwaliles fure.");
    expect(mission.phase() == bs3d::MissionPhase::Completed, "completed mission cannot be failed");
}

void missionFailureCanRetryFromVehicleObjective() {
    bs3d::DialogueSystem dialogue;
    bs3d::MissionController mission(dialogue);

    mission.start();
    mission.onShopVisitedOnFoot();
    mission.onReturnedToBogus();
    mission.onPlayerEnteredVehicle();
    mission.onShopReachedByVehicle();
    expect(mission.phase() == bs3d::MissionPhase::ChaseActive, "test setup reaches chase phase");

    mission.fail("Zgubiono auto.");
    expect(mission.phase() == bs3d::MissionPhase::Failed, "mission can fail before completion");
    expect(mission.failureReason() == "Zgubiono auto.", "failure reason is retained");

    mission.retry();
    expect(mission.phase() == bs3d::MissionPhase::ReachVehicle, "retry returns to vehicle objective");
    expect(mission.failureReason().empty(), "retry clears failure reason");
    expect(!mission.chaseWanted(), "retry clears pending chase request");
    expect(mission.objectiveText().find("gruza") != std::string::npos, "retry objective returns to gruz stage");
}

void missionFailureCanRetryToSerializedCheckpointPhase() {
    bs3d::DialogueSystem dialogue;
    bs3d::MissionController mission(dialogue);

    mission.start();
    mission.onShopVisitedOnFoot();
    mission.onReturnedToBogus();
    mission.onPlayerEnteredVehicle();
    mission.onShopReachedByVehicle();
    mission.fail("checkpoint test");

    mission.retryToCheckpoint(bs3d::MissionPhase::ChaseActive);
    expect(mission.phase() == bs3d::MissionPhase::ChaseActive,
           "retry can restore mission phase from checkpoint snapshot");
    expect(mission.chaseWanted(), "chase checkpoint re-arms chase start request");
    expect(mission.failureReason().empty(), "checkpoint retry clears failure reason");
    expect(mission.objectiveText().find("Zgub") != std::string::npos,
           "checkpoint retry restores phase objective");
}

void missionTriggerSystemFiresDataDrivenPhaseZonesOnce() {
    bs3d::MissionTriggerSystem triggers;
    triggers.setTriggers({
        {"shop_on_foot",
         bs3d::MissionPhase::WalkToShop,
         bs3d::MissionTriggerActor::Player,
         {10.0f, 0.0f, 0.0f},
         3.0f,
         bs3d::MissionTriggerAction::ShopVisitedOnFoot},
        {"shop_by_vehicle",
         bs3d::MissionPhase::DriveToShop,
         bs3d::MissionTriggerActor::Vehicle,
         {20.0f, 0.0f, 0.0f},
         4.0f,
         bs3d::MissionTriggerAction::ShopReachedByVehicle},
        {"dropoff",
         bs3d::MissionPhase::ReturnToLot,
         bs3d::MissionTriggerActor::Vehicle,
         {-5.0f, 0.0f, 0.0f},
         2.0f,
         bs3d::MissionTriggerAction::DropoffReached}
    });

    bs3d::MissionTriggerResult none =
        triggers.update(bs3d::MissionPhase::WalkToShop, {0.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f}, false);
    expect(!none.triggered, "mission trigger ignores actors outside trigger radius");

    bs3d::MissionTriggerResult foot =
        triggers.update(bs3d::MissionPhase::WalkToShop, {11.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f}, false);
    expect(foot.triggered, "mission trigger fires for matching player phase zone");
    expect(foot.action == bs3d::MissionTriggerAction::ShopVisitedOnFoot, "mission trigger exposes action enum");
    expect(foot.id == "shop_on_foot", "mission trigger exposes authored id");

    bs3d::MissionTriggerResult duplicate =
        triggers.update(bs3d::MissionPhase::WalkToShop, {11.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f}, false);
    expect(!duplicate.triggered, "mission trigger does not refire while already consumed");

    bs3d::MissionTriggerResult wrongActor =
        triggers.update(bs3d::MissionPhase::DriveToShop, {20.0f, 0.0f, 0.0f}, {40.0f, 0.0f, 0.0f}, false);
    expect(!wrongActor.triggered, "vehicle trigger does not fire while player is on foot");

    bs3d::MissionTriggerResult vehicle =
        triggers.update(bs3d::MissionPhase::DriveToShop, {0.0f, 0.0f, 0.0f}, {21.0f, 0.0f, 0.0f}, true);
    expect(vehicle.triggered, "mission trigger fires for matching vehicle phase zone");
    expect(vehicle.action == bs3d::MissionTriggerAction::ShopReachedByVehicle, "vehicle trigger exposes shop action");

    triggers.resetConsumed();
    bs3d::MissionTriggerResult afterReset =
        triggers.update(bs3d::MissionPhase::WalkToShop, {11.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f}, false);
    expect(afterReset.triggered, "resetConsumed allows retry/restart to use trigger again");
}

bool containsAnyOldPlaceholder(const std::string& text) {
    return text.find("Spejson") != std::string::npos ||
           text.find("Wojtas") != std::string::npos ||
           text.find("Mietek") != std::string::npos ||
           text.find("Blok Ekipa World") != std::string::npos;
}

void missionObjectiveTextIsShortCanonAndPlaceholderFree() {
    bs3d::DialogueSystem dialogue;
    bs3d::MissionController mission(dialogue);

    auto expectObjective = [](const bs3d::MissionController& mission, const std::string& message) {
        const std::string objective = mission.objectiveText();
        expect(!objective.empty(), message + " objective is present");
        expect(objective.size() <= 44, message + " objective is short enough for HUD");
        expect(!containsAnyOldPlaceholder(objective), message + " objective has no old placeholder text");
    };

    expectObjective(mission, "waiting");
    mission.start();
    expectObjective(mission, "walk to shop");
    mission.onShopVisitedOnFoot();
    expectObjective(mission, "return to Bogus");
    mission.onReturnedToBogus();
    expectObjective(mission, "reach vehicle");
    mission.onPlayerEnteredVehicle();
    expectObjective(mission, "drive to shop");
    mission.onShopReachedByVehicle();
    expectObjective(mission, "chase");
    mission.onChaseEscaped();
    expectObjective(mission, "return to lot");
    mission.onDropoffReached();
    expectObjective(mission, "completed");
}

void missionCanonDialogueUsesReadableDurationsAndNoOldSpeakers() {
    bs3d::DialogueSystem dialogue;
    bs3d::MissionController mission(dialogue);

    auto expectReadableLine = [](const bs3d::DialogueSystem& dialogue, const std::string& expectedSpeaker) {
        expect(dialogue.hasLine(), "canon dialogue line exists");
        const bs3d::DialogueLine& line = dialogue.currentLine();
        expect(line.speaker == expectedSpeaker, "canon speaker matches expected beat");
        expect(!containsAnyOldPlaceholder(line.speaker), "speaker has no old placeholder text");
        expect(!containsAnyOldPlaceholder(line.text), "dialogue has no old placeholder text");
        expect(line.channel == bs3d::DialogueChannel::MissionCritical,
               "canon mission dialogue uses mission critical channel");
        expect(line.durationSeconds >= 4.0f, "canon dialogue stays onscreen long enough to read");
        expect(line.text.size() <= 86, "canon dialogue line is short enough for subtitle box");
    };

    mission.start();
    dialogue.update(2.5f);
    expectReadableLine(dialogue, "Boguś");

    dialogue.clear();
    mission.onShopVisitedOnFoot();
    expectReadableLine(dialogue, "Zenon");

    dialogue.clear();
    mission.onReturnedToBogus();
    expectReadableLine(dialogue, "Boguś");

    dialogue.clear();
    mission.onPlayerEnteredVehicle();
    expectReadableLine(dialogue, "Rysiek");

    dialogue.clear();
    mission.onShopReachedByVehicle();
    expectReadableLine(dialogue, "Zenon");
    dialogue.update(dialogue.currentLine().durationSeconds + 0.1f);
    expectReadableLine(dialogue, "Halina");

    dialogue.clear();
    mission.onChaseEscaped();
    expectReadableLine(dialogue, "Rysiek");
}

void missionControllerUsesChannelsForHintsCriticalAndFail() {
    bs3d::DialogueSystem dialogue;
    bs3d::MissionController mission(dialogue);

    mission.start();
    expect(dialogue.hasLine(), "mission start queues dialogue");
    expect(dialogue.currentLine().speaker == "Boguś", "character line takes priority over objective hint");
    expect(dialogue.currentLine().channel == bs3d::DialogueChannel::MissionCritical,
           "canon character line uses mission critical channel");

    dialogue.update(dialogue.currentLine().durationSeconds + 0.1f);
    expect(dialogue.currentLine().speaker == "Misja", "objective hint follows after character line");
    expect(dialogue.currentLine().channel == bs3d::DialogueChannel::SystemHint,
           "objective text uses system hint channel instead of blocking reactions as story dialogue");

    mission.fail("test fail");
    expect(dialogue.currentLine().channel == bs3d::DialogueChannel::FailChase,
           "mission fail uses fail/chase channel");
    expect(dialogue.hasBlockingLine(), "mission fail blocks reaction subtitles");
}

void storyStateFlagsAdvanceOnlyFromMissionEvents() {
    bs3d::StoryState story;
    expect(!story.visitedShopOnFoot && !story.gruzUnlocked && !story.introCompleted && !story.shopTroubleSeen,
           "story state starts with intro flags clear");

    bs3d::WorldEventAddResult ignored;
    ignored.addedToLedger = true;
    ignored.heatPulseAccepted = true;
    ignored.accepted = true;
    ignored.type = bs3d::WorldEventType::ShopTrouble;
    ignored.location = bs3d::WorldLocationTag::Shop;
    ignored.intensity = 1.0f;
    ignored.heatMultiplier = 1.0f;
    story.onWorldEvent(ignored);
    expect(!story.shopTroubleSeen, "raw world events do not mutate story flags directly");

    story.onShopVisitedOnFoot();
    expect(story.visitedShopOnFoot, "on-foot shop visit marks story flag");
    expect(story.shopTroubleSeen, "on-foot shop visit records first shop trouble story beat");
    expect(!story.gruzUnlocked, "shop visit alone does not unlock gruz");

    story.onReturnedToBogus();
    expect(story.gruzUnlocked, "returning to Bogus unlocks gruz story flag");

    story.onIntroCompleted();
    expect(story.introCompleted, "dropoff completion marks intro complete");
}

void paragonMissionBranchesIntoPeacefulTrickAndChaosOutcomes() {
    bs3d::DialogueSystem dialogue;
    bs3d::ParagonMission mission(dialogue);

    expect(mission.phase() == bs3d::ParagonMissionPhase::Locked, "paragon starts locked before intro completion");
    expect(!mission.start(false), "paragon cannot start before intro is completed");
    expect(mission.start(true), "paragon can start after intro completion");
    expect(mission.phase() == bs3d::ParagonMissionPhase::TalkToZenon,
           "paragon starts at Zenon accusation beat");
    expect(mission.objectiveText().find("Zenona") != std::string::npos,
           "first paragon objective points to Zenon");

    const bs3d::ParagonMissionPulse accusation = mission.onZenonAccusesBogus();
    expect(accusation.available, "Zenon accusation emits first shop trouble pulse");
    expect(accusation.type == bs3d::WorldEventType::ShopTrouble, "accusation is shop trouble");
    expect(accusation.intensity > 0.15f && accusation.intensity < 0.50f,
           "accusation uses readable but not punishing heat");
    expect(mission.phase() == bs3d::ParagonMissionPhase::FindReceiptHolder,
           "after accusation player looks for receipt holder");

    mission.onReceiptHolderFound();
    expect(mission.phase() == bs3d::ParagonMissionPhase::ResolveReceipt,
           "finding receipt holder unlocks solution choice");

    const bs3d::ParagonMissionPulse peaceful = mission.resolve(bs3d::ParagonSolution::Peaceful);
    expect(peaceful.available, "peaceful solution still emits a small world memory pulse");
    expect(!mission.shopBanActive(), "peaceful paragon solution does not ban the player from shop");
    expect(mission.solution() == bs3d::ParagonSolution::Peaceful, "peaceful solution is recorded");

    mission.onReturnedToZenon();
    expect(mission.phase() == bs3d::ParagonMissionPhase::Completed, "return to Zenon completes peaceful path");

    bs3d::DialogueSystem chaosDialogue;
    bs3d::ParagonMission chaos(chaosDialogue);
    chaos.start(true);
    chaos.onZenonAccusesBogus();
    chaos.onReceiptHolderFound();
    const bs3d::ParagonMissionPulse chaosPulse = chaos.resolve(bs3d::ParagonSolution::Chaos);
    expect(chaosPulse.available, "chaos solution emits a world event pulse");
    expect(chaosPulse.type == bs3d::WorldEventType::ShopTrouble, "chaos solution is shop trouble");
    expect(chaosPulse.intensity >= 0.80f, "chaos solution is clearly hotter than peaceful path");
    expect(chaos.shopBanActive(), "chaos solution applies runtime shop ban");
    expect(chaos.solution() == bs3d::ParagonSolution::Chaos, "chaos solution is recorded");
}

void storyStateTracksParagonRuntimeConsequencesWithoutSaveGame() {
    bs3d::StoryState story;
    expect(!story.paragonCompleted && !story.shopBanActive,
           "paragon story flags start clean");

    story.onParagonResolved(bs3d::ParagonSolution::Trick);
    expect(story.paragonCompleted, "paragon completion is tracked in runtime story");
    expect(story.paragonSolution == bs3d::ParagonSolution::Trick,
           "runtime story records chosen paragon solution");
    expect(!story.shopBanActive, "trick solution avoids shop ban");

    story.onParagonResolved(bs3d::ParagonSolution::Chaos);
    expect(story.shopBanActive, "chaos solution activates runtime shop ban");
}

void saveGameRoundTripsStoryProgressCheckpointAndRuntimeState() {
    bs3d::SaveGame save;
    save.story.visitedShopOnFoot = true;
    save.story.gruzUnlocked = true;
    save.story.paragonCompleted = true;
    save.story.paragonSolution = bs3d::ParagonSolution::Trick;
    save.playerPosition = {1.0f, 2.0f, 3.0f};
    save.playerYawRadians = 0.75f;
    save.playerInVehicle = true;
    save.vehiclePosition = {-4.0f, 0.0f, 8.0f};
    save.vehicleYawRadians = -0.25f;
    save.vehicleCondition = 72.5f;
    save.przypalValue = 44.0f;
    save.mission.phase = bs3d::MissionPhase::ChaseActive;
    save.mission.phaseSeconds = 6.5f;
    save.mission.checkpointId = "intro_chase_started";
    save.paragonPhase = bs3d::ParagonMissionPhase::ResolveReceipt;

    const std::string encoded = bs3d::serializeSaveGame(save);
    expect(encoded.find("version=1") != std::string::npos, "save game is versioned");
    expect(encoded.find("checkpoint=intro_chase_started") != std::string::npos,
           "save game persists checkpoint id");

    const bs3d::SaveGame loaded = bs3d::deserializeSaveGame(encoded);
    expect(loaded.version == save.version, "save version round-trips");
    expect(loaded.story.visitedShopOnFoot && loaded.story.gruzUnlocked,
           "story progression flags round-trip");
    expect(loaded.story.paragonCompleted &&
               loaded.story.paragonSolution == bs3d::ParagonSolution::Trick,
           "paragon consequences round-trip");
    expect(loaded.playerInVehicle, "vehicle occupancy round-trips");
    expectNear(loaded.playerPosition.z, save.playerPosition.z, 0.001f, "player position round-trips");
    expectNear(loaded.vehicleCondition, save.vehicleCondition, 0.001f, "vehicle condition round-trips");
    expect(loaded.mission.phase == bs3d::MissionPhase::ChaseActive, "mission phase round-trips");
    expect(loaded.mission.checkpointId == "intro_chase_started", "mission checkpoint round-trips");
    expect(loaded.paragonPhase == bs3d::ParagonMissionPhase::ResolveReceipt,
           "paragon phase round-trips");
}

void saveGameRoundTripsActiveWorldMemoryEvents() {
    bs3d::SaveGame save;

    bs3d::WorldEvent garage;
    garage.id = 42;
    garage.type = bs3d::WorldEventType::PropertyDamage;
    garage.location = bs3d::WorldLocationTag::Garage;
    garage.position = {-18.0f, 0.0f, 23.0f};
    garage.intensity = 1.4f;
    garage.remainingSeconds = 12.5f;
    garage.source = "garage_pressure_memory";
    garage.stackCount = 2;
    garage.ageSeconds = 5.5f;
    save.worldEvents.push_back(garage);

    const std::string encoded = bs3d::serializeSaveGame(save);
    expect(encoded.find("world.events.count=1") != std::string::npos,
           "save game persists world memory event count");
    expect(encoded.find("world.event.0.source=garage_pressure_memory") != std::string::npos,
           "save game persists world memory event source");

    const bs3d::SaveGame loaded = bs3d::deserializeSaveGame(encoded);
    expect(loaded.worldEvents.size() == 1, "world memory events round-trip through save text");
    expect(loaded.worldEvents.front().id == garage.id, "world memory event id round-trips");
    expect(loaded.worldEvents.front().type == bs3d::WorldEventType::PropertyDamage,
           "world memory event type round-trips");
    expect(loaded.worldEvents.front().location == bs3d::WorldLocationTag::Garage,
           "world memory event location round-trips");
    expectNear(loaded.worldEvents.front().position.x, garage.position.x, 0.001f,
               "world memory event position round-trips");
    expectNear(loaded.worldEvents.front().remainingSeconds, garage.remainingSeconds, 0.001f,
               "world memory event remaining time round-trips");
    expect(loaded.worldEvents.front().stackCount == garage.stackCount,
           "world memory event stack count round-trips");

    bs3d::WorldEventLedger restoredLedger;
    restoredLedger.restoreRecentEvents(loaded.worldEvents);
    const bs3d::LocalRewirServiceDigest digest = bs3d::buildLocalRewirServiceDigest(restoredLedger);
    expect(digest.wary == 1, "restored world memory reactivates local rewir service pressure");
    expect(std::find(digest.waryInteractionPointIds.begin(),
                     digest.waryInteractionPointIds.end(),
                     "garage_rysiek") != digest.waryInteractionPointIds.end(),
           "restored world memory keeps the garage service consequence visible");
}

void saveGameRewirPressureReliefLongTailPersistsAcrossReload() {
    bs3d::SaveGame save;

    bs3d::WorldEvent garage;
    garage.id = 31;
    garage.type = bs3d::WorldEventType::PropertyDamage;
    garage.location = bs3d::WorldLocationTag::Garage;
    garage.position = {-18.0f, 0.0f, 23.0f};
    garage.intensity = 1.8f;
    garage.remainingSeconds = 12.5f;
    garage.source = "garage_pressure_memory";
    garage.stackCount = 2;
    garage.ageSeconds = 0.0f;
    save.worldEvents.push_back(garage);

    const std::string encoded = bs3d::serializeSaveGame(save);
    expect(encoded.find("world.events.count=1") != std::string::npos,
           "save game persists a pressure-relevant memory event");
    expect(encoded.find("world.event.0.source=garage_pressure_memory") != std::string::npos,
           "save game preserves pressure event source");

    const bs3d::SaveGame loaded = bs3d::deserializeSaveGame(encoded);
    bs3d::WorldEventLedger ledger;
    ledger.restoreRecentEvents(loaded.worldEvents);

    const auto pressureBeforeRelief =
        bs3d::resolveLocalRewirServiceStateForInteraction(ledger, {-18.0f, 0.0f, 23.0f}, "garage_service_rysiek");
    expect(pressureBeforeRelief.has_value() && pressureBeforeRelief->access == bs3d::LocalRewirServiceAccess::Wary,
           "restored garage memory starts in wary state before relief");

    const auto initialDigest = bs3d::buildLocalRewirServiceDigest(ledger);
    expect(initialDigest.wary == 1, "restored pressure event is counted as an active rewir pressure consequence");

    const bs3d::LocalRewirServiceReliefResult relief =
        bs3d::applyLocalRewirServiceRelief(ledger, {-18.0f, 0.0f, 23.0f}, "garage_service_rysiek");
    expect(relief.softenedEvents == 1, "applying garage relief softens the matching saved memory");
    expect(relief.location == bs3d::WorldLocationTag::Garage,
           "relief result reports the expected rewir location");

    const auto pressureAfterRelief =
        bs3d::resolveLocalRewirServiceStateForInteraction(ledger, {-18.0f, 0.0f, 23.0f}, "garage_service_rysiek");
    expect(pressureAfterRelief.has_value() && pressureAfterRelief->access == bs3d::LocalRewirServiceAccess::Normal,
           "relief returns the same rewir to calm state immediately");

    const std::vector<bs3d::WorldEvent> softened = ledger.queryByLocationAndSource(
        bs3d::WorldLocationTag::Garage, "garage_pressure_memory");
    expect(!softened.empty(), "relief leaves a persisted rewir memory record for long-tail decay");
    expect(softened.front().intensity < 1.0f, "relieved memory drops below watchful pressure threshold");
    expect(softened.front().stackCount == 1, "relieved memory resets stack count to one");

    const auto calmDigest = bs3d::buildLocalRewirServiceDigest(ledger);
    expect(calmDigest.wary == 0, "long-tail memory remains but does not keep rewir service in wary mode");

    const float nearExpiryTail = softened.front().remainingSeconds;
    expect(nearExpiryTail > 0.0f && nearExpiryTail <= 2.5f,
           "relief caps long-tail memory to the short reactivation window");

    const float preExpiryStep = std::max(0.0f, nearExpiryTail - 0.05f);
    ledger.update(preExpiryStep);
    expect(!ledger.recentEvents().empty(), "relieved memory is still present before its final expiry");

    ledger.update(0.1f);
    expect(ledger.recentEvents().empty(), "long-tail memory fully expires when timer ends");
    const auto postExpireDigest = bs3d::buildLocalRewirServiceDigest(ledger);
    expect(postExpireDigest.wary == 0 && postExpireDigest.total > 0,
           "post-expiry digest has no wary services and keeps catalog coverage");
}

void saveGameRoundTripsWorldReactionRhythm() {
    bs3d::SaveGame save;
    save.przypalValue = 52.0f;
    save.przypalDecayDelayRemaining = 1.1f;
    save.przypalBand = bs3d::PrzypalBand::Hot;
    save.przypalBandPulseAvailable = true;
    save.przypalContributors.push_back(
        {bs3d::WorldEventType::PropertyDamage, bs3d::WorldLocationTag::Garage, 22.4f, 3.25f});
    save.eventCooldowns.push_back({bs3d::WorldEventType::PropertyDamage, "vehicle_impact", 2.75f});

    const std::string encoded = bs3d::serializeSaveGame(save);
    expect(encoded.find("przypal.decayDelayRemaining=1.1") != std::string::npos,
           "save game persists Przypal decay delay");
    expect(encoded.find("przypal.contributors.count=1") != std::string::npos,
           "save game persists Przypal contributor count");
    expect(encoded.find("world.cooldowns.count=1") != std::string::npos,
           "save game persists world-event cooldown count");

    const bs3d::SaveGame loaded = bs3d::deserializeSaveGame(encoded);
    expectNear(loaded.przypalDecayDelayRemaining, save.przypalDecayDelayRemaining, 0.001f,
               "Przypal decay delay round-trips");
    expect(loaded.przypalBand == bs3d::PrzypalBand::Hot, "Przypal band round-trips");
    expect(loaded.przypalBandPulseAvailable, "Przypal band pulse round-trips");
    expect(loaded.przypalContributors.size() == 1, "Przypal contributors round-trip");
    expect(loaded.przypalContributors.front().location == bs3d::WorldLocationTag::Garage,
           "Przypal contributor location round-trips");
    expect(loaded.eventCooldowns.size() == 1, "world-event cooldowns round-trip");
    expectNear(loaded.eventCooldowns.front().remainingSeconds, 2.75f, 0.001f,
               "world-event cooldown remaining time round-trips");

    bs3d::PrzypalSystem restoredPrzypal;
    restoredPrzypal.restore({loaded.przypalValue,
                             loaded.przypalDecayDelayRemaining,
                             loaded.przypalBand,
                             loaded.przypalBandPulseAvailable,
                             loaded.przypalContributors});
    expect(restoredPrzypal.band() == bs3d::PrzypalBand::Hot,
           "restored save snapshot preserves Przypal band");
    expect(restoredPrzypal.consumeBandPulse(),
           "restored save snapshot preserves pending Przypal pulse");
    restoredPrzypal.update(0.5f);
    expectNear(restoredPrzypal.value(), 52.0f, 0.001f,
               "restored Przypal keeps decay delay rhythm after load");

    bs3d::WorldEventEmitterCooldowns restoredCooldowns;
    restoredCooldowns.restore(loaded.eventCooldowns);
    expect(!restoredCooldowns.consume(bs3d::WorldEventType::PropertyDamage, "vehicle_impact", 1.0f),
           "restored world cooldown prevents immediate repeated emission");
}

void saveGameRoundTripsCompletedLocalRewirFavors() {
    bs3d::SaveGame save;
    save.completedLocalRewirFavorIds.push_back("garage_favor_rysiek");
    save.completedLocalRewirFavorIds.push_back("parking_favor_parkingowy");

    const std::string encoded = bs3d::serializeSaveGame(save);
    expect(encoded.find("local.favors.completed.count=2") != std::string::npos,
           "save game persists completed local favor count");
    expect(encoded.find("local.favor.completed.0.id=garage_favor_rysiek") != std::string::npos,
           "save game persists first completed local favor id");

    const bs3d::SaveGame loaded = bs3d::deserializeSaveGame(encoded);
    expect(loaded.completedLocalRewirFavorIds.size() == 2,
           "completed local favor ids round-trip through save text");
    expect(loaded.completedLocalRewirFavorIds.front() == "garage_favor_rysiek",
           "first completed local favor id round-trips");
    expect(loaded.completedLocalRewirFavorIds.back() == "parking_favor_parkingowy",
           "second completed local favor id round-trips");

    const bs3d::SaveGameValidation valid = bs3d::validateSaveGame(loaded);
    expect(valid.ok, "completed local favor ids validate when they are simple stable ids");

    bs3d::SaveGame corrupt = loaded;
    corrupt.completedLocalRewirFavorIds.push_back("bad\nfavor");
    const bs3d::SaveGameValidation rejected = bs3d::validateSaveGame(corrupt);
    expect(!rejected.ok, "completed local favor ids reject newline payloads");

    bs3d::SaveGame unknown = loaded;
    unknown.completedLocalRewirFavorIds.push_back("garage_favor_from_another_timeline");
    const bs3d::SaveGameValidation rejectedUnknown = bs3d::validateSaveGame(unknown);
    expect(!rejectedUnknown.ok, "completed local favor ids must exist in the authored catalog");
}

void localRewirFavorDigestReportsActiveAndCompletedFavors() {
    bs3d::WorldEventLedger ledger;

    bs3d::WorldEvent garage;
    garage.type = bs3d::WorldEventType::PropertyDamage;
    garage.location = bs3d::WorldLocationTag::Garage;
    garage.position = {-18.0f, 0.0f, 23.0f};
    garage.intensity = 1.6f;
    garage.remainingSeconds = 18.0f;
    garage.source = "garage_favor_pressure";
    ledger.add(garage);

    bs3d::WorldEvent trash = garage;
    trash.location = bs3d::WorldLocationTag::Trash;
    trash.position = {9.0f, 0.0f, -4.0f};
    trash.source = "trash_favor_pressure";
    ledger.add(trash);
    ledger.update(5.0f);

    const bs3d::LocalRewirFavorDigest digest =
        bs3d::buildLocalRewirFavorDigest(ledger, {"garage_favor_rysiek"});
    expect(digest.total == static_cast<int>(bs3d::localRewirFavorCatalog().size()),
           "local favor digest counts every authored favor");
    expect(digest.completed == 1, "local favor digest counts completed favor ids");
    expect(digest.active == 1, "local favor digest reports only uncompleted hot favors as active");
    expect(digest.firstActiveInteractionPointId == "trash_favor_dozorca_point",
           "local favor digest exposes the first active uncompleted favor point");
}

void saveGameValidationRejectsCorruptRuntimeState() {
    const bs3d::SaveGameValidation valid = bs3d::validateSaveGame(bs3d::deserializeSaveGame(
        "version=1\n"
        "mission.phase=5\n"
        "mission.phaseSeconds=1.25\n"
        "checkpoint=phase_5\n"
        "paragon.phase=2\n"
        "player.position=1,0,2\n"
        "player.yaw=0.25\n"
        "player.inVehicle=1\n"
        "vehicle.position=3,0,4\n"
        "vehicle.yaw=0.5\n"
        "vehicle.condition=91\n"
        "przypal.value=20\n"));
    expect(valid.ok, "well formed save validates before boot restore");

    const bs3d::SaveGameValidation corrupt = bs3d::validateSaveGame(bs3d::deserializeSaveGame(
        "version=99\n"
        "mission.phase=4242\n"
        "paragon.phase=-9\n"
        "player.position=nan_payload\n"
        "vehicle.condition=9000\n"
        "przypal.value=-10\n"));
    expect(!corrupt.ok, "corrupt save is rejected instead of being silently restored");
    expect(corrupt.errors.size() >= 4, "validation reports every unsafe save field");
}

void saveGameFileWritesAreAtomicAndValidatedOnLoad() {
    const std::string path = "artifacts/test_save_atomic.sav";
    bs3d::SaveGame save;
    save.mission.phase = bs3d::MissionPhase::ReturnToLot;
    save.mission.checkpointId = "phase_return";
    save.playerPosition = {4.0f, 0.0f, 8.0f};
    save.vehicleCondition = 64.0f;

    expect(bs3d::saveGameToFile(save, path), "atomic save write succeeds for valid slot");

    bs3d::SaveGame loaded;
    expect(bs3d::loadSaveGameFromFile(path, loaded), "validated save slot loads after atomic write");
    expect(loaded.mission.phase == bs3d::MissionPhase::ReturnToLot,
           "validated load restores saved mission phase");
    expect(loaded.mission.checkpointId == "phase_return", "validated load restores checkpoint id");
}

void missionControllerCanRestoreSaveStateAndUseDataObjectiveOverrides() {
    bs3d::DialogueSystem dialogue;
    bs3d::MissionController mission(dialogue);
    mission.setObjectiveOverride(bs3d::MissionPhase::DriveToShop, "Jedź pod sklep - data");

    mission.restoreForSave(bs3d::MissionPhase::DriveToShop, 7.5f);
    expect(mission.phase() == bs3d::MissionPhase::DriveToShop, "mission restore accepts saved runtime phase");
    expect(!mission.chaseWanted(), "restoring a non-chase phase does not start pursuit");
    expect(mission.phaseSeconds() == 7.5f, "mission restore keeps phase elapsed seconds");
    expect(mission.objectiveText() == "Jedź pod sklep - data",
           "mission objective can be provided by runtime mission data");

    mission.restoreForSave(bs3d::MissionPhase::ChaseActive, 2.0f);
    expect(mission.chaseWanted(), "restoring chase phase arms the chase runtime once");
}

void paragonMissionCanRestorePersistedConsequences() {
    bs3d::DialogueSystem dialogue;
    bs3d::ParagonMission paragon(dialogue);

    paragon.restoreForSave(bs3d::ParagonMissionPhase::ReturnToZenon,
                           bs3d::ParagonSolution::Chaos,
                           true);
    expect(paragon.phase() == bs3d::ParagonMissionPhase::ReturnToZenon,
           "paragon mission restores saved phase");
    expect(paragon.solution() == bs3d::ParagonSolution::Chaos,
           "paragon mission restores saved solution branch");
    expect(paragon.shopBanActive(), "paragon mission restores shop ban consequence");
    expect(paragon.objectiveText().find("okienko") != std::string::npos,
           "restored paragon consequence affects runtime objective");
}

void retryCheckpointSnapshotRestoresWorldHeatEventsAndCooldowns() {
    bs3d::PrzypalSystem przypal;
    bs3d::WorldEventLedger ledger;
    bs3d::WorldEventEmitterCooldowns cooldowns;

    const bs3d::WorldEventAddResult added =
        ledger.add({0,
                    bs3d::WorldEventType::PropertyDamage,
                    bs3d::WorldLocationTag::Parking,
                    {1.0f, 0.0f, 1.0f},
                    1.2f,
                    10.0f,
                    "test_prop"});
    przypal.onWorldEvent(added);
    expect(cooldowns.consume(bs3d::WorldEventType::PropertyDamage, "test_prop", 4.0f),
           "cooldown starts consumed before snapshot");

    const bs3d::WorldReactionSnapshot snapshot =
        bs3d::captureWorldReactionSnapshot(przypal, ledger, cooldowns);

    przypal.clear();
    ledger.clear();
    cooldowns.clear();
    bs3d::restoreWorldReactionSnapshot(snapshot, przypal, ledger, cooldowns);

    expect(przypal.value() > 0.0f, "retry checkpoint restores Przypal heat");
    expect(ledger.recentEvents().size() == 1, "retry checkpoint restores recent world event ledger");
    expect(!cooldowns.consume(bs3d::WorldEventType::PropertyDamage, "test_prop", 4.0f),
           "retry checkpoint restores emitter cooldown state");
}

void fixedStepClockClampsLargeFramesAndReportsSubsteps() {
    bs3d::FixedStepClock clock({1.0f / 60.0f, 0.10f, 5});

    bs3d::FixedStepFrame first = clock.advance(1.0f / 30.0f);
    expect(first.steps == 2, "1/30 frame produces two 60 Hz simulation steps");
    expectNear(first.fixedDeltaSeconds, 1.0f / 60.0f, 0.0001f, "fixed step uses 60 Hz dt");
    expect(first.alpha >= 0.0f && first.alpha <= 1.0f, "interpolation alpha is bounded");

    bs3d::FixedStepFrame hitch = clock.advance(0.80f);
    expect(hitch.clamped, "large hitch is clamped");
    expect(hitch.steps <= 5, "substep count is capped for stability");
    expect(hitch.consumedSeconds <= 5.0f / 60.0f + 0.001f,
           "consumed simulation time is bounded by max steps");
}

void renderInterpolationClampsAlphaAndUsesShortestYawPath() {
    const bs3d::Vec3 midpoint = bs3d::interpolateVec3({0.0f, 1.0f, -2.0f}, {10.0f, 5.0f, 6.0f}, 0.5f);
    expectNear(midpoint.x, 5.0f, 0.001f, "position interpolation lerps x");
    expectNear(midpoint.y, 3.0f, 0.001f, "position interpolation lerps y");
    expectNear(midpoint.z, 2.0f, 0.001f, "position interpolation lerps z");

    const bs3d::Vec3 clampedLow = bs3d::interpolateVec3({1.0f, 2.0f, 3.0f}, {9.0f, 8.0f, 7.0f}, -1.0f);
    expectNear(clampedLow.x, 1.0f, 0.001f, "alpha below zero clamps to previous state");

    const bs3d::Vec3 clampedHigh = bs3d::interpolateVec3({1.0f, 2.0f, 3.0f}, {9.0f, 8.0f, 7.0f}, 2.0f);
    expectNear(clampedHigh.x, 9.0f, 0.001f, "alpha above one clamps to current state");

    const float from = 170.0f * bs3d::Pi / 180.0f;
    const float to = -170.0f * bs3d::Pi / 180.0f;
    const float yaw = bs3d::interpolateYawRadians(from, to, 0.5f);
    expect(std::fabs(std::fabs(yaw) - bs3d::Pi) < 0.001f,
           "yaw interpolation uses shortest wrap-around path instead of spinning through zero");
}

void renderInterpolationBlendsVehicleAndCameraState() {
    bs3d::VehicleRuntimeState previousVehicle;
    previousVehicle.position = {0.0f, 0.0f, 0.0f};
    previousVehicle.yawRadians = 0.0f;
    previousVehicle.speed = 2.0f;
    previousVehicle.lateralSlip = 1.0f;
    previousVehicle.instability = 0.2f;
    previousVehicle.frontWheelSteerRadians = -0.2f;
    previousVehicle.condition = 80.0f;
    previousVehicle.conditionBand = bs3d::VehicleConditionBand::CosStuka;

    bs3d::VehicleRuntimeState currentVehicle = previousVehicle;
    currentVehicle.position = {10.0f, 0.0f, 4.0f};
    currentVehicle.yawRadians = bs3d::Pi;
    currentVehicle.speed = 6.0f;
    currentVehicle.lateralSlip = 3.0f;
    currentVehicle.instability = 0.6f;
    currentVehicle.frontWheelSteerRadians = 0.2f;
    currentVehicle.condition = 70.0f;
    currentVehicle.conditionBand = bs3d::VehicleConditionBand::Zlom;

    const bs3d::VehicleRuntimeState vehicle = bs3d::interpolateVehicleRuntimeState(previousVehicle, currentVehicle, 0.25f);
    expectNear(vehicle.position.x, 2.5f, 0.001f, "vehicle render state interpolates position");
    expectNear(vehicle.position.z, 1.0f, 0.001f, "vehicle render state interpolates position z");
    expectNear(vehicle.speed, 3.0f, 0.001f, "vehicle render state interpolates speed");
    expectNear(vehicle.lateralSlip, 1.5f, 0.001f, "vehicle render state interpolates lateral slip");
    expectNear(vehicle.instability, 0.3f, 0.001f, "vehicle render state interpolates instability");
    expectNear(vehicle.frontWheelSteerRadians, -0.1f, 0.001f, "vehicle render state interpolates wheel steering");
    expectNear(vehicle.condition, 77.5f, 0.001f, "vehicle render state interpolates condition");
    expect(vehicle.conditionBand == bs3d::VehicleConditionBand::Zlom,
           "discrete vehicle render fields come from current simulation state");

    bs3d::CameraRigState previousCamera;
    previousCamera.position = {0.0f, 2.0f, -6.0f};
    previousCamera.target = {0.0f, 1.0f, 0.0f};
    previousCamera.fovY = 50.0f;
    previousCamera.cameraYawRadians = 0.0f;

    bs3d::CameraRigState currentCamera = previousCamera;
    currentCamera.position = {6.0f, 4.0f, -2.0f};
    currentCamera.target = {4.0f, 1.0f, 3.0f};
    currentCamera.fovY = 60.0f;
    currentCamera.cameraYawRadians = bs3d::Pi;

    const bs3d::CameraRigState camera = bs3d::interpolateCameraRigState(previousCamera, currentCamera, 0.5f);
    expectNear(camera.position.x, 3.0f, 0.001f, "camera render state interpolates position");
    expectNear(camera.position.y, 3.0f, 0.001f, "camera render state interpolates height");
    expectNear(camera.target.x, 2.0f, 0.001f, "camera render state interpolates target");
    expectNear(camera.target.z, 1.5f, 0.001f, "camera render state interpolates target z");
    expectNear(camera.fovY, 55.0f, 0.001f, "camera render state interpolates fov");
}

void paragonMissionSpecAuthorsActorsChoicesEventsAndConsequences() {
    const bs3d::ParagonMissionSpec& spec = bs3d::defaultParagonMissionSpec();

    expect(spec.actor("bogus") != nullptr, "paragon spec authors Bogus actor");
    expect(spec.actor("zenon") != nullptr, "paragon spec authors Zenon actor");
    expect(spec.actor("lolek") != nullptr, "paragon spec authors Lolek actor");
    expect(spec.actor("receipt_holder") != nullptr, "paragon spec authors receipt holder actor");
    expect(spec.actor("halina") != nullptr, "paragon spec authors Halina reaction source");

    const bs3d::ParagonChoiceSpec* peaceful = spec.choice(bs3d::ParagonSolution::Peaceful);
    const bs3d::ParagonChoiceSpec* trick = spec.choice(bs3d::ParagonSolution::Trick);
    const bs3d::ParagonChoiceSpec* chaos = spec.choice(bs3d::ParagonSolution::Chaos);

    expect(peaceful != nullptr && peaceful->prompt.find("spokojnie") != std::string::npos,
           "peaceful choice has readable peaceful prompt");
    expect(trick != nullptr && trick->prompt.find("zakombinuj") != std::string::npos,
           "trick choice has readable trick prompt");
    expect(chaos != nullptr && chaos->prompt.find("chaos") != std::string::npos,
           "chaos choice advertises chaos path");
    expect(chaos != nullptr && chaos->worldEventType == bs3d::WorldEventType::ShopTrouble,
           "chaos path emits ShopTrouble");
    expect(chaos != nullptr && chaos->shopBanConsequence,
           "chaos path authors runtime shop ban consequence");
    expect(chaos != nullptr && chaos->intensity >= trick->intensity,
           "chaos path is louder than trick path");
}

void paragonMissionPolishUsesReadableObjectivesAndPostChaosShopLine() {
    bs3d::DialogueSystem dialogue;
    bs3d::ParagonMission mission(dialogue);

    expect(mission.start(true), "paragon can start after intro");
    expect(mission.objectiveText() == "Idź do sklepu Zenona", "start objective points to Zenon's shop clearly");

    mission.onZenonAccusesBogus();
    expect(mission.objectiveText() == "Zapytaj Lolka o paragon", "clue objective points to Lolek");

    mission.onReceiptHolderFound();
    expect(mission.objectiveText().find("E spokojnie") != std::string::npos,
           "resolve objective teaches peaceful option");
    expect(mission.objectiveText().find("F kombinuj") != std::string::npos,
           "resolve objective teaches trick option");
    expect(mission.objectiveText().find("LMB/X chaos") != std::string::npos,
           "resolve objective teaches chaos option");

    mission.resolve(bs3d::ParagonSolution::Chaos);
    expect(mission.objectiveText() == "Wróć do Zenona przez okienko",
           "chaos return objective reflects shop ban consequence");
    expect(mission.shopBanActive(), "chaos activates runtime shop ban before return");
    mission.onReturnedToZenon();
    expect(mission.zenonShopLineAfterCompletion().find("okienko") != std::string::npos,
           "Zenon has a shop line for chaos/ban state");
}

void chaseSystemEscapesOnlyAfterSustainedDistance() {
    bs3d::ChaseSystem chase;
    chase.start({0.0f, 0.0f, 0.0f});

    bs3d::ChaseResult close = chase.update({1.0f, 0.0f, 0.0f}, 0.25f);
    expect(close == bs3d::ChaseResult::Running, "chase warning grace starts at fail distance");

    chase.start({0.0f, 0.0f, 0.0f});
    bs3d::ChaseResult first = chase.update({30.0f, 0.0f, 0.0f}, 1.0f);
    expect(first == bs3d::ChaseResult::Running, "escape requires sustained distance");

    bs3d::ChaseResult second = chase.update({30.0f, 0.0f, 0.0f}, 2.1f);
    expect(second == bs3d::ChaseResult::Escaped, "escape succeeds after timer");
    expect(chase.state() == bs3d::ChaseState::Escaped, "chase state records escape");
}

void chaseRequiresCaughtGraceBeforeFailing() {
    bs3d::ChaseSystem chase;
    chase.start({0.0f, 0.0f, 0.0f});

    bs3d::ChaseResult first = chase.update({1.0f, 0.0f, 0.0f}, 0.2f);
    expect(first == bs3d::ChaseResult::Running, "caught grace prevents instant failure");
    expect(chase.caughtProgress() > 0.0f, "caught timer records pressure");

    bs3d::ChaseResult second = chase.update({1.0f, 0.0f, 0.0f}, 1.1f);
    expect(second == bs3d::ChaseResult::Caught, "caught state triggers after grace duration");

    chase.start({0.0f, 0.0f, 0.0f});
    chase.update({1.0f, 0.0f, 0.0f}, 0.4f);
    chase.update({8.0f, 0.0f, 0.0f}, 0.2f);
    expect(chase.caughtProgress() == 0.0f, "caught timer resets after player opens distance");
}

void chaseSuggestsFairRubberbandFollowSpeed() {
    bs3d::ChaseSystem chase;
    chase.start({0.0f, 0.0f, 0.0f});

    const float closeSpeed = chase.pursuerFollowSpeed(5.0f, 0.2f);
    const float farSpeed = chase.pursuerFollowSpeed(40.0f, 0.2f);

    expect(farSpeed > closeSpeed, "rubberband speed increases when chase is far behind");
    expect(closeSpeed < 8.3f, "close pursuer speed leaves parking-lot room to recover after contact");
    expect(farSpeed < 16.0f, "rubberband speed is capped below unfair teleport speed");
}

void chaseEscapeDistanceFitsSmallRewirLoop() {
    bs3d::ChaseSystem chase;
    chase.start({0.0f, 0.0f, 0.0f});

    bs3d::ChaseResult result = chase.update({20.0f, 0.0f, 0.0f}, 1.0f);
    expect(result == bs3d::ChaseResult::Running, "escape still requires a sustained gap");

    result = chase.update({20.0f, 0.0f, 0.0f}, 1.5f);
    expect(result == bs3d::ChaseResult::Escaped, "rewir-sized chase can be escaped at a readable 20m gap");
}

void chaseContactRecoverAndFastImpactDoNotInstantlyFailMission() {
    bs3d::ChaseSystem chase;
    chase.start({0.0f, 0.0f, 0.0f});

    bs3d::ChaseUpdateContext recover;
    recover.playerPosition = {0.5f, 0.0f, 0.0f};
    recover.pursuerPosition = {0.0f, 0.0f, 0.0f};
    recover.contactRecoverActive = true;
    recover.lineOfSight = false;
    bs3d::ChaseResult result = chase.updateWithContext(recover, 2.0f);
    expect(result == bs3d::ChaseResult::Running, "contact recovery does not fail mission while cars are untangling");
    expectNear(chase.caughtProgress(), 0.0f, 0.001f, "contact recovery keeps caught timer clear");

    bs3d::ChaseUpdateContext fastImpact;
    fastImpact.playerPosition = {0.5f, 0.0f, 0.0f};
    fastImpact.playerVelocity = {12.0f, 0.0f, 0.0f};
    fastImpact.pursuerPosition = {0.0f, 0.0f, 0.0f};
    fastImpact.pursuerVelocity = {-8.0f, 0.0f, 0.0f};
    result = chase.updateWithContext(fastImpact, 2.0f);
    expect(result == bs3d::ChaseResult::Running, "high relative speed contact is a crash/recover beat, not a clean caught fail");
    expectNear(chase.caughtProgress(), 0.0f, 0.001f, "fast impact does not build caught progress");

    bs3d::ChaseUpdateContext controlled;
    controlled.playerPosition = {0.5f, 0.0f, 0.0f};
    controlled.playerVelocity = {0.5f, 0.0f, 0.0f};
    controlled.pursuerPosition = {0.0f, 0.0f, 0.0f};
    controlled.pursuerVelocity = {0.4f, 0.0f, 0.0f};
    result = chase.updateWithContext(controlled, 1.1f);
    expect(result == bs3d::ChaseResult::Caught, "controlled close pressure can still fail the mission");
}

void dialogueAdvancesAfterDuration() {
    bs3d::DialogueSystem dialogue;
    dialogue.push({"Boguś", "Idziesz do sklepu i wracasz.", 1.0f, bs3d::DialogueChannel::MissionCritical});
    dialogue.push({"Zenon", "Tylko bez przypału pod drzwiami.", 2.0f, bs3d::DialogueChannel::MissionCritical});

    expect(dialogue.hasLine(), "dialogue starts with active line");
    expect(dialogue.currentLine().speaker == "Boguś", "first line is active");

    dialogue.update(1.1f);
    expect(dialogue.currentLine().speaker == "Zenon", "dialogue advances after duration");

    dialogue.update(2.1f);
    expect(!dialogue.hasLine(), "dialogue queue empties after final duration");
}

void dialogueChannelsKeepCriticalLinesAboveReactions() {
    bs3d::DialogueSystem dialogue;
    dialogue.push({"Halina", "Ja to widze z okna!", 4.0f, bs3d::DialogueChannel::Reaction});
    expect(dialogue.hasLine(), "reaction line can be active by itself");
    expect(dialogue.currentLine().speaker == "Halina", "reaction line is visible when no critical line exists");
    expect(!dialogue.hasBlockingLine(), "reaction subtitle does not block mission UI");

    dialogue.push({"Misja", "Zgub przypal.", 1.0f, bs3d::DialogueChannel::FailChase});
    expect(dialogue.currentLine().speaker == "Misja", "fail/chase channel overrides lower reaction subtitle");
    expect(dialogue.currentLine().channel == bs3d::DialogueChannel::FailChase, "current line exposes active channel");
    expect(dialogue.hasBlockingLine(), "fail/chase line blocks low-priority reaction spam");

    dialogue.update(1.1f);
    expect(dialogue.currentLine().speaker == "Halina", "reaction resumes after critical line expires");
}

void gameplayCameraModeIgnoresSubtitleLinesUnlessInteractionCameraIsExplicit() {
    bs3d::GameplayCameraModeInput input;
    input.dialogueLineActive = true;
    input.playerSpeed = 0.0f;
    input.sprintSpeed = 6.2f;

    expect(bs3d::resolveGameplayCameraMode(input) == bs3d::CameraRigMode::OnFootNormal,
           "subtitle lines alone do not switch camera to interaction framing");

    input.explicitInteractionCameraActive = true;
    expect(bs3d::resolveGameplayCameraMode(input) == bs3d::CameraRigMode::Interaction,
           "explicit interaction camera request uses interaction framing");

    input.playerInVehicle = true;
    expect(bs3d::resolveGameplayCameraMode(input) == bs3d::CameraRigMode::Vehicle,
           "vehicle camera wins over any stale interaction camera request");
}

void stableCameraModeForcesCleanFramingExceptExplicitInteraction() {
    expect(bs3d::resolveStableCameraMode(bs3d::CameraRigMode::OnFootNormal, false) ==
               bs3d::CameraRigMode::OnFootNormal,
           "stable camera off keeps normal mode");
    expect(bs3d::resolveStableCameraMode(bs3d::CameraRigMode::OnFootSprint, false) ==
               bs3d::CameraRigMode::OnFootSprint,
           "stable camera off keeps sprint mode");
    expect(bs3d::resolveStableCameraMode(bs3d::CameraRigMode::Vehicle, true) ==
               bs3d::CameraRigMode::OnFootNormal,
           "stable camera removes vehicle look-ahead and FOV tuning");
    expect(bs3d::resolveStableCameraMode(bs3d::CameraRigMode::OnFootSprint, true) ==
               bs3d::CameraRigMode::OnFootNormal,
           "stable camera removes sprint look-ahead and FOV tuning");
    expect(bs3d::resolveStableCameraMode(bs3d::CameraRigMode::Interaction, true) ==
               bs3d::CameraRigMode::Interaction,
           "stable camera keeps explicit interaction framing");
}

void chaseStartGraceBlocksImmediateCaughtFail() {
    bs3d::ChaseSystem chase;
    chase.start({0.0f, 0.0f, 0.0f}, 2.0f);

    bs3d::ChaseResult result = chase.update({0.5f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.5f);
    expect(result == bs3d::ChaseResult::Running, "chase grace prevents immediate caught fail");
    expectNear(chase.caughtProgress(), 0.0f, 0.001f, "caught timer does not advance during start grace");

    result = chase.update({0.5f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.6f);
    expect(result == bs3d::ChaseResult::Running, "grace expiry does not instantly fail on same frame");
    result = chase.update({0.5f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f);
    expect(result == bs3d::ChaseResult::Caught, "caught fail can happen after grace and caught duration");
}

void chaseDifficultyScalesEscapeAndFailParameters() {
    bs3d::ChaseSystem chase;
    chase.start({0.0f, 0.0f, 0.0f});

    const float baseEscape = chase.requiredEscapeSeconds();
    const float baseEscapeDist = chase.escapeDistance();
    const float baseFailDist = chase.failDistance();

    chase.setDifficulty(1.0f);
    expectNear(chase.requiredEscapeSeconds(), baseEscape, 0.001f, "difficulty 1.0 keeps base escape seconds");
    expectNear(chase.escapeDistance(), baseEscapeDist, 0.001f, "difficulty 1.0 keeps base escape distance");
    expectNear(chase.failDistance(), baseFailDist, 0.001f, "difficulty 1.0 keeps base fail distance");

    chase.setDifficulty(1.5f);
    expectNear(chase.requiredEscapeSeconds(), baseEscape * 1.5f, 0.001f, "difficulty 1.5 scales escape seconds");
    expectNear(chase.escapeDistance(), baseEscapeDist * 1.5f, 0.001f, "difficulty 1.5 scales escape distance");
    expectNear(chase.failDistance(), baseFailDist * 1.5f, 0.001f, "difficulty 1.5 scales fail distance");

    chase.setDifficulty(2.0f);
    expectNear(chase.requiredEscapeSeconds(), baseEscape * 2.0f, 0.001f, "difficulty 2.0 doubles escape seconds");
    expectNear(chase.escapeDistance(), baseEscapeDist * 2.0f, 0.001f, "difficulty 2.0 doubles escape distance");
    expectNear(chase.failDistance(), baseFailDist * 2.0f, 0.001f, "difficulty 2.0 doubles fail distance");

    chase.setDifficulty(3.0f);
    expectNear(chase.requiredEscapeSeconds(), baseEscape * 2.0f, 0.001f, "difficulty clamps at 2.0 for escape seconds");
}

void controlContextMapsSpaceAsJumpOnFootAndHandbrakeInVehicle() {
    bs3d::RawInputState raw;
    raw.dynamicPressed = true;
    raw.dynamicDown = true;

    bs3d::InputState onFoot = bs3d::mapRawInputToInputState(raw, bs3d::ControlContext::OnFootExploration);
    expect(onFoot.jumpPressed, "Space is jump in on-foot exploration");
    expect(!onFoot.handbrake, "Space is not handbrake on foot");

    bs3d::InputState vehicle = bs3d::mapRawInputToInputState(raw, bs3d::ControlContext::VehicleDriving);
    expect(!vehicle.jumpPressed, "Space is not jump in vehicle");
    expect(vehicle.handbrake, "Space is handbrake in vehicle context");
}

void controlContextMapsShiftAsSprintOnFootAndBoostInVehicle() {
    bs3d::RawInputState raw;
    raw.fastDown = true;
    raw.moveForwardDown = true;

    bs3d::InputState onFoot = bs3d::mapRawInputToInputState(raw, bs3d::ControlContext::OnFootExploration);
    expect(onFoot.sprint, "Shift is sprint on foot");
    expect(!onFoot.vehicleBoost, "Shift is not vehicle boost on foot");

    bs3d::InputState vehicle = bs3d::mapRawInputToInputState(raw, bs3d::ControlContext::VehicleDriving);
    expect(!vehicle.sprint, "Shift is not sprint in vehicle");
    expect(vehicle.vehicleBoost, "Shift is desperate gas in vehicle");
    expect(vehicle.accelerate, "W remains accelerator in vehicle");
}

void controlContextLocksMovementDuringHardInteraction() {
    bs3d::RawInputState raw;
    raw.moveForwardDown = true;
    raw.fastDown = true;
    raw.dynamicPressed = true;
    raw.dynamicDown = true;
    raw.usePressed = true;

    bs3d::InputState locked = bs3d::mapRawInputToInputState(raw, bs3d::ControlContext::InteractionLocked);

    expect(!locked.moveForward && !locked.sprint && !locked.jumpPressed, "interaction lock blocks on-foot movement");
    expect(!locked.accelerate && !locked.handbrake && !locked.vehicleBoost, "interaction lock blocks vehicle controls");
    expect(locked.enterExitPressed, "interaction lock still lets E acknowledge/use");
}

void controlContextSupportsVehicleInteractionAliasAndHornFamily() {
    bs3d::RawInputState raw;
    raw.vehiclePressed = true;
    raw.primaryPressed = true;
    raw.primaryDown = true;

    bs3d::InputState onFoot = bs3d::mapRawInputToInputState(raw, bs3d::ControlContext::OnFootExploration);
    expect(onFoot.enterExitPressed, "F is an enter/interact alias on foot");
    expect(!onFoot.hornPressed, "primary action does not honk on foot");

    bs3d::InputState vehicle = bs3d::mapRawInputToInputState(raw, bs3d::ControlContext::VehicleDriving);
    expect(vehicle.enterExitPressed, "F exits vehicle in vehicle context");
    expect(vehicle.hornPressed && vehicle.hornDown, "LMB joins H as a horn/action family in vehicle");
}

void controlContextMapsOnFootCharacterActionButtons() {
    bs3d::RawInputState raw;
    raw.primaryPressed = true;
    raw.secondaryPressed = true;
    raw.carryPressed = true;
    raw.pushPressed = true;
    raw.drinkPressed = true;
    raw.smokePressed = true;
    raw.lowVaultPressed = true;
    raw.fallPressed = true;

    bs3d::InputState onFoot = bs3d::mapRawInputToInputState(raw, bs3d::ControlContext::OnFootExploration);
    expect(onFoot.primaryActionPressed, "LMB is the on-foot primary character action");
    expect(onFoot.secondaryActionPressed, "RMB press is the on-foot secondary character action");
    expect(onFoot.carryActionPressed, "C triggers the carry pose/action test hook on foot");
    expect(onFoot.pushActionPressed, "X triggers the push pose/action test hook on foot");
    expect(onFoot.drinkActionPressed, "Z triggers the drink pose/action test hook on foot");
    expect(onFoot.smokeActionPressed, "V triggers the smoke pose/action test hook on foot");
    expect(onFoot.lowVaultActionPressed, "Q triggers the low-vault pose/action test hook on foot");
    expect(onFoot.fallActionPressed, "G triggers the fall/get-up pose/action test hook on foot");
    expect(!onFoot.hornPressed, "on-foot LMB action does not become a horn");
}

void rawInputFrameEdgesAreClearedAfterFirstFixedStep() {
    bs3d::RawInputState raw;
    raw.moveForwardDown = true;
    raw.fastDown = true;
    raw.dynamicDown = true;
    raw.primaryDown = true;
    raw.hornDown = true;
    raw.mouseLookActive = true;
    raw.dynamicPressed = true;
    raw.usePressed = true;
    raw.vehiclePressed = true;
    raw.primaryPressed = true;
    raw.secondaryPressed = true;
    raw.carryPressed = true;
    raw.pushPressed = true;
    raw.drinkPressed = true;
    raw.smokePressed = true;
    raw.lowVaultPressed = true;
    raw.fallPressed = true;
    raw.hornPressed = true;
    raw.retryPressed = true;
    raw.toggleDebugPressed = true;
    raw.mouseCaptureTogglePressed = true;
    raw.mouseDeltaX = 8.0f;
    raw.mouseDeltaY = -5.0f;

    bs3d::clearRawInputFrameEdges(raw);

    expect(raw.moveForwardDown && raw.fastDown && raw.dynamicDown && raw.primaryDown && raw.hornDown,
           "held raw input survives additional fixed steps");
    expect(raw.mouseLookActive, "mouse look active state survives additional fixed steps");
    expect(!raw.dynamicPressed && !raw.usePressed && !raw.vehiclePressed,
           "gameplay pressed edges are cleared after first fixed step");
    expect(!raw.primaryPressed && !raw.secondaryPressed && !raw.carryPressed && !raw.pushPressed,
           "action pressed edges are cleared after first fixed step");
    expect(!raw.drinkPressed && !raw.smokePressed && !raw.lowVaultPressed && !raw.fallPressed,
           "pose pressed edges are cleared after first fixed step");
    expect(!raw.hornPressed && !raw.retryPressed && !raw.toggleDebugPressed && !raw.mouseCaptureTogglePressed,
           "meta pressed edges are cleared after first fixed step");
    expectNear(raw.mouseDeltaX, 0.0f, 0.001f, "mouse delta x is consumed only once per render frame");
    expectNear(raw.mouseDeltaY, 0.0f, 0.001f, "mouse delta y is consumed only once per render frame");
}

void controlContextKeepsCharacterActionsOutOfVehicleAndHardLocks() {
    bs3d::RawInputState raw;
    raw.primaryPressed = true;
    raw.primaryDown = true;
    raw.secondaryPressed = true;
    raw.carryPressed = true;
    raw.pushPressed = true;
    raw.drinkPressed = true;
    raw.smokePressed = true;
    raw.lowVaultPressed = true;
    raw.fallPressed = true;

    bs3d::InputState vehicle = bs3d::mapRawInputToInputState(raw, bs3d::ControlContext::VehicleDriving);
    expect(vehicle.hornPressed && vehicle.hornDown, "vehicle keeps LMB as horn");
    expect(!vehicle.primaryActionPressed && !vehicle.secondaryActionPressed, "vehicle suppresses on-foot action buttons");
    expect(!vehicle.carryActionPressed && !vehicle.pushActionPressed, "vehicle suppresses prop action hooks");
    expect(!vehicle.drinkActionPressed && !vehicle.smokeActionPressed, "vehicle suppresses ambient action hooks");
    expect(!vehicle.lowVaultActionPressed && !vehicle.fallActionPressed, "vehicle suppresses vault/fall action hooks");

    bs3d::InputState locked = bs3d::mapRawInputToInputState(raw, bs3d::ControlContext::InteractionLocked);
    expect(!locked.primaryActionPressed && !locked.secondaryActionPressed, "hard interaction lock suppresses character actions");
    expect(!locked.carryActionPressed && !locked.pushActionPressed, "hard interaction lock suppresses prop action hooks");
    expect(!locked.drinkActionPressed && !locked.smokeActionPressed, "hard interaction lock suppresses ambient action hooks");
    expect(!locked.lowVaultActionPressed && !locked.fallActionPressed, "hard interaction lock suppresses vault/fall action hooks");
}

void controlContextResolutionPrioritizesLocksBeforeVehicle() {
    bs3d::ControlContextState state;
    state.playerInVehicle = true;
    state.interactionLocked = true;
    state.highPrzypal = true;
    expect(bs3d::resolveControlContext(state) == bs3d::ControlContext::InteractionLocked,
           "hard interaction lock wins before vehicle controls");

    state.interactionLocked = false;
    expect(bs3d::resolveControlContext(state) == bs3d::ControlContext::VehicleDriving,
           "vehicle controls win over panic context while in car");

    state.playerInVehicle = false;
    expect(bs3d::resolveControlContext(state) == bs3d::ControlContext::OnFootPanic,
           "high Przypal can choose panic on-foot context");
}

void interactionLockedInputReachesPlayerMotorState() {
    bs3d::RawInputState raw;
    raw.moveForwardDown = true;
    raw.fastDown = true;
    raw.dynamicPressed = true;

    const bs3d::InputState input = bs3d::mapRawInputToInputState(raw, bs3d::ControlContext::InteractionLocked);
    expect(input.interactionLocked, "hard interaction context marks input as interaction locked");

    const bs3d::PlayerInputIntent intent = bs3d::PlayerInputIntent::fromInputState(input);
    expect(intent.interactionLocked, "player intent carries the hard interaction lock into the motor");

    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    motor.update(intent, collision, 1.0f / 60.0f);

    expect(motor.state().movementMode == bs3d::PlayerMovementMode::InteractionLocked,
           "motor exposes interaction locked movement mode for presentation and debug");
}

void vehicleMovesForwardAndClampsSpeed() {
    bs3d::VehicleController vehicle;
    vehicle.setPosition({0.0f, 0.0f, 0.0f});

    bs3d::InputState input;
    input.accelerate = true;
    vehicle.update(input, 10.0f);

    expect(vehicle.speed() <= vehicle.maxForwardSpeed(), "vehicle speed is clamped");
    expect(vehicle.position().z > 0.0f, "vehicle moves along forward axis");

    input.accelerate = false;
    input.brake = true;
    vehicle.update(input, 1.0f);
    expect(vehicle.speed() < vehicle.maxForwardSpeed(), "braking reduces speed");
}

void vehicleHandbrakeSharpensTurningWithoutBreakingSpeedClamp() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState gas;
    gas.accelerate = true;
    for (int i = 0; i < 80; ++i) {
        vehicle.update(gas, 1.0f / 60.0f);
    }

    bs3d::InputState normalInput;
    normalInput.accelerate = true;
    normalInput.steerLeft = true;
    vehicle.update(normalInput, 0.35f);
    const float normalYaw = vehicle.yawRadians();
    const float normalSpeed = vehicle.speed();

    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    for (int i = 0; i < 80; ++i) {
        vehicle.update(gas, 1.0f / 60.0f);
    }
    bs3d::InputState handbrakeInput = normalInput;
    handbrakeInput.handbrake = true;
    vehicle.update(handbrakeInput, 0.35f);

    expect(std::fabs(vehicle.yawRadians()) > std::fabs(normalYaw), "handbrake sharpens left steering");
    expect(vehicle.speed() <= vehicle.maxForwardSpeed(), "handbrake still respects max speed");
    expect(vehicle.speed() < normalSpeed, "handbrake bleeds speed");
    expect(vehicle.speedRatio() >= 0.0f && vehicle.speedRatio() <= 1.0f, "speed ratio is normalized");
}

void vehicleSteersLeftAndRightWithExpectedYawSigns() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState left;
    left.accelerate = true;
    left.steerLeft = true;
    vehicle.update(left, 0.5f);
    expect(vehicle.yawRadians() > 0.0f, "A steers left from forward heading");

    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    bs3d::InputState right;
    right.accelerate = true;
    right.steerRight = true;
    vehicle.update(right, 0.5f);
    expect(vehicle.yawRadians() < 0.0f, "D steers right from forward heading");
}

void vehicleLowSpeedSteeringHasReadableAuthority() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState input;
    input.accelerate = true;
    input.steerLeft = true;
    vehicle.update(input, 1.0f / 30.0f);

    expect(vehicle.runtimeState().steeringAuthority >= vehicle.config().minimumSteeringAuthority - 0.001f,
           "parking launch uses minimum steering authority");
    expect(vehicle.yawRadians() > 0.006f, "W+A still starts a readable left turn");
    expect(vehicle.yawRadians() < 0.030f, "W+A no longer snaps the gruz into a toy-like turn");
}

void vehicleFrontWheelsSteerAtStandstillWithoutRotatingBody() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState steer;
    steer.steerLeft = true;
    for (int i = 0; i < 10; ++i) {
        vehicle.update(steer, 1.0f / 60.0f);
    }

    expect(vehicle.yawRadians() == 0.0f, "A/D at standstill does not rotate the whole car body");
    expect(vehicle.runtimeState().frontWheelSteerRadians > 0.08f,
           "front wheels visibly steer left while parked");

    steer.steerLeft = false;
    steer.steerRight = true;
    for (int i = 0; i < 20; ++i) {
        vehicle.update(steer, 1.0f / 60.0f);
    }

    expect(vehicle.runtimeState().frontWheelSteerRadians < -0.08f,
           "front wheels visibly steer right while parked");
}

void vehicleUrbanSpeedTurnRadiusFeelsLikePassengerCar() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState gas;
    gas.accelerate = true;
    for (int i = 0; i < 60; ++i) {
        vehicle.update(gas, 1.0f / 60.0f);
    }

    bs3d::InputState turn;
    turn.accelerate = true;
    turn.steerLeft = true;
    for (int i = 0; i < 60; ++i) {
        vehicle.update(turn, 1.0f / 60.0f);
    }

    expect(vehicle.yawRadians() > 1.15f,
           "urban-speed W+A turn has passenger-car radius, not truck radius; yaw=" +
               std::to_string(vehicle.yawRadians()));
    expect(vehicle.yawRadians() < 2.25f,
           "urban-speed W+A turn remains controlled and not toy-snappy; yaw=" +
               std::to_string(vehicle.yawRadians()));
}

void vehicleAccelerationBuildsLikeHeavierGruz() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState gas;
    gas.accelerate = true;
    for (int i = 0; i < 120; ++i) {
        vehicle.update(gas, 1.0f / 60.0f);
    }
    const float speedAfterTwoSeconds = vehicle.speed();

    expect(speedAfterTwoSeconds < vehicle.maxForwardSpeed() * 0.64f,
           "gruz does not hit near-top-speed after only two seconds of gas");

    for (int i = 0; i < 240; ++i) {
        vehicle.update(gas, 1.0f / 60.0f);
    }

    expect(vehicle.speed() > speedAfterTwoSeconds + 1.5f, "gruz keeps building speed after the launch phase");
    expect(vehicle.speed() <= vehicle.maxForwardSpeed() + 0.01f, "heavier acceleration still respects speed clamp");
}

void vehicleSteeringRampsInBeforeFullLock() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState input;
    input.accelerate = true;
    input.steerLeft = true;
    vehicle.update(input, 1.0f / 60.0f);
    const float firstFrameYaw = std::fabs(vehicle.yawRadians());

    expect(firstFrameYaw < 0.014f, "first steering frame is damped instead of snapping");

    for (int i = 0; i < 36; ++i) {
        vehicle.update(input, 1.0f / 60.0f);
    }

    expect(std::fabs(vehicle.yawRadians()) > firstFrameYaw * 5.0f, "steering ramps into a committed turn");
    expect(std::fabs(vehicle.runtimeState().steeringInputSmoothed) > 0.70f,
           "vehicle exposes smoothed steering reaching full lock");
}

void vehicleFakeGearboxCreatesReadableShifts() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState gas;
    gas.accelerate = true;
    bool sawShiftTimer = false;
    int highestGear = 1;
    for (int i = 0; i < 360; ++i) {
        vehicle.update(gas, 1.0f / 60.0f);
        sawShiftTimer = sawShiftTimer || vehicle.runtimeState().shiftTimerSeconds > 0.0f;
        highestGear = std::max(highestGear, vehicle.runtimeState().gear);
        expect(vehicle.runtimeState().rpmNormalized >= 0.0f && vehicle.runtimeState().rpmNormalized <= 1.0f,
               "fake rpm stays normalized");
    }

    expect(highestGear >= 3, "automatic fake gearbox shifts up under sustained acceleration");
    expect(sawShiftTimer, "gear changes expose a short shift state for feel/audio");
}

void vehicleSurfaceResponseMakesGrassSlowerAndLooser() {
    bs3d::VehicleSurfaceResponse asphalt;
    bs3d::VehicleSurfaceResponse grass;
    grass.gripMultiplier = 0.58f;
    grass.dragMultiplier = 1.55f;
    grass.accelerationMultiplier = 0.82f;

    bs3d::VehicleController asphaltVehicle;
    asphaltVehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    bs3d::VehicleController grassVehicle;
    grassVehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState input;
    input.accelerate = true;
    input.steerLeft = true;
    for (int i = 0; i < 150; ++i) {
        const bs3d::VehicleMoveProposal asphaltMove = asphaltVehicle.previewMove(input, 1.0f / 60.0f, asphalt);
        bs3d::VehicleCollisionResult asphaltClear;
        asphaltClear.position = asphaltMove.desiredPosition;
        asphaltVehicle.applyMoveResolution(asphaltMove, asphaltClear);

        const bs3d::VehicleMoveProposal grassMove = grassVehicle.previewMove(input, 1.0f / 60.0f, grass);
        bs3d::VehicleCollisionResult grassClear;
        grassClear.position = grassMove.desiredPosition;
        grassVehicle.applyMoveResolution(grassMove, grassClear);
    }

    expect(grassVehicle.speed() < asphaltVehicle.speed(), "grass surface reduces usable acceleration");
    expect(std::fabs(grassVehicle.lateralSlip()) > std::fabs(asphaltVehicle.lateralSlip()),
           "grass surface feels looser under steering asphalt=" +
               std::to_string(std::fabs(asphaltVehicle.lateralSlip())) +
               " grass=" + std::to_string(std::fabs(grassVehicle.lateralSlip())));
}

void vehicleForwardDiagonalMovesInPressedDirection() {
    bs3d::VehicleController left;
    left.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    bs3d::InputState input;
    input.accelerate = true;
    input.steerLeft = true;
    left.update(input, 0.5f);
    expect(left.position().x > 0.0f, "W+A moves gruz left while going forward");
    expect(left.position().z > 0.0f, "W+A moves gruz forward");

    bs3d::VehicleController right;
    right.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    input.steerLeft = false;
    input.steerRight = true;
    right.update(input, 0.5f);
    expect(right.position().x < 0.0f, "W+D moves gruz right while going forward");
    expect(right.position().z > 0.0f, "W+D moves gruz forward");
}

void vehicleForwardMatchesVisibleFrontConvention() {
    bs3d::VehicleController vehicle;
    vehicle.reset({3.0f, 0.0f, -2.0f}, 0.0f);

    bs3d::InputState gas;
    gas.accelerate = true;
    vehicle.update(gas, 0.25f);

    expect(vehicle.position().z > -2.0f, "W moves gruz along +forwardFromYaw, matching visible front");
}

void vehiclePreviewMoveDoesNotMutateUntilResolutionIsApplied() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState gas;
    gas.accelerate = true;

    const bs3d::VehicleMoveProposal proposal = vehicle.previewMove(gas, 0.5f);

    expectNear(vehicle.position().z, 0.0f, 0.001f, "previewMove does not move the live vehicle");
    expectNear(vehicle.speed(), 0.0f, 0.001f, "previewMove does not mutate live speed");
    expect(proposal.desiredPosition.z > 0.0f, "previewMove still proposes forward travel");
    expect(proposal.proposedState.speed > 0.0f, "previewMove computes the future speed");

    bs3d::VehicleCollisionResult freeMove;
    freeMove.position = proposal.desiredPosition;
    vehicle.applyMoveResolution(proposal, freeMove);

    expect(vehicle.position().z > 0.0f, "applyMoveResolution commits the resolved position");
    expect(vehicle.speed() > 0.0f, "applyMoveResolution commits the proposed speed");
}

void vehicleApplyMoveResolutionAppliesImpactOnlyForRealHits() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState gas;
    gas.accelerate = true;
    const bs3d::VehicleMoveProposal proposal = vehicle.previewMove(gas, 1.0f);

    bs3d::VehicleCollisionResult hit;
    hit.hit = true;
    hit.position = proposal.previousPosition + bs3d::Vec3{0.0f, 0.0f, 0.25f};
    hit.impactSpeed = 9.0f;

    vehicle.applyMoveResolution(proposal, hit);

    expectNear(vehicle.position().z, hit.position.z, 0.001f, "collision resolution uses resolved hit position");
    expect(vehicle.condition() < 100.0f, "real vehicle hit damages condition");
    expect(vehicle.speed() < proposal.proposedState.speed, "real vehicle hit bleeds speed");

    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    const bs3d::VehicleMoveProposal clearProposal = vehicle.previewMove(gas, 0.25f);
    bs3d::VehicleCollisionResult clear;
    clear.position = clearProposal.desiredPosition;
    vehicle.applyMoveResolution(clearProposal, clear);

    expectNear(vehicle.condition(), 100.0f, 0.001f, "clear vehicle move does not fake damage");
}

void vehicleReverseSteeringKeepsArcadeScreenDirection() {
    bs3d::VehicleController leftReverse;
    leftReverse.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    bs3d::InputState reverse;
    reverse.brake = true;
    leftReverse.update(reverse, 0.5f);
    reverse.steerLeft = true;
    leftReverse.update(reverse, 0.4f);
    expect(leftReverse.speed() < 0.0f, "vehicle is reversing before reverse steering check");
    expect(leftReverse.position().x > 0.0f, "A+S moves gruz left while reversing");
    expect(leftReverse.position().z < 0.0f, "A+S moves gruz backward");

    bs3d::VehicleController rightReverse;
    rightReverse.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    reverse.steerLeft = false;
    reverse.steerRight = false;
    rightReverse.update(reverse, 0.5f);
    reverse.steerRight = true;
    rightReverse.update(reverse, 0.4f);
    expect(rightReverse.position().x < 0.0f, "D+S moves gruz right while reversing");
    expect(rightReverse.position().z < 0.0f, "D+S moves gruz backward");
}

void driveRouteGuideAdvancesThroughParkingRoute() {
    bs3d::DriveRouteGuide guide;
    guide.reset({
        {{0.0f, 0.0f, 5.0f}, 2.0f, "wyjazd"},
        {{8.0f, 0.0f, -6.0f}, 2.0f, "droga"},
        {{18.0f, 0.0f, -18.0f}, 3.0f, "sklep"}
    });

    expect(guide.activePoint() != nullptr, "route guide exposes first target");
    expect(guide.activeIndex() == 0, "route starts at parking exit");
    expect(guide.activePoint()->label == "wyjazd", "route exposes active label");

    guide.update({0.5f, 0.0f, 5.2f});
    expect(guide.activeIndex() == 1, "route advances after first point");

    guide.update({8.2f, 0.0f, -5.8f});
    expect(guide.activeIndex() == 2, "route advances to final shop point");

    guide.update({18.0f, 0.0f, -18.0f});
    expect(guide.isComplete(), "route completes at final point");
    expect(guide.activePoint() == nullptr, "completed route has no active target");
}

void driveRouteGuideCanAimVehicleSpawnTowardFirstPoint() {
    bs3d::DriveRouteGuide guide;
    const bs3d::Vec3 vehicleStart{-7.0f, 0.0f, 11.5f};
    guide.reset({{{-1.0f, 0.0f, 7.0f}, 2.0f, "wyjazd"}});

    const float yaw = guide.yawFrom(vehicleStart);
    const bs3d::Vec3 forward = bs3d::forwardFromYaw(yaw);

    expect(forward.x > 0.0f, "route yaw points gruz out of parking toward the right side of the lot");
    expect(forward.z < 0.0f, "route yaw points gruz toward the shop side instead of away from it");
}

void vehicleBrakesBeforeEnteringReverse() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState accelerate;
    accelerate.accelerate = true;
    vehicle.update(accelerate, 0.2f);
    expect(vehicle.speed() > 0.0f, "vehicle has forward speed before braking");

    bs3d::InputState brake;
    brake.brake = true;
    vehicle.update(brake, 0.25f);
    expect(vehicle.speed() >= 0.0f, "S brakes to zero before reverse");

    vehicle.update(brake, 0.4f);
    expect(vehicle.speed() < 0.0f, "holding S after stopping enters reverse");
}

void vehicleBoostIncreasesAccelerationButKeepsSpeedClamped() {
    bs3d::VehicleController normal;
    normal.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    bs3d::InputState input;
    input.accelerate = true;
    normal.update(input, 0.5f);
    const float normalSpeed = normal.speed();

    bs3d::VehicleController boosted;
    boosted.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    input.vehicleBoost = true;
    boosted.update(input, 0.5f);
    expect(boosted.speed() > normalSpeed, "desperacki gaz gives gruz a stronger launch");
    expect(boosted.boostActive(), "vehicle exposes active boost state");

    boosted.update(input, 10.0f);
    expect(boosted.speed() <= boosted.maxForwardSpeed() + 0.01f, "boost still respects forward speed clamp");
}

void vehicleSteersMoreAtParkingSpeedThanHighSpeed() {
    bs3d::VehicleController lowSpeed;
    lowSpeed.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    bs3d::InputState input;
    input.accelerate = true;
    lowSpeed.update(input, 0.12f);
    input.accelerate = false;
    input.steerLeft = true;
    lowSpeed.update(input, 0.45f);
    const float lowYaw = std::fabs(lowSpeed.yawRadians());

    bs3d::VehicleController highSpeed;
    highSpeed.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    bs3d::InputState accelerate;
    accelerate.accelerate = true;
    for (int i = 0; i < 80; ++i) {
        highSpeed.update(accelerate, 0.033f);
    }
    bs3d::InputState steer;
    steer.steerLeft = true;
    highSpeed.update(steer, 0.45f);
    const float highYawDelta = std::fabs(highSpeed.yawRadians());

    expect(lowYaw > highYawDelta,
           "parking-speed steering is stronger than high-speed steering low=" +
               std::to_string(lowYaw) + " high=" + std::to_string(highYawDelta));
}

void vehicleHandbrakeAddsSlipAndInstabilityWithoutBreakingClamp() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState input;
    input.accelerate = true;
    vehicle.update(input, 1.0f);

    input.accelerate = false;
    input.steerLeft = true;
    input.handbrake = true;
    vehicle.update(input, 0.35f);

    expect(std::fabs(vehicle.lateralSlip()) > 0.05f, "handbrake exposes lateral slip");
    expect(vehicle.instability() > 0.0f, "handbrake raises gruz instability");
    expect(vehicle.speed() <= vehicle.maxForwardSpeed() + 0.01f, "handbrake keeps speed clamped");
}

void feelGateVehicleHandbrakeSlipRecoversAfterRelease() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState accelerate;
    accelerate.accelerate = true;
    for (int i = 0; i < 30; ++i) {
        vehicle.update(accelerate, 1.0f / 30.0f);
    }

    bs3d::InputState handbrake;
    handbrake.steerLeft = true;
    handbrake.handbrake = true;
    vehicle.update(handbrake, 0.35f);
    const float slipDuringHandbrake = std::fabs(vehicle.lateralSlip());
    expect(slipDuringHandbrake > 0.05f, "feel gate setup creates readable handbrake slip");

    bs3d::InputState recover;
    recover.accelerate = true;
    for (int i = 0; i < 30; ++i) {
        vehicle.update(recover, 1.0f / 30.0f);
    }

    expect(std::fabs(vehicle.lateralSlip()) < slipDuringHandbrake * 0.35f,
           "feel gate handbrake slip recovers after release");
    expect(vehicle.instability() < 0.12f, "feel gate handbrake instability cools down quickly");
    expect(vehicle.speedRatio() >= 0.0f && vehicle.speedRatio() <= 1.0f,
           "feel gate handbrake recovery keeps speed ratio valid");
}

void vehicleHandbrakeDriftEntersControlledSlideWithoutKillingSpeed() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState accelerate;
    accelerate.accelerate = true;
    for (int i = 0; i < 70; ++i) {
        vehicle.update(accelerate, 1.0f / 60.0f);
    }
    const float speedBeforeDrift = vehicle.speed();

    bs3d::InputState drift;
    drift.accelerate = true;
    drift.steerRight = true;
    drift.handbrake = true;
    for (int i = 0; i < 18; ++i) {
        vehicle.update(drift, 1.0f / 60.0f);
    }

    expect(vehicle.runtimeState().driftActive, "handbrake + steer enters a readable drift state");
    expect(vehicle.runtimeState().driftAmount > 0.20f, "drift amount becomes visible quickly");
    expect(std::fabs(vehicle.runtimeState().driftAngleRadians) > 0.12f,
           "drift exposes a readable body slip angle");
    expect(std::fabs(vehicle.lateralSlip()) > 0.35f, "drift creates lateral slide");
    expect(vehicle.speed() > speedBeforeDrift * 0.55f, "controlled drift does not kill all forward speed");
    expect(vehicle.runtimeState().wheelSlipVisual > 0.30f, "drift is visible through wheel slip feedback");
}

void vehicleCounterSteerRecoversDriftWithoutSnapping() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState accelerate;
    accelerate.accelerate = true;
    for (int i = 0; i < 70; ++i) {
        vehicle.update(accelerate, 1.0f / 60.0f);
    }

    bs3d::InputState drift;
    drift.accelerate = true;
    drift.steerRight = true;
    drift.handbrake = true;
    for (int i = 0; i < 24; ++i) {
        vehicle.update(drift, 1.0f / 60.0f);
    }

    const float driftAngleBeforeCounter = std::fabs(vehicle.runtimeState().driftAngleRadians);
    const float slipBeforeCounter = std::fabs(vehicle.lateralSlip());
    const float speedBeforeCounter = vehicle.speed();

    bs3d::InputState counter;
    counter.accelerate = true;
    counter.steerLeft = true;
    for (int i = 0; i < 32; ++i) {
        vehicle.update(counter, 1.0f / 60.0f);
    }

    expect(std::fabs(vehicle.runtimeState().driftAngleRadians) < driftAngleBeforeCounter * 0.55f,
           "counter-steer brings drift angle back under control");
    expect(std::fabs(vehicle.lateralSlip()) < slipBeforeCounter * 0.70f,
           "counter-steer reduces lateral slide");
    expect(vehicle.speed() > speedBeforeCounter * 0.70f, "counter-steer recovery preserves useful speed");
}

void vehicleDriftFallsOutCleanlyAtLowSpeed() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState accelerate;
    accelerate.accelerate = true;
    for (int i = 0; i < 70; ++i) {
        vehicle.update(accelerate, 1.0f / 60.0f);
    }

    bs3d::InputState drift;
    drift.accelerate = true;
    drift.steerLeft = true;
    drift.handbrake = true;
    for (int i = 0; i < 24; ++i) {
        vehicle.update(drift, 1.0f / 60.0f);
    }
    expect(vehicle.runtimeState().driftActive, "drift setup enters drift state");

    bs3d::InputState brake;
    brake.brake = true;
    for (int i = 0; i < 100; ++i) {
        vehicle.update(brake, 1.0f / 60.0f);
    }

    expect(!vehicle.runtimeState().driftActive, "drift exits cleanly at low speed");
    expect(vehicle.runtimeState().driftAmount < 0.08f, "drift amount cools down near stop");
    expect(std::fabs(vehicle.lateralSlip()) < 0.08f, "low-speed drift recovery clears lateral slip");
}

void vehicleConditionDropsAndChangesBandAfterImpacts() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    expectNear(vehicle.condition(), 100.0f, 0.001f, "vehicle starts in good condition");
    expect(vehicle.conditionBand() == bs3d::VehicleConditionBand::Igla, "fresh gruz starts as Igla");

    vehicle.applyCollisionImpact(12.0f);
    expect(vehicle.condition() < 100.0f, "collision impact damages gruz condition");
    expect(vehicle.instability() > 0.0f, "collision impact creates vehicle bump instability");

    for (int i = 0; i < 8; ++i) {
        vehicle.applyCollisionImpact(18.0f);
    }
    expect(vehicle.conditionBand() != bs3d::VehicleConditionBand::Igla, "repeated impacts change condition band");
    expect(vehicle.condition() >= 0.0f, "condition never drops below zero");
}

void damagedVehicleStillDrivesButFeelsWeaker() {
    bs3d::VehicleController healthy;
    healthy.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    bs3d::InputState input;
    input.accelerate = true;
    healthy.update(input, 1.0f);

    bs3d::VehicleController damaged;
    damaged.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    for (int i = 0; i < 12; ++i) {
        damaged.applyCollisionImpact(20.0f);
    }
    damaged.update(input, 1.0f);

    expect(damaged.conditionBand() == bs3d::VehicleConditionBand::Zlom, "heavy impacts can reach Zlom band");
    expect(damaged.speed() > 0.0f, "damaged gruz still drives");
    expect(damaged.speed() < healthy.speed(), "damaged gruz accelerates slightly weaker");
}

void feelGateDamagedVehicleBoostAndSteeringRemainReadable() {
    bs3d::VehicleController damaged;
    damaged.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    for (int i = 0; i < 12; ++i) {
        damaged.applyCollisionImpact(18.0f);
    }
    expect(damaged.conditionBand() == bs3d::VehicleConditionBand::Zlom,
           "feel gate setup damages gruz into the worst condition band");

    bs3d::InputState input;
    input.accelerate = true;
    input.vehicleBoost = true;
    input.steerLeft = true;
    damaged.update(input, 0.5f);

    expect(damaged.boostActive(), "feel gate damaged gruz can still use desperate gas");
    expect(damaged.speed() > 1.0f, "feel gate damaged gruz still has enough launch to be controllable");
    expect(damaged.yawRadians() > 0.05f, "feel gate damaged gruz still responds to steering");
    expect(damaged.speedRatio() <= 1.0f, "feel gate damaged gruz keeps speed ratio clamped");
}

void vehicleHornPulsesWithCooldownWithoutMovingCar() {
    bs3d::VehicleController vehicle;
    vehicle.reset({3.0f, 0.0f, 4.0f}, 0.7f);

    bs3d::InputState horn;
    horn.hornPressed = true;
    vehicle.update(horn, 0.016f);

    expect(vehicle.hornPulse() > 0.0f, "horn creates a visible/audio pulse");
    expect(vehicle.hornCooldown() > 0.0f, "horn starts cooldown");
    expectNear(vehicle.position().x, 3.0f, 0.001f, "horn does not move vehicle x");
    expectNear(vehicle.position().z, 4.0f, 0.001f, "horn does not move vehicle z");

    const float firstPulse = vehicle.hornPulse();
    vehicle.update(horn, 0.016f);
    expect(vehicle.hornPulse() <= firstPulse, "cooldown prevents horn pulse spam");
}

void vehicleDoesNotPivotInPlaceWithoutSpeed() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.35f);

    bs3d::InputState steer;
    steer.steerLeft = true;
    vehicle.update(steer, 1.0f);

    expectNear(vehicle.yawRadians(), 0.35f, 0.001f, "parked gruz does not rotate like a debug pawn");
    expectNear(vehicle.position().x, 0.0f, 0.001f, "parked steering does not drift x");
    expectNear(vehicle.position().z, 0.0f, 0.001f, "parked steering does not drift z");
}

void vehicleBrakeWinsWhenGasAndBrakeAreHeld() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState gas;
    gas.accelerate = true;
    vehicle.update(gas, 0.6f);
    const float speedBeforeConflict = vehicle.speed();

    bs3d::InputState conflict;
    conflict.accelerate = true;
    conflict.brake = true;
    vehicle.update(conflict, 0.25f);

    expect(vehicle.speed() < speedBeforeConflict, "S remains brake even if W is also held");
    expect(vehicle.speed() >= 0.0f, "gas+brake conflict does not snap into reverse");
}

void vehicleWheelVisualsSpinFromSpeedAndReverseDirection() {
    bs3d::VehicleController vehicle;
    bs3d::InputState gas;
    gas.accelerate = true;

    for (int i = 0; i < 25; ++i) {
        vehicle.update(gas, 1.0f / 60.0f);
    }

    const float forwardSpin = vehicle.runtimeState().wheelSpinRadians;
    expect(std::fabs(forwardSpin) > 0.20f, "wheel spin advances when the gruz drives forward");

    bs3d::InputState brake;
    brake.brake = true;
    for (int i = 0; i < 90; ++i) {
        vehicle.update(brake, 1.0f / 60.0f);
    }

    expect(vehicle.speed() < -0.05f, "test setup reaches reverse speed");
    expect(vehicle.runtimeState().wheelSpinRadians < forwardSpin,
           "wheel spin reverses direction when the gruz backs up");
}

void vehicleWheelVisualsSteerFrontWheelsAndCenterWhenReleased() {
    bs3d::VehicleController vehicle;
    bs3d::InputState input;
    input.accelerate = true;
    input.steerLeft = true;

    for (int i = 0; i < 20; ++i) {
        vehicle.update(input, 1.0f / 60.0f);
    }

    expect(vehicle.runtimeState().frontWheelSteerRadians > 0.05f,
           "left steering visibly turns the front wheels left");

    input.steerLeft = false;
    for (int i = 0; i < 45; ++i) {
        vehicle.update(input, 1.0f / 60.0f);
    }

    expect(std::fabs(vehicle.runtimeState().frontWheelSteerRadians) < 0.08f,
           "front wheel visual steer recenters after steering input is released");
}

void vehicleWheelVisualsShowBrakeDiveAndSlipLoad() {
    bs3d::VehicleController vehicle;
    bs3d::InputState gas;
    gas.accelerate = true;

    for (int i = 0; i < 40; ++i) {
        vehicle.update(gas, 1.0f / 60.0f);
    }

    const float cruisingCompression = vehicle.runtimeState().suspensionCompression;

    bs3d::InputState brake;
    brake.brake = true;
    for (int i = 0; i < 12; ++i) {
        vehicle.update(brake, 1.0f / 60.0f);
    }

    expect(vehicle.runtimeState().suspensionCompression > cruisingCompression + 0.01f,
           "braking adds readable suspension compression cruise=" +
               std::to_string(cruisingCompression) + " brake=" +
               std::to_string(vehicle.runtimeState().suspensionCompression));

    bs3d::InputState slide;
    slide.accelerate = true;
    slide.steerRight = true;
    slide.handbrake = true;
    for (int i = 0; i < 24; ++i) {
        vehicle.update(slide, 1.0f / 60.0f);
    }

    expect(vehicle.runtimeState().wheelSlipVisual > 0.15f,
           "handbrake slide exposes a wheel slip visual value for renderer/skid feedback");
}

void vehicleYawStaysBoundedDuringLongArcadeSteering() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState input;
    input.accelerate = true;
    input.steerRight = true;
    for (int i = 0; i < 480; ++i) {
        vehicle.update(input, 0.033f);
    }

    expect(vehicle.yawRadians() <= bs3d::Pi && vehicle.yawRadians() >= -bs3d::Pi,
           "vehicle yaw is wrapped to a stable range for camera/math");
}

void playerWalksAndRespectsCollision() {
    bs3d::WorldCollision collision;
    bs3d::PlayerController player;
    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState input;
    input.moveForward = true;
    player.update(input, collision, 1.0f);

    expect(player.position().z > 0.0f, "player moves forward on foot");
    expectNear(player.yawRadians(), 0.0f, 0.001f, "forward movement keeps zero yaw");

    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    collision.addBox({0.0f, 0.0f, 4.0f}, {4.0f, 3.0f, 4.0f});
    player.update(input, collision, 1.0f);
    expect(player.position().z <= 1.6f, "player does not walk through block collision");
}

void playerAcceleratesAndDeceleratesSmoothly() {
    bs3d::WorldCollision collision;
    bs3d::PlayerController player;
    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState input;
    input.moveForward = true;
    player.update(input, collision, 0.1f);
    const float earlySpeed = bs3d::distanceXZ(player.velocity(), {});
    expect(earlySpeed > 0.0f, "player begins accelerating");
    expect(earlySpeed < player.jogSpeed(), "player does not snap to full speed");

    player.update(input, collision, 1.0f);
    expect(bs3d::distanceXZ(player.velocity(), {}) <= player.jogSpeed() + 0.01f, "player speed is clamped");

    input.moveForward = false;
    player.update(input, collision, 0.1f);
    expect(bs3d::distanceXZ(player.velocity(), {}) > 0.0f, "player decelerates instead of stopping instantly");
}

void playerControllerExposesDistinctOnFootSpeedCaps() {
    bs3d::PlayerController player;

    expect(player.walkSpeed() > 0.0f, "walk speed is exposed");
    expect(player.jogSpeed() > player.walkSpeed(), "jog speed is faster than walk speed");
    expect(player.sprintSpeed() > player.jogSpeed(), "sprint speed is faster than jog speed");
}

void playerMotorUsesWalkJogAndSprintSpeedStates() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::PlayerMotor motor;
    bs3d::PlayerInputIntent intent;
    intent.moveDirection = {0.0f, 0.0f, 1.0f};
    intent.walk = true;

    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    motor.update(intent, collision, 1.0f);
    const float walkSpeed = bs3d::distanceXZ(motor.state().velocity, {});
    expect(motor.state().speedState == bs3d::PlayerSpeedState::Walk, "walk modifier uses walk speed state");
    expect(walkSpeed <= motor.config().walkSpeed + 0.01f, "walk modifier clamps to walk speed");

    intent.walk = false;
    intent.sprint = false;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    motor.update(intent, collision, 1.0f);
    const float jogSpeed = bs3d::distanceXZ(motor.state().velocity, {});
    expect(motor.state().speedState == bs3d::PlayerSpeedState::Jog, "default WASD uses jog state");
    expect(jogSpeed > walkSpeed + 0.75f, "default jog is clearly faster than walk");
    expect(jogSpeed <= motor.config().jogSpeed + 0.01f, "default jog clamps to jog speed");

    intent.sprint = true;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    motor.update(intent, collision, 1.0f);
    expect(motor.state().speedState == bs3d::PlayerSpeedState::Sprint, "shift uses sprint state");
    expect(bs3d::distanceXZ(motor.state().velocity, {}) > jogSpeed + 1.0f, "sprint is clearly faster than jog");
}

void playerMotorQuickTurnsWithoutLongForwardSlide() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent intent;
    intent.moveDirection = {0.0f, 0.0f, 1.0f};
    motor.update(intent, collision, 1.0f);
    expect(motor.state().velocity.z > motor.config().jogSpeed - 0.1f, "motor reaches forward jog before quick turn");

    intent.moveDirection = {0.0f, 0.0f, -1.0f};
    motor.update(intent, collision, 0.1f);
    expect(motor.state().velocity.z <= 0.1f, "quick reverse cancels forward slide in one short beat");
    expect(std::fabs(motor.state().facingYawRadians) > 0.6f, "quick reverse starts turning character immediately");
}

void feelGatePlayerStopsWithinShortReadableDistanceAfterSprint() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent sprint;
    sprint.moveDirection = {0.0f, 0.0f, 1.0f};
    sprint.sprint = true;
    motor.update(sprint, collision, 1.0f);
    expect(bs3d::distanceXZ(motor.state().velocity, {}) > motor.config().jogSpeed,
           "feel gate setup reaches sprint speed before stop test");

    const bs3d::Vec3 releasePosition = motor.state().position;
    bs3d::PlayerInputIntent released;
    for (int i = 0; i < 8; ++i) {
        motor.update(released, collision, 1.0f / 30.0f);
    }

    expect(bs3d::distanceXZ(motor.state().position, releasePosition) < 0.85f,
           "feel gate sprint release stops within a short readable distance");
    expect(bs3d::distanceXZ(motor.state().velocity, {}) < 0.25f,
           "feel gate sprint release reaches near-idle speed quickly");
}

void feelGatePlayerSlidesAlongWallWithoutLongStaggerLock() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({1.10f, 1.0f, 3.0f}, {0.35f, 2.0f, 8.0f});

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent diagonal;
    diagonal.moveDirection = bs3d::normalizeXZ({1.0f, 0.0f, 1.0f});
    diagonal.sprint = true;
    for (int i = 0; i < 14; ++i) {
        motor.update(diagonal, collision, 1.0f / 30.0f);
    }

    expect(motor.state().hitWallThisFrame, "feel gate setup hits the wall while moving diagonally");
    expect(motor.state().position.z > 1.0f, "feel gate wall contact still allows forward slide");

    bs3d::PlayerInputIntent forward;
    forward.moveDirection = {0.0f, 0.0f, 1.0f};
    for (int i = 0; i < 18; ++i) {
        motor.update(forward, collision, 1.0f / 30.0f);
    }

    expect(motor.state().staggerSeconds <= 0.001f, "feel gate corner bump does not leave a long control lock");
    expect(motor.state().position.z > 1.7f, "feel gate player can keep moving after wall contact");
}

void playerMovesRelativeToCameraYaw() {
    bs3d::WorldCollision collision;
    bs3d::PlayerController player;
    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState input;
    input.moveForward = true;
    input.cameraYawRadians = bs3d::Pi * 0.5f;
    player.update(input, collision, 0.5f);

    expect(player.position().x > 0.1f, "forward input follows camera yaw on x axis");
    expect(std::fabs(player.position().z) < 0.2f, "camera-relative forward does not use world z");
}

void playerStrafesRelativeToCameraYaw() {
    bs3d::WorldCollision collision;
    bs3d::PlayerController player;
    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState input;
    input.moveLeft = true;
    input.cameraYawRadians = bs3d::Pi * 0.5f;
    player.update(input, collision, 0.5f);

    expect(player.position().z < -0.1f, "A moves screen-left at 90 degree camera yaw");
    expect(std::fabs(player.position().x) < 0.2f, "strafe does not leak into camera forward");

    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    input.moveLeft = false;
    input.moveRight = true;
    player.update(input, collision, 0.5f);
    expect(player.position().z > 0.1f, "D moves screen-right at 90 degree camera yaw");
}

void playerInputMapsWasdConsistentlyAcrossCardinalCameraYaws() {
    struct Case {
        float yaw;
        bs3d::Vec3 forward;
        bs3d::Vec3 left;
        bs3d::Vec3 right;
    };

    const Case cases[] = {
        {0.0f, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {bs3d::Pi * 0.5f, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
        {bs3d::Pi, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {-bs3d::Pi * 0.5f, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
    };

    for (const Case& testCase : cases) {
        bs3d::InputState input;
        input.cameraYawRadians = testCase.yaw;

        input.moveForward = true;
        bs3d::PlayerInputIntent intent = bs3d::PlayerInputIntent::fromInputState(input);
        expectNear(intent.moveDirection.x, testCase.forward.x, 0.001f, "W x follows camera forward");
        expectNear(intent.moveDirection.z, testCase.forward.z, 0.001f, "W z follows camera forward");

        input.moveForward = false;
        input.moveLeft = true;
        intent = bs3d::PlayerInputIntent::fromInputState(input);
        expectNear(intent.moveDirection.x, testCase.left.x, 0.001f, "A x remains screen-left");
        expectNear(intent.moveDirection.z, testCase.left.z, 0.001f, "A z remains screen-left");

        input.moveLeft = false;
        input.moveRight = true;
        intent = bs3d::PlayerInputIntent::fromInputState(input);
        expectNear(intent.moveDirection.x, testCase.right.x, 0.001f, "D x remains screen-right");
        expectNear(intent.moveDirection.z, testCase.right.z, 0.001f, "D z remains screen-right");
    }
}

void playerInputMapsDToRaylibScreenRight() {
    struct Case {
        float yaw;
        bs3d::Vec3 screenRight;
    };

    const Case cases[] = {
        {0.0f, {-1.0f, 0.0f, 0.0f}},
        {bs3d::Pi * 0.5f, {0.0f, 0.0f, 1.0f}},
        {bs3d::Pi, {1.0f, 0.0f, 0.0f}},
        {-bs3d::Pi * 0.5f, {0.0f, 0.0f, -1.0f}},
    };

    for (const Case& testCase : cases) {
        bs3d::InputState input;
        input.cameraYawRadians = testCase.yaw;
        input.moveRight = true;

        const bs3d::PlayerInputIntent intent = bs3d::PlayerInputIntent::fromInputState(input);

        expectNear(intent.moveDirection.x,
                   testCase.screenRight.x,
                   0.001f,
                   "D maps to raylib screen-right x, not raw +X");
        expectNear(intent.moveDirection.z,
                   testCase.screenRight.z,
                   0.001f,
                   "D maps to raylib screen-right z, not raw +X");
    }
}

void screenRightFromYawMatchesRaylibViewConvention() {
    const bs3d::Vec3 defaultRight = bs3d::screenRightFromYaw(0.0f);
    expectNear(defaultRight.x, -1.0f, 0.001f, "screen-right at yaw 0 points to raylib screen-right x");
    expectNear(defaultRight.z, 0.0f, 0.001f, "screen-right at yaw 0 has no forward component");

    const bs3d::Vec3 turnedRight = bs3d::screenRightFromYaw(bs3d::Pi * 0.5f);
    expectNear(turnedRight.x, 0.0f, 0.001f, "screen-right at yaw 90 has no x component");
    expectNear(turnedRight.z, 1.0f, 0.001f, "screen-right at yaw 90 points toward positive z");
}

void vehicleInputMapsDToRaylibScreenRightAtDefaultCamera() {
    bs3d::VehicleController vehicle;
    vehicle.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState input;
    input.accelerate = true;
    input.steerRight = true;
    vehicle.update(input, 0.5f);

    expect(vehicle.position().x < 0.0f, "W+D moves the gruz toward raylib screen-right at yaw 0");
    expect(vehicle.position().z > 0.0f, "W+D still moves the gruz forward");
}

void playerInputMapsDiagonalWasdConsistentlyAcrossCardinalCameraYaws() {
    struct Case {
        float yaw;
        bs3d::Vec3 forwardLeft;
        bs3d::Vec3 forwardRight;
        bs3d::Vec3 backLeft;
        bs3d::Vec3 backRight;
    };

    const float d = 0.70710678f;
    const Case cases[] = {
        {0.0f, {d, 0.0f, d}, {-d, 0.0f, d}, {d, 0.0f, -d}, {-d, 0.0f, -d}},
        {bs3d::Pi * 0.5f, {d, 0.0f, -d}, {d, 0.0f, d}, {-d, 0.0f, -d}, {-d, 0.0f, d}},
        {bs3d::Pi, {-d, 0.0f, -d}, {d, 0.0f, -d}, {-d, 0.0f, d}, {d, 0.0f, d}},
        {-bs3d::Pi * 0.5f, {-d, 0.0f, d}, {-d, 0.0f, -d}, {d, 0.0f, d}, {d, 0.0f, -d}},
    };

    for (const Case& testCase : cases) {
        bs3d::InputState input;
        input.cameraYawRadians = testCase.yaw;

        input.moveForward = true;
        input.moveLeft = true;
        bs3d::PlayerInputIntent intent = bs3d::PlayerInputIntent::fromInputState(input);
        expectNear(intent.moveDirection.x, testCase.forwardLeft.x, 0.001f, "W+A x remains camera-relative diagonal");
        expectNear(intent.moveDirection.z, testCase.forwardLeft.z, 0.001f, "W+A z remains camera-relative diagonal");
        expectNear(bs3d::distanceXZ(intent.moveDirection, {}), 1.0f, 0.001f, "W+A diagonal is normalized");

        input.moveLeft = false;
        input.moveRight = true;
        intent = bs3d::PlayerInputIntent::fromInputState(input);
        expectNear(intent.moveDirection.x, testCase.forwardRight.x, 0.001f, "W+D x remains camera-relative diagonal");
        expectNear(intent.moveDirection.z, testCase.forwardRight.z, 0.001f, "W+D z remains camera-relative diagonal");

        input.moveForward = false;
        input.moveBackward = true;
        input.moveRight = false;
        input.moveLeft = true;
        intent = bs3d::PlayerInputIntent::fromInputState(input);
        expectNear(intent.moveDirection.x, testCase.backLeft.x, 0.001f, "S+A x remains camera-relative diagonal");
        expectNear(intent.moveDirection.z, testCase.backLeft.z, 0.001f, "S+A z remains camera-relative diagonal");

        input.moveLeft = false;
        input.moveRight = true;
        intent = bs3d::PlayerInputIntent::fromInputState(input);
        expectNear(intent.moveDirection.x, testCase.backRight.x, 0.001f, "S+D x remains camera-relative diagonal");
        expectNear(intent.moveDirection.z, testCase.backRight.z, 0.001f, "S+D z remains camera-relative diagonal");
    }
}

void cameraRigAppliesMouseLookBeforePlayerMovement() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::Walking;
    rig.reset(target);

    bs3d::CameraRigControlInput look;
    look.mode = bs3d::CameraRigMode::Walking;
    look.followYawRadians = 0.0f;
    look.mouseDeltaX = 100.0f;
    look.mouseLookActive = true;
    const float controlYaw = rig.beginFrame(look, 0.016f);

    bs3d::WorldCollision collision;
    bs3d::PlayerController player;
    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState input;
    input.moveForward = true;
    input.cameraYawRadians = controlYaw;
    player.update(input, collision, 0.5f);

    expect(controlYaw < -0.05f, "mouse look changes control yaw before late camera update");
    expect(player.position().x < -0.1f, "same-frame movement uses updated camera yaw");
}

void cameraRigMouseRightTurnsViewDirectionRight() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootNormal;
    rig.reset(target);

    bs3d::CameraRigControlInput look;
    look.mode = bs3d::CameraRigMode::OnFootNormal;
    look.followYawRadians = 0.0f;
    look.mouseDeltaX = 80.0f;
    look.mouseLookActive = true;
    const float yaw = rig.beginFrame(look, 0.016f);
    const bs3d::Vec3 viewForward = bs3d::forwardFromYaw(yaw);

    expect(viewForward.x < 0.0f, "mouse right turns the view direction to raylib screen-right");
    expect(yaw < 0.0f, "mouse right decreases yaw in this raylib camera convention");

    target.yawRadians = yaw;
    const bs3d::CameraRigState state = rig.update(target, 0.016f);
    expect(state.position.x > state.target.x, "orbit boom moves opposite the raylib screen-right view direction");
}

void cameraRigManualMouseLookKeepsVisualYawSyncedWithControlYaw() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootNormal;
    rig.reset(target);
    rig.update(target, 0.016f);

    bs3d::CameraRigControlInput look;
    look.mode = bs3d::CameraRigMode::OnFootNormal;
    look.followYawRadians = 0.0f;
    look.mouseDeltaX = 160.0f;
    look.mouseLookActive = true;
    const float controlYaw = rig.beginFrame(look, 0.016f);
    const bs3d::CameraRigState state = rig.update(target, 0.016f);

    expectNear(state.visualYawRadians, controlYaw, 0.035f,
               "manual mouse look keeps rendered camera direction synced with movement yaw");
    expectNear(std::atan2(std::sin(state.visualYawRadians - state.cameraYawRadians),
                          std::cos(state.visualYawRadians - state.cameraYawRadians)),
               0.0f,
               0.035f,
               "debug visual yaw exposes no large yaw desync during manual look");
}

void cameraRigCanInvertMouseXForOrbitPreference() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.mode = bs3d::CameraRigMode::OnFootNormal;
    rig.reset(target);

    bs3d::CameraRigControlInput look;
    look.mode = bs3d::CameraRigMode::OnFootNormal;
    look.mouseDeltaX = 80.0f;
    look.mouseLookActive = true;
    look.invertMouseX = true;

    const float yaw = rig.beginFrame(look, 0.016f);
    expect(yaw > 0.0f, "invert mouse X lets mouse right orbit camera the opposite way");
}

void cameraRigMouseSensitivityIsControlledForDesktopLook() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootNormal;
    rig.reset(target);

    bs3d::CameraRigControlInput look;
    look.mode = bs3d::CameraRigMode::OnFootNormal;
    look.followYawRadians = 0.0f;
    look.mouseDeltaX = 100.0f;
    look.mouseLookActive = true;
    const float yaw = rig.beginFrame(look, 0.016f);

    expect(std::fabs(yaw) >= 0.20f, "mouse look remains responsive enough");
    expect(std::fabs(yaw) <= 0.27f, "mouse look is not twitchy for desktop camera control");
}

void playerSprintsJumpsAndLands() {
    bs3d::WorldCollision collision;
    bs3d::PlayerController player;
    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::InputState walking;
    walking.moveForward = true;
    player.update(walking, collision, 1.0f);
    const float walkingSpeed = bs3d::distanceXZ(player.velocity(), {});

    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    bs3d::InputState sprinting = walking;
    sprinting.sprint = true;
    player.update(sprinting, collision, 1.0f);
    expect(bs3d::distanceXZ(player.velocity(), {}) > walkingSpeed, "sprint raises movement speed cap");

    bs3d::InputState jumpInput;
    jumpInput.jumpPressed = true;
    player.update(jumpInput, collision, 0.016f);
    expect(!player.isGrounded(), "jump leaves ground");
    expect(player.position().y > 0.0f, "jump raises player");

    for (int i = 0; i < 180; ++i) {
        player.update(bs3d::InputState{}, collision, 0.016f);
    }
    expect(player.isGrounded(), "player lands after falling");
    expectNear(player.position().y, 0.0f, 0.001f, "landing returns to ground height");
}

void playerMotorMovesFromIntentAndReportsSpeedStates() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent intent;
    intent.moveDirection = {0.0f, 0.0f, 1.0f};
    motor.update(intent, collision, 0.1f);

    expect(motor.state().position.z > 0.0f, "motor moves from normalized intent");
    expect(motor.state().speedState == bs3d::PlayerSpeedState::Jog, "moving without sprint is jog state");
    expect(motor.state().grounded, "flat ground keeps motor grounded");

    intent.sprint = true;
    motor.update(intent, collision, 1.0f);
    expect(motor.state().speedState == bs3d::PlayerSpeedState::Sprint, "sprint intent reports sprint state");
    expect(bs3d::distanceXZ(motor.state().velocity, {}) <= motor.config().sprintSpeed + 0.01f,
           "sprint speed stays clamped");
}

void playerMotorDeceleratesPredictablyAfterInputRelease() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent intent;
    intent.moveDirection = {0.0f, 0.0f, 1.0f};
    motor.update(intent, collision, 0.5f);
    const float movingSpeed = bs3d::distanceXZ(motor.state().velocity, {});

    intent.moveDirection = {};
    motor.update(intent, collision, 0.2f);
    const float slowingSpeed = bs3d::distanceXZ(motor.state().velocity, {});
    expect(slowingSpeed < movingSpeed, "motor decelerates after input release");

    motor.update(intent, collision, 0.4f);
    expect(bs3d::distanceXZ(motor.state().velocity, {}) < 0.2f, "motor stops in a short predictable window");
}

void playerMotorSupportsCoyoteJumpButRejectsDoubleJump() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    motor.update(bs3d::PlayerInputIntent{}, collision, 0.016f);

    collision.clear();
    bs3d::PlayerInputIntent jump;
    jump.jumpPressed = true;
    motor.update(jump, collision, 0.05f);
    expect(motor.state().velocity.y > 0.0f, "coyote time allows jump just after leaving ground");
    expect(motor.state().movementMode == bs3d::PlayerMovementMode::Airborne, "jump switches to airborne mode");

    const float jumpVelocity = motor.state().velocity.y;
    motor.update(jump, collision, 0.05f);
    expect(motor.state().velocity.y < jumpVelocity, "second jump in air is rejected");
}

void playerMotorBuffersJumpJustBeforeLanding() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.30f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent jump;
    jump.jumpPressed = true;
    motor.update(jump, collision, 0.016f);
    expect(motor.state().velocity.y <= 0.0f, "jump buffer does not fire before landing");

    for (int i = 0; i < 8; ++i) {
        motor.update(bs3d::PlayerInputIntent{}, collision, 0.016f);
    }

    expect(motor.state().velocity.y > 0.0f, "buffered jump fires when landing becomes valid");
    expect(!motor.state().grounded, "buffered jump immediately leaves ground");
}

void worldCollisionStepOffsetAllowsLowCurbAndBlocksHighCurb() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addGroundBox({0.0f, 0.0f, 1.25f}, {4.0f, 0.25f, 1.5f});

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    bs3d::PlayerInputIntent intent;
    intent.moveDirection = {0.0f, 0.0f, 1.0f};
    for (int i = 0; i < 7; ++i) {
        motor.update(intent, collision, 0.033f);
    }

    expect(motor.state().position.z > 0.8f, "low curb does not block movement");
    expectNear(motor.state().position.y, 0.25f, 0.04f, "low curb steps player up");
    expect(motor.state().ground.hit, "ground probe reports stepped surface");

    bs3d::WorldCollision highCollision;
    highCollision.addGroundPlane(0.0f);
    highCollision.addGroundBox({0.0f, 0.0f, 1.25f}, {4.0f, 0.80f, 1.5f});

    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    for (int i = 0; i < 7; ++i) {
        motor.update(intent, highCollision, 0.033f);
    }
    expect(motor.state().position.z < 0.8f, "high curb blocks movement instead of popping player up");
    expectNear(motor.state().position.y, 0.0f, 0.01f, "blocked high curb keeps player on lower ground");
}

void worldCollisionSlopeLimitAllowsGentleRampAndBlocksSteepRamp() {
    bs3d::WorldCollision gentle;
    gentle.addGroundPlane(0.0f);
    gentle.addRamp({0.0f, 0.0f, 2.0f}, {4.0f, 0.0f, 4.0f}, 0.0f, 0.8f);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    bs3d::PlayerInputIntent intent;
    intent.moveDirection = {0.0f, 0.0f, 1.0f};
    for (int i = 0; i < 12; ++i) {
        motor.update(intent, gentle, 0.033f);
    }

    expect(motor.state().position.z > 1.0f, "gentle ramp is walkable");
    expect(motor.state().position.y > 0.05f, "gentle ramp raises player");
    expect(motor.state().ground.walkable, "ground hit marks gentle ramp walkable");

    bs3d::WorldCollision steep;
    steep.addGroundPlane(0.0f);
    steep.addRamp({0.0f, 0.0f, 2.0f}, {4.0f, 0.0f, 4.0f}, 0.0f, 4.2f);

    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    for (int i = 0; i < 12; ++i) {
        motor.update(intent, steep, 0.033f);
    }
    expect(motor.state().position.z < 1.0f, "steep ramp blocks movement");
    expect(!steep.probeGround({0.0f, 2.1f, 2.0f}, 3.0f, motor.config().walkableSlopeDegrees).walkable,
           "ground probe marks steep ramp unwalkable");
}

void worldCollisionSlidesAlongWallsWithoutSticking() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({1.10f, 0.0f, 1.0f}, {0.7f, 3.0f, 3.0f});

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent intent;
    intent.moveDirection = bs3d::normalizeXZ({1.0f, 0.0f, 1.0f});
    for (int i = 0; i < 12; ++i) {
        motor.update(intent, collision, 0.033f);
    }

    expect(motor.state().position.x < 0.30f, "wall blocks horizontal penetration");
    expect(motor.state().position.z > 0.7f, "motor slides along wall instead of sticking");
}

void worldCollisionReportsCharacterHitOwnerForNpcBodies() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    bs3d::CollisionProfile npcProfile = bs3d::CollisionProfile::playerBlocker();
    npcProfile.ownerId = 8101;
    collision.addDynamicBox({0.0f, 0.85f, 1.0f}, {0.72f, 1.70f, 0.72f}, npcProfile);

    bs3d::CharacterCollisionConfig config;
    config.radius = 0.42f;
    config.stepHeight = 0.32f;

    const bs3d::CharacterCollisionResult hitNpc =
        collision.resolveCharacter({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, config);
    expect(hitNpc.hitWall, "character collision hits the NPC body");
    expect(hitNpc.hitOwnerId == 8101, "character collision reports the NPC owner id");
    expect(hitNpc.hitProfile.ownerId == 8101, "character collision carries the hit collision profile");
    expect(bs3d::hasCollisionLayer(hitNpc.hitProfile.layers, bs3d::CollisionLayer::PlayerBlocker),
           "hit profile keeps the player-blocker layer");
}

void playerMotorExposesCharacterHitOwnerForPresentationAndReactions() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    bs3d::CollisionProfile npcProfile = bs3d::CollisionProfile::playerBlocker();
    npcProfile.ownerId = 8101;
    collision.addDynamicBox({0.0f, 0.85f, 1.0f}, {0.72f, 1.70f, 0.72f}, npcProfile);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent run;
    run.moveDirection = {0.0f, 0.0f, 1.0f};
    run.sprint = true;
    for (int i = 0; i < 20; ++i) {
        motor.update(run, collision, 1.0f / 60.0f);
        if (motor.state().hitWallThisFrame) {
            break;
        }
    }

    expect(motor.state().hitWallThisFrame, "player motor reaches the NPC body");
    expect(motor.state().hitOwnerIdThisFrame == 8101,
           "player motor exposes the hit owner id for runtime reactions");
    expect(motor.state().hitSpeedThisFrame > 0.5f,
           "player motor keeps the pre-resolution hit speed for runtime reactions");
}

void characterWorldHitPolicyConvertsNpcBodyHitToReactionEvent() {
    bs3d::PlayerMotorState state;
    state.hitWallThisFrame = true;
    state.hitOwnerIdThisFrame = 8101;
    state.hitSpeedThisFrame = 3.4f;
    state.velocity = {0.0f, 0.0f, 0.0f};
    state.grounded = true;

    const bs3d::CharacterWorldHit hit = bs3d::resolveCharacterNpcBodyHit(state, 8101, {0.0f, 0.0f, 1.0f});

    expect(hit.available, "NPC body hit becomes an available character world hit");
    expect(hit.event.source == bs3d::CharacterImpactSource::Npc,
           "NPC body hit is classified as an NPC impact source");
    expectNear(hit.event.impactSpeed, 3.4f, 0.001f,
               "NPC body hit uses motor hit speed instead of post-collision zero velocity");
    expect(hit.event.impactNormal.z < -0.9f,
           "NPC body hit reacts against the attempted movement direction");
    expect(hit.event.grounded, "NPC body hit preserves grounded state for reaction policy");
}

void characterWorldHitPolicyIgnoresAnonymousOrVerySoftHits() {
    bs3d::PlayerMotorState anonymous;
    anonymous.hitWallThisFrame = true;
    anonymous.hitOwnerIdThisFrame = 0;
    anonymous.hitSpeedThisFrame = 3.4f;
    expect(!bs3d::resolveCharacterNpcBodyHit(anonymous, 8101, {0.0f, 0.0f, 1.0f}).available,
           "anonymous wall hit is not treated as NPC body contact");

    bs3d::PlayerMotorState soft;
    soft.hitWallThisFrame = true;
    soft.hitOwnerIdThisFrame = 8101;
    soft.hitSpeedThisFrame = 0.12f;
    expect(!bs3d::resolveCharacterNpcBodyHit(soft, 8101, {0.0f, 0.0f, 1.0f}).available,
           "very soft NPC overlap does not spam reactions");
}

void playerPresentationAddsLeanBobAndLandingDip() {
    bs3d::PlayerPresentation presentation;
    bs3d::PlayerMotorState state;
    state.velocity = {3.0f, 0.0f, 5.0f};
    state.facingYawRadians = 0.0f;
    state.speedState = bs3d::PlayerSpeedState::Sprint;
    state.movementMode = bs3d::PlayerMovementMode::Grounded;

    presentation.update(state, 0.25f);
    expect(std::fabs(presentation.state().leanRadians) > 0.01f, "presentation leans during fast lateral motion");
    expect(std::fabs(presentation.state().bobHeight) > 0.001f, "presentation bobs while moving");

    state.speedState = bs3d::PlayerSpeedState::Landing;
    state.landingRecoverySeconds = 0.10f;
    presentation.update(state, 0.016f);
    expect(presentation.state().landingDip > 0.0f, "landing state drives dip feedback");
}

void playerMotorStatusEffectsKeepComedyResponsive() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::PlayerMotor normal;
    normal.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    bs3d::PlayerInputIntent sprint;
    sprint.moveDirection = {0.0f, 0.0f, 1.0f};
    sprint.sprint = true;
    normal.update(sprint, collision, 1.0f);
    const float normalSprint = bs3d::distanceXZ(normal.state().velocity, {});

    bs3d::PlayerMotor scared;
    scared.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    scared.setStatus(bs3d::PlayerStatus::Scared, true);
    scared.update(sprint, collision, 1.0f);
    expect(bs3d::distanceXZ(scared.state().velocity, {}) > normalSprint,
           "scared status makes sprint feel more panicked");
    expect(scared.state().panicAmount > 0.0f, "scared sprint exposes panic amount");

    bs3d::PlayerMotor tired;
    tired.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    tired.setStatus(bs3d::PlayerStatus::Tired, true);
    tired.update(sprint, collision, 1.0f);
    expect(bs3d::distanceXZ(tired.state().velocity, {}) < normalSprint,
           "tired status reduces sprint without changing input mapping");
}

void characterStatusPolicyUsesChaseAndHighPrzypalForPanic() {
    expect(bs3d::characterShouldBeScared(false, bs3d::PrzypalBand::Calm) == false,
           "calm Przypal without chase does not make the player panic");
    expect(bs3d::characterShouldBeScared(false, bs3d::PrzypalBand::Hot) == false,
           "Hot Przypal is pressure, but not yet panic sprint");
    expect(bs3d::characterShouldBeScared(false, bs3d::PrzypalBand::ChaseRisk),
           "ChaseRisk Przypal enables panic sprint even before full chase system");
    expect(bs3d::characterShouldBeScared(false, bs3d::PrzypalBand::Meltdown),
           "Meltdown always enables panic sprint");
    expect(bs3d::characterShouldBeScared(true, bs3d::PrzypalBand::Calm),
           "active chase still enables panic sprint regardless of current heat band");
}

void characterStatusPolicyDrivesMotorPanicFromPrzypal() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    motor.setStatus(bs3d::PlayerStatus::Scared,
                    bs3d::characterShouldBeScared(false, bs3d::PrzypalBand::ChaseRisk));

    bs3d::PlayerInputIntent sprint;
    sprint.moveDirection = {0.0f, 0.0f, 1.0f};
    sprint.sprint = true;
    motor.update(sprint, collision, 0.25f);

    expect(motor.state().statuses.scared, "Przypal policy applies scared status to the motor");
    expect(motor.state().panicAmount > 0.0f, "high Przypal drives panic presentation during sprint");
}

void playerMotorShortStaggerExpiresWithoutLongControlLoss() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    motor.triggerStagger({0.0f, 0.0f, -1.2f}, 0.35f, 0.8f);
    expect(motor.state().staggerSeconds > 0.0f, "stagger starts immediately");
    expect(motor.state().impactIntensity > 0.0f, "stagger exposes impact intensity");

    bs3d::PlayerInputIntent intent;
    intent.moveDirection = {0.0f, 0.0f, 1.0f};
    motor.update(intent, collision, 0.1f);
    expect(motor.state().position.z > -0.2f, "player keeps meaningful control during short stagger");

    motor.update(intent, collision, 0.4f);
    expectNear(motor.state().staggerSeconds, 0.0f, 0.001f, "short stagger expires quickly");
}

void playerPresentationShowsPanicSprintAndStagger() {
    bs3d::PlayerPresentation presentation;
    bs3d::PlayerMotorState state;
    state.velocity = {2.0f, 0.0f, 7.0f};
    state.facingYawRadians = 0.0f;
    state.speedState = bs3d::PlayerSpeedState::Sprint;
    state.movementMode = bs3d::PlayerMovementMode::Grounded;
    state.panicAmount = 0.85f;
    state.staggerSeconds = 0.2f;
    state.impactIntensity = 0.7f;

    presentation.update(state, 0.25f, 0.85f);
    expect(presentation.state().panicFlail > 0.0f, "panic sprint drives arm flail");
    expect(presentation.state().staggerAmount > 0.0f, "stagger drives visible body wobble");
}

void playerPresentationShowsTiredAndBruisedStatusShape() {
    bs3d::PlayerPresentation presentation;
    bs3d::PlayerMotorState state;
    state.grounded = true;
    state.velocity = {0.8f, 0.0f, 1.1f};
    state.speedState = bs3d::PlayerSpeedState::Walk;
    state.statuses.tired = true;

    presentation.update(state, 0.16f);
    expect(presentation.state().tiredAmount > 0.0f, "tired status exposes a visual tired/slouch amount");
    expectNear(presentation.state().bruisedAmount, 0.0f, 0.001f, "tired status does not imply bruised visual amount");

    state.statuses.tired = false;
    state.statuses.bruised = true;
    presentation.update(state, 0.16f);
    expect(presentation.state().bruisedAmount > 0.0f, "bruised status exposes a visual limp/bruise amount");
}

void playerPresentationSelectsRuntimeCharacterPoseKinds() {
    bs3d::PlayerPresentation presentation;
    bs3d::PlayerMotorState state;
    state.grounded = true;
    state.movementMode = bs3d::PlayerMovementMode::Grounded;

    state.speedState = bs3d::PlayerSpeedState::Idle;
    state.velocity = {};
    presentation.update(state, 0.016f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Idle,
           "idle motor state maps to idle character pose");

    state.speedState = bs3d::PlayerSpeedState::Walk;
    state.velocity = {1.7f, 0.0f, 0.0f};
    presentation.update(state, 0.016f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Walk,
           "walk motor state maps to walk character pose");

    state.speedState = bs3d::PlayerSpeedState::Jog;
    state.velocity = {3.5f, 0.0f, 0.0f};
    presentation.update(state, 0.016f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Jog,
           "jog motor state maps to jog character pose");

    state.speedState = bs3d::PlayerSpeedState::Sprint;
    state.velocity = {6.7f, 0.0f, 0.0f};
    state.panicAmount = 0.8f;
    presentation.update(state, 0.10f, 0.8f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::PanicSprint,
           "scared/high tension sprint maps to panic sprint pose");

    state.speedState = bs3d::PlayerSpeedState::Airborne;
    state.movementMode = bs3d::PlayerMovementMode::Airborne;
    state.grounded = false;
    presentation.update(state, 0.016f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Jump,
           "airborne motor state maps to jump/fall pose");

    state.grounded = true;
    state.movementMode = bs3d::PlayerMovementMode::Grounded;
    state.speedState = bs3d::PlayerSpeedState::Landing;
    state.landingRecoverySeconds = 0.12f;
    presentation.update(state, 0.016f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Land,
           "landing motor state maps to land pose");

    state.staggerSeconds = 0.20f;
    state.impactIntensity = 0.7f;
    presentation.update(state, 0.016f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Stagger,
           "active stagger overrides locomotion pose");
}

void playerPresentationProducesReadableLimbAnimationSignals() {
    bs3d::PlayerPresentation presentation;
    bs3d::PlayerMotorState idle;
    idle.grounded = true;
    idle.speedState = bs3d::PlayerSpeedState::Idle;
    presentation.update(idle, 0.016f);
    expectNear(presentation.state().locomotionAmount, 0.0f, 0.001f,
               "idle character pose has no locomotion blend");

    bs3d::PlayerMotorState jog;
    jog.grounded = true;
    jog.movementMode = bs3d::PlayerMovementMode::Grounded;
    jog.speedState = bs3d::PlayerSpeedState::Jog;
    jog.velocity = {3.4f, 0.0f, 2.2f};
    jog.facingYawRadians = 0.0f;
    presentation.update(jog, 0.25f);
    expect(presentation.state().locomotionAmount > 0.35f, "jog drives locomotion blend");
    expect(std::fabs(presentation.state().leftLegSwing) > 0.05f, "jog animates left leg");
    expectNear(presentation.state().leftLegSwing, -presentation.state().rightLegSwing, 0.001f,
               "legs swing opposite each other");
    expectNear(presentation.state().leftArmSwing, -presentation.state().leftLegSwing * 0.65f, 0.05f,
               "arms counter-swing against legs for readable TPP motion");

    bs3d::PlayerMotorState jump;
    jump.grounded = false;
    jump.speedState = bs3d::PlayerSpeedState::Airborne;
    jump.velocity = {1.0f, 3.0f, 1.0f};
    presentation.update(jump, 0.016f);
    expect(presentation.state().jumpStretch > 0.0f, "airborne pose exposes jump stretch for renderer");
}

void playerPresentationUsesActionContextWithoutMutatingMotorContract() {
    bs3d::PlayerPresentation presentation;
    bs3d::PlayerMotorState state;
    state.grounded = true;
    state.speedState = bs3d::PlayerSpeedState::Idle;

    bs3d::PlayerPresentationContext talk;
    talk.talkActive = true;
    presentation.update(state, 0.10f, 0.0f, talk);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Talk,
           "talk context maps idle body to talk pose");
    expect(presentation.state().talkGesture > 0.0f, "talk pose exposes a hand gesture signal");

    bs3d::PlayerPresentationContext interaction;
    interaction.interactActive = true;
    presentation.update(state, 0.10f, 0.0f, interaction);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Interact,
           "interaction context overrides idle/talk for interaction pose");
    expect(presentation.state().interactReach > 0.0f, "interaction pose exposes reach signal");

    bs3d::PlayerPresentationContext vehicle;
    vehicle.inVehicle = true;
    vehicle.steerInput = 0.0f;
    presentation.update(state, 0.10f, 0.0f, vehicle);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::SitVehicle,
           "vehicle context maps player presentation to seated pose");
    expect(presentation.state().vehicleSitAmount > 0.0f, "vehicle pose exposes sit amount");
    expect(std::fabs(presentation.state().steerAmount) < 0.1f, "neutral vehicle pose keeps steering amount near zero");

    bs3d::PlayerPresentationContext onCar;
    onCar.standingOnVehicle = true;
    presentation.update(state, 0.10f, 0.0f, onCar);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::StandVehicle,
           "standing on vehicle gets a dedicated balance pose");
    expect(presentation.state().balanceAmount > 0.0f, "standing on vehicle exposes balance signal");
}

void characterPoseCatalogCoversRequestedRuntimeAnimationHooks() {
    const std::vector<bs3d::CharacterPoseKind> catalog = bs3d::characterPoseCatalog();
    auto contains = [&](bs3d::CharacterPoseKind kind) {
        return std::find(catalog.begin(), catalog.end(), kind) != catalog.end();
    };

    expect(contains(bs3d::CharacterPoseKind::Idle), "pose catalog covers Idle");
    expect(contains(bs3d::CharacterPoseKind::Walk), "pose catalog covers walk");
    expect(contains(bs3d::CharacterPoseKind::Jog), "pose catalog covers jog");
    expect(contains(bs3d::CharacterPoseKind::Sprint), "pose catalog covers sprint");
    expect(contains(bs3d::CharacterPoseKind::StartMove), "pose catalog covers start move");
    expect(contains(bs3d::CharacterPoseKind::StopMove), "pose catalog covers stop move");
    expect(contains(bs3d::CharacterPoseKind::QuickTurn), "pose catalog covers quick turn");
    expect(contains(bs3d::CharacterPoseKind::Jump), "pose catalog covers jump");
    expect(contains(bs3d::CharacterPoseKind::Land), "pose catalog covers land");
    expect(contains(bs3d::CharacterPoseKind::Stagger), "pose catalog covers bump/stagger");
    expect(contains(bs3d::CharacterPoseKind::Talk), "pose catalog covers talk idle");
    expect(contains(bs3d::CharacterPoseKind::Interact), "pose catalog covers interact");
    expect(contains(bs3d::CharacterPoseKind::EnterVehicle), "pose catalog covers enter vehicle");
    expect(contains(bs3d::CharacterPoseKind::ExitVehicle), "pose catalog covers exit vehicle");
    expect(contains(bs3d::CharacterPoseKind::SitVehicle), "pose catalog covers sit in vehicle");
    expect(contains(bs3d::CharacterPoseKind::SteerLeft), "pose catalog covers steer left");
    expect(contains(bs3d::CharacterPoseKind::SteerRight), "pose catalog covers steer right");
    expect(contains(bs3d::CharacterPoseKind::StandVehicle), "pose catalog covers standing on car");
    expect(contains(bs3d::CharacterPoseKind::JumpOffVehicle), "pose catalog covers jumping off car");
    expect(contains(bs3d::CharacterPoseKind::LowVault), "pose catalog covers low vault");
    expect(contains(bs3d::CharacterPoseKind::CarryObject), "pose catalog covers carry object");
    expect(contains(bs3d::CharacterPoseKind::PushObject), "pose catalog covers push object");
    expect(contains(bs3d::CharacterPoseKind::Punch), "pose catalog covers punch");
    expect(contains(bs3d::CharacterPoseKind::Dodge), "pose catalog covers dodge");
    expect(contains(bs3d::CharacterPoseKind::Fall), "pose catalog covers fall");
    expect(contains(bs3d::CharacterPoseKind::GetUp), "pose catalog covers get up");
    expect(contains(bs3d::CharacterPoseKind::Drink), "pose catalog covers drinking");
    expect(contains(bs3d::CharacterPoseKind::Smoke), "pose catalog covers smoking");
    expect(contains(bs3d::CharacterPoseKind::PanicSprint), "pose catalog covers panic sprint");
}

void playerPresentationOneShotActionsTemporarilyOverrideBasePose() {
    bs3d::PlayerPresentation presentation;
    bs3d::PlayerMotorState state;
    state.grounded = true;
    state.speedState = bs3d::PlayerSpeedState::Idle;

    presentation.playOneShot(bs3d::CharacterPoseKind::Punch, 0.35f);
    presentation.update(state, 0.10f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Punch,
           "one-shot punch overrides idle pose while active");
    expect(presentation.state().actionAmount > 0.0f, "one-shot exposes action amount for renderer");
    expect(presentation.oneShotActive(), "one-shot reports active during its duration");

    presentation.update(state, 0.40f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Idle,
           "one-shot returns to base pose after duration");
    expect(!presentation.oneShotActive(), "one-shot reports inactive after expiry");
}

void playerPresentationExposesForwardOneShotNormalizedTime() {
    bs3d::PlayerPresentation presentation;
    bs3d::PlayerMotorState state;
    state.grounded = true;
    state.speedState = bs3d::PlayerSpeedState::Idle;

    presentation.playOneShot(bs3d::CharacterPoseKind::Punch, 0.50f);
    presentation.update(state, 0.0f);
    expectNear(presentation.state().oneShotNormalizedTime, 0.0f, 0.001f,
               "one-shot normalized time starts at zero");

    presentation.update(state, 0.25f);
    expectNear(presentation.state().oneShotNormalizedTime, 0.5f, 0.02f,
               "one-shot normalized time advances forward through the action");

    presentation.update(state, 0.25f);
    expectNear(presentation.state().oneShotNormalizedTime, 1.0f, 0.001f,
               "one-shot normalized time reaches one when the action completes");
}

void playerPresentationVehicleSteerAndEnterExitUseActionPoses() {
    bs3d::PlayerPresentation presentation;
    bs3d::PlayerMotorState state;
    state.grounded = true;
    state.speedState = bs3d::PlayerSpeedState::Idle;

    bs3d::PlayerPresentationContext vehicle;
    vehicle.inVehicle = true;
    vehicle.steerInput = -0.85f;
    presentation.update(state, 0.12f, 0.0f, vehicle);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::SteerLeft,
           "vehicle left steer gets a dedicated steering pose");
    expect(presentation.state().vehicleSitAmount > 0.0f,
           "vehicle steering pose keeps the seated body channel active");
    expect(presentation.state().steerAmount < -0.05f,
           "vehicle left steer drives a visible negative steering amount");

    vehicle.steerInput = 0.85f;
    presentation.update(state, 0.12f, 0.0f, vehicle);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::SteerRight,
           "vehicle right steer gets a dedicated steering pose");
    expect(presentation.state().vehicleSitAmount > 0.0f,
           "vehicle right steering keeps the seated body channel active");
    expect(presentation.state().steerAmount > 0.05f,
           "vehicle right steer drives a visible positive steering amount");

    presentation.playOneShot(bs3d::CharacterPoseKind::EnterVehicle, 0.55f);
    presentation.update(state, 0.10f, 0.0f, vehicle);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::EnterVehicle,
           "enter vehicle one-shot overrides seated steering pose briefly");

    presentation.update(state, 0.60f, 0.0f, vehicle);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::SteerRight,
           "after enter vehicle one-shot expires, steering pose resumes");
}

void playerPresentationUsesStartStopAndQuickTurnTransitionPoses() {
    bs3d::PlayerPresentation presentation;
    bs3d::PlayerMotorState state;
    state.grounded = true;
    state.speedState = bs3d::PlayerSpeedState::Idle;
    presentation.update(state, 0.016f);

    state.velocity = bs3d::forwardFromYaw(0.0f) * 4.4f;
    state.speedState = bs3d::PlayerSpeedState::Jog;
    presentation.update(state, 0.016f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::StartMove,
           "presentation exposes a short start-move pose when leaving idle");
    expect(presentation.state().startMoveAmount > 0.0f,
           "start-move pose exposes a dedicated animation amount");

    presentation.update(state, 0.24f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Jog,
           "start-move pose quickly returns to normal locomotion");

    state.velocity = bs3d::forwardFromYaw(bs3d::Pi) * 4.4f;
    state.speedState = bs3d::PlayerSpeedState::Jog;
    presentation.update(state, 0.016f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::QuickTurn,
           "presentation exposes a quick-turn pose for fast opposite direction changes");
    expect(presentation.state().quickTurnAmount > 0.0f,
           "quick-turn pose exposes a dedicated animation amount");

    presentation.update(state, 0.26f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Jog,
           "quick-turn pose quickly returns to normal locomotion");

    state.velocity = {};
    state.speedState = bs3d::PlayerSpeedState::Idle;
    presentation.update(state, 0.016f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::StopMove,
           "presentation exposes a short stop pose when braking to idle");
    expect(presentation.state().stopMoveAmount > 0.0f,
           "stop pose exposes a dedicated animation amount");

    presentation.update(state, 0.22f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Idle,
           "stop pose returns to idle without locking control");
}

void playerPresentationUsesJumpOffVehicleWhenLeavingDynamicSupport() {
    bs3d::PlayerPresentation presentation;
    bs3d::PlayerMotorState state;
    state.grounded = true;
    state.speedState = bs3d::PlayerSpeedState::Idle;
    state.ground.hit = true;
    state.ground.ownerId = 77;

    bs3d::PlayerPresentationContext onVehicle;
    onVehicle.standingOnVehicle = true;
    presentation.update(state, 0.10f, 0.0f, onVehicle);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::StandVehicle,
           "standing on dynamic vehicle support uses vehicle balance pose");

    state.grounded = false;
    state.movementMode = bs3d::PlayerMovementMode::Airborne;
    state.speedState = bs3d::PlayerSpeedState::Airborne;
    state.ground = {};
    state.velocity = {0.8f, 0.0f, 1.2f};
    presentation.update(state, 0.016f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::JumpOffVehicle,
           "leaving dynamic vehicle support gets a dedicated jump-off pose");
    expect(presentation.state().jumpOffVehicleAmount > 0.0f,
           "jump-off vehicle pose exposes a dedicated animation amount");

    presentation.update(state, 0.34f);
    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::Jump,
           "jump-off vehicle pose quickly returns to regular airborne pose");
}

void playerPresentationExposesDistinctActionPoseChannels() {
    bs3d::PlayerMotorState state;
    state.grounded = true;
    state.speedState = bs3d::PlayerSpeedState::Idle;

    bs3d::PlayerPresentation punch;
    punch.playOneShot(bs3d::CharacterPoseKind::Punch, 0.34f);
    punch.update(state, 0.08f);
    expect(punch.state().punchAmount > 0.0f, "punch one-shot exposes punch animation channel");
    expect(punch.state().dodgeAmount == 0.0f, "punch one-shot does not drive dodge channel");

    bs3d::PlayerPresentation dodge;
    dodge.playOneShot(bs3d::CharacterPoseKind::Dodge, 0.34f);
    dodge.update(state, 0.08f);
    expect(dodge.state().dodgeAmount > 0.0f, "dodge one-shot exposes dodge animation channel");
    expect(dodge.state().punchAmount == 0.0f, "dodge one-shot does not drive punch channel");

    bs3d::PlayerPresentation carry;
    carry.playOneShot(bs3d::CharacterPoseKind::CarryObject, 0.58f);
    carry.update(state, 0.08f);
    expect(carry.state().carryAmount > 0.0f, "carry one-shot exposes carry animation channel");

    bs3d::PlayerPresentation push;
    push.playOneShot(bs3d::CharacterPoseKind::PushObject, 0.48f);
    push.update(state, 0.08f);
    expect(push.state().pushAmount > 0.0f, "push one-shot exposes push animation channel");

    bs3d::PlayerPresentation drink;
    drink.playOneShot(bs3d::CharacterPoseKind::Drink, 0.74f);
    drink.update(state, 0.08f);
    expect(drink.state().drinkAmount > 0.0f, "drink one-shot exposes drink animation channel");

    bs3d::PlayerPresentation smoke;
    smoke.playOneShot(bs3d::CharacterPoseKind::Smoke, 0.74f);
    smoke.update(state, 0.08f);
    expect(smoke.state().smokeAmount > 0.0f, "smoke one-shot exposes smoke animation channel");

    bs3d::PlayerPresentation fall;
    fall.playOneShot(bs3d::CharacterPoseKind::Fall, 0.62f);
    fall.update(state, 0.08f);
    expect(fall.state().fallAmount > 0.0f, "fall one-shot exposes fall animation channel");

    bs3d::PlayerPresentation getUp;
    getUp.playOneShot(bs3d::CharacterPoseKind::GetUp, 0.54f);
    getUp.update(state, 0.08f);
    expect(getUp.state().getUpAmount > 0.0f, "get-up one-shot exposes get-up animation channel");

    bs3d::PlayerPresentation vault;
    vault.playOneShot(bs3d::CharacterPoseKind::LowVault, 0.46f);
    vault.update(state, 0.08f);
    expect(vault.state().lowVaultAmount > 0.0f, "low-vault one-shot exposes vault animation channel");
}

void playerPresentationUsesPersistentCarryPoseWhileHoldingObject() {
    bs3d::PlayerPresentation presentation;
    bs3d::PlayerMotorState state;
    state.grounded = true;
    state.velocity = bs3d::forwardFromYaw(0.0f) * 2.0f;
    state.speedState = bs3d::PlayerSpeedState::Walk;

    bs3d::PlayerPresentationContext context;
    context.carryingObject = true;
    presentation.update(state, 0.10f, 0.0f, context);

    expect(presentation.state().poseKind == bs3d::CharacterPoseKind::CarryObject,
           "carrying a prop keeps the character in carry pose while moving");
    expect(presentation.state().carryAmount > 0.0f,
           "persistent carry state drives the carry animation channel");
    expect(presentation.state().locomotionAmount > 0.0f,
           "carry pose still allows locomotion instead of freezing the controller");
}

void characterActionMapperTurnsRuntimeInputsIntoReadablePoses() {
    bs3d::InputState input;
    input.primaryActionPressed = true;
    bs3d::CharacterActionRequest request = bs3d::resolveCharacterAction(input);
    expect(request.available, "primary action creates a character pose request");
    expect(request.poseKind == bs3d::CharacterPoseKind::Punch, "primary action maps to punch");
    expectNear(request.durationSeconds, 0.34f, 0.001f, "punch has a short responsive duration");

    input = {};
    input.secondaryActionPressed = true;
    request = bs3d::resolveCharacterAction(input);
    expect(request.available, "secondary action creates a character pose request");
    expect(request.poseKind == bs3d::CharacterPoseKind::Dodge, "secondary action maps to dodge");

    input = {};
    input.carryActionPressed = true;
    request = bs3d::resolveCharacterAction(input);
    expect(request.available && request.poseKind == bs3d::CharacterPoseKind::CarryObject,
           "carry action maps to carry pose");

    input = {};
    input.pushActionPressed = true;
    request = bs3d::resolveCharacterAction(input);
    expect(request.available && request.poseKind == bs3d::CharacterPoseKind::PushObject,
           "push action maps to push pose");

    input = {};
    input.drinkActionPressed = true;
    request = bs3d::resolveCharacterAction(input);
    expect(request.available && request.poseKind == bs3d::CharacterPoseKind::Drink,
           "drink action maps to drink pose");

    input = {};
    input.smokeActionPressed = true;
    request = bs3d::resolveCharacterAction(input);
    expect(request.available && request.poseKind == bs3d::CharacterPoseKind::Smoke,
           "smoke action maps to smoke pose");

    input = {};
    input.lowVaultActionPressed = true;
    request = bs3d::resolveCharacterAction(input);
    expect(request.available && request.poseKind == bs3d::CharacterPoseKind::LowVault,
           "low-vault action maps to vault pose");
}

void characterActionMapperQueuesGetUpAfterFallLite() {
    bs3d::InputState input;
    input.fallActionPressed = true;

    const bs3d::CharacterActionRequest request = bs3d::resolveCharacterAction(input);
    expect(request.available, "fall action creates a character pose request");
    expect(request.poseKind == bs3d::CharacterPoseKind::Fall, "fall action maps to fall pose");
    expect(request.followUpAvailable, "fall action queues a get-up follow-up");
    expect(request.followUpPoseKind == bs3d::CharacterPoseKind::GetUp, "fall follow-up is get-up");
    expectNear(request.followUpDelaySeconds, request.durationSeconds, 0.001f,
               "get-up follows after the fall one-shot duration");
}

void characterActionStartPolicyRequiresVaultOpportunityForLowVault() {
    bs3d::InputState input;
    input.lowVaultActionPressed = true;
    const bs3d::CharacterActionRequest request = bs3d::resolveCharacterAction(input);

    bs3d::CharacterActionStartContext context;
    context.lowVaultAvailable = false;
    expect(!bs3d::canStartCharacterAction(request, context),
           "low-vault action does not start when no low obstacle can be vaulted");

    context.lowVaultAvailable = true;
    expect(bs3d::canStartCharacterAction(request, context),
           "low-vault action starts when vault policy found a valid obstacle");
}

void characterActionStartPolicyAllowsNormalActionsWithoutVaultOpportunity() {
    bs3d::InputState input;
    input.primaryActionPressed = true;
    const bs3d::CharacterActionRequest punch = bs3d::resolveCharacterAction(input);

    bs3d::CharacterActionStartContext context;
    context.lowVaultAvailable = false;
    expect(bs3d::canStartCharacterAction(punch, context),
           "normal character actions are not blocked by missing vault opportunity");
}

void characterActionStartPolicyRequiresPushTargetForPushObject() {
    bs3d::InputState input;
    input.pushActionPressed = true;
    const bs3d::CharacterActionRequest push = bs3d::resolveCharacterAction(input);

    bs3d::CharacterActionStartContext context;
    context.pushTargetAvailable = false;
    expect(!bs3d::canStartCharacterAction(push, context),
           "push-object action does not start when there is no movable prop target");

    context.pushTargetAvailable = true;
    expect(bs3d::canStartCharacterAction(push, context),
           "push-object action starts when a movable prop is in reach");
}

void characterLowVaultRequiresLowObstacleAhead() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::CharacterVaultRequest request;
    request.position = {0.0f, 0.0f, 0.0f};
    request.forward = {0.0f, 0.0f, 1.0f};

    const bs3d::CharacterVaultResult noObstacle = bs3d::resolveCharacterLowVault(collision, request);
    expect(!noObstacle.available, "low vault is not a magic hop when there is no obstacle ahead");

    collision.addGroundBox({0.0f, 0.0f, 0.78f}, {1.0f, 0.42f, 0.45f});

    const bs3d::CharacterVaultResult lowObstacle = bs3d::resolveCharacterLowVault(collision, request);
    expect(lowObstacle.available, "low vault is available for a low walkable obstacle ahead");
    expect(lowObstacle.obstacleHeight > 0.25f && lowObstacle.obstacleHeight < 0.70f,
           "low vault reports the obstacle height");
}

void characterLowVaultRejectsHighWalls() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addGroundBox({0.0f, 0.0f, 0.78f}, {1.0f, 1.10f, 0.45f});

    bs3d::CharacterVaultRequest request;
    request.position = {0.0f, 0.0f, 0.0f};
    request.forward = {0.0f, 0.0f, 1.0f};

    const bs3d::CharacterVaultResult result = bs3d::resolveCharacterLowVault(collision, request);
    expect(!result.available, "low vault rejects obstacles that are too high for a quick hop");
}

void characterLowVaultRequiresClearLandingSpace() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addGroundBox({0.0f, 0.0f, 0.78f}, {1.0f, 0.36f, 0.45f});
    collision.addBox({0.0f, 0.75f, 1.52f}, {1.8f, 1.5f, 0.20f});

    bs3d::CharacterVaultRequest request;
    request.position = {0.0f, 0.0f, 0.0f};
    request.forward = {0.0f, 0.0f, 1.0f};

    const bs3d::CharacterVaultResult result = bs3d::resolveCharacterLowVault(collision, request);
    expect(!result.available, "low vault does not start when the landing capsule is blocked");
}

void characterLowVaultProducesControlledForwardImpulse() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addGroundBox({0.0f, 0.0f, 0.78f}, {1.0f, 0.36f, 0.45f});

    bs3d::CharacterVaultRequest request;
    request.position = {0.0f, 0.0f, 0.0f};
    request.forward = {0.0f, 0.0f, 1.0f};

    const bs3d::CharacterVaultResult result = bs3d::resolveCharacterLowVault(collision, request);
    expect(result.available, "low vault result is available before checking impulse");
    expect(result.takeoffImpulse.z > 0.8f, "low vault pushes mostly forward over the obstacle");
    expect(result.takeoffImpulse.y > 0.45f && result.takeoffImpulse.y < 1.05f,
           "low vault uses a controlled hop impulse");
    expect(std::fabs(result.takeoffImpulse.x) < 0.05f, "low vault does not add side drift");
}

void characterActionMapperUsesCinematicPriorityForConflictingInputs() {
    bs3d::InputState input;
    input.primaryActionPressed = true;
    input.fallActionPressed = true;
    input.lowVaultActionPressed = true;

    const bs3d::CharacterActionRequest request = bs3d::resolveCharacterAction(input);
    expect(request.poseKind == bs3d::CharacterPoseKind::Fall,
           "fall wins over smaller action buttons when multiple test hooks are pressed");
}

void characterPhysicalReactionKeepsSmallBumpsShortAndControllable() {
    bs3d::CharacterImpactEvent event;
    event.source = bs3d::CharacterImpactSource::Wall;
    event.impactSpeed = 1.35f;
    event.impactNormal = {-1.0f, 0.0f, 0.0f};
    event.grounded = true;

    const bs3d::CharacterPhysicalReaction reaction = bs3d::resolveCharacterPhysicalReaction(event);

    expect(reaction.kind == bs3d::CharacterReactionKind::Flinch,
           "small wall bump becomes a flinch, not a full fall");
    expect(reaction.poseKind == bs3d::CharacterPoseKind::Stagger,
           "flinch still drives the visible stagger pose");
    expect(reaction.staggerSeconds > 0.0f && reaction.staggerSeconds <= 0.16f,
           "small bump keeps control lock extremely short");
    expect(!reaction.followUpAvailable, "small bump does not queue a get-up");
    expect(bs3d::distanceXZ(reaction.impulse, {}) < 0.65f,
           "small bump impulse is readable but not disruptive");
}

void characterPhysicalReactionVehicleHitFallsLiteAndQueuesGetUp() {
    bs3d::CharacterImpactEvent event;
    event.source = bs3d::CharacterImpactSource::Vehicle;
    event.impactSpeed = 7.2f;
    event.impactNormal = {1.0f, 0.0f, 0.0f};
    event.grounded = true;

    const bs3d::CharacterPhysicalReaction reaction = bs3d::resolveCharacterPhysicalReaction(event);

    expect(reaction.kind == bs3d::CharacterReactionKind::FallLite,
           "strong vehicle hit uses controlled fall-lite instead of normal stagger");
    expect(reaction.poseKind == bs3d::CharacterPoseKind::Fall, "vehicle hit plays fall pose");
    expect(reaction.followUpAvailable, "fall-lite queues get-up automatically");
    expect(reaction.followUpPoseKind == bs3d::CharacterPoseKind::GetUp, "fall-lite follow-up is get-up");
    expect(reaction.staggerSeconds <= 0.72f, "fall-lite does not steal control for too long");
    expect(reaction.impulse.x > 1.0f && reaction.impulse.y > 0.55f,
           "vehicle hit gives a readable sideways/up comedy impulse");
    expect(reaction.intensity <= 1.0f, "reaction intensity is clamped for camera/feedback safety");
}

void characterPhysicalReactionAppliesToMotorAndPresentation() {
    bs3d::PlayerController player;
    bs3d::PlayerPresentation presentation;
    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::CharacterPhysicalReaction reaction;
    reaction.kind = bs3d::CharacterReactionKind::Stagger;
    reaction.poseKind = bs3d::CharacterPoseKind::Stagger;
    reaction.poseDurationSeconds = 0.22f;
    reaction.staggerSeconds = 0.20f;
    reaction.intensity = 0.55f;
    reaction.impulse = {1.2f, 0.0f, 0.0f};

    bs3d::applyCharacterPhysicalReaction(player, presentation, reaction);

    expect(player.motorState().staggerSeconds > 0.0f, "physical reaction starts motor stagger");
    expect(player.velocity().x > 0.8f, "physical reaction applies controller impulse");
    expect(presentation.oneShotActive(), "physical reaction starts a visible one-shot pose");
}

void characterPhysicalReactionApplicationReturnsFeedbackAndFollowUpContract() {
    bs3d::PlayerController player;
    bs3d::PlayerPresentation presentation;
    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::CharacterPhysicalReaction reaction;
    reaction.kind = bs3d::CharacterReactionKind::FallLite;
    reaction.poseKind = bs3d::CharacterPoseKind::Fall;
    reaction.poseDurationSeconds = 0.62f;
    reaction.staggerSeconds = 0.60f;
    reaction.intensity = 0.78f;
    reaction.impulse = {2.2f, 0.7f, 0.0f};
    reaction.followUpAvailable = true;
    reaction.followUpPoseKind = bs3d::CharacterPoseKind::GetUp;
    reaction.followUpDelaySeconds = 0.62f;
    reaction.followUpDurationSeconds = 0.52f;

    const bs3d::CharacterReactionApplicationResult result =
        bs3d::applyCharacterPhysicalReaction(player, presentation, reaction);

    expect(result.applied, "reaction application reports that it actually changed runtime state");
    expect(result.feedbackIntensity >= reaction.intensity * 0.75f,
           "reaction application exposes feedback intensity for camera/audio glue");
    expect(result.followUpAvailable, "reaction application preserves follow-up scheduling contract");
    expect(result.followUpPoseKind == bs3d::CharacterPoseKind::GetUp, "runtime follow-up remains get-up");
    expectNear(result.followUpDelaySeconds, reaction.followUpDelaySeconds, 0.001f,
               "runtime follow-up delay matches reaction policy");
}

void cameraRigSmoothlyFollowsTargetWithoutTeleporting() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::Walking;

    bs3d::CameraRigState first = rig.update(target, 0.016f);
    expectNear(first.target.z, 0.0f, 0.01f, "camera initially targets player");

    target.position = {0.0f, 0.0f, 20.0f};
    bs3d::CameraRigState second = rig.update(target, 0.016f);
    expect(second.target.z > first.target.z, "camera begins following moved target");
    expect(second.target.z < 20.0f, "camera target does not teleport");
    expect(second.fovY > 40.0f && second.fovY < 70.0f, "camera produces playable fov");

    target.mode = bs3d::CameraRigMode::Driving;
    bs3d::CameraRigState driving = rig.update(target, 0.5f);
    expect(driving.position.y > second.position.y, "driving camera sits higher");
}

void cameraRigOnFootNormalUsesCloserReadableFraming() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootNormal;

    rig.reset(target);
    const bs3d::CameraRigState state = rig.update(target, 0.016f);

    expect(state.boomLength > 4.2f, "normal on-foot camera is not over-the-shoulder cramped");
    expect(state.boomLength < 5.8f, "normal on-foot camera is close enough for osiedle readability");
    expect(state.target.y >= 1.45f && state.target.y <= 1.80f, "normal on-foot camera targets upper body");
    expect(state.fovY >= 60.0f && state.fovY <= 70.0f, "normal on-foot camera uses readable TPP fov");
}

void cameraRigOnFootLookAheadStaysSmallForStableWalkingBaseline() {
    bs3d::CameraRig walkingRig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.velocity = {0.0f, 0.0f, 4.8f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootNormal;
    target.speedRatio = 1.0f;

    walkingRig.reset(target);
    const bs3d::CameraRigState walking = walkingRig.update(target, 0.016f);
    expect(bs3d::distanceXZ(walking.desiredTarget, {0.0f, walking.desiredTarget.y, 0.0f}) <= 0.001f,
           "walking camera target stays exactly anchored to player instead of drifting forward");

    bs3d::CameraRig sprintRig;
    target.velocity = {0.0f, 0.0f, 7.2f};
    target.mode = bs3d::CameraRigMode::OnFootSprint;
    sprintRig.reset(target);
    const bs3d::CameraRigState sprint = sprintRig.update(target, 0.016f);
    expect(bs3d::distanceXZ(sprint.desiredTarget, {0.0f, sprint.desiredTarget.y, 0.0f}) <= 0.45f,
           "sprint camera keeps only a small readable look-ahead during rescue baseline");
}

void cameraRigOnFootTargetDoesNotRubberBandDuringJog() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.velocity = {0.0f, 0.0f, 4.8f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootNormal;
    target.speedRatio = 4.8f / 6.2f;
    rig.reset(target);

    bs3d::CameraRigState state = rig.state();
    for (int i = 0; i < 60; ++i) {
        target.position.z += target.velocity.z * (1.0f / 60.0f);
        state = rig.update(target, 1.0f / 60.0f, nullptr);
    }

    expect(bs3d::distanceXZ(state.target, state.desiredTarget) < 0.18f,
           "on-foot camera target stays locked enough to avoid screen rubber-banding during jog");
    expect(bs3d::distanceXZ(state.position, state.desiredPosition) < 0.38f,
           "on-foot camera body follows closely enough to avoid visible whole-scene jitter");
}

void cameraRigSupportsOrbitAndDrivingRecenter() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::Walking;
    rig.reset(target);

    bs3d::CameraRigControlInput look;
    look.mode = bs3d::CameraRigMode::Walking;
    look.followYawRadians = 0.0f;
    look.mouseDeltaX = 100.0f;
    look.mouseDeltaY = -20.0f;
    look.mouseLookActive = true;
    rig.beginFrame(look, 0.016f);
    bs3d::CameraRigState orbit = rig.update(target, 0.016f);
    expect(orbit.cameraYawRadians < -0.1f, "mouse delta rotates orbit camera toward raylib screen-right");
    expect(orbit.pitchRadians > -0.36f, "mouse y changes pitch naturally");

    target.mode = bs3d::CameraRigMode::Driving;
    target.yawRadians = bs3d::Pi;
    look.mode = bs3d::CameraRigMode::Driving;
    look.followYawRadians = bs3d::Pi;
    look.mouseDeltaX = 0.0f;
    look.mouseDeltaY = 0.0f;
    bs3d::CameraRigState beforeRecenter = rig.update(target, 0.1f);
    rig.beginFrame(look, 2.0f);
    bs3d::CameraRigState afterRecenter = rig.update(target, 2.0f);
    expect(afterRecenter.cameraYawRadians < beforeRecenter.cameraYawRadians, "driving camera recenters toward vehicle yaw");
}

void cameraRigVehicleSpeedExpandsFovAndBoomForEscapeReadability() {
    bs3d::CameraRig idleRig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::Vehicle;
    target.speedRatio = 0.0f;
    idleRig.reset(target);
    const bs3d::CameraRigState idle = idleRig.update(target, 0.016f);

    bs3d::CameraRig fastRig;
    target.velocity = {0.0f, 0.0f, 18.0f};
    target.speedRatio = 1.0f;
    fastRig.reset(target);
    const bs3d::CameraRigState fast = fastRig.update(target, 0.016f);

    expect(idle.fovY >= 66.0f, "vehicle idle fov is already readable");
    expect(fast.fovY > idle.fovY + 4.0f, "vehicle speed increases fov");
    expect(fast.boomLength > idle.boomLength + 1.0f, "vehicle speed pulls camera back");
    expect(fast.fovY <= 82.0f, "vehicle speed fov stays capped");
}

void cameraRigVehicleLowSpeedTargetDoesNotDanceForward() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::Vehicle;
    target.velocity = {0.0f, 0.0f, 0.30f};
    target.speedRatio = 0.025f;

    rig.reset(target);
    const bs3d::CameraRigState slow = rig.state();

    expect(std::fabs(slow.desiredTarget.z) < 0.36f,
           "tiny throttle does not push vehicle camera target far forward");

    target.velocity = {};
    target.speedRatio = 0.0f;
    const bs3d::CameraRigState stopped = rig.update(target, 1.0f / 60.0f);
    expect(bs3d::distanceXZ(stopped.desiredTarget, slow.desiredTarget) < 0.28f,
           "stopping after light throttle does not make the camera target dance");
}

void cameraRigUsesNaturalMouseYAndTrueOrbitPitch() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::Walking;
    rig.reset(target);

    const bs3d::CameraRigState start = rig.state();

    bs3d::CameraRigControlInput lookUp;
    lookUp.mode = bs3d::CameraRigMode::Walking;
    lookUp.followYawRadians = 0.0f;
    lookUp.mouseDeltaY = -80.0f;
    lookUp.mouseLookActive = true;
    rig.beginFrame(lookUp, 0.016f);
    const bs3d::CameraRigState afterLookUp = rig.update(target, 0.016f);

    expect(afterLookUp.pitchRadians > start.pitchRadians, "mouse up lowers orbit pitch without inverted Y");
    expect(afterLookUp.position.y < start.position.y, "true orbit pitch changes camera height");
    expect(afterLookUp.position.z < start.position.z, "true orbit pitch also changes camera distance");
}

void cameraRigDrivingRecentersAfterMouseLookEnds() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::Driving;
    rig.reset(target);

    bs3d::CameraRigControlInput look;
    look.mode = bs3d::CameraRigMode::Driving;
    look.followYawRadians = 0.0f;
    look.mouseDeltaX = 100.0f;
    look.mouseLookActive = true;
    const float turnedYaw = rig.beginFrame(look, 0.016f);
    expect(turnedYaw < -0.05f, "mouse can look away while driving");

    look.mouseDeltaX = 0.0f;
    look.followYawRadians = bs3d::Pi;
    float heldYaw = turnedYaw;
    for (int i = 0; i < 60; ++i) {
        heldYaw = rig.beginFrame(look, 1.0f / 60.0f);
    }
    expectNear(heldYaw, turnedYaw, 0.01f, "driving camera gives manual look at least one second before recenter");

    float gentleYaw = heldYaw;
    for (int i = 0; i < 36; ++i) {
        gentleYaw = rig.beginFrame(look, 1.0f / 60.0f);
    }
    expect(gentleYaw < heldYaw - 0.04f, "driving recenter eventually helps after comfort delay");
    expect(gentleYaw > -1.65f, "driving recenter does not snap aggressively toward vehicle heading");
}

void cameraRigVehicleResetStartsBehindVehicleYaw() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {2.0f, 0.0f, 3.0f};
    target.yawRadians = bs3d::Pi * 0.5f;
    target.mode = bs3d::CameraRigMode::Vehicle;

    rig.reset(target);
    const bs3d::CameraRigState state = rig.state();

    expectNear(state.cameraYawRadians, target.yawRadians, 0.001f, "vehicle camera reset adopts vehicle yaw");
    expect(state.position.x < target.position.x, "camera starts behind +X-facing gruz after vehicle enter");
}

void cameraRigDoesNotFightJoggingWithOnFootAutoRecenter() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootNormal;
    rig.reset(target);

    bs3d::CameraRigControlInput look;
    look.mode = bs3d::CameraRigMode::OnFootNormal;
    look.followYawRadians = bs3d::Pi;
    look.speedRatio = 0.55f;
    look.mouseDeltaX = 100.0f;
    look.mouseLookActive = true;
    const float playerYaw = rig.beginFrame(look, 0.016f);

    look.mouseDeltaX = 0.0f;
    const float afterDelay = rig.beginFrame(look, 3.0f);
    expectNear(afterDelay, playerYaw, 0.01f, "jogging on foot does not auto-recenter against camera intent");
}

void cameraRigRecentersOnFootOnlyForSprintPressure() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootSprint;
    rig.reset(target);

    bs3d::CameraRigControlInput look;
    look.mode = bs3d::CameraRigMode::OnFootSprint;
    look.followYawRadians = bs3d::Pi;
    look.speedRatio = 0.95f;
    look.mouseDeltaX = 100.0f;
    look.mouseLookActive = true;
    const float playerYaw = rig.beginFrame(look, 0.016f);

    look.mouseDeltaX = 0.0f;
    const float beforeRecenter = rig.beginFrame(look, 1.0f);
    const float afterRecenter = rig.beginFrame(look, 1.0f);
    expect(afterRecenter < beforeRecenter, "sprint pressure can gently recenter camera behind movement");
    expect(afterRecenter < playerYaw, "sprint recenter moves away from manually offset yaw after delay");
}

void cameraRigSprintAutoAlignWaitsBeforeHelping() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootSprint;
    rig.reset(target);

    bs3d::CameraRigControlInput look;
    look.mode = bs3d::CameraRigMode::OnFootSprint;
    look.followYawRadians = bs3d::Pi;
    look.speedRatio = 1.0f;
    look.mouseDeltaX = 100.0f;
    look.mouseLookActive = true;
    const float manualYaw = rig.beginFrame(look, 0.016f);

    look.mouseDeltaX = 0.0f;
    const float beforeHelp = rig.beginFrame(look, 1.0f);
    expectNear(beforeHelp, manualYaw, 0.001f, "sprint auto-align waits long enough to avoid fighting mouse look");

    const float afterHelp = rig.beginFrame(look, 0.4f);
    expect(afterHelp < beforeHelp, "sprint auto-align helps after the comfort delay");
}

void cameraRigOnFootTrackingLagStaysTightAtSprintSpeed() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.velocity = {0.0f, 0.0f, 7.2f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootSprint;
    target.speedRatio = 1.0f;
    rig.reset(target);

    const float dt = 1.0f / 30.0f;
    bs3d::CameraRigState state = rig.state();
    for (int i = 0; i < 30; ++i) {
        target.position.z += target.velocity.z * dt;
        state = rig.update(target, dt, nullptr);
    }

    const float expectedTargetZ = target.position.z + 0.40f;
    const float expectedCameraZ = expectedTargetZ - std::cos(-0.34f) * 5.75f;
    expect(expectedTargetZ - state.target.z < 0.75f, "on-foot sprint camera target does not feel rubber-banded behind player");
    expect(expectedCameraZ - state.position.z < 0.95f, "on-foot sprint camera body follows tightly enough to keep controls readable");
}

void feelGateCameraExposesTargetLagForDebugOverlay() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.velocity = {0.0f, 0.0f, 7.2f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootSprint;
    target.speedRatio = 1.0f;
    rig.reset(target);

    const float dt = 1.0f / 30.0f;
    bs3d::CameraRigState state = rig.state();
    for (int i = 0; i < 12; ++i) {
        target.position.z += target.velocity.z * dt;
        state = rig.update(target, dt, nullptr);
    }

    expect(state.desiredTarget.z > state.target.z, "feel gate exposes desired target ahead of smoothed target");
    expect(bs3d::distanceXZ(state.desiredTarget, state.target) < 0.75f,
           "feel gate debug target lag remains inside readable threshold");
}

void cameraRigDoesNotSnapBoomJustBecauseTargetMovedWithCollisionProbe() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.velocity = {0.0f, 0.0f, 7.2f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootSprint;
    target.speedRatio = 1.0f;
    rig.reset(target);

    target.position.z += target.velocity.z * (1.0f / 30.0f);
    const bs3d::CameraRigState state = rig.update(target, 1.0f / 30.0f, &collision);

    expect(bs3d::distanceXZ(state.position, state.desiredPosition) > 0.02f,
           "non-blocking collision probe does not make fast target movement snap camera body to desired boom");
    expect(state.fullBoomLength - state.boomLength < 0.05f,
           "non-blocking collision probe keeps the camera at full boom while following smoothly");
}

void feelGateCameraBoomReturnsOutSmoothlyAfterWallClears() {
    bs3d::WorldCollision collision;
    collision.addBox({0.0f, 1.5f, -2.0f}, {8.0f, 3.0f, 0.5f});

    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootNormal;
    rig.reset(target);

    const bs3d::CameraRigState blocked = rig.update(target, 0.016f, &collision);
    const bs3d::CameraRigState oneFrameOpen = rig.update(target, 1.0f / 30.0f, nullptr);

    expect(oneFrameOpen.boomLength > blocked.boomLength, "feel gate camera starts returning outward when wall clears");
    expect(oneFrameOpen.boomLength < oneFrameOpen.fullBoomLength,
           "feel gate camera does not snap fully outward in one frame after wall clears");

    bs3d::CameraRigState recovered = oneFrameOpen;
    for (int i = 0; i < 45; ++i) {
        recovered = rig.update(target, 1.0f / 30.0f, nullptr);
    }
    expect(recovered.fullBoomLength - recovered.boomLength < 0.12f,
           "feel gate camera returns to full boom after a short clear period");
}

void cameraRigShortensBoomAgainstWorldCollision() {
    bs3d::WorldCollision collision;
    collision.addBox({0.0f, 1.5f, -3.0f}, {8.0f, 3.0f, 0.6f});

    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::Walking;
    rig.reset(target);

    const bs3d::CameraRigState blocked = rig.update(target, 0.5f, &collision);
    const bs3d::CameraRigState open = rig.update(target, 0.5f, nullptr);

    expect(blocked.boomLength < open.boomLength, "camera boom shortens when wall is behind target");
    expect(blocked.position.z > open.position.z, "resolved camera stays in front of blocking wall");
}

void cameraRigDoesNotRemainOccludedWhenBoomObstacleAppears() {
    bs3d::CameraRig rig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::Walking;
    rig.reset(target);
    const bs3d::CameraRigState open = rig.update(target, 0.016f, nullptr);

    bs3d::WorldCollision collision;
    collision.addBox({0.0f, 1.5f, -2.0f}, {8.0f, 3.0f, 0.5f});
    const bs3d::CameraRigState blocked = rig.update(target, 0.016f, &collision);

    expect(blocked.boomLength < open.boomLength - 1.5f, "camera snaps inward when a wall suddenly occludes boom");
    expect(blocked.position.z > -1.6f, "camera resolves in front of the sudden wall");
}

void cameraBoomCanResolveVeryCloseObstacles() {
    bs3d::WorldCollision collision;
    collision.addBox({0.0f, 1.35f, -0.35f}, {6.0f, 2.5f, 0.18f});

    const bs3d::Vec3 target{0.0f, 1.35f, 0.0f};
    const bs3d::Vec3 desired{0.0f, 1.8f, -4.0f};
    const bs3d::Vec3 resolved = collision.resolveCameraBoom(target, desired, 0.22f);

    expect(resolved.z > -0.12f, "camera boom can move almost to target when obstacle is very close");
}

void cameraRigTensionMakesSprintCameraFeelPanicked() {
    bs3d::CameraRig calm;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.velocity = {0.0f, 0.0f, 7.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootSprint;
    target.speedRatio = 1.0f;
    calm.reset(target);
    const bs3d::CameraRigState calmState = calm.update(target, 0.016f);

    bs3d::CameraRig tense;
    target.tension = 1.0f;
    tense.reset(target);
    const bs3d::CameraRigState tenseState = tense.update(target, 0.016f);

    expect(tenseState.fovY > calmState.fovY, "high przypal raises sprint fov");
    expect(tenseState.boomLength >= calmState.boomLength, "panic sprint keeps or widens escape framing");
}

void cameraRigInteractionModeFramesCloserThanNormal() {
    bs3d::CameraRig normal;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootNormal;
    normal.reset(target);
    const bs3d::CameraRigState normalState = normal.update(target, 0.016f);

    bs3d::CameraRig interaction;
    target.mode = bs3d::CameraRigMode::Interaction;
    interaction.reset(target);
    const bs3d::CameraRigState interactionState = interaction.update(target, 0.016f);

    expect(interactionState.boomLength < normalState.boomLength, "interaction camera comes closer for quick dialogue");
    expect(interactionState.fovY < normalState.fovY, "interaction camera uses mild zoom");
}

void feedbackEventsDecayOverTime() {
    bs3d::GameFeedback feedback;
    feedback.trigger(bs3d::FeedbackEvent::MissionComplete);
    feedback.setWorldTension(0.75f);

    expect(feedback.flashAlpha() > 0.0f, "mission complete starts flash");
    expect(feedback.hudPulse() > 0.0f, "mission complete starts hud pulse");
    expectNear(feedback.worldTension(), 0.75f, 0.001f, "world tension is stored");
    expectNear(feedback.chaseIntensity(), 0.75f, 0.001f, "temporary chase intensity alias matches world tension");

    feedback.update(5.0f);
    expectNear(feedback.flashAlpha(), 0.0f, 0.001f, "flash expires");
    expectNear(feedback.hudPulse(), 0.0f, 0.001f, "hud pulse expires");
    expect(feedback.worldTension() < 0.75f, "world tension decays");
}

void feedbackCameraTensionIgnoresPrzypalHeatDuringFeelGate() {
    expectNear(bs3d::resolveCameraWorldTension(0.0f, 1.0f),
               0.0f,
               0.001f,
               "heat alone does not feed camera tension during feel gate");
    expectNear(bs3d::resolveCameraWorldTension(0.42f, 1.0f),
               0.42f,
               0.001f,
               "chase pressure remains the only camera tension source");
    expectNear(bs3d::resolveCameraWorldTension(1.4f, 0.0f),
               1.0f,
               0.001f,
               "camera tension stays clamped");
}

void feedbackPrzypalNoticePulsesHudWithoutCameraShake() {
    bs3d::GameFeedback feedback;
    feedback.trigger(bs3d::FeedbackEvent::PrzypalNotice);

    expectNear(feedback.screenShake(), 0.0f, 0.001f, "przypal notice does not move the camera");
    expect(feedback.hudPulse() > 0.0f, "przypal notice still pulses HUD");
}

void feedbackShortComedyShakeAfterLongShakeStartsAtFullStrength() {
    bs3d::GameFeedback feedback;
    feedback.trigger(bs3d::FeedbackEvent::MissionFailed);
    feedback.update(0.60f);

    expect(feedback.triggerComedyEvent(0.0f), "comedy event can start after mission shake decays");

    expectNear(feedback.screenShake(), 1.0f, 0.001f, "new shorter shake gets its own duration baseline");
}

void stableCameraFeedbackDisablesNonChaseCameraEffects() {
    bs3d::CameraFeedbackInput input;
    input.stableCameraMode = true;
    input.chaseActive = false;
    input.screenShake = 1.0f;
    input.comedyZoom = 0.8f;
    input.worldTension = 0.6f;
    input.playerInVehicle = true;
    input.vehicleInstability = 1.0f;
    input.vehicleBoostActive = true;
    input.hornPulse = 1.0f;

    const bs3d::CameraFeedbackOutput output = bs3d::resolveCameraFeedback(input);

    expectNear(output.screenShake, 0.0f, 0.001f, "stable camera suppresses shake");
    expectNear(output.comedyZoom, 0.0f, 0.001f, "stable camera suppresses comedy zoom");
    expectNear(output.vehicleShake, 0.0f, 0.001f, "stable camera suppresses vehicle wobble");
    expectNear(output.cameraTension, 0.0f, 0.001f, "stable camera ignores non-chase tension");

    input.chaseActive = true;
    const bs3d::CameraFeedbackOutput chaseOutput = bs3d::resolveCameraFeedback(input);
    expectNear(chaseOutput.cameraTension, 0.0f, 0.001f, "stable camera suppresses chase tension");
}

void feedbackComedyEventsCooldownAndExpire() {
    bs3d::GameFeedback feedback;
    expect(feedback.triggerComedyEvent(0.8f), "first comedy event triggers");
    expect(feedback.comedyZoom() > 0.0f, "comedy event starts zoom");
    expect(feedback.comedyFreeze() > 0.0f, "comedy event starts freeze pulse");
    expect(!feedback.triggerComedyEvent(1.0f), "cooldown prevents spammy event cam");

    feedback.update(2.0f);
    expectNear(feedback.comedyZoom(), 0.0f, 0.001f, "comedy zoom expires");
    expectNear(feedback.comedyFreeze(), 0.0f, 0.001f, "comedy freeze expires");
    expect(feedback.triggerComedyEvent(0.5f), "event can trigger again after cooldown");
}

void worldEventLedgerStoresMergesAndExpiresEvents() {
    bs3d::WorldEventLedger ledger;

    bs3d::WorldEvent event;
    event.type = bs3d::WorldEventType::PublicNuisance;
    event.location = bs3d::WorldLocationTag::Block;
    event.position = {1.0f, 0.0f, 2.0f};
    event.intensity = 0.75f;
    event.remainingSeconds = 1.0f;
    event.source = "horn";

    const bs3d::WorldEventAddResult first = ledger.add(event);
    expect(first.addedToLedger, "first world event is accepted into memory");
    expect(first.heatPulseAccepted, "first world event produces a Przypal pulse");
    expect(!first.merged, "first world event is stored as a new memory");
    expectNear(first.heatMultiplier, 1.0f, 0.001f, "first world event has full heat multiplier");
    expect(ledger.recentEvents().size() == 1, "ledger stores active event memory");

    ledger.update(0.5f);
    expect(ledger.recentEvents().size() == 1, "ledger keeps event before expiry");

    ledger.update(0.6f);
    expect(ledger.recentEvents().empty(), "ledger expires event memory after remaining time");
}

void activeWorldEventDoesNotAddPrzypalAgainWithoutNewPulse() {
    bs3d::WorldEventLedger ledger;
    bs3d::PrzypalSystem przypal;

    bs3d::WorldEvent event;
    event.type = bs3d::WorldEventType::PropertyDamage;
    event.location = bs3d::WorldLocationTag::Parking;
    event.position = {0.0f, 0.0f, 0.0f};
    event.intensity = 1.0f;
    event.remainingSeconds = 10.0f;
    event.source = "vehicle_impact";

    const bs3d::WorldEventAddResult pulse = ledger.add(event);
    przypal.onWorldEvent(pulse);
    const float afterPulse = przypal.value();

    for (int i = 0; i < 20; ++i) {
        ledger.update(0.05f);
        przypal.update(0.05f);
    }

    expect(przypal.value() <= afterPulse + 0.001f,
           "active event memory does not pump Przypal without a new add result");
}

void worldEventLedgerMergesNearbyRepeatsWithDampenedHeatAndStackCap() {
    bs3d::WorldEventLedger ledger;

    bs3d::WorldEvent event;
    event.type = bs3d::WorldEventType::PublicNuisance;
    event.location = bs3d::WorldLocationTag::Block;
    event.position = {0.0f, 0.0f, 0.0f};
    event.intensity = 1.0f;
    event.remainingSeconds = 18.0f;
    event.source = "horn";

    const bs3d::WorldEventAddResult first = ledger.add(event);
    expect(first.addedToLedger && first.heatPulseAccepted && !first.merged, "first nuisance creates a fresh event");

    event.position = {1.0f, 0.0f, 1.0f};
    const bs3d::WorldEventAddResult second = ledger.add(event);
    expect(second.addedToLedger && second.heatPulseAccepted && second.merged, "nearby same-source nuisance merges");
    expect(second.newStackCount == 2, "merged event increments stack count");
    expectNear(second.heatMultiplier, 0.45f, 0.001f, "second stack uses dampened heat");

    const bs3d::WorldEventAddResult third = ledger.add(event);
    expect(third.addedToLedger && third.heatPulseAccepted && third.merged, "third nearby event still merges");
    expect(third.newStackCount == 3, "third event reaches stack three");
    expectNear(third.heatMultiplier, 0.25f, 0.001f, "stack three uses stronger heat dampening");

    ledger.add(event);
    ledger.add(event);
    const bs3d::WorldEventAddResult capped = ledger.add(event);
    expect(capped.merged, "repeats after stack cap still refresh memory");
    expect(capped.addedToLedger, "stack cap still refreshes ledger memory");
    expect(!capped.heatPulseAccepted, "stack cap prevents infinite heat pumping");
    expect(capped.newStackCount == 5, "stack count is capped at five");
    expectNear(capped.heatMultiplier, 0.0f, 0.001f, "capped merge has no heat multiplier");
    expect(ledger.recentEvents().front().stackCount == 5, "stored event stack is capped at five");
}

void worldEventLedgerQueriesByTypeLocationAndRadius() {
    bs3d::WorldEventLedger ledger;

    bs3d::WorldEvent shop;
    shop.type = bs3d::WorldEventType::ShopTrouble;
    shop.location = bs3d::WorldLocationTag::Shop;
    shop.position = {10.0f, 0.0f, 0.0f};
    shop.remainingSeconds = 10.0f;
    shop.source = "shop";
    ledger.add(shop);

    bs3d::WorldEvent parking = shop;
    parking.type = bs3d::WorldEventType::PropertyDamage;
    parking.location = bs3d::WorldLocationTag::Parking;
    parking.position = {0.0f, 0.0f, 0.0f};
    parking.source = "vehicle_impact";
    ledger.add(parking);

    const std::vector<bs3d::WorldEvent> nearbyDamage =
        ledger.query(bs3d::WorldEventType::PropertyDamage, bs3d::WorldLocationTag::Parking, {0.5f, 0.0f, 0.0f}, 3.0f);
    expect(nearbyDamage.size() == 1, "query returns matching nearby damage event");

    const std::vector<bs3d::WorldEvent> wrongLocation =
        ledger.query(bs3d::WorldEventType::PropertyDamage, bs3d::WorldLocationTag::Shop, {0.5f, 0.0f, 0.0f}, 3.0f);
    expect(wrongLocation.empty(), "query filters by location");

    const std::vector<bs3d::WorldEvent> tooFar =
        ledger.query(bs3d::WorldEventType::ShopTrouble, bs3d::WorldLocationTag::Shop, {0.0f, 0.0f, 0.0f}, 3.0f);
    expect(tooFar.empty(), "query filters by radius");
}

void worldEventLedgerKeepsRewirMemoryLocationsSeparate() {
    bs3d::WorldEventLedger ledger;

    bs3d::WorldEvent garage;
    garage.type = bs3d::WorldEventType::PropertyDamage;
    garage.location = bs3d::WorldLocationTag::Garage;
    garage.position = {-18.0f, 0.0f, 23.0f};
    garage.remainingSeconds = 10.0f;
    garage.source = "garage_bump";
    ledger.add(garage);

    bs3d::WorldEvent trash = garage;
    trash.location = bs3d::WorldLocationTag::Trash;
    trash.position = {9.0f, 0.0f, -4.0f};
    trash.source = "trash_bump";
    ledger.add(trash);

    const std::vector<bs3d::WorldEvent> garageHits =
        ledger.query(bs3d::WorldEventType::PropertyDamage, bs3d::WorldLocationTag::Garage, {-18.1f, 0.0f, 23.0f}, 2.0f);
    expect(garageHits.size() == 1, "garage memory can be queried separately from parking");

    const std::vector<bs3d::WorldEvent> trashHits =
        ledger.query(bs3d::WorldEventType::PropertyDamage, bs3d::WorldLocationTag::Trash, {9.1f, 0.0f, -4.0f}, 2.0f);
    expect(trashHits.size() == 1, "trash memory can be queried separately from road loop");

    const std::vector<bs3d::WorldEvent> parkingHits =
        ledger.query(bs3d::WorldEventType::PropertyDamage, bs3d::WorldLocationTag::Parking, {-18.1f, 0.0f, 23.0f}, 2.0f);
    expect(parkingHits.empty(), "garage damage does not pollute parking memory");
}

void worldEventLedgerQueriesActiveMemoryByLocationAndSource() {
    bs3d::WorldEventLedger ledger;

    bs3d::WorldEvent firstShopVisit;
    firstShopVisit.type = bs3d::WorldEventType::ShopTrouble;
    firstShopVisit.location = bs3d::WorldLocationTag::Shop;
    firstShopVisit.position = {18.0f, 0.0f, -22.0f};
    firstShopVisit.remainingSeconds = 8.0f;
    firstShopVisit.source = "shop_walk_intro";
    ledger.add(firstShopVisit);

    bs3d::WorldEvent laterShopDamage = firstShopVisit;
    laterShopDamage.type = bs3d::WorldEventType::PropertyDamage;
    laterShopDamage.source = "shop_prop_hit";
    ledger.add(laterShopDamage);

    bs3d::WorldEvent garageTrouble = firstShopVisit;
    garageTrouble.location = bs3d::WorldLocationTag::Garage;
    garageTrouble.position = {-18.0f, 0.0f, 23.0f};
    garageTrouble.source = "shop_walk_intro";
    ledger.add(garageTrouble);

    const std::vector<bs3d::WorldEvent> shopIntroMemory =
        ledger.queryByLocationAndSource(bs3d::WorldLocationTag::Shop, "shop_walk_intro");
    expect(shopIntroMemory.size() == 1, "query finds active memory for one source in one rewir");
    expect(shopIntroMemory.front().type == bs3d::WorldEventType::ShopTrouble,
           "query returns the matching event instead of another source");

    const std::vector<bs3d::WorldEvent> shopDamageMemory =
        ledger.queryByLocationAndSource(bs3d::WorldLocationTag::Shop, "shop_prop_hit");
    expect(shopDamageMemory.size() == 1, "query can find a second source in the same rewir");

    const std::vector<bs3d::WorldEvent> missingShopMemory =
        ledger.queryByLocationAndSource(bs3d::WorldLocationTag::Shop, "missing_source");
    expect(missingShopMemory.empty(), "query filters out unknown sources");

    ledger.update(8.1f);
    const std::vector<bs3d::WorldEvent> expiredMemory =
        ledger.queryByLocationAndSource(bs3d::WorldLocationTag::Shop, "shop_walk_intro");
    expect(expiredMemory.empty(), "expired source memory is not returned");
}

void worldEventLedgerCountsActiveMemoryByRewir() {
    bs3d::WorldEventLedger ledger;

    bs3d::WorldEvent shop;
    shop.type = bs3d::WorldEventType::ShopTrouble;
    shop.location = bs3d::WorldLocationTag::Shop;
    shop.position = {18.0f, 0.0f, -22.0f};
    shop.remainingSeconds = 6.0f;
    shop.source = "shop_walk_intro";
    ledger.add(shop);

    bs3d::WorldEvent garage = shop;
    garage.location = bs3d::WorldLocationTag::Garage;
    garage.position = {-18.0f, 0.0f, 23.0f};
    garage.remainingSeconds = 10.0f;
    garage.source = "garage_bump";
    ledger.add(garage);

    bs3d::WorldEvent trash = shop;
    trash.location = bs3d::WorldLocationTag::Trash;
    trash.position = {9.0f, 0.0f, -4.0f};
    trash.remainingSeconds = 10.0f;
    trash.source = "trash_bump";
    ledger.add(trash);

    const bs3d::WorldEventLocationCounts counts = ledger.locationCounts();
    expect(counts.shop == 1, "location counts include active shop memory");
    expect(counts.garage == 1, "location counts include active garage memory");
    expect(counts.trash == 1, "location counts include active trash memory");
    expect(counts.block == 0, "location counts leave quiet block at zero");
    expect(counts.parking == 0, "location counts leave quiet parking at zero");
    expect(counts.roadLoop == 0, "location counts leave quiet road loop at zero");

    ledger.update(6.1f);

    const bs3d::WorldEventLocationCounts cooledCounts = ledger.locationCounts();
    expect(cooledCounts.shop == 0, "location counts drop expired shop memory");
    expect(cooledCounts.garage == 1, "location counts keep active garage memory after shop cools");
    expect(cooledCounts.trash == 1, "location counts keep active trash memory after shop cools");
}

void worldEventLedgerBuildsStrongestActiveMemoryHotspotsByRewir() {
    bs3d::WorldEventLedger ledger;

    bs3d::WorldEvent quietParking;
    quietParking.type = bs3d::WorldEventType::PropertyDamage;
    quietParking.location = bs3d::WorldLocationTag::Parking;
    quietParking.position = {-7.0f, 0.0f, 8.6f};
    quietParking.intensity = 0.5f;
    quietParking.remainingSeconds = 12.0f;
    quietParking.source = "parking_scrape";
    ledger.add(quietParking);

    bs3d::WorldEvent loudParking = quietParking;
    loudParking.position = {-4.0f, 0.0f, 9.0f};
    loudParking.intensity = 2.0f;
    loudParking.remainingSeconds = 4.0f;
    loudParking.source = "parking_crash";
    const bs3d::WorldEventAddResult loudParkingResult = ledger.add(loudParking);

    bs3d::WorldEvent garage = quietParking;
    garage.location = bs3d::WorldLocationTag::Garage;
    garage.position = {-18.0f, 0.0f, 23.0f};
    garage.intensity = 1.0f;
    garage.remainingSeconds = 12.0f;
    garage.source = "garage_bump";
    const bs3d::WorldEventAddResult garageResult = ledger.add(garage);

    const std::vector<bs3d::WorldEventHotspot> hotspots = ledger.memoryHotspots();
    expect(hotspots.size() == 2, "memory hotspots include only active rewir memories");
    expect(hotspots[0].location == bs3d::WorldLocationTag::Parking,
           "memory hotspots are emitted in stable rewir order");
    expect(hotspots[0].eventId == loudParkingResult.eventId,
           "parking hotspot picks strongest active memory in that rewir");
    expect(hotspots[0].source == "parking_crash", "hotspot preserves the selected memory source");
    expectNear(hotspots[0].position.x, -4.0f, 0.001f, "hotspot exposes selected memory position");
    expect(hotspots[1].location == bs3d::WorldLocationTag::Garage, "second hotspot is garage memory");
    expect(hotspots[1].eventId == garageResult.eventId, "garage hotspot preserves event identity");

    ledger.update(4.1f);

    const std::vector<bs3d::WorldEventHotspot> cooledHotspots = ledger.memoryHotspots();
    expect(cooledHotspots.size() == 2, "expired strongest memory falls back to remaining active rewir memory");
    expect(cooledHotspots[0].location == bs3d::WorldLocationTag::Parking,
           "parking hotspot remains while quieter parking memory is active");
    expect(cooledHotspots[0].source == "parking_scrape", "hotspot falls back after strongest event expires");
}

void rewirPressureUsesNearbyStrongMemoryHotspot() {
    bs3d::WorldEventLedger ledger;

    bs3d::WorldEvent parking;
    parking.type = bs3d::WorldEventType::PropertyDamage;
    parking.location = bs3d::WorldLocationTag::Parking;
    parking.position = {-7.0f, 0.0f, 8.6f};
    parking.intensity = 1.4f;
    parking.remainingSeconds = 18.0f;
    parking.source = "parking_crash";
    const bs3d::WorldEventAddResult addResult = ledger.add(parking);
    ledger.update(5.0f);

    const bs3d::RewirPressureSnapshot pressure =
        bs3d::resolveRewirPressure(ledger, {-6.0f, 0.0f, 8.0f});

    expect(pressure.active, "nearby strong rewir memory creates local pressure");
    expect(pressure.location == bs3d::WorldLocationTag::Parking, "pressure keeps the active rewir location");
    expect(pressure.level == bs3d::RewirPressureLevel::Watchful,
           "single remembered parking crash makes the rewir watchful");
    expect(pressure.patrolInterest, "watchful rewir pressure can feed future patrol routing");
    expect(pressure.source == "parking_crash", "pressure preserves the memory source for debug");
    expect(pressure.eventId == addResult.eventId, "pressure preserves memory event id for follow-up logic");
    expect(pressure.distanceToPlayer < 2.0f, "pressure reports distance to the remembered spot");
}

void rewirPressureIgnoresWeakRemoteAndShopMemory() {
    bs3d::WorldEventLedger weakLedger;
    bs3d::WorldEvent weakRoad;
    weakRoad.type = bs3d::WorldEventType::PublicNuisance;
    weakRoad.location = bs3d::WorldLocationTag::RoadLoop;
    weakRoad.position = {0.0f, 0.0f, 0.0f};
    weakRoad.intensity = 0.25f;
    weakRoad.remainingSeconds = 18.0f;
    weakRoad.source = "old_noise";
    weakLedger.add(weakRoad);
    weakLedger.update(5.0f);

    const bs3d::RewirPressureSnapshot weakPressure =
        bs3d::resolveRewirPressure(weakLedger, {0.0f, 0.0f, 0.0f});
    expect(!weakPressure.active, "weak old road noise does not create local pressure");

    bs3d::WorldEventLedger remoteLedger;
    bs3d::WorldEvent garage;
    garage.type = bs3d::WorldEventType::PropertyDamage;
    garage.location = bs3d::WorldLocationTag::Garage;
    garage.position = {-18.0f, 0.0f, 23.0f};
    garage.intensity = 2.0f;
    garage.remainingSeconds = 18.0f;
    garage.source = "garage_smash";
    remoteLedger.add(garage);

    const bs3d::RewirPressureSnapshot remotePressure =
        bs3d::resolveRewirPressure(remoteLedger, {25.0f, 0.0f, -25.0f});
    expect(!remotePressure.active, "distant rewir memory does not pressure the player elsewhere");

    bs3d::WorldEventLedger shopLedger;
    bs3d::WorldEvent shop;
    shop.type = bs3d::WorldEventType::ShopTrouble;
    shop.location = bs3d::WorldLocationTag::Shop;
    shop.position = {18.0f, 0.0f, -22.0f};
    shop.intensity = 3.0f;
    shop.remainingSeconds = 18.0f;
    shop.source = "paragon_chaos";
    shopLedger.add(shop);

    const bs3d::RewirPressureSnapshot shopPressure =
        bs3d::resolveRewirPressure(shopLedger, {18.0f, 0.0f, -22.0f});
    expect(!shopPressure.active, "shop memory stays in shop service logic, not generic rewir pressure");
}

void shopServiceStateReflectsWorldMemoryAndStoryBan() {
    bs3d::WorldEventLedger ledger;

    bs3d::ShopServiceState calm = bs3d::resolveShopServiceState(ledger, false);
    expect(calm.access == bs3d::ShopServiceAccess::Normal, "quiet shop starts in normal service mode");
    expect(calm.prompt == "E: pogadaj z Zenonem", "normal shop service prompt is readable");

    bs3d::WorldEvent shopVisit;
    shopVisit.type = bs3d::WorldEventType::ShopTrouble;
    shopVisit.location = bs3d::WorldLocationTag::Shop;
    shopVisit.position = {18.0f, 0.0f, -22.0f};
    shopVisit.remainingSeconds = 8.0f;
    shopVisit.source = "shop_walk_intro";
    ledger.add(shopVisit);

    bs3d::ShopServiceState wary = bs3d::resolveShopServiceState(ledger, false);
    expect(wary.access == bs3d::ShopServiceAccess::Wary, "recent shop trouble makes Zenon wary");
    expect(wary.prompt == "E: Zenon patrzy spod lady", "wary shop service is visible in prompt text");

    ledger.update(8.1f);
    bs3d::ShopServiceState cooled = bs3d::resolveShopServiceState(ledger, false);
    expect(cooled.access == bs3d::ShopServiceAccess::Normal, "wary service cools down when shop memory expires");

    bs3d::WorldEvent chaos;
    chaos.type = bs3d::WorldEventType::ShopTrouble;
    chaos.location = bs3d::WorldLocationTag::Shop;
    chaos.position = {18.0f, 0.0f, -22.0f};
    chaos.remainingSeconds = 8.0f;
    chaos.source = "paragon_chaos";
    ledger.add(chaos);

    bs3d::ShopServiceState windowOnly = bs3d::resolveShopServiceState(ledger, false);
    expect(windowOnly.access == bs3d::ShopServiceAccess::WindowOnly,
           "chaotic paragon memory moves shop to window-only service");
    expect(windowOnly.prompt == "E: Zenon przez okienko", "window-only service is visible in prompt text");

    ledger.clear();
    bs3d::ShopServiceState storyBan = bs3d::resolveShopServiceState(ledger, true);
    expect(storyBan.access == bs3d::ShopServiceAccess::WindowOnly,
           "persistent story ban wins even after active memory cools down");
}

void garageServiceStateReflectsLocalRewirPressure() {
    bs3d::WorldEventLedger ledger;

    bs3d::GarageServiceState calm =
        bs3d::resolveGarageServiceState(ledger, {-18.0f, 0.0f, 23.0f});
    expect(calm.access == bs3d::GarageServiceAccess::Normal, "quiet garage starts in normal service mode");
    expect(calm.prompt == "E: pogadaj z Ryskiem", "normal garage service prompt is readable");

    bs3d::WorldEvent garage;
    garage.type = bs3d::WorldEventType::PropertyDamage;
    garage.location = bs3d::WorldLocationTag::Garage;
    garage.position = {-18.0f, 0.0f, 23.0f};
    garage.intensity = 1.4f;
    garage.remainingSeconds = 18.0f;
    garage.source = "garage_pressure_memory";
    ledger.add(garage);
    ledger.update(5.0f);

    bs3d::GarageServiceState wary =
        bs3d::resolveGarageServiceState(ledger, {-18.0f, 0.0f, 23.0f});
    expect(wary.access == bs3d::GarageServiceAccess::Wary, "hot garage pressure makes Rysiek wary");
    expect(wary.prompt == "E: Rysiek liczy rysy", "wary garage service is visible in prompt text");

    ledger.update(13.1f);
    bs3d::GarageServiceState cooled =
        bs3d::resolveGarageServiceState(ledger, {-18.0f, 0.0f, 23.0f});
    expect(cooled.access == bs3d::GarageServiceAccess::Normal, "garage service cools down when memory expires");
}

void caretakerServiceStateReflectsTrashRewirPressure() {
    bs3d::WorldEventLedger ledger;

    bs3d::CaretakerServiceState calm =
        bs3d::resolveCaretakerServiceState(ledger, {9.0f, 0.0f, -4.0f});
    expect(calm.access == bs3d::CaretakerServiceAccess::Normal,
           "quiet trash area starts in normal caretaker mode");
    expect(calm.prompt == "E: pogadaj z Dozorca", "normal caretaker service prompt is readable");

    bs3d::WorldEvent trash;
    trash.type = bs3d::WorldEventType::PropertyDamage;
    trash.location = bs3d::WorldLocationTag::Trash;
    trash.position = {9.0f, 0.0f, -4.0f};
    trash.intensity = 1.4f;
    trash.remainingSeconds = 18.0f;
    trash.source = "trash_pressure_memory";
    ledger.add(trash);
    ledger.update(5.0f);

    bs3d::CaretakerServiceState wary =
        bs3d::resolveCaretakerServiceState(ledger, {9.0f, 0.0f, -4.0f});
    expect(wary.access == bs3d::CaretakerServiceAccess::Wary,
           "hot trash pressure makes the caretaker wary");
    expect(wary.prompt == "E: Dozorca zamyka altane",
           "wary caretaker service is visible in prompt text");

    ledger.update(13.1f);
    bs3d::CaretakerServiceState cooled =
        bs3d::resolveCaretakerServiceState(ledger, {9.0f, 0.0f, -4.0f});
    expect(cooled.access == bs3d::CaretakerServiceAccess::Normal,
           "caretaker service cools down when trash memory expires");
}

void localRewirServiceCatalogDescribesPressureDrivenCivilServices() {
    const std::vector<bs3d::LocalRewirServiceSpec>& catalog = bs3d::localRewirServiceCatalog();

    const bs3d::LocalRewirServiceSpec* garage = nullptr;
    const bs3d::LocalRewirServiceSpec* trash = nullptr;
    const bs3d::LocalRewirServiceSpec* parking = nullptr;
    const bs3d::LocalRewirServiceSpec* road = nullptr;
    for (const bs3d::LocalRewirServiceSpec& spec : catalog) {
        if (spec.interactionPointId == "garage_rysiek") {
            garage = &spec;
        }
        if (spec.interactionPointId == "trash_dozorca") {
            trash = &spec;
        }
        if (spec.interactionPointId == "parking_parkingowy") {
            parking = &spec;
        }
        if (spec.interactionPointId == "road_kierowca") {
            road = &spec;
        }
    }

    expect(garage != nullptr, "local service catalog includes Rysiek's garage service");
    expect(trash != nullptr, "local service catalog includes Dozorca's trash service");
    expect(parking != nullptr, "local service catalog includes Parkingowy's parking service");
    expect(road != nullptr, "local service catalog includes Kierowca's road-loop service");
    if (garage != nullptr) {
        expect(garage->interactionId == "garage_service_rysiek",
               "garage service catalog maps point id to runtime interaction id");
        expect(garage->location == bs3d::WorldLocationTag::Garage,
               "garage service catalog is tied to garage pressure");
        expect(bs3d::distanceXZ(garage->interactionPosition, {-18.0f, 0.0f, 23.0f}) < 0.8f,
               "garage service catalog owns its authored interaction position");
        expect(garage->interactionRadius >= 2.4f,
               "garage service catalog owns a reachable interaction radius");
        expect(garage->speaker == "Rysiek", "garage service catalog keeps dialogue speaker");
    }
    if (trash != nullptr) {
        expect(trash->interactionId == "trash_service_dozorca",
               "trash service catalog maps point id to runtime interaction id");
        expect(trash->location == bs3d::WorldLocationTag::Trash,
               "trash service catalog is tied to trash pressure");
        expect(bs3d::distanceXZ(trash->interactionPosition, {9.0f, 0.0f, -4.0f}) < 0.8f,
               "trash service catalog owns its authored interaction position");
        expect(trash->interactionRadius >= 2.4f,
               "trash service catalog owns a reachable interaction radius");
        expect(trash->speaker == "Dozorca", "trash service catalog keeps dialogue speaker");
    }
    if (parking != nullptr) {
        expect(parking->interactionId == "parking_service_parkingowy",
               "parking service catalog maps point id to runtime interaction id");
        expect(parking->location == bs3d::WorldLocationTag::Parking,
               "parking service catalog is tied to parking pressure");
        expect(bs3d::distanceXZ(parking->interactionPosition, {-7.0f, 0.0f, 8.6f}) < 0.8f,
               "parking service catalog owns its authored interaction position");
        expect(parking->interactionRadius >= 2.4f,
               "parking service catalog owns a reachable interaction radius");
        expect(parking->speaker == "Parkingowy", "parking service catalog keeps dialogue speaker");
    }
    if (road != nullptr) {
        expect(road->interactionId == "road_service_kierowca",
               "road-loop service catalog maps point id to runtime interaction id");
        expect(road->location == bs3d::WorldLocationTag::RoadLoop,
               "road-loop service catalog is tied to road-loop pressure");
        expect(bs3d::distanceXZ(road->interactionPosition, {8.0f, 0.0f, -18.0f}) < 0.8f,
               "road-loop service catalog owns its authored interaction position");
        expect(road->interactionRadius >= 2.4f,
               "road-loop service catalog owns a reachable interaction radius");
        expect(road->speaker == "Kierowca", "road-loop service catalog keeps dialogue speaker");
    }
}

void localRewirServiceCatalogCoversEveryPressureEnabledRewir() {
    const std::vector<bs3d::WorldLocationTag>& pressureLocations = bs3d::rewirPressureLocations();
    const std::vector<bs3d::LocalRewirServiceSpec>& catalog = bs3d::localRewirServiceCatalog();

    expect(!pressureLocations.empty(), "rewir pressure exposes the locations that can create local pressure");

    for (bs3d::WorldLocationTag pressureLocation : pressureLocations) {
        bool covered = false;
        for (const bs3d::LocalRewirServiceSpec& service : catalog) {
            covered = covered || service.location == pressureLocation;
        }

        expect(covered,
               std::string("local service catalog covers pressure rewir: ") +
                   bs3d::rewirPressureLocationLabel(pressureLocation));
    }

    for (const bs3d::LocalRewirServiceSpec& service : catalog) {
        bool pressureEnabled = false;
        for (bs3d::WorldLocationTag pressureLocation : pressureLocations) {
            pressureEnabled = pressureEnabled || service.location == pressureLocation;
        }

        expect(pressureEnabled,
               "local service catalog only binds pressure-enabled rewirs: " + service.interactionPointId);
    }
}

void localRewirFavorCatalogCoversEveryLocalService() {
    const std::vector<bs3d::LocalRewirServiceSpec>& services = bs3d::localRewirServiceCatalog();
    const std::vector<bs3d::LocalRewirFavorSpec>& favors = bs3d::localRewirFavorCatalog();

    expect(!favors.empty(), "local rewir favor catalog has authored mini-task specs");

    for (const bs3d::LocalRewirServiceSpec& service : services) {
        const bs3d::LocalRewirFavorSpec* favor = nullptr;
        for (const bs3d::LocalRewirFavorSpec& candidate : favors) {
            if (candidate.serviceInteractionId == service.interactionId) {
                favor = &candidate;
            }
        }

        expect(favor != nullptr, "every local service has a rewir favor spec: " + service.interactionId);
        if (favor == nullptr) {
            continue;
        }

        expect(favor->location == service.location, "rewir favor stays in its service location");
        expect(!favor->id.empty(), "rewir favor has stable id");
        expect(!favor->interactionPointId.empty(), "rewir favor has future interaction point id");
        expect(!favor->prompt.empty(), "rewir favor has an interaction prompt");
        expect(!favor->startLine.empty(), "rewir favor has start dialogue");
        expect(!favor->completeLine.empty(), "rewir favor has completion dialogue");
        expect(favor->interactionRadius >= 1.8f, "rewir favor target is reachable");
        expect(bs3d::distanceXZ(favor->interactionPosition, service.interactionPosition) <= 8.0f,
               "rewir favor target stays near its service anchor");
    }

    const std::optional<bs3d::LocalRewirFavorSpec> garageFavor =
        bs3d::localRewirFavorForService("garage_service_rysiek");
    expect(garageFavor.has_value(), "garage service resolves its favor spec");
    if (garageFavor.has_value()) {
        expect(garageFavor->id == "garage_favor_rysiek", "garage favor id is stable");
        expect(garageFavor->prompt == "E: przetrzyj rysy", "garage favor prompt is authored");
    }

    const std::optional<bs3d::LocalRewirFavorSpec> garageFavorByPoint =
        bs3d::localRewirFavorForPoint("garage_favor_rysiek_point");
    expect(garageFavorByPoint.has_value(), "garage favor resolves from its interaction point id");
    if (garageFavorByPoint.has_value()) {
        expect(garageFavorByPoint->serviceInteractionId == "garage_service_rysiek",
               "garage favor keeps its service interaction id");
    }

    const std::optional<bs3d::LocalRewirFavorSpec> garageFavorById =
        bs3d::localRewirFavorById("garage_favor_rysiek");
    expect(garageFavorById.has_value(), "garage favor resolves from its runtime favor id");
    if (garageFavorById.has_value()) {
        expect(garageFavorById->interactionPointId == "garage_favor_rysiek_point",
               "garage favor keeps its point id");
    }

    expect(!bs3d::localRewirFavorForService("shop_service_zenon").has_value(),
           "non-local shop service does not resolve a rewir favor");
    expect(!bs3d::localRewirFavorForPoint("npc_zenon").has_value(),
           "non-favor interaction point does not resolve a rewir favor");
    expect(!bs3d::localRewirFavorById("shop_favor_zenon").has_value(),
           "unknown favor id does not resolve a rewir favor");
}

void localRewirFavorStateReflectsHotLocalPressure() {
    bs3d::WorldEventLedger ledger;

    const auto quiet =
        bs3d::resolveLocalRewirFavorState(ledger, {-21.4f, 0.0f, 25.2f}, "garage_favor_rysiek_point");
    expect(!quiet.has_value(), "quiet rewir does not expose a favor state");

    bs3d::WorldEvent garage;
    garage.type = bs3d::WorldEventType::PropertyDamage;
    garage.location = bs3d::WorldLocationTag::Garage;
    garage.position = {-18.0f, 0.0f, 23.0f};
    garage.intensity = 1.6f;
    garage.remainingSeconds = 18.0f;
    garage.source = "garage_favor_pressure";
    ledger.add(garage);
    ledger.update(5.0f);

    const auto active =
        bs3d::resolveLocalRewirFavorState(ledger, {-21.4f, 0.0f, 25.2f}, "garage_favor_rysiek_point");
    expect(active.has_value(), "hot garage pressure exposes its local favor state");
    if (active.has_value()) {
        expect(active->id == "garage_favor_rysiek", "favor state exposes runtime favor id");
        expect(active->serviceInteractionId == "garage_service_rysiek",
               "favor state preserves service relief target");
        expect(active->prompt == "E: przetrzyj rysy", "favor state exposes prompt");
        expect(active->startLine == "Jak juz tu stoisz, przetrzyj rysy i bedzie ciszej.",
               "favor state exposes start line");
        expect(active->completeLine == "No, teraz garaz oddycha.",
               "favor state exposes completion line");
        expect(active->priority > 0, "favor state has an interaction priority");
    }

    const auto wrongRewir =
        bs3d::resolveLocalRewirFavorState(ledger, {5.8f, 0.0f, -6.4f}, "trash_favor_dozorca_point");
    expect(!wrongRewir.has_value(), "garage pressure does not expose the trash favor state");
}

void localRewirServiceStateResolvesByPointAndInteractionId() {
    bs3d::WorldEventLedger ledger;

    const auto calmGarage =
        bs3d::resolveLocalRewirServiceState(ledger, {-18.0f, 0.0f, 23.0f}, "garage_rysiek");
    expect(calmGarage.has_value(), "local service state resolves from an authored interaction point id");
    if (calmGarage.has_value()) {
        expect(calmGarage->access == bs3d::LocalRewirServiceAccess::Normal,
               "garage local service starts calm through generic resolver");
        expect(calmGarage->prompt == "E: pogadaj z Ryskiem",
               "generic resolver preserves calm garage prompt");
    }

    const auto missing =
        bs3d::resolveLocalRewirServiceState(ledger, {-18.0f, 0.0f, 23.0f}, "npc_zenon");
    expect(!missing.has_value(), "generic local service resolver ignores non-local-service points");

    bs3d::WorldEvent trash;
    trash.type = bs3d::WorldEventType::PropertyDamage;
    trash.location = bs3d::WorldLocationTag::Trash;
    trash.position = {9.0f, 0.0f, -4.0f};
    trash.intensity = 1.4f;
    trash.remainingSeconds = 18.0f;
    trash.source = "trash_pressure_memory";
    ledger.add(trash);
    ledger.update(5.0f);

    const auto waryTrash =
        bs3d::resolveLocalRewirServiceStateForInteraction(ledger, {9.0f, 0.0f, -4.0f}, "trash_service_dozorca");
    expect(waryTrash.has_value(), "local service state resolves from runtime interaction id");
    if (waryTrash.has_value()) {
        expect(waryTrash->access == bs3d::LocalRewirServiceAccess::Wary,
               "trash local service turns wary through generic resolver");
        expect(waryTrash->speaker == "Dozorca", "generic resolver preserves wary trash speaker");
        expect(waryTrash->line == "Po takim balecie altana ma godziny ciszy.",
               "generic resolver preserves wary trash line");
    }

    bs3d::WorldEventLedger parkingLedger;
    const auto calmParking =
        bs3d::resolveLocalRewirServiceState(parkingLedger, {-7.0f, 0.0f, 8.6f}, "parking_parkingowy");
    expect(calmParking.has_value(), "parking local service resolves from authored point id");
    if (calmParking.has_value()) {
        expect(calmParking->access == bs3d::LocalRewirServiceAccess::Normal,
               "parking local service starts calm through generic resolver");
        expect(calmParking->prompt == "E: pogadaj z Parkingowym",
               "generic resolver preserves calm parking prompt");
    }

    bs3d::WorldEvent parking;
    parking.type = bs3d::WorldEventType::PropertyDamage;
    parking.location = bs3d::WorldLocationTag::Parking;
    parking.position = {-7.0f, 0.0f, 8.6f};
    parking.intensity = 1.4f;
    parking.remainingSeconds = 18.0f;
    parking.source = "parking_pressure_memory";
    parkingLedger.add(parking);
    parkingLedger.update(5.0f);

    const auto waryParking =
        bs3d::resolveLocalRewirServiceStateForInteraction(parkingLedger,
                                                          {-7.0f, 0.0f, 8.6f},
                                                          "parking_service_parkingowy");
    expect(waryParking.has_value(), "parking local service resolves from runtime interaction id");
    if (waryParking.has_value()) {
        expect(waryParking->access == bs3d::LocalRewirServiceAccess::Wary,
               "parking local service turns wary through generic resolver");
        expect(waryParking->speaker == "Parkingowy", "generic resolver preserves parking speaker");
        expect(waryParking->line == "Parking pamieta kazdy bokiem wjazd.",
               "generic resolver preserves wary parking line");
    }

    bs3d::WorldEventLedger roadLedger;
    const auto calmRoad =
        bs3d::resolveLocalRewirServiceState(roadLedger, {8.0f, 0.0f, -18.0f}, "road_kierowca");
    expect(calmRoad.has_value(), "road-loop local service resolves from authored point id");
    if (calmRoad.has_value()) {
        expect(calmRoad->access == bs3d::LocalRewirServiceAccess::Normal,
               "road-loop local service starts calm through generic resolver");
        expect(calmRoad->prompt == "E: pogadaj z Kierowca",
               "generic resolver preserves calm road-loop prompt");
    }

    bs3d::WorldEvent road;
    road.type = bs3d::WorldEventType::PropertyDamage;
    road.location = bs3d::WorldLocationTag::RoadLoop;
    road.position = {8.0f, 0.0f, -18.0f};
    road.intensity = 1.4f;
    road.remainingSeconds = 18.0f;
    road.source = "road_pressure_memory";
    roadLedger.add(road);
    roadLedger.update(5.0f);

    const auto waryRoad =
        bs3d::resolveLocalRewirServiceStateForInteraction(roadLedger, {8.0f, 0.0f, -18.0f}, "road_service_kierowca");
    expect(waryRoad.has_value(), "road-loop local service resolves from runtime interaction id");
    if (waryRoad.has_value()) {
        expect(waryRoad->access == bs3d::LocalRewirServiceAccess::Wary,
               "road-loop local service turns wary through generic resolver");
        expect(waryRoad->speaker == "Kierowca", "generic resolver preserves road-loop speaker");
        expect(waryRoad->line == "Ta droga ma pamiec lepsza niz kierowcy.",
               "generic resolver preserves wary road-loop line");
    }
}

void localRewirServiceDigestReportsWaryPressureServices() {
    bs3d::WorldEventLedger ledger;

    bs3d::LocalRewirServiceDigest quietDigest = bs3d::buildLocalRewirServiceDigest(ledger);
    expect(quietDigest.total == static_cast<int>(bs3d::localRewirServiceCatalog().size()),
           "local service digest counts every catalog service");
    expect(quietDigest.wary == 0, "quiet local service digest starts without wary services");
    expect(quietDigest.waryInteractionPointIds.empty(),
           "quiet local service digest has no wary service ids");

    bs3d::WorldEvent parking;
    parking.type = bs3d::WorldEventType::PropertyDamage;
    parking.location = bs3d::WorldLocationTag::Parking;
    parking.position = {-7.0f, 0.0f, 8.6f};
    parking.intensity = 1.4f;
    parking.remainingSeconds = 18.0f;
    parking.source = "parking_pressure_memory";
    ledger.add(parking);
    ledger.update(5.0f);

    bs3d::LocalRewirServiceDigest parkingDigest = bs3d::buildLocalRewirServiceDigest(ledger);
    expect(parkingDigest.wary == 1, "parking pressure makes one catalog service wary");
    expect(std::find(parkingDigest.waryInteractionPointIds.begin(),
                     parkingDigest.waryInteractionPointIds.end(),
                     "parking_parkingowy") != parkingDigest.waryInteractionPointIds.end(),
           "local service digest exposes the parking service id");
    expect(std::find(parkingDigest.waryInteractionPointIds.begin(),
                     parkingDigest.waryInteractionPointIds.end(),
                     "garage_rysiek") == parkingDigest.waryInteractionPointIds.end(),
           "local service digest does not mark quiet garage service");

    bs3d::WorldEvent garage;
    garage.type = bs3d::WorldEventType::PropertyDamage;
    garage.location = bs3d::WorldLocationTag::Garage;
    garage.position = {-18.0f, 0.0f, 23.0f};
    garage.intensity = 1.4f;
    garage.remainingSeconds = 18.0f;
    garage.source = "garage_pressure_memory";
    ledger.add(garage);
    ledger.update(5.0f);

    bs3d::LocalRewirServiceDigest multiDigest = bs3d::buildLocalRewirServiceDigest(ledger);
    expect(multiDigest.wary == 2, "digest reports multiple simultaneous wary local services");
    expect(std::find(multiDigest.waryInteractionPointIds.begin(),
                     multiDigest.waryInteractionPointIds.end(),
                     "garage_rysiek") != multiDigest.waryInteractionPointIds.end(),
           "local service digest exposes the garage service id");

    ledger.update(13.1f);
    bs3d::LocalRewirServiceDigest cooledDigest = bs3d::buildLocalRewirServiceDigest(ledger);
    expect(cooledDigest.wary == 0, "local service digest cools down when pressure memories expire");
}

void localRewirServiceReliefSoftensOnlyItsOwnRewirMemory() {
    bs3d::WorldEventLedger ledger;

    bs3d::WorldEvent garage;
    garage.type = bs3d::WorldEventType::PropertyDamage;
    garage.location = bs3d::WorldLocationTag::Garage;
    garage.position = {-18.0f, 0.0f, 23.0f};
    garage.intensity = 1.6f;
    garage.remainingSeconds = 18.0f;
    garage.source = "garage_pressure_memory";
    ledger.add(garage);

    bs3d::WorldEvent trash = garage;
    trash.location = bs3d::WorldLocationTag::Trash;
    trash.position = {9.0f, 0.0f, -4.0f};
    trash.source = "trash_pressure_memory";
    ledger.add(trash);
    ledger.update(5.0f);

    const auto waryGarage =
        bs3d::resolveLocalRewirServiceStateForInteraction(ledger, {-18.0f, 0.0f, 23.0f}, "garage_service_rysiek");
    expect(waryGarage.has_value() && waryGarage->access == bs3d::LocalRewirServiceAccess::Wary,
           "garage service starts wary before relief");

    const bs3d::LocalRewirServiceReliefResult relief =
        bs3d::applyLocalRewirServiceRelief(ledger, {-18.0f, 0.0f, 23.0f}, "garage_service_rysiek");
    expect(relief.matchedService, "local service relief recognizes the runtime interaction id");
    expect(relief.pressureWasActive, "local service relief reports active pressure");
    expect(relief.softenedEvents == 1, "local service relief softens one matching rewir event");
    expect(relief.location == bs3d::WorldLocationTag::Garage, "local service relief reports its rewir");
    expect(relief.feedbackLine == "Garaz odpuszcza.",
           "local service relief exposes a short player-facing feedback line");

    const auto calmGarage =
        bs3d::resolveLocalRewirServiceStateForInteraction(ledger, {-18.0f, 0.0f, 23.0f}, "garage_service_rysiek");
    expect(calmGarage.has_value() && calmGarage->access == bs3d::LocalRewirServiceAccess::Normal,
           "relieved garage service returns to calm mode immediately");

    const auto stillWaryTrash =
        bs3d::resolveLocalRewirServiceStateForInteraction(ledger, {9.0f, 0.0f, -4.0f}, "trash_service_dozorca");
    expect(stillWaryTrash.has_value() && stillWaryTrash->access == bs3d::LocalRewirServiceAccess::Wary,
           "local service relief does not clear another hot rewir");

    const bs3d::LocalRewirServiceReliefResult repeated =
        bs3d::applyLocalRewirServiceRelief(ledger, {-18.0f, 0.0f, 23.0f}, "garage_service_rysiek");
    expect(repeated.matchedService && !repeated.pressureWasActive && repeated.softenedEvents == 0,
           "relief is a no-op once the service is calm");
    expect(repeated.feedbackLine.empty(), "no-op relief does not show a stale feedback line");
}

void przypalRisesFromEventPulseCapsAndDecaysAfterDelay() {
    bs3d::PrzypalSystem przypal;

    bs3d::WorldEventAddResult nuisance;
    nuisance.addedToLedger = true;
    nuisance.heatPulseAccepted = true;
    nuisance.accepted = true;
    nuisance.type = bs3d::WorldEventType::PublicNuisance;
    nuisance.location = bs3d::WorldLocationTag::Block;
    nuisance.intensity = 1.0f;
    nuisance.heatMultiplier = 1.0f;
    przypal.onWorldEvent(nuisance);
    expectNear(przypal.value(), 8.0f, 0.001f, "public nuisance adds base heat once");

    przypal.update(1.49f);
    expectNear(przypal.value(), 8.0f, 0.001f, "Przypal waits before decay starts");

    przypal.update(1.0f);
    expect(przypal.value() < 5.2f && przypal.value() > 4.8f, "Przypal decays at about 3 per second after delay");

    bs3d::WorldEventAddResult huge = nuisance;
    huge.type = bs3d::WorldEventType::ShopTrouble;
    huge.intensity = 20.0f;
    przypal.onWorldEvent(huge);
    expectNear(przypal.value(), 100.0f, 0.001f, "Przypal caps at 100");
}

void przypalBandsLabelsAndPulseOnlyOncePerCrossing() {
    expect(bs3d::PrzypalSystem::bandForValue(19.9f) == bs3d::PrzypalBand::Calm, "below 20 is Calm");
    expect(bs3d::PrzypalSystem::bandForValue(20.0f) == bs3d::PrzypalBand::Noticed, "20 is Noticed");
    expect(bs3d::PrzypalSystem::bandForValue(45.0f) == bs3d::PrzypalBand::Hot, "45 is Hot");
    expect(bs3d::PrzypalSystem::bandForValue(70.0f) == bs3d::PrzypalBand::ChaseRisk, "70 is ChaseRisk");
    expect(bs3d::PrzypalSystem::bandForValue(90.0f) == bs3d::PrzypalBand::Meltdown, "90 is Meltdown");
    expect(std::string(bs3d::PrzypalSystem::hudLabel(bs3d::PrzypalBand::ChaseRisk)) == "Zaraz psy",
           "HUD label maps internal band to osiedle wording");

    bs3d::PrzypalSystem przypal;
    bs3d::WorldEventAddResult damage;
    damage.addedToLedger = true;
    damage.heatPulseAccepted = true;
    damage.accepted = true;
    damage.type = bs3d::WorldEventType::PropertyDamage;
    damage.location = bs3d::WorldLocationTag::Parking;
    damage.intensity = 2.0f;
    damage.heatMultiplier = 1.0f;
    przypal.onWorldEvent(damage);
    expect(przypal.consumeBandPulse(), "first threshold crossing creates a HUD pulse");
    expect(!przypal.consumeBandPulse(), "band pulse is consumed only once");

    przypal.onWorldEvent(damage);
    expect(przypal.consumeBandPulse(), "next threshold crossing creates one new pulse");
    expect(!przypal.consumeBandPulse(), "second threshold pulse is not repeated every frame");
}

void przypalBandPulseOnlyFiresWhenHeatRisesAcrossThreshold() {
    bs3d::PrzypalSystem przypal;

    bs3d::WorldEventAddResult trouble;
    trouble.addedToLedger = true;
    trouble.heatPulseAccepted = true;
    trouble.accepted = true;
    trouble.type = bs3d::WorldEventType::ShopTrouble;
    trouble.location = bs3d::WorldLocationTag::Shop;
    trouble.intensity = 3.0f;
    trouble.heatMultiplier = 1.0f;

    przypal.onWorldEvent(trouble);
    expect(przypal.band() == bs3d::PrzypalBand::Hot, "setup raises Przypal into Hot band");
    expect(przypal.consumeBandPulse(), "rising band creates one pulse");

    for (int i = 0; i < 30; ++i) {
        przypal.update(1.0f);
    }

    expect(przypal.band() == bs3d::PrzypalBand::Calm, "setup decays back to calm");
    expect(!przypal.consumeBandPulse(), "falling band changes do not create alarm pulses");
}

void worldEventEmitterCooldownsBlockChaseSeenSpamAndFilterTinyDamage() {
    bs3d::WorldEventEmitterCooldowns cooldowns;

    expect(bs3d::shouldEmitPropertyDamage(3.99f) == false, "tiny vehicle scrape is not property damage");
    expect(bs3d::shouldEmitPropertyDamage(4.0f), "impact threshold emits property damage");

    expect(cooldowns.consume(bs3d::WorldEventType::ChaseSeen, "chase", 5.0f), "first ChaseSeen event passes cooldown");
    expect(!cooldowns.consume(bs3d::WorldEventType::ChaseSeen, "chase", 5.0f), "ChaseSeen cooldown blocks frame spam");
    cooldowns.update(4.9f);
    expect(!cooldowns.consume(bs3d::WorldEventType::ChaseSeen, "chase", 5.0f), "cooldown still blocks before five seconds");
    cooldowns.update(0.2f);
    expect(cooldowns.consume(bs3d::WorldEventType::ChaseSeen, "chase", 5.0f), "cooldown opens after five seconds");
}

void npcReactionSystemPicksDeterministicVariantsWithoutImmediateRepeat() {
    bs3d::WorldEventLedger ledger;
    bs3d::PrzypalSystem przypal;
    bs3d::NpcReactionSystem reactions;

    bs3d::WorldEvent event;
    event.type = bs3d::WorldEventType::PublicNuisance;
    event.location = bs3d::WorldLocationTag::Block;
    event.position = {0.0f, 0.0f, 0.0f};
    event.intensity = 1.0f;
    event.remainingSeconds = 18.0f;
    event.source = "horn";
    przypal.onWorldEvent(ledger.add(event));

    const bs3d::ReactionIntent first = reactions.update(ledger, przypal, {0.0f, 0.0f, 0.0f}, false, 0.016f);
    expect(first.available, "reaction system emits first local reaction");
    expect(first.source == bs3d::ReactionSource::BabkaWindow, "public nuisance chooses Babka alarm first");

    const bs3d::ReactionIntent blocked = reactions.update(ledger, przypal, {0.0f, 0.0f, 0.0f}, false, 0.016f);
    expect(!blocked.available, "reaction global cooldown blocks immediate subtitle spam");

    const bs3d::ReactionIntent sameEvent = reactions.update(ledger, przypal, {0.0f, 0.0f, 0.0f}, false, 8.5f);
    expect(!sameEvent.available, "same source does not comment the same event id again after cooldown");

    bs3d::WorldEvent secondEvent = event;
    secondEvent.position = {12.0f, 0.0f, 0.0f};
    secondEvent.source = "horn_second";
    przypal.onWorldEvent(ledger.add(secondEvent));
    const bs3d::ReactionIntent second = reactions.update(ledger, przypal, {12.0f, 0.0f, 0.0f}, false, 3.1f);
    expect(second.available, "reaction can fire again for a new event after cooldown");
    expect(second.source == first.source, "same source can react to a new event");
    expect(second.text != first.text, "same source does not repeat the same line twice in a row");
}

void npcReactionSystemUsesRewirSpecificWitnessesForGarageAndTrashMemory() {
    bs3d::WorldEventLedger garageLedger;
    bs3d::PrzypalSystem garagePrzypal;
    bs3d::NpcReactionSystem garageReactions;

    bs3d::WorldEvent garage;
    garage.type = bs3d::WorldEventType::PropertyDamage;
    garage.location = bs3d::WorldLocationTag::Garage;
    garage.position = {-18.0f, 0.0f, 23.0f};
    garage.intensity = 1.0f;
    garage.remainingSeconds = 18.0f;
    garage.source = "garage_bump";
    garagePrzypal.onWorldEvent(garageLedger.add(garage));

    const bs3d::ReactionIntent garageIntent =
        garageReactions.update(garageLedger, garagePrzypal, {-18.0f, 0.0f, 23.0f}, false, 0.016f);
    expect(garageIntent.available, "garage memory emits a local reaction");
    expect(garageIntent.source == bs3d::ReactionSource::GarageCrew,
           "garage memory uses a garage-specific witness instead of generic alarm");
    expect(garageIntent.speaker == "Garazowy", "garage witness speaker is readable in subtitles");

    bs3d::WorldEventLedger trashLedger;
    bs3d::PrzypalSystem trashPrzypal;
    bs3d::NpcReactionSystem trashReactions;

    bs3d::WorldEvent trash = garage;
    trash.location = bs3d::WorldLocationTag::Trash;
    trash.position = {9.0f, 0.0f, -4.0f};
    trash.source = "trash_bump";
    trashPrzypal.onWorldEvent(trashLedger.add(trash));

    const bs3d::ReactionIntent trashIntent =
        trashReactions.update(trashLedger, trashPrzypal, {9.0f, 0.0f, -4.0f}, false, 0.016f);
    expect(trashIntent.available, "trash memory emits a local reaction");
    expect(trashIntent.source == bs3d::ReactionSource::TrashWitness,
           "trash memory uses a trash-specific witness instead of generic alarm");
    expect(trashIntent.speaker == "Dozorca", "trash witness speaker is readable in subtitles");
}

void npcReactionSystemUsesRewirSpecificWitnessesForParkingAndRoadMemory() {
    bs3d::WorldEventLedger parkingLedger;
    bs3d::PrzypalSystem parkingPrzypal;
    bs3d::NpcReactionSystem parkingReactions;

    bs3d::WorldEvent parking;
    parking.type = bs3d::WorldEventType::PropertyDamage;
    parking.location = bs3d::WorldLocationTag::Parking;
    parking.position = {-7.0f, 0.0f, 8.6f};
    parking.intensity = 1.0f;
    parking.remainingSeconds = 18.0f;
    parking.source = "parking_bump";
    parkingPrzypal.onWorldEvent(parkingLedger.add(parking));

    const bs3d::ReactionIntent parkingIntent =
        parkingReactions.update(parkingLedger, parkingPrzypal, {-7.0f, 0.0f, 8.6f}, false, 0.016f);
    expect(parkingIntent.available, "parking memory emits a local reaction");
    expect(parkingIntent.source == bs3d::ReactionSource::ParkingWitness,
           "parking memory uses a parking-specific witness instead of generic alarm");
    expect(parkingIntent.speaker == "Parkingowy", "parking witness speaker is readable in subtitles");

    bs3d::WorldEventLedger roadLedger;
    bs3d::PrzypalSystem roadPrzypal;
    bs3d::NpcReactionSystem roadReactions;

    bs3d::WorldEvent road = parking;
    road.location = bs3d::WorldLocationTag::RoadLoop;
    road.position = {0.0f, 0.0f, 0.0f};
    road.source = "road_loop_bump";
    roadPrzypal.onWorldEvent(roadLedger.add(road));

    const bs3d::ReactionIntent roadIntent =
        roadReactions.update(roadLedger, roadPrzypal, {0.0f, 0.0f, 0.0f}, false, 0.016f);
    expect(roadIntent.available, "road-loop memory emits a local reaction");
    expect(roadIntent.source == bs3d::ReactionSource::RoadLoopWitness,
           "road-loop memory uses a road-specific witness instead of generic alarm");
    expect(roadIntent.speaker == "Kierowca", "road witness speaker is readable in subtitles");
}

void npcReactionSystemUsesAgedRewirHotspotsBeforeGenericGossip() {
    bs3d::WorldEventLedger ledger;
    bs3d::PrzypalSystem przypal;
    bs3d::NpcReactionSystem reactions;

    bs3d::WorldEvent parking;
    parking.type = bs3d::WorldEventType::PropertyDamage;
    parking.location = bs3d::WorldLocationTag::Parking;
    parking.position = {-7.0f, 0.0f, 8.6f};
    parking.intensity = 1.2f;
    parking.remainingSeconds = 18.0f;
    parking.source = "parking_hotspot_memory";
    przypal.onWorldEvent(ledger.add(parking));

    ledger.update(5.0f);

    const bs3d::ReactionIntent intent =
        reactions.update(ledger, przypal, {-7.0f, 0.0f, 8.6f}, false, 0.016f);
    expect(intent.available, "aged rewir hotspot can still surface a reaction");
    expect(intent.source == bs3d::ReactionSource::ParkingWitness,
           "aged parking hotspot uses local witness before generic gossip");
    expect(intent.speaker == "Parkingowy", "aged hotspot keeps local speaker identity");
}

void npcReactionSystemTurnsLingeringRewirPressureIntoPatrolHintAfterWitnessBeat() {
    bs3d::WorldEventLedger ledger;
    bs3d::PrzypalSystem przypal;
    bs3d::NpcReactionSystem reactions;

    bs3d::WorldEvent parking;
    parking.type = bs3d::WorldEventType::PropertyDamage;
    parking.location = bs3d::WorldLocationTag::Parking;
    parking.position = {-7.0f, 0.0f, 8.6f};
    parking.intensity = 1.4f;
    parking.remainingSeconds = 18.0f;
    parking.source = "parking_pressure_memory";
    przypal.onWorldEvent(ledger.add(parking));
    ledger.update(5.0f);

    const bs3d::ReactionIntent witness =
        reactions.update(ledger, przypal, {-7.0f, 0.0f, 8.6f}, false, 0.016f);
    expect(witness.source == bs3d::ReactionSource::ParkingWitness,
           "local witness owns the first lingering pressure beat");

    ledger.update(3.1f);
    const bs3d::ReactionIntent patrol =
        reactions.update(ledger, przypal, {-7.0f, 0.0f, 8.6f}, false, 3.1f);
    expect(patrol.available, "lingering rewir pressure can surface a second beat");
    expect(patrol.source == bs3d::ReactionSource::PatrolHint,
           "rewir pressure becomes patrol interest after local witness beat");
    expect(patrol.priority > 35, "pressure patrol hint beats generic gossip");
    expect(patrol.priority < witness.priority, "pressure patrol hint stays below the local witness");
}

void playerControllerSettersPreserveMotorRuntimeState() {
    bs3d::PlayerController player;
    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    player.setStatus(bs3d::PlayerStatus::Scared, true);
    player.triggerStagger({1.0f, 0.0f, 0.5f}, 0.5f, 0.8f);

    const bs3d::Vec3 velocityBefore = player.velocity();
    const float staggerBefore = player.motorState().staggerSeconds;
    player.teleportTo({4.0f, 0.0f, -3.0f});

    expectNear(player.position().x, 4.0f, 0.001f, "teleportTo moves player x");
    expectNear(player.position().z, -3.0f, 0.001f, "teleportTo moves player z");
    expectNear(player.velocity().x, velocityBefore.x, 0.001f, "teleportTo preserves velocity x");
    expectNear(player.velocity().z, velocityBefore.z, 0.001f, "teleportTo preserves velocity z");
    expectNear(player.motorState().staggerSeconds, staggerBefore, 0.001f, "teleportTo preserves stagger timer");
    expect(player.motorState().statuses.scared, "teleportTo preserves status flags");

    player.setFacingYaw(1.25f);
    expectNear(player.yawRadians(), 1.25f, 0.001f, "setFacingYaw updates facing yaw");
    expectNear(player.velocity().x, velocityBefore.x, 0.001f, "setFacingYaw preserves velocity x");
    expect(player.motorState().statuses.scared, "setFacingYaw preserves status flags");

    player.setPosition({6.0f, 0.0f, 2.0f});
    player.setYaw(-0.75f);
    expectNear(player.position().x, 6.0f, 0.001f, "legacy setPosition remains a no-reset teleport alias");
    expectNear(player.yawRadians(), -0.75f, 0.001f, "legacy setYaw remains a no-reset yaw alias");
    expectNear(player.velocity().x, velocityBefore.x, 0.001f, "legacy setter aliases preserve velocity");
    expect(player.motorState().statuses.scared, "legacy setter aliases preserve status flags");
}

void playerControllerVehicleSeatSyncClearsStaleMotionButKeepsStatus() {
    bs3d::PlayerController player;
    player.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    player.setStatus(bs3d::PlayerStatus::Scared, true);
    player.applyImpulse({2.5f, 3.0f, -1.0f});
    player.triggerStagger({1.0f, 0.0f, 0.0f}, 0.6f, 0.9f);
    expect(!player.isGrounded(), "test setup puts player in stale airborne vehicle-entry state");
    expect(player.motorState().staggerSeconds > 0.0f, "test setup has stale stagger");

    player.syncToVehicleSeat({5.0f, 0.0f, 7.0f}, 1.1f);

    expectNear(player.position().x, 5.0f, 0.001f, "vehicle seat sync moves player x");
    expectNear(player.position().z, 7.0f, 0.001f, "vehicle seat sync moves player z");
    expectNear(player.yawRadians(), 1.1f, 0.001f, "vehicle seat sync faces vehicle yaw");
    expectNear(player.velocity().x, 0.0f, 0.001f, "vehicle seat sync clears horizontal velocity x");
    expectNear(player.velocity().y, 0.0f, 0.001f, "vehicle seat sync clears vertical velocity");
    expectNear(player.velocity().z, 0.0f, 0.001f, "vehicle seat sync clears horizontal velocity z");
    expect(player.isGrounded(), "vehicle seat sync forces safe grounded state for exit");
    expectNear(player.motorState().staggerSeconds, 0.0f, 0.001f, "vehicle seat sync clears stale stagger");
    expectNear(player.motorState().impactIntensity, 0.0f, 0.001f, "vehicle seat sync clears stale impact");
    expect(player.motorState().statuses.scared, "vehicle seat sync keeps gameplay status flags");
}

void playerControllerVehicleExitSyncClearsStaleMotionAndKeepsStatus() {
    bs3d::PlayerController player;
    player.reset({2.0f, 0.0f, 3.0f}, 0.0f);
    player.setStatus(bs3d::PlayerStatus::Scared, true);
    player.applyImpulse({4.0f, 2.0f, -3.0f});
    player.triggerStagger({1.0f, 0.0f, 0.0f}, 0.4f, 0.7f);

    player.syncAfterVehicleExit({8.0f, 0.0f, 6.0f}, -0.65f);

    expectNear(player.position().x, 8.0f, 0.001f, "vehicle exit sync places player x");
    expectNear(player.position().z, 6.0f, 0.001f, "vehicle exit sync places player z");
    expectNear(player.yawRadians(), -0.65f, 0.001f, "vehicle exit sync faces exit yaw");
    expectNear(player.velocity().x, 0.0f, 0.001f, "vehicle exit sync clears stale velocity x");
    expectNear(player.velocity().y, 0.0f, 0.001f, "vehicle exit sync clears stale velocity y");
    expectNear(player.velocity().z, 0.0f, 0.001f, "vehicle exit sync clears stale velocity z");
    expect(player.isGrounded(), "vehicle exit sync forces grounded state");
    expectNear(player.motorState().staggerSeconds, 0.0f, 0.001f, "vehicle exit sync clears stale stagger");
    expectNear(player.motorState().impactIntensity, 0.0f, 0.001f, "vehicle exit sync clears stale impact");
    expect(player.motorState().statuses.scared, "vehicle exit sync keeps gameplay status flags");
}

void playerMotorWallBumpKeepsShortOutwardImpulseAfterCollisionResolution() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({0.0f, 1.0f, 3.2f}, {4.0f, 2.0f, 0.35f});

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent sprint;
    sprint.moveDirection = {0.0f, 0.0f, 1.0f};
    sprint.sprint = true;
    for (int i = 0; i < 40; ++i) {
        motor.update(sprint, collision, 1.0f / 30.0f);
        if (motor.state().hitWallThisFrame) {
            break;
        }
    }

    expect(motor.state().hitWallThisFrame, "setup reaches wall collision");
    expect(motor.state().impactIntensity > 0.0f, "wall bump exposes presentation impact");
    expect(motor.state().velocity.z < -0.05f, "wall bump leaves a short outward impulse after blocked velocity is zeroed");
}

void playerMotorUpwardStaggerImpulseLeavesGroundImmediately() {
    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.0f, 0.0f}, 0.0f);
    expect(motor.state().grounded, "setup starts grounded");

    motor.triggerStagger({0.0f, 0.75f, 0.0f}, 0.45f, 0.8f);

    expect(!motor.state().grounded, "upward stagger impulse immediately breaks grounded state");
    expect(motor.state().movementMode == bs3d::PlayerMovementMode::Airborne,
           "upward stagger impulse reports airborne before the next motor update");
}

void vehicleCapsuleCollisionChecksFrontCenterAndRear() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({0.0f, 1.0f, 2.5f}, {2.0f, 2.0f, 0.35f});

    bs3d::VehicleCollisionConfig config;
    config.speed = 8.0f;

    const bs3d::Vec3 from{0.0f, 0.0f, 0.0f};
    const bs3d::Vec3 desired{0.0f, 0.0f, 1.0f};
    const bs3d::VehicleCollisionResult result = collision.resolveVehicleCapsule(from, desired, 0.0f, config);

    expect(result.hit, "vehicle capsule catches obstacle near visible front");
    expect(result.hitFront, "front collision point reports hit");
    expect(result.hitZone == bs3d::VehicleHitZone::Front, "vehicle capsule reports front hit zone");
    expect(result.impactSpeed >= 7.9f, "vehicle capsule reports configured impact speed");
    expect(result.impactNormal.z < -0.5f, "front hit normal points away from obstacle");
    expect(result.position.z < desired.z, "vehicle capsule prevents front clipping through wall");
    expect(!collision.isCircleBlocked(result.circles[0], config.radius), "resolved front circle is not embedded");
}

void vehicleCapsuleReportsSideHitZonesAndProfile() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({-1.45f, 1.0f, 0.0f},
                     {0.25f, 2.0f, 5.0f},
                     bs3d::CollisionProfile::vehicleBlocker());

    bs3d::VehicleCollisionConfig config;
    config.speed = 5.5f;
    config.lateralSlip = 1.0f;

    const bs3d::VehicleCollisionResult result =
        collision.resolveVehicleCapsule({0.0f, 0.0f, 0.0f},
                                        {-1.2f, 0.0f, 0.0f},
                                        0.0f,
                                        config);

    expect(result.hit, "vehicle side capsule detects side blocker");
    expect(result.hitZone == bs3d::VehicleHitZone::RightSide,
           "screen-right travel reports the vehicle right side as the hit zone");
    expect(result.impactNormal.x > 0.5f, "right-side hit normal pushes back toward vehicle center");
    expect(result.hitProfile.layers & bs3d::collisionLayerMask(bs3d::CollisionLayer::VehicleBlocker),
           "vehicle hit keeps the blocker collision profile for gameplay/debug");
}

void vehicleCapsuleSlidesAlongWallInsteadOfSticking() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({1.40f, 1.0f, 1.8f}, {0.30f, 2.0f, 5.0f});

    const bs3d::VehicleCollisionConfig config;
    const bs3d::Vec3 from{0.0f, 0.0f, 0.0f};
    const bs3d::Vec3 desired{1.2f, 0.0f, 2.0f};

    const bs3d::VehicleCollisionResult result = collision.resolveVehicleCapsule(from, desired, 0.0f, config);

    expect(result.hit, "vehicle capsule reports wall contact while sliding");
    expect(result.position.x < desired.x - 0.2f, "vehicle capsule blocks movement into side wall");
    expect(result.position.z > 1.35f, "vehicle capsule keeps useful forward slide along wall");
    expect(!collision.isCircleBlocked(result.circles[0], config.radius), "slid front circle is not embedded");
    expect(!collision.isCircleBlocked(result.circles[1], config.radius), "slid center circle is not embedded");
    expect(!collision.isCircleBlocked(result.circles[2], config.radius), "slid rear circle is not embedded");
}

void vehicleCollisionConfigMatchesVisibleBorexFootprint() {
    constexpr float VisualWidth = 1.9f;
    constexpr float VisualLength = 3.3f;
    const bs3d::VehicleCollisionConfig config;
    const float collisionWidth = config.radius * 2.0f;
    const float collisionLength = (config.radius + config.halfLength) * 2.0f;

    expect(collisionWidth >= VisualWidth * 0.94f, "vehicle collision is not much narrower than visible body");
    expect(collisionWidth <= VisualWidth * 1.02f, "vehicle collision is not wider than visible body");
    expect(collisionLength >= VisualLength * 0.96f, "vehicle collision reaches visible front and rear");
    expect(collisionLength <= VisualLength * 1.06f, "vehicle collision does not hit far beyond visible front/rear");
    expectNear(config.halfLength, VisualLength * 0.5f - config.radius, 0.08f,
           "front/rear circle offset is derived from visual half length minus radius");
}

void missionUiBusyBlocksLowPriorityGossipButKeepsLedgerMemory() {
    bs3d::WorldEventLedger ledger;
    bs3d::PrzypalSystem przypal;
    bs3d::NpcReactionSystem reactions;

    bs3d::WorldEvent event;
    event.type = bs3d::WorldEventType::PublicNuisance;
    event.location = bs3d::WorldLocationTag::RoadLoop;
    event.position = {0.0f, 0.0f, 0.0f};
    event.intensity = 0.25f;
    event.remainingSeconds = 18.0f;
    event.source = "old_noise";
    przypal.onWorldEvent(ledger.add(event));
    ledger.update(5.0f);

    const bs3d::ReactionIntent busy = reactions.update(ledger, przypal, {0.0f, 0.0f, 0.0f}, true, 5.0f);
    expect(!busy.available, "mission UI busy blocks low-priority gossip reaction");
    expect(!ledger.recentEvents().empty(), "mission UI busy does not delete world memory");

    const bs3d::ReactionIntent free = reactions.update(ledger, przypal, {0.0f, 0.0f, 0.0f}, false, 3.1f);
    expect(free.available, "gossip can surface later when UI is free");
    expect(free.source == bs3d::ReactionSource::ZulGossip, "old low heat memory becomes Zul gossip");
}

void worldReactionsDoNotMutateInputMovementOrCameraState() {
    bs3d::InputState input;
    input.moveForward = true;
    input.cameraYawRadians = bs3d::Pi * 0.5f;

    bs3d::PlayerInputIntent beforeIntent = bs3d::PlayerInputIntent::fromInputState(input);

    bs3d::CameraRig cameraRig;
    bs3d::CameraRigTarget target;
    target.position = {0.0f, 0.0f, 0.0f};
    target.yawRadians = 0.0f;
    target.mode = bs3d::CameraRigMode::OnFootNormal;
    cameraRig.reset(target);
    const float beforeYaw = cameraRig.state().cameraYawRadians;

    bs3d::WorldEventLedger ledger;
    bs3d::PrzypalSystem przypal;
    bs3d::NpcReactionSystem reactions;

    bs3d::WorldEvent event;
    event.type = bs3d::WorldEventType::ChaseSeen;
    event.location = bs3d::WorldLocationTag::Parking;
    event.position = {0.0f, 0.0f, 0.0f};
    event.intensity = 1.0f;
    event.remainingSeconds = 18.0f;
    event.source = "chase";
    przypal.onWorldEvent(ledger.add(event));
    reactions.update(ledger, przypal, {0.0f, 0.0f, 0.0f}, false, 0.016f);
    przypal.update(0.5f);

    expect(input.moveForward, "world reactions do not mutate raw input flags");
    expectNear(input.cameraYawRadians, bs3d::Pi * 0.5f, 0.001f, "world reactions do not mutate camera yaw input");

    bs3d::PlayerInputIntent afterIntent = bs3d::PlayerInputIntent::fromInputState(input);
    expectNear(afterIntent.moveDirection.x, beforeIntent.moveDirection.x, 0.001f,
               "world reactions do not change movement direction x");
    expectNear(afterIntent.moveDirection.z, beforeIntent.moveDirection.z, 0.001f,
               "world reactions do not change movement direction z");
    expectNear(cameraRig.state().cameraYawRadians, beforeYaw, 0.001f,
               "world reactions do not mutate camera rig yaw");
}

void worldInteractionFindsNearestPrompt() {
    bs3d::WorldInteraction interaction;
    interaction.addPoint({"npc_start", "Gadaj", {-2.0f, 0.0f, 0.0f}, 2.5f});
    interaction.addPoint({"shop", "Wejdz do sklepu", {8.0f, 0.0f, 0.0f}, 2.0f});

    bs3d::InteractionPrompt prompt = interaction.query({-1.0f, 0.0f, 0.0f});
    expect(prompt.available, "nearby interaction prompt is available");
    expect(prompt.id == "npc_start", "nearest interaction prompt wins");

    bs3d::InteractionPrompt missing = interaction.query({20.0f, 0.0f, 0.0f});
    expect(!missing.available, "distant interaction prompt is unavailable");
}

void interactionResolverUsePrioritizesMissionNpcOverNearbyVehicle() {
    bs3d::InteractionResolver resolver;
    resolver.addCandidate({"npc_start",
                           bs3d::InteractionAction::StartMission,
                           bs3d::InteractionInput::Use,
                           "E: gadaj z Mietkiem",
                           {0.5f, 0.0f, 0.0f},
                           2.0f,
                           100});
    resolver.addCandidate({"gruz",
                           bs3d::InteractionAction::EnterVehicle,
                           bs3d::InteractionInput::Use,
                           "E: wsiadz do gruza",
                           {0.2f, 0.0f, 0.0f},
                           2.0f,
                           40});
    resolver.addCandidate({"gruz",
                           bs3d::InteractionAction::EnterVehicle,
                           bs3d::InteractionInput::Vehicle,
                           "F: wsiadz do gruza",
                           {0.2f, 0.0f, 0.0f},
                           2.0f,
                           80});

    const bs3d::InteractionResolution use = resolver.resolve({0.0f, 0.0f, 0.0f}, bs3d::InteractionInput::Use);
    expect(use.available, "E has an available interaction");
    expect(use.action == bs3d::InteractionAction::StartMission, "E prioritizes mission/NPC over nearby car");

    const bs3d::InteractionResolution vehicle = resolver.resolve({0.0f, 0.0f, 0.0f}, bs3d::InteractionInput::Vehicle);
    expect(vehicle.available, "F has an available vehicle interaction");
    expect(vehicle.action == bs3d::InteractionAction::EnterVehicle, "F can still choose the nearby car");
}

void interactionResolverUsesDistanceForSamePriorityCandidates() {
    bs3d::InteractionResolver resolver;
    resolver.addCandidate({"far_npc", bs3d::InteractionAction::Talk, bs3d::InteractionInput::Use, "E: far", {1.8f, 0.0f, 0.0f}, 3.0f, 50});
    resolver.addCandidate({"near_npc", bs3d::InteractionAction::Talk, bs3d::InteractionInput::Use, "E: near", {0.7f, 0.0f, 0.0f}, 3.0f, 50});

    const bs3d::InteractionResolution result = resolver.resolve({0.0f, 0.0f, 0.0f}, bs3d::InteractionInput::Use);
    expect(result.id == "near_npc", "same-priority interactions choose nearest candidate");
}

void interactionResolverFiltersDisabledOutOfRangeAndWrongInput() {
    bs3d::InteractionResolver resolver;
    resolver.addCandidate({"disabled", bs3d::InteractionAction::Talk, bs3d::InteractionInput::Use, "E: disabled", {0.0f, 0.0f, 0.0f}, 2.0f, 100, false});
    resolver.addCandidate({"far", bs3d::InteractionAction::Talk, bs3d::InteractionInput::Use, "E: far", {9.0f, 0.0f, 0.0f}, 1.0f, 90});
    resolver.addCandidate({"car", bs3d::InteractionAction::EnterVehicle, bs3d::InteractionInput::Vehicle, "F: car", {0.0f, 0.0f, 0.0f}, 2.0f, 80});

    const bs3d::InteractionResolution use = resolver.resolve({0.0f, 0.0f, 0.0f}, bs3d::InteractionInput::Use);
    expect(!use.available, "disabled, distant, and wrong-input candidates do not resolve for E");

    const bs3d::InteractionResolution vehicle = resolver.resolve({0.0f, 0.0f, 0.0f}, bs3d::InteractionInput::Vehicle);
    expect(vehicle.available && vehicle.id == "car", "matching F candidate remains available");
}

void sceneCreatesAndFindsTransformsByEntityId() {
    bs3d::Scene scene;
    bs3d::Transform transform;
    transform.position = {2.0f, 0.0f, 3.0f};

    const bs3d::EntityId id = scene.createObject("auto", transform);
    expect(id != bs3d::InvalidEntity, "scene creates valid entity ids");
    expect(scene.objectCount() == 1, "scene tracks object count");

    const bs3d::Transform* found = scene.findTransform(id);
    expect(found != nullptr, "scene finds transform by id");
    expectNear(found->position.x, 2.0f, 0.001f, "scene preserves transform position");

    scene.clear();
    expect(scene.objectCount() == 0, "scene clears objects");
    expect(scene.findTransform(id) == nullptr, "cleared scene forgets entity ids");
}

void collisionPreventsCircleEnteringBox() {
    bs3d::WorldCollision collision;
    collision.addBox({0.0f, 0.0f, 0.0f}, {4.0f, 3.0f, 4.0f});

    expect(collision.isCircleBlocked({0.0f, 0.0f, 0.0f}, 0.5f), "circle inside box is blocked");
    expect(!collision.isCircleBlocked({5.0f, 0.0f, 0.0f}, 0.5f), "distant circle is free");

    bs3d::Vec3 resolved = collision.resolveCircle({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 3.0f}, 0.5f);
    expect(resolved.z >= 2.5f, "resolution pushes circle outside box on z axis");
}

void circleCollisionDoesNotTunnelThroughThinBoxOnLargeMove() {
    bs3d::WorldCollision collision;
    collision.addBox({0.0f, 0.0f, 4.0f}, {2.0f, 2.0f, 0.35f});

    const bs3d::Vec3 resolved = collision.resolveCircle({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 8.0f}, 0.55f);

    expect(resolved.z < 3.4f, "large vehicle frame does not tunnel through a thin obstacle");
}

void vehicleCapsuleDoesNotTunnelThroughThinBlockerOnLargeMove() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({0.0f, 1.0f, 4.0f},
                     {5.0f, 2.0f, 0.35f},
                     bs3d::CollisionProfile::solidWorld());

    bs3d::VehicleCollisionConfig config;
    config.speed = 9.0f;
    const bs3d::VehicleCollisionResult result =
        collision.resolveVehicleCapsule({0.0f, 0.0f, 0.0f},
                                        {0.0f, 0.0f, 8.0f},
                                        0.0f,
                                        config);

    expect(result.hit, "swept vehicle capsule detects thin blocker even when final center is clear");
    expect(result.position.z < 3.4f, "vehicle capsule does not tunnel through thin blocker on a large frame");
}

void characterCollisionDoesNotTunnelThroughThinWallsOnLargeFrame() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({0.0f, 1.0f, 2.0f}, {4.0f, 2.0f, 0.20f});

    bs3d::CharacterCollisionConfig config;
    config.radius = 0.45f;
    config.skinWidth = 0.05f;

    const bs3d::CharacterCollisionResult result =
        collision.resolveCharacter({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 5.0f}, config);

    expect(result.hitWall, "swept character collision detects thin wall during a large frame");
    expect(result.position.z < 1.5f, "large frame does not tunnel through thin wall");
}

void collisionLayersSeparatePlayerVehicleCameraAndTriggers() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({0.0f, 1.0f, -2.0f},
                     {4.0f, 2.0f, 0.4f},
                     bs3d::CollisionProfile::playerBlocker());

    expect(collision.isCircleBlocked({0.0f, 0.0f, -2.0f},
                                     0.45f,
                                     bs3d::CollisionMasks::Player),
           "player blocker stops player collision queries");
    expect(!collision.isCircleBlocked({0.0f, 0.0f, -2.0f},
                                      0.45f,
                                      bs3d::CollisionMasks::Vehicle),
           "player-only blocker does not stop vehicle queries");

    const bs3d::Vec3 target{0.0f, 1.5f, 0.0f};
    const bs3d::Vec3 desiredCamera{0.0f, 1.5f, -4.0f};
    const bs3d::Vec3 camera = collision.resolveCameraBoom(target, desiredCamera, 0.2f);
    expectNear(camera.z, desiredCamera.z, 0.001f, "camera ignores non-camera blockers");

    collision.addBox({0.0f, 1.0f, -2.0f},
                     {4.0f, 2.0f, 0.4f},
                     bs3d::CollisionProfile::cameraBlocker());
    const bs3d::Vec3 blockedCamera = collision.resolveCameraBoom(target, desiredCamera, 0.2f);
    expect(blockedCamera.z > -2.0f, "camera blocker shortens camera boom");
}

void broadphaseHandlesNegativeCoordinateCells() {
    bs3d::WorldCollision collision;
    collision.clear();
    collision.addGroundPlane(0.0f);
    collision.addBox({-16.0f, 1.0f, -9.0f},
                     {2.0f, 2.0f, 2.0f},
                     bs3d::CollisionProfile::solidWorld());

    expect(collision.isCircleBlocked({-16.0f, 0.0f, -9.0f}, 0.45f, bs3d::CollisionMasks::Player),
           "broadphase queries find blockers in negative coordinate cells");
    expect(collision.debugBroadphaseCandidateCount({-16.0f, 0.0f, -9.0f},
                                                   0.45f,
                                                   bs3d::CollisionMasks::Player) == 1,
           "negative coordinate broadphase cells produce one candidate");
}

void staticAndDynamicGroundBoxesUseSameBaseHeightSemantics() {
    bs3d::CollisionProfile profile = bs3d::CollisionProfile::walkableSurface();
    bs3d::WorldCollision collision;
    collision.clear();
    collision.addGroundPlane(0.0f);
    collision.addGroundBox({0.0f, 0.20f, 0.0f}, {2.0f, 0.35f, 2.0f}, profile);
    collision.addDynamicGroundBox({3.0f, 0.20f, 0.0f}, {2.0f, 0.35f, 2.0f}, 0.0f, profile);

    const bs3d::GroundHit staticGround = collision.probeGround({0.0f, 1.0f, 0.0f}, 2.0f, 38.0f);
    const bs3d::GroundHit dynamicGround = collision.probeGround({3.0f, 1.0f, 0.0f}, 2.0f, 38.0f);

    expect(staticGround.hit, "static ground box is probeable");
    expect(dynamicGround.hit, "dynamic ground box is probeable");
    expectNear(staticGround.height, 0.55f, 0.001f, "static ground top is base plus height");
    expectNear(dynamicGround.height, 0.55f, 0.001f, "dynamic ground top is base plus height");
}

void dynamicVehicleRoofIsWalkableWithoutBlockingPlayerOnTop() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addDynamicVehiclePlayerCollision({0.0f, 0.0f, 0.0f}, 0.0f);

    const bs3d::GroundHit roof = collision.probeGround({0.0f, 1.15f, 0.0f}, 0.6f, 38.0f);
    expect(roof.hit, "vehicle roof registers as a walkable ground surface");
    expectNear(roof.height, 0.90f, 0.06f, "vehicle roof ground height matches authored roof");

    bs3d::CharacterCollisionConfig config;
    const bs3d::CharacterCollisionResult onRoof =
        collision.resolveCharacter({0.0f, roof.height, 0.0f}, {0.25f, roof.height, 0.2f}, config);
    expect(!onRoof.hitWall, "player standing on roof is not blocked by vehicle body collision");

    const bs3d::CharacterCollisionResult side =
        collision.resolveCharacter({1.7f, 0.0f, 0.0f}, {0.8f, 0.0f, 0.0f}, config);
    expect(side.hitWall || side.blockedByStep, "vehicle side body blocks player at ground height");
}

void vehicleRoofSupportCoversVisibleBodyEdgesForCharacterController() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addDynamicVehiclePlayerCollision({0.0f, 0.0f, 0.0f}, 0.0f);

    const bs3d::GroundHit frontEdge =
        collision.probeGround({0.0f, 1.12f, 1.58f}, 0.45f, 38.0f);
    expect(frontEdge.hit, "vehicle front edge still supports a character center above the hood");
    expectNear(frontEdge.height, 0.92f, 0.08f, "front edge support resolves to vehicle roof height");

    const bs3d::GroundHit sideEdge =
        collision.probeGround({0.92f, 1.12f, 0.0f}, 0.45f, 38.0f);
    expect(sideEdge.hit, "vehicle side edge still supports a character center above the body");
    expectNear(sideEdge.height, 0.92f, 0.08f, "side edge support resolves to vehicle roof height");
}

void vehicleTopGroundDoesNotCreateWideInvisibleLedge() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addDynamicVehiclePlayerCollision({0.0f, 0.0f, 0.0f}, 0.0f);

    const bs3d::GroundHit justBeyondSide =
        collision.probeGround({1.08f, 1.12f, 0.0f}, 0.45f, 38.0f);
    const bs3d::GroundHit justBeyondFront =
        collision.probeGround({0.0f, 1.12f, 1.72f}, 0.45f, 38.0f);

    expect(!justBeyondSide.hit || justBeyondSide.height < 0.10f,
           "vehicle roof support does not extend into a wide invisible side ledge");
    expect(!justBeyondFront.hit || justBeyondFront.height < 0.10f,
           "vehicle roof support does not extend into a wide invisible front ledge");
}

void multipleVehiclePlayerCollisionOwnersRemainDistinct() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::VehiclePlayerCollisionConfig first;
    first.ownerId = 7001;
    bs3d::VehiclePlayerCollisionConfig second;
    second.ownerId = 7002;
    collision.addDynamicVehiclePlayerCollision({0.0f, 0.0f, 0.0f}, 0.0f, first);
    collision.addDynamicVehiclePlayerCollision({5.0f, 0.0f, 0.0f}, 0.0f, second);

    bs3d::CharacterCollisionConfig config;
    const bs3d::CharacterCollisionResult hitFirst =
        collision.resolveCharacter({-2.0f, 0.0f, 0.0f}, {-0.75f, 0.0f, 0.0f}, config);
    const bs3d::CharacterCollisionResult hitSecond =
        collision.resolveCharacter({7.0f, 0.0f, 0.0f}, {5.75f, 0.0f, 0.0f}, config);

    expect(hitFirst.hitWall && hitFirst.hitOwnerId == 7001,
           "first vehicle body reports its configured owner id");
    expect(hitSecond.hitWall && hitSecond.hitOwnerId == 7002,
           "second vehicle body reports its configured owner id");

    const bs3d::GroundHit firstRoof = collision.probeGround({0.0f, 1.12f, 0.0f}, 0.45f, 38.0f);
    const bs3d::GroundHit secondRoof = collision.probeGround({5.0f, 1.12f, 0.0f}, 0.45f, 38.0f);
    expect(firstRoof.ownerId == 7001, "first vehicle roof reports its configured owner id");
    expect(secondRoof.ownerId == 7002, "second vehicle roof reports its configured owner id");
}

void vehicleImpactSpeedUsesNormalComponentForWallSlides() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({1.40f, 1.0f, 1.8f}, {0.30f, 2.0f, 5.0f});

    bs3d::VehicleCollisionConfig config;
    config.speed = 12.0f;
    config.lateralSlip = 0.15f;

    const bs3d::VehicleCollisionResult result =
        collision.resolveVehicleCapsule({0.0f, 0.0f, 0.0f},
                                        {1.2f, 0.0f, 2.0f},
                                        0.0f,
                                        config);

    expect(result.hit, "vehicle contacts the side wall while sliding forward");
    expect(result.impactSpeed < 4.0f,
           "impact speed reflects movement into the wall, not full forward slide speed");
}

void collisionDebugSnapshotIncludesStaticDynamicAndGroundShapes() {
    bs3d::WorldCollision collision;
    collision.clear();
    collision.addGroundPlane(0.0f);
    collision.addBox({0.0f, 1.0f, 0.0f}, {1.0f, 2.0f, 1.0f});
    collision.addOrientedBox({2.0f, 1.0f, 0.0f}, {1.0f, 2.0f, 1.0f}, 0.4f, bs3d::CollisionProfile::solidWorld());
    collision.addDynamicBox({4.0f, 1.0f, 0.0f}, {1.0f, 2.0f, 1.0f}, bs3d::CollisionProfile::playerBlocker());
    collision.addDynamicOrientedBox({6.0f, 1.0f, 0.0f}, {1.0f, 2.0f, 1.0f}, 0.2f, bs3d::CollisionProfile::playerBlocker());
    collision.addDynamicGroundBox({8.0f, 0.0f, 0.0f}, {1.0f, 0.5f, 1.0f}, 0.0f, bs3d::CollisionProfile::walkableSurface());

    const bs3d::CollisionDebugSnapshot snapshot = collision.debugSnapshot();
    expect(snapshot.staticBoxes.size() == 1, "debug snapshot includes static AABB boxes");
    expect(snapshot.staticOrientedBoxes.size() == 1, "debug snapshot includes static oriented boxes");
    expect(snapshot.dynamicBoxes.size() == 1, "debug snapshot includes dynamic AABB boxes");
    expect(snapshot.dynamicOrientedBoxes.size() == 1, "debug snapshot includes dynamic oriented boxes");
    expect(snapshot.groundSurfaces.size() == 2, "debug snapshot includes static and dynamic ground surfaces");
}

void playerCanWalkOffVehicleRoofWithoutBeingPinnedByVehicleBody() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addDynamicVehiclePlayerCollision({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::PlayerMotor motor;
    motor.reset({0.72f, 0.90f, 0.0f}, 0.0f);
    motor.forceGrounded();

    bs3d::PlayerInputIntent intent;
    intent.moveDirection = {1.0f, 0.0f, 0.0f};

    for (int i = 0; i < 32; ++i) {
        motor.update(intent, collision, 1.0f / 60.0f);
    }

    expect(motor.state().position.x > 1.35f,
           "player can walk off vehicle roof instead of being pinned by vehicle body");
    expect(motor.state().position.y < 0.90f,
           "player starts falling after leaving the roof walkable surface");
}

void playerCanMoveWhileStandingOnVehicleTop() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addDynamicVehiclePlayerCollision({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.92f, 0.0f}, 0.0f);
    motor.forceGrounded();

    bs3d::PlayerInputIntent intent;
    intent.moveDirection = {0.0f, 0.0f, 1.0f};

    for (int i = 0; i < 30; ++i) {
        motor.update(intent, collision, 1.0f / 60.0f);
    }

    expect(motor.state().position.z > 0.85f,
           "player can walk across vehicle top instead of rotating in place");
    expect(!motor.state().hitWallThisFrame,
           "vehicle top movement is not treated as a wall hit");
}

void playerCanLandOnVehicleThenKeepMoving() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addDynamicVehiclePlayerCollision({0.0f, 0.0f, 0.0f}, 0.0f);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 1.85f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent idle;
    for (int i = 0; i < 90; ++i) {
        motor.update(idle, collision, 1.0f / 60.0f);
    }

    expect(motor.state().grounded, "player lands on vehicle roof instead of staying in a broken airborne state");
    expectNear(motor.state().position.y, 0.92f, 0.08f, "landing snap chooses the vehicle roof height");

    bs3d::PlayerInputIntent move;
    move.moveDirection = {0.0f, 0.0f, 1.0f};
    const float startZ = motor.state().position.z;
    for (int i = 0; i < 30; ++i) {
        motor.update(move, collision, 1.0f / 60.0f);
    }

    expect(motor.state().position.z > startZ + 0.70f,
           "player can keep walking after landing on vehicle roof");
}

void playerStandingOnSlowVehicleIsCarriedByPlatformVelocity() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.92f, 0.0f}, 0.0f);

    bs3d::VehiclePlayerCollisionConfig vehicleSupport;
    vehicleSupport.platformVelocity = {1.0f, 0.0f, 0.0f};

    bs3d::Vec3 vehiclePosition{};
    bs3d::PlayerInputIntent idle;
    for (int i = 0; i < 60; ++i) {
        collision.clearDynamic();
        collision.addDynamicVehiclePlayerCollision(vehiclePosition, 0.0f, vehicleSupport);
        motor.update(idle, collision, 1.0f / 60.0f);
        vehiclePosition = vehiclePosition + vehicleSupport.platformVelocity * (1.0f / 60.0f);
    }

    expect(motor.state().ground.ownerId != 0, "vehicle roof reports dynamic support owner");
    expect(motor.state().position.x > 0.70f,
           "slow vehicle platform carries standing player instead of leaving them embedded");
}

void playerPlatformCarryResolvesAgainstWorldBlockers() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addBox({0.90f, 0.90f, 0.0f}, {0.20f, 1.80f, 4.0f});

    bs3d::VehiclePlayerCollisionConfig vehicleSupport;
    vehicleSupport.platformVelocity = {1.0f, 0.0f, 0.0f};
    collision.addDynamicVehiclePlayerCollision({0.0f, 0.0f, 0.0f}, 0.0f, vehicleSupport);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.92f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent idle;
    motor.update(idle, collision, 1.0f);

    expect(motor.state().position.x < 0.45f,
           "platform carry is resolved through character collision instead of pushing through a wall");
    expect(motor.state().hitWallThisFrame, "platform carry reports the blocker hit for reaction/debug systems");
}

void fastVehicleUnderPlayerPushesThemOffInsteadOfCagingThem() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    bs3d::VehiclePlayerCollisionConfig vehicleSupport;
    vehicleSupport.platformVelocity = {7.0f, 0.0f, 0.0f};
    collision.addDynamicVehiclePlayerCollision({0.0f, 0.0f, 0.0f}, 0.0f, vehicleSupport);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 0.92f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent idle;
    motor.update(idle, collision, 1.0f / 60.0f);

    expect(!motor.state().grounded || motor.state().velocity.x > 0.5f,
           "fast vehicle support creates controlled push/fall-off instead of a stuck roof state");
}

void vehicleRoofMovementIgnoresOwnBodySkirts() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);

    bs3d::VehiclePlayerCollisionConfig config;
    config.bodySize.y = 1.35f;
    config.topHeight = 0.92f;
    collision.addDynamicVehiclePlayerCollision({0.0f, 0.0f, 0.0f}, 0.0f, config);

    bs3d::PlayerMotor motor;
    motor.reset({0.0f, 1.85f, 0.0f}, 0.0f);

    bs3d::PlayerInputIntent idle;
    for (int i = 0; i < 90; ++i) {
        motor.update(idle, collision, 1.0f / 60.0f);
    }
    expect(motor.state().grounded, "player lands on the vehicle roof");

    bs3d::PlayerInputIntent move;
    move.moveDirection = {0.0f, 0.0f, 1.0f};
    const float startZ = motor.state().position.z;
    for (int i = 0; i < 60; ++i) {
        motor.update(move, collision, 1.0f / 60.0f);
    }

    expect(motor.state().position.z > startZ + 1.55f,
           "vehicle body/cabin blockers from the same vehicle do not cage the player on its roof");
}

void vehicleTopCollisionLetsPlayerLeaveEveryEdgeNaturally() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addDynamicVehiclePlayerCollision({0.0f, 0.0f, 0.0f}, bs3d::Pi * 0.25f);

    bs3d::CharacterCollisionConfig config;
    const bs3d::Vec3 directions[] = {
        bs3d::forwardFromYaw(bs3d::Pi * 0.25f),
        bs3d::forwardFromYaw(bs3d::Pi * 0.25f) * -1.0f,
        bs3d::screenRightFromYaw(bs3d::Pi * 0.25f),
        bs3d::screenRightFromYaw(bs3d::Pi * 0.25f) * -1.0f,
    };

    for (const bs3d::Vec3& direction : directions) {
        const bs3d::Vec3 from = direction * 0.65f + bs3d::Vec3{0.0f, 0.92f, 0.0f};
        const bs3d::Vec3 desired = from + direction * 1.25f;
        const bs3d::CharacterCollisionResult result = collision.resolveCharacter(from, desired, config);
        expect(!result.hitWall, "vehicle top does not cage the player at roof edges");
        expect(bs3d::distanceXZ(result.position, from) > 0.75f,
               "player can keep moving off each roof/hood edge");
    }
}

void orientedBoxCollisionUsesYawForRuntimeProps() {
    bs3d::WorldCollision collision;
    collision.addGroundPlane(0.0f);
    collision.addOrientedBox({0.0f, 0.5f, 0.0f},
                             {1.0f, 1.0f, 4.0f},
                             bs3d::Pi * 0.5f,
                             bs3d::CollisionProfile::solidWorld());

    expect(collision.isCircleBlocked({1.6f, 0.0f, 0.0f}, 0.45f),
           "yawed oriented box blocks along rotated long axis");
    expect(!collision.isCircleBlocked({0.0f, 0.0f, 1.6f}, 0.45f),
           "yawed oriented box does not block where unrotated long axis would have been");
}

void worldCollisionBuildsBroadphaseForStaticAndDynamicBlockers() {
    bs3d::WorldCollision collision;
    collision.clear();
    collision.addGroundPlane(0.0f);
    for (int i = 0; i < 64; ++i) {
        collision.addBox({static_cast<float>(i * 4), 0.5f, 0.0f},
                         {1.0f, 1.0f, 1.0f},
                         bs3d::CollisionProfile::solidWorld());
    }
    collision.addDynamicBox({0.0f, 0.5f, 6.0f}, {1.0f, 1.0f, 1.0f}, bs3d::CollisionProfile::dynamicProp());

    const bs3d::CollisionBroadphaseStats stats = collision.broadphaseStats();
    expect(stats.staticShapeCount >= 64, "broadphase tracks static shapes");
    expect(stats.dynamicShapeCount >= 1, "broadphase tracks dynamic shapes");
    expect(stats.staticBucketCount > 1, "broadphase partitions static world into more than one bucket");

    const int nearCandidates = collision.debugBroadphaseCandidateCount({0.0f, 0.0f, 0.0f}, 0.55f, bs3d::CollisionMasks::Player);
    expect(nearCandidates < 16, "near player query does not inspect every authored blocker");
    expect(collision.isCircleBlocked({0.0f, 0.0f, 0.0f}, 0.55f, bs3d::CollisionMasks::Player),
           "broadphase-backed query still detects nearby collision");
}

void collisionUsesBroadphaseForCameraBoomSegments() {
    bs3d::WorldCollision collision;
    collision.clear();
    for (int i = 0; i < 96; ++i) {
        collision.addBox({static_cast<float>(i * 5 + 40), 1.0f, 0.0f},
                         {1.0f, 2.0f, 1.0f},
                         bs3d::CollisionProfile::cameraBlocker());
    }
    collision.addBox({0.0f, 1.0f, -2.0f},
                     {4.0f, 2.0f, 0.4f},
                     bs3d::CollisionProfile::cameraBlocker());

    const int segmentCandidates =
        collision.debugBroadphaseSegmentCandidateCount({0.0f, 1.0f, 0.0f},
                                                       {0.0f, 1.0f, -6.0f},
                                                       0.20f,
                                                       bs3d::CollisionMasks::Camera);
    expect(segmentCandidates < 12, "camera boom segment query does not inspect every authored blocker");

    const bs3d::Vec3 resolved =
        collision.resolveCameraBoom({0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, -6.0f}, 0.2f);
    expect(resolved.z > -2.0f, "broadphase-backed camera boom still detects near blocker");
}

} // namespace

int main() {
    try {
        missionAdvancesThroughDrivingErrand();
        missionFailureCanRetryFromVehicleObjective();
        missionFailureCanRetryToSerializedCheckpointPhase();
        missionTriggerSystemFiresDataDrivenPhaseZonesOnce();
        missionObjectiveTextIsShortCanonAndPlaceholderFree();
        missionCanonDialogueUsesReadableDurationsAndNoOldSpeakers();
        missionControllerUsesChannelsForHintsCriticalAndFail();
        storyStateFlagsAdvanceOnlyFromMissionEvents();
        paragonMissionBranchesIntoPeacefulTrickAndChaosOutcomes();
        storyStateTracksParagonRuntimeConsequencesWithoutSaveGame();
        saveGameRoundTripsStoryProgressCheckpointAndRuntimeState();
        saveGameRoundTripsActiveWorldMemoryEvents();
        saveGameRoundTripsWorldReactionRhythm();
        saveGameRoundTripsCompletedLocalRewirFavors();
        saveGameValidationRejectsCorruptRuntimeState();
        saveGameFileWritesAreAtomicAndValidatedOnLoad();
        missionControllerCanRestoreSaveStateAndUseDataObjectiveOverrides();
        paragonMissionCanRestorePersistedConsequences();
        retryCheckpointSnapshotRestoresWorldHeatEventsAndCooldowns();
        saveGameRewirPressureReliefLongTailPersistsAcrossReload();
        fixedStepClockClampsLargeFramesAndReportsSubsteps();
        renderInterpolationClampsAlphaAndUsesShortestYawPath();
        renderInterpolationBlendsVehicleAndCameraState();
        paragonMissionSpecAuthorsActorsChoicesEventsAndConsequences();
        paragonMissionPolishUsesReadableObjectivesAndPostChaosShopLine();
        chaseSystemEscapesOnlyAfterSustainedDistance();
        chaseRequiresCaughtGraceBeforeFailing();
        chaseStartGraceBlocksImmediateCaughtFail();
        chaseSuggestsFairRubberbandFollowSpeed();
        chaseEscapeDistanceFitsSmallRewirLoop();
        chaseContactRecoverAndFastImpactDoNotInstantlyFailMission();
        dialogueAdvancesAfterDuration();
        dialogueChannelsKeepCriticalLinesAboveReactions();
        gameplayCameraModeIgnoresSubtitleLinesUnlessInteractionCameraIsExplicit();
        stableCameraModeForcesCleanFramingExceptExplicitInteraction();
        controlContextMapsSpaceAsJumpOnFootAndHandbrakeInVehicle();
        controlContextMapsShiftAsSprintOnFootAndBoostInVehicle();
        controlContextLocksMovementDuringHardInteraction();
        controlContextSupportsVehicleInteractionAliasAndHornFamily();
        controlContextMapsOnFootCharacterActionButtons();
        rawInputFrameEdgesAreClearedAfterFirstFixedStep();
        controlContextKeepsCharacterActionsOutOfVehicleAndHardLocks();
        controlContextResolutionPrioritizesLocksBeforeVehicle();
        interactionLockedInputReachesPlayerMotorState();
        vehicleMovesForwardAndClampsSpeed();
        vehicleHandbrakeSharpensTurningWithoutBreakingSpeedClamp();
        vehicleSteersLeftAndRightWithExpectedYawSigns();
        vehicleLowSpeedSteeringHasReadableAuthority();
        vehicleFrontWheelsSteerAtStandstillWithoutRotatingBody();
        vehicleUrbanSpeedTurnRadiusFeelsLikePassengerCar();
        vehicleAccelerationBuildsLikeHeavierGruz();
        vehicleSteeringRampsInBeforeFullLock();
        vehicleFakeGearboxCreatesReadableShifts();
        vehicleSurfaceResponseMakesGrassSlowerAndLooser();
        vehicleForwardDiagonalMovesInPressedDirection();
        vehicleForwardMatchesVisibleFrontConvention();
        vehiclePreviewMoveDoesNotMutateUntilResolutionIsApplied();
        vehicleApplyMoveResolutionAppliesImpactOnlyForRealHits();
        vehicleReverseSteeringKeepsArcadeScreenDirection();
        vehicleBrakesBeforeEnteringReverse();
        driveRouteGuideAdvancesThroughParkingRoute();
        driveRouteGuideCanAimVehicleSpawnTowardFirstPoint();
        vehicleBoostIncreasesAccelerationButKeepsSpeedClamped();
        vehicleSteersMoreAtParkingSpeedThanHighSpeed();
        vehicleHandbrakeAddsSlipAndInstabilityWithoutBreakingClamp();
        feelGateVehicleHandbrakeSlipRecoversAfterRelease();
        vehicleHandbrakeDriftEntersControlledSlideWithoutKillingSpeed();
        vehicleCounterSteerRecoversDriftWithoutSnapping();
        vehicleDriftFallsOutCleanlyAtLowSpeed();
        vehicleConditionDropsAndChangesBandAfterImpacts();
        damagedVehicleStillDrivesButFeelsWeaker();
        feelGateDamagedVehicleBoostAndSteeringRemainReadable();
        vehicleHornPulsesWithCooldownWithoutMovingCar();
        vehicleDoesNotPivotInPlaceWithoutSpeed();
        vehicleBrakeWinsWhenGasAndBrakeAreHeld();
        vehicleWheelVisualsSpinFromSpeedAndReverseDirection();
        vehicleWheelVisualsSteerFrontWheelsAndCenterWhenReleased();
        vehicleWheelVisualsShowBrakeDiveAndSlipLoad();
        vehicleYawStaysBoundedDuringLongArcadeSteering();
        playerWalksAndRespectsCollision();
        playerAcceleratesAndDeceleratesSmoothly();
        playerControllerExposesDistinctOnFootSpeedCaps();
        playerControllerSettersPreserveMotorRuntimeState();
        playerControllerVehicleSeatSyncClearsStaleMotionButKeepsStatus();
        playerControllerVehicleExitSyncClearsStaleMotionAndKeepsStatus();
        playerMotorUsesWalkJogAndSprintSpeedStates();
        playerMotorQuickTurnsWithoutLongForwardSlide();
        feelGatePlayerStopsWithinShortReadableDistanceAfterSprint();
        characterStatusPolicyUsesChaseAndHighPrzypalForPanic();
        characterStatusPolicyDrivesMotorPanicFromPrzypal();
        feelGatePlayerSlidesAlongWallWithoutLongStaggerLock();
        playerMotorWallBumpKeepsShortOutwardImpulseAfterCollisionResolution();
        playerMotorUpwardStaggerImpulseLeavesGroundImmediately();
        playerMovesRelativeToCameraYaw();
        playerStrafesRelativeToCameraYaw();
        playerInputMapsWasdConsistentlyAcrossCardinalCameraYaws();
        playerInputMapsDToRaylibScreenRight();
        screenRightFromYawMatchesRaylibViewConvention();
        vehicleInputMapsDToRaylibScreenRightAtDefaultCamera();
        playerInputMapsDiagonalWasdConsistentlyAcrossCardinalCameraYaws();
        cameraRigAppliesMouseLookBeforePlayerMovement();
        cameraRigMouseRightTurnsViewDirectionRight();
        cameraRigManualMouseLookKeepsVisualYawSyncedWithControlYaw();
        cameraRigCanInvertMouseXForOrbitPreference();
        cameraRigMouseSensitivityIsControlledForDesktopLook();
        playerSprintsJumpsAndLands();
        playerMotorMovesFromIntentAndReportsSpeedStates();
        playerMotorDeceleratesPredictablyAfterInputRelease();
        playerMotorSupportsCoyoteJumpButRejectsDoubleJump();
        playerMotorBuffersJumpJustBeforeLanding();
        worldCollisionStepOffsetAllowsLowCurbAndBlocksHighCurb();
        worldCollisionSlopeLimitAllowsGentleRampAndBlocksSteepRamp();
        worldCollisionSlidesAlongWallsWithoutSticking();
        worldCollisionReportsCharacterHitOwnerForNpcBodies();
        playerMotorExposesCharacterHitOwnerForPresentationAndReactions();
        playerPresentationAddsLeanBobAndLandingDip();
        playerMotorStatusEffectsKeepComedyResponsive();
        playerMotorShortStaggerExpiresWithoutLongControlLoss();
        playerPresentationShowsPanicSprintAndStagger();
        playerPresentationShowsTiredAndBruisedStatusShape();
        playerPresentationSelectsRuntimeCharacterPoseKinds();
        playerPresentationProducesReadableLimbAnimationSignals();
        playerPresentationUsesActionContextWithoutMutatingMotorContract();
        characterPoseCatalogCoversRequestedRuntimeAnimationHooks();
        playerPresentationOneShotActionsTemporarilyOverrideBasePose();
        playerPresentationExposesForwardOneShotNormalizedTime();
        playerPresentationVehicleSteerAndEnterExitUseActionPoses();
        playerPresentationUsesStartStopAndQuickTurnTransitionPoses();
        playerPresentationUsesJumpOffVehicleWhenLeavingDynamicSupport();
        playerPresentationExposesDistinctActionPoseChannels();
        playerPresentationUsesPersistentCarryPoseWhileHoldingObject();
        characterActionMapperTurnsRuntimeInputsIntoReadablePoses();
        characterActionMapperQueuesGetUpAfterFallLite();
        characterActionStartPolicyRequiresVaultOpportunityForLowVault();
        characterActionStartPolicyAllowsNormalActionsWithoutVaultOpportunity();
        characterActionStartPolicyRequiresPushTargetForPushObject();
        characterLowVaultRequiresLowObstacleAhead();
        characterLowVaultRejectsHighWalls();
        characterLowVaultRequiresClearLandingSpace();
        characterLowVaultProducesControlledForwardImpulse();
        characterActionMapperUsesCinematicPriorityForConflictingInputs();
        characterPhysicalReactionKeepsSmallBumpsShortAndControllable();
        characterPhysicalReactionVehicleHitFallsLiteAndQueuesGetUp();
        characterPhysicalReactionAppliesToMotorAndPresentation();
        characterPhysicalReactionApplicationReturnsFeedbackAndFollowUpContract();
        cameraRigSmoothlyFollowsTargetWithoutTeleporting();
        cameraRigOnFootNormalUsesCloserReadableFraming();
        cameraRigOnFootLookAheadStaysSmallForStableWalkingBaseline();
        cameraRigOnFootTargetDoesNotRubberBandDuringJog();
        cameraRigSupportsOrbitAndDrivingRecenter();
        cameraRigVehicleSpeedExpandsFovAndBoomForEscapeReadability();
        cameraRigVehicleLowSpeedTargetDoesNotDanceForward();
        cameraRigUsesNaturalMouseYAndTrueOrbitPitch();
        cameraRigDrivingRecentersAfterMouseLookEnds();
        cameraRigVehicleResetStartsBehindVehicleYaw();
        cameraRigDoesNotFightJoggingWithOnFootAutoRecenter();
        cameraRigRecentersOnFootOnlyForSprintPressure();
        cameraRigSprintAutoAlignWaitsBeforeHelping();
        cameraRigOnFootTrackingLagStaysTightAtSprintSpeed();
        feelGateCameraExposesTargetLagForDebugOverlay();
        cameraRigDoesNotSnapBoomJustBecauseTargetMovedWithCollisionProbe();
        feelGateCameraBoomReturnsOutSmoothlyAfterWallClears();
        cameraRigShortensBoomAgainstWorldCollision();
        cameraRigDoesNotRemainOccludedWhenBoomObstacleAppears();
        cameraBoomCanResolveVeryCloseObstacles();
        cameraRigTensionMakesSprintCameraFeelPanicked();
        cameraRigInteractionModeFramesCloserThanNormal();
        feedbackEventsDecayOverTime();
        feedbackCameraTensionIgnoresPrzypalHeatDuringFeelGate();
        feedbackPrzypalNoticePulsesHudWithoutCameraShake();
        feedbackShortComedyShakeAfterLongShakeStartsAtFullStrength();
        stableCameraFeedbackDisablesNonChaseCameraEffects();
        feedbackComedyEventsCooldownAndExpire();
        worldEventLedgerStoresMergesAndExpiresEvents();
        activeWorldEventDoesNotAddPrzypalAgainWithoutNewPulse();
        worldEventLedgerMergesNearbyRepeatsWithDampenedHeatAndStackCap();
        worldEventLedgerQueriesByTypeLocationAndRadius();
        worldEventLedgerKeepsRewirMemoryLocationsSeparate();
        worldEventLedgerQueriesActiveMemoryByLocationAndSource();
        worldEventLedgerCountsActiveMemoryByRewir();
        worldEventLedgerBuildsStrongestActiveMemoryHotspotsByRewir();
        rewirPressureUsesNearbyStrongMemoryHotspot();
        rewirPressureIgnoresWeakRemoteAndShopMemory();
        shopServiceStateReflectsWorldMemoryAndStoryBan();
        garageServiceStateReflectsLocalRewirPressure();
        caretakerServiceStateReflectsTrashRewirPressure();
        localRewirServiceCatalogDescribesPressureDrivenCivilServices();
        localRewirServiceCatalogCoversEveryPressureEnabledRewir();
        localRewirFavorCatalogCoversEveryLocalService();
        localRewirFavorStateReflectsHotLocalPressure();
        localRewirFavorDigestReportsActiveAndCompletedFavors();
        localRewirServiceStateResolvesByPointAndInteractionId();
        localRewirServiceDigestReportsWaryPressureServices();
        localRewirServiceReliefSoftensOnlyItsOwnRewirMemory();
        przypalRisesFromEventPulseCapsAndDecaysAfterDelay();
        przypalBandsLabelsAndPulseOnlyOncePerCrossing();
        przypalBandPulseOnlyFiresWhenHeatRisesAcrossThreshold();
        worldEventEmitterCooldownsBlockChaseSeenSpamAndFilterTinyDamage();
        npcReactionSystemPicksDeterministicVariantsWithoutImmediateRepeat();
        npcReactionSystemUsesRewirSpecificWitnessesForGarageAndTrashMemory();
        npcReactionSystemUsesRewirSpecificWitnessesForParkingAndRoadMemory();
        npcReactionSystemUsesAgedRewirHotspotsBeforeGenericGossip();
        npcReactionSystemTurnsLingeringRewirPressureIntoPatrolHintAfterWitnessBeat();
        missionUiBusyBlocksLowPriorityGossipButKeepsLedgerMemory();
        worldReactionsDoNotMutateInputMovementOrCameraState();
        worldInteractionFindsNearestPrompt();
        interactionResolverUsePrioritizesMissionNpcOverNearbyVehicle();
        interactionResolverUsesDistanceForSamePriorityCandidates();
        interactionResolverFiltersDisabledOutOfRangeAndWrongInput();
        sceneCreatesAndFindsTransformsByEntityId();
        collisionPreventsCircleEnteringBox();
        circleCollisionDoesNotTunnelThroughThinBoxOnLargeMove();
        vehicleCapsuleDoesNotTunnelThroughThinBlockerOnLargeMove();
        collisionLayersSeparatePlayerVehicleCameraAndTriggers();
        broadphaseHandlesNegativeCoordinateCells();
        staticAndDynamicGroundBoxesUseSameBaseHeightSemantics();
        dynamicVehicleRoofIsWalkableWithoutBlockingPlayerOnTop();
        characterWorldHitPolicyConvertsNpcBodyHitToReactionEvent();
        characterWorldHitPolicyIgnoresAnonymousOrVerySoftHits();
        vehicleRoofSupportCoversVisibleBodyEdgesForCharacterController();
        vehicleTopGroundDoesNotCreateWideInvisibleLedge();
        multipleVehiclePlayerCollisionOwnersRemainDistinct();
        vehicleImpactSpeedUsesNormalComponentForWallSlides();
        collisionDebugSnapshotIncludesStaticDynamicAndGroundShapes();
        playerCanWalkOffVehicleRoofWithoutBeingPinnedByVehicleBody();
        playerCanMoveWhileStandingOnVehicleTop();
        playerCanLandOnVehicleThenKeepMoving();
        playerStandingOnSlowVehicleIsCarriedByPlatformVelocity();
        playerPlatformCarryResolvesAgainstWorldBlockers();
        fastVehicleUnderPlayerPushesThemOffInsteadOfCagingThem();
        vehicleRoofMovementIgnoresOwnBodySkirts();
        vehicleTopCollisionLetsPlayerLeaveEveryEdgeNaturally();
        orientedBoxCollisionUsesYawForRuntimeProps();
        worldCollisionBuildsBroadphaseForStaticAndDynamicBlockers();
        collisionUsesBroadphaseForCameraBoomSegments();
        vehicleCapsuleCollisionChecksFrontCenterAndRear();
        vehicleCapsuleReportsSideHitZonesAndProfile();
        vehicleCapsuleSlidesAlongWallInsteadOfSticking();
        vehicleCollisionConfigMatchesVisibleBorexFootprint();
        characterCollisionDoesNotTunnelThroughThinWallsOnLargeFrame();
        chaseDifficultyScalesEscapeAndFailParameters();
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }

    std::cout << "bs3d core tests passed\n";
    return 0;
}
