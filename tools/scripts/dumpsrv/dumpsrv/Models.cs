using System.Text.Json.Serialization;

namespace DumpSrv;

public sealed record RunOptions
{
    public required string GameDirectory { get; init; }
    public required string OutputDirectory { get; init; }
    public string DosBoxConfigPath { get; init; } = Path.Combine(AppContext.BaseDirectory, "dosbox.proc.conf");
    public int PartitionCount { get; init; } = 1;
    public int ShardIndex { get; init; }
    public int ShardCount { get; init; } = 1;
    public bool PhysicsTests { get; init; } = true;
    public bool RendererTests { get; init; } = true;
    public int RendererTestPercentage { get; init; } = 100;
    public int DosBoxTimeoutSeconds { get; init; } = 60;
    public int RendererTimeoutSeconds { get; init; } = 60;
}

public sealed record ServiceOptions
{
    public required string ApiKey { get; init; }
    public required int PartitionCount { get; init; }
    public int Port { get; init; } = 8080;
    public int DosBoxTimeoutSeconds { get; init; } = 60;
    public int RendererTestPercentage { get; init; } = 100;
    public int ResponseProcessingTimeoutSeconds { get; init; } = 1800;
    public string ServiceDirectory { get; init; } = AppContext.BaseDirectory;
}

public sealed class ShardResult
{
    [JsonRequired]
    public int Version { get; set; } = 1;
    [JsonRequired]
    public int ShardIndex { get; set; }
    [JsonRequired]
    public int ShardCount { get; set; }
    [JsonRequired]
    public int PartitionCount { get; set; }
    [JsonRequired]
    public bool PhysicsTests { get; set; }
    [JsonRequired]
    public bool RendererTests { get; set; }
    [JsonRequired]
    public int RendererTestPercentage { get; set; }
    [JsonRequired]
    public int DosBoxTimeoutSeconds { get; set; } = 60;
    [JsonRequired]
    public int RendererTimeoutSeconds { get; set; } = 60;
    [JsonRequired]
    public List<string> ReplayFiles { get; set; } = [];
    [JsonRequired]
    public List<string> PhysicsCompleted { get; set; } = [];
    [JsonRequired]
    public List<string> RendererCompleted { get; set; } = [];
    [JsonRequired]
    public List<string> Diagnostics { get; set; } = [];
    [JsonRequired]
    public bool Completed { get; set; }
    [JsonRequired]
    public string? Failure { get; set; }
    [JsonIgnore]
    public TimeSpan? PhysicsElapsed { get; set; }
    [JsonIgnore]
    public TimeSpan? RendererElapsed { get; set; }
    [JsonIgnore]
    public List<string> OwnedFiles { get; set; } = [];
}
