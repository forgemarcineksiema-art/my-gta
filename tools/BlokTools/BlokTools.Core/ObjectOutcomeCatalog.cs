using System.Text.Json.Serialization;

namespace BlokTools.Core;

public sealed class ObjectOutcomeCatalog
{
    [JsonPropertyName("schemaVersion")]
    public int SchemaVersion { get; set; }

    [JsonPropertyName("outcomes")]
    public List<ObjectOutcomeDefinition> Outcomes { get; set; } = new();
}

public sealed class ObjectOutcomeDefinition
{
    [JsonPropertyName("id")]
    public string Id { get; set; } = "";

    [JsonPropertyName("idPattern")]
    public string IdPattern { get; set; } = "";

    [JsonPropertyName("label")]
    public string Label { get; set; } = "";

    [JsonPropertyName("source")]
    public string Source { get; set; } = "";

    [JsonPropertyName("category")]
    public string Category { get; set; } = "";

    [JsonPropertyName("location")]
    public string Location { get; set; } = "";

    [JsonPropertyName("speaker")]
    public string Speaker { get; set; } = "";

    [JsonPropertyName("line")]
    public string Line { get; set; } = "";

    [JsonPropertyName("missionTag")]
    public string MissionTag { get; set; } = "";

    [JsonPropertyName("worldEvent")]
    public ObjectOutcomeWorldEvent? WorldEvent { get; set; }

    [JsonIgnore]
    public string Key => string.IsNullOrWhiteSpace(Id) ? IdPattern : Id;
}

public sealed class ObjectOutcomeWorldEvent
{
    [JsonPropertyName("type")]
    public string Type { get; set; } = "";

    [JsonPropertyName("intensity")]
    public double Intensity { get; set; }

    [JsonPropertyName("cooldownSeconds")]
    public double CooldownSeconds { get; set; }
}
