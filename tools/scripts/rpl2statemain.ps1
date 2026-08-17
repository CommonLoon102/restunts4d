#requires -Version 7.0

<#
.SYNOPSIS
Runs the requested number of rpl2state.ps1 replay partitions concurrently.

.EXAMPLE
.\rpl2statemain.ps1 -PartitionCount 12
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)]
    [ValidateRange(1, 64)]
    [int]$PartitionCount
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

$workerScript = Join-Path $PSScriptRoot 'rpl2state.ps1'
if (-not (Test-Path -LiteralPath $workerScript -PathType Leaf)) {
    throw "Worker script not found: $workerScript"
}

$jobs = @()
$failedJobs = @()

try {
    for ($partition = 0; $partition -lt $PartitionCount; $partition++) {
        $jobParameters = @{
            Name = "rpl2state-$partition"
            ScriptBlock = {
                param(
                    [string]$WorkerScript,
                    [int]$Partition,
                    [int]$PartitionCount
                )

                $workerParameters = @{
                    Partition = $Partition
                    PartitionCount = $PartitionCount
                }
                & $WorkerScript @workerParameters
            }
            ArgumentList = @(
                $workerScript
                $partition
                $PartitionCount
            )
        }

        $jobs += Start-Job @jobParameters
        Write-Output (
            'Started replay partition {0} of {1}.' -f
            $partition,
            $PartitionCount
        )
    }

    # Receive each job's unread output while the workers are still running.
    # Without this loop, Start-Job buffers all progress until Wait-Job returns.
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

    # Drain output produced between the last polling pass and job completion.
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

$combinedOutputFile = Join-Path $PSScriptRoot 'partitions_all.txt'
$uniqueLines = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::Ordinal
)
$entries = [System.Collections.Generic.List[object]]::new()
$sequence = 0

for ($partition = 0; $partition -lt $PartitionCount; $partition++) {
    $partitionFile = Join-Path $PSScriptRoot "partition_$partition.txt"
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
    throw "One or more replay jobs failed: $failedNames"
}

$stopwatch.Stop()
$elapsedMinutes = [long][math]::Floor($stopwatch.Elapsed.TotalMinutes)
Write-Output (
    'Elapsed time: {0}:{1:D2}' -f
    $elapsedMinutes,
    $stopwatch.Elapsed.Seconds
)
