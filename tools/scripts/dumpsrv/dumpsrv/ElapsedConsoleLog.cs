using System.Globalization;

namespace DumpSrv;

public sealed class ProcessingTimer(TimeProvider? timeProvider = null)
{
    private readonly TimeProvider timeProvider = timeProvider ?? TimeProvider.System;
    private readonly object sync = new();
    private long startedTimestamp;
    private bool hasStarted;
    private bool isRunning;
    private TimeSpan elapsed;

    public bool HasStarted
    {
        get
        {
            lock (sync)
            {
                return hasStarted;
            }
        }
    }

    public bool IsRunning
    {
        get
        {
            lock (sync)
            {
                return isRunning;
            }
        }
    }

    public TimeSpan Elapsed
    {
        get
        {
            lock (sync)
            {
                return isRunning ? timeProvider.GetElapsedTime(startedTimestamp) : elapsed;
            }
        }
    }

    public void Start()
    {
        lock (sync)
        {
            if (hasStarted)
            {
                return;
            }
            startedTimestamp = timeProvider.GetTimestamp();
            hasStarted = true;
            isRunning = true;
        }
    }

    public bool Stop()
    {
        lock (sync)
        {
            if (!isRunning)
            {
                return false;
            }
            elapsed = timeProvider.GetElapsedTime(startedTimestamp);
            isRunning = false;
            return true;
        }
    }
}

public sealed class ElapsedConsoleLog(IHttpContextAccessor accessor, TextWriter? output = null) : ILoggerProvider
{
    private readonly IHttpContextAccessor accessor = accessor ?? throw new ArgumentNullException(nameof(accessor));
    private readonly TextWriter output = output ?? Console.Out;
    private readonly object sync = new();

    public void Write(string message)
    {
        WriteCore(message, accessor.HttpContext?.Features.Get<ProcessingTimer>());
    }

    public void Write(string message, ProcessingTimer timer)
    {
        ArgumentNullException.ThrowIfNull(timer);
        WriteCore(message, timer);
    }

    public static string FormatElapsed(TimeSpan elapsed)
    {
        var seconds = Math.Max(0L, elapsed.Ticks / TimeSpan.TicksPerSecond);
        return string.Create(CultureInfo.InvariantCulture,
            $"{seconds / 3600:00}:{seconds / 60 % 60:00}:{seconds % 60:00}");
    }

    public ILogger CreateLogger(string categoryName) => new Logger(this, categoryName);

    public void Dispose()
    {
        // The console or injected writer belongs to the caller.
    }

    private void WriteCore(string message, ProcessingTimer? timer)
    {
        ArgumentNullException.ThrowIfNull(message);
        lock (sync)
        {
            var prefix = $"[{FormatElapsed(timer?.Elapsed ?? TimeSpan.Zero)}] ";
            using var reader = new StringReader(message);
            var line = reader.ReadLine();
            if (line is null)
            {
                output.WriteLine(prefix);
                return;
            }
            do
            {
                output.WriteLine(prefix + line);
            } while ((line = reader.ReadLine()) is not null);
        }
    }

    private sealed class Logger(ElapsedConsoleLog owner, string category) : ILogger
    {
        public bool IsEnabled(LogLevel logLevel) => logLevel != LogLevel.None;

        public IDisposable? BeginScope<TState>(TState state) where TState : notnull => null;

        public void Log<TState>(LogLevel logLevel, EventId eventId, TState state, Exception? exception,
            Func<TState, Exception?, string> formatter)
        {
            if (!IsEnabled(logLevel))
            {
                return;
            }
            var message = $"{logLevel}: {category}: {formatter(state, exception)}";
            if (exception is not null)
            {
                message += Environment.NewLine + exception;
            }
            owner.Write(message);
        }
    }
}
