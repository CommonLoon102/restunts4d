using System.Net;
using System.Net.Http.Headers;
using System.Net.Sockets;
using System.Text;
using DumpSrv;
using Microsoft.AspNetCore.Builder;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;

namespace DumpSrv.Tests;

public sealed class HttpServiceTests
{
    [Theory]
    [InlineData(true, true, "physics and renderer")]
    [InlineData(true, false, "physics")]
    [InlineData(false, true, "renderer")]
    public async Task ElapsedLogsIncludeRequestedPhaseTotalBeforeResponseHeaders(
        bool physics, bool renderer, string phases)
    {
        using var output = new CapturedLog();
        var clock = new RequestTimeProvider();
        var engine = new FakeEngine();
        await using var server = await Server.StartAsync(engine, logOutput: output, timeProvider: clock);
        var logger = server.App.Services.GetRequiredService<ILoggerFactory>().CreateLogger("Worker");
        engine.Handler = async (_, token) =>
        {
            clock.Advance(TimeSpan.FromSeconds(3723));
            await Task.Run(() => logger.LogInformation("Worker progress\r\nSecond line"), token);
            return new ShardResult
            {
                Completed = true,
                PhysicsElapsed = physics ? TimeSpan.FromSeconds(1220) : null,
                RendererElapsed = renderer ? TimeSpan.FromSeconds(2500) : null,
                Diagnostics = ["ERROR|type=file_mismatch|input=race.rpl"]
            };
        };
        var form = Form();
        form.Add(new StringContent(physics.ToString()), "physics_tests");
        form.Add(new StringContent(renderer.ToString()), "renderer_tests");
        using var request = Request(form);
        using var response = await server.Client.SendAsync(request, HttpCompletionOption.ResponseHeadersRead,
            TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.OK, response.StatusCode);
        var lines = output.ToString().Split(Environment.NewLine, StringSplitOptions.RemoveEmptyEntries);
        Assert.All(lines, line => Assert.Matches(@"^\[\d{2,}:\d{2}:\d{2}\] ", line));
        Assert.Contains($"[00:00:00] Processing requested phases: {phases}.", lines);
        Assert.Contains("[01:02:03] Second line", lines);
        Assert.Equal(physics ? 1 : 0, lines.Count(line =>
            line == "[01:02:03] Physics phase took 00:20:20."));
        Assert.Equal(renderer ? 1 : 0, lines.Count(line =>
            line == "[01:02:03] Renderer phase took 00:41:40."));
        Assert.DoesNotContain(lines, line => line.Contains("phase did not start.", StringComparison.Ordinal));
        Assert.Single(lines, line => line ==
            $"[01:02:03] Processing requested phases ({phases}) took 01:02:03.");
        Assert.Equal("ERROR|type=file_mismatch|input=race.rpl\n",
            await response.Content.ReadAsStringAsync(TestContext.Current.CancellationToken));
    }

    [Fact]
    public async Task FailedProcessingReportsStartedAndUnstartedPhasesBeforeResponse()
    {
        using var output = new CapturedLog();
        var clock = new RequestTimeProvider();
        var engine = new FakeEngine
        {
            Handler = (_, _) =>
            {
                clock.Advance(TimeSpan.FromSeconds(17));
                return Task.FromResult(new ShardResult
                {
                    Failure = "cancelled",
                    PhysicsElapsed = TimeSpan.FromSeconds(17)
                });
            }
        };
        await using var server = await Server.StartAsync(engine, logOutput: output, timeProvider: clock);
        using var request = Request(Form());
        using var response = await server.Client.SendAsync(request, HttpCompletionOption.ResponseHeadersRead,
            TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.InternalServerError, response.StatusCode);
        var lines = output.ToString().Split(Environment.NewLine, StringSplitOptions.RemoveEmptyEntries);
        Assert.Contains("[00:00:17] Physics phase took 00:00:17.", lines);
        Assert.Contains("[00:00:17] Renderer phase did not start.", lines);
    }

