using BlokTools.Core;

var tests = new (string Name, Action Body)[]
{
    ("mission document store loads current mission with dialogue", MissionDocumentStoreLoadsCurrentMissionWithDialogue),
    ("mission document validation rejects broken graph", MissionDocumentValidationRejectsBrokenGraph),
    ("mission document validation rejects unsupported phase", MissionDocumentValidationRejectsUnsupportedPhase),
    ("mission editor session saves valid edits with backup", MissionEditorSessionSavesValidEditsWithBackup),
    ("mission editor session refuses invalid saves", MissionEditorSessionRefusesInvalidSaves),
    ("mission editor session adds steps and dialogue", MissionEditorSessionAddsStepsAndDialogue),
    ("mission editor session suggests runtime phase for new steps", MissionEditorSessionSuggestsRuntimePhaseForNewSteps),
    ("mission dialogue line with lineKey only passes validation", MissionDocumentValidationAllowsDialogueLineKey),
    ("mission npc reaction line with lineKey only passes validation", MissionDocumentValidationAllowsNpcReactionLineKey),
    ("mission cutscene line with lineKey only passes validation", MissionDocumentValidationAllowsCutsceneLineKey),
    ("mission document validation rejects unknown localization lineKey", MissionDocumentValidationRejectsUnknownLocalizationLineKey),
    ("mission editor session binds step trigger to object outcome", MissionEditorSessionBindsStepTriggerToObjectOutcome),
    ("mission editor session adds npc reaction and cutscene", MissionEditorSessionAddsNpcReactionAndCutscene),
    ("mission editor session rejects unknown outcome triggers", MissionEditorSessionRejectsUnknownOutcomeTriggers),
    ("mission document validation accepts concrete outcome covered by pattern", MissionDocumentValidationAcceptsConcreteOutcomeCoveredByPattern),
    ("shop catalog fixture validates and loads", ShopCatalogFixtureLoadsAndValidates),
    ("object outcome catalog loads stable object hooks", ObjectOutcomeCatalogLoadsStableObjectHooks),
    ("object outcome catalog validates world event metadata", ObjectOutcomeCatalogValidatesWorldEventMetadata),
    ("object outcome catalog requires player-facing feedback", ObjectOutcomeCatalogRequiresPlayerFacingFeedback),
    ("workspace snapshot presents mission editor surface", WorkspaceSnapshotPresentsMissionEditorSurface),
    ("workspace loader validates mission outcome triggers against catalog", WorkspaceLoaderValidatesMissionOutcomeTriggersAgainstCatalog),
    ("ci verify builds BlokTools app", CiVerifyBuildsBlokToolsApp),
    ("ci verify builds dev-tools preset", CiVerifyBuildsDevToolsPreset),
    ("ci verify runs dev-tools tests", CiVerifyRunsDevToolsTests),
    ("ci verify runs game smoke", CiVerifyRunsGameSmoke),
    ("github actions workflow runs quality gate", GitHubActionsWorkflowRunsQualityGate),
    ("ci verify fails on native command errors", CiVerifyFailsOnNativeCommandErrors),
    ("run dev fails on native command errors", RunDevFailsOnNativeCommandErrors),
    ("run release smoke fails on native command errors", RunReleaseSmokeFailsOnNativeCommandErrors),
};

var failures = 0;
foreach (var test in tests)
{
    try
    {
        test.Body();
        Console.WriteLine($"PASS {test.Name}");
    }
    catch (Exception error)
    {
        failures++;
        Console.WriteLine($"FAIL {test.Name}");
        Console.WriteLine(error.Message);
    }
}

if (failures > 0)
{
    Console.WriteLine($"{failures} test(s) failed.");
    return 1;
}

Console.WriteLine($"{tests.Length} test(s) passed.");
return 0;

