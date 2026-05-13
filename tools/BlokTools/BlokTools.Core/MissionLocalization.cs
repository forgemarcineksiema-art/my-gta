using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace BlokTools.Core;

public sealed class MissionLocalization
{
    [JsonPropertyName("schemaVersion")]
    public int SchemaVersion { get; set; }

    [JsonPropertyName("lines")]
    public Dictionary<string, string> Lines { get; set; } = new(StringComparer.Ordinal);

    public bool HasLine(string lineKey)
    {
        return !string.IsNullOrWhiteSpace(lineKey) &&
            Lines.TryGetValue(lineKey, out var text) &&
            !string.IsNullOrWhiteSpace(text);
    }

    public string Resolve(string lineKey)
    {
        return Lines.TryGetValue(lineKey, out var text) ? text : "";
    }
}

public static class MissionLocalizationStore
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        AllowTrailingCommas = true,
        PropertyNameCaseInsensitive = true,
        ReadCommentHandling = JsonCommentHandling.Skip,
        WriteIndented = true,
    };

    public static MissionLocalization Load(string path)
    {
        var json = File.ReadAllText(path);
        return JsonSerializer.Deserialize<MissionLocalization>(json, JsonOptions) ??
            throw new InvalidDataException($"Could not parse mission localization '{path}'.");
    }

    public static void Save(string path, MissionLocalization localization)
    {
        var json = JsonSerializer.Serialize(localization, JsonOptions);
        File.WriteAllText(path, json);
    }
}
