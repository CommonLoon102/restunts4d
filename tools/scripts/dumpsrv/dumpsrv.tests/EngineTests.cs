using System.Buffers.Binary;
using System.Collections.Concurrent;
using System.Globalization;
using System.Text;
using DumpSrv;

namespace DumpSrv.Tests;

public sealed class EngineTests
{
    [Theory]
    [InlineData(true, true)]
    [InlineData(true, false)]
    [InlineData(false, true)]
    public async Task PhaseDurationsAreIndependentAndExcludeDisabledPhases(bool physics, bool renderer)
    {
        using var directory = CreateGame("race.rpl");
        var clock = new PhaseTimeProvider();
        var runner = new FakeRunner((invocation, _) =>
        {
            clock.Advance(TimeSpan.FromSeconds(invocation.Executable.StartsWith("repl",
                StringComparison.Ordinal) ? 11 : 23));
            WriteOutput(invocation);
            return Task.FromResult(new DosBoxResult(0));
        });
        var result = await new RegressionEngine(runner, _ => { }, clock).RunAsync(
            Options(directory) with { PhysicsTests = physics, RendererTests = renderer },
            TestContext.Current.CancellationToken);
        Assert.True(result.Completed);
        Assert.Equal(physics ? TimeSpan.FromSeconds(22) : (TimeSpan?)null, result.PhysicsElapsed);
        Assert.Equal(renderer ? TimeSpan.FromSeconds(46) : (TimeSpan?)null, result.RendererElapsed);
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task InterruptedPhaseRetainsDurationIncludingCleanup(bool cancelRenderer)
    {
        using var directory = CreateGame("race.rpl");
        using var cancellation = CancellationTokenSource.CreateLinkedTokenSource(
            TestContext.Current.CancellationToken);
        var clock = new PhaseTimeProvider();
        var runner = new FakeRunner((invocation, token) =>
        {
            clock.Advance(TimeSpan.FromSeconds(11));
            if (invocation.Executable.StartsWith("pix", StringComparison.Ordinal) == cancelRenderer)
            {
                cancellation.Cancel();
                clock.Advance(TimeSpan.FromSeconds(3));
                token.ThrowIfCancellationRequested();
            }
            WriteOutput(invocation);
            return Task.FromResult(new DosBoxResult(0));
        });
        var result = await new RegressionEngine(runner, _ => { }, clock).RunAsync(
            Options(directory), cancellation.Token);
        Assert.False(result.Completed);
        Assert.Equal(TimeSpan.FromSeconds(cancelRenderer ? 22 : 14), result.PhysicsElapsed);
        Assert.Equal(cancelRenderer ? TimeSpan.FromSeconds(14) : (TimeSpan?)null, result.RendererElapsed);
    }

    [Fact]
    public async Task EngineRunsBothPhasesAndUsesCompletedCachesWithoutTouchingUnownedFiles()
    {
        using var directory = CreateGame("mix.RpL", "other.rpl");
        File.WriteAllBytes(System.IO.Path.Combine(directory.Path, "MIX.bin"), DumpBytes(false, 6));
        File.WriteAllBytes(System.IO.Path.Combine(directory.Path, "MIX.pdo"), DumpBytes(true, 6));
        directory.Write("unowned.bni", "keep");
        directory.Write("mix.BNI", "stale");
        var calls = new ConcurrentQueue<DosBoxInvocation>();
        var runner = new FakeRunner((invocation, _) =>
        {
            calls.Enqueue(invocation);
            WriteOutput(invocation);
            return Task.FromResult(new DosBoxResult(0));
        });
        var result = await new RegressionEngine(runner, _ => { }).RunAsync(Options(directory), TestContext.Current.CancellationToken);
        Assert.True(result.Completed);
        Assert.Empty(result.Diagnostics);
        Assert.Equal(new[] { "mix.RpL", "other.rpl" }, result.PhysicsCompleted);
        Assert.Equal(result.PhysicsCompleted, result.RendererCompleted);
        Assert.DoesNotContain(calls, call => call.ReplayBaseName == "mix" && call.Executable.EndsWith("o.exe"));
        Assert.All(calls, call => Assert.DoesNotContain('.', call.ReplayBaseName));
        var ordered = calls.ToArray();
        var firstRenderer = Array.FindIndex(ordered, call => call.Executable.StartsWith("pix", StringComparison.Ordinal));
        Assert.All(ordered[..firstRenderer], call => Assert.Equal("1", call.Arguments));
        Assert.All(ordered[firstRenderer..], call => Assert.Equal("2 0", call.Arguments));
        RegressionEngine.Cleanup(result);
        Assert.Equal("keep", File.ReadAllText(System.IO.Path.Combine(directory.Path, "unowned.bni")));
        Assert.All(new[] { "mix.BIN", "mix.PDO", "other.BIN", "other.PDO" }, name =>
            Assert.True(File.Exists(DosFiles.Resolve(directory.Path, name))));
        Assert.DoesNotContain(Directory.GetFiles(directory.Path), path =>
            System.IO.Path.GetExtension(path).Equals(".pdd", StringComparison.OrdinalIgnoreCase));
        Assert.False(File.Exists(DosFiles.Resolve(directory.Path, "mix.BNI")));
    }

    [Fact]
    public async Task InterruptedOracleAndStaleCandidateAreRegenerated()
    {
        using var directory = CreateGame("track.rpl");
        directory.Write("TRACK.BIN", "partial");
        directory.Write("track.bin.pending");
        directory.Write("TRACK.BNI", "stale");
        var calls = new List<string>();
        var runner = new FakeRunner((invocation, _) =>
        {
            calls.Add(invocation.Executable);
            if (invocation.Executable == "repldumo.exe")
            {
                Assert.False(File.Exists(DosFiles.Resolve(directory.Path, "track.BIN")));
                Assert.False(File.Exists(DosFiles.Resolve(directory.Path, "track.BNI")));
                Assert.True(File.Exists(DosFiles.Resolve(directory.Path, "track.BIN.pending")));
            }
            WriteOutput(invocation);
            return Task.FromResult(new DosBoxResult(0));
        });
        var result = await new RegressionEngine(runner, _ => { }).RunAsync(
            Options(directory) with { RendererTests = false }, TestContext.Current.CancellationToken);
        Assert.Equal(new[] { "repldumo.exe", "repldump.exe" }, calls);
        Assert.True(result.Completed);
        Assert.Empty(result.Diagnostics);
        Assert.False(File.Exists(DosFiles.Resolve(directory.Path, "track.BIN.pending")));
    }

    [Fact]
    public async Task ErrorsDoNotSkipRemainingReplaysOrRendererPhase()
    {
        using var directory = CreateGame("missing.rpl", "mismatch.rpl", "timeout.rpl", "failed.rpl");
        var runner = new FakeRunner((invocation, _) =>
        {
            if (invocation.Executable == "repldumo.exe" && invocation.ReplayBaseName == "timeout")
            {
                return Task.FromResult(new DosBoxResult(TimedOut: true));
            }
            if (invocation.Executable == "repldumo.exe" && invocation.ReplayBaseName == "failed")
            {
                return Task.FromResult(new DosBoxResult(ExitCode: 7));
            }
            if (!(invocation.Executable == "repldump.exe" && invocation.ReplayBaseName == "missing"))
            {
                WriteOutput(invocation, invocation.Executable == "repldump.exe" &&
                    invocation.ReplayBaseName == "mismatch");
            }
            return Task.FromResult(new DosBoxResult(0));
        });
        var result = await new RegressionEngine(runner, _ => { }).RunAsync(Options(directory), TestContext.Current.CancellationToken);
        Assert.True(result.Completed);
        Assert.Equal(4, result.PhysicsCompleted.Count);
        Assert.Equal(4, result.RendererCompleted.Count);
        Assert.Equal(4, result.Diagnostics.Count);
        foreach (var type in new[] { "timeout", "dosbox_failure", "missing_output", "file_mismatch" })
        {
            Assert.Contains(result.Diagnostics, line => line.StartsWith($"ERROR|type={type}|", StringComparison.Ordinal));
        }
        Assert.True(File.Exists(DosFiles.Resolve(directory.Path, "timeout.BIN.pending")));
    }

    [Fact]
    public async Task CancellationWaitsForAllWorkersAndKeepsPartialDiagnostics()
    {
        using var directory = CreateGame("alpha.rpl", "beta.rpl", "gamma.rpl");
        using var cancellation = new CancellationTokenSource();
        var started = 0;
        var active = 0;
        var allStarted = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var runner = new FakeRunner(async (invocation, token) =>
        {
            Interlocked.Increment(ref active);
            if (Interlocked.Increment(ref started) == 3)
            {
                allStarted.SetResult();
            }
            try
            {
                await Task.Delay(Timeout.Infinite, token);
                return new DosBoxResult(0);
            }
            finally
            {
                Interlocked.Decrement(ref active);
            }
        });
        var execution = new RegressionEngine(runner, _ => { }).RunAsync(
            Options(directory) with { PartitionCount = 3 }, cancellation.Token);
        await allStarted.Task.WaitAsync(TimeSpan.FromSeconds(5), TestContext.Current.CancellationToken);
        cancellation.Cancel();
        var result = await execution;
        Assert.False(result.Completed);
        Assert.NotNull(result.Failure);
        Assert.Equal(0, active);
        Assert.Equal(3, started);
        Assert.Empty(result.PhysicsCompleted);
        Assert.Empty(result.RendererCompleted);
        Assert.Contains(result.Diagnostics, line => line.StartsWith("ERROR|type=cancelled|", StringComparison.Ordinal));
        Assert.Equal(3, Directory.GetFiles(directory.Path, "*.pending").Length);
    }

    [Fact]
    public async Task BufferedComparisonDetectsEqualLengthDifferencesBeyondFirstBuffer()
    {
        using var directory = CreateGame("long.rpl");
        WriteReplay(directory, "long.rpl", 150);
        var runner = new FakeRunner((invocation, _) =>
        {
            WriteOutput(invocation, invocation.Executable == "repldump.exe");
            return Task.FromResult(new DosBoxResult(0));
        });
        var result = await new RegressionEngine(runner, _ => { }).RunAsync(
            Options(directory) with { RendererTests = false }, TestContext.Current.CancellationToken);
        Assert.Single(result.Diagnostics);
        Assert.StartsWith("ERROR|type=file_mismatch|", result.Diagnostics[0]);
    }

    [Fact]
    public async Task EmptyShardIsCompletedAndInvalidCorpusReturnsFailedResult()
    {
        using var directory = CreateGame("one.rpl");
        var runner = new FakeRunner((_, _) => throw new Xunit.Sdk.XunitException("An empty shard ran DOSBox."));
        var engine = new RegressionEngine(runner, _ => { });
        var emptyShard = await engine.RunAsync(Options(directory) with { ShardCount = 20, ShardIndex = 19 }, TestContext.Current.CancellationToken);
        Assert.True(emptyShard.Completed);
        Assert.Empty(emptyShard.PhysicsCompleted);
        directory.Write("too-long-name.rpl");
        var invalid = await engine.RunAsync(Options(directory), TestContext.Current.CancellationToken);
        Assert.False(invalid.Completed);
        Assert.Contains("Unsupported DOS 8.3", invalid.Failure);
    }

    [Fact]
    public void DosBoxArgumentsMountOnlyGameDataAndForceFastCpu()
    {
        var invocation = new DosBoxInvocation(System.IO.Path.Combine(System.IO.Path.GetTempPath(), "game with spaces"),
            "custom.conf", "pixldump.exe", "replay", "2 0", 60);
        var info = DosBoxRunner.CreateStartInfo(invocation, "custom-dosbox");
        Assert.Equal("custom-dosbox", info.FileName);
        Assert.Contains("cpu core=dynamic", info.ArgumentList);
        Assert.Contains("cpu cycles=max", info.ArgumentList);
        Assert.Contains("-noautoexec", info.ArgumentList);
        Assert.Single(info.ArgumentList, argument => argument.StartsWith("mount ", StringComparison.Ordinal));
        Assert.Contains($"mount c \"{System.IO.Path.GetFullPath(invocation.GameDirectory)}\"", info.ArgumentList);
        Assert.Contains("pixldump.exe \"replay\" 2 0", info.ArgumentList);
        Assert.True(info.RedirectStandardOutput);
        Assert.True(info.RedirectStandardError);
        Assert.False(info.UseShellExecute);
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task RealProcessTimeoutAndCancellationKillParentAndChild(bool cancel)
    {
        if (OperatingSystem.IsWindows())
        {
            return;
        }
        using var directory = new EngineDirectory();
        var script = System.IO.Path.Combine(directory.Path, "fake-dosbox");
        directory.Write("fake-dosbox", "#!/bin/sh\nsleep 120 &\necho $! > child.pid\necho $$ > parent.pid\nwait\n");
        File.SetUnixFileMode(script, UnixFileMode.UserRead | UnixFileMode.UserWrite | UnixFileMode.UserExecute);
        using var cancellation = new CancellationTokenSource();
        var execution = new DosBoxRunner(script).RunAsync(new DosBoxInvocation(directory.Path,
            "unused.conf", "repldump.exe", "track", "1", cancel ? 30 : 1), cancellation.Token);
        await WaitForFileAsync(System.IO.Path.Combine(directory.Path, "parent.pid"));
        var parent = int.Parse(await File.ReadAllTextAsync(System.IO.Path.Combine(directory.Path, "parent.pid"), TestContext.Current.CancellationToken));
        var child = int.Parse(await File.ReadAllTextAsync(System.IO.Path.Combine(directory.Path, "child.pid"), TestContext.Current.CancellationToken));
        if (cancel)
        {
            cancellation.Cancel();
            await Assert.ThrowsAnyAsync<OperationCanceledException>(async () => await execution);
        }
        else
        {
            Assert.True((await execution).TimedOut);
        }
        Assert.False(IsRunning(parent));
        // Kill(true) signals every descendant. Parent WaitForExit does not
        // wait for grandchildren, so observe their actual termination too.
        using var childExit = new CancellationTokenSource(TimeSpan.FromSeconds(5));
        while (IsRunning(child))
        {
            await Task.Delay(10, childExit.Token);
        }
    }

    private static async Task WaitForFileAsync(string path)
    {
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(5));
        while (!File.Exists(path) || new FileInfo(path).Length == 0)
        {
            await Task.Delay(10, timeout.Token);
        }
    }

    private static bool IsRunning(int pid)
    {
        var statusPath = $"/proc/{pid}/stat";
        if (File.Exists(statusPath))
        {
            var state = File.ReadAllText(statusPath).Split(' ')[2];
            return state is not "Z" and not "X";
        }
        return false;
    }

    [Theory]
    [InlineData(false, "empty")]
    [InlineData(false, "partial")]
    [InlineData(false, "record")]
    [InlineData(false, "extra")]
    [InlineData(false, "frame")]
    [InlineData(false, "shorter")]
    [InlineData(true, "empty")]
    [InlineData(true, "partial")]
    [InlineData(true, "record")]
    [InlineData(true, "extra")]
    [InlineData(true, "frame")]
    [InlineData(true, "shorter")]
    [InlineData(true, "hash")]
    public async Task InvalidOracleCacheWithoutPendingMarkerIsRegenerated(
        bool renderer, string damage)
    {
        using var directory = CreateGame("track.rpl");
        var extension = renderer ? "PDO" : "BIN";
        var path = System.IO.Path.Combine(directory.Path, $"TRACK.{extension}");
        var bytes = DumpBytes(renderer, 6);
        bytes = damage switch
        {
            "empty" => [],
            "partial" => bytes[..^1],
            "record" => bytes[..^(renderer ? 36 : 1120)],
            "extra" => [.. bytes, 0],
            "frame" => [(byte)(bytes[0] + 1), .. bytes[1..]],
            "shorter" => DumpBytes(renderer, 0),
            "hash" => [.. bytes[..2], (byte)'g', .. bytes[3..]],
            _ => throw new InvalidOperationException()
        };
        File.WriteAllBytes(path, bytes);
        var calls = new List<string>();
        var runner = new FakeRunner((invocation, _) =>
        {
            calls.Add(invocation.Executable);
            WriteOutput(invocation);
            return Task.FromResult(new DosBoxResult(0));
        });
        var result = await new RegressionEngine(runner, _ => { }).RunAsync(
            Options(directory) with
            { PhysicsTests = !renderer, RendererTests = renderer, PartitionCount = 1 },
            TestContext.Current.CancellationToken);
        Assert.Empty(result.Diagnostics);
        Assert.Equal(renderer ? new[] { "pixldumo.exe", "pixldump.exe" } :
            new[] { "repldumo.exe", "repldump.exe" }, calls);
        Assert.Equal(DumpBytes(renderer, 6), File.ReadAllBytes(path));
        Assert.False(File.Exists(DosFiles.Resolve(directory.Path, $"track.{extension}.pending")));
    }

    [Theory]
    [InlineData(false, false, false)]
    [InlineData(false, false, true)]
    [InlineData(false, true, false)]
    [InlineData(false, true, true)]
    [InlineData(true, false, false)]
    [InlineData(true, false, true)]
    [InlineData(true, true, false)]
    [InlineData(true, true, true)]
    public async Task SuccessfulProcessCannotValidateOrCacheIncompleteOutput(
        bool renderer, bool damageOracle, bool empty)
    {
        using var directory = CreateGame("track.rpl");
        var oracleExtension = renderer ? "PDO" : "BIN";
        var candidateExtension = renderer ? "PDD" : "BNI";
        var damagedExtension = damageOracle ? oracleExtension : candidateExtension;
        var oracle = System.IO.Path.Combine(directory.Path, $"TRACK.{oracleExtension}");
        var runner = new FakeRunner((invocation, _) =>
        {
            WriteOutput(invocation);
            if (invocation.Executable.EndsWith("o.exe", StringComparison.Ordinal) == damageOracle)
            {
                var path = System.IO.Path.Combine(directory.Path, $"TRACK.{damagedExtension}");
                var bytes = File.ReadAllBytes(path);
                File.WriteAllBytes(path, empty ? [] : bytes[..^1]);
            }
            return Task.FromResult(new DosBoxResult(0));
        });
        var options = Options(directory) with
        { PhysicsTests = !renderer, RendererTests = renderer };
        var result = await new RegressionEngine(runner, _ => { }).RunAsync(options,
            TestContext.Current.CancellationToken);
        Assert.True(result.Completed);
        Assert.Single(result.Diagnostics);
        Assert.StartsWith($"ERROR|type=invalid_output|input=track.rpl|" +
            $"output=TRACK.{damagedExtension}|",
            result.Diagnostics[0]);
        Assert.Equal(damageOracle,
            File.Exists(DosFiles.Resolve(directory.Path, $"track.{oracleExtension}.pending")));
        RegressionEngine.Cleanup(result);
        Assert.Equal(!damageOracle, File.Exists(oracle));
        Assert.False(File.Exists(System.IO.Path.Combine(directory.Path,
            $"TRACK.{candidateExtension}")));
        var retry = await new RegressionEngine(new FakeRunner((invocation, _) =>
        {
            WriteOutput(invocation);
            return Task.FromResult(new DosBoxResult(0));
        }), _ => { }).RunAsync(options, TestContext.Current.CancellationToken);
        Assert.Empty(retry.Diagnostics);
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task IdenticalPartialOutputsCannotPassComparison(bool renderer)
    {
        using var directory = CreateGame("track.rpl");
        var calls = 0;
        var result = await new RegressionEngine(new FakeRunner((invocation, _) =>
        {
            calls++;
            WriteOutput(invocation);
            var extension = invocation.Executable switch
            {
                "repldumo.exe" => "BIN",
                "repldump.exe" => "BNI",
                "pixldumo.exe" => "PDO",
                _ => "PDD"
            };
            var path = System.IO.Path.Combine(directory.Path, $"TRACK.{extension}");
            File.WriteAllBytes(path, File.ReadAllBytes(path)[..^1]);
            return Task.FromResult(new DosBoxResult(0));
        }), _ => { }).RunAsync(
            Options(directory) with { PhysicsTests = !renderer, RendererTests = renderer },
            TestContext.Current.CancellationToken);
        Assert.Single(result.Diagnostics);
        Assert.StartsWith("ERROR|type=invalid_output|", result.Diagnostics[0]);
        Assert.Equal(1, calls);
    }

    [Theory]
    [InlineData(0)]
    [InlineData(4)]
    [InlineData(5)]
    [InlineData(6)]
    [InlineData(10)]
    public async Task CompleteOutputsIncludeZeroFrameAndFinalRendererSample(ushort frames)
    {
        using var directory = CreateGame("track.rpl");
        WriteReplay(directory, "track.rpl", frames);
        var result = await new RegressionEngine(new FakeRunner((invocation, _) =>
        {
            WriteOutput(invocation);
            return Task.FromResult(new DosBoxResult(0));
        }), _ => { }).RunAsync(Options(directory), TestContext.Current.CancellationToken);
        Assert.True(result.Completed);
        Assert.Empty(result.Diagnostics);
    }

    private static EngineDirectory CreateGame(params string[] replays)
    {
        var directory = new EngineDirectory();
        foreach (var name in replays.Concat(new[] { "repldumo.exe", "repldump.exe", "pixldumo.exe", "pixldump.exe", "dosbox.conf" }))
        {
            directory.Write(name);
        }
        foreach (var replay in replays)
        {
            WriteReplay(directory, replay, 6);
        }
        return directory;
    }

    private static void WriteReplay(EngineDirectory directory, string replay, ushort frames)
    {
        var bytes = new byte[26 + 1802 + frames];
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(22), 20);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(24), frames);
        File.WriteAllBytes(System.IO.Path.Combine(directory.Path, replay), bytes);
    }

