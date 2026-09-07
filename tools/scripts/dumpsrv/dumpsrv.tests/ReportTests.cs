using System.Text.Json;
using DumpSrv;

namespace DumpSrv.Tests;

public sealed class ReportTests : IDisposable
{
    private readonly string directory = Path.Combine(Path.GetTempPath(), "dumpsrv-report-" + Guid.NewGuid().ToString("N"));

    public ReportTests()
    {
        Directory.CreateDirectory(Path.Combine(directory, "replays"));
        Directory.CreateDirectory(Path.Combine(directory, "results"));
        foreach (var name in new[] { "ALPHA.RPL", "b.RpL", "race-2.rpl", "track.rpl", "z.RPL" })
        {
            File.WriteAllText(Path.Combine(directory, "replays", name), "replay");
        }
    }

    [Fact]
    public async Task MergeReadsIdentitiesFromContentsAndWritesEmptyReportForCompleteCoverage()
    {
        await WriteShards();
        var result = await ResultMerger.MergeAsync(Options(), TestContext.Current.CancellationToken);
        Assert.True(result.Success);
        Assert.Empty(await File.ReadAllTextAsync(Options().OutputFile, TestContext.Current.CancellationToken));
        Assert.Contains("5/5 physics and 2/2 renderer", result.Summary);
        Assert.Contains("agree byte for byte", await File.ReadAllTextAsync(Options().SummaryFile!, TestContext.Current.CancellationToken));
    }

    [Theory]
    [InlineData("missing", "missing_shard")]
    [InlineData("duplicate", "duplicate_shard")]
    [InlineData("truncated", "invalid_result")]
    [InlineData("missing_diagnostics", "invalid_result")]
    [InlineData("duplicate_property", "invalid_result")]
    [InlineData("null_diagnostics", "invalid_result")]
    [InlineData("incomplete", "incomplete_run")]
    [InlineData("wrong_settings", "inconsistent_shard")]
    [InlineData("wrong_timeout", "inconsistent_shard")]
    [InlineData("wrong_corpus", "inconsistent_shard")]
    [InlineData("missing_replay", "missing_replay")]
    [InlineData("duplicate_replay", "duplicate_replay")]
    [InlineData("substituted_replay", "unexpected_replay")]
    public async Task MergeCannotPassIncompleteOrInconsistentCoverage(string mutation, string expectedType)
    {
        var shards = await WriteShards();
        var path = Path.Combine(directory, "results", "arbitrary-a.json");
        switch (mutation)
        {
            case "missing":
                File.Delete(path);
                break;
            case "duplicate":
                File.Copy(path, Path.Combine(directory, "results", "duplicate.json"));
                break;
            case "truncated":
                await File.WriteAllTextAsync(path, "{", TestContext.Current.CancellationToken);
                break;
            case "missing_diagnostics":
            case "null_diagnostics":
                var json = System.Text.Json.Nodes.JsonNode.Parse(JsonSerializer.Serialize(shards[0]))!;
                if (mutation == "missing_diagnostics")
                {
                    json.AsObject().Remove("Diagnostics");
                }
                else
                {
                    json["Diagnostics"] = null;
                }
                await File.WriteAllTextAsync(path, json.ToJsonString(), TestContext.Current.CancellationToken);
                break;
            case "duplicate_property":
                var original = JsonSerializer.Serialize(shards[0]);
                await File.WriteAllTextAsync(path, original[..^1] + ",\"Diagnostics\":[]}",
                    TestContext.Current.CancellationToken);
                break;
            default:
                var shard = shards[0];
                switch (mutation)
                {
                    case "incomplete": shard.Completed = false; break;
                    case "wrong_settings": shard.RendererTestPercentage = 100; break;
                    case "wrong_timeout": shard.DosBoxTimeoutSeconds = 120; break;
                    case "wrong_corpus": shard.ReplayFiles = ["different.rpl"]; break;
                    case "missing_replay": shard.PhysicsCompleted.RemoveAt(0); break;
                    case "duplicate_replay": shard.PhysicsCompleted.Add(shard.PhysicsCompleted[0]); break;
                    case "substituted_replay": shard.PhysicsCompleted[0] = "other.rpl"; break;
                }
                await File.WriteAllTextAsync(path, JsonSerializer.Serialize(shard), TestContext.Current.CancellationToken);
                break;
        }
        var result = await ResultMerger.MergeAsync(Options(), TestContext.Current.CancellationToken);
        Assert.False(result.Success);
        Assert.Contains(result.Diagnostics, line => line.Contains($"type={expectedType}|", StringComparison.Ordinal) ||
            line.EndsWith($"type={expectedType}", StringComparison.Ordinal));
        Assert.NotEmpty(await File.ReadAllTextAsync(Options().OutputFile, TestContext.Current.CancellationToken));
    }

