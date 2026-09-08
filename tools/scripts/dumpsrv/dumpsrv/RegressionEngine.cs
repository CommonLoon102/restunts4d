namespace DumpSrv;

public class RegressionEngine(IDosBoxRunner? runner = null, Action<string>? log = null,
    TimeProvider? timeProvider = null)
{
    private readonly IDosBoxRunner runner = runner ?? new DosBoxRunner();
    private readonly Action<string> log = log ?? Console.WriteLine;

    private sealed record Phase(bool Renderer, string Oracle, string Candidate, string OracleExtension,
        string CandidateExtension, string Arguments, int TimeoutSeconds);

    public virtual async Task<ShardResult> RunAsync(RunOptions options, CancellationToken cancellationToken)
    {
        var result = new ShardResult
        {
            ShardIndex = options.ShardIndex,
            ShardCount = options.ShardCount,
            PartitionCount = options.PartitionCount,
            PhysicsTests = options.PhysicsTests,
            RendererTests = options.RendererTests,
            RendererTestPercentage = options.RendererTestPercentage,
            DosBoxTimeoutSeconds = options.DosBoxTimeoutSeconds,
            RendererTimeoutSeconds = options.RendererTimeoutSeconds
        };
        var gate = new object();
        void Diagnostic(string message)
        {
            lock (gate)
            {
                result.Diagnostics.Add(message);
            }
            log(message);
        }
        void Own(string path)
        {
            lock (gate)
            {
                result.OwnedFiles.Add(path);
            }
        }
        try
        {
            Validate(options);
            var replays = ReplayCatalog.Discover(options.GameDirectory, cancellationToken);
            result.ReplayFiles = replays.ToList();
            var phases = new List<Phase>();
            if (options.PhysicsTests)
            {
                phases.Add(new Phase(false, "repldumo.exe", "repldump.exe", "BIN", "BNI", "1",
                    options.DosBoxTimeoutSeconds));
            }
            if (options.RendererTests)
            {
                phases.Add(new Phase(true, "pixldumo.exe", "pixldump.exe", "PDO", "PDD", "2 0",
                    options.RendererTimeoutSeconds));
            }
            foreach (var executable in phases.SelectMany(phase => new[] { phase.Oracle, phase.Candidate }))
            {
                if (!File.Exists(DosFiles.Resolve(options.GameDirectory, executable)))
                {
                    throw new FileNotFoundException($"DOS executable not found: {executable}");
                }
            }
            Directory.CreateDirectory(options.OutputDirectory);
            foreach (var phase in phases)
            {
                cancellationToken.ThrowIfCancellationRequested();
                var timer = new ProcessingTimer(timeProvider);
                timer.Start();
                try
                {
                    var assigned = ReplayCatalog.Assigned(replays, phase.Renderer, options.RendererTestPercentage,
                        options.ShardIndex, options.ShardCount);
                    var partitions = ReplayCatalog.RoundRobin(assigned, options.PartitionCount);
                    await Task.WhenAll(partitions.Where(partition => partition.Count > 0).Select(async partition =>
                    {
                        foreach (var replay in partition)
                        {
                            cancellationToken.ThrowIfCancellationRequested();
                            log($"Processing {(phase.Renderer ? "renderer" : "physics")} replay: {replay}");
                            try
                            {
                                await ProcessReplayAsync(options, replay, phase, Diagnostic, Own, cancellationToken);
                            }
                            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
                            {
                                throw;
                            }
                            catch (Exception exception) when (exception is not OutOfMemoryException)
                            {
                                Diagnostic($"ERROR|type=processing_failure|input={replay}|message={ReportFormatter.Safe(exception.Message)}");
                            }
                            lock (gate)
                            {
                                (phase.Renderer ? result.RendererCompleted : result.PhysicsCompleted).Add(replay);
                            }
                        }
                    }));
                }
                finally
                {
                    timer.Stop();
                    if (phase.Renderer)
                    {
                        result.RendererElapsed = timer.Elapsed;
                    }
                    else
                    {
                        result.PhysicsElapsed = timer.Elapsed;
                    }
                }
            }
            result.Completed = true;
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            result.Failure = "Regression processing was cancelled.";
            Diagnostic("ERROR|type=cancelled|message=Regression processing was cancelled.");
        }
        catch (Exception exception) when (exception is not OutOfMemoryException)
        {
            result.Failure = exception.Message;
            Diagnostic($"ERROR|type=processing_failure|message={ReportFormatter.Safe(exception.Message)}");
        }
        result.PhysicsCompleted.Sort(StringComparer.Ordinal);
        result.RendererCompleted.Sort(StringComparer.Ordinal);
        result.Diagnostics = ReportFormatter.Lines(result.Diagnostics).ToList();
        return result;
    }

    private async Task ProcessReplayAsync(RunOptions options, string replay, Phase phase,
        Action<string> diagnostic, Action<string> own, CancellationToken cancellationToken)
    {
        var basename = Path.GetFileNameWithoutExtension(replay);
        string Resolve(string extension) => DosFiles.Resolve(options.GameDirectory, $"{basename}.{extension}");
        var candidatePath = Resolve(phase.CandidateExtension);
        own(candidatePath);
        File.Delete(candidatePath);
        var oraclePath = Resolve(phase.OracleExtension);
        var pendingPath = Resolve($"{phase.OracleExtension}.pending");
        if (File.Exists(pendingPath) || !File.Exists(oraclePath))
        {
            own(pendingPath);
            await File.WriteAllTextAsync(pendingPath, "", cancellationToken);
            File.Delete(oraclePath);
            if (!await ExecuteAsync(phase.Oracle))
            {
                return;
            }
            oraclePath = Resolve(phase.OracleExtension);
            if (File.Exists(oraclePath))
            {
                File.Delete(pendingPath);
            }
        }
        if (!await ExecuteAsync(phase.Candidate))
        {
            return;
        }
        // DOS may create upper-case paths even for lower-case host replay names.
        candidatePath = Resolve(phase.CandidateExtension);
        own(candidatePath);
        var oracleExists = File.Exists(oraclePath);
        var candidateExists = File.Exists(candidatePath);
        if (!oracleExists)
        {
            diagnostic($"ERROR|type=missing_output|input={replay}|output={Path.GetFileName(oraclePath)}");
        }
        if (!candidateExists)
        {
            diagnostic($"ERROR|type=missing_output|input={replay}|output={Path.GetFileName(candidatePath)}");
        }
        if (oracleExists && candidateExists && !await FilesEqualAsync(oraclePath, candidatePath, cancellationToken))
        {
            diagnostic($"ERROR|type=file_mismatch|input={replay}|" +
                $"{phase.OracleExtension.ToLowerInvariant()}={Path.GetFileName(oraclePath)}|" +
                $"{phase.CandidateExtension.ToLowerInvariant()}={Path.GetFileName(candidatePath)}");
        }
        return;

        async Task<bool> ExecuteAsync(string executable)
        {
            var outcome = await runner.RunAsync(new DosBoxInvocation(options.GameDirectory,
                options.DosBoxConfigPath, executable, basename, phase.Arguments, phase.TimeoutSeconds), cancellationToken);
            if (outcome.TimedOut)
            {
                diagnostic($"ERROR|type=timeout|exe={executable}|input={replay}|timeout_seconds={phase.TimeoutSeconds}");
            }
            else if (!outcome.Success)
            {
                var detail = outcome.ExitCode.HasValue ? $"exit_code={outcome.ExitCode}" :
                    $"message={ReportFormatter.Safe(outcome.Failure ?? "DOSBox-X failed.")}";
                diagnostic($"ERROR|type=dosbox_failure|exe={executable}|input={replay}|{detail}");
            }
            // Track partially written candidates too, including different DOS host casing.
            own(Resolve(phase.CandidateExtension));
            return outcome.Success;
        }
    }

    public static void Cleanup(ShardResult result)
    {
        foreach (var path in result.OwnedFiles.Distinct(StringComparer.Ordinal))
        {
            if (path.EndsWith(".pending", StringComparison.OrdinalIgnoreCase) && File.Exists(path))
            {
                var incompletePath = DosFiles.Resolve(Path.GetDirectoryName(path)!,
                    Path.GetFileName(path)[..^".pending".Length]);
                File.Delete(incompletePath);
            }
            File.Delete(path);
        }
    }

    private static async Task<bool> FilesEqualAsync(string left, string right, CancellationToken cancellationToken)
    {
        await using var leftStream = File.OpenRead(left);
        await using var rightStream = File.OpenRead(right);
        if (leftStream.Length != rightStream.Length)
        {
            return false;
        }
        var leftBuffer = new byte[65536];
        var rightBuffer = new byte[65536];
        while (true)
        {
            var leftCount = await leftStream.ReadAtLeastAsync(leftBuffer, leftBuffer.Length, false, cancellationToken);
            var rightCount = await rightStream.ReadAtLeastAsync(rightBuffer, rightBuffer.Length, false, cancellationToken);
            if (leftCount != rightCount || !leftBuffer.AsSpan(0, leftCount).SequenceEqual(rightBuffer.AsSpan(0, rightCount)))
            {
                return false;
            }
            if (leftCount == 0)
            {
                return true;
            }
        }
    }

    private static void Validate(RunOptions options)
    {
        ArgumentOutOfRangeException.ThrowIfLessThan(options.PartitionCount, 1);
        ArgumentOutOfRangeException.ThrowIfGreaterThan(options.PartitionCount, 64);
        ArgumentOutOfRangeException.ThrowIfLessThan(options.ShardCount, 1);
        ArgumentOutOfRangeException.ThrowIfLessThan(options.ShardIndex, 0);
        ArgumentOutOfRangeException.ThrowIfGreaterThanOrEqual(options.ShardIndex, options.ShardCount);
        ArgumentOutOfRangeException.ThrowIfLessThan(options.RendererTestPercentage, 1);
        ArgumentOutOfRangeException.ThrowIfGreaterThan(options.RendererTestPercentage, 100);
        ArgumentOutOfRangeException.ThrowIfLessThan(options.DosBoxTimeoutSeconds, 1);
        ArgumentOutOfRangeException.ThrowIfLessThan(options.RendererTimeoutSeconds, 1);
        ArgumentOutOfRangeException.ThrowIfGreaterThan(options.DosBoxTimeoutSeconds, 2147483);
        ArgumentOutOfRangeException.ThrowIfGreaterThan(options.RendererTimeoutSeconds, 2147483);
        if (!options.PhysicsTests && !options.RendererTests)
        {
            throw new ArgumentException("At least one test phase must be enabled.");
        }
        if (!File.Exists(options.DosBoxConfigPath))
        {
            throw new FileNotFoundException("DOSBox configuration not found.", options.DosBoxConfigPath);
        }
    }

}
