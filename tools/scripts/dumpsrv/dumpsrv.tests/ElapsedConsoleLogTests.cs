using DumpSrv;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.Logging;

namespace DumpSrv.Tests;

public sealed class ElapsedConsoleLogTests
{
    [Theory]
    [InlineData(0, "00:00:00")]
    [InlineData(61, "00:01:01")]
    [InlineData(3661, "01:01:01")]
    [InlineData(90061, "25:01:01")]
    [InlineData(360061, "100:01:01")]
    public void ElapsedFormatUsesTotalHoursAndWholeSeconds(long seconds, string expected)
    {
        Assert.Equal(expected, ElapsedConsoleLog.FormatElapsed(TimeSpan.FromSeconds(seconds) +
            TimeSpan.FromMilliseconds(999)));
    }

    [Fact]
    public void TimerStartsOnceAndFreezesWhenStopped()
    {
        var clock = new ManualTimeProvider();
        var timer = new ProcessingTimer(clock);
        clock.Advance(TimeSpan.FromHours(3));
        Assert.Equal(TimeSpan.Zero, timer.Elapsed);
        Assert.False(timer.HasStarted);
        Assert.False(timer.IsRunning);
        Assert.False(timer.Stop());

        timer.Start();
        clock.Advance(TimeSpan.FromSeconds(42));
        timer.Start();
        Assert.Equal(TimeSpan.FromSeconds(42), timer.Elapsed);
        Assert.True(timer.HasStarted);
        Assert.True(timer.IsRunning);
        Assert.True(timer.Stop());
        Assert.False(timer.Stop());

        clock.Advance(TimeSpan.FromHours(2));
        timer.Start();
        Assert.Equal(TimeSpan.FromSeconds(42), timer.Elapsed);
        Assert.True(timer.HasStarted);
        Assert.False(timer.IsRunning);
        Assert.False(timer.Stop());
    }

    [Fact]
    public void EveryPhysicalLineGetsOnePrefixIncludingBlankLines()
    {
        using var output = new StringWriter();
        var clock = new ManualTimeProvider();
        var timer = new ProcessingTimer(clock);
        timer.Start();
        clock.Advance(TimeSpan.FromSeconds(62));
        using var log = new ElapsedConsoleLog(new HttpContextAccessor(), output);
        log.Write("first\r\nsecond\n\rthird\rfourth\n", timer);
        log.Write("", timer);
        Assert.Equal(new[]
        {
            "[00:01:02] first",
            "[00:01:02] second",
            "[00:01:02] ",
            "[00:01:02] third",
            "[00:01:02] fourth",
            "[00:01:02] "
        }, Lines(output));
    }

    [Fact]
    public void ProviderPrefixesMultilineMessagesAndExceptionStack()
    {
        using var output = new StringWriter();
        var clock = new ManualTimeProvider();
        var timer = new ProcessingTimer(clock);
        timer.Start();
        clock.Advance(TimeSpan.FromSeconds(7));
        var accessor = Accessor(timer);
        using var provider = new ElapsedConsoleLog(accessor, output);
        var logger = provider.CreateLogger("DumpSrv");
        Exception failure;
        try
        {
            throw new InvalidOperationException("first failure line\r\nsecond failure line");
        }
        catch (InvalidOperationException exception)
        {
            failure = exception;
        }
        logger.LogError(failure, "Processing failed.\nMore details.");
        var lines = Lines(output);
        Assert.Equal("[00:00:07] Error: DumpSrv: Processing failed.", lines[0]);
        Assert.Equal("[00:00:07] More details.", lines[1]);
        Assert.Contains(lines, line => line.Contains(nameof(InvalidOperationException), StringComparison.Ordinal));
        Assert.Contains(lines, line => line.Contains(nameof(ProviderPrefixesMultilineMessagesAndExceptionStack),
            StringComparison.Ordinal));
        Assert.All(lines, line => Assert.StartsWith("[00:00:07] ", line));
        Assert.False(logger.IsEnabled(LogLevel.None));
        provider.Dispose();
        output.WriteLine("writer remains open");
        Assert.EndsWith("writer remains open" + Environment.NewLine, output.ToString());
    }

    [Fact]
    public async Task RequestTimersStayIndependentAcrossAsyncContexts()
    {
        using var output = new StringWriter();
        var clock = new ManualTimeProvider();
        var active = new ProcessingTimer(clock);
        active.Start();
        clock.Advance(TimeSpan.FromSeconds(12));
        var accessor = Accessor(active);
        using var log = new ElapsedConsoleLog(accessor, output);
        await Task.Run(() => log.Write("active worker"), TestContext.Current.CancellationToken);
        Task busyRequest;
        using (ExecutionContext.SuppressFlow())
        {
            busyRequest = Task.Run(() =>
            {
                // A separate incoming request does not inherit the active context.
                var context = new DefaultHttpContext();
                context.Features.Set(new ProcessingTimer(clock));
                accessor.HttpContext = context;
                log.Write("busy request");
                accessor.HttpContext = null;
            }, TestContext.Current.CancellationToken);
        }
        await busyRequest;
        log.Write("active request continues");
        log.Write("explicit active callback", active);
        using var idleLog = new ElapsedConsoleLog(new FixedAccessor(), output);
        idleLog.Write("idle");
        Assert.Equal(new[]
        {
            "[00:00:12] active worker",
            "[00:00:00] busy request",
            "[00:00:12] active request continues",
            "[00:00:12] explicit active callback",
            "[00:00:00] idle"
        }, Lines(output));
    }

    [Fact]
    public async Task ConcurrentWorkersCannotInterleavePhysicalLinesOrMessageBlocks()
    {
        using var output = new StringWriter();
        var clock = new ManualTimeProvider();
        var timer = new ProcessingTimer(clock);
        timer.Start();
        clock.Advance(TimeSpan.FromSeconds(9));
        using var log = new ElapsedConsoleLog(new FixedAccessor(), output);
        await Task.WhenAll(Enumerable.Range(0, 20).Select(index => Task.Run(() =>
            log.Write($"worker {index} first\nworker {index} second", timer),
            TestContext.Current.CancellationToken)));
        var lines = Lines(output);
        Assert.Equal(40, lines.Length);
        Assert.All(lines, line => Assert.StartsWith("[00:00:09] worker ", line));
        for (var index = 0; index < lines.Length; index += 2)
        {
            Assert.Equal(lines[index].Replace(" first", " second", StringComparison.Ordinal), lines[index + 1]);
        }
    }

    private static HttpContextAccessor Accessor(ProcessingTimer timer)
    {
        var context = new DefaultHttpContext();
        context.Features.Set(timer);
        return new HttpContextAccessor { HttpContext = context };
    }

    private static string[] Lines(StringWriter output) => output.ToString()
        .Split(Environment.NewLine, StringSplitOptions.None)[..^1];

    private sealed class FixedAccessor : IHttpContextAccessor
    {
        public HttpContext? HttpContext { get; set; }
    }

    private sealed class ManualTimeProvider : TimeProvider
    {
        private long timestamp;
        public override long TimestampFrequency => TimeSpan.TicksPerSecond;
        public override long GetTimestamp() => Interlocked.Read(ref timestamp);
        public void Advance(TimeSpan duration) => Interlocked.Add(ref timestamp, duration.Ticks);
    }
}
