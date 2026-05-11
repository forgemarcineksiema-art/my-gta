using System.Text.Json.Serialization;

namespace BlokTools.Core;

public sealed class MissionDocument
{
    [JsonPropertyName("id")]
    public string Id { get; set; } = "";

    [JsonPropertyName("title")]
    public string Title { get; set; } = "";

    [JsonPropertyName("steps")]
    public List<MissionStep> Steps { get; set; } = new();

    [JsonPropertyName("dialogue")]
    public List<MissionDialogueLine> Dialogue { get; set; } = new();

    [JsonPropertyName("npcReactions")]
    public List<MissionNpcReaction> NpcReactions { get; set; } = new();

    [JsonPropertyName("cutscenes")]
    public List<MissionCutsceneLine> Cutscenes { get; set; } = new();
}

public sealed class MissionStep
{
    [JsonPropertyName("phase")]
    public string Phase { get; set; } = "";

    [JsonPropertyName("objective")]
    public string Objective { get; set; } = "";

    [JsonPropertyName("trigger")]
    public string Trigger { get; set; } = "";
}

public sealed class MissionDialogueLine
{
    [JsonPropertyName("phase")]
    public string Phase { get; set; } = "";

    [JsonPropertyName("speaker")]
    public string Speaker { get; set; } = "";

    [JsonPropertyName("lineKey")]
    public string LineKey { get; set; } = "";

    [JsonPropertyName("text")]
    public string Text { get; set; } = "";

    [JsonPropertyName("line")]
    public string Line { get; set; } = "";

    [JsonPropertyName("durationSeconds")]
    public double DurationSeconds { get; set; } = 3.0;
}

public sealed class MissionNpcReaction
{
    [JsonPropertyName("phase")]
    public string Phase { get; set; } = "";

    [JsonPropertyName("speaker")]
    public string Speaker { get; set; } = "";

    [JsonPropertyName("lineKey")]
    public string LineKey { get; set; } = "";

    [JsonPropertyName("text")]
    public string Text { get; set; } = "";

    [JsonPropertyName("line")]
    public string Line { get; set; } = "";

    [JsonPropertyName("durationSeconds")]
    public double DurationSeconds { get; set; } = 3.0;
}

public sealed class MissionCutsceneLine
{
    [JsonPropertyName("cutscene")]
    public string Cutscene { get; set; } = "";

    [JsonPropertyName("phase")]
    public string Phase { get; set; } = "";

    [JsonPropertyName("speaker")]
    public string Speaker { get; set; } = "";

    [JsonPropertyName("lineKey")]
    public string LineKey { get; set; } = "";

    [JsonPropertyName("text")]
    public string Text { get; set; } = "";

    [JsonPropertyName("line")]
    public string Line { get; set; } = "";

    [JsonPropertyName("durationSeconds")]
    public double DurationSeconds { get; set; } = 3.0;
}
