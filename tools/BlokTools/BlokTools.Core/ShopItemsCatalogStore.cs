using System.IO;
using System.Text.Json;

namespace BlokTools.Core;

public static class ShopItemsCatalogStore
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        AllowTrailingCommas = true,
        PropertyNameCaseInsensitive = true,
        ReadCommentHandling = JsonCommentHandling.Skip,
        WriteIndented = true,
    };

    public static ShopItemsCatalog Load(string path)
    {
        var json = File.ReadAllText(path);
        return JsonSerializer.Deserialize<ShopItemsCatalog>(json, JsonOptions) ??
            throw new InvalidDataException($"Could not parse shop/item catalog '{path}'.");
    }

    public static void Save(string path, ShopItemsCatalog catalog)
    {
        var directory = Path.GetDirectoryName(Path.GetFullPath(path));
        if (!string.IsNullOrEmpty(directory))
        {
            Directory.CreateDirectory(directory);
        }

        var json = JsonSerializer.Serialize(catalog, JsonOptions);
        File.WriteAllText(path, json);
    }

    public static string Backup(string path)
    {
        return path + ".bak";
    }
}
