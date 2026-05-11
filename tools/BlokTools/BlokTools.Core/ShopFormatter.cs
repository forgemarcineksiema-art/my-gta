namespace BlokTools.Core;

public static class ShopFormatter
{
    public static string ShopEditorText(ShopDefinition shop)
    {
        var openLabel = shop.IsOpen ? "open" : "closed";
        var owner = string.IsNullOrWhiteSpace(shop.Owner) ? "unknown" : shop.Owner.Trim();
        var tagText = shop.Tags.Count == 0
            ? "no-tags"
            : string.Join(", ", shop.Tags.Select(tag => tag.Trim()));

        return $"{shop.Id}: {shop.Name} [{shop.Kind}] in {shop.LocationTag} ({openLabel}) owner={owner} tags=[{tagText}]";
    }

    public static string ItemEditorText(ShopItemDefinition item)
    {
        var description = string.IsNullOrWhiteSpace(item.Description) ? "no-description" : item.Description;
        return $"{item.Id}: {item.Name} ({item.Category}) for {item.Price} {item.Currency} in shop={item.ShopId} - {description}";
    }
}
