using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Input;
using BlokTools.Core;

namespace BlokTools.App;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        DataContext = MainWindowModel.Load();
    }
}

public sealed class MainWindowModel : INotifyPropertyChanged
{
    private readonly MissionEditorSession session;
    private readonly ShopCatalogEditorSession shopSession;
    private readonly ObjectOutcomeCatalog outcomeCatalog;
    private string status = string.Empty;
    private EditableMissionStepRow? selectedMissionStep;
    private EditorRow? selectedObjectOutcomeHook;
    private EditableShopRow? selectedShop;
    private EditableShopItemRow? selectedShopItem;

    private MainWindowModel(
        MissionEditorSession session,
        ShopCatalogEditorSession shopSession,
        ObjectOutcomeCatalog outcomeCatalog)
    {
        this.session = session;
        this.shopSession = shopSession;
        this.outcomeCatalog = outcomeCatalog;
        SaveCommand = new RelayCommand(Save);
        AddStepCommand = new RelayCommand(AddStep);
        AddDialogueCommand = new RelayCommand(AddDialogue);
        AddNpcReactionCommand = new RelayCommand(AddNpcReaction);
        AddCutsceneCommand = new RelayCommand(AddCutsceneLine);
        AddShopCommand = new RelayCommand(AddShop);
        AddShopItemCommand = new RelayCommand(AddShopItem);
        BindOutcomeCommand = new RelayCommand(BindOutcome);
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public string Header { get; private init; } = string.Empty;

    public string Status
    {
        get => status;
        private set
        {
            status = value;
            OnPropertyChanged();
        }
    }

    public ObservableCollection<EditableMissionStepRow> MissionSteps { get; } = new();
    public ObservableCollection<EditableDialogueLineRow> DialogueLines { get; } = new();
    public ObservableCollection<EditableNpcReactionRow> NpcReactions { get; } = new();
    public ObservableCollection<EditableCutsceneLineRow> CutsceneLines { get; } = new();
    public ObservableCollection<EditableShopRow> Shops { get; } = new();
    public ObservableCollection<EditableShopItemRow> ShopItems { get; } = new();
    public IReadOnlyList<EditorRow> ObjectOutcomeHooks { get; private init; } = Array.Empty<EditorRow>();
    public ObservableCollection<EditorRow> IssueRows { get; } = new();
    public ICommand SaveCommand { get; }
    public ICommand AddStepCommand { get; }
    public ICommand AddDialogueCommand { get; }
    public ICommand AddNpcReactionCommand { get; }
    public ICommand AddCutsceneCommand { get; }
    public ICommand AddShopCommand { get; }
    public ICommand AddShopItemCommand { get; }
    public ICommand BindOutcomeCommand { get; }

    public EditableMissionStepRow? SelectedMissionStep
    {
        get => selectedMissionStep;
        set
        {
            selectedMissionStep = value;
            OnPropertyChanged();
        }
    }

    public EditorRow? SelectedObjectOutcomeHook
    {
        get => selectedObjectOutcomeHook;
        set
        {
            selectedObjectOutcomeHook = value;
            OnPropertyChanged();
        }
    }

    public EditableShopRow? SelectedShop
    {
        get => selectedShop;
        set
        {
            selectedShop = value;
            OnPropertyChanged();
        }
    }

    public EditableShopItemRow? SelectedShopItem
    {
        get => selectedShopItem;
        set
        {
            selectedShopItem = value;
            OnPropertyChanged();
        }
    }

    public static MainWindowModel Load()
    {
        var workspaceRoot = FindWorkspaceRoot(AppContext.BaseDirectory);
        var missionPath = Path.Combine(workspaceRoot, "data", "mission_driving_errand.json");
        var outcomes = ObjectOutcomeCatalogStore.Load(Path.Combine(workspaceRoot, "data", "world", "object_outcome_catalog.json"));
        var localization = MissionLocalizationStore.Load(Path.Combine(workspaceRoot, "data", "world", "mission_localization_pl.json"));
        var shopPath = Path.Combine(workspaceRoot, "data", "world", "shop_items_catalog.json");
        var shopSession = ShopCatalogEditorSession.Load(shopPath);
        var session = MissionEditorSession.Load(missionPath, outcomes, localization);

        var model = new MainWindowModel(session, shopSession, outcomes)
        {
            Header = $"{session.Mission.Title} ({session.Mission.Id})",
            Status = $"Loaded {missionPath}",
            ObjectOutcomeHooks = outcomes.Outcomes
                .Select(outcome => new EditorRow(outcome.Key, ObjectOutcomeFormatter.EditorText(outcome)))
                .ToList(),
        };

        foreach (var step in session.Mission.Steps)
        {
            model.MissionSteps.Add(new EditableMissionStepRow(step.Phase, step.Objective, step.Trigger));
        }

        foreach (var line in session.Mission.Dialogue)
        {
            model.DialogueLines.Add(new EditableDialogueLineRow(line.Phase, line.Speaker, line.LineKey, line.Text, line.DurationSeconds));
        }

        foreach (var reaction in session.Mission.NpcReactions)
        {
            model.NpcReactions.Add(new EditableNpcReactionRow(
                reaction.Phase,
                reaction.Speaker,
                reaction.LineKey,
                reaction.Text,
                reaction.DurationSeconds));
        }

        foreach (var line in session.Mission.Cutscenes)
        {
            model.CutsceneLines.Add(new EditableCutsceneLineRow(
                line.Cutscene,
                line.Phase,
                line.Speaker,
                line.LineKey,
                line.Text,
                line.DurationSeconds));
        }

        foreach (var shop in shopSession.Catalog.Shops)
        {
            model.Shops.Add(
                new EditableShopRow(
                    shop.Id,
                    shop.Name,
                    shop.LocationTag,
                    shop.Kind,
                    shop.Owner,
                    shop.IsOpen,
                    string.Join(", ", shop.Tags)));
        }

        foreach (var item in shopSession.Catalog.Items)
        {
            model.ShopItems.Add(new EditableShopItemRow(
                item.Id,
                item.ShopId,
                item.Name,
                item.Category,
                item.Price,
                item.Currency,
                item.Description));
        }

        model.RefreshIssues(session.Issues.Concat(shopSession.Issues).ToList());
        return model;
    }

    private void AddStep()
    {
        try
        {
            var phase = session.AddSuggestedStep("Nowy cel misji", "manual_trigger");
            MissionSteps.Add(new EditableMissionStepRow(phase, "Nowy cel misji", "manual_trigger"));
            SelectedMissionStep = MissionSteps[^1];
            RefreshIssues(session.Issues);
            Status = $"Added mission step {phase}.";
        }
        catch (InvalidOperationException error)
        {
            Status = error.Message;
        }
    }

    private void AddDialogue()
    {
        var phase = PreferredNewLinePhase();
        session.AddDialogueLine("Misja", phase, "dlg_new_line", "Nowa linia dialogu.", 3.0);
        DialogueLines.Add(new EditableDialogueLineRow(phase, "Misja", "dlg_new_line", "Nowa linia dialogu.", 3.0));
        RefreshIssues(session.Issues);
        Status = "Added dialogue line.";
    }

    private void AddNpcReaction()
    {
        var phase = PreferredNewLinePhase();
        session.AddNpcReaction(phase, "NPC", "npc_new_reaction", "Nowa reakcja NPC.", 2.5);
        NpcReactions.Add(new EditableNpcReactionRow(phase, "NPC", "npc_new_reaction", "Nowa reakcja NPC.", 2.5));
        RefreshIssues(session.Issues);
        Status = "Added NPC reaction.";
    }

    private void AddCutsceneLine()
    {
        var phase = PreferredNewLinePhase();
        session.AddCutsceneLine("Cutscene01", phase, "Narrator", "cutscene_new_line", "Nowa linia cutsceny.", 2.0);
        CutsceneLines.Add(new EditableCutsceneLineRow("Cutscene01", phase, "Narrator", "cutscene_new_line", "Nowa linia cutsceny.", 2.0));
        RefreshIssues(session.Issues);
        Status = "Added cutscene line.";
    }

    private string PreferredNewLinePhase()
    {
        if (SelectedMissionStep is not null)
        {
            return SelectedMissionStep.Phase;
        }

        return MissionSteps.Count > 0 ? MissionSteps[0].Phase : "ReachVehicle";
    }

    private void AddShop()
    {
        var id = $"shop_{Guid.NewGuid():N}"[..8];
        const string locationTag = "Blok13";
        const string kind = "spozywczy";
        const string owner = "wlasiciel";

        shopSession.AddShop(id, "Nowy sklep", locationTag, kind, owner, true);
        shopSession.Catalog.Shops[^1].Tags = new List<string>();
        Shops.Add(new EditableShopRow(id, "Nowy sklep", locationTag, kind, owner, true, string.Empty));
        RefreshIssues(session.Issues.Concat(shopSession.Issues).ToList());
        SelectedShop = Shops[^1];
        Status = $"Added shop {id}.";
    }

    private void AddShopItem()
    {
        if (Shops.Count == 0)
        {
            Status = "Add a shop first before adding items.";
            return;
        }

        var shop = SelectedShop ?? Shops[0];
        var id = $"item_{Guid.NewGuid():N}"[..8];
        shopSession.AddItem(id, shop.Id, "Nowy produkt", "ogolne", 0, "zl", "Krotki opis produktu.");
        ShopItems.Add(new EditableShopItemRow(id, shop.Id, "Nowy produkt", "ogolne", 0, "zl", "Krotki opis produktu."));
        RefreshIssues(session.Issues.Concat(shopSession.Issues).ToList());
        SelectedShopItem = ShopItems[^1];
        Status = $"Added item {id} to {shop.Id}.";
    }

    private void BindOutcome()
    {
        if (SelectedMissionStep is null || SelectedObjectOutcomeHook is null)
        {
            Status = "Select a mission step and object outcome before binding.";
            return;
        }

        var outcome = outcomeCatalog.Outcomes.FirstOrDefault(candidate => candidate.Key == SelectedObjectOutcomeHook.Id);
        if (outcome is null)
        {
            Status = $"Unknown outcome hook: {SelectedObjectOutcomeHook.Id}";
            return;
        }

        var stepIndex = MissionSteps.IndexOf(SelectedMissionStep);
        session.BindStepTriggerToOutcome(stepIndex, outcome);
        SelectedMissionStep.Trigger = session.Mission.Steps[stepIndex].Trigger;
        RefreshIssues(session.Issues);
        Status = $"Bound {SelectedMissionStep.Phase} to {SelectedMissionStep.Trigger}.";
    }

    private void Save()
    {
        for (var index = 0; index < MissionSteps.Count; index++)
        {
            session.Mission.Steps[index].Objective = MissionSteps[index].Objective;
            session.Mission.Steps[index].Trigger = MissionSteps[index].Trigger;
        }

        for (var index = 0; index < DialogueLines.Count; index++)
        {
            session.Mission.Dialogue[index].Speaker = DialogueLines[index].Speaker;
            session.Mission.Dialogue[index].Phase = DialogueLines[index].Phase;
            session.Mission.Dialogue[index].LineKey = DialogueLines[index].LineKey;
            session.Mission.Dialogue[index].Text = DialogueLines[index].Text;
            session.Mission.Dialogue[index].DurationSeconds = DialogueLines[index].DurationSeconds;
        }

        for (var index = 0; index < NpcReactions.Count; index++)
        {
            session.Mission.NpcReactions[index].Phase = NpcReactions[index].Phase;
            session.Mission.NpcReactions[index].Speaker = NpcReactions[index].Speaker;
            session.Mission.NpcReactions[index].LineKey = NpcReactions[index].LineKey;
            session.Mission.NpcReactions[index].Text = NpcReactions[index].Text;
            session.Mission.NpcReactions[index].DurationSeconds = NpcReactions[index].DurationSeconds;
        }

        for (var index = 0; index < CutsceneLines.Count; index++)
        {
            session.Mission.Cutscenes[index].Cutscene = CutsceneLines[index].Cutscene;
            session.Mission.Cutscenes[index].Phase = CutsceneLines[index].Phase;
            session.Mission.Cutscenes[index].Speaker = CutsceneLines[index].Speaker;
            session.Mission.Cutscenes[index].LineKey = CutsceneLines[index].LineKey;
            session.Mission.Cutscenes[index].Text = CutsceneLines[index].Text;
            session.Mission.Cutscenes[index].DurationSeconds = CutsceneLines[index].DurationSeconds;
        }

        shopSession.Catalog.Shops.Clear();
        foreach (var shop in Shops)
        {
            shopSession.Catalog.Shops.Add(new ShopDefinition
            {
                Id = shop.Id,
                Name = shop.Name,
                Kind = shop.Kind,
                LocationTag = shop.LocationTag,
                IsOpen = shop.IsOpen,
                Owner = shop.Owner,
                Tags = ParseTags(shop.Tags),
            });
        }

        shopSession.Catalog.Items.Clear();
        foreach (var item in ShopItems)
        {
            shopSession.Catalog.Items.Add(new ShopItemDefinition
            {
                Id = item.Id,
                ShopId = item.ShopId,
                Name = item.Name,
                Category = item.Category,
                Price = item.Price,
                Currency = item.Currency,
                Description = item.Description,
            });
        }

        var precheckIssues = new List<ValidationIssue>();
        precheckIssues.AddRange(session.Issues);
        precheckIssues.AddRange(shopSession.Issues);
        if (precheckIssues.Count > 0)
        {
            RefreshIssues(precheckIssues);
            Status = "Save blocked by validation issues.";
            return;
        }

        var missionResult = session.Save();
        var shopResult = shopSession.SaveValidated();

        var allIssues = new List<ValidationIssue>();
        allIssues.AddRange(missionResult.Issues);
        allIssues.AddRange(shopResult.Issues);
        RefreshIssues(allIssues);

        if (missionResult.Saved && shopResult.Saved)
        {
            Status = $"Saved mission + shop catalog JSON. Mission backup: {missionResult.BackupPath}, shop backup: {shopResult.BackupPath}";
            return;
        }

        if (missionResult.Saved && !shopResult.Saved)
        {
            Status = $"Mission saved, shop catalog blocked: {string.Join(", ", shopResult.Issues.Select(issue => issue.Code))}.";
            return;
        }

        if (!missionResult.Saved && shopResult.Saved)
        {
            Status = $"Mission blocked: {string.Join(", ", missionResult.Issues.Select(issue => issue.Code))}, shop catalog saved.";
            return;
        }

        Status = "Save blocked by validation issues.";
    }

    private static List<string> ParseTags(string tagsText)
    {
        return tagsText
            .Split([',', ';'], StringSplitOptions.RemoveEmptyEntries)
            .Select(tag => tag.Trim())
            .Where(tag => !string.IsNullOrWhiteSpace(tag))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    private void RefreshIssues(IReadOnlyList<ValidationIssue> issues)
    {
        IssueRows.Clear();
        if (issues.Count == 0)
        {
            IssueRows.Add(new EditorRow("ok", "Validation: OK"));
            return;
        }

        foreach (var issue in issues)
        {
            IssueRows.Add(new EditorRow(issue.Code, $"{issue.Code}: {issue.Message}"));
        }
    }

    private static string FindWorkspaceRoot(string startDirectory)
    {
        var directory = new DirectoryInfo(startDirectory);
        while (directory is not null)
        {
            if (File.Exists(Path.Combine(directory.FullName, "README.md")) &&
                Directory.Exists(Path.Combine(directory.FullName, "data")))
            {
                return directory.FullName;
            }

            directory = directory.Parent;
        }

        throw new DirectoryNotFoundException("Could not locate Blok 13 workspace root.");
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}

public sealed class EditableMissionStepRow : INotifyPropertyChanged
{
    private string objective;
    private string trigger;

    public EditableMissionStepRow(string phase, string objective, string trigger)
    {
        Phase = phase;
        this.objective = objective;
        this.trigger = trigger;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public string Phase { get; }

    public string Objective
    {
        get => objective;
        set
        {
            objective = value;
            OnPropertyChanged();
        }
    }

    public string Trigger
    {
        get => trigger;
        set
        {
            trigger = value;
            OnPropertyChanged();
        }
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}

public sealed class EditableDialogueLineRow : INotifyPropertyChanged
{
    private string phase;
    private string speaker;
    private string lineKey;
    private string text;
    private double durationSeconds;

    public EditableDialogueLineRow(string phase, string speaker, string lineKey, string text, double durationSeconds)
    {
        this.phase = phase;
        this.speaker = speaker;
        this.lineKey = lineKey;
        this.text = text;
        this.durationSeconds = durationSeconds;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public string Phase
    {
        get => phase;
        set
        {
            phase = value;
            OnPropertyChanged();
        }
    }

    public string Speaker
    {
        get => speaker;
        set
        {
            speaker = value;
            OnPropertyChanged();
        }
    }

    public string LineKey
    {
        get => lineKey;
        set
        {
            lineKey = value;
            OnPropertyChanged();
        }
    }

    public string Text
    {
        get => text;
        set
        {
            text = value;
            OnPropertyChanged();
        }
    }

    public double DurationSeconds
    {
        get => durationSeconds;
        set
        {
            durationSeconds = value;
            OnPropertyChanged();
        }
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}

public sealed class EditableNpcReactionRow : INotifyPropertyChanged
{
    private string phase;
    private string speaker;
    private string lineKey;
    private string text;
    private double durationSeconds;

    public EditableNpcReactionRow(string phase, string speaker, string lineKey, string text, double durationSeconds)
    {
        this.phase = phase;
        this.speaker = speaker;
        this.lineKey = lineKey;
        this.text = text;
        this.durationSeconds = durationSeconds;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public string Phase
    {
        get => phase;
        set
        {
            phase = value;
            OnPropertyChanged();
        }
    }

    public string Speaker
    {
        get => speaker;
        set
        {
            speaker = value;
            OnPropertyChanged();
        }
    }

    public string LineKey
    {
        get => lineKey;
        set
        {
            lineKey = value;
            OnPropertyChanged();
        }
    }

    public string Text
    {
        get => text;
        set
        {
            text = value;
            OnPropertyChanged();
        }
    }

    public double DurationSeconds
    {
        get => durationSeconds;
        set
        {
            durationSeconds = value;
            OnPropertyChanged();
        }
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}

public sealed class EditableCutsceneLineRow : INotifyPropertyChanged
{
    private string cutscene;
    private string phase;
    private string speaker;
    private string lineKey;
    private string text;
    private double durationSeconds;

    public EditableCutsceneLineRow(string cutscene, string phase, string speaker, string lineKey, string text, double durationSeconds)
    {
        this.cutscene = cutscene;
        this.phase = phase;
        this.speaker = speaker;
        this.lineKey = lineKey;
        this.text = text;
        this.durationSeconds = durationSeconds;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public string Cutscene
    {
        get => cutscene;
        set
        {
            cutscene = value;
            OnPropertyChanged();
        }
    }

    public string Phase
    {
        get => phase;
        set
        {
            phase = value;
            OnPropertyChanged();
        }
    }

    public string Speaker
    {
        get => speaker;
        set
        {
            speaker = value;
            OnPropertyChanged();
        }
    }

    public string LineKey
    {
        get => lineKey;
        set
        {
            lineKey = value;
            OnPropertyChanged();
        }
    }

    public string Text
    {
        get => text;
        set
        {
            text = value;
            OnPropertyChanged();
        }
    }

    public double DurationSeconds
    {
        get => durationSeconds;
        set
        {
            durationSeconds = value;
            OnPropertyChanged();
        }
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}

public sealed class EditableShopRow : INotifyPropertyChanged
{
    private string id;
    private string name;
    private string locationTag;
    private string kind;
    private string owner;
    private bool isOpen;
    private string tags;

    public EditableShopRow(string id, string name, string locationTag, string kind, string owner, bool isOpen, string tags)
    {
        this.id = id;
        this.name = name;
        this.locationTag = locationTag;
        this.kind = kind;
        this.owner = owner;
        this.isOpen = isOpen;
        this.tags = tags;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public string Id
    {
        get => id;
        set
        {
            id = value;
            OnPropertyChanged();
        }
    }

    public string Name
    {
        get => name;
        set
        {
            name = value;
            OnPropertyChanged();
        }
    }

    public string LocationTag
    {
        get => locationTag;
        set
        {
            locationTag = value;
            OnPropertyChanged();
        }
    }

    public string Kind
    {
        get => kind;
        set
        {
            kind = value;
            OnPropertyChanged();
        }
    }

    public string Owner
    {
        get => owner;
        set
        {
            owner = value;
            OnPropertyChanged();
        }
    }

    public bool IsOpen
    {
        get => isOpen;
        set
        {
            isOpen = value;
            OnPropertyChanged();
        }
    }

    public string Tags
    {
        get => tags;
        set
        {
            tags = value;
            OnPropertyChanged();
        }
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}

public sealed class EditableShopItemRow : INotifyPropertyChanged
{
    private string id;
    private string shopId;
    private string name;
    private string category;
    private int price;
    private string currency;
    private string description;

    public EditableShopItemRow(string id, string shopId, string name, string category, int price, string currency, string description)
    {
        this.id = id;
        this.shopId = shopId;
        this.name = name;
        this.category = category;
        this.price = price;
        this.currency = currency;
        this.description = description;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public string Id
    {
        get => id;
        set
        {
            id = value;
            OnPropertyChanged();
        }
    }

    public string ShopId
    {
        get => shopId;
        set
        {
            shopId = value;
            OnPropertyChanged();
        }
    }

    public string Name
    {
        get => name;
        set
        {
            name = value;
            OnPropertyChanged();
        }
    }

    public string Category
    {
        get => category;
        set
        {
            category = value;
            OnPropertyChanged();
        }
    }

    public int Price
    {
        get => price;
        set
        {
            price = value;
            OnPropertyChanged();
        }
    }

    public string Currency
    {
        get => currency;
        set
        {
            currency = value;
            OnPropertyChanged();
        }
    }

    public string Description
    {
        get => description;
        set
        {
            description = value;
            OnPropertyChanged();
        }
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}

public sealed class RelayCommand : ICommand
{
    private readonly Action execute;

    public RelayCommand(Action execute)
    {
        this.execute = execute;
    }

    public event EventHandler? CanExecuteChanged
    {
        add => CommandManager.RequerySuggested += value;
        remove => CommandManager.RequerySuggested -= value;
    }

    public bool CanExecute(object? parameter) => true;

    public void Execute(object? parameter)
    {
        execute();
    }
}