    private static byte[] DumpBytes(bool renderer, ushort frames, bool different = false)
    {
        if (renderer)
        {
            return Encoding.ASCII.GetBytes(string.Concat(Enumerable.Range(0, frames / 5 + 1)
                .Select(index => $"{(index * 5).ToString(CultureInfo.InvariantCulture)} " +
                    new string(different ? '1' : '0', 32) + "\r\n")));
        }
        var bytes = new byte[2 + frames * 1120];
        BinaryPrimitives.WriteUInt16LittleEndian(bytes, frames);
        if (different)
        {
            bytes[^1] = 1;
        }
        return bytes;
    }

    private static RunOptions Options(EngineDirectory directory) => new()
    {
        GameDirectory = directory.Path,
        OutputDirectory = System.IO.Path.Combine(directory.Path, "results"),
        DosBoxConfigPath = System.IO.Path.Combine(directory.Path, "dosbox.conf"),
        PartitionCount = 2
    };

    private static void WriteOutput(DosBoxInvocation invocation, bool different = false)
    {
        var extension = invocation.Executable switch
        {
            "repldumo.exe" => "BIN",
            "repldump.exe" => "BNI",
            "pixldumo.exe" => "PDO",
            "pixldump.exe" => "PDD",
            _ => throw new InvalidOperationException()
        };
        var replay = File.ReadAllBytes(DosFiles.Resolve(invocation.GameDirectory,
            $"{invocation.ReplayBaseName}.rpl"));
        var frames = BinaryPrimitives.ReadUInt16LittleEndian(replay.AsSpan(24));
        File.WriteAllBytes(System.IO.Path.Combine(invocation.GameDirectory,
            $"{invocation.ReplayBaseName.ToUpperInvariant()}.{extension}"),
            DumpBytes(extension is "PDO" or "PDD", frames, different));
    }

    private sealed class PhaseTimeProvider : TimeProvider
    {
        private long timestamp;
        public override long TimestampFrequency => TimeSpan.TicksPerSecond;
        public override long GetTimestamp() => Interlocked.Read(ref timestamp);
        public void Advance(TimeSpan elapsed) => Interlocked.Add(ref timestamp, elapsed.Ticks);
    }

    private sealed class FakeRunner(Func<DosBoxInvocation, CancellationToken, Task<DosBoxResult>> run) : IDosBoxRunner
    {
        public Task<DosBoxResult> RunAsync(DosBoxInvocation invocation, CancellationToken cancellationToken) =>
            run(invocation, cancellationToken);
    }
}
