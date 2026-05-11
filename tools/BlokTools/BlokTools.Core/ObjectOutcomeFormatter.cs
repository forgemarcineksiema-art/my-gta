namespace BlokTools.Core;

public static class ObjectOutcomeFormatter
{
    public static string EditorText(ObjectOutcomeDefinition outcome)
    {
        var consequence = outcome.WorldEvent is null
            ? "quiet"
            : $"{outcome.WorldEvent.Type} {outcome.WorldEvent.Intensity:0.##} cd {outcome.WorldEvent.CooldownSeconds:0.##}s";
        return $"{outcome.Key}: {outcome.Label} [{outcome.Location}] ({consequence})";
    }
}
