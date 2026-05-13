using System.Collections.ObjectModel;
using System.IO;

namespace BlokTools.Core;

public sealed class BlokWorkspaceSnapshot
{
    public string MissionId { get; init; } = "";
    public string MissionTitle { get; init; } = "";
    public IReadOnlyList<EditorRow> MissionNodes { get; init; } = ReadOnlyCollection<EditorRow>.Empty;
    public IReadOnlyList<EditorRow> DialogueLines { get; init; } = ReadOnlyCollection<EditorRow>.Empty;
    public IReadOnlyList<EditorRow> NpcReactions { get; init; } = ReadOnlyCollection<EditorRow>.Empty;
    public IReadOnlyList<EditorRow> CutsceneLines { get; init; } = ReadOnlyCollection<EditorRow>.Empty;
    public IReadOnlyList<EditorRow> Shops { get; init; } = ReadOnlyCollection<EditorRow>.Empty;
    public IReadOnlyList<EditorRow> ShopItems { get; init; } = ReadOnlyCollection<EditorRow>.Empty;
    public IReadOnlyList<EditorRow> ObjectOutcomeHooks { get; init; } = ReadOnlyCollection<EditorRow>.Empty;
    public IReadOnlyList<ValidationIssue> Issues { get; init; } = ReadOnlyCollection<ValidationIssue>.Empty;
}

public sealed record EditorRow(string Id, string Text);

public static class BlokWorkspaceLoader
{
    public static BlokWorkspaceSnapshot Load(string workspaceRoot)
    {
        var root = Path.GetFullPath(workspaceRoot);
        var mission = MissionDocumentStore.Load(Path.Combine(root, "data", "mission_driving_errand.json"));
        var outcomes = ObjectOutcomeCatalogStore.Load(Path.Combine(root, "data", "world", "object_outcome_catalog.json"));
        var shops = ShopItemsCatalogStore.Load(Path.Combine(root, "data", "world", "shop_items_catalog.json"));
        var localization = MissionLocalizationStore.Load(Path.Combine(root, "data", "world", "mission_localization_pl.json"));
        var issues = new List<ValidationIssue>();
        issues.AddRange(MissionDocumentValidator.Validate(mission, outcomes, localization));
        issues.AddRange(ObjectOutcomeCatalogValidator.Validate(outcomes));
        issues.AddRange(ShopItemsCatalogValidator.Validate(shops));

        return new BlokWorkspaceSnapshot
        {
            MissionId = mission.Id,
            MissionTitle = mission.Title,
            MissionNodes = mission.Steps
                .Select(step => new EditorRow(step.Phase, $"{step.Phase}: {step.Objective} -> {step.Trigger}"))
                .ToList(),
            DialogueLines = mission.Dialogue
                .Select((line, index) => new EditorRow($"{index:000}", $"{line.Speaker}: {ResolveText(line.Text, line.LineKey, localization)}"))
                .ToList(),
            NpcReactions = mission.NpcReactions
                .Select((line, index) => new EditorRow($"{index:000}", $"{line.Phase} / {line.Speaker}: {ResolveText(line.Text, line.LineKey, localization)}"))
                .ToList(),
            CutsceneLines = mission.Cutscenes
                .Select((line, index) => new EditorRow($"{index:000}", $"{line.Cutscene}:{line.Phase} / {line.Speaker}: {ResolveText(line.Text, line.LineKey, localization)}"))
                .ToList(),
            Shops = shops.Shops.Select(shop => new EditorRow(shop.Id, ShopFormatter.ShopEditorText(shop))).ToList(),
            ShopItems = shops.Items
                .Select(item => new EditorRow(item.Id, ShopFormatter.ItemEditorText(item)))
                .ToList(),
            ObjectOutcomeHooks = outcomes.Outcomes
                .Select(outcome => new EditorRow(outcome.Key, ObjectOutcomeFormatter.EditorText(outcome)))
                .ToList(),
            Issues = issues,
        };
    }

    private static string ResolveText(string text, string lineKey, MissionLocalization localization)
    {
        if (!string.IsNullOrWhiteSpace(text))
        {
            return text;
        }

        return string.IsNullOrWhiteSpace(lineKey) ? "" : localization.Resolve(lineKey);
    }
}
