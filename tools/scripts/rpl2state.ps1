#requires -Version 7.0

<#
.SYNOPSIS
Generates or reuses .BIN files and compares them with .BNI files for one
replay partition.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)]
    [ValidateRange(0, 63)]
    [int]$Partition,

    [Parameter(Mandatory, Position = 1)]
    [ValidateRange(1, 64)]
    [int]$PartitionCount,

    [Parameter()]
    [ValidateRange(1, 2147483)]
    [int]$DosBoxTimeoutSeconds = 30
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

# Target filenames end with a four-digit counter, such as 0000.rpl or
# sl0000.RPL. Calculate each file's partition from that counter at runtime.
$ReplayFilePattern = [regex]::new(
    '([0-9]{4})\.rpl$',
    [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
)
$ReplayFiles = @(
    Get-ChildItem -LiteralPath $GameDir -File |
        Where-Object {
            $suffixMatch = $ReplayFilePattern.Match($_.Name)
            $suffixMatch.Success -and
            ([long]$suffixMatch.Groups[1].Value % $PartitionCount) -eq
                $Partition
        } |
        Sort-Object -Property Name
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
    $runCommand = '{0} "{1}" 1' -f $Executable, $FileName
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
    $startInfo.FileName = 'C:\DOSBox-x\dosbox-X.exe'
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

        # Drain both streams asynchronously so a full output buffer cannot
        # block DOSBox-X while its output remains hidden.
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

foreach ($file in $ReplayFiles) {
    $processed++
    $fileName = $file.Name
    $basePath = Join-Path $file.DirectoryName $file.BaseName
    $binFile = "$basePath.BIN"
    $bniFile = "$basePath.BNI"

    Write-Output ('Processing {0}/{1}: {2}' -f $processed, $total, $fileName)

    # REPLDUMPO.EXE produces stable output, so retain and reuse an existing BIN.
    if (-not (Test-Path -LiteralPath $binFile -PathType Leaf)) {
        if (-not (Invoke-DosBoxExecutable 'repldumo.exe' $fileName)) {
            continue
        }
    }

    if (-not (Invoke-DosBoxExecutable 'repldump.exe' $fileName)) {
        continue
    }

    $binExists = Test-Path -LiteralPath $binFile -PathType Leaf
    $bniExists = Test-Path -LiteralPath $bniFile -PathType Leaf

    if (-not $binExists) {
        Write-ReplayError (
            "ERROR|type=missing_output|input=$fileName|" +
            "output=$([System.IO.Path]::GetFileName($binFile))"
        )
    }

    if (-not $bniExists) {
        Write-ReplayError (
            "ERROR|type=missing_output|input=$fileName|" +
            "output=$([System.IO.Path]::GetFileName($bniFile))"
        )
    }

    if ($binExists -and $bniExists -and
        -not (Test-FilesEqual $binFile $bniFile)) {
        Write-ReplayError (
            "ERROR|type=file_mismatch|input=$fileName|" +
            "bin=$([System.IO.Path]::GetFileName($binFile))|" +
            "bni=$([System.IO.Path]::GetFileName($bniFile))"
        )
    }
}
