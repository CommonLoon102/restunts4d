using System.Security.Cryptography;
using System.Text;
using Microsoft.AspNetCore.Http.Features;
using Microsoft.AspNetCore.WebUtilities;
using Microsoft.Net.Http.Headers;

namespace DumpSrv;

public static class HttpService
{
    private const int MaximumExecutableBytes = 1024 * 1024;
    private const int MaximumUploadBytes = 2 * MaximumExecutableBytes + 64 * 1024;

    public static WebApplication Build(ServiceOptions options, RegressionEngine? engine = null,
        TextWriter? logOutput = null, TimeProvider? timeProvider = null)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(options.ApiKey);
        ValidateRange(options.PartitionCount, 1, 64, nameof(options.PartitionCount));
        ValidateRange(options.Port, 1, 65535, nameof(options.Port));
        ValidateRange(options.DosBoxTimeoutSeconds, 1, 2147483, nameof(options.DosBoxTimeoutSeconds));
        ValidateRange(options.RendererTestPercentage, 1, 100, nameof(options.RendererTestPercentage));
        ValidateRange(options.ResponseProcessingTimeoutSeconds, 1, 2147483,
            nameof(options.ResponseProcessingTimeoutSeconds));

        var serviceDirectory = Path.GetFullPath(options.ServiceDirectory);
        var gameDirectory = Path.Combine(serviceDirectory, "stunts");
        if (!Directory.Exists(gameDirectory))
        {
            throw new DirectoryNotFoundException($"Stunts directory not found: {gameDirectory}");
        }
        var builder = WebApplication.CreateBuilder(new WebApplicationOptions
        {
            Args = [],
            ContentRootPath = serviceDirectory
        });
        // Service options are explicit; ambient ASP.NET configuration must not
        // override the port or add listeners from appsettings or environment.
        builder.Configuration.Sources.Clear();
        builder.Configuration.AddInMemoryCollection();
        var contextAccessor = new HttpContextAccessor();
        builder.Services.AddSingleton<IHttpContextAccessor>(contextAccessor);
        var console = new ElapsedConsoleLog(contextAccessor, logOutput);
        builder.Logging.ClearProviders();
        builder.Logging.AddProvider(console);
        builder.WebHost.ConfigureKestrel(server => server.Limits.MaxRequestBodySize = MaximumUploadBytes);
        builder.WebHost.UseUrls($"http://*:{options.Port}");
        var app = builder.Build();
        var gate = new SemaphoreSlim(1, 1);
        engine ??= new RegressionEngine(log: message => console.Write(message), timeProvider: timeProvider);
        var regressionEngine = engine;
        app.Run(context => HandleAsync(context, options, regressionEngine, gate,
            app.Lifetime.ApplicationStopping, console, timeProvider));
        return app;
    }

    private static void ValidateRange(int value, int minimum, int maximum, string name)
    {
        if (value < minimum || value > maximum)
        {
            throw new ArgumentOutOfRangeException(name, $"Must be between {minimum} and {maximum}.");
        }
    }

    private static async Task HandleAsync(HttpContext context, ServiceOptions options,
        RegressionEngine engine, SemaphoreSlim gate, CancellationToken stopping,
        ElapsedConsoleLog console, TimeProvider? timeProvider)
    {
        var timer = new ProcessingTimer(timeProvider);
        context.Features.Set(timer);
        var rawPath = context.Features.Get<IHttpRequestFeature>()?.RawTarget.Split('?')[0];
        if (!string.Equals(rawPath, "/process", StringComparison.Ordinal))
        {
            await SendTextAsync(context, 404, "Not Found");
            return;
        }
        if (!string.Equals(context.Request.Method, "POST", StringComparison.Ordinal))
        {
            context.Response.Headers.Allow = "POST";
            await SendTextAsync(context, 405, "Method Not Allowed");
            return;
        }
        var candidate = Encoding.UTF8.GetBytes(context.Request.Headers["X-API-Key"].ToString());
        var expected = Encoding.UTF8.GetBytes(options.ApiKey);
        if (!CryptographicOperations.FixedTimeEquals(candidate, expected))
        {
            await SendTextAsync(context, 401, "Unauthorized");
            return;
        }
        if (!await gate.WaitAsync(0, context.RequestAborted))
        {
            context.Response.Headers.RetryAfter = "60";
            await SendTextAsync(context, 503, "Another job is already running.");
            return;
        }

        using var disconnected = CancellationTokenSource.CreateLinkedTokenSource(
            context.RequestAborted, stopping);
        CancellationTokenSource? deadline = null;
        var uploadValidated = false;
        ShardResult? partialResult = null;
        Action? logTotal = null;
        try
        {
            if (context.Request.ContentLength > MaximumUploadBytes)
            {
                throw new UploadTooLargeException();
            }
            if (context.Request.ContentLength == 0)
            {
                throw new InvalidDataException("The request body is empty.");
            }
            var parts = await ReadMultipartAsync(context.Request, disconnected.Token);
            var physics = BooleanField(parts, "physics_tests");
            var renderer = BooleanField(parts, "renderer_tests");
            if (!physics && !renderer)
            {
                throw new InvalidDataException("At least one test type must be enabled.");
            }
            uploadValidated = true;

            var serviceDirectory = Path.GetFullPath(options.ServiceDirectory);
            var gameDirectory = Path.Combine(serviceDirectory, "stunts");
            await File.WriteAllBytesAsync(DosFiles.Resolve(gameDirectory, "repldump.exe"),
                parts["repldump"], disconnected.Token);
            await File.WriteAllBytesAsync(DosFiles.Resolve(gameDirectory, "pixldump.exe"),
                parts["pixldump"], disconnected.Token);

            var phases = physics && renderer ? "physics and renderer" : physics ? "physics" : "renderer";
            timer.Start();
            console.Write($"Processing requested phases: {phases}.");
            logTotal = () =>
            {
                if (timer.Stop())
                {
                    void LogPhase(string name, TimeSpan? elapsed)
                    {
                        console.Write(elapsed is { } duration
                            ? $"{name} phase took {ElapsedConsoleLog.FormatElapsed(duration)}."
                            : $"{name} phase did not start.", timer);
                    }
                    if (physics)
                    {
                        LogPhase("Physics", partialResult?.PhysicsElapsed);
                    }
                    if (renderer)
                    {
                        LogPhase("Renderer", partialResult?.RendererElapsed);
                    }
                    console.Write($"Processing requested phases ({phases}) took " +
                        $"{ElapsedConsoleLog.FormatElapsed(timer.Elapsed)}.", timer);
                }
            };
            context.Response.OnStarting(() =>
            {
                logTotal();
                return Task.CompletedTask;
            });
            deadline = new CancellationTokenSource(TimeSpan.FromSeconds(
                options.ResponseProcessingTimeoutSeconds));
            using var processing = CancellationTokenSource.CreateLinkedTokenSource(
                disconnected.Token, deadline.Token);
            partialResult = await engine.RunAsync(new RunOptions
            {
                GameDirectory = gameDirectory,
                OutputDirectory = serviceDirectory,
                DosBoxConfigPath = Path.Combine(serviceDirectory, "dosbox.proc.conf"),
                PartitionCount = options.PartitionCount,
                PhysicsTests = physics,
                RendererTests = renderer,
                RendererTestPercentage = options.RendererTestPercentage,
                DosBoxTimeoutSeconds = options.DosBoxTimeoutSeconds,
                RendererTimeoutSeconds = options.DosBoxTimeoutSeconds
            }, processing.Token);
            var result = partialResult;
            processing.Token.ThrowIfCancellationRequested();
            await ResultFiles.WriteAsync(result, serviceDirectory, processing.Token);
            if (!result.Completed || result.Failure is not null)
            {
                await SendTextAsync(context, 500, "Processing failed.");
                return;
            }

            var bytes = Encoding.UTF8.GetBytes(ReportFormatter.Text(result.Diagnostics));
            processing.Token.ThrowIfCancellationRequested();
            deadline.CancelAfter(Timeout.InfiniteTimeSpan);
            processing.Token.ThrowIfCancellationRequested();
            context.Response.StatusCode = 200;
            context.Response.ContentType = "text/plain; charset=utf-8";
            context.Response.Headers.ContentDisposition = "attachment; filename=\"partitions_all.txt\"";
            context.Response.ContentLength = bytes.Length;
            await context.Response.Body.WriteAsync(bytes, disconnected.Token);
            await context.Response.Body.FlushAsync(disconnected.Token);
            await context.Response.CompleteAsync();
            disconnected.Token.ThrowIfCancellationRequested();
            try
            {
                RegressionEngine.Cleanup(result);
            }
            catch (Exception exception)
            {
                context.RequestServices.GetRequiredService<ILoggerFactory>().CreateLogger("DumpSrv")
                    .LogWarning(exception, "The report was sent, but output cleanup failed.");
            }
        }
        catch (OperationCanceledException) when (disconnected.IsCancellationRequested)
        {
            await PreservePartialResultAsync(context, partialResult, options.ServiceDirectory,
                "Request processing was cancelled.");
            context.Abort();
        }
        catch (OperationCanceledException) when (deadline?.IsCancellationRequested == true)
        {
            await PreservePartialResultAsync(context, partialResult, options.ServiceDirectory,
                "Processing timed out.");
            await SendFailureAsync(context, 504, "Processing timed out.");
        }
        catch (UploadTooLargeException)
        {
            await SendFailureAsync(context, 413, "Payload Too Large");
        }
        catch (BadHttpRequestException exception) when (exception.StatusCode == 413)
        {
            await SendFailureAsync(context, 413, "Payload Too Large");
        }
        catch (InvalidDataException exception) when (!uploadValidated)
        {
            await SendFailureAsync(context, 400, exception.Message);
        }
        catch (Exception exception)
        {
            context.RequestServices.GetRequiredService<ILoggerFactory>().CreateLogger("DumpSrv")
                .LogError(exception, "Request processing failed.");
            await SendFailureAsync(context, 500, "Internal Server Error");
        }
        finally
        {
            // Aborted requests may never send headers and invoke OnStarting.
            try
            {
                logTotal?.Invoke();
            }
            finally
            {
                deadline?.Dispose();
                gate.Release();
            }
        }
    }

    private static async Task PreservePartialResultAsync(HttpContext context, ShardResult? result,
        string directory, string failure)
    {
        if (result is null || context.Response.HasStarted)
        {
            return;
        }
        result.Completed = false;
        result.Failure ??= failure;
        try
        {
            await ResultFiles.WriteAsync(result, directory, CancellationToken.None);
        }
        catch (Exception exception)
        {
            context.RequestServices.GetRequiredService<ILoggerFactory>().CreateLogger("DumpSrv")
                .LogWarning(exception, "Could not retain the cancelled request's partial report.");
        }
    }

    private static async Task<Dictionary<string, byte[]>> ReadMultipartAsync(HttpRequest request,
        CancellationToken cancellationToken)
    {
        if (!MediaTypeHeaderValue.TryParse(request.ContentType, out var contentType) ||
            !contentType.MediaType.Equals("multipart/form-data", StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidDataException("Content-Type must be multipart/form-data with a boundary.");
        }
        var boundary = HeaderUtilities.RemoveQuotes(contentType.Boundary).Value;
        if (string.IsNullOrWhiteSpace(boundary) || boundary.Length > 200 ||
            boundary.Contains('\r') || boundary.Contains('\n'))
        {
            throw new InvalidDataException("The multipart boundary is invalid.");
        }

        using var body = new MemoryStream();
        var buffer = new byte[65536];
        int count;
        while ((count = await request.Body.ReadAsync(buffer, cancellationToken)) != 0)
        {
            if (body.Length + count > MaximumUploadBytes)
            {
                throw new UploadTooLargeException();
            }
            await body.WriteAsync(buffer.AsMemory(0, count), cancellationToken);
        }
        if (body.Length == 0)
        {
            throw new InvalidDataException("The request body is empty.");
        }
        ValidateEnvelope(body, boundary);
        body.Position = 0;
        var reader = new MultipartReader(boundary, body)
        {
            HeadersCountLimit = MaximumUploadBytes,
            HeadersLengthLimit = MaximumUploadBytes,
            BodyLengthLimit = MaximumUploadBytes
        };
        var parts = new Dictionary<string, byte[]>(StringComparer.OrdinalIgnoreCase);
        try
        {
            MultipartSection? section;
            while ((section = await reader.ReadNextSectionAsync(cancellationToken)) is not null)
            {
                if (section.Headers is null ||
                    !section.Headers.TryGetValue(HeaderNames.ContentDisposition, out var dispositions) ||
                    dispositions.Count != 1 ||
                    !ContentDispositionHeaderValue.TryParse(dispositions[0], out var disposition))
                {
                    throw new InvalidDataException("Each multipart part must have Content-Disposition.");
                }
                var name = HeaderUtilities.RemoveQuotes(disposition.Name).Value;
                if (string.IsNullOrEmpty(name))
                {
                    throw new InvalidDataException("A multipart part has no field name.");
                }
                if (!name.Equals("repldump", StringComparison.OrdinalIgnoreCase) &&
                    !name.Equals("pixldump", StringComparison.OrdinalIgnoreCase) &&
                    !name.Equals("physics_tests", StringComparison.OrdinalIgnoreCase) &&
                    !name.Equals("renderer_tests", StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidDataException($"Unexpected multipart field: {name}");
                }
                if (parts.ContainsKey(name))
                {
                    throw new InvalidDataException($"Duplicate multipart field: {name}");
                }
                ValidateUploadFileName(name, disposition);
                using var partBody = new MemoryStream();
                await section.Body.CopyToAsync(partBody, cancellationToken);
                parts.Add(name, partBody.ToArray());
            }
        }
        catch (IOException exception)
        {
            throw new InvalidDataException("The multipart request is malformed.", exception);
        }
        foreach (var name in new[] { "repldump", "pixldump" })
        {
            if (!parts.TryGetValue(name, out var executable) || executable.Length == 0)
            {
                throw new InvalidDataException($"Missing or empty multipart field: {name}");
            }
            if (executable.Length > MaximumExecutableBytes)
            {
                throw new UploadTooLargeException();
            }
        }
        return parts;
    }

    private static void ValidateUploadFileName(string name,
        ContentDispositionHeaderValue disposition)
    {
        var expectedFileName = name.ToLowerInvariant() switch
        {
            "repldump" => "repldump.exe",
            "pixldump" => "pixldump.exe",
            _ => null
        };
        var parameters = disposition.Parameters.Where(parameter =>
            parameter.Name.Equals("filename", StringComparison.OrdinalIgnoreCase) ||
            parameter.Name.Equals("filename*", StringComparison.OrdinalIgnoreCase)).ToArray();
        if (expectedFileName is null)
        {
            if (parameters.Length != 0)
            {
                throw new InvalidDataException($"{name} must be a text field, not a file upload.");
            }
            return;
        }
        // Check both filename forms so a conflicting fallback cannot hide an invalid name.
        if (parameters.Length == 0 || parameters
            .GroupBy(parameter => parameter.Name.Value, StringComparer.OrdinalIgnoreCase)
            .Any(group => group.Count() > 1) || parameters.Any(parameter => !string.Equals(
                parameter.Name.Equals("filename*", StringComparison.OrdinalIgnoreCase)
                    ? disposition.FileNameStar.Value
                    : HeaderUtilities.RemoveQuotes(parameter.Value).Value,
                expectedFileName, StringComparison.Ordinal)))
        {
            throw new InvalidDataException($"{name} must have filename {expectedFileName}.");
        }
    }

    private static void ValidateEnvelope(MemoryStream body, string boundary)
    {
        var bytes = body.GetBuffer().AsSpan(0, (int)body.Length);
        var prefix = Encoding.ASCII.GetBytes("--" + boundary);
        var closing = Encoding.ASCII.GetBytes("--" + boundary + "--");
        if (bytes.EndsWith("\r\n"u8))
        {
            bytes = bytes[..^2];
        }
        if (!bytes.StartsWith(prefix) || !bytes.EndsWith(closing))
        {
            throw new InvalidDataException("The multipart body contains an invalid boundary.");
        }
    }

    private static bool BooleanField(Dictionary<string, byte[]> parts, string name)
    {
        if (!parts.TryGetValue(name, out var bytes))
        {
            return true;
        }
        if (bool.TryParse(Encoding.UTF8.GetString(bytes).Trim(), out var value))
        {
            return value;
        }
        throw new InvalidDataException($"{name} must be true or false.");
    }

    private static async Task SendTextAsync(HttpContext context, int status, string message)
    {
        var bytes = Encoding.UTF8.GetBytes(message + Environment.NewLine);
        context.Response.StatusCode = status;
        context.Response.ContentType = "text/plain; charset=utf-8";
        context.Response.ContentLength = bytes.Length;
        await context.Response.Body.WriteAsync(bytes, context.RequestAborted);
    }

    private static async Task SendFailureAsync(HttpContext context, int status, string message)
    {
        if (context.Response.HasStarted || context.RequestAborted.IsCancellationRequested)
        {
            context.Abort();
            return;
        }
        context.Response.Clear();
        await SendTextAsync(context, status, message);
    }

    private sealed class UploadTooLargeException : Exception;
}
