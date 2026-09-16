using System.IO.Compression;

namespace DumpSrv;

public static class OracleArchive
{
    public static (int Extracted, int Missing) Extract(string archivePath, string gameDirectory,
        bool renderer, int percentage, int shardIndex, int shardCount,
        int camera = 2, int target = 0, string? shardPlanPath = null,
        CancellationToken cancellation = default)
    {
        ArgumentOutOfRangeException.ThrowIfLessThan(camera, 1);
        ArgumentOutOfRangeException.ThrowIfGreaterThan(camera, 4);
        ArgumentOutOfRangeException.ThrowIfLessThan(target, 0);
        ArgumentOutOfRangeException.ThrowIfGreaterThan(target, 1);
        var replays = ReplayCatalog.Discover(gameDirectory, cancellation);
        var rendererReplays = ReplayCatalog.RendererReplays(gameDirectory, replays,
            target, cancellation);
        var plan = ShardPlan.Load(shardPlanPath, replays, rendererReplays,
            percentage, shardCount, target);
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
            var pendingPath = DosFiles.Resolve(gameDirectory, name + ".pending");
            File.WriteAllText(pendingPath, "");
            entry.ExtractToFile(DosFiles.Resolve(gameDirectory, name), overwrite: true);
            if (renderer)
            {
                File.WriteAllText(DosFiles.Resolve(gameDirectory, name + ".settings"),
                    $"{camera} {target}");
            }
            File.Delete(pendingPath);
            extracted++;
        }
        return (extracted, assigned.Count - extracted);
    }
}
