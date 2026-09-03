# Replay regression HTTP service

This PowerShell 7 service accepts replacement `REPLDUMP.EXE` and
`PIXLDUMP.EXE` files, runs optional physics and renderer regression tests,
and returns the combined `partitions_all.txt` report.

## Directory layout

Keep the PowerShell scripts and DOSBox configuration in the service directory:

- `dumpsrv.ps1`
- `rpl2statemain.ps1`
- `rpl2state.ps1`
- `rpl2pixdumpmain.ps1`
- `rpl2pixdump.ps1`
- `dosbox.proc.conf`

Create a `stunts` subdirectory beneath the service directory and put all
replay-processing inputs there:

- `stunts\REPLDUMO.EXE`
- the numeric `.rpl` input files
- the `.PDX` renderer reference files
- all files for custom cars
- the rest of the files needed for Stunts

The uploaded executable parts are always written to `stunts\REPLDUMP.EXE`
and `stunts\PIXLDUMP.EXE`; uploaded filenames are not used. Both executables
must be included even when either test type is disabled for a request.
DOSBox mounts only the `stunts` subdirectory as drive `C:`, so code running
inside DOSBox cannot access the PowerShell scripts or the DOSBox configuration
in the parent directory. The per-partition `partition_<n>.txt` files and
combined `partitions_all.txt` result are written to the parent service
directory, outside the DOSBox mount.

Renderer reference files must end in a four-digit counter and use the `.PDX`
extension. The part before `.PDX` is the replay base name, so `0000.PDX`
requires `0000.rpl` (case-insensitive). A `.PDX` file has the same contents as
a normal `.PDD` hash dump; the different extension prevents `PIXLDUMP.EXE`
from overwriting the reference.

## First time setup

```powershell
Get-NetConnectionProfile
```

Then use the index from previous command:

```powershell
Set-NetConnectionProfile -InterfaceIndex <index> -NetworkCategory Private
```

The Windows Firewall must also allow inbound TCP traffic on the selected port.

```powershell
netsh http add urlacl url=http://+:8080/ user="$env:USERDOMAIN\$env:USERNAME"

New-NetFirewallRule `
    -DisplayName "REPLDUMP HTTP service" `
    -Direction Inbound `
    -Action Allow `
    -Protocol TCP `
    -LocalPort 8080 `
    -Profile Private `
    -RemoteAddress LocalSubnet
```

## Start the service

Set a long, random API key and choose the partition count expected by
`rpl2statemain.ps1`. `DosBoxTimeoutSeconds` is the maximum time allowed for
each DOSBox execution and defaults to 60 seconds:

```powershell
$env:DUMPSRV_API_KEY = 'replace-with-a-long-random-secret'
./dumpsrv.ps1 `
    -PartitionCount 12 `
    -Port 8080 `
    -DosBoxTimeoutSeconds 60
```

The service listens on all local interfaces. On Windows, `HttpListener` may
require a one-time URL reservation from an elevated prompt:

## Call the service

The one endpoint is `POST /process`. Its request body is
`multipart/form-data` with these fields:

| Field | Required | Description |
| --- | --- | --- |
| `repldump` | yes | Replacement `REPLDUMP.EXE`; maximum 1 MiB. |
| `pixldump` | yes | Replacement `PIXLDUMP.EXE`; maximum 1 MiB. |
| `physics_tests` | no | `true` or `false`; defaults to `true`. |
| `renderer_tests` | no | `true` or `false`; defaults to `true`. |

At least one test type must be enabled. A request that sets both optional
fields to `false` receives `400 Bad Request`.

This PowerShell 7 example runs both physics and renderer tests:

```powershell
$headers = @{ 'X-API-Key' = $env:DUMPSRV_API_KEY }
$form = @{
    repldump = Get-Item './REPLDUMP.EXE'
    pixldump = Get-Item './PIXLDUMP.EXE'
    physics_tests = 'true'
    renderer_tests = 'true'
}
Invoke-WebRequest `
    -Uri 'http://server-name:8080/process' `
    -Method Post `
    -Headers $headers `
    -Form $form `
    -OutFile './partitions_all.txt' `
    -TimeoutSec 2100
```

### Shell client

Copy `dumpsrv-client.sh` to the client machine and place `REPLDUMP.EXE` and
`PIXLDUMP.EXE` beside it. Set the same API key used by the server, then pass
the full endpoint URL:

```sh
chmod +x dumpsrv-client.sh
export DUMPSRV_API_KEY='replace-with-the-server-secret'
./dumpsrv-client.sh http://server-name:8080/process
```

Physics and renderer tests run by default. Use either optional client switch
to skip that test type for a request while still uploading both executables:

```sh
./dumpsrv-client.sh \
    --physics-tests false \
    http://server-name:8080/process

./dumpsrv-client.sh \
    --renderer-tests false \
    http://server-name:8080/process
```

The `--physics-tests=true|false` and `--renderer-tests=true|false` forms are
also accepted. Options may appear before or after the URL. Setting both
switches to `false` sends a request that the service rejects with
`400 Bad Request`.

Alternatively, set `DUMPSRV_URL` and run the script without an argument. A
successful response is saved beside the script as `partitions_all.txt`. If the
request fails, an existing result file is left unchanged.

## Processing and results

If `physics_tests` is true, the state comparison runs first using the existing
replay partitions. If `renderer_tests` is true, renderer partitions then
process every matching `.PDX` reference. For each reference, `PIXLDUMP.EXE`
runs with camera `2` (F2) and target `0` (player), producing a `.PDD` file that
is compared byte for byte with the `.PDX` file.

Physics and renderer diagnostics use the same one-line format and are sorted
by the `input` field in the returned report. Renderer examples include:

```text
ERROR|type=missing_input|input=0000.rpl|reference=0000.PDX
ERROR|type=missing_output|input=0000.rpl|output=0000.PDD
ERROR|type=file_mismatch|input=0000.rpl|pdx=0000.PDX|pdd=0000.PDD
```

An empty `partitions_all.txt` means that all enabled comparisons matched and
no processing errors were reported.

The processing script has a 30-minute limit. Only one job can run at a time;
an authenticated request received while it is running gets `503 Service
Unavailable` with `Retry-After: 60`.

Other error responses are:

- `400 Bad Request` for an empty or malformed multipart request, a missing
  executable field, an invalid test setting, or both test settings being
  `false`
- `401 Unauthorized` for a missing or incorrect API key
- `404 Not Found` for another path
- `405 Method Not Allowed` for a non-POST request
- `413 Payload Too Large` if either executable exceeds 1 MiB or the complete
  multipart request exceeds 2 MiB plus 64 KiB of overhead
- `500 Internal Server Error` if processing fails or the result is missing
- `504 Gateway Timeout` after 30 minutes of processing

After successful processing and a successful response, every `.bni` and
generated `.pdd` file in the `stunts` directory and every `.txt` file in the
service directory is deleted non-recursively. These files are not deleted on
failure, timeout, missing output, or a failed response. Existing `.bin` files
are reused because `REPLDUMO.EXE` output does not change between requests.

This service uses plain HTTP. The API key prevents unauthenticated use, but it
is visible to anyone able to capture traffic on the local network.
