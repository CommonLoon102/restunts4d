using System.IO.Compression;

namespace DumpSrv;

public static class OracleArchive
{
    public static (int Extracted, int Missing) Extract(string archivePath, string gameDirectory,
        bool renderer, int percentage, int shardIndex, int shardCount,
        string? shardPlanPath = null, CancellationToken cancellation = default)
    {
        var replays = ReplayCatalog.Discover(gameDirectory, cancellation);
        var plan = ShardPlan.Load(shardPlanPath, replays, percentage, shardCount);
        var assigned = plan.Assigned(renderer, shardIndex);
        using var archive = ZipFile.OpenRead(archivePath);
        var entries = archive.Entries.ToDictionary(entry => entry.FullName,
            StringComparer.OrdinalIgnoreCase);
        var extension = renderer ? ".PDO" : ".BIN";
        var extracted = 0;
        foreach (var replay in assigned)
        {
            cancellation.ThrowIfCancellationRequested();
            var name = Path.ChangeExtension(replay, extension);
            if (!entries.TryGetValue(name, out var entry))
            {
                // The regression engine generates any missing oracle outputs.
                continue;
            }
            entry.ExtractToFile(DosFiles.Resolve(gameDirectory, name), overwrite: true);
            File.Delete(DosFiles.Resolve(gameDirectory, name + ".pending"));
            extracted++;
        }
        return (extracted, assigned.Count - extracted);
    }
}
