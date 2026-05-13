namespace BlokTools.Core;

public static class MissionDocumentValidator
{
    public static IEnumerable<ValidationIssue> Validate(MissionDocument mission, ObjectOutcomeCatalog? outcomeCatalog = null)
    {
        var outcomeKeys = outcomeCatalog?.Outcomes
            .Select(outcome => outcome.Key)
            .Where(key => !string.IsNullOrWhiteSpace(key))
            .ToHashSet(StringComparer.Ordinal);
        var outcomePatternPrefixes = outcomeCatalog?.Outcomes
            .Select(outcome => outcome.IdPattern)
            .Where(pattern => !string.IsNullOrWhiteSpace(pattern) && pattern.EndsWith("*", StringComparison.Ordinal))
            .Select(pattern => pattern[..^1])
            .ToList();

        var missionPhases = mission.Steps.Select(step => step.Phase).ToHashSet(StringComparer.Ordinal);

        if (string.IsNullOrWhiteSpace(mission.Id))
        {
            yield return new ValidationIssue("mission.id", "Mission id is required.");
        }
        if (string.IsNullOrWhiteSpace(mission.Title))
        {
            yield return new ValidationIssue("mission.title", "Mission title is required.");
        }
        if (mission.Steps.Count == 0)
        {
            yield return new ValidationIssue("mission.steps", "Mission needs at least one step.");
        }

        var phases = new HashSet<string>(StringComparer.Ordinal);
        foreach (var step in mission.Steps)
        {
            if (string.IsNullOrWhiteSpace(step.Phase))
            {
                yield return new ValidationIssue("mission.step.phase", "Mission step phase is required.");
            }
            else if (!MissionPhaseCatalog.IsSupported(step.Phase))
            {
                yield return new ValidationIssue(
                    "mission.step.phase.unknown",
                    $"Mission step phase '{step.Phase}' is not supported by the runtime phase catalog.");
            }
            else if (!phases.Add(step.Phase))
            {
                yield return new ValidationIssue("mission.step.phase.duplicate", $"Duplicate mission phase '{step.Phase}'.");
            }

            if (string.IsNullOrWhiteSpace(step.Objective))
            {
                yield return new ValidationIssue("mission.step.objective", $"Mission step '{step.Phase}' needs an objective.");
            }
            if (string.IsNullOrWhiteSpace(step.Trigger))
            {
                yield return new ValidationIssue("mission.step.trigger", $"Mission step '{step.Phase}' needs a trigger.");
            }
            else if (outcomeKeys is not null && step.Trigger.StartsWith("outcome:", StringComparison.Ordinal))
            {
                var outcomeKey = step.Trigger["outcome:".Length..];
                var exactMatch = outcomeKeys.Contains(outcomeKey);
                var patternMatch = outcomePatternPrefixes is not null &&
                    outcomePatternPrefixes.Any(prefix => outcomeKey.StartsWith(prefix, StringComparison.Ordinal));
                if (!exactMatch && !patternMatch)
                {
                    yield return new ValidationIssue(
                        "mission.step.trigger.outcome",
                        $"Mission step '{step.Phase}' references unknown object outcome '{outcomeKey}'.");
                }
            }
        }

        foreach (var line in mission.Dialogue)
        {
            if (string.IsNullOrWhiteSpace(line.Phase))
            {
                yield return new ValidationIssue("mission.dialogue.phase", "Dialogue phase is required.");
            }
            else if (!missionPhases.Contains(line.Phase))
            {
                yield return new ValidationIssue(
                    "mission.dialogue.phase.unknown",
                    $"Dialogue phase '{line.Phase}' is not present in mission steps.");
            }
            if (string.IsNullOrWhiteSpace(line.Speaker))
            {
                yield return new ValidationIssue("mission.dialogue.speaker", "Dialogue line needs a speaker.");
            }
            var hasText = !string.IsNullOrWhiteSpace(line.Text);
            var hasLineKey = !string.IsNullOrWhiteSpace(line.LineKey);
            if (!hasText && !hasLineKey)
            {
                yield return new ValidationIssue("mission.dialogue.text", "Dialogue line needs text or a localization lineKey.");
            }
            if (line.DurationSeconds <= 0.0)
            {
                yield return new ValidationIssue("mission.dialogue.duration", "Dialogue duration must be positive.");
            }
        }

        foreach (var line in mission.NpcReactions)
        {
            if (string.IsNullOrWhiteSpace(line.Phase))
            {
                yield return new ValidationIssue("mission.npcReactions.phase", "NPC reaction needs a phase.");
            }
            else if (!missionPhases.Contains(line.Phase))
            {
                yield return new ValidationIssue(
                    "mission.npcReactions.phase.unknown",
                    $"NPC reaction phase '{line.Phase}' is not present in mission steps.");
            }

            if (string.IsNullOrWhiteSpace(line.Speaker))
            {
                yield return new ValidationIssue("mission.npcReactions.speaker", "NPC reaction line needs a speaker.");
            }
            var hasText = !string.IsNullOrWhiteSpace(line.Text);
            var hasLineKey = !string.IsNullOrWhiteSpace(line.LineKey);
            if (!hasText && !hasLineKey)
            {
                yield return new ValidationIssue("mission.npcReactions.text", "NPC reaction line needs text or a localization lineKey.");
            }
            if (line.DurationSeconds <= 0.0)
            {
                yield return new ValidationIssue("mission.npcReactions.duration", "NPC reaction duration must be positive.");
            }
        }

        foreach (var line in mission.Cutscenes)
        {
            if (string.IsNullOrWhiteSpace(line.Cutscene))
            {
                yield return new ValidationIssue("mission.cutscenes.cutscene", "Cutscene line needs a cutscene id.");
            }
            if (string.IsNullOrWhiteSpace(line.Phase))
            {
                yield return new ValidationIssue("mission.cutscenes.phase", "Cutscene line needs a phase.");
            }
            else if (!missionPhases.Contains(line.Phase))
            {
                yield return new ValidationIssue(
                    "mission.cutscenes.phase.unknown",
                    $"Cutscene phase '{line.Phase}' is not present in mission steps.");
            }

            if (string.IsNullOrWhiteSpace(line.Speaker))
            {
                yield return new ValidationIssue("mission.cutscenes.speaker", "Cutscene line needs a speaker.");
            }
            var hasText = !string.IsNullOrWhiteSpace(line.Text);
            var hasLineKey = !string.IsNullOrWhiteSpace(line.LineKey);
            if (!hasText && !hasLineKey)
            {
                yield return new ValidationIssue("mission.cutscenes.text", "Cutscene line needs text or a localization lineKey.");
            }
            if (line.DurationSeconds <= 0.0)
            {
                yield return new ValidationIssue("mission.cutscenes.duration", "Cutscene duration must be positive.");
            }
        }
    }
}