static void MissionDocumentStoreLoadsCurrentMissionWithDialogue()
{
    var root = WorkspaceRoot();
    var mission = MissionDocumentStore.Load(Path.Combine(root, "data", "mission_driving_errand.json"));

    AssertEqual("driving_errand_vertical_slice", mission.Id, "mission id");
    AssertEqual("Szybki kurs przez osiedle", mission.Title, "mission title");
    AssertEqual(6, mission.Steps.Count, "mission step count");
    AssertEqual("WalkToShop", mission.Steps[0].Phase, "first phase");
    AssertEqual("shop_walk_intro", mission.Steps[0].Trigger, "first trigger");
    AssertEqual("ReturnToLot", mission.Steps[^1].Phase, "last phase");
    AssertEqual(5, mission.Dialogue.Count, "dialogue count");
    AssertEqual("Boguś", mission.Dialogue[0].Speaker, "first dialogue speaker");
    AssertEqual(3, mission.NpcReactions.Count, "mission npc reaction count");
    AssertEqual(1, mission.Cutscenes.Count, "mission cutscene count");
}

static void MissionDocumentValidationRejectsBrokenGraph()
{
    var mission = new MissionDocument
    {
        Id = "broken",
        Title = "Broken",
        Steps =
        {
            new MissionStep { Phase = "ReachVehicle", Objective = "", Trigger = "" },
        },
        Dialogue =
        {
            new MissionDialogueLine { Speaker = "", Text = "Line without speaker.", DurationSeconds = 0.0 },
        },
    };

    var issues = MissionDocumentValidator.Validate(mission).ToList();

    AssertIssue(issues, "mission.dialogue.phase");
    AssertIssue(issues, "mission.step.objective");
    AssertIssue(issues, "mission.step.trigger");
    AssertIssue(issues, "mission.dialogue.speaker");
    AssertIssue(issues, "mission.dialogue.duration");
}

static void MissionDocumentValidationAllowsDialogueLineKey()
{
    var mission = new MissionDocument
    {
        Id = "line-key",
        Title = "Line Key Test",
        Steps =
        {
            new MissionStep { Phase = "ReachVehicle", Objective = "Test", Trigger = "player_enters_vehicle" },
        },
        Dialogue =
        {
            new MissionDialogueLine { Phase = "ReachVehicle", Speaker = "Misja", LineKey = "dlg_test_line", Text = "", DurationSeconds = 2.2 },
        },
    };

    var issues = MissionDocumentValidator.Validate(mission, null).ToList();

    AssertTrue(issues.Count == 0, "dialogue line with lineKey should be valid");
}

static void MissionDocumentValidationAllowsNpcReactionLineKey()
{
    var mission = new MissionDocument
    {
        Id = "line-key",
        Title = "NPC Reaction Test",
        Steps =
        {
            new MissionStep { Phase = "ReachVehicle", Objective = "Test", Trigger = "player_enters_vehicle" },
        },
        NpcReactions =
        {
            new MissionNpcReaction { Phase = "ReachVehicle", Speaker = "Bogus", LineKey = "npc_test_line", DurationSeconds = 1.9 },
        },
    };

    var issues = MissionDocumentValidator.Validate(mission, null).ToList();

    AssertTrue(issues.Count == 0, "npc reaction line with lineKey should be valid");
}

static void MissionDocumentValidationAllowsCutsceneLineKey()
{
    var mission = new MissionDocument
    {
        Id = "line-key",
        Title = "Cutscene Test",
        Steps =
        {
            new MissionStep { Phase = "ReachVehicle", Objective = "Test", Trigger = "player_enters_vehicle" },
        },
        Cutscenes =
        {
            new MissionCutsceneLine
            {
                Cutscene = "CutsceneIntro",
                Phase = "ReachVehicle",
                Speaker = "Narrator",
                LineKey = "cutscene_test_line",
                DurationSeconds = 2.2,
            },
        },
    };

    var issues = MissionDocumentValidator.Validate(mission, null).ToList();

    AssertTrue(issues.Count == 0, "cutscene line with lineKey should be valid");
}

static void MissionDocumentValidationRejectsUnsupportedPhase()
{
    var mission = ValidTestMission();
    mission.Steps[0].Phase = "CheckShopDoor";

    var issues = MissionDocumentValidator.Validate(mission).ToList();

    AssertIssue(issues, "mission.step.phase.unknown");
}

