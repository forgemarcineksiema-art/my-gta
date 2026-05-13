using System.IO;

namespace BlokTools.Core;

public sealed class MissionEditorSession
{
    private readonly ObjectOutcomeCatalog? outcomeCatalog;
    private readonly MissionLocalization? localization;

    private MissionEditorSession(
        string path,
        MissionDocument mission,
        ObjectOutcomeCatalog? outcomeCatalog,
        MissionLocalization? localization)
    {
        Path = path;
        Mission = mission;
        this.outcomeCatalog = outcomeCatalog;
        this.localization = localization;
    }

    public string Path { get; }
    public MissionDocument Mission { get; }

    public IReadOnlyList<ValidationIssue> Issues => MissionDocumentValidator.Validate(Mission, outcomeCatalog, localization).ToList();

    public static MissionEditorSession Load(
        string path,
        ObjectOutcomeCatalog? outcomeCatalog = null,
        MissionLocalization? localization = null)
    {
        return new MissionEditorSession(path, MissionDocumentStore.Load(path), outcomeCatalog, localization);
    }

    public void UpdateStepObjective(int index, string objective)
    {
        Mission.Steps[index].Objective = objective;
    }

    public void UpdateStepTrigger(int index, string trigger)
    {
        Mission.Steps[index].Trigger = trigger;
    }

    public void UpdateDialogueSpeaker(int index, string speaker)
    {
        Mission.Dialogue[index].Speaker = speaker;
    }

    public void UpdateDialogueText(int index, string text)
    {
        Mission.Dialogue[index].Text = text;
    }

    public void UpdateDialoguePhase(int index, string phase)
    {
        Mission.Dialogue[index].Phase = phase;
    }

    public void UpdateDialogueLineKey(int index, string lineKey)
    {
        Mission.Dialogue[index].LineKey = lineKey;
    }

    public void UpdateDialogueDuration(int index, double durationSeconds)
    {
        Mission.Dialogue[index].DurationSeconds = durationSeconds;
    }

    public void UpdateNpcReactionSpeaker(int index, string speaker)
    {
        Mission.NpcReactions[index].Speaker = speaker;
    }

    public void UpdateNpcReactionText(int index, string text)
    {
        Mission.NpcReactions[index].Text = text;
    }

    public void UpdateNpcReactionLineKey(int index, string lineKey)
    {
        Mission.NpcReactions[index].LineKey = lineKey;
    }

    public void UpdateNpcReactionDuration(int index, double durationSeconds)
    {
        Mission.NpcReactions[index].DurationSeconds = durationSeconds;
    }

    public void AddNpcReaction(string phase, string speaker, string lineKey, string text, double durationSeconds)
    {
        Mission.NpcReactions.Add(new MissionNpcReaction
        {
            Phase = phase,
            Speaker = speaker,
            LineKey = lineKey,
            Text = text,
            DurationSeconds = durationSeconds,
        });
    }

    public void UpdateCutsceneCutscene(int index, string cutscene)
    {
        Mission.Cutscenes[index].Cutscene = cutscene;
    }

    public void UpdateCutscenePhase(int index, string phase)
    {
        Mission.Cutscenes[index].Phase = phase;
    }

    public void UpdateCutsceneSpeaker(int index, string speaker)
    {
        Mission.Cutscenes[index].Speaker = speaker;
    }

    public void UpdateCutsceneText(int index, string text)
    {
        Mission.Cutscenes[index].Text = text;
    }

    public void UpdateCutsceneLineKey(int index, string lineKey)
    {
        Mission.Cutscenes[index].LineKey = lineKey;
    }

    public void UpdateCutsceneDuration(int index, double durationSeconds)
    {
        Mission.Cutscenes[index].DurationSeconds = durationSeconds;
    }

    public void AddCutsceneLine(string cutscene, string phase, string speaker, string lineKey, string text, double durationSeconds)
    {
        Mission.Cutscenes.Add(new MissionCutsceneLine
        {
            Cutscene = cutscene,
            Phase = phase,
            Speaker = speaker,
            LineKey = lineKey,
            Text = text,
            DurationSeconds = durationSeconds,
        });
    }

    public void AddStep(string phase, string objective, string trigger)
    {
        Mission.Steps.Add(new MissionStep
        {
            Phase = phase,
            Objective = objective,
            Trigger = trigger,
        });
    }

    public string AddSuggestedStep(string objective, string trigger)
    {
        var phase = MissionPhaseCatalog.FirstAvailable(Mission.Steps.Select(step => step.Phase));
        AddStep(phase, objective, trigger);
        return phase;
    }

    public void AddDialogueLine(string speaker, string text, double durationSeconds)
    {
        AddDialogueLine(speaker, "ReachVehicle", string.Empty, text, durationSeconds);
    }

    public void AddDialogueLine(string speaker, string lineKey, string text, double durationSeconds)
    {
        AddDialogueLine(speaker, "ReachVehicle", lineKey, text, durationSeconds);
    }

    public void AddDialogueLine(string speaker, string phase, string lineKey, string text, double durationSeconds)
    {
        Mission.Dialogue.Add(new MissionDialogueLine
        {
            Speaker = speaker,
            Phase = phase,
            LineKey = lineKey,
            Text = text,
            DurationSeconds = durationSeconds,
        });
    }

    public void BindStepTriggerToOutcome(int stepIndex, ObjectOutcomeDefinition outcome)
    {
        Mission.Steps[stepIndex].Trigger = "outcome:" + outcome.Key;
    }

    public MissionSaveResult Save()
    {
        var issues = Issues;
        if (issues.Count > 0)
        {
            return MissionSaveResult.Failed(issues);
        }

        return MissionDocumentStore.SaveValidated(Path, Mission, outcomeCatalog, localization);
    }
}

public sealed class MissionSaveResult
{
    private MissionSaveResult(bool saved, string backupPath, IReadOnlyList<ValidationIssue> issues)
    {
        Saved = saved;
        BackupPath = backupPath;
        Issues = issues;
    }

    public bool Saved { get; }
    public string BackupPath { get; }
    public IReadOnlyList<ValidationIssue> Issues { get; }

    public static MissionSaveResult Success(string backupPath)
    {
        return new MissionSaveResult(true, backupPath, Array.Empty<ValidationIssue>());
    }

    public static MissionSaveResult Failed(IReadOnlyList<ValidationIssue> issues)
    {
        return new MissionSaveResult(false, "", issues);
    }
}