    [Fact]
    public async Task SequentialRequestsEachStartTheirOwnElapsedClock()
    {
        using var output = new CapturedLog();
        var clock = new RequestTimeProvider();
        var engine = new FakeEngine();
        await using var server = await Server.StartAsync(engine, logOutput: output, timeProvider: clock);
        engine.Handler = (_, _) =>
        {
            clock.Advance(TimeSpan.FromSeconds(engine.Calls == 1 ? 37 : 5));
            return Task.FromResult(new ShardResult { Completed = true });
        };
        for (var index = 0; index < 2; index++)
        {
            using var request = Request(Form());
            using var response = await server.Client.SendAsync(request, TestContext.Current.CancellationToken);
            Assert.Equal(HttpStatusCode.OK, response.StatusCode);
        }
        var lines = output.ToString().Split(Environment.NewLine, StringSplitOptions.RemoveEmptyEntries);
        Assert.Equal(2, lines.Count(line =>
            line == "[00:00:00] Processing requested phases: physics and renderer."));
        Assert.Contains("[00:00:37] Processing requested phases (physics and renderer) took 00:00:37.", lines);
        Assert.Contains("[00:00:05] Processing requested phases (physics and renderer) took 00:00:05.", lines);
    }

    [Fact]
    public async Task AmbientAppSettingsCannotAddEndpointsOrOverrideExplicitOptions()
    {
        await using var server = await Server.StartAsync(appSettings: """
            {
                "Kestrel": {
                    "Endpoints": {
                        "Conflicting": { "Url": "http://localhost:0" }
                    }
                },
                "urls": "http://localhost:0",
                "ApiKey": "wrong"
            }
            """);
        Assert.Equal("127.0.0.1", server.Client.BaseAddress!.Host);
        using var request = Request(Form());
        using var response = await server.Client.SendAsync(request, TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.OK, response.StatusCode);
    }

    [Theory]
    [InlineData("/other", "POST", HttpStatusCode.NotFound)]
    [InlineData("/Process", "POST", HttpStatusCode.NotFound)]
    [InlineData("/process/", "POST", HttpStatusCode.NotFound)]
    [InlineData("/process", "GET", HttpStatusCode.MethodNotAllowed)]
    [InlineData("/process", "POST", HttpStatusCode.Unauthorized)]
    public async Task ChecksPathThenMethodThenAuthentication(string path, string method,
        HttpStatusCode expected)
    {
        await using var server = await Server.StartAsync();
        using var request = new HttpRequestMessage(new HttpMethod(method), path);
        using var response = await server.Client.SendAsync(request, TestContext.Current.CancellationToken);
        Assert.Equal(expected, response.StatusCode);
        if (expected == HttpStatusCode.MethodNotAllowed)
        {
            Assert.Contains("POST", response.Content.Headers.Allow);
        }
    }

    [Fact]
    public async Task RejectsEscapedPathEvenWhenItDecodesToProcess()
    {
        await using var server = await Server.StartAsync();
        using var client = new TcpClient();
        await client.ConnectAsync("127.0.0.1", server.Client.BaseAddress!.Port,
            TestContext.Current.CancellationToken);
        await using var stream = client.GetStream();
        await stream.WriteAsync(Encoding.ASCII.GetBytes(
            "POST /%70rocess HTTP/1.1\r\nHost: localhost\r\nX-API-Key: secret\r\n" +
            "Content-Length: 0\r\nConnection: close\r\n\r\n"), TestContext.Current.CancellationToken);
        using var reader = new StreamReader(stream);
        var status = await reader.ReadLineAsync(TestContext.Current.CancellationToken);
        Assert.StartsWith("HTTP/1.1 404", status);
    }

    [Theory]
    [InlineData("application/json", "{}")]
    [InlineData("multipart/form-data", "missing boundary")]
    [InlineData("multipart/form-data; boundary=test", "--test\r\nContent-Disposition: form-data; name=\"repldump\"\r\n--test--\r\n")]
    [InlineData("multipart/form-data; boundary=test", "--test--\r\ntrailing data")]
    public async Task MalformedMultipartProducesBadRequest(string contentType, string body)
    {
        await using var server = await Server.StartAsync();
        var content = new ByteArrayContent(Encoding.UTF8.GetBytes(body));
        content.Headers.ContentType = MediaTypeHeaderValue.Parse(contentType);
        using var request = Request(content);
        using var response = await server.Client.SendAsync(request, TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.BadRequest, response.StatusCode);
    }

