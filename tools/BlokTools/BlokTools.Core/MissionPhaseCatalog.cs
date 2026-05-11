namespace BlokTools.Core;

public static class MissionPhaseCatalog
{
    private static readonly string[] RuntimePhaseNames =
    {
        "WalkToShop",
        "ReturnToBench",
        "ReachVehicle",
        "DriveToShop",
        "ChaseActive",
        "ReturnToLot",
        "Completed",
    };

    public static IReadOnlyList<string> SupportedPhaseNames => RuntimePhaseNames;

    public static bool IsSupported(string phase)
    {
        return RuntimePhaseNames.Contains(phase, StringComparer.Ordinal);
    }

    public static string FirstAvailable(IEnumerable<string> usedPhases)
    {
        var used = usedPhases.ToHashSet(StringComparer.Ordinal);
        foreach (var phase in RuntimePhaseNames)
        {
            if (!used.Contains(phase))
            {
                return phase;
            }
        }

        throw new InvalidOperationException("All runtime-supported mission phases are already present.");
    }
}
