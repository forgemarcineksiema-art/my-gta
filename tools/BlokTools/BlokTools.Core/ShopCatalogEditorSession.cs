using System.IO;

namespace BlokTools.Core;

public sealed class ShopCatalogEditorSession
{
    private ShopCatalogEditorSession(string filePath, ShopItemsCatalog catalog)
    {
        FilePath = filePath;
        Catalog = catalog;
    }

    public string FilePath { get; }
    public ShopItemsCatalog Catalog { get; }

    public IReadOnlyList<ValidationIssue> Issues => ShopItemsCatalogValidator.Validate(Catalog).ToList();

    public static ShopCatalogEditorSession Load(string path)
    {
        return new ShopCatalogEditorSession(path, ShopItemsCatalogStore.Load(path));
    }

    public void AddShop(string id, string name, string locationTag, string kind, string owner, bool isOpen)
    {
        Catalog.Shops.Add(new ShopDefinition
        {
            Id = id,
            Name = name,
            LocationTag = locationTag,
            Kind = kind,
            Owner = owner,
            IsOpen = isOpen,
        });
    }

    public void AddItem(
        string id,
        string shopId,
        string name,
        string category,
        int price,
        string currency,
        string description)
    {
        Catalog.Items.Add(new ShopItemDefinition
        {
            Id = id,
            ShopId = shopId,
            Name = name,
            Category = category,
            Price = price,
            Currency = currency,
            Description = description,
        });
    }

    public void SetShopTagSet(int index, IEnumerable<string> tags)
    {
        Catalog.Shops[index].Tags = tags.Where(tag => !string.IsNullOrWhiteSpace(tag)).Select(tag => tag.Trim()).ToList();
    }

    public ShopCatalogSaveResult SaveValidated()
    {
        var issues = Issues;
        if (issues.Count > 0)
        {
            return ShopCatalogSaveResult.Failed(issues);
        }

        var fullPath = Path.GetFullPath(FilePath);
        var directory = Path.GetDirectoryName(fullPath);
        if (!string.IsNullOrEmpty(directory))
        {
            Directory.CreateDirectory(directory);
        }

        var backupPath = fullPath + ".bak";
        var tempPath = fullPath + ".tmp";
        ShopItemsCatalogStore.Save(tempPath, Catalog);

        if (File.Exists(fullPath))
        {
            File.Copy(fullPath, backupPath, overwrite: true);
        }
        else
        {
            File.WriteAllText(backupPath, string.Empty);
        }

        File.Move(tempPath, fullPath, overwrite: true);
        return ShopCatalogSaveResult.Success(backupPath);
    }
}

public sealed class ShopCatalogSaveResult
{
    private ShopCatalogSaveResult(bool saved, string backupPath, IReadOnlyList<ValidationIssue> issues)
    {
        Saved = saved;
        BackupPath = backupPath;
        Issues = issues;
    }

    public bool Saved { get; }
    public string BackupPath { get; }
    public IReadOnlyList<ValidationIssue> Issues { get; }

    public static ShopCatalogSaveResult Success(string backupPath)
    {
        return new ShopCatalogSaveResult(true, backupPath, Array.Empty<ValidationIssue>());
    }

    public static ShopCatalogSaveResult Failed(IReadOnlyList<ValidationIssue> issues)
    {
        return new ShopCatalogSaveResult(false, string.Empty, issues);
    }
}
