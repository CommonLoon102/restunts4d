#requires -Version 7.0

<#
.SYNOPSIS
Receives REPLDUMP.EXE and PIXLDUMP.EXE, runs replay regression tests, and
returns partitions_all.txt.

.EXAMPLE
$env:DUMPSRV_API_KEY = 'replace-with-a-long-random-secret'
./dumpsrv.ps1 -PartitionCount 12 -DosBoxTimeoutSeconds 60
#>

[CmdletBinding()]
param(
    [Parameter()]
    [string]$ApiKey = $env:DUMPSRV_API_KEY,

    [Parameter(Mandatory)]
    [ValidateRange(1, 64)]
    [int]$PartitionCount,

    [Parameter()]
    [ValidateRange(1, 65535)]
    [int]$Port = 8080,

    [Parameter()]
    [ValidateRange(1, 2147483)]
    [int]$DosBoxTimeoutSeconds = 60
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$endpointPath = '/process'
$maximumExecutableBytes = 1MB
$maximumUploadBytes = (2 * $maximumExecutableBytes) + 64KB
$processingTimeoutMilliseconds = 30 * 60 * 1000
$scriptDirectory = $PSScriptRoot
$stuntsDirectory = Join-Path $scriptDirectory 'stunts'
$stateProcessingScript = Join-Path $scriptDirectory 'rpl2statemain.ps1'
$stateWorkerScript = Join-Path $scriptDirectory 'rpl2state.ps1'
$rendererProcessingScript = Join-Path $scriptDirectory 'rpl2pixdumpmain.ps1'
$rendererWorkerScript = Join-Path $scriptDirectory 'rpl2pixdump.ps1'
$repldumpUploadPath = Join-Path $stuntsDirectory 'REPLDUMP.EXE'
$pixldumpUploadPath = Join-Path $stuntsDirectory 'PIXLDUMP.EXE'
$resultPath = Join-Path $scriptDirectory 'partitions_all.txt'

if ([string]::IsNullOrWhiteSpace($ApiKey)) {
    throw 'Set DUMPSRV_API_KEY or provide -ApiKey.'
}

foreach ($requiredScript in @(
        $stateProcessingScript,
        $stateWorkerScript,
        $rendererProcessingScript,
        $rendererWorkerScript
    )) {
    if (-not (Test-Path -LiteralPath $requiredScript -PathType Leaf)) {
        throw "Required script not found: $requiredScript"
    }
}

if (-not (Test-Path -LiteralPath $stuntsDirectory -PathType Container)) {
    throw "Stunts directory not found: $stuntsDirectory"
}

if (-not [System.Net.HttpListener]::IsSupported) {
    throw 'System.Net.HttpListener is not supported on this system.'
}

if ($null -eq (Get-Command Start-ThreadJob -ErrorAction SilentlyContinue)) {
    throw 'Start-ThreadJob is required. Run this service with PowerShell 7.'
}

function Test-ApiKey {
    param(
        [AllowNull()]
        [string]$Candidate,

        [Parameter(Mandatory)]
        [string]$Expected
    )

    if ($null -eq $Candidate) {
        return $false
    }

    $candidateBytes = [System.Text.Encoding]::UTF8.GetBytes($Candidate)
    $expectedBytes = [System.Text.Encoding]::UTF8.GetBytes($Expected)
    if ($candidateBytes.Length -ne $expectedBytes.Length) {
        return $false
    }

    $difference = 0
    for ($index = 0; $index -lt $expectedBytes.Length; $index++) {
        $difference = $difference -bor (
            $candidateBytes[$index] -bxor $expectedBytes[$index]
        )
    }

    return $difference -eq 0
}

function Send-TextResponse {
    param(
        [Parameter(Mandatory)]
        [System.Net.HttpListenerContext]$Context,

        [Parameter(Mandatory)]
        [int]$StatusCode,

        [Parameter(Mandatory)]
        [string]$Message,

        [hashtable]$Headers = @{}
    )

    $body = [System.Text.Encoding]::UTF8.GetBytes(
        $Message + [Environment]::NewLine
    )
    $response = $Context.Response
    $response.StatusCode = $StatusCode
    $response.ContentType = 'text/plain; charset=utf-8'
    $response.ContentLength64 = $body.Length

    foreach ($headerName in $Headers.Keys) {
        $response.Headers[$headerName] = [string]$Headers[$headerName]
    }

    try {
        $response.OutputStream.Write($body, 0, $body.Length)
    }
    finally {
        $response.Close()
    }
}

$requestHandler = {
    param(
        [System.Net.HttpListenerContext]$Context,
        [string]$StuntsDirectory,
        [string]$StateProcessingScript,
        [string]$RendererProcessingScript,
        [string]$RepldumpUploadPath,
        [string]$PixldumpUploadPath,
        [string]$ResultPath,
        [int]$PartitionCount,
        [int]$DosBoxTimeoutSeconds,
        [long]$MaximumExecutableBytes,
        [long]$MaximumUploadBytes,
        [int]$ProcessingTimeoutMilliseconds
    )

    Set-StrictMode -Version Latest
    $ErrorActionPreference = 'Stop'
    $responseClosed = $false
    $process = $null

    function Send-HandlerTextResponse {
        param(
            [int]$StatusCode,
            [string]$Message
        )

        $body = [System.Text.Encoding]::UTF8.GetBytes(
            $Message + [Environment]::NewLine
        )
        $response = $Context.Response
        $response.StatusCode = $StatusCode
        $response.ContentType = 'text/plain; charset=utf-8'
        $response.ContentLength64 = $body.Length

        try {
            $response.OutputStream.Write($body, 0, $body.Length)
        }
        finally {
            $response.Close()
        }
    }

    function Test-BytesAt {
        param(
            [byte[]]$Bytes,
            [byte[]]$Pattern,
            [int]$Offset
        )

        if ($Offset -lt 0 -or
            $Offset + $Pattern.Length -gt $Bytes.Length) {
            return $false
        }

        for ($index = 0; $index -lt $Pattern.Length; $index++) {
            if ($Bytes[$Offset + $index] -ne $Pattern[$index]) {
                return $false
            }
        }

        return $true
    }

    function Find-ByteSequence {
        param(
            [byte[]]$Bytes,
            [byte[]]$Pattern,
            [int]$StartIndex
        )

        $lastIndex = $Bytes.Length - $Pattern.Length
        for ($index = $StartIndex; $index -le $lastIndex; $index++) {
            if (Test-BytesAt $Bytes $Pattern $index) {
                return $index
            }
        }

        return -1
    }

    function ConvertFrom-MultipartFormData {
        param(
            [byte[]]$Bytes,
            [string]$Boundary
        )

        if ([string]::IsNullOrWhiteSpace($Boundary) -or
            $Boundary.Length -gt 200 -or
            $Boundary.IndexOfAny([char[]]"`r`n") -ge 0) {
            throw [System.IO.InvalidDataException]::new(
                'The multipart boundary is invalid.'
            )
        }

        [byte[]]$boundaryBytes =
            [System.Text.Encoding]::ASCII.GetBytes("--$Boundary")
        [byte[]]$delimiterBytes =
            [System.Text.Encoding]::ASCII.GetBytes("`r`n--$Boundary")
        [byte[]]$headerTerminator = @(13, 10, 13, 10)
        [byte[]]$crlf = @(13, 10)
        [byte[]]$closingMarker = @(45, 45)
        $parts = [System.Collections.Generic.Dictionary[string, byte[]]]::new(
            [System.StringComparer]::OrdinalIgnoreCase
        )
        $cursor = 0

        while ($true) {
            if (-not (Test-BytesAt $Bytes $boundaryBytes $cursor)) {
                throw [System.IO.InvalidDataException]::new(
                    'The multipart body contains an invalid boundary.'
                )
            }

            $cursor += $boundaryBytes.Length
            if (Test-BytesAt $Bytes $closingMarker $cursor) {
                $cursor += $closingMarker.Length
                if (Test-BytesAt $Bytes $crlf $cursor) {
                    $cursor += $crlf.Length
                }
                if ($cursor -ne $Bytes.Length) {
                    throw [System.IO.InvalidDataException]::new(
                        'The multipart body has data after its final boundary.'
                    )
                }
                break
            }

            if (-not (Test-BytesAt $Bytes $crlf $cursor)) {
                throw [System.IO.InvalidDataException]::new(
                    'The multipart boundary is not followed by CRLF.'
                )
            }
            $cursor += $crlf.Length

            $headerEnd = Find-ByteSequence $Bytes $headerTerminator $cursor
            if ($headerEnd -lt 0) {
                throw [System.IO.InvalidDataException]::new(
                    'A multipart part has incomplete headers.'
                )
            }

            $headerLength = $headerEnd - $cursor
            $headerText = [System.Text.Encoding]::UTF8.GetString(
                $Bytes,
                $cursor,
                $headerLength
            )
            $contentDisposition = @(
                $headerText -split "`r`n" |
                    Where-Object {
                        $_ -match '(?i)^Content-Disposition\s*:'
                    }
            )
            if ($contentDisposition.Count -ne 1) {
                throw [System.IO.InvalidDataException]::new(
                    'Each multipart part must have Content-Disposition.'
                )
            }

            $nameMatch = [regex]::Match(
                $contentDisposition[0],
                '(?i)(?:^|;\s*)name=(?:"([^"]+)"|([^;\s]+))'
            )
            if (-not $nameMatch.Success) {
                throw [System.IO.InvalidDataException]::new(
                    'A multipart part has no field name.'
                )
            }

            $contentStart = $headerEnd + $headerTerminator.Length
            $nextBoundary = Find-ByteSequence `
                $Bytes $delimiterBytes $contentStart
            if ($nextBoundary -lt 0) {
                throw [System.IO.InvalidDataException]::new(
                    'A multipart part has no terminating boundary.'
                )
            }

            $contentLength = $nextBoundary - $contentStart
            [byte[]]$content = [byte[]]::new($contentLength)
            [System.Array]::Copy(
                $Bytes,
                $contentStart,
                $content,
                0,
                $contentLength
            )

            $fieldName = $nameMatch.Groups[1].Value
            if ([string]::IsNullOrEmpty($fieldName)) {
                $fieldName = $nameMatch.Groups[2].Value
            }
            if ($parts.ContainsKey($fieldName)) {
                throw [System.IO.InvalidDataException]::new(
                    "Duplicate multipart field: $fieldName"
                )
            }
            $parts.Add($fieldName, $content)
            $cursor = $nextBoundary + $crlf.Length
        }

        return ,$parts
    }

    function Get-BooleanMultipartField {
        param(
            [Parameter(Mandatory)]
            [object]$Parts,

            [Parameter(Mandatory)]
            [string]$Name,

            [Parameter(Mandatory)]
            [bool]$DefaultValue
        )

        if (-not $Parts.ContainsKey($Name)) {
            return $DefaultValue
        }

        $value = [System.Text.Encoding]::UTF8.GetString(
            $Parts[$Name]
        ).Trim()
        switch ($value.ToLowerInvariant()) {
            'true' { return $true }
            'false' { return $false }
            default {
                throw [System.IO.InvalidDataException]::new(
                    "$Name must be true or false."
                )
            }
        }
    }

    try {
        $uploadBuffer = [System.IO.MemoryStream]::new()
        try {
            $contentType = $Context.Request.ContentType
            $boundaryMatch = [regex]::Match(
                [string]$contentType,
                '(?i)(?:^|;)\s*boundary=(?:"([^"]+)"|([^;]+))'
            )
            if ($null -eq $contentType -or
                -not $contentType.StartsWith(
                    'multipart/form-data',
                    [System.StringComparison]::OrdinalIgnoreCase
                ) -or
                -not $boundaryMatch.Success) {
                throw [System.IO.InvalidDataException]::new(
                    'Content-Type must be multipart/form-data with a boundary.'
                )
            }

            $boundary = $boundaryMatch.Groups[1].Value
            if ([string]::IsNullOrEmpty($boundary)) {
                $boundary = $boundaryMatch.Groups[2].Value.Trim()
            }

            [byte[]]$readBuffer = [byte[]]::new(65536)

            while (($bytesRead = $Context.Request.InputStream.Read(
                        $readBuffer,
                        0,
                        $readBuffer.Length
                    )) -gt 0) {
                if ($uploadBuffer.Length + $bytesRead -gt
                    $MaximumUploadBytes) {
                    Send-HandlerTextResponse 413 'Payload Too Large'
                    $responseClosed = $true
                    return
                }

                $uploadBuffer.Write($readBuffer, 0, $bytesRead)
            }

            if ($uploadBuffer.Length -eq 0) {
                throw [System.IO.InvalidDataException]::new(
                    'The request body is empty.'
                )
            }

            [byte[]]$uploadBytes = $uploadBuffer.ToArray()
        }
        finally {
            $uploadBuffer.Dispose()
            $Context.Request.InputStream.Close()
        }

        $parts = ConvertFrom-MultipartFormData $uploadBytes $boundary
        foreach ($fieldName in $parts.Keys) {
            if ($fieldName -notin @(
                    'repldump',
                    'pixldump',
                    'physics_tests',
                    'renderer_tests'
                )) {
                throw [System.IO.InvalidDataException]::new(
                    "Unexpected multipart field: $fieldName"
                )
            }
        }

        foreach ($requiredField in @('repldump', 'pixldump')) {
            if (-not $parts.ContainsKey($requiredField) -or
                $parts[$requiredField].Length -eq 0) {
                throw [System.IO.InvalidDataException]::new(
                    "Missing or empty multipart field: $requiredField"
                )
            }
            if ($parts[$requiredField].Length -gt $MaximumExecutableBytes) {
                Send-HandlerTextResponse 413 'Payload Too Large'
                $responseClosed = $true
                return
            }
        }

        $runPhysicsTests = Get-BooleanMultipartField `
            $parts 'physics_tests' $true
        $runRendererTests = Get-BooleanMultipartField `
            $parts 'renderer_tests' $true
        if (-not $runPhysicsTests -and -not $runRendererTests) {
            throw [System.IO.InvalidDataException]::new(
                'At least one test type must be enabled.'
            )
        }

        [System.IO.File]::WriteAllBytes(
            $RepldumpUploadPath,
            $parts['repldump']
        )
        [System.IO.File]::WriteAllBytes(
            $PixldumpUploadPath,
            $parts['pixldump']
        )

        Write-Output (
            (('[{0:O}] Uploaded {1} and {2}; physics tests: {3}; ' +
            'renderer tests: {4}.') -f
            [DateTime]::UtcNow,
            $RepldumpUploadPath,
            $PixldumpUploadPath,
            $runPhysicsTests,
            $runRendererTests)
        )

        # Initialize the shared report before either optional phase. The state
        # script also does this for standalone runs, but renderer-only requests
        # must not reuse output retained after an earlier failure.
        for ($partition = 0; $partition -lt $PartitionCount; $partition++) {
            $partitionFile = Join-Path (
                [System.IO.Path]::GetDirectoryName($ResultPath)
            ) "partition_$partition.txt"
            if (Test-Path -LiteralPath $partitionFile -PathType Leaf) {
                Remove-Item -LiteralPath $partitionFile -Force
            }
        }
        $utf8NoBom = [System.Text.UTF8Encoding]::new($false)
        [System.IO.File]::WriteAllText($ResultPath, '', $utf8NoBom)

        $processingPhases = @()
        if ($runPhysicsTests) {
            $processingPhases += [pscustomobject]@{
                Name = 'State comparison'
                Script = $StateProcessingScript
            }
        }
        if ($runRendererTests) {
            $processingPhases += [pscustomobject]@{
                Name = 'Renderer comparison'
                Script = $RendererProcessingScript
            }
        }

        $processingStopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        foreach ($phase in $processingPhases) {
            $remainingMilliseconds =
                $ProcessingTimeoutMilliseconds -
                [long]$processingStopwatch.ElapsedMilliseconds
            if ($remainingMilliseconds -le 0) {
                Write-Warning 'Replay processing exceeded the 30-minute limit.'
                Send-HandlerTextResponse 504 'Processing timed out.'
                $responseClosed = $true
                return
            }

            Write-Output ("Starting $($phase.Name.ToLowerInvariant()).")

            $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
            $startInfo.FileName = [Environment]::ProcessPath
            $startInfo.WorkingDirectory = $StuntsDirectory
            $startInfo.UseShellExecute = $false
            $startInfo.CreateNoWindow = $true
            $startInfo.RedirectStandardOutput = $true
            $startInfo.RedirectStandardError = $true

            foreach ($argument in @(
                    '-NoLogo',
                    '-NoProfile',
                    '-NonInteractive',
                    '-File',
                    $phase.Script,
                    '-PartitionCount',
                    [string]$PartitionCount,
                    '-DosBoxTimeoutSeconds',
                    [string]$DosBoxTimeoutSeconds
                )) {
                [void]$startInfo.ArgumentList.Add($argument)
            }

            $process = [System.Diagnostics.Process]::new()
            $process.StartInfo = $startInfo
            $processStarted = $false
            $standardOutputTask = $null
            $standardErrorTask = $null

            try {
                $processStarted = $process.Start()
                if (-not $processStarted) {
                    throw 'The processing PowerShell process did not start.'
                }

                $standardOutputTask =
                    $process.StandardOutput.ReadToEndAsync()
                $standardErrorTask =
                    $process.StandardError.ReadToEndAsync()
                $completed = $process.WaitForExit(
                    [int]$remainingMilliseconds
                )

                if (-not $completed) {
                    try {
                        $process.Kill($true)
                    }
                    catch {
                        $process.Kill()
                    }
                    $process.WaitForExit()
                }

                $standardOutput =
                    $standardOutputTask.GetAwaiter().GetResult()
                $standardError =
                    $standardErrorTask.GetAwaiter().GetResult()

                if (-not [string]::IsNullOrWhiteSpace($standardOutput)) {
                    Write-Output $standardOutput.TrimEnd()
                }
                if (-not [string]::IsNullOrWhiteSpace($standardError)) {
                    Write-Warning $standardError.TrimEnd()
                }

                if (-not $completed) {
                    Write-Warning (
                        "$($phase.Name) exceeded the 30-minute request limit."
                    )
                    Send-HandlerTextResponse 504 'Processing timed out.'
                    $responseClosed = $true
                    return
                }

                if ($process.ExitCode -ne 0) {
                    Write-Warning (
                        '{0} exited with code {1}.' -f
                        $phase.Name,
                        $process.ExitCode
                    )
                    Send-HandlerTextResponse 500 'Processing failed.'
                    $responseClosed = $true
                    return
                }
            }
            finally {
                if ($null -ne $process) {
                    try {
                        if ($processStarted -and -not $process.HasExited) {
                            try {
                                $process.Kill($true)
                            }
                            catch {
                                $process.Kill()
                            }
                            $process.WaitForExit()
                        }
                    }
                    finally {
                        $process.Dispose()
                        $process = $null
                    }
                }
            }
        }

        if (-not (Test-Path -LiteralPath $ResultPath -PathType Leaf)) {
            Write-Warning "Result file not found: $ResultPath"
            Send-HandlerTextResponse 500 'The result file was not produced.'
            $responseClosed = $true
            return
        }

        [byte[]]$resultBytes = [System.IO.File]::ReadAllBytes($ResultPath)
        $response = $Context.Response
        $response.StatusCode = 200
        $response.ContentType = 'text/plain; charset=utf-8'
        $response.Headers['Content-Disposition'] =
            'attachment; filename="partitions_all.txt"'
        $response.ContentLength64 = $resultBytes.Length
        $response.OutputStream.Write(
            $resultBytes,
            0,
            $resultBytes.Length
        )
        $response.Close()
        $responseClosed = $true

        # Keep generated files available until the successful response closes.
        $cleanupErrors = @()

        try {
            Get-ChildItem -LiteralPath $StuntsDirectory -File |
                Where-Object {
                    $_.Extension -ieq '.bni' -or
                    $_.Extension -ieq '.pdd'
                } |
                Remove-Item -Force
        }
        catch {
            $cleanupErrors += (
                'BNI/PDD cleanup failed: {0}' -f $_.Exception.Message
            )
        }

        try {
            Get-ChildItem -LiteralPath (
                [System.IO.Path]::GetDirectoryName($StateProcessingScript)
            ) -File |
                Where-Object { $_.Extension -ieq '.txt' } |
                Remove-Item -Force
        }
        catch {
            $cleanupErrors += (
                'TXT cleanup failed: {0}' -f $_.Exception.Message
            )
        }

        if ($cleanupErrors.Count -eq 0) {
            Write-Output (
                '[{0:O}] Result sent; removed all BNI, PDD, and TXT files.' -f
                [DateTime]::UtcNow
            )
        }
        else {
            Write-Warning (
                'The response was sent, but cleanup failed: {0}' -f
                ($cleanupErrors -join '; ')
            )
        }
    }
    catch [System.IO.InvalidDataException] {
        Write-Warning ('Bad request: {0}' -f $_.Exception.Message)

        if (-not $responseClosed) {
            try {
                Send-HandlerTextResponse 400 $_.Exception.Message
                $responseClosed = $true
            }
            catch {
                Write-Warning (
                    'Could not send the error response: {0}' -f
                    $_.Exception.Message
                )
            }
        }
    }
    catch {
        Write-Warning ('Request failed: {0}' -f $_.Exception.Message)

        if (-not $responseClosed) {
            try {
                Send-HandlerTextResponse 500 'Internal Server Error'
                $responseClosed = $true
            }
            catch {
                Write-Warning (
                    'Could not send the error response: {0}' -f
                    $_.Exception.Message
                )
            }
        }
    }
    finally {
        if ($null -ne $process) {
            try {
                if (-not $process.HasExited) {
                    try {
                        $process.Kill($true)
                    }
                    catch {
                        $process.Kill()
                    }
                    $process.WaitForExit()
                }
            }
            finally {
                $process.Dispose()
            }
        }

        if (-not $responseClosed) {
            try {
                $Context.Response.Close()
            }
            catch {
                # The caller may have disconnected.
            }
        }
    }
}

$listener = [System.Net.HttpListener]::new()
$listenerPrefix = "http://+:$Port/"
$listener.Prefixes.Add($listenerPrefix)
$activeJob = $null

try {
    $listener.Start()
    Write-Host "Listening on $listenerPrefix"
    Write-Host "POST multipart executable uploads to $endpointPath"

    $contextTask = $listener.GetContextAsync()

    while ($listener.IsListening) {
        if ($null -ne $activeJob) {
            Receive-Job -Job $activeJob -ErrorAction Continue

            if ($activeJob.State -in @('Completed', 'Failed', 'Stopped')) {
                Receive-Job -Job $activeJob -ErrorAction Continue
                Remove-Job -Job $activeJob -Force
                $activeJob = $null
            }
        }

        if (-not $contextTask.Wait(250)) {
            continue
        }

        $context = $contextTask.GetAwaiter().GetResult()
        $contextTask = $listener.GetContextAsync()

        try {
            if ($context.Request.Url.AbsolutePath -cne $endpointPath) {
                Send-TextResponse $context 404 'Not Found'
                continue
            }

            if ($context.Request.HttpMethod -cne 'POST') {
                Send-TextResponse `
                    $context `
                    405 `
                    'Method Not Allowed' `
                    @{ Allow = 'POST' }
                continue
            }

            if (-not (Test-ApiKey `
                        $context.Request.Headers['X-API-Key'] `
                        $ApiKey)) {
                Send-TextResponse $context 401 'Unauthorized'
                continue
            }

            if ($null -ne $activeJob -and
                $activeJob.State -notin @('Completed', 'Failed', 'Stopped')) {
                Send-TextResponse `
                    $context `
                    503 `
                    'Another job is already running.' `
                    @{ 'Retry-After' = '60' }
                continue
            }

            if ($context.Request.ContentLength64 -gt $maximumUploadBytes) {
                Send-TextResponse $context 413 'Payload Too Large'
                continue
            }

            if ($context.Request.ContentLength64 -eq 0) {
                Send-TextResponse $context 400 'The request body is empty.'
                continue
            }

            if ($null -ne $activeJob) {
                Receive-Job -Job $activeJob -ErrorAction Continue
                Remove-Job -Job $activeJob -Force
                $activeJob = $null
            }

            $activeJob = Start-ThreadJob `
                -Name 'dumpsrv-request' `
                -ScriptBlock $requestHandler `
                -ArgumentList @(
                    $context,
                    $stuntsDirectory,
                    $stateProcessingScript,
                    $rendererProcessingScript,
                    $repldumpUploadPath,
                    $pixldumpUploadPath,
                    $resultPath,
                    $PartitionCount,
                    $DosBoxTimeoutSeconds,
                    $maximumExecutableBytes,
                    $maximumUploadBytes,
                    $processingTimeoutMilliseconds
                )
        }
        catch {
            Write-Warning ('Could not accept request: {0}' -f $_.Exception.Message)
            try {
                Send-TextResponse $context 500 'Internal Server Error'
            }
            catch {
                try {
                    $context.Response.Close()
                }
                catch {
                    # The caller may have disconnected.
                }
            }
        }
    }
}
finally {
    if ($listener.IsListening) {
        $listener.Stop()
    }
    $listener.Close()

    if ($null -ne $activeJob) {
        if ($activeJob.State -notin @('Completed', 'Failed', 'Stopped')) {
            Stop-Job -Job $activeJob
        }
        Receive-Job -Job $activeJob -ErrorAction Continue
        Remove-Job -Job $activeJob -Force
    }
}
