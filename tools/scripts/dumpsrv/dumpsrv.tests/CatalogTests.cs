using DumpSrv;

namespace DumpSrv.Tests;

public sealed class CatalogTests
{
    [Fact]
    public void DiscoveryAcceptsNonnumericNamesAndMixedExtensionsInOrdinalOrder()
    {
        using var directory = new EngineDirectory();
        foreach (var name in new[] { "zebra.RpL", "Alpha.rpl", "BETA.RPL", "short-1.rPl", "track#1.rpl" })
        {
            directory.Write(name);
        }
        Directory.CreateDirectory(Path.Combine(directory.Path, "nested"));
        directory.Write("nested/ignored.rpl");
        directory.Write("readme.txt");
        Assert.Equal(new[] { "Alpha.rpl", "BETA.RPL", "short-1.rPl", "track#1.rpl", "zebra.RpL" },
            ReplayCatalog.Discover(directory.Path, TestContext.Current.CancellationToken));
    }

    [Theory]
    [InlineData("toolong99")]
    [InlineData("two words")]
    [InlineData("bad%var")]
    [InlineData("bad&cmd")]
    [InlineData("bad^cmd")]
    [InlineData("a.b")]
    [InlineData("CON")]
    [InlineData("lpt9")]
    [InlineData("CLOCK$")]
    [InlineData("NUL")]
    [InlineData("é")]
    public void UnsafeOrUnsupportedBasenamesAreRejected(string basename)
    {
        Assert.Throws<InvalidDataException>(() => ReplayCatalog.ValidateBaseName(basename));
    }

    [Fact]
    public void EmptyCorpusIsRejected()
    {
        using var directory = new EngineDirectory();
        Assert.Throws<InvalidDataException>(() => ReplayCatalog.Discover(directory.Path, TestContext.Current.CancellationToken));
    }

    [Fact]
    public void ReplayAndOutputCaseCollisionsAreRejectedOnCaseSensitiveHosts()
    {
        using var directory = new EngineDirectory();
        directory.Write("track.rpl");
        directory.Write("TRACK.RPL");
        if (Directory.GetFiles(directory.Path).Length != 2)
        {
            return;
        }
        Assert.Throws<InvalidDataException>(() => ReplayCatalog.Discover(directory.Path, TestContext.Current.CancellationToken));
        directory.Write("track.bin");
        directory.Write("TRACK.BIN");
        Assert.Throws<InvalidDataException>(() => DosFiles.Resolve(directory.Path, "Track.BIN"));
    }

    [Fact]
    public void RoundRobinHasExactlyOnceBalancedCoverageIncludingEmptyWorkers()
    {
        foreach (var length in new[] { 0, 1, 3, 11, 28 })
        {
            var items = Enumerable.Range(0, length).Select(index => $"replay-{index}").ToArray();
            foreach (var count in new[] { 1, 2, 5, 64 })
            {
                var lists = ReplayCatalog.RoundRobin(items, count);
                Assert.Equal(count, lists.Count);
                Assert.Equal(items.Order(), lists.SelectMany(list => list).Order());
                Assert.True(lists.Max(list => list.Count) - lists.Min(list => list.Count) <= 1);
                for (var index = 0; index < items.Length; index++)
                {
                    Assert.Equal(items[index], lists[index % count][index / count]);
                }
            }
        }
    }

    [Fact]
    public void SingleShardRendererSamplingUsesEvenlySpacedEntries()
    {
        var corpus = Enumerable.Range(0, 37).Select(index => $"replay-{index}").ToArray();
        var sample = ReplayCatalog.Sample(corpus, 13);
        Assert.Equal(new[] { "replay-0", "replay-7", "replay-14", "replay-22", "replay-29" }, sample);
    }

}

internal sealed class EngineDirectory : IDisposable
{
    public string Path { get; } = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "dumpsrv-engine-" + Guid.NewGuid());

    public EngineDirectory()
    {
        Directory.CreateDirectory(Path);
    }

    public void Write(string relativePath, string content = "") => File.WriteAllText(System.IO.Path.Combine(Path, relativePath), content);

    public void Dispose() => Directory.Delete(Path, true);
}