static void MissionEditorSessionSavesValidEditsWithBackup()
{
    var path = TempMissionPath();
    var session = MissionEditorSession.Load(path);

    session.UpdateStepObjective(0, "Wsiadz do gruza i nie pytaj.");
    session.UpdateDialoguePhase(0, "ReachVehicle");
    session.UpdateDialogueLineKey(0, "dlg_override_key");
    session.UpdateDialogueText(0, "Nowa linia z edytora.");
    var result = session.Save();
    var saved = MissionDocumentStore.Load(path);

    AssertTrue(result.Saved, "valid mission edit should save");
    AssertTrue(File.Exists(result.BackupPath), "save creates backup");
    AssertEqual("Wsiadz do gruza i nie pytaj.", saved.Steps[0].Objective, "saved objective");
    AssertEqual("Nowa linia z edytora.", saved.Dialogue[0].Text, "saved dialogue text");
    AssertEqual("dlg_override_key", saved.Dialogue[0].LineKey, "saved dialogue line key");
    AssertEqual("ReachVehicle", saved.Dialogue[0].Phase, "saved dialogue phase");
}

static void MissionEditorSessionRefusesInvalidSaves()
{
    var path = TempMissionPath();
    var before = File.ReadAllText(path);
    var session = MissionEditorSession.Load(path);

    session.UpdateStepObjective(0, "");
    var result = session.Save();
    var after = File.ReadAllText(path);

    AssertTrue(!result.Saved, "invalid mission edit should not save");
    AssertIssue(result.Issues, "mission.step.objective");
    AssertEqual(before, after, "invalid save keeps file unchanged");
}

static void MissionEditorSessionAddsStepsAndDialogue()
{
    var path = TempMissionPath();
    var session = MissionEditorSession.Load(path);

    session.AddStep("WalkToShop", "Sprawdz drzwi Zenona.", "outcome:shop_door_checked");
    session.AddDialogueLine("Zenon", "Drzwi drzwiami, ale paragon sam sie nie znajdzie.", 3.4);
    var result = session.Save();
    var saved = MissionDocumentStore.Load(path);

    AssertTrue(result.Saved, "new step and dialogue should save");
    AssertEqual(2, saved.Steps.Count, "saved step count");
    AssertEqual("WalkToShop", saved.Steps[1].Phase, "added phase");
    AssertEqual("outcome:shop_door_checked", saved.Steps[1].Trigger, "added trigger");
    AssertEqual(2, saved.Dialogue.Count, "saved dialogue count");
    AssertEqual("Zenon", saved.Dialogue[1].Speaker, "added dialogue speaker");
}

static void MissionEditorSessionAddsNpcReactionAndCutscene()
{
    var path = TempMissionPath();
    var session = MissionEditorSession.Load(path);

    session.AddNpcReaction("ReachVehicle", "Boguś", "", "NPC uśmiecha się i patrzy w bok.", 2.1);
    session.AddCutsceneLine("Intro", "ReachVehicle", "Narrator", "", "Kamery się kręcą, scena się otwiera.", 2.7);
    var result = session.Save();
    var saved = MissionDocumentStore.Load(path);

    AssertTrue(result.Saved, "new npc reaction and cutscene should save");
    AssertEqual(1, saved.NpcReactions.Count, "saved npc reaction count");
    AssertEqual("Boguś", saved.NpcReactions[0].Speaker, "saved npc reaction speaker");
    AssertEqual(1, saved.Cutscenes.Count, "saved cutscene count");
    AssertEqual("Narrator", saved.Cutscenes[0].Speaker, "saved cutscene speaker");
    AssertEqual("Intro", saved.Cutscenes[0].Cutscene, "saved cutscene id");
}

static void MissionEditorSessionSuggestsRuntimePhaseForNewSteps()
{
    var path = TempMissionPath();
    var session = MissionEditorSession.Load(path);

    var phase = session.AddSuggestedStep("Nowy cel z katalogu faz.", "manual_trigger");
    var result = session.Save();

    AssertTrue(result.Saved, "suggested runtime phase should save");
    AssertEqual("WalkToShop", phase, "suggested phase uses first available runtime-supported phase");
    AssertEqual("WalkToShop", session.Mission.Steps[1].Phase, "suggested phase is applied to mission step");
}

