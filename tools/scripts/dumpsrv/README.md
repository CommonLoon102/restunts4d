# REPLDUMP HTTP service

This PowerShell 7 service accepts a replacement `REPLDUMP.EXE`, runs the
existing replay comparison scripts, and returns `partitions_all.txt`.

## Directory layout

Keep the PowerShell scripts and DOSBox configuration in the service directory:

- `dumpsrv.ps1`
- `rpl2statemain.ps1`
- `rpl2state.ps1`
- `dosbox.proc.conf`

Create a `stunts` subdirectory beneath the service directory and put all
replay-processing inputs there:

- `stunts\REPLDUMO.EXE`
- the numeric `.rpl` input files
- all files for custom cars
- the rest of the files needed for Stunts

The request body is always written to `stunts\REPLDUMP.EXE`; no uploaded
filename is used. DOSBox mounts only the `stunts` subdirectory as drive `C:`,
so code running inside DOSBox cannot access the PowerShell scripts or the
DOSBox configuration in the parent directory. The per-partition
`partition_<n>.txt` files and combined `partitions_all.txt` result are also
written to the parent service directory, outside the DOSBox mount.

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
each DOSBox execution and defaults to 30 seconds:

```powershell
$env:DUMPSRV_API_KEY = 'replace-with-a-long-random-secret'
./dumpsrv.ps1 `
    -PartitionCount 12 `
    -Port 8080 `
    -DosBoxTimeoutSeconds 30
```

The service listens on all local interfaces. On Windows, `HttpListener` may
require a one-time URL reservation from an elevated prompt:

## Call the service

The one endpoint is `POST /process`. Its request body is the raw executable,
not `multipart/form-data`.

```powershell
$headers = @{ 'X-API-Key' = $env:DUMPSRV_API_KEY }
Invoke-WebRequest `
    -Uri 'http://server-name:8080/process' `
    -Method Post `
    -Headers $headers `
    -ContentType 'application/octet-stream' `
    -InFile './REPLDUMP.EXE' `
    -OutFile './partitions_all.txt' `
    -TimeoutSec 2100
```

### Shell client

Copy `dumpsrv-client.sh` to the client machine and place `REPLDUMP.EXE` beside
it. Set the same API key used by the server, then pass the full endpoint URL:

```sh
chmod +x dumpsrv-client.sh
export DUMPSRV_API_KEY='replace-with-the-server-secret'
./dumpsrv-client.sh http://server-name:8080/process
```

Alternatively, set `DUMPSRV_URL` and run the script without an argument. A
successful response is saved beside the script as `partitions_all.txt`. If the
request fails, an existing result file is left unchanged.

The processing script has a 30-minute limit. Only one job can run at a time;
an authenticated request received while it is running gets `503 Service
Unavailable` with `Retry-After: 60`.

Other error responses are:

- `400 Bad Request` for an empty body
- `401 Unauthorized` for a missing or incorrect API key
- `404 Not Found` for another path
- `405 Method Not Allowed` for a non-POST request
- `413 Payload Too Large` for an upload larger than 1 MiB
- `500 Internal Server Error` if processing fails or the result is missing
- `504 Gateway Timeout` after 30 minutes of processing

After a successful script exit and successful response, every `.bni` file in
the `stunts` directory and every `.txt` file in the service directory is
deleted non-recursively. These files are not deleted on failure, timeout,
missing output, or a failed response. Existing `.bin` files are reused because
`REPLDUMO.EXE` output does not change between requests.

This service uses plain HTTP. The API key prevents unauthenticated use, but it
is visible to anyone able to capture traffic on the local network.