    [Theory]
    [InlineData("secretx")]
    [InlineData("SECRET")]
    [InlineData("wrong")]
    public async Task RejectsIncorrectApiKeys(string key)
    {
        await using var server = await Server.StartAsync();
        using var request = Request(Form());
        request.Headers.Remove("X-API-Key");
        request.Headers.TryAddWithoutValidation("X-API-Key", key);
        using var response = await server.Client.SendAsync(request, TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.Unauthorized, response.StatusCode);
    }

    [Theory]
    [InlineData("missing")]
    [InlineData("empty")]
    [InlineData("duplicate")]
    [InlineData("unknown")]
    [InlineData("timeout")]
    [InlineData("invalid_boolean")]
    [InlineData("both_disabled")]
    public async Task RejectsInvalidMultipartWithoutRunningEngine(string kind)
    {
        var engine = new FakeEngine();
        await using var server = await Server.StartAsync(engine);
        var form = kind == "missing" ? new MultipartFormDataContent() : Form(
            kind == "empty" ? [] : [1, 2, 3]);
        switch (kind)
        {
            case "duplicate":
                form.Add(new StringContent("duplicate"), "REPLDUMP");
                break;
            case "unknown":
                form.Add(new StringContent("true"), "unexpected");
                break;
            case "timeout":
                form.Add(new StringContent("60"), "ResponseProcessingTimeoutSeconds");
                break;
            case "invalid_boolean":
                form.Add(new StringContent("yes"), "physics_tests");
                break;
            case "both_disabled":
                form.Add(new StringContent("false"), "physics_tests");
                form.Add(new StringContent("false"), "renderer_tests");
                break;
        }
        using var request = Request(form);
        using var response = await server.Client.SendAsync(request, TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.BadRequest, response.StatusCode);
        Assert.Equal(0, engine.Calls);
        Assert.False(File.Exists(Path.Combine(server.Directory, "stunts", "REPLDUMP.EXE")));
    }

    [Theory]
    [InlineData("repldump", "other.exe", null)]
    [InlineData("pixldump", "other.exe", null)]
    [InlineData("repldump", "pixldump.exe", null)]
    [InlineData("pixldump", "repldump.exe", null)]
    [InlineData("pixldump", "pixeldump.exe", null)]
    [InlineData("repldump", "../../repldump.exe", null)]
    [InlineData("pixldump", @"C:\uploads\pixldump.exe", null)]
    [InlineData("repldump", "repldump.exe.bak", null)]
    [InlineData("pixldump", "pixldump.exe ", null)]
    [InlineData("repldump", null, null)]
    [InlineData("pixldump", null, null)]
    [InlineData("repldump", "\"\"", null)]
    [InlineData("pixldump", null, "")]
    [InlineData("repldump", null, "other.exe")]
    [InlineData("pixldump", null, "../pixldump.exe")]
    [InlineData("repldump", "repldump.exe", "other.exe")]
    [InlineData("pixldump", "other.exe", "pixldump.exe")]
    [InlineData("repldump", "repldump.exe", "")]
    [InlineData("repldump", "REPLDUMP.EXE", null)]
    [InlineData("pixldump", "PIXLDUMP.EXE", null)]
    [InlineData("repldump", "Repldump.exe", null)]
    [InlineData("pixldump", "pixldump.EXE", null)]
    [InlineData("repldump", null, "REPLDUMP.EXE")]
    [InlineData("pixldump", null, "Pixldump.exe")]
    [InlineData("repldump", "repldump.exe", "REPLDUMP.EXE")]
    [InlineData("pixldump", "PIXLDUMP.EXE", "pixldump.exe")]
    public async Task RejectsInvalidUploadFilenamesBeforeSavingEitherExecutable(
        string field, string? fileName, string? fileNameStar)
    {
        var engine = new FakeEngine();
        await using var server = await Server.StartAsync(engine);
        var gameDirectory = Path.Combine(server.Directory, "stunts");
        var physicsPath = Path.Combine(gameDirectory, "REPLDUMP.EXE");
        var rendererPath = Path.Combine(gameDirectory, "PIXLDUMP.EXE");
        await File.WriteAllBytesAsync(physicsPath, new byte[] { 98 },
            TestContext.Current.CancellationToken);
        await File.WriteAllBytesAsync(rendererPath, new byte[] { 99 },
            TestContext.Current.CancellationToken);
        var form = Form();
        var disposition = form.ElementAt(field == "repldump" ? 0 : 1).Headers.ContentDisposition!;
        disposition.FileName = fileName;
        disposition.FileNameStar = fileNameStar;
        if (fileNameStar == "")
        {
            disposition.Parameters.Add(new NameValueHeaderValue("filename*", "UTF-8''"));
        }
        using var request = Request(form);
        using var response = await server.Client.SendAsync(request,
            TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.BadRequest, response.StatusCode);
        Assert.Contains($"{field} must have filename {field}.exe",
            await response.Content.ReadAsStringAsync(TestContext.Current.CancellationToken));
        Assert.Equal(0, engine.Calls);
        Assert.Equal(new byte[] { 98 }, await File.ReadAllBytesAsync(physicsPath,
            TestContext.Current.CancellationToken));
        Assert.Equal(new byte[] { 99 }, await File.ReadAllBytesAsync(rendererPath,
            TestContext.Current.CancellationToken));
    }

