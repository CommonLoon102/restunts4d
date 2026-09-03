#requires -Version 7.0

<#
.SYNOPSIS
Runs the requested number of rpl2pixdump.ps1 partitions concurrently and
merges their errors into the existing state comparison report.

.EXAMPLE
.\rpl2pixdumpmain.ps1 -PartitionCount 12
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)]
    [ValidateRange(1, 64)]
    [int]$PartitionCount,

    [Parameter()]
    [ValidateRange(1, 2147483)]
    [int]$DosBoxTimeoutSeconds = 60
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

$scriptDirectory = $PSScriptRoot
$stuntsDirectory = Join-Path $scriptDirectory 'stunts'
$workerScript = Join-Path $scriptDirectory 'rpl2pixdump.ps1'
if (-not (Test-Path -LiteralPath $workerScript -PathType Leaf)) {
    throw "Worker script not found: $workerScript"
}

if (-not (Test-Path -LiteralPath $stuntsDirectory -PathType Container)) {
    throw "Stunts directory not found: $stuntsDirectory"
}

$jobs = @()
$failedJobs = @()

try {
    for ($partition = 0; $partition -lt $PartitionCount; $partition++) {
        $jobParameters = @{
            Name = "rpl2pixdump-$partition"
            ScriptBlock = {
                param(
                    [string]$WorkerScript,
                    [int]$Partition,
                    [int]$PartitionCount,
                    [int]$DosBoxTimeoutSeconds
                )

                $workerParameters = @{
                    Partition = $Partition
                    PartitionCount = $PartitionCount
                    DosBoxTimeoutSeconds = $DosBoxTimeoutSeconds
                }
                & $WorkerScript @workerParameters
            }
            ArgumentList = @(
                $workerScript
                $partition
                $PartitionCount
                $DosBoxTimeoutSeconds
            )
        }

        $jobs += Start-Job @jobParameters
        Write-Output (
            'Started renderer partition {0} of {1}.' -f
            $partition,
            $PartitionCount
        )
    }

    do {
        foreach ($job in $jobs) {
            Receive-Job -Job $job -ErrorAction Continue
        }

        $unfinishedJobs = @(
            $jobs | Where-Object {
                $_.State -notin @('Completed', 'Failed', 'Stopped')
            }
        )

        if ($unfinishedJobs.Count -gt 0) {
            Start-Sleep -Milliseconds 200
        }
    } while ($unfinishedJobs.Count -gt 0)

    foreach ($job in $jobs) {
        Receive-Job -Job $job -ErrorAction Continue
    }

    $failedJobs = @(
        $jobs | Where-Object { $_.State -eq 'Failed' }
    )
}
finally {
    if ($jobs.Count -gt 0) {
        Remove-Job -Job $jobs
    }
}

$combinedOutputFile = Join-Path $scriptDirectory 'partitions_all.txt'
$uniqueLines = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::Ordinal
)
$entries = [System.Collections.Generic.List[object]]::new()
$sequence = 0

for ($partition = 0; $partition -lt $PartitionCount; $partition++) {
    $partitionFile = Join-Path $scriptDirectory "partition_$partition.txt"
    if (-not (Test-Path -LiteralPath $partitionFile -PathType Leaf)) {
        continue
    }

    foreach ($line in [System.IO.File]::ReadLines($partitionFile)) {
        if (-not $uniqueLines.Add($line)) {
            continue
        }

        $inputName = ''
        $inputMatch = [regex]::Match($line, '(?:^|\|)input=([^|]*)')
        if ($inputMatch.Success) {
            $inputName = $inputMatch.Groups[1].Value
        }

        $entries.Add([pscustomobject]@{
            InputName = $inputName
            Sequence = $sequence
            Line = $line
        })
        $sequence++
    }
}

$sortedLines = @(
    $entries |
        Sort-Object -Property InputName, Sequence |
        ForEach-Object { $_.Line }
)
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllLines(
    $combinedOutputFile,
    [string[]]$sortedLines,
    $utf8NoBom
)

if ($failedJobs.Count -gt 0) {
    $failedNames = $failedJobs.Name -join ', '
    throw "One or more renderer jobs failed: $failedNames"
}

$stopwatch.Stop()
$elapsedMinutes = [long][math]::Floor($stopwatch.Elapsed.TotalMinutes)
Write-Output (
    'Elapsed time: {0}:{1:D2}' -f
    $elapsedMinutes,
    $stopwatch.Elapsed.Seconds
)