static void MissionEditorSessionBindsStepTriggerToObjectOutcome()
{
    var path = TempMissionPath();
    var catalog = new ObjectOutcomeCatalog
    {
        SchemaVersion = 1,
        Outcomes =
        {
            new ObjectOutcomeDefinition
            {
                Id = "shop_door_checked",
                Label = "Shop door checked",
                Source = "object:shop_door_front",
                Category = "object_use",
                Location = "Shop",
            },
        },
    };
    var session = MissionEditorSession.Load(path, catalog);

    session.BindStepTriggerToOutcome(0, catalog.Outcomes[0]);
    var result = session.Save();
    var saved = MissionDocumentStore.Load(path);

    AssertTrue(result.Saved, "known outcome trigger should save");
    AssertEqual("outcome:shop_door_checked", saved.Steps[0].Trigger, "bound outcome trigger");
}

static void MissionEditorSessionRejectsUnknownOutcomeTriggers()
{
    var path = TempMissionPath();
    var catalog = new ObjectOutcomeCatalog
    {
        SchemaVersion = 1,
        Outcomes =
        {
            new ObjectOutcomeDefinition
            {
                Id = "shop_door_checked",
                Label = "Shop door checked",
                Source = "object:shop_door_front",
                Category = "object_use",
                Location = "Shop",
            },
        },
    };
    var session = MissionEditorSession.Load(path, catalog);

    session.UpdateStepTrigger(0, "outcome:missing_hook");
    var result = session.Save();

    AssertTrue(!result.Saved, "unknown outcome trigger should not save");
    AssertIssue(result.Issues, "mission.step.trigger.outcome");
}

static void MissionDocumentValidationAcceptsConcreteOutcomeCoveredByPattern()
{
    var mission = ValidTestMission();
    mission.Steps[0].Trigger = "outcome:shop_prices_read_shop_price_card_0";
    var catalog = new ObjectOutcomeCatalog
    {
        SchemaVersion = 1,
        Outcomes =
        {
            new ObjectOutcomeDefinition
            {
                IdPattern = "shop_prices_read_*",
                Label = "Shop prices read",
                Source = "tag:shop_price_card",
                Category = "object_use",
                Location = "Shop",
                Speaker = "Kartka",
                Line = "line",
            },
        },
    };

    var issues = MissionDocumentValidator.Validate(mission, catalog).ToList();

    AssertEmpty(issues, "concrete outcome trigger covered by idPattern should validate");
}

static void ShopCatalogFixtureLoadsAndValidates()
{
    var root = WorkspaceRoot();
    var catalog = ShopItemsCatalogStore.Load(Path.Combine(root, "data", "world", "shop_items_catalog.json"));
    var issues = ShopItemsCatalogValidator.Validate(catalog).ToList();

    AssertEqual(1, catalog.SchemaVersion, "shop catalog schema");
    AssertEqual(2, catalog.Shops.Count, "shop fixture shop count");
    AssertEqual(4, catalog.Items.Count, "shop fixture item count");
    AssertEmpty(issues, "shop catalog fixture issues");
    AssertTrue(catalog.Shops.All(shop => !string.IsNullOrWhiteSpace(shop.Id)), "shop ids");
    AssertTrue(catalog.Items.All(item => !string.IsNullOrWhiteSpace(item.ShopId)), "item shop ids");
}