    [Theory]
    [InlineData("filename", false)]
    [InlineData("filename*", false)]
    [InlineData("both", false)]
    [InlineData("filename", true)]
    [InlineData("filename*", true)]
    [InlineData("both", true)]
    public async Task AcceptsExpectedUploadFilenames(string header, bool upperCaseField)
    {
        var engine = new FakeEngine();
        await using var server = await Server.StartAsync(engine);
        var form = Form();
        foreach (var part in form)
        {
            var disposition = part.Headers.ContentDisposition!;
            if (upperCaseField)
            {
                disposition.Name = disposition.Name!.ToUpperInvariant();
            }
            if (header == "filename")
            {
                disposition.FileNameStar = null;
            }
            else if (header == "filename*")
            {
                disposition.FileName = null;
            }
        }
        using var request = Request(form);
        using var response = await server.Client.SendAsync(request,
            TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.OK, response.StatusCode);
        Assert.Equal(1, engine.Calls);
        var gameDirectory = Path.Combine(server.Directory, "stunts");
        Assert.Equal(new byte[] { 1, 2, 3 }, await File.ReadAllBytesAsync(
            Path.Combine(gameDirectory, "REPLDUMP.EXE"), TestContext.Current.CancellationToken));
        Assert.Equal(new byte[] { 4, 5, 6 }, await File.ReadAllBytesAsync(
            Path.Combine(gameDirectory, "PIXLDUMP.EXE"), TestContext.Current.CancellationToken));
    }

    [Theory]
    [InlineData("filename=repldump.exe; filename=other.exe")]
    [InlineData("filename*=UTF-8''repldump.exe; FILENAME*=UTF-8''other.exe")]
    [InlineData("filename=repldump.exe; filename*=invalid")]
    public async Task RejectsAmbiguousOrMalformedFilenameHeaders(string parameters)
    {
        var engine = new FakeEngine();
        await using var server = await Server.StartAsync(engine);
        var form = Form();
        var headers = form.First().Headers;
        headers.Remove("Content-Disposition");
        headers.TryAddWithoutValidation("Content-Disposition",
            $"form-data; name=repldump; {parameters}");
        using var request = Request(form);
        using var response = await server.Client.SendAsync(request,
            TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.BadRequest, response.StatusCode);
        Assert.Equal(0, engine.Calls);
        Assert.Empty(System.IO.Directory.EnumerateFiles(Path.Combine(server.Directory, "stunts")));
        using var next = Request(Form());
        using var nextResponse = await server.Client.SendAsync(next,
            TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.OK, nextResponse.StatusCode);
    }

