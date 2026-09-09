using DumpSrv;
using System.IO.Compression;

namespace DumpSrv.Tests;

public sealed class OracleArchiveTests
{
    [Theory]
    [InlineData(false, 100)]
    [InlineData(true, 100)]
    [InlineData(true, 13)]
    public void ExtractionUsesTheRunnersSamplingAndShardAssignment(bool renderer, int percentage)
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
        for (var shard = 0; shard < 5; shard++)
        {
            using var game = new EngineDirectory();
            foreach (var replay in replays)
            {
                game.Write(replay);
            }
            var expected = ReplayCatalog.Assigned(replays, renderer, percentage, shard, 5);
            var result = OracleArchive.Extract(zipPath, game.Path, renderer, percentage, shard, 5,
                TestContext.Current.CancellationToken);
            Assert.Equal(expected.Count, result.Extracted);
            Assert.Equal(0, result.Missing);
            Assert.Equal(expected.Select(replay => Path.ChangeExtension(replay, extension)),
                Directory.GetFiles(game.Path, "*" + extension).Select(Path.GetFileName).Order());
            foreach (var replay in expected)
            {
                Assert.Equal(replay, File.ReadAllText(
                    Path.Combine(game.Path, Path.ChangeExtension(replay, extension))));
            }
            Assert.Equal(replays.Length + expected.Count, Directory.GetFiles(game.Path).Length);
        }
    }

    [Fact]
    public async Task CommandExtractsMixedCaseCachesAndLeavesMissingOutputsForTheRunner()
    {
        using var game = new EngineDirectory();
        game.Write("track.rpl");
        game.Write("missing.rpl");
        game.Write("track.bin", "stale");
        game.Write("TRACK.BIN.pending");
        var zipPath = Path.Combine(game.Path, "oracles.zip");
        using (var archive = ZipFile.Open(zipPath, ZipArchiveMode.Create))
        {
            using var writer = new StreamWriter(archive.CreateEntry("TRACK.BIN").Open());
            writer.Write("precomputed");
        }
        var code = await CommandLine.ExecuteAsync(
            ["extract-oracles", "-Archive", zipPath, "-GameDirectory", game.Path],
            TestContext.Current.CancellationToken);
        Assert.Equal(0, code);
        Assert.Equal("precomputed", File.ReadAllText(Path.Combine(game.Path, "track.bin")));
        Assert.Single(Directory.GetFiles(game.Path), path =>
            Path.GetExtension(path).Equals(".BIN", StringComparison.OrdinalIgnoreCase));
        Assert.False(File.Exists(Path.Combine(game.Path, "TRACK.BIN.pending")));
        Assert.False(File.Exists(Path.Combine(game.Path, "missing.BIN")));
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