    [Fact]
    public async Task MissingResultsAndEmptyCorpusStillProduceFailureReport()
    {
        Directory.Delete(Path.Combine(directory, "results"));
        Directory.Delete(Path.Combine(directory, "replays"), true);
        Directory.CreateDirectory(Path.Combine(directory, "replays"));
        var result = await ResultMerger.MergeAsync(Options(), TestContext.Current.CancellationToken);
        Assert.False(result.Success);
        Assert.Contains(result.Diagnostics, line => line.Contains("type=invalid_corpus", StringComparison.Ordinal));
        Assert.Contains(result.Diagnostics, line => line.Contains("type=missing_shard", StringComparison.Ordinal));
        Assert.NotEmpty(await File.ReadAllTextAsync(Options().OutputFile, TestContext.Current.CancellationToken));
    }

    [Fact]
    public async Task RendererOnlyMergeChecksThatPhysicsResultsAreAbsent()
    {
        var shards = await WriteShards();
        foreach (var shard in shards)
        {
            shard.PhysicsTests = false;
            shard.PhysicsCompleted.Clear();
            await File.WriteAllTextAsync(Path.Combine(directory, "results", $"arbitrary-{(char)('a' + shard.ShardIndex)}.json"),
                JsonSerializer.Serialize(shard), TestContext.Current.CancellationToken);
        }
        Assert.True((await ResultMerger.MergeAsync(Options() with { PhysicsTests = false }, TestContext.Current.CancellationToken)).Success);
    }

    [Fact]
    public async Task ResultsAreUtf8WithoutBomAndFailedRunCannotHaveEmptyReport()
    {
        var shard = new ShardResult { ShardIndex = 2, Completed = false, Failure = "interrupted" };
        await ResultFiles.WriteAsync(shard, Path.Combine(directory, "single"), CancellationToken.None);
        var path = Path.Combine(directory, "single", "partitions_all.txt");
        var bytes = await File.ReadAllBytesAsync(path, TestContext.Current.CancellationToken);
        Assert.Equal((byte)'E', bytes[0]);
        Assert.Contains("type=incomplete_run", await File.ReadAllTextAsync(path, TestContext.Current.CancellationToken));
        Assert.Contains(path, shard.OwnedFiles);
        Assert.Contains(Path.Combine(directory, "single", "shard-2.json"), shard.OwnedFiles);
    }

    [Fact]
    public void DiagnosticsAreDeduplicatedAndSortedByInputThenCompleteLine()
    {
        const string alpha = "ERROR|type=file_mismatch|input=ALPHA.RPL|bin=ALPHA.BIN|bni=ALPHA.BNI";
        const string zeta = "ERROR|type=missing_output|input=z.RPL|output=z.PDD";
        Assert.Equal(alpha + "\n" + zeta + "\n", ReportFormatter.Text([zeta, alpha, alpha]));
        Assert.Empty(ReportFormatter.Text([]));
    }

    private MergeOptions Options() => new()
    {
        ReplayDirectory = Path.Combine(directory, "replays"),
        ResultsDirectory = Path.Combine(directory, "results"),
        OutputFile = Path.Combine(directory, "partitions_all.txt"),
        SummaryFile = Path.Combine(directory, "summary.md"),
        ShardCount = 3,
        RendererTestPercentage = 40
    };

    private async Task<List<ShardResult>> WriteShards()
    {
        var corpus = ReplayCatalog.Discover(Options().ReplayDirectory);
        var results = new List<ShardResult>();
        for (var index = 0; index < 3; index++)
        {
            var result = new ShardResult
            {
                ShardIndex = index,
                ShardCount = 3,
                PartitionCount = 2,
                PhysicsTests = true,
                RendererTests = true,
                RendererTestPercentage = 40,
                ReplayFiles = corpus.ToList(),
                PhysicsCompleted = ReplayCatalog.Assigned(corpus, false, 40, index, 3).ToList(),
                RendererCompleted = ReplayCatalog.Assigned(corpus, true, 40, index, 3).ToList(),
                Completed = true
            };
            await File.WriteAllTextAsync(Path.Combine(directory, "results", $"arbitrary-{(char)('a' + index)}.json"),
                JsonSerializer.Serialize(result), TestContext.Current.CancellationToken);
            results.Add(result);
        }
        return results;
    }

    public void Dispose() => Directory.Delete(directory, true);
}
