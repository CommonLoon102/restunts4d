using DumpSrv;

namespace DumpSrv.Tests;

public sealed class CommandLineTests
{
    [Theory]
    [InlineData("Camera", "0")]
    [InlineData("Camera", "5")]
    [InlineData("Camera", "1.5")]
    [InlineData("Camera", "F1")]
    [InlineData("Target", "-1")]
    [InlineData("Target", "2")]
    [InlineData("Target", "0.5")]
    [InlineData("Target", "player")]
    public async Task EveryCommandRejectsInvalidView(string option, string value)
    {
        using var directory = new EngineDirectory();
        string[][] commands =
        [
            ["serve", "-ApiKey", "test-secret", "-PartitionCount", "1"],
            ["run", "-GameDirectory", directory.Path, "-OutputDirectory", directory.Path,
                "-PartitionCount", "1"],
            ["extract-oracles", "-GameDirectory", directory.Path, "-Archive", "unused.zip"],
            ["merge", "-ReplayDirectory", directory.Path, "-ResultsDirectory", directory.Path]
        ];
        foreach (var command in commands)
        {
            Assert.Equal(2, await CommandLine.ExecuteAsync([.. command, "-" + option, value],
                TestContext.Current.CancellationToken));
        }
    }

    [Theory]
    [InlineData(0, 0)]
    [InlineData(5, 0)]
    [InlineData(2, -1)]
    [InlineData(2, 2)]
    public void ServiceRejectsInvalidViewAtStartup(int camera, int target)
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => HttpService.Build(new ServiceOptions
        {
            ApiKey = "test-secret",
            PartitionCount = 1,
            Camera = camera,
            Target = target
        }));
    }
}