static void ObjectOutcomeCatalogLoadsStableObjectHooks()
{
    var root = WorkspaceRoot();
    var catalog = ObjectOutcomeCatalogStore.Load(Path.Combine(root, "data", "world", "object_outcome_catalog.json"));
    var issues = ObjectOutcomeCatalogValidator.Validate(catalog).ToList();

    AssertEqual(1, catalog.SchemaVersion, "outcome catalog schema");
    AssertEmpty(issues, "object outcome catalog issues");
    AssertTrue(catalog.Outcomes.Any(outcome => outcome.Key == "shop_door_checked"), "catalog exposes shop door outcome");
    AssertTrue(catalog.Outcomes.Any(outcome => outcome.Key == "garage_door_checked_*"), "catalog exposes garage door outcome pattern");
    AssertTrue(catalog.Outcomes.Any(outcome => outcome.Key == "shop_rolling_grille_checked"), "catalog exposes shop grille outcome");
    AssertTrue(catalog.Outcomes.Any(outcome => outcome.Key == "block_intercom_buzzed"), "catalog exposes block intercom outcome");
    AssertTrue(catalog.Outcomes.Any(outcome => outcome.Key == "shop_notice_read_*"), "catalog exposes quiet shop notice outcome pattern");
    AssertTrue(catalog.Outcomes.Any(outcome => outcome.Key == "block_notice_read_*"), "catalog exposes quiet block notice outcome pattern");
    var intercom = catalog.Outcomes.Single(outcome => outcome.Key == "block_intercom_buzzed");
    AssertTrue(intercom.WorldEvent is not null, "intercom outcome exposes world event metadata");
    AssertEqual("PublicNuisance", intercom.WorldEvent!.Type, "intercom world event type");
    AssertTrue(intercom.WorldEvent.Intensity >= 0.12 && intercom.WorldEvent.Intensity <= 0.28,
        "intercom world event intensity mirrors runtime low-stakes nuisance");
    AssertTrue(catalog.Outcomes.Single(outcome => outcome.Key == "shop_prices_read_*").WorldEvent is null,
        "quiet shop price outcome stays without world event metadata");
    AssertTrue(catalog.Outcomes.Single(outcome => outcome.Key == "shop_notice_read_*").WorldEvent is null,
        "quiet shop notice outcome stays without world event metadata");
    AssertTrue(catalog.Outcomes.Single(outcome => outcome.Key == "block_notice_read_*").WorldEvent is null,
        "quiet block notice outcome stays without world event metadata");
    AssertTrue(catalog.Outcomes.All(outcome => !string.IsNullOrWhiteSpace(outcome.Location)), "outcomes expose locations");
    AssertTrue(catalog.Outcomes.All(outcome => !string.IsNullOrWhiteSpace(outcome.Label)), "outcomes expose editor labels");
}

static void ObjectOutcomeCatalogValidatesWorldEventMetadata()
{
    var catalog = new ObjectOutcomeCatalog
    {
        SchemaVersion = 1,
        Outcomes =
        {
            new ObjectOutcomeDefinition
            {
                Id = "bad_event",
                Label = "Bad event",
                Source = "object:test",
                Category = "object_use",
                Location = "Block",
                WorldEvent = new ObjectOutcomeWorldEvent
                {
                    Type = "Noise",
                    Intensity = 1.4,
                    CooldownSeconds = 0.0,
                },
            },
        },
    };

    var issues = ObjectOutcomeCatalogValidator.Validate(catalog).ToList();

    AssertIssue(issues, "outcome.worldEvent.type");
    AssertIssue(issues, "outcome.worldEvent.intensity");
    AssertIssue(issues, "outcome.worldEvent.cooldown");
}

static void ObjectOutcomeCatalogRequiresPlayerFacingFeedback()
{
    var catalog = new ObjectOutcomeCatalog
    {
        SchemaVersion = 1,
        Outcomes =
        {
            new ObjectOutcomeDefinition
            {
                Id = "silent_notice",
                Label = "Silent notice",
                Source = "object:silent_notice",
                Category = "object_use",
                Location = "Block",
                Speaker = "",
                Line = "",
            },
        },
    };

    var issues = ObjectOutcomeCatalogValidator.Validate(catalog).ToList();

    AssertIssue(issues, "outcome.speaker");
    AssertIssue(issues, "outcome.line");
}

