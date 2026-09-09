# Replay regression service and runner

`tools/scripts/dumpsrv` is one C# application for Linux, Windows, and GitHub Actions.
Its HTTP service and direct runner share replay discovery, sampling, task
scheduling, DOSBox execution, oracle caching, comparisons, and reporting.
The shell client continues to use the same HTTP endpoint and report format.

## Build and deployment

Install the .NET 10 SDK to build and test the application. From the repository
root, publish the portable application:

```sh
dotnet publish tools/scripts/dumpsrv/dumpsrv/dumpsrv.csproj --configuration Release --output out/dumpsrv
```

Copy the complete publish directory to the service machine. The same published
files work on Linux and Windows with the .NET 10 ASP.NET Core Runtime installed.
The publish directory includes `dosbox.proc.conf`; install DOSBox-X separately.
Set `DUMPSRV_DOSBOX_PATH` to an explicit DOSBox-X executable path when needed;
otherwise it uses `C:\DOSBox-x\dosbox-X.exe` on Windows if present, then
`dosbox-x` from `PATH`.

Create a `stunts` directory alongside `dumpsrv.dll` and put the processing inputs
there:

- `repldumo.exe` and `pixldumo.exe`, the original physics and renderer tools.
- The `.rpl` replay corpus, all required custom-car files, and other Stunts data.

The service writes uploads to `stunts/repldump.exe` and `stunts/pixldump.exe`.
The `repldump` upload must be named exactly `repldump.exe`, and the `pixldump`
upload exactly `pixldump.exe`, all lowercase. Missing or different filenames,
including uppercase, mixed case, paths, or swapped names, return HTTP `400` before either
executable is saved. Both `filename` and `filename*` are checked when supplied.
DOSBox mounts only this game directory as drive `C:`, keeping service binaries,
configuration, and reports outside the mount.
The direct runner accepts an explicit game directory and requires all four
physics and renderer executables there when both phases are enabled.

## Start the service

Set a long, random API key and choose the number of concurrent workers. These
examples start the same application on either platform.

Linux:

```sh
export DUMPSRV_API_KEY='replace-with-a-long-random-secret'
dotnet out/dumpsrv/dumpsrv.dll serve \
    -PartitionCount 12 -Port 8080 \
    -DosBoxTimeoutSeconds 60 -RendererTestPercentage 5 \
    -ResponseProcessingTimeoutSeconds 1800
```

Windows PowerShell:

```powershell
$env:DUMPSRV_API_KEY = 'replace-with-a-long-random-secret'
dotnet out/dumpsrv/dumpsrv.dll serve `
    -PartitionCount 12 -Port 8080 `
    -DosBoxTimeoutSeconds 60 -RendererTestPercentage 5 `
    -ResponseProcessingTimeoutSeconds 1800
```

| Parameter | Default | Meaning |
| --- | --- | --- |
| `ApiKey` | `DUMPSRV_API_KEY` | Secret required in the `X-API-Key` request header. |
| `PartitionCount` | Required | Number of concurrent workers, from 1 through 64. |
| `Port` | `8080` | HTTP port, from 1 through 65535. |
| `DosBoxTimeoutSeconds` | `60` | Positive time limit for each DOSBox execution. |
| `RendererTestPercentage` | `100` | Whole-number renderer coverage, from 1 through 100. |
| `ResponseProcessingTimeoutSeconds` | `1800` | Positive overall processing limit, in seconds. |

The `serve` command is optional; named service parameters can follow
`dumpsrv.dll` directly.

`ResponseProcessingTimeoutSeconds` is accepted only at service startup. HTTP
requests cannot set or override it.

The processing timeout starts after validated uploads are saved and covers the
enabled test phases and report generation together. It is separate from the
timeout for an individual DOSBox execution. A processing timeout
cancels the workers, forcibly terminates their DOSBox processes, and returns
HTTP `504`. Configure the client's timeout slightly above this limit to allow
for the upload and response transfer.

Service console lines begin with the request's elapsed processing time in
`[HH:mm:ss]` format. Each request starts its own timer after validated uploads
are saved. Immediately before sending the response, the service prints the
individual durations of the requested physics and renderer phases, followed by
its total processing time including discovery and report generation. Phase
durations measure elapsed time across all parallel workers:

```text
[00:00:00] Processing requested phases: physics and renderer.
[00:00:12] Processing physics replay: race.rpl
[00:02:08] Physics phase took 00:00:45.
[00:02:08] Renderer phase took 00:01:22.
[00:02:08] Processing requested phases (physics and renderer) took 00:02:08.
```

Disabled phases are omitted. If processing is interrupted, the summary includes
time spent in any started phase and identifies requested phases that did not start.
The final duration excludes response transmission. Startup messages and requests
that have not started processing show `[00:00:00]`. Elapsed prefixes appear only
in the console; the returned report retains its existing format.

The service listens on all local interfaces using ASP.NET Core Kestrel. Windows
HTTP URL reservations are not required. Allow inbound traffic on the selected
port in the host firewall. For example, in an elevated Windows PowerShell
session, allow the private local subnet:

```powershell
New-NetFirewallRule `
    -DisplayName 'Replay regression service' `
    -Direction Inbound -Action Allow -Protocol TCP -LocalPort 8080 `
    -Profile Private -RemoteAddress LocalSubnet
```

