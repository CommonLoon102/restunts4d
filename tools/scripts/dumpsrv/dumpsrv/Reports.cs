using System.Text;
using System.Text.Json;

namespace DumpSrv;

public static class ReportFormatter
{
    public static IReadOnlyList<string> Lines(IEnumerable<string> diagnostics) => diagnostics
        .Distinct(StringComparer.Ordinal)
        .OrderBy(Input, StringComparer.Ordinal)
        .ThenBy(line => line, StringComparer.Ordinal)
        .ToArray();

    public static string Text(IEnumerable<string> diagnostics)
    {
        var lines = Lines(diagnostics);
        return lines.Count == 0 ? "" : string.Join('\n', lines) + "\n";
    }

    public static string Safe(string value) => value.Replace('\r', ' ').Replace('\n', ' ').Replace('|', ' ');

    public static string RunSummary(ShardResult result) =>
        $"Shard {result.ShardIndex}: {result.PhysicsCompleted.Count} physics, " +
        $"{result.RendererCompleted.Count} renderer replays; {result.Diagnostics.Count} diagnostic(s); " +
        (result.Completed ? "complete." : $"incomplete: {result.Failure}");

    public static IEnumerable<string> Diagnostics(ShardResult result)
    {
        foreach (var line in result.Diagnostics)
        {
            yield return line;
        }
        if (!result.Completed || result.Failure is not null)
        {
            yield return $"ERROR|type=incomplete_run|shard={result.ShardIndex}|" +
                $"message={Safe(result.Failure ?? "Processing did not finish.")}";
        }
    }

    private static string Input(string line) =>
        line.Split('|').FirstOrDefault(field => field.StartsWith("input=", StringComparison.Ordinal))?[6..] ?? "";
}

public static class ResultFiles
{
    public static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true,
        AllowDuplicateProperties = false
    };

    public static async Task WriteAsync(ShardResult result, string outputDirectory, CancellationToken cancellation)
    {
        Directory.CreateDirectory(outputDirectory);
        var jsonPath = Path.Combine(outputDirectory, $"shard-{result.ShardIndex}.json");
        var reportPath = Path.Combine(outputDirectory, "partitions_all.txt");
        await WriteTextAsync(jsonPath, JsonSerializer.Serialize(result, JsonOptions), cancellation);
        result.OwnedFiles.Add(jsonPath);
        await WriteTextAsync(reportPath, ReportFormatter.Text(ReportFormatter.Diagnostics(result)), cancellation);
        result.OwnedFiles.Add(reportPath);
    }

    public static async Task WriteTextAsync(string path, string text, CancellationToken cancellation)
    {
        path = Path.GetFullPath(path);
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        var temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            await File.WriteAllTextAsync(temporary, text, new UTF8Encoding(false), cancellation);
            cancellation.ThrowIfCancellationRequested();
            File.Move(temporary, path, true);
        }
        finally
        {
            File.Delete(temporary);
        }
    }
}

public sealed record MergeOptions
{
    public required string ReplayDirectory { get; init; }
    public required string ResultsDirectory { get; init; }
    public required string OutputFile { get; init; }
    public string? SummaryFile { get; init; }
    public int ShardCount { get; init; } = 1;
    public bool PhysicsTests { get; init; } = true;
    public bool RendererTests { get; init; } = true;
    public int RendererTestPercentage { get; init; } = 100;
}

public sealed record MergeResult(bool Success, string Summary, IReadOnlyList<string> Diagnostics);