static void WorkspaceSnapshotPresentsMissionEditorSurface()
{
    var root = WorkspaceRoot();
    var snapshot = BlokWorkspaceLoader.Load(root);

    AssertEqual("driving_errand_vertical_slice", snapshot.MissionId, "snapshot mission id");
    AssertEqual(6, snapshot.MissionNodes.Count, "snapshot mission node count");
    AssertEqual(3, snapshot.NpcReactions.Count, "snapshot npc reaction count");
    AssertEqual(1, snapshot.CutsceneLines.Count, "snapshot cutscene count");
    AssertEqual(2, snapshot.Shops.Count, "snapshot shop count");
    AssertEqual(4, snapshot.ShopItems.Count, "snapshot shop item count");
    AssertTrue(snapshot.ObjectOutcomeHooks.Count >= 10, "snapshot has outcome hooks from catalog");
    AssertEmpty(snapshot.Issues, "current workspace editor snapshot issues");
    AssertTrue(snapshot.MissionNodes[0].Text.Contains("shop_walk_intro", StringComparison.Ordinal),
        "first mission node includes trigger");
    AssertTrue(snapshot.MissionNodes.Any(node => node.Text.Contains("player_enters_vehicle", StringComparison.Ordinal)),
        "mission nodes include vehicle entry trigger");
    AssertTrue(snapshot.DialogueLines.Any(line => line.Text.Contains("Zenona", StringComparison.Ordinal)),
        "snapshot resolves dialogue lineKey text from mission localization");
    AssertTrue(snapshot.ObjectOutcomeHooks.Any(hook => hook.Text.Contains("shop_door_checked", StringComparison.Ordinal)),
        "snapshot includes shop door hook");
    AssertTrue(snapshot.ObjectOutcomeHooks.Any(hook =>
            hook.Id == "block_intercom_buzzed" &&
            hook.Text.Contains("PublicNuisance", StringComparison.Ordinal) &&
            hook.Text.Contains("cd 3", StringComparison.Ordinal)),
        "snapshot exposes noisy affordance world-event consequence");
    AssertTrue(snapshot.ObjectOutcomeHooks.Any(hook =>
            hook.Id == "shop_prices_read_*" &&
            hook.Text.Contains("quiet", StringComparison.OrdinalIgnoreCase)),
        "snapshot marks quiet affordances as non-eventful");
    AssertTrue(snapshot.ObjectOutcomeHooks.Any(hook =>
            hook.Id == "shop_notice_read_*" &&
            hook.Text.Contains("quiet", StringComparison.OrdinalIgnoreCase)),
        "snapshot exposes quiet shop notice affordance");
    AssertTrue(snapshot.ObjectOutcomeHooks.Any(hook =>
            hook.Id == "block_notice_read_*" &&
            hook.Text.Contains("quiet", StringComparison.OrdinalIgnoreCase)),
        "snapshot exposes quiet block notice affordance");
}

static void WorkspaceLoaderValidatesMissionOutcomeTriggersAgainstCatalog()
{
    var root = TempWorkspacePath();
    var mission = ValidTestMission();
    mission.Steps[0].Trigger = "outcome:missing_hook";
    MissionDocumentStore.Save(Path.Combine(root, "data", "mission_driving_errand.json"), mission);
    ObjectOutcomeCatalogStore.Save(
        Path.Combine(root, "data", "world", "object_outcome_catalog.json"),
        new ObjectOutcomeCatalog
        {
            SchemaVersion = 1,
            Outcomes =
            {
                new ObjectOutcomeDefinition
                {
                    Id = "shop_door_checked",
                    Label = "Shop door checked",
                    Source = "object:shop_door_front",
                    Category = "object_use",
                    Location = "Shop",
                },
            },
        });
    CreateMinimalShopCatalog(root);
    CreateMinimalMissionLocalization(root);

    var snapshot = BlokWorkspaceLoader.Load(root);

    AssertIssue(snapshot.Issues, "mission.step.trigger.outcome");
}

static void MissionDocumentValidationRejectsUnknownLocalizationLineKey()
{
    var mission = ValidTestMission();
    mission.NpcReactions.Add(new MissionNpcReaction { Phase = "ReachVehicle", Speaker = "NPC", DurationSeconds = 2.0 });
    mission.Cutscenes.Add(new MissionCutsceneLine { Cutscene = "test_cutscene", Phase = "ReachVehicle", Speaker = "Narrator", DurationSeconds = 2.0 });
    mission.Dialogue[0].Text = "";
    mission.Dialogue[0].LineKey = "mission.missing";
    mission.NpcReactions[0].Text = "";
    mission.NpcReactions[0].LineKey = "mission.npc_missing";
    mission.Cutscenes[0].Text = "";
    mission.Cutscenes[0].LineKey = "mission.cutscene_missing";
    var localization = new MissionLocalization
    {
        SchemaVersion = 1,
        Lines =
        {
            ["mission.present"] = "Present line.",
        },
    };

    var issues = MissionDocumentValidator.Validate(mission, null, localization).ToList();

    AssertIssue(issues, "mission.dialogue.lineKey");
    AssertIssue(issues, "mission.npcReactions.lineKey");
    AssertIssue(issues, "mission.cutscenes.lineKey");
}

