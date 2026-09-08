# Install the pinned Windows and Linux tools without running the GUI installer.
$ErrorActionPreference = 'Stop'
$pin = Get-Content -Raw (Join-Path $PSScriptRoot 'open-watcom.conf') | ConvertFrom-StringData
$repository = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$destination = Join-Path $repository 'tools/watcom'
$marker = Join-Path $destination 'restunts-toolchain.version'
$version = "$($pin.OW2_RELEASE) $($pin.OW2_SHA256)"
if ((Test-Path $marker) -and ((Get-Content -Raw $marker).Trim() -eq $version) -and
    (Test-Path (Join-Path $destination 'binnt/wcc.exe')) -and
    (Test-Path (Join-Path $destination 'binnt/wlink.exe'))) {
    Write-Host "Open Watcom 2 $($pin.OW2_RELEASE) is already installed in $destination"
    exit 0
}
if (Test-Path $destination) {
    throw "Refusing to replace $destination; move it aside before installing."
}

$staging = Join-Path $repository ("tools/.watcom-install." + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $staging | Out-Null
try {
    $archive = Join-Path $staging 'ow-snapshot.tar.xz'
    $url = "https://github.com/open-watcom/open-watcom-v2/releases/download/$($pin.OW2_RELEASE)/ow-snapshot.tar.xz"
    Invoke-WebRequest -Uri $url -OutFile $archive -UseBasicParsing
    if ((Get-FileHash -Algorithm SHA256 $archive).Hash -ne $pin.OW2_SHA256) {
        throw 'Open Watcom snapshot checksum does not match the pinned release.'
    }
    $extracted = Join-Path $staging 'watcom'
    New-Item -ItemType Directory -Path $extracted | Out-Null
    & tar -xJf $archive -C $extracted ./binnt ./binl64 ./h ./lib286 ./license.txt ./readme.txt
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not extract the Open Watcom snapshot; Windows tar.exe is required.'
    }
    foreach ($tool in @('binnt/wcc.exe', 'binnt/wlink.exe')) {
        if (-not (Test-Path (Join-Path $extracted $tool))) {
            throw "Open Watcom snapshot is missing $tool."
        }
    }
    Set-Content -Path (Join-Path $extracted 'restunts-toolchain.version') -Value $version -Encoding Ascii
    Move-Item -Path $extracted -Destination $destination
    Write-Host "Installed Open Watcom 2 $($pin.OW2_RELEASE) in $destination"
} finally {
    Remove-Item -Recurse -Force $staging
}
