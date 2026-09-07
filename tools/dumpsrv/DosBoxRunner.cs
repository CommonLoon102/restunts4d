using System.Diagnostics;

namespace DumpSrv;

public sealed record DosBoxInvocation(string GameDirectory, string ConfigPath, string Executable,
    string ReplayBaseName, string Arguments, int TimeoutSeconds);

public sealed record DosBoxResult(int? ExitCode = null, bool TimedOut = false, string? Failure = null)
{
    public bool Success => ExitCode == 0 && !TimedOut && Failure is null;
}

public interface IDosBoxRunner
{
    Task<DosBoxResult> RunAsync(DosBoxInvocation invocation, CancellationToken cancellationToken);
}

public sealed class DosBoxRunner(string? executablePath = null) : IDosBoxRunner
{
    public async Task<DosBoxResult> RunAsync(DosBoxInvocation invocation, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        using var process = new Process { StartInfo = CreateStartInfo(invocation, executablePath) };
        var started = false;
        Task output = Task.CompletedTask;
        Task error = Task.CompletedTask;
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        timeout.CancelAfter(TimeSpan.FromSeconds(invocation.TimeoutSeconds));
        try
        {
            started = process.Start();
            if (!started)
            {
                return new DosBoxResult(Failure: "DOSBox-X did not start.");
            }
            // Drain without retaining unlimited emulator output in memory.
            output = process.StandardOutput.BaseStream.CopyToAsync(Stream.Null);
            error = process.StandardError.BaseStream.CopyToAsync(Stream.Null);
            await process.WaitForExitAsync(timeout.Token);
            return new DosBoxResult(process.ExitCode);
        }
        catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
        {
            return new DosBoxResult(TimedOut: true);
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception exception) when (exception is not OutOfMemoryException)
        {
            return new DosBoxResult(Failure: exception.Message);
        }
        finally
        {
            if (started && !process.HasExited)
            {
                // SIGKILL avoids DOSBox's shutdown confirmation window.
                try
                {
                    process.Kill(entireProcessTree: true);
                }
                catch (InvalidOperationException) when (process.HasExited)
                {
                }
                catch (System.ComponentModel.Win32Exception) when (!process.HasExited)
                {
                    // Still terminate DOSBox if child-tree enumeration failed.
                    process.Kill();
                }
                await process.WaitForExitAsync(CancellationToken.None);
            }
            await Task.WhenAll(output, error);
        }
    }

    public static ProcessStartInfo CreateStartInfo(DosBoxInvocation invocation, string? executablePath = null)
    {
        ReplayCatalog.ValidateBaseName(invocation.ReplayBaseName);
        var gameDirectory = Path.GetFullPath(invocation.GameDirectory);
        if (gameDirectory.IndexOfAny(['"', '\r', '\n', '%']) >= 0)
        {
            throw new InvalidDataException("The game directory contains unsupported DOS command characters.");
        }
        var configuredPath = executablePath ?? Environment.GetEnvironmentVariable("DUMPSRV_DOSBOX_PATH");
        if (string.IsNullOrWhiteSpace(configuredPath))
        {
            const string windowsPath = @"C:\DOSBox-x\dosbox-X.exe";
            configuredPath = OperatingSystem.IsWindows() && File.Exists(windowsPath) ? windowsPath : "dosbox-x";
        }
        var startInfo = new ProcessStartInfo
        {
            FileName = configuredPath,
            WorkingDirectory = gameDirectory,
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };
        string[] arguments =
        [
            "-silent", "-conf", Path.GetFullPath(invocation.ConfigPath),
            "-noautoexec", "-set", "cpu core=dynamic", "-set", "cpu cycles=max",
            "-c", $"mount c \"{gameDirectory}\"", "-c", "c:",
            "-c", $"{invocation.Executable} \"{invocation.ReplayBaseName}\" {invocation.Arguments}",
            "-c", "exit"
        ];
        foreach (var argument in arguments)
        {
            startInfo.ArgumentList.Add(argument);
        }
        return startInfo;
    }
}
