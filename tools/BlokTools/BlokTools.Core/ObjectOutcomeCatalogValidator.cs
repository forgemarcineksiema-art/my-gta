namespace BlokTools.Core;

public static class ObjectOutcomeCatalogValidator
{
    private static readonly HashSet<string> AllowedWorldEventTypes = new(StringComparer.Ordinal)
    {
        "PublicNuisance",
        "PropertyDamage",
        "ChaseSeen",
        "ShopTrouble",
    };

    public static IEnumerable<ValidationIssue> Validate(ObjectOutcomeCatalog catalog)
    {
        if (catalog.SchemaVersion != 1)
        {
            yield return new ValidationIssue("outcomes.schema", "Object outcome catalog schemaVersion must be 1.");
        }
        if (catalog.Outcomes.Count == 0)
        {
            yield return new ValidationIssue("outcomes.empty", "Object outcome catalog needs at least one outcome.");
        }

        var keys = new HashSet<string>(StringComparer.Ordinal);
        foreach (var outcome in catalog.Outcomes)
        {
            var hasId = !string.IsNullOrWhiteSpace(outcome.Id);
            var hasPattern = !string.IsNullOrWhiteSpace(outcome.IdPattern);
            if (hasId == hasPattern)
            {
                yield return new ValidationIssue("outcome.key", "Outcome must define exactly one of id or idPattern.");
                continue;
            }

            if (!keys.Add(outcome.Key))
            {
                yield return new ValidationIssue("outcome.key.duplicate", $"Duplicate object outcome key '{outcome.Key}'.");
            }
            if (string.IsNullOrWhiteSpace(outcome.Label))
            {
                yield return new ValidationIssue("outcome.label", $"Outcome '{outcome.Key}' needs an editor label.");
            }
            if (string.IsNullOrWhiteSpace(outcome.Source))
            {
                yield return new ValidationIssue("outcome.source", $"Outcome '{outcome.Key}' needs a source.");
            }
            if (string.IsNullOrWhiteSpace(outcome.Category))
            {
                yield return new ValidationIssue("outcome.category", $"Outcome '{outcome.Key}' needs a category.");
            }
            if (string.IsNullOrWhiteSpace(outcome.Location))
            {
                yield return new ValidationIssue("outcome.location", $"Outcome '{outcome.Key}' needs a location.");
            }
            if (string.IsNullOrWhiteSpace(outcome.Speaker))
            {
                yield return new ValidationIssue("outcome.speaker", $"Outcome '{outcome.Key}' needs a player-facing speaker.");
            }
            if (string.IsNullOrWhiteSpace(outcome.Line))
            {
                yield return new ValidationIssue("outcome.line", $"Outcome '{outcome.Key}' needs a player-facing line.");
            }

            if (outcome.WorldEvent is not null)
            {
                if (!AllowedWorldEventTypes.Contains(outcome.WorldEvent.Type))
                {
                    yield return new ValidationIssue(
                        "outcome.worldEvent.type",
                        $"Outcome '{outcome.Key}' uses unsupported world event type '{outcome.WorldEvent.Type}'.");
                }
                if (outcome.WorldEvent.Intensity <= 0.0 || outcome.WorldEvent.Intensity > 1.0)
                {
                    yield return new ValidationIssue(
                        "outcome.worldEvent.intensity",
                        $"Outcome '{outcome.Key}' worldEvent intensity must be in the 0.0-1.0 range.");
                }
                if (outcome.WorldEvent.CooldownSeconds < 1.0)
                {
                    yield return new ValidationIssue(
                        "outcome.worldEvent.cooldown",
                        $"Outcome '{outcome.Key}' worldEvent cooldown must be at least one second.");
                }
            }
        }
    }
}