## Replay selection and scheduling

Every top-level file with a case-insensitive `.rpl` extension is discovered.
Filenames do not need a numeric suffix. The existing DOS executables still
require DOS-compatible basenames of 1 through 8 characters. Unsupported names,
reserved DOS device names, and case-insensitive name collisions are reported
as errors instead of being skipped. The application sorts the corpus once
using ordinal ordering to make repeated runs deterministic. Physics tests use
the entire corpus. Renderer tests select
`ceiling(replay count * percentage / 100)` evenly spaced entries from that same
complete list before any work is assigned.

The selected lists are distributed round robin across shards and then round
robin into each shard's worker lists. Each worker processes its list in order,
and workers run as asynchronous C# tasks. No filename counter, hash, or numeric
suffix controls assignment. Partition and shard sizes differ by at most one
replay. Selection is independent of the number of shards or workers.

The service uses one shard. GitHub Actions assigns one shard to each matrix job;
all jobs use the same complete replay corpus and renderer percentage. Physics
runs first, followed by rendering. A comparison error does not stop the other
replays or the remaining enabled phase.

## Call the service

The endpoint is `POST /process`, authenticated with `X-API-Key`. Its
`multipart/form-data` fields are unchanged:

| Field | Required | Description |
| --- | --- | --- |
| `repldump` | Yes | Upload filename `repldump.exe`; maximum 1 MiB. |
| `pixldump` | Yes | Upload filename `pixldump.exe`; maximum 1 MiB. |
| `physics_tests` | No | `true` or `false`; defaults to `true`. |
| `renderer_tests` | No | `true` or `false`; defaults to `true`. |

Both executables must be uploaded even if one test type is disabled. At least
one test type must be enabled. A completed run returns HTTP `200` and a UTF-8
`partitions_all.txt` attachment. Comparison errors are recorded in the report;
an empty report means all enabled comparisons passed.

Copy `dumpsrv-client.sh`, `repldump.exe`, and `pixldump.exe` into the same client
directory, using these lowercase filenames. Set the API key, then pass the
endpoint URL:

```sh
export DUMPSRV_API_KEY='replace-with-the-server-secret'
./dumpsrv-client.sh http://server-name:8080/process
```

Alternatively, set `DUMPSRV_URL` and omit the URL argument. Client options may
appear before or after the URL:

```sh
./dumpsrv-client.sh --physics-tests false http://server-name:8080/process
./dumpsrv-client.sh --renderer-tests=false http://server-name:8080/process
./dumpsrv-client.sh --timeout-seconds 3660 http://server-name:8080/process
./dumpsrv-client.sh http://server-name:8080/process --timeout-seconds=3660
```

Both `--physics-tests` and `--renderer-tests` accept `true` or `false`, either
as the next argument or after `=`. `--timeout-seconds` accepts a positive
integer in either form and defaults to `1860` seconds. It controls only how
long the client waits and is never sent as a request field. The connection timeout
remains 10 seconds. The client saves a successful response beside the script as
`partitions_all.txt`; a failed request leaves an existing result unchanged.

Only one authenticated request runs at a time. Other authenticated requests
receive `503 Service Unavailable` with `Retry-After: 60`.

Other response codes are:

- `400`: empty or malformed multipart body, missing executable, invalid test
  setting, or both tests disabled.
- `401`: missing or incorrect API key.
- `404`: another path.
- `405`: a non-POST request to `/process`.
- `413`: an executable exceeds 1 MiB, or the total body exceeds 2 MiB plus 64 KiB.
- `500`: processing could not complete or its report could not be produced.
- `504`: the configured overall processing timeout expired.

