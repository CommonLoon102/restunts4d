#requires -Version 7.0

<#
.SYNOPSIS
Generates or reuses .PDO files from PIXLDUMO and compares them with fresh
.PDD files from PIXLDUMP for one replay partition.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)]
    [ValidateRange(0, 2147483646)]
    [int]$Partition,

    [Parameter(Mandatory, Position = 1)]
    [ValidateRange(1, 2147483647)]
    [int]$PartitionCount,

    [Parameter()]
    [ValidateRange(1, 2147483)]
    [int]$DosBoxTimeoutSeconds = 60,

    [Parameter()]
    [ValidateRange(1, 100)]
    [int]$RendererTestPercentage = 100
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($Partition -ge $PartitionCount) {
    throw 'Partition must be less than PartitionCount.'
}

$ScriptDirectory = $PSScriptRoot
$GameDir = Join-Path $ScriptDirectory 'stunts'
$Config = Join-Path $ScriptDirectory 'dosbox.proc.conf'
$OutputFile = Join-Path $ScriptDirectory "partition_$Partition.txt"
$TimeoutMilliseconds = $DosBoxTimeoutSeconds * 1000

if (-not (Test-Path -LiteralPath $GameDir -PathType Container)) {
    throw "Stunts directory not found: $GameDir"
}

if (-not (Test-Path -LiteralPath $Config -PathType Leaf)) {
    throw "DOSBox configuration not found: $Config"
}

# Sort the full eligible set before sampling so the sample spans the whole
# sequence, including prefixed names and gaps in the numeric counters.
$ReplayFilePattern = [regex]::new(
    '[0-9]{4}\.rpl$',
    [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
)
$AllReplayFiles = @(
    Get-ChildItem -LiteralPath $GameDir -File |
        Where-Object { $ReplayFilePattern.IsMatch($_.Name) } |
        Sort-Object -Property Name
)
$sampleCount = [int][math]::Ceiling(
    [long]$AllReplayFiles.Count * $RendererTestPercentage / 100.0
)

# Space samples evenly, then partition by sample position rather than replay
# counter. This keeps partition sizes within one replay of each other even
# when the sampling interval shares a factor with the partition count.
$ReplayFiles = @(
    for ($sampleIndex = $Partition; $sampleIndex -lt $sampleCount;
        $sampleIndex += $PartitionCount) {
        $replayIndex = [int][math]::Floor(
            [long]$sampleIndex * $AllReplayFiles.Count / $sampleCount
        )
        $AllReplayFiles[$replayIndex]
    }
)

Write-Output (
    'Renderer partition {0}: {1} of {2} sampled replays ({3}% of {4} eligible).' -f
    $Partition,
    $ReplayFiles.Count,
    $sampleCount,
    $RendererTestPercentage,
    $AllReplayFiles.Count
)

if ($ReplayFiles.Count -eq 0) {
    Write-Output 'No matching files found.'
    return
}

function Write-ReplayError {
    param(
        [Parameter(Mandatory)]
        [string]$Message
    )

    [System.IO.File]::AppendAllText(
        $OutputFile,
        $Message + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false)
    )
    Write-Error -Message $Message -ErrorAction Continue
}