    [Theory]
    [InlineData("physics_tests")]
    [InlineData("renderer_tests")]
    public async Task RejectsFileUploadsInBooleanFields(string field)
    {
        var engine = new FakeEngine();
        await using var server = await Server.StartAsync(engine);
        var form = Form();
        form.Add(new StringContent("true"), field, "other.exe");
        using var request = Request(form);
        using var response = await server.Client.SendAsync(request,
            TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.BadRequest, response.StatusCode);
        Assert.Equal(0, engine.Calls);
        Assert.Empty(System.IO.Directory.EnumerateFiles(Path.Combine(server.Directory, "stunts")));
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task EnforcesTotalUploadLimitWithAndWithoutContentLength(bool chunked)
    {
        await using var server = await Server.StartAsync();
        var bytes = new byte[2 * 1024 * 1024 + 64 * 1024 + 1];
        HttpContent content = chunked ? new ChunkedContent(bytes) : new ByteArrayContent(bytes);
        content.Headers.ContentType = MediaTypeHeaderValue.Parse("multipart/form-data; boundary=test");
        using var request = Request(content);
        using var response = await server.Client.SendAsync(request, TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.RequestEntityTooLarge, response.StatusCode);
    }

    [Fact]
    public async Task EnforcesIndividualExecutableLimit()
    {
        await using var server = await Server.StartAsync();
        using var request = Request(Form(new byte[1024 * 1024 + 1]));
        using var response = await server.Client.SendAsync(request, TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.RequestEntityTooLarge, response.StatusCode);
    }

    [Fact]
    public async Task AcceptsMaximumExecutablesAndOptionalSettings()
    {
        var engine = new FakeEngine();
        await using var server = await Server.StartAsync(engine);
        var form = Form(new byte[1024 * 1024], new byte[1024 * 1024]);
        form.Add(new StringContent(" FALSE "), "PHYSICS_TESTS");
        form.Add(new StringContent(" TrUe "), "renderer_tests");
        using var request = Request(form, "/process?ignored=true");
        using var response = await server.Client.SendAsync(request, TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.OK, response.StatusCode);
        Assert.False(engine.LastOptions!.PhysicsTests);
        Assert.True(engine.LastOptions.RendererTests);
        Assert.Equal(7, engine.LastOptions.PartitionCount);
        Assert.Equal(19, engine.LastOptions.DosBoxTimeoutSeconds);
        Assert.Equal(19, engine.LastOptions.RendererTimeoutSeconds);
        Assert.Equal(23, engine.LastOptions.RendererTestPercentage);
    }

    [Fact]
    public async Task ReturnsDiagnosticsAsSuccessfulSortedReportAndCleansOnlyOwnedFiles()
    {
        var engine = new FakeEngine();
        await using var server = await Server.StartAsync(engine);
        var owned = Path.Combine(server.Directory, "stunts", "race.BNI");
        var unrelated = Path.Combine(server.Directory, "notes.txt");
        await File.WriteAllTextAsync(owned, "generated", TestContext.Current.CancellationToken);
        await File.WriteAllTextAsync(unrelated, "keep", TestContext.Current.CancellationToken);
        var existingExecutable = Path.Combine(server.Directory, "stunts", "repldump.exe");
        await File.WriteAllBytesAsync(existingExecutable, new byte[] { 99 }, TestContext.Current.CancellationToken);
        engine.Handler = (_, _) => Task.FromResult(new ShardResult
        {
            Completed = true,
            Diagnostics =
            [
                "ERROR|type=missing_output|input=z.rpl|output=z.BNI",
                "ERROR|type=missing_output|input=a.rpl|output=a.BNI",
                "ERROR|type=missing_output|input=z.rpl|output=z.BNI"
            ],
            OwnedFiles = [owned]
        });
        using var request = Request(Form());
        using var response = await server.Client.SendAsync(request, TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.OK, response.StatusCode);
        Assert.Equal("text/plain", response.Content.Headers.ContentType!.MediaType);
        Assert.Equal("utf-8", response.Content.Headers.ContentType.CharSet);
        Assert.Equal("partitions_all.txt", response.Content.Headers.ContentDisposition!.FileName);
        var report = await response.Content.ReadAsStringAsync(TestContext.Current.CancellationToken);
        Assert.Equal(2, report.Split('\n', StringSplitOptions.RemoveEmptyEntries).Length);
        Assert.StartsWith("ERROR|type=missing_output|input=a.rpl", report);
        await UntilAsync(() => !File.Exists(owned));
        Assert.True(File.Exists(unrelated));
        Assert.Equal(new byte[] { 1, 2, 3 }, await File.ReadAllBytesAsync(existingExecutable, TestContext.Current.CancellationToken));
        Assert.Single(System.IO.Directory.EnumerateFiles(Path.Combine(server.Directory, "stunts")),
            path => Path.GetFileName(path).Equals("repldump.exe", StringComparison.OrdinalIgnoreCase));
    }

    [Fact]
    public async Task FailedProcessingRetainsDiagnosticsAndReleasesGate()
    {
        var engine = new FakeEngine();
        await using var server = await Server.StartAsync(engine);
        var owned = Path.Combine(server.Directory, "stunts", "race.BNI");
        await File.WriteAllTextAsync(owned, "partial", TestContext.Current.CancellationToken);
        engine.Handler = (_, _) => Task.FromResult(new ShardResult
        {
            Completed = false,
            Failure = "worker failed",
            OwnedFiles = [owned]
        });
        using var request = Request(Form());
        using var response = await server.Client.SendAsync(request, TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.InternalServerError, response.StatusCode);
        Assert.True(File.Exists(owned));
        Assert.True(File.Exists(Path.Combine(server.Directory, "partitions_all.txt")));
        engine.Handler = (_, _) => Task.FromResult(new ShardResult { Completed = true });
        using var next = Request(Form());
        using var nextResponse = await server.Client.SendAsync(next, TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.OK, nextResponse.StatusCode);
    }

    [Fact]
    public async Task BusyRequestsRejectImmediatelyAfterAuthentication()
    {
        using var output = new CapturedLog();
        var clock = new RequestTimeProvider();
        var entered = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var release = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var engine = new FakeEngine
        {
            Handler = async (_, token) =>
            {
                clock.Advance(TimeSpan.FromHours(1));
                entered.SetResult();
                await release.Task.WaitAsync(token);
                clock.Advance(TimeSpan.FromSeconds(5));
                return new ShardResult { Completed = true };
            }
        };
        await using var server = await Server.StartAsync(engine, logOutput: output, timeProvider: clock);
        using var first = Request(Form());
        var firstResponse = server.Client.SendAsync(first, TestContext.Current.CancellationToken);
        await entered.Task.WaitAsync(TimeSpan.FromSeconds(5), TestContext.Current.CancellationToken);
        try
        {
            using var busy = Request(new ByteArrayContent([]));
            using var response = await server.Client.SendAsync(busy, TestContext.Current.CancellationToken);
            Assert.Equal(HttpStatusCode.ServiceUnavailable, response.StatusCode);
            Assert.Equal(TimeSpan.FromSeconds(60), response.Headers.RetryAfter!.Delta);
            using var unauthorized = new HttpRequestMessage(HttpMethod.Post, "/process");
            using var denied = await server.Client.SendAsync(unauthorized, TestContext.Current.CancellationToken);
            Assert.Equal(HttpStatusCode.Unauthorized, denied.StatusCode);
        }
        finally
        {
            release.TrySetResult();
        }
        using var completed = await firstResponse;
        Assert.Equal(HttpStatusCode.OK, completed.StatusCode);
        var lines = output.ToString().Split(Environment.NewLine, StringSplitOptions.RemoveEmptyEntries);
        Assert.Single(lines, line => line.StartsWith("[00:00:00] Processing requested phases:", StringComparison.Ordinal));
        Assert.Contains("[01:00:05] Processing requested phases (physics and renderer) took 01:00:05.", lines);
    }

    [Fact]
    public async Task DeadlineWaitsForWorkerCleanupAndRetainsPartialFiles()
    {
        var cancelled = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var release = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var engine = new FakeEngine
        {
            Handler = async (_, token) =>
            {
                try
                {
                    await Task.Delay(Timeout.InfiniteTimeSpan, token);
                }
                catch (OperationCanceledException)
                {
                    cancelled.TrySetResult();
                    await release.Task;
                }
                return new ShardResult { Completed = false, Failure = "cancelled" };
            }
        };
        await using var server = await Server.StartAsync(engine, 1);
        var partial = Path.Combine(server.Directory, "stunts", "race.BIN.pending");
        await File.WriteAllTextAsync(partial, "", TestContext.Current.CancellationToken);
        using var first = Request(Form());
        var responseTask = server.Client.SendAsync(first, TestContext.Current.CancellationToken);
        await cancelled.Task.WaitAsync(TimeSpan.FromSeconds(5), TestContext.Current.CancellationToken);
        try
        {
            using var busy = Request(Form());
            using var busyResponse = await server.Client.SendAsync(busy, TestContext.Current.CancellationToken);
            Assert.Equal(HttpStatusCode.ServiceUnavailable, busyResponse.StatusCode);
            Assert.False(responseTask.IsCompleted);
        }
        finally
        {
            release.TrySetResult();
        }
        using var response = await responseTask;
        Assert.Equal(HttpStatusCode.GatewayTimeout, response.StatusCode);
        Assert.True(File.Exists(partial));
        var retained = await File.ReadAllTextAsync(Path.Combine(server.Directory, "partitions_all.txt"),
            TestContext.Current.CancellationToken);
        Assert.Contains("ERROR|type=incomplete_run", retained);
        Assert.True(File.Exists(Path.Combine(server.Directory, "shard-0.json")));
    }

    [Fact]
    public async Task ClientDisconnectCancelsWorkerAndRetainsFiles()
    {
        var entered = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var cancelled = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var engine = new FakeEngine
        {
            Handler = async (_, token) =>
            {
                entered.TrySetResult();
                try
                {
                    await Task.Delay(Timeout.InfiniteTimeSpan, token);
                }
                catch (OperationCanceledException)
                {
                    cancelled.TrySetResult();
                    throw;
                }
                return new ShardResult { Completed = true };
            }
        };
        await using var server = await Server.StartAsync(engine);
        var partial = Path.Combine(server.Directory, "stunts", "race.PDD");
        await File.WriteAllTextAsync(partial, "partial", TestContext.Current.CancellationToken);
        using var cancellation = new CancellationTokenSource();
        using var request = Request(Form());
        var responseTask = server.Client.SendAsync(request, cancellation.Token);
        await entered.Task.WaitAsync(TimeSpan.FromSeconds(5), TestContext.Current.CancellationToken);
        cancellation.Cancel();
        await Assert.ThrowsAnyAsync<OperationCanceledException>(() => responseTask);
        await cancelled.Task.WaitAsync(TimeSpan.FromSeconds(5), TestContext.Current.CancellationToken);
        Assert.True(File.Exists(partial));
    }

    [Fact]
    public async Task ProcessingDeadlineDoesNotLimitSendingCompletedReport()
    {
        var diagnostic = "ERROR|type=missing_output|input=race.rpl|output=" + new string('x', 16 * 1024 * 1024);
        var engine = new FakeEngine
        {
            Handler = (_, _) => Task.FromResult(new ShardResult
            {
                Completed = true,
                Diagnostics = [diagnostic]
            })
        };
        await using var server = await Server.StartAsync(engine, 1);
        using var request = Request(Form());
        using var response = await server.Client.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, TestContext.Current.CancellationToken);
        Assert.Equal(HttpStatusCode.OK, response.StatusCode);
        await Task.Delay(1500, TestContext.Current.CancellationToken);
        var report = await response.Content.ReadAsStringAsync(TestContext.Current.CancellationToken);
        Assert.Equal(diagnostic, report.TrimEnd('\r', '\n'));
    }