## Direct execution and CI reports

The `run` command uses the same engine without HTTP. For example, from the
repository root:

```sh
dotnet out/dumpsrv/dumpsrv.dll run \
    -GameDirectory stunts -OutputDirectory results \
    -DosBoxConfigPath tools/scripts/dosbox.proc.conf \
    -PartitionCount 12 -DosBoxTimeoutSeconds 30 \
    -RendererTimeoutSeconds 120 -RendererTestPercentage 5
```

`ShardIndex` defaults to `0` and `ShardCount` to `1`. For a distributed run, pass
both explicitly and retain the full replay corpus on every shard. `PhysicsTests`
and `RendererTests` default to `true`; use `-PhysicsTests false` or
`-RendererTests false` to disable a phase. `DosBoxConfigPath` defaults to the
configuration alongside the application. Progress is written to the console.

Each run writes `shard-<index>.json` with shard identity, completed replay names,
and diagnostics, plus a local `partitions_all.txt` report. `run` returns zero
only when the assigned work finishes without errors. Copy every shard's JSON
result into one directory, then merge against the complete replay corpus:

```sh
dotnet out/dumpsrv/dumpsrv.dll merge \
    -ReplayDirectory stunts -ResultsDirectory results \
    -ShardCount 1 -RendererTestPercentage 5 \
    -OutputFile partitions_all.txt
```

Pass the same shard count, percentage, and enabled test phases used by `run`.
`merge` reads shard identity from the JSON content, checks actual completed
replay names against expected coverage, and rejects missing or duplicate shards.
It writes the text report even when validation fails and returns nonzero for
errors or incomplete coverage. Optional `-SummaryFile PATH` writes the Markdown
summary used by GitHub Actions.

CI uploads each shard's JSON as `partitions-<index>` and the combined text report
as `partitions_all`, including diagnostics when validation fails. The summary
shows physics and renderer coverage separately, errors grouped by type, and up
to the first 200 diagnostic lines. The existing DOS executable build and
`restunts-exes` artifact remain separate from the C# regression runner.

## Cached outputs and diagnostics

Physics compares original `.BIN` output against fresh `.BNI` output. Rendering
uses camera `2` and target `0`, comparing original `.PDO` output against fresh
`.PDD` output. Comparisons are byte for byte.

Completed `.BIN` and `.PDO` files are reused only after checking their contents
against the replay's recorded frame count. Physics dumps must contain the
matching two-byte frame count and exactly 1,120 bytes per frame. Renderer dumps
must contain every CRLF-terminated MD5 sample at frames 0, 5, 10, ... through
the last sampled frame. Empty, truncated, or malformed caches are regenerated,
including files left by older runners without a pending marker.

A `.BIN.pending` or `.PDO.pending` marker is written before oracle generation
and removed only after successful execution produces a complete output.
An interrupted oracle dump is regenerated on the next run. Fresh oracle and
candidate outputs undergo the same completeness checks; invalid output is
reported as `type=invalid_output` instead of a replay desync or a successful
comparison of two partial files. Candidate outputs are removed before execution
so stale files cannot hide failures. DOSBox uses `core=dynamic` and `cycles=max`;
cancellation and execution timeouts kill the process tree forcibly.

Diagnostics retain the existing one-line format. Duplicate lines are removed
and results are ordered by the `input` field:

```text
ERROR|type=timeout|exe=pixldump.exe|input=0000.rpl|timeout_seconds=60
ERROR|type=missing_output|input=0000.rpl|output=0000.PDD
ERROR|type=file_mismatch|input=0000.rpl|pdo=0000.PDO|pdd=0000.PDD
```

After successfully sending a report, the service removes generated candidate
outputs, incomplete oracle outputs, and report files owned by that request.
Completed oracle caches are retained. If processing or response delivery fails,
diagnostics and generated outputs are retained for investigation.

## Development checks

```sh
dotnet test tools/scripts/dumpsrv/dumpsrv.slnx --configuration Release
dotnet format tools/scripts/dumpsrv/dumpsrv.slnx --verify-no-changes
```

CI runs both commands on Linux and Windows. The HTTP and engine tests use
controlled process fixtures so they can exercise failures, cancellation, and
coverage without requiring DOSBox for every test. On Linux, also run the POSIX
client tests; CI runs them on its Linux job:

```sh
python3 tools/scripts/dumpsrv/dumpsrv.tests/client-tests.py
```