static void CreateMinimalShopCatalog(string root)
{
    ShopItemsCatalogStore.Save(
        Path.Combine(root, "data", "world", "shop_items_catalog.json"),
        new ShopItemsCatalog
        {
            SchemaVersion = 1,
            Shops =
            {
                new ShopDefinition
                {
                    Id = "temp_shop",
                    Name = "temp shop",
                    LocationTag = "TempBlock",
                    Kind = "spozywczy",
                    Owner = "local",
                    IsOpen = true,
                    Tags = new List<string>(),
                },
            },
            Items =
            {
                new ShopItemDefinition
                {
                    Id = "temp_item",
                    ShopId = "temp_shop",
                    Name = "temp item",
                    Category = "sample",
                    Price = 1,
                    Currency = "zl",
                    Description = "fixture product",
                },
            },
        });
}

static void CreateMinimalMissionLocalization(string root)
{
    MissionLocalizationStore.Save(
        Path.Combine(root, "data", "world", "mission_localization_pl.json"),
        new MissionLocalization
        {
            SchemaVersion = 1,
            Lines =
            {
                ["mission.test"] = "Test line.",
                ["mission.bogus_intro"] = "Mission intro.",
            },
        });
}

static void CiVerifyBuildsBlokToolsApp()
{
    var root = WorkspaceRoot();
    var script = File.ReadAllText(Path.Combine(root, "tools", "ci_verify.ps1"));

    AssertTrue(
        script.Contains("BlokTools.App\\BlokTools.App.csproj", StringComparison.Ordinal),
        "ci_verify.ps1 should build the WPF BlokTools app, not only the test project");
}

static void CiVerifyBuildsDevToolsPreset()
{
    var root = WorkspaceRoot();
    var script = File.ReadAllText(Path.Combine(root, "tools", "ci_verify.ps1"));

    AssertTrue(
        script.Contains("--preset dev-tools", StringComparison.Ordinal) &&
        script.Contains("cmake --build --preset dev-tools", StringComparison.Ordinal),
        "ci_verify.ps1 should include the dev-tools build in the quality gate");
}

static void CiVerifyRunsDevToolsTests()
{
    var root = WorkspaceRoot();
    var script = File.ReadAllText(Path.Combine(root, "tools", "ci_verify.ps1"));
    var presets = File.ReadAllText(Path.Combine(root, "CMakePresets.json"));

    AssertTrue(
        presets.Contains("\"name\": \"dev-tools\"", StringComparison.Ordinal) &&
        presets.Contains("\"testPresets\"", StringComparison.Ordinal),
        "CMakePresets.json should expose a dev-tools test preset");
    AssertTrue(
        script.Contains("ctest --preset dev-tools", StringComparison.Ordinal),
        "ci_verify.ps1 should run dev-tools tests after building the dev-tools preset");
}

static void CiVerifyRunsGameSmoke()
{
    var root = WorkspaceRoot();
    var script = File.ReadAllText(Path.Combine(root, "tools", "ci_verify.ps1"));

    AssertTrue(
        script.Contains("ci_smoke.ps1", StringComparison.Ordinal) &&
        script.Contains("-SmokeFrames", StringComparison.Ordinal),
        "ci_verify.ps1 should run the game smoke script as part of the quality gate");
}

static void GitHubActionsWorkflowRunsQualityGate()
{
    var root = WorkspaceRoot();
    var workflowPath = Path.Combine(root, ".github", "workflows", "quality-gate.yml");

    AssertTrue(File.Exists(workflowPath), "root GitHub Actions workflow should exist");
    var workflow = File.ReadAllText(workflowPath);

    AssertTrue(
        workflow.Contains("pull_request:", StringComparison.Ordinal) &&
        workflow.Contains("push:", StringComparison.Ordinal),
        "quality gate workflow should run for PRs and pushes");
    AssertTrue(
        workflow.Contains("windows-latest", StringComparison.Ordinal),
        "quality gate workflow should use a Windows runner for the C++/WPF toolchain");
    AssertTrue(
        workflow.Contains("actions/checkout@v4", StringComparison.Ordinal) &&
        workflow.Contains("actions/setup-dotnet@v4", StringComparison.Ordinal),
        "quality gate workflow should checkout the repo and install the requested .NET SDK");
    AssertTrue(
        workflow.Contains(".\\tools\\ci_verify.ps1 -Preset ci", StringComparison.Ordinal),
        "quality gate workflow should run the same ci_verify.ps1 preset as local CI");
}

