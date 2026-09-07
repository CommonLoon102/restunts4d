using System.Globalization;

namespace DumpSrv;

public static class CommandLine
{
    public static async Task<int> ExecuteAsync(string[] args, CancellationToken cancellation = default)
    {
        try
        {
            if (args.Any(a => a is "-h" or "--help" or "-Help"))
            {
                Console.WriteLine(Help);
                return 0;
            }

            var arguments = new Arguments(args);
            switch (arguments.Command)
            {
                case "serve":
                    var service = new ServiceOptions
                    {
                        ApiKey = arguments.String("ApiKey", Environment.GetEnvironmentVariable("DUMPSRV_API_KEY")),
                        PartitionCount = arguments.Number("PartitionCount", null, 1, 64),
                        Port = arguments.Number("Port", 8080, 1, 65535),
                        DosBoxTimeoutSeconds = arguments.Timeout("DosBoxTimeoutSeconds", 60),
                        RendererTestPercentage = arguments.Number("RendererTestPercentage", 100, 1, 100),
                        ResponseProcessingTimeoutSeconds = arguments.Timeout("ResponseProcessingTimeoutSeconds", 1800)
                    };
                    arguments.CheckUnused();
                    await using (var app = HttpService.Build(service))
                    {
                        await app.RunAsync(cancellation);
                    }
                    return 0;
                case "run":
                    var timeout = arguments.Timeout("DosBoxTimeoutSeconds", 60);
                    var run = new RunOptions
                    {
                        GameDirectory = Path.GetFullPath(arguments.String("GameDirectory")),
                        OutputDirectory = Path.GetFullPath(arguments.String("OutputDirectory")),
                        DosBoxConfigPath = Path.GetFullPath(arguments.String("DosBoxConfigPath",
                            Path.Combine(AppContext.BaseDirectory, "dosbox.proc.conf"))),
                        PartitionCount = arguments.Number("PartitionCount", null, 1, 64),
                        ShardIndex = arguments.Number("ShardIndex", 0, 0, int.MaxValue),
                        ShardCount = arguments.Number("ShardCount", 1, 1, int.MaxValue),
                        PhysicsTests = arguments.Boolean("PhysicsTests", true),
                        RendererTests = arguments.Boolean("RendererTests", true),
                        RendererTestPercentage = arguments.Number("RendererTestPercentage", 100, 1, 100),
                        DosBoxTimeoutSeconds = timeout,
                        RendererTimeoutSeconds = arguments.Timeout("RendererTimeoutSeconds", timeout)
                    };
                    arguments.CheckUnused();
                    if (run.ShardIndex >= run.ShardCount)
                    {
                        throw new ArgumentException("ShardIndex must be less than ShardCount.");
                    }
                    RequireTests(run.PhysicsTests, run.RendererTests);
                    var result = await new RegressionEngine(log: Console.WriteLine).RunAsync(run, cancellation);
                    await ResultFiles.WriteAsync(result, run.OutputDirectory, CancellationToken.None);
                    Console.WriteLine(ReportFormatter.RunSummary(result));
                    return result.Completed && result.Failure is null && result.Diagnostics.Count == 0 ? 0 : 1;
                case "merge":
                    var merge = new MergeOptions
                    {
                        ReplayDirectory = Path.GetFullPath(arguments.String("ReplayDirectory")),
                        ResultsDirectory = Path.GetFullPath(arguments.String("ResultsDirectory")),
                        OutputFile = Path.GetFullPath(arguments.String("OutputFile", "partitions_all.txt")),
                        SummaryFile = arguments.Optional("SummaryFile"),
                        ShardCount = arguments.Number("ShardCount", 1, 1, int.MaxValue),
                        PhysicsTests = arguments.Boolean("PhysicsTests", true),
                        RendererTests = arguments.Boolean("RendererTests", true),
                        RendererTestPercentage = arguments.Number("RendererTestPercentage", 100, 1, 100)
                    };
                    arguments.CheckUnused();
                    RequireTests(merge.PhysicsTests, merge.RendererTests);
                    var merged = await ResultMerger.MergeAsync(merge, cancellation);
                    Console.WriteLine(merged.Summary);
                    return merged.Success ? 0 : 1;
                default:
                    throw new ArgumentException("Expected serve, run, or merge.");
            }
        }
        catch (ArgumentException e)
        {
            Console.Error.WriteLine(e.Message);
            Console.Error.WriteLine("Use --help for usage.");
            return 2;
        }
        catch (OperationCanceledException)
        {
            Console.Error.WriteLine("Processing cancelled.");
            return 1;
        }
        catch (Exception e)
        {
            Console.Error.WriteLine(e.Message);
            return 1;
        }
    }

