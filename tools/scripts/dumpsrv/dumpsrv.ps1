#requires -Version 7.0

<#
.SYNOPSIS
Receives REPLDUMP.EXE, runs rpl2statemain.ps1, and returns partitions_all.txt.

.EXAMPLE
$env:DUMPSRV_API_KEY = 'replace-with-a-long-random-secret'
./dumpsrv.ps1 -PartitionCount 12
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
    [int]$Port = 8080
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$endpointPath = '/process'
$maximumUploadBytes = 1MB
$processingTimeoutMilliseconds = 30 * 60 * 1000
$serviceDirectory = $PSScriptRoot
$processingScript = Join-Path $serviceDirectory 'rpl2statemain.ps1'
$workerScript = Join-Path $serviceDirectory 'rpl2state.ps1'
$uploadPath = Join-Path $serviceDirectory 'REPLDUMP.EXE'
$resultPath = Join-Path $serviceDirectory 'partitions_all.txt'

if ([string]::IsNullOrWhiteSpace($ApiKey)) {
    throw 'Set DUMPSRV_API_KEY or provide -ApiKey.'
}

foreach ($requiredScript in @($processingScript, $workerScript)) {
    if (-not (Test-Path -LiteralPath $requiredScript -PathType Leaf)) {
        throw "Required script not found: $requiredScript"
    }
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
        [string]$ServiceDirectory,
        [string]$ProcessingScript,
        [string]$UploadPath,
        [string]$ResultPath,
        [int]$PartitionCount,
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

    try {
        $uploadBuffer = [System.IO.MemoryStream]::new()
        try {
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
                Send-HandlerTextResponse 400 'The request body is empty.'
                $responseClosed = $true
                return
            }

            [System.IO.File]::WriteAllBytes(
                $UploadPath,
                $uploadBuffer.ToArray()
            )
        }
        finally {
            $uploadBuffer.Dispose()
            $Context.Request.InputStream.Close()
        }

        Write-Output (
            '[{0:O}] Uploaded {1}; starting replay processing.' -f
            [DateTime]::UtcNow,
            $UploadPath
        )

        $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
        $startInfo.FileName = [Environment]::ProcessPath
        $startInfo.WorkingDirectory = $ServiceDirectory
        $startInfo.UseShellExecute = $false
        $startInfo.CreateNoWindow = $true
        $startInfo.RedirectStandardOutput = $true
        $startInfo.RedirectStandardError = $true

        foreach ($argument in @(
                '-NoLogo',
                '-NoProfile',
                '-NonInteractive',
                '-File',
                $ProcessingScript,
                '-PartitionCount',
                [string]$PartitionCount
            )) {
            [void]$startInfo.ArgumentList.Add($argument)
        }

        $process = [System.Diagnostics.Process]::new()
        $process.StartInfo = $startInfo

        if (-not $process.Start()) {
            throw 'The processing PowerShell process did not start.'
        }

        $standardOutputTask = $process.StandardOutput.ReadToEndAsync()
        $standardErrorTask = $process.StandardError.ReadToEndAsync()
        $completed = $process.WaitForExit($ProcessingTimeoutMilliseconds)

        if (-not $completed) {
            try {
                $process.Kill($true)
            }
            catch {
                $process.Kill()
            }
            $process.WaitForExit()
        }

        $standardOutput = $standardOutputTask.GetAwaiter().GetResult()
        $standardError = $standardErrorTask.GetAwaiter().GetResult()

        if (-not [string]::IsNullOrWhiteSpace($standardOutput)) {
            Write-Output $standardOutput.TrimEnd()
        }
        if (-not [string]::IsNullOrWhiteSpace($standardError)) {
            Write-Warning $standardError.TrimEnd()
        }

        if (-not $completed) {
            Write-Warning 'Replay processing exceeded the 30-minute limit.'
            Send-HandlerTextResponse 504 'Processing timed out.'
            $responseClosed = $true
            return
        }

        if ($process.ExitCode -ne 0) {
            Write-Warning (
                'Replay processing exited with code {0}.' -f
                $process.ExitCode
            )
            Send-HandlerTextResponse 500 'Processing failed.'
            $responseClosed = $true
            return
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

        try {
            Get-ChildItem -LiteralPath $ServiceDirectory -File |
                Where-Object { $_.Extension -ieq '.txt' } |
                Remove-Item -Force
            Write-Output (
                '[{0:O}] Result sent; removed all TXT files.' -f
                [DateTime]::UtcNow
            )
        }
        catch {
            Write-Warning (
                'The response was sent, but TXT cleanup failed: {0}' -f
                $_.Exception.Message
            )
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
    Write-Host "POST raw REPLDUMP.EXE bytes to $endpointPath"

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
                    $serviceDirectory,
                    $processingScript,
                    $uploadPath,
                    $resultPath,
                    $PartitionCount,
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