    private static MultipartFormDataContent Form(byte[]? repldump = null, byte[]? pixldump = null)
    {
        var form = new MultipartFormDataContent();
        form.Add(new ByteArrayContent(repldump ?? [1, 2, 3]), "repldump", "repldump.exe");
        form.Add(new ByteArrayContent(pixldump ?? [4, 5, 6]), "pixldump", "pixldump.exe");
        return form;
    }

    private static HttpRequestMessage Request(HttpContent content, string path = "/process")
    {
        var request = new HttpRequestMessage(HttpMethod.Post, path) { Content = content };
        request.Headers.Add("X-API-Key", "secret");
        return request;
    }

    private static async Task UntilAsync(Func<bool> condition)
    {
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(5));
        while (!condition())
        {
            await Task.Delay(10, timeout.Token);
        }
    }

    private sealed class FakeEngine : RegressionEngine
    {
        public int Calls { get; private set; }
        public RunOptions? LastOptions { get; private set; }
        public Func<RunOptions, CancellationToken, Task<ShardResult>> Handler { get; set; } =
            (_, _) => Task.FromResult(new ShardResult { Completed = true });

        public override Task<ShardResult> RunAsync(RunOptions options, CancellationToken cancellationToken)
        {
            Calls++;
            LastOptions = options;
            return Handler(options, cancellationToken);
        }
    }

    private sealed class Server : IAsyncDisposable
    {
        public required WebApplication App { get; init; }
        public required string Directory { get; init; }
        public required HttpClient Client { get; init; }

        public static async Task<Server> StartAsync(FakeEngine? engine = null, int timeout = 1800,
            string? appSettings = null, TextWriter? logOutput = null, TimeProvider? timeProvider = null)
        {
            var directory = Path.Combine(Path.GetTempPath(), "dumpsrv-http-" + Guid.NewGuid().ToString("N"));
            System.IO.Directory.CreateDirectory(Path.Combine(directory, "stunts"));
            if (appSettings is not null)
            {
                await File.WriteAllTextAsync(Path.Combine(directory, "appsettings.json"), appSettings);
            }
            var app = HttpService.Build(new ServiceOptions
            {
                ApiKey = "secret",
                PartitionCount = 7,
                DosBoxTimeoutSeconds = 19,
                RendererTestPercentage = 23,
                ResponseProcessingTimeoutSeconds = timeout,
                ServiceDirectory = directory
            }, engine ?? new FakeEngine(), logOutput, timeProvider);
            app.Urls.Clear();
            app.Urls.Add("http://127.0.0.1:0");
            await app.StartAsync();
            return new Server
            {
                App = app,
                Directory = directory,
                Client = new HttpClient { BaseAddress = new Uri(app.Urls.Single()) }
            };
        }

        public async ValueTask DisposeAsync()
        {
            Client.Dispose();
            await App.StopAsync();
            await App.DisposeAsync();
            System.IO.Directory.Delete(Directory, true);
        }
    }

    private sealed class RequestTimeProvider : TimeProvider
    {
        private long timestamp;
        public override long TimestampFrequency => TimeSpan.TicksPerSecond;
        public override long GetTimestamp() => Interlocked.Read(ref timestamp);
        public void Advance(TimeSpan elapsed) => Interlocked.Add(ref timestamp, elapsed.Ticks);
    }

    private sealed class CapturedLog : StringWriter
    {
        private readonly object sync = new();

        public override void WriteLine(string? value)
        {
            lock (sync)
            {
                base.WriteLine(value);
            }
        }

        public override string ToString()
        {
            lock (sync)
            {
                return base.ToString();
            }
        }
    }

    private sealed class ChunkedContent(byte[] bytes) : HttpContent
    {
        protected override Task SerializeToStreamAsync(Stream stream, TransportContext? context)
        {
            return stream.WriteAsync(bytes).AsTask();
        }

        protected override bool TryComputeLength(out long length)
        {
            length = 0;
            return false;
        }
    }
}
