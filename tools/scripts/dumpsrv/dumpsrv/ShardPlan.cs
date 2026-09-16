using System.Text.Json;
using System.Text.Json.Serialization;

namespace DumpSrv;

public sealed class ReplayShard
{
    [JsonRequired]
    public List<string> Physics { get; init; } = [];
    [JsonRequired]
    public List<string> Renderer { get; init; } = [];
}

public sealed class ShardPlan
{
    [JsonRequired]
    public int Version { get; init; } = 1;
    [JsonRequired]
    public int RendererTestPercentage { get; init; }
    [JsonRequired]
    public int Target { get; init; }
    [JsonRequired]
    public List<ReplayShard> Shards { get; init; } = [];

    public static ShardPlan Load(string? path, IReadOnlyList<string> replays,
        IReadOnlyList<string> rendererReplays, int percentage, int shardCount, int target = 0)
    {
        ArgumentOutOfRangeException.ThrowIfLessThan(shardCount, 1);
        ArgumentOutOfRangeException.ThrowIfLessThan(target, 0);
        ArgumentOutOfRangeException.ThrowIfGreaterThan(target, 1);
        if (path is null)
        {
            if (shardCount != 1)
            {
                throw new InvalidDataException("ShardPlan is required for a distributed run.");
            }
            return new ShardPlan
            {
                RendererTestPercentage = percentage,
                Target = target,
                Shards = [new ReplayShard
                {
                    Physics = replays.ToList(),
                    Renderer = ReplayCatalog.Sample(rendererReplays, percentage).ToList()
                }]
            };
        }
        using var stream = File.OpenRead(path);
        var plan = JsonSerializer.Deserialize<ShardPlan>(stream,
            new JsonSerializerOptions(JsonSerializerDefaults.Web)
            {
                AllowDuplicateProperties = false
            });
        if (plan is null || plan.Version != 1 || plan.RendererTestPercentage != percentage ||
            plan.Target != target ||
            plan.Shards is null || plan.Shards.Count != shardCount || plan.Shards.Any(shard =>
                shard is null || shard.Physics is null || shard.Renderer is null ||
                shard.Physics.Any(replay => replay is null) ||
                shard.Renderer.Any(replay => replay is null)))
        {
            throw new InvalidDataException("Malformed shard plan or settings do not match.");
        }
        ValidateCoverage(plan.Shards.SelectMany(shard => shard.Physics), replays, "physics");
        ValidateCoverage(plan.Shards.SelectMany(shard => shard.Renderer),
            ReplayCatalog.Sample(rendererReplays, percentage), "renderer");
        return plan;
    }

    public IReadOnlyList<string> Assigned(bool renderer, int shardIndex)
    {
        ArgumentOutOfRangeException.ThrowIfLessThan(shardIndex, 0);
        ArgumentOutOfRangeException.ThrowIfGreaterThanOrEqual(shardIndex, Shards.Count);
        return renderer ? Shards[shardIndex].Renderer : Shards[shardIndex].Physics;
    }

    private static void ValidateCoverage(IEnumerable<string> assigned,
        IReadOnlyList<string> expected, string phase)
    {
        if (!assigned.Order(StringComparer.Ordinal).SequenceEqual(expected, StringComparer.Ordinal))
        {
            throw new InvalidDataException(
                $"Shard plan has missing, duplicate or unexpected {phase} replays.");
        }
    }
}
