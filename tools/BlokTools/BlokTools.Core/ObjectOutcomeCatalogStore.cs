using System.IO;
using System.Text.Json;

namespace BlokTools.Core;

public static class ObjectOutcomeCatalogStore
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        AllowTrailingCommas = true,
        PropertyNameCaseInsensitive = true,
        ReadCommentHandling = JsonCommentHandling.Skip,
        WriteIndented = true,
    };

    public static ObjectOutcomeCatalog Load(string path)
    {
        var json = File.ReadAllText(path);
        return JsonSerializer.Deserialize<ObjectOutcomeCatalog>(json, JsonOptions) ??
            throw new InvalidDataException($"Could not parse object outcome catalog '{path}'.");
    }

    public static void Save(string path, ObjectOutcomeCatalog catalog)
    {
        var json = JsonSerializer.Serialize(catalog, JsonOptions);
        File.WriteAllText(path, json);
    }
}
