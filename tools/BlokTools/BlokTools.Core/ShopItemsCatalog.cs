using System.Text.Json.Serialization;

namespace BlokTools.Core;

public sealed class ShopItemsCatalog
{
    [JsonPropertyName("schemaVersion")]
    public int SchemaVersion { get; set; }

    [JsonPropertyName("shops")]
    public List<ShopDefinition> Shops { get; set; } = new();

    [JsonPropertyName("items")]
    public List<ShopItemDefinition> Items { get; set; } = new();
}

public sealed class ShopDefinition
{
    [JsonPropertyName("id")]
    public string Id { get; set; } = "";

    [JsonPropertyName("name")]
    public string Name { get; set; } = "";

    [JsonPropertyName("locationTag")]
    public string LocationTag { get; set; } = "";

    [JsonPropertyName("kind")]
    public string Kind { get; set; } = "";

    [JsonPropertyName("owner")]
    public string Owner { get; set; } = "";

    [JsonPropertyName("isOpen")]
    public bool IsOpen { get; set; } = true;

    [JsonPropertyName("tags")]
    public List<string> Tags { get; set; } = new();
}

public sealed class ShopItemDefinition
{
    [JsonPropertyName("id")]
    public string Id { get; set; } = "";

    [JsonPropertyName("shopId")]
    public string ShopId { get; set; } = "";

    [JsonPropertyName("name")]
    public string Name { get; set; } = "";

    [JsonPropertyName("category")]
    public string Category { get; set; } = "";

    [JsonPropertyName("price")]
    public int Price { get; set; }

    [JsonPropertyName("currency")]
    public string Currency { get; set; } = "zl";

    [JsonPropertyName("description")]
    public string Description { get; set; } = "";
}