function Invoke-DosBoxExecutable {
    param(
        [Parameter(Mandatory)]
        [string]$Executable,

        [Parameter(Mandatory)]
        [string]$FileName
    )

    $mountCommand = 'mount c "{0}"' -f $GameDir
    $runCommand = '{0} "{1}" 2 0' -f $Executable, $FileName
    $arguments = @(
        '-silent'
        '-conf'
        $Config
        '-c'
        $mountCommand
        '-c'
        'c:'
        '-c'
        $runCommand
        '-c'
        'exit'
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = if ($IsWindows) {
        'C:\DOSBox-x\dosbox-X.exe'
    }
    else {
        'dosbox-x'
    }
    $startInfo.WorkingDirectory = $GameDir
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    foreach ($argument in $arguments) {
        [void]$startInfo.ArgumentList.Add($argument)
    }

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    $started = $false
    $standardOutputTask = $null
    $standardErrorTask = $null

    try {
        $started = $process.Start()
        if (-not $started) {
            throw 'DOSBox-X did not start.'
        }

        $standardOutputTask = $process.StandardOutput.ReadToEndAsync()
        $standardErrorTask = $process.StandardError.ReadToEndAsync()

        if (-not $process.WaitForExit($TimeoutMilliseconds)) {
            try {
                $process.Kill($true)
            }
            catch {
                $process.Kill()
            }
            $process.WaitForExit()

            Write-ReplayError (
                "ERROR|type=timeout|exe=$Executable|input=$FileName|" +
                "timeout_seconds=$DosBoxTimeoutSeconds"
            )
            return $false
        }

        $exitCode = $process.ExitCode
        if ($exitCode -eq 0) {
            return $true
        }

        Write-ReplayError (
            "ERROR|type=dosbox_failure|exe=$Executable|input=$FileName|" +
            "exit_code=$exitCode"
        )
        return $false
    }
    catch {
        if ($started -and -not $process.HasExited) {
            try {
                $process.Kill($true)
            }
            catch {
                $process.Kill()
            }
            $process.WaitForExit()
        }

        $errorMessage = $_.Exception.Message -replace '[\r\n|]+', ' '
        Write-ReplayError (
            "ERROR|type=dosbox_failure|exe=$Executable|input=$FileName|" +
            "message=$errorMessage"
        )
        return $false
    }
    finally {
        if ($null -ne $standardOutputTask) {
            [void]$standardOutputTask.GetAwaiter().GetResult()
        }
        if ($null -ne $standardErrorTask) {
            [void]$standardErrorTask.GetAwaiter().GetResult()
        }
        $process.Dispose()
    }
}

function Test-FilesEqual {
    param(
        [Parameter(Mandatory)]
        [string]$LeftPath,

        [Parameter(Mandatory)]
        [string]$RightPath
    )

    $leftStream = $null
    $rightStream = $null

    try {
        $leftStream = [System.IO.File]::OpenRead($LeftPath)
        $rightStream = [System.IO.File]::OpenRead($RightPath)

        if ($leftStream.Length -ne $rightStream.Length) {
            return $false
        }

        [byte[]]$leftBuffer = [byte[]]::new(65536)
        [byte[]]$rightBuffer = [byte[]]::new(65536)

        while ($true) {
            $leftBytesRead = $leftStream.Read(
                $leftBuffer,
                0,
                $leftBuffer.Length
            )
            $rightBytesRead = $rightStream.Read(
                $rightBuffer,
                0,
                $rightBuffer.Length
            )

            if ($leftBytesRead -ne $rightBytesRead) {
                return $false
            }
            if ($leftBytesRead -eq 0) {
                return $true
            }

            for ($index = 0; $index -lt $leftBytesRead; $index++) {
                if ($leftBuffer[$index] -ne $rightBuffer[$index]) {
                    return $false
                }
            }
        }
    }
    finally {
        if ($null -ne $leftStream) {
            $leftStream.Dispose()
        }
        if ($null -ne $rightStream) {
            $rightStream.Dispose()
        }
    }
}

$total = $ReplayFiles.Count
$processed = 0

foreach ($replayFile in $ReplayFiles) {
    $processed++
    $pdoFile = Join-Path $GameDir ($replayFile.BaseName + '.PDO')
    $pdoPendingFile = "$pdoFile.pending"
    $pddFile = Join-Path $GameDir ($replayFile.BaseName + '.PDD')

    Write-Output (
        'Processing renderer replay {0}/{1}: {2}' -f
        $processed,
        $total,
        $replayFile.Name
    )

    # Prevent output from an earlier failed invocation from being accepted.
    if (Test-Path -LiteralPath $pddFile -PathType Leaf) {
        Remove-Item -LiteralPath $pddFile -Force
    }

    # Retain completed PDO files. A pending marker survives even if the worker
    # is killed, so interrupted oracle output is regenerated on the next run.
    if ((Test-Path -LiteralPath $pdoPendingFile -PathType Leaf) -or
        -not (Test-Path -LiteralPath $pdoFile -PathType Leaf)) {
        [System.IO.File]::WriteAllText($pdoPendingFile, '')
        if (Test-Path -LiteralPath $pdoFile -PathType Leaf) {
            Remove-Item -LiteralPath $pdoFile -Force
        }

        if (-not (Invoke-DosBoxExecutable 'pixldumo.exe' $replayFile.Name)) {
            continue
        }

        if (Test-Path -LiteralPath $pdoFile -PathType Leaf) {
            Remove-Item -LiteralPath $pdoPendingFile -Force
        }
    }

    if (-not (Invoke-DosBoxExecutable 'pixldump.exe' $replayFile.Name)) {
        continue
    }

    $outputsExist = $true
    foreach ($dumpFile in @($pdoFile, $pddFile)) {
        if (-not (Test-Path -LiteralPath $dumpFile -PathType Leaf)) {
            Write-ReplayError (
                "ERROR|type=missing_output|input=$($replayFile.Name)|" +
                "output=$([System.IO.Path]::GetFileName($dumpFile))"
            )
            $outputsExist = $false
        }
    }

    if ($outputsExist -and -not (Test-FilesEqual $pdoFile $pddFile)) {
        Write-ReplayError (
            "ERROR|type=file_mismatch|input=$($replayFile.Name)|" +
            "pdo=$([System.IO.Path]::GetFileName($pdoFile))|" +
            "pdd=$([System.IO.Path]::GetFileName($pddFile))"
        )
    }
}