static void CiVerifyFailsOnNativeCommandErrors()
{
    var root = WorkspaceRoot();
    var script = File.ReadAllText(Path.Combine(root, "tools", "ci_verify.ps1"));

    AssertTrue(
        script.Contains("$LASTEXITCODE", StringComparison.Ordinal) &&
        script.Contains("throw", StringComparison.Ordinal),
        "ci_verify.ps1 should fail when a native command exits non-zero");
}

static void RunDevFailsOnNativeCommandErrors()
{
    var root = WorkspaceRoot();
    var script = File.ReadAllText(Path.Combine(root, "tools", "run_dev.ps1"));

    AssertTrue(
        script.Contains("Invoke-CheckedNative", StringComparison.Ordinal) &&
        script.Contains("$LASTEXITCODE", StringComparison.Ordinal) &&
        script.Contains("\"--preset\"", StringComparison.Ordinal) &&
        script.Contains("Invoke-CheckedNative $ExePath", StringComparison.Ordinal),
        "run_dev.ps1 should fail when configure, build, or the game executable exits non-zero");
}

static void RunReleaseSmokeFailsOnNativeCommandErrors()
{
    var root = WorkspaceRoot();
    var script = File.ReadAllText(Path.Combine(root, "tools", "run_release_smoke.ps1"));

    AssertTrue(
        script.Contains("Invoke-CheckedNative", StringComparison.Ordinal) &&
        script.Contains("$LASTEXITCODE", StringComparison.Ordinal) &&
        script.Contains("\"--preset\"", StringComparison.Ordinal) &&
        script.Contains("Invoke-CheckedNative $ExePath", StringComparison.Ordinal),
        "run_release_smoke.ps1 should fail when configure, build, or the game executable exits non-zero");
}

static string WorkspaceRoot()
{
    var directory = new DirectoryInfo(AppContext.BaseDirectory);
    while (directory is not null)
    {
        if (File.Exists(Path.Combine(directory.FullName, "README.md")) &&
            Directory.Exists(Path.Combine(directory.FullName, "data")))
        {
            return directory.FullName;
        }
        directory = directory.Parent;
    }
    throw new InvalidOperationException("Could not locate workspace root from test output directory.");
}

static string TempWorkspacePath()
{
    var root = Directory.CreateDirectory(Path.Combine(Path.GetTempPath(), "BlokToolsWorkspaceTests", Guid.NewGuid().ToString("N")));
    Directory.CreateDirectory(Path.Combine(root.FullName, "data", "world"));
    return root.FullName;
}

static string TempMissionPath()
{
    var directory = Directory.CreateDirectory(Path.Combine(Path.GetTempPath(), "BlokToolsTests", Guid.NewGuid().ToString("N")));
    var path = Path.Combine(directory.FullName, "mission_driving_errand.json");
    var mission = ValidTestMission();
    MissionDocumentStore.Save(path, mission);
    return path;
}

static MissionDocument ValidTestMission()
{
    return new MissionDocument
    {
        Id = "test_mission",
        Title = "Test Mission",
        Steps =
        {
            new MissionStep { Phase = "ReachVehicle", Objective = "Old objective", Trigger = "player_enters_vehicle" },
        },
        Dialogue =
        {
            new MissionDialogueLine { Phase = "ReachVehicle", Speaker = "Misja", Text = "Old line", DurationSeconds = 2.5 },
        },
    };
}

static void AssertEqual<T>(T expected, T actual, string label)
{
    if (!EqualityComparer<T>.Default.Equals(expected, actual))
    {
        throw new InvalidOperationException($"{label}: expected '{expected}', got '{actual}'.");
    }
}

static void AssertTrue(bool condition, string label)
{
    if (!condition)
    {
        throw new InvalidOperationException(label);
    }
}

static void AssertEmpty(IReadOnlyCollection<ValidationIssue> issues, string label)
{
    if (issues.Count != 0)
    {
        throw new InvalidOperationException($"{label}: {string.Join("; ", issues.Select(issue => issue.Message))}");
    }
}

static void AssertIssue(IReadOnlyCollection<ValidationIssue> issues, string code)
{
    if (!issues.Any(issue => issue.Code == code))
    {
        throw new InvalidOperationException($"Expected validation issue '{code}'.");
    }
}
