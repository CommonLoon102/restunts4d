using System.Text.Json;
using System.Text.Json.Nodes;
using DumpSrv;

namespace DumpSrv.Tests;

public sealed class ShardPlanTests
{
    private static readonly string[] Replays = ["Alpha.rpl", "beta.RPL", "race.rpl", "z.rpl"];

    [Fact]
    public void JsonListsAreUsedVerbatimByShardIdWithoutRedistribution()
    {
        using var directory = new EngineDirectory();
        var path = WritePlan(directory);
        var plan = ShardPlan.Load(path, Replays, 50, 2);
        Assert.Equal(new[] { "z.rpl", "beta.RPL" }, plan.Assigned(false, 0));
        Assert.Equal(new[] { "race.rpl", "Alpha.rpl" }, plan.Assigned(false, 1));
        Assert.Equal(new[] { "race.rpl" }, plan.Assigned(true, 0));
        Assert.Equal(new[] { "Alpha.rpl" }, plan.Assigned(true, 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => plan.Assigned(false, -1));
        Assert.Throws<ArgumentOutOfRangeException>(() => plan.Assigned(true, 2));
    }

    [Fact]
    public void OnlySingleShardRunsMayOmitThePlan()
    {
        var plan = ShardPlan.Load(null, Replays, 50, 1);
        Assert.Equal(Replays, plan.Assigned(false, 0));
        Assert.Equal(new[] { "Alpha.rpl", "race.rpl" }, plan.Assigned(true, 0));
        Assert.Throws<InvalidDataException>(() => ShardPlan.Load(null, Replays, 50, 2));
    }

    [Theory]
    [InlineData("version")]
    [InlineData("percentage")]
    [InlineData("shards")]
    [InlineData("missing_field")]
    [InlineData("null_shard")]
    [InlineData("null_list")]
    [InlineData("null_replay")]
    [InlineData("duplicate_replay")]
    [InlineData("missing_replay")]
    [InlineData("unexpected_replay")]
    [InlineData("wrong_renderer_sample")]
    public void InvalidPlansCannotSilentlyChangeCoverage(string mutation)
    {
        using var directory = new EngineDirectory();
        var path = WritePlan(directory);
        var json = JsonNode.Parse(File.ReadAllText(path))!;
        var shards = json["shards"]!.AsArray();
        switch (mutation)
        {
            case "version": json["version"] = 2; break;
            case "percentage": json["rendererTestPercentage"] = 100; break;
            case "shards": shards.RemoveAt(1); break;
            case "missing_field": shards[0]!.AsObject().Remove("renderer"); break;
            case "null_shard": shards[0] = null; break;
            case "null_list": shards[0]!["physics"] = null; break;
            case "null_replay": shards[0]!["physics"]![0] = null; break;
            case "duplicate_replay": shards[0]!["physics"]!.AsArray().Add("Alpha.rpl"); break;
            case "missing_replay": shards[0]!["physics"]!.AsArray().RemoveAt(0); break;
            case "unexpected_replay": shards[0]!["physics"]![0] = "../other.rpl"; break;
            case "wrong_renderer_sample": shards[0]!["renderer"]![0] = "beta.RPL"; break;
        }
        File.WriteAllText(path, json.ToJsonString());
        var exception = Record.Exception(() => ShardPlan.Load(path, Replays, 50, 2));
        Assert.True(exception is InvalidDataException or JsonException);
    }

    [Fact]
    public void DuplicateJsonPropertiesAndMismatchedSettingsAreRejected()
    {
        using var directory = new EngineDirectory();
        var path = WritePlan(directory);
        Assert.Throws<InvalidDataException>(() => ShardPlan.Load(path, Replays, 50, 3));
        Assert.Throws<InvalidDataException>(() => ShardPlan.Load(path, Replays, 100, 2));
        var json = File.ReadAllText(path);
        File.WriteAllText(path, "{\"version\":1," + json[1..]);
        Assert.Throws<JsonException>(() => ShardPlan.Load(path, Replays, 50, 2));
    }

    private static string WritePlan(EngineDirectory directory) => TestShardPlans.Write(
        directory.Path, 50,
        new ReplayShard { Physics = ["z.rpl", "beta.RPL"], Renderer = ["race.rpl"] },
        new ReplayShard { Physics = ["race.rpl", "Alpha.rpl"], Renderer = ["Alpha.rpl"] });
}

internal static class TestShardPlans
{
    public static string Write(string directory, int percentage, params ReplayShard[] shards)
    {
        var path = Path.Combine(directory, "shard-plan.json");
        var plan = new ShardPlan { RendererTestPercentage = percentage, Shards = shards.ToList() };
        File.WriteAllText(path, JsonSerializer.Serialize(plan,
            new JsonSerializerOptions(JsonSerializerDefaults.Web)));
        return path;
    }
}