    private static void RequireTests(bool physics, bool renderer)
    {
        if (!physics && !renderer)
        {
            throw new ArgumentException("At least one test type must be enabled.");
        }
    }

    private sealed class Arguments
    {
        private readonly Dictionary<string, string> values = new(StringComparer.OrdinalIgnoreCase);
        public string Command { get; }

        public Arguments(string[] args)
        {
            var offset = args.Length > 0 && !args[0].StartsWith('-') ? 1 : 0;
            Command = offset == 0 ? "serve" : args[0];
            for (var i = offset; i < args.Length; i++)
            {
                if (!args[i].StartsWith('-'))
                {
                    throw new ArgumentException($"Expected a named parameter: {args[i]}");
                }
                var pair = args[i].TrimStart('-').Split('=', 2);
                var value = pair.Length == 2 ? pair[1] : ++i < args.Length ? args[i] :
                    throw new ArgumentException($"Missing value for {pair[0]}.");
                if (!values.TryAdd(pair[0], value))
                {
                    throw new ArgumentException($"Duplicate parameter: {pair[0]}");
                }
            }
        }

        public string? Optional(string name) => values.Remove(name, out var value) ? value : null;

        public string String(string name, string? fallback = null)
        {
            var value = Optional(name) ?? fallback;
            return !string.IsNullOrWhiteSpace(value) ? value :
                throw new ArgumentException($"{name} is required.");
        }

        public int Number(string name, int? fallback, int min, int max)
        {
            var text = Optional(name);
            if (text is null && fallback.HasValue)
            {
                return fallback.Value;
            }
            if (!int.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out var value) ||
                value < min || value > max)
            {
                throw new ArgumentException($"{name} must be an integer from {min} through {max}.");
            }
            return value;
        }

        public int Timeout(string name, int fallback) => Number(name, fallback, 1, 2147483);

        public bool Boolean(string name, bool fallback)
        {
            var value = Optional(name);
            return value is null ? fallback : bool.TryParse(value, out var parsed) ? parsed :
                throw new ArgumentException($"{name} must be true or false.");
        }

        public void CheckUnused()
        {
            if (values.Count != 0)
            {
                throw new ArgumentException($"Unknown parameter: {values.Keys.First()}");
            }
        }
    }

    private const string Help = """
        Usage: dotnet dumpsrv.dll [serve] -PartitionCount N [options]
               dotnet dumpsrv.dll run -GameDirectory DIR -OutputDirectory DIR -PartitionCount N [options]
               dotnet dumpsrv.dll merge -ReplayDirectory DIR -ResultsDirectory DIR [options]

        serve: -ApiKey KEY (or DUMPSRV_API_KEY), -Port 8080,
               -DosBoxTimeoutSeconds 60, -RendererTestPercentage 100,
               -ResponseProcessingTimeoutSeconds 1800 (server startup only).
        run:   -ShardIndex 0, -ShardCount 1, -PhysicsTests true, -RendererTests true,
               -RendererTestPercentage 100, -DosBoxTimeoutSeconds 60,
               -RendererTimeoutSeconds N (defaults to DosBoxTimeoutSeconds), -DosBoxConfigPath FILE.
        merge: -ShardCount 1, -PhysicsTests true, -RendererTests true,
               -RendererTestPercentage 100, -OutputFile partitions_all.txt, -SummaryFile FILE.

        PartitionCount: 1-64. Timeouts: 1-2147483 seconds. Renderer percentage: 1-100.
        DUMPSRV_DOSBOX_PATH overrides DOSBox-X discovery for serve and run.
        """;
}
