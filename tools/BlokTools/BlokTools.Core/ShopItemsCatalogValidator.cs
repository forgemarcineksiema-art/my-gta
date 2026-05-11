namespace BlokTools.Core;

public static class ShopItemsCatalogValidator
{
    public static IEnumerable<ValidationIssue> Validate(ShopItemsCatalog catalog)
    {
        if (catalog.SchemaVersion != 1)
        {
            yield return new ValidationIssue("shopItems.schema", "Shop/item catalog schemaVersion must be 1.");
        }

        if (catalog.Shops.Count == 0)
        {
            yield return new ValidationIssue("shopItems.shops.empty", "Shop/item catalog needs at least one shop.");
        }
        if (catalog.Items.Count == 0)
        {
            yield return new ValidationIssue("shopItems.items.empty", "Shop/item catalog needs at least one item.");
        }

        var shopIds = new HashSet<string>(StringComparer.Ordinal);
        foreach (var shop in catalog.Shops)
        {
            if (string.IsNullOrWhiteSpace(shop.Id))
            {
                yield return new ValidationIssue("shopItems.shop.id", "Shop entry needs an id.");
                continue;
            }
            if (!shopIds.Add(shop.Id))
            {
                yield return new ValidationIssue("shopItems.shop.id.duplicate", $"Duplicate shop id '{shop.Id}'.");
            }
            if (string.IsNullOrWhiteSpace(shop.Name))
            {
                yield return new ValidationIssue("shopItems.shop.name", $"Shop '{shop.Id}' needs a name.");
            }
            if (string.IsNullOrWhiteSpace(shop.LocationTag))
            {
                yield return new ValidationIssue("shopItems.shop.location", $"Shop '{shop.Id}' needs a locationTag.");
            }
            if (string.IsNullOrWhiteSpace(shop.Kind))
            {
                yield return new ValidationIssue("shopItems.shop.kind", $"Shop '{shop.Id}' needs a kind.");
            }
        }

        var itemIds = new HashSet<string>(StringComparer.Ordinal);
        foreach (var item in catalog.Items)
        {
            if (string.IsNullOrWhiteSpace(item.Id))
            {
                yield return new ValidationIssue("shopItems.item.id", "Item entry needs an id.");
                continue;
            }
            if (!itemIds.Add(item.Id))
            {
                yield return new ValidationIssue("shopItems.item.id.duplicate", $"Duplicate item id '{item.Id}'.");
            }
            if (string.IsNullOrWhiteSpace(item.ShopId))
            {
                yield return new ValidationIssue("shopItems.item.shopId", $"Item '{item.Id}' needs a shopId.");
            }
            else if (!shopIds.Contains(item.ShopId))
            {
                yield return new ValidationIssue("shopItems.item.shopId", $"Item '{item.Id}' references unknown shop '{item.ShopId}'.");
            }

            if (string.IsNullOrWhiteSpace(item.Name))
            {
                yield return new ValidationIssue("shopItems.item.name", $"Item '{item.Id}' needs a name.");
            }
            if (string.IsNullOrWhiteSpace(item.Category))
            {
                yield return new ValidationIssue("shopItems.item.category", $"Item '{item.Id}' needs a category.");
            }
            if (string.IsNullOrWhiteSpace(item.Currency))
            {
                yield return new ValidationIssue("shopItems.item.currency", $"Item '{item.Id}' needs currency.");
            }
            if (item.Price < 0)
            {
                yield return new ValidationIssue("shopItems.item.price", $"Item '{item.Id}' price cannot be negative.");
            }
        }
    }
}