public static class ResultMerger
{
    public static async Task<MergeResult> MergeAsync(MergeOptions options, CancellationToken cancellation = default)
    {
        var diagnostics = new List<string>();
        var results = new Dictionary<int, ShardResult>();
        IReadOnlyList<string> corpus = [];
        try
        {
            corpus = ReplayCatalog.Discover(options.ReplayDirectory);
        }
        catch (Exception e) when (e is ArgumentException or InvalidDataException or IOException or UnauthorizedAccessException)
        {
            diagnostics.Add($"ERROR|type=invalid_corpus|message={ReportFormatter.Safe(e.Message)}");
        }

        string[] files = [];
        try
        {
            files = Directory.Exists(options.ResultsDirectory)
                ? Directory.EnumerateFiles(options.ResultsDirectory, "*", SearchOption.AllDirectories)
                    .Where(path => Path.GetExtension(path).Equals(".json", StringComparison.OrdinalIgnoreCase))
                    .Order(StringComparer.Ordinal).ToArray()
                : [];
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            diagnostics.Add($"ERROR|type=invalid_results_directory|message={ReportFormatter.Safe(e.Message)}");
        }
        foreach (var path in files)
        {
            cancellation.ThrowIfCancellationRequested();
            try
            {
                await using var stream = File.OpenRead(path);
                var result = await JsonSerializer.DeserializeAsync<ShardResult>(stream, ResultFiles.JsonOptions, cancellation);
                if (result is null || result.Version != 1 || result.ReplayFiles is null ||
                    result.PhysicsCompleted is null || result.RendererCompleted is null || result.Diagnostics is null ||
                    result.ReplayFiles.Any(x => x is null) || result.PhysicsCompleted.Any(x => x is null) ||
                    result.RendererCompleted.Any(x => x is null) || result.Diagnostics.Any(x => x is null))
                {
                    throw new InvalidDataException("Malformed or unsupported shard result.");
                }
                if (result.ShardIndex < 0 || result.ShardIndex >= options.ShardCount)
                {
                    diagnostics.Add($"ERROR|type=unexpected_shard|shard={result.ShardIndex}");
                    continue;
                }
                if (!results.TryAdd(result.ShardIndex, result))
                {
                    diagnostics.Add($"ERROR|type=duplicate_shard|shard={result.ShardIndex}");
                    continue;
                }
                diagnostics.AddRange(ReportFormatter.Diagnostics(result));
                if (result.ShardCount != options.ShardCount || result.PartitionCount is < 1 or > 64 ||
                    result.DosBoxTimeoutSeconds is < 1 or > 2147483 ||
                    result.RendererTimeoutSeconds is < 1 or > 2147483 ||
                    result.PhysicsTests != options.PhysicsTests || result.RendererTests != options.RendererTests ||
                    result.RendererTestPercentage != options.RendererTestPercentage ||
                    !result.ReplayFiles.SequenceEqual(corpus, StringComparer.Ordinal))
                {
                    diagnostics.Add($"ERROR|type=inconsistent_shard|shard={result.ShardIndex}");
                }
                CheckCoverage(result.PhysicsCompleted, options.PhysicsTests
                    ? ReplayCatalog.Assigned(corpus, false, options.RendererTestPercentage, result.ShardIndex, options.ShardCount)
                    : [], result.ShardIndex, "physics", diagnostics);
                CheckCoverage(result.RendererCompleted, options.RendererTests
                    ? ReplayCatalog.Assigned(corpus, true, options.RendererTestPercentage, result.ShardIndex, options.ShardCount)
                    : [], result.ShardIndex, "renderer", diagnostics);
            }
            catch (Exception e) when (e is JsonException or InvalidDataException or IOException or UnauthorizedAccessException)
            {
                diagnostics.Add($"ERROR|type=invalid_result|file={ReportFormatter.Safe(Path.GetFileName(path))}|" +
                    $"message={ReportFormatter.Safe(e.Message)}");
            }
        }
        for (var index = 0; index < options.ShardCount; index++)
        {
            if (!results.ContainsKey(index))
            {
                diagnostics.Add($"ERROR|type=missing_shard|shard={index}");
            }
        }
        if (results.Values.Select(result => (result.PartitionCount, result.DosBoxTimeoutSeconds,
            result.RendererTimeoutSeconds)).Distinct().Skip(1).Any())
        {
            diagnostics.Add("ERROR|type=inconsistent_shard|message=Partition counts or executable timeouts differ.");
        }

        var lines = ReportFormatter.Lines(diagnostics);
        var physics = results.Values.Sum(result => result.PhysicsCompleted.Count);
        var renderer = results.Values.Sum(result => result.RendererCompleted.Count);
        var expectedPhysics = options.PhysicsTests ? corpus.Count : 0;
        var expectedRenderer = options.RendererTests ? ReplayCatalog.Sample(corpus, options.RendererTestPercentage).Count : 0;
        var summary = $"{physics}/{expectedPhysics} physics and {renderer}/{expectedRenderer} renderer replays; " +
            $"{lines.Count} error(s).";
        await ResultFiles.WriteTextAsync(options.OutputFile, ReportFormatter.Text(lines), cancellation);
        if (options.SummaryFile is not null)
        {
            var markdown = new StringBuilder("## Replay validation\n\n| | |\n|---|---|\n")
                .AppendLine($"| Replays in the golden set | {corpus.Count} |")
                .AppendLine($"| Physics replays processed | {physics} |")
                .AppendLine($"| Renderer sample percentage | {options.RendererTestPercentage}% |")
                .AppendLine($"| Renderer replays expected | {expectedRenderer} |")
                .AppendLine($"| Renderer replays processed | {renderer} |")
                .AppendLine($"| Errors | {lines.Count} |")
                .AppendLine();
            if (lines.Count == 0)
            {
                markdown.AppendLine($"{physics} physics and {renderer} renderer replays agree byte for byte.");
            }
            else
            {
                markdown.AppendLine("Validation failed. Full diagnostics are in the `partitions_all` artifact.\n")
                    .AppendLine("### Errors by type\n");
                foreach (var group in lines.GroupBy(line => line.Split('|')
                    .FirstOrDefault(field => field.StartsWith("type=", StringComparison.Ordinal)) ?? "type=unknown")
                    .OrderByDescending(group => group.Count()).ThenBy(group => group.Key, StringComparer.Ordinal))
                {
                    markdown.AppendLine($"- {group.Key[5..]}: {group.Count()}");
                }
                markdown.AppendLine("\n### partitions_all.txt (first 200 lines)\n\n````")
                    .Append(ReportFormatter.Text(lines.Take(200))).AppendLine("````");
            }
            await File.AppendAllTextAsync(options.SummaryFile, markdown.ToString(), new UTF8Encoding(false), cancellation);
        }
        return new MergeResult(lines.Count == 0, summary, lines);
    }

    private static void CheckCoverage(IReadOnlyList<string> actual, IReadOnlyList<string> expected,
        int shard, string phase, List<string> diagnostics)
    {
        var actualSet = actual.ToHashSet(StringComparer.Ordinal);
        var expectedSet = expected.ToHashSet(StringComparer.Ordinal);
        foreach (var duplicate in actual.GroupBy(name => name, StringComparer.Ordinal).Where(group => group.Count() > 1))
        {
            diagnostics.Add($"ERROR|type=duplicate_replay|phase={phase}|shard={shard}|input={ReportFormatter.Safe(duplicate.Key)}");
        }
        foreach (var missing in expectedSet.Except(actualSet, StringComparer.Ordinal))
        {
            diagnostics.Add($"ERROR|type=missing_replay|phase={phase}|shard={shard}|input={missing}");
        }
        foreach (var unexpected in actualSet.Except(expectedSet, StringComparer.Ordinal))
        {
            diagnostics.Add($"ERROR|type=unexpected_replay|phase={phase}|shard={shard}|input={ReportFormatter.Safe(unexpected)}");
        }
    }
}
