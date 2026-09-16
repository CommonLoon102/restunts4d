using DumpSrv;
using System.IO.Compression;

namespace DumpSrv.Tests;

public sealed class OracleArchiveTests
{
    [Theory]
    [InlineData(false, 100, 0)]
    [InlineData(true, 100, 0)]
    [InlineData(true, 13, 0)]
    [InlineData(false, 100, 1)]
    [InlineData(true, 100, 1)]
    [InlineData(true, 13, 1)]
    public void ExtractionUsesTheRunnersSamplingAndShardAssignment(
        bool renderer, int percentage, int target)
    {
        using var source = new EngineDirectory();
        var extension = renderer ? ".PDO" : ".BIN";
        var replays = Enumerable.Range(0, 37).Select(index => $"t{index:000}.rpl").ToArray();
        var zipPath = Path.Combine(source.Path, "oracles.zip");
        using (var archive = ZipFile.Open(zipPath, ZipArchiveMode.Create))
        {
            foreach (var replay in replays)
            {
                using var writer = new StreamWriter(archive.CreateEntry(
                    Path.ChangeExtension(replay.ToUpperInvariant(), extension)).Open());
                writer.Write(replay);
            }
            archive.CreateEntry("../unrelated.txt");
            archive.CreateEntry("repldump.exe");
        }
        var opponents = replays.Where((_, index) => index % 3 == 1).ToArray();
        var sample = ReplayCatalog.Sample(target == 1 ? opponents : replays, percentage);
        var assignments = Enumerable.Range(0, 5).Select(index => new ReplayShard
        {
            Physics = replays.Skip(index * 8).Take(8).Reverse().ToList(),
            Renderer = sample.Skip(index * 8).Take(8).Reverse().ToList()
        }).ToArray();
        var planPath = TestShardPlans.Write(source.Path, percentage, target, assignments);
        for (var shard = 0; shard < 5; shard++)
        {
            using var game = new EngineDirectory();
            foreach (var replay in replays)
            {
                game.WriteReplay(replay, opponents.Contains(replay) ? (byte)1 : (byte)0);
            }
            var expected = renderer ? assignments[shard].Renderer : assignments[shard].Physics;
            var result = OracleArchive.Extract(zipPath, game.Path, renderer, percentage, shard, 5,
                camera: 4, target: target, shardPlanPath: planPath,
                cancellation: TestContext.Current.CancellationToken);
            Assert.Equal(expected.Count, result.Extracted);
            Assert.Equal(0, result.Missing);
            Assert.Equal(expected.Select(replay => Path.ChangeExtension(replay, extension)).Order(),
                Directory.GetFiles(game.Path, "*" + extension).Select(Path.GetFileName).Order());
            foreach (var replay in expected)
            {
                Assert.Equal(replay, File.ReadAllText(
                    Path.Combine(game.Path, Path.ChangeExtension(replay, extension))));
            }
            if (renderer)
            {
                foreach (var replay in expected)
                {
                    Assert.Equal($"4 {target}", File.ReadAllText(Path.Combine(game.Path,
                        Path.ChangeExtension(replay, ".PDO.settings"))));
                }
            }
            Assert.Equal(replays.Length + expected.Count * (renderer ? 2 : 1),
                Directory.GetFiles(game.Path).Length);
        }
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task CommandExtractsMixedCaseCachesAndLeavesMissingOutputsForTheRunner(
        bool renderer)
    {
        using var game = new EngineDirectory();
        game.Write("track.rpl");
        game.Write("missing.rpl");
        var extension = renderer ? "PDO" : "BIN";
        game.Write($"track.{extension.ToLowerInvariant()}", "stale");
        game.Write($"TRACK.{extension}.pending");
        var zipPath = Path.Combine(game.Path, "oracles.zip");
        using (var archive = ZipFile.Open(zipPath, ZipArchiveMode.Create))
        {
            using var writer = new StreamWriter(archive.CreateEntry($"TRACK.{extension}").Open());
            writer.Write("precomputed");
        }
        var code = await CommandLine.ExecuteAsync(
            ["extract-oracles", "-Archive", zipPath, "-GameDirectory", game.Path,
                "-Renderer", renderer.ToString(), "-Camera", "3", "-Target", "0"],
            TestContext.Current.CancellationToken);
        Assert.Equal(0, code);
        Assert.Equal("precomputed",
            File.ReadAllText(DosFiles.Resolve(game.Path, $"track.{extension}")));
        Assert.Single(Directory.GetFiles(game.Path), path =>
            Path.GetExtension(path).Equals($".{extension}", StringComparison.OrdinalIgnoreCase));
        Assert.False(File.Exists(Path.Combine(game.Path, $"TRACK.{extension}.pending")));
        Assert.False(File.Exists(Path.Combine(game.Path, $"missing.{extension}")));
        if (renderer)
        {
            Assert.Equal("3 0",
                File.ReadAllText(DosFiles.Resolve(game.Path, "track.PDO.settings")));
        }
    }

    [Theory]
    [InlineData("-ShardCount", "0")]
    [InlineData("-ShardIndex", "1")]
    [InlineData("-RendererTestPercentage", "0")]
    public async Task CommandRejectsInvalidSelection(string option, string value)
    {
        using var game = new EngineDirectory();
        game.Write("track.rpl");
        var code = await CommandLine.ExecuteAsync(
            ["extract-oracles", "-Archive", "unused.zip", "-GameDirectory", game.Path, option, value],
            TestContext.Current.CancellationToken);
        Assert.Equal(2, code);
    }

    [Fact]
    public async Task CommandFailsForInvalidArchives()
    {
        using var game = new EngineDirectory();
        game.Write("track.rpl");
        game.Write("oracles.zip", "not a zip");
        var code = await CommandLine.ExecuteAsync(
            ["extract-oracles", "-Archive", Path.Combine(game.Path, "oracles.zip"),
                "-GameDirectory", game.Path], TestContext.Current.CancellationToken);
        Assert.Equal(1, code);
        Assert.False(File.Exists(Path.Combine(game.Path, "track.BIN")));
    }
}
