using System.IO;
using System.Text.Json;

namespace BlokTools.Core;

public static class MissionDocumentStore
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        AllowTrailingCommas = true,
        PropertyNameCaseInsensitive = true,
        ReadCommentHandling = JsonCommentHandling.Skip,
        WriteIndented = true,
    };

    public static MissionDocument Load(string path)
    {
        var json = File.ReadAllText(path);
        var mission = JsonSerializer.Deserialize<MissionDocument>(json, JsonOptions) ??
            throw new InvalidDataException($"Could not parse mission document '{path}'.");

        foreach (var line in mission.NpcReactions)
        {
            if (string.IsNullOrWhiteSpace(line.Text) && !string.IsNullOrWhiteSpace(line.Line))
            {
                line.Text = line.Line;
            }
        }

        foreach (var line in mission.Cutscenes)
        {
            if (string.IsNullOrWhiteSpace(line.Text) && !string.IsNullOrWhiteSpace(line.Line))
            {
                line.Text = line.Line;
            }
        }

        return mission;
    }

    public static void Save(string path, MissionDocument mission)
    {
        var json = JsonSerializer.Serialize(mission, JsonOptions);
        File.WriteAllText(path, json);
    }

    public static MissionSaveResult SaveValidated(
        string path,
        MissionDocument mission,
        ObjectOutcomeCatalog? outcomeCatalog = null,
        MissionLocalization? localization = null)
    {
        var issues = MissionDocumentValidator.Validate(mission, outcomeCatalog, localization).ToList();
        if (issues.Count > 0)
        {
            return MissionSaveResult.Failed(issues);
        }

        var fullPath = Path.GetFullPath(path);
        var directory = Path.GetDirectoryName(fullPath);
        if (!string.IsNullOrEmpty(directory))
        {
            Directory.CreateDirectory(directory);
        }

        var backupPath = fullPath + ".bak";
        var tempPath = fullPath + ".tmp";
        var json = JsonSerializer.Serialize(mission, JsonOptions);
        File.WriteAllText(tempPath, json);

        if (File.Exists(fullPath))
        {
            File.Copy(fullPath, backupPath, overwrite: true);
        }
        else
        {
            File.WriteAllText(backupPath, "");
        }

        File.Move(tempPath, fullPath, overwrite: true);
        return MissionSaveResult.Success(backupPath);
    }
}
