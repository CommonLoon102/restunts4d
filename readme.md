# Restunts - The Stunts reverse engineering project

https://wiki.stunts.hu/wiki/Restunts

Main repository: https://github.com/4d-stunts/restunts

## Repository contents:
	docs
		Various technical docs related to (re)stunts itself.

	src\restunts
		Project directory containing disassembly, ported c and makefiles to
		produce various executables based on the (re)stunts code.

	stunts
		Broderbund Stunts 1.1, the game.

	tools
		Contains setup scripts for Open Watcom 2 and bundled Windows build and
		testing tools.


### Contents of src\restunts:

	src\restunts\asmorig
		Contains asm code from the disassembled original exe.

	src\restunts\c
		Contains c functions ported from the disassembly.

	src\restunts\dos
		Makefile to build restunts for DOS.
		
	src\restunts\platform\dos
		DOS specific code.

	src\restunts\repldump
		Tool based on the original game code, loads replays and dumps the game
		state contents at each frame in a file for further analysis.

	src\restunts\pixldump
		Replay-driven renderer test tool. It hashes the selected 320x200 camera
		framebuffer at frame 0 and every fifth frame thereafter.


## C coding style

Project `.c` and `.h` files under `src/` use tabs with a width of four columns,
a 100-column target, K&R braces (function opening braces on their own line),
and a required braced body for every `if`, `else`, `for`, `while`, and `do`.
Conventional `else if` chains are allowed. Empty loops also need braces. Keep
CRLF line endings, as required by `.gitattributes`.

C sources use the C99 features supported by Open Watcom (`-zastd=c99`).
Declare local variables close to their first use, combining the declaration
and first assignment when possible. Keep declarations in the enclosing scope
when values are shared across branches or loops, and preserve initialization
order and object lifetime.

Two standard tools check the style directly:

- [editorconfig-checker](https://github.com/editorconfig-checker/editorconfig-checker)
  reads the whitespace rules in `.editorconfig`. Its Python package installs
  the `ec` command. `.editorconfig-checker.json` limits discovery to `src/`
  and permits alignment spaces after indentation tabs. Only C/H files have
  EditorConfig rules.
- [clang-format](https://clang.llvm.org/docs/ClangFormat.html) reads C layout
  and brace rules from `.clang-format`. Keep its whitespace settings consistent
  with `.editorconfig`. Include order is preserved.

Install the pinned tools once in a Python virtual environment:

```sh
python3 -m venv .venv
# Linux/macOS:
. .venv/bin/activate
# Windows cmd.exe: .venv\Scripts\activate.bat
python -m pip install -r tools/scripts/requirements-format.txt
```

From the repository root, check against `.editorconfig` with one command:

```sh
ec
```

Check C formatting too (Bash or Git Bash):

```sh
git ls-files -z 'src/*.c' 'src/*.h' | xargs -0 -r clang-format --dry-run --Werror
```

Format all tracked project C/H files:

```sh
git ls-files -z 'src/*.c' 'src/*.h' | xargs -0 -r clang-format -i
```

For an individual file, use `clang-format -i path/to/file.c`. Add new files to
Git before running the commands based on `git ls-files`. CI runs both checks
for pull requests and before releases. No project-specific formatter or checker
script is needed.

Braces are required even where clang-format cannot insert them automatically,
including macro bodies, empty loops, and bodies spanning preprocessor
branches. Review those cases manually; a successful clang-format check does
not prove that those cases have braces.

## C# coding style

The regression application and its tests under `tools/scripts/dumpsrv` use four-space
indentation, opening braces on their own line, braced control-flow bodies, a
100-column target, and CRLF line endings. `.editorconfig` defines these rules.
Use the .NET 10 SDK to check or apply formatting:

```sh
dotnet format tools/scripts/dumpsrv/dumpsrv.slnx --verify-no-changes
dotnet format tools/scripts/dumpsrv/dumpsrv.slnx
```

## Complexity audit

See [the complexity report](docs/complexity.md) for measurements, completed
refactors and audit results.

## How to build

The DOS compiler, assembler, and linker are Open Watcom 2, pinned to the official
[2026-09-01 build](https://github.com/open-watcom/open-watcom-v2/releases/tag/2026-09-01-Build).
The setup scripts verify its SHA256 and install into ignored `tools/watcom/`.
The shared release pin is `tools/scripts/open-watcom.conf`. WCC, WASM, WLINK,
and GNU Make 4.3 or newer run natively on Linux or Windows. The standard build
does not require Wine or DOSBox. Python 3.9 or newer is also required for the
`*-original` targets, which prepare assembler-compatible copies of the
preserved original sources.

### On Windows

1. Install the toolchain from PowerShell at the repository root (Windows 10/11
   `tar.exe` and PowerShell are required):

   ```powershell
   powershell -ExecutionPolicy Bypass -File tools\scripts\install-open-watcom.ps1
   ```

2. In cmd.exe at the repository root, run:

   ```text
   cd src\restunts
   setpath
   make restunts repldump pixldump
   ```

3. To build the original game and both dump tools from source as
   `stunts/restunto.exe`, `stunts/repldumo.exe`, and `stunts/pixldumo.exe`,
   install Python 3.9 or newer and run in the same cmd.exe window:

   ```text
   make restunts-original repldump-original pixldump-original
   ```

### On Linux (x86-64)

1. Install GNU Make 4.3 or newer, Bash, curl, tar, xz, and
   coreutils. Run the setup script from the repository root:

   ```sh
   tools/scripts/install-open-watcom.sh
   ```

2. Build with native GNU Make:

   ```sh
   make -C src/restunts restunts repldump pixldump
   ```

3. To build the original game and both dump tools from source as
   `stunts/restunto.exe`, `stunts/repldumo.exe`, and `stunts/pixldumo.exe`,
   run from the repository root after installing Python 3.9 or newer:

   ```sh
   make -C src/restunts restunts-original repldump-original pixldump-original
   ```

The makefiles use `python3` on Linux and `python` on Windows; override
`PYTHON` if needed.

### On both platforms

Build outputs are copied to `stunts/`. Run `restunts.exe` in DOSBox with
`core=dynamic` and `cycles=max`. The supplied DOSBox development configuration
uses drive S:. For that configuration, Windows users can run
`tools\mount_stunts_to_s.bat` once per reboot; Linux users running the bundled
Windows tools can map S: in `winecfg`. Native compilation does not need that mapping.

The supported targets are:

| Target | Result |
| --- | --- |
| `restunts` | Builds `restunts.exe` from the ported C game and DOS platform layer. |
| `restunts-original` | Assembles the original game and links `restunto.exe` with WLINK. |
| `repldump` | Builds the C physics dump tool, `repldump.exe`. |
| `pixldump` | Builds the C renderer dump tool, `pixldump.exe`. |
| `repldump-original` | Builds `repldumo.exe` from the original assembly and physics dump wrapper. |
| `pixldump-original` | Builds `pixldumo.exe` from the original assembly and renderer dump wrapper. |
| `test-dos-platform` | Builds the DOS platform ABI test, `tests/build/watcom/<configuration>/DOSPLAT.EXE`. |
| `clean` | Removes generated build objects and candidate executables. |

The `*-original` dump targets assemble the original game code with WASM,
compile the dump wrappers with WCC, and link them with WLINK. The resulting
executables are rebuilt development tools. Regression validation uses the
independent Borland binaries preserved under
[tools/oracles/borland](tools/oracles/borland/README.md), whose SHA256 hashes
are recorded alongside them. Source builds never replace those archived files.

The original game assembly under `src/restunts/asmorig/` is preserved
unchanged. The WASM build runs
[the source adapter](tools/scripts/prepare-wasm-original.py) to prepare
compatible copies under `asmorig/build/watcom/<configuration>/wasm/source/`.
All three `*-original` targets reuse these objects. The `restunto.exe` link
response file reproduces the original segment order, disables automatic segment packing,
and preserves the original 8000-byte stack. The old empty `segments.obj`
layout helper is replaced by WLINK ordering directives; its ASM source is
retained unchanged.

See [the assembler guide](docs/assembler.md) for generated-source handling,
original-code layout requirements, and object/executable comparison commands.

`makerepldump.bat` builds both games and both physics dump tools;
`makepixldump.bat` builds both renderer dump tools. Both stop on build errors.

### pixldump parameters

pixldump and pixldumo use the same mandatory parameters. The number of
parameters selects the output mode:

```text
pixldump.exe <replay> <camera> <target>
pixldump.exe <replay> <camera> <target> <frame>

pixldumo.exe <replay> <camera> <target>
pixldumo.exe <replay> <camera> <target> <frame>
```

| Parameter | Accepted values | Description |
| --- | --- | --- |
| `replay` | Replay base name, `.rpl`, or `.RPL` filename | Replay to render. The extension may be omitted. |
| `camera` | `1`, `2`, `3`, or `4` | Selects F1, F2, F3, or F4 respectively. |
| `target` | `0` or `1` | Selects the player (`0`) or opponent (`1`). Opponent mode requires a replay containing an opponent. |
| `frame` | `0` through `65535` | Exact frame to render. It must not exceed the replay's final frame. Supplying it selects BMP mode. |

With three parameters, the tools generate the normal hash dump. pixldumo writes
`<replay>.PDO` and pixldump writes `<replay>.PDD`. Each CRLF-terminated row
contains the decimal frame number, one space, and the lowercase MD5 of the raw
64,000-byte Mode 13h framebuffer. The tools sample frames 0, 5, 10, ...

With four parameters, the tools generate only the requested 320x200 indexed BMP
using the game's VGA palette. The filename includes the camera, target, frame,
and renderer:

```text
<replay>.<camera>.<target>.<frame>.PDO.bmp
<replay>.<camera>.<target>.<frame>.PDD.bmp
```

For example, `default.2.0.5.PDD.bmp` is frame 5 from the ported renderer, using
the F2 helicopter camera and following the player.

Examples:

```text
REM Generate a player/F2 hash dump.
pixldump.exe default.rpl 2 0

REM Generate an opponent/F4 hash dump with the original renderer.
pixldumo.exe default.rpl 4 1

REM Generate a player/F1 BMP at frame 5.
pixldump.exe default.rpl 1 0 5
```

pixldump must reproduce the original renderer, including its bugs. In particular,
polygon depth averages use unsigned division for non-power-of-two vertex counts
even when near-plane clipping retains a negative depth sum. This can put a grille
behind opaque surfaces, as in `0027.rpl`, camera 2, player, frame 665. Preserve
this behavior in the C port; `asmorig` and pixldumo remain the unchanged oracle.

The C renderer also preserves the original sphere bounding-box writes used by
crash explosions and the renderer stack values reused by stopped-wheel physics.
The pixel-dump wrapper supplies the archived caller context, deriving addresses
from the DOS load segment, decoded arguments, and resource allocations. See
[renderer parity notes](docs/renderer-parity.md) for the assembly evidence and
regression coverage.

Both modes force maximum graphical detail and hide the dashboard and replay
controls. Invalid arguments are rejected before an output file is created. The
complete output path, including its generated suffix, must fit in 127
characters. BMP filenames require DOS long-filename support; the supplied
`tools/scripts/dosbox.proc.conf` enables it for DOSBox-X.

### pixelcheck parameters

On Linux, `pixelcheck.sh` builds or reuses both executables, runs them with the
same replay, camera, target, and optional BMP frame, and compares the resulting
files:

```text
tools/scripts/pixelcheck.sh <replay-file> <rebuild> <camera> <target>
tools/scripts/pixelcheck.sh <replay-file> <rebuild> <camera> <target> <frame>
```

| Parameter | Accepted values | Description |
| --- | --- | --- |
| `replay-file` | Replay filename under `stunts/` | Replay passed to both executables. |
| `rebuild` | `true` or `false` | `true` rebuilds both executables first; `false` reuses the existing executables in `stunts/`. |
| `camera` | `1`, `2`, `3`, or `4` | Camera passed to both executables: F1, F2, F3, or F4 respectively. |
| `target` | `0` or `1` | Player (`0`) or opponent (`1`) passed to both executables. |
| `frame` | `0` through `65535` | Present in the BMP form only. It selects the exact frame compared by both executables. |

Examples:

```text
# Rebuild both executables and compare player/F2 hash dumps.
tools/scripts/pixelcheck.sh 0610.rpl true 2 0

# Reuse existing executables and compare opponent/F4 hash dumps.
tools/scripts/pixelcheck.sh 0610.rpl false 4 1

# Reuse existing executables and compare player/F1 BMPs at frame 5.
tools/scripts/pixelcheck.sh 0610.rpl false 1 0 5
```

Set the `PIXELDUMP_TIMEOUT_SECONDS` environment variable to a positive integer
to override the default 120-second timeout for each DOSBox run.




## CI replay validation

Pull requests and releases build the game, physics dump tools, and both
renderer dump tools. CI compares the full golden replay set for physics and
rendering by default, comparing pixldump `.PDD` files
against pixldumo `.PDO` files with camera 2 and player target 0.

Before testing, each CI shard downloads `BINs.zip` and `PDOs.zip` from
[restunts4d-oracles v1.0.0](https://github.com/CommonLoon102/restunts4d-oracles/releases/tag/v1.0.0).
It extracts only the oracle outputs assigned to that shard into the prepared
game directory, using the same renderer sampling and shard selection as the
regression runner. This avoids unpacking the full 15 GB physics archive on
every runner. Complete `.BIN` and `.PDO` files are reused; missing or invalid
outputs are generated during testing with the archived Borland executables
from `tools/oracles/borland`. Ported `.BNI` and `.PDD` outputs are always
generated afresh.

The C# application in `tools/scripts/dumpsrv` runs these comparisons on Linux, Windows,
and GitHub Actions. Its HTTP service, direct runner, and report merger share the
same engine. See the [service and runner guide](tools/scripts/dumpsrv/README.md)
for publishing, service parameters, client options, and local execution.

Set `renderer-test-percentage` (an integer from 1 to 100) when manually
starting **PR validation** or **Release** to change renderer coverage. Calls
to the reusable `build-and-validate.yml` workflow can set the same input;
pull request events use 100%. The HTTP service retains its separate 100% default
for `RendererTestPercentage`.

Every top-level `.rpl` file is eligible, regardless of filename structure.
Renderer sampling happens across the complete, ordinal-sorted corpus before
work is distributed round robin into shard lists and then worker lists.
The sample is independent of shard and worker counts, and both shard and
worker lists differ in size by at most one replay. CI validates completed replay
identities from the JSON shard results, so missing or duplicate coverage,
processing errors, and byte mismatches fail validation.

The reusable workflow defaults to 20 shards with 5 workers each, 120 seconds
per physics execution, and 480 seconds per renderer execution. It checks C/H
formatting and host regressions before building with native Open Watcom 2,
then runs the DOS ABI test. The C# regression application and its formatting
are checked on Linux before replay validation. Run these checks locally with the .NET 10 SDK:

```sh
dotnet test tools/scripts/dumpsrv/dumpsrv.slnx --configuration Release
dotnet format tools/scripts/dumpsrv/dumpsrv.slnx --verify-no-changes
```

### Compiler migration validation

For a deterministic comparison of 100 evenly spaced golden replays in both
physics and rendering, build `repldump` and `pixldump`, then run:

```sh
python3 tools/scripts/validate-toolchain.py --output out/watcom-validation
```

Use a new output directory. This verifies the archived Borland checksums,
records SHA-256 fingerprints of executables and inputs, and generates fresh
outputs in an isolated DOS directory. The shared C# runner checks complete
per-frame physics data and camera-2/player framebuffer samples byte for byte.
See [the oracle guide](tools/oracles/borland/README.md) for coverage, timeout,
and cache options. Run the platform ABI check separately:

```sh
make -C src/restunts test-dos-platform
python3 tools/scripts/run-dos-platform-test.py
```

## Build options

### Assembler

All assembly targets use Open Watcom WASM from the same pinned installation
as WCC and WLINK, with 8086 code generation. `-zcm=tasm` selects WASM's
built-in compatibility mode for the preserved assembly syntax; it does not
require the Turbo Assembler tools.

### Compiler, linker, and debugging symbols

All DOS C targets use Open Watcom 2 WCC and WLINK. `LINKER=wlink` is the only
supported linker setting. `setpath.bat` puts the pinned Watcom tools before
bundled utilities on PATH and sets `WATCOM` and `INCLUDE` accordingly.

Shared flags live in `src/restunts/watcom.mk`: 8086 instructions, the medium
memory model (far code and near data), the stack-based C calling convention,
signed `char`, and byte-packed structures. These settings preserve the
original game's 16-bit data layout and assembly interfaces. The custom DOS
startup initializes the stack and BSS; compiler stack probes are disabled.
The runtime libraries and headers come from the same pinned Watcom release.
Portable game C uses size optimization (`-os`). The DOS platform layer and
startup are compiled without optimization (`-od`) because the pinned compiler
can incorrectly merge branches around inline assembly interrupt calls.
DOS resource pointers are explicitly normalized to a paragraph plus an offset
below 16 bytes before they reach fixed-segment sprite code.
[Watcom huge-pointer arithmetic](https://github.com/open-watcom/open-watcom-v2/blob/2026-09-01-Build/bld/clib/cgsupp/a/pia.asm)
preserves larger offsets; normalization retains the Borland representation
and prevents bitmap reads from wrapping at a 64 KiB boundary.

Use `CONFIG=debug` to request Watcom C debug information. C optimization is
disabled except for the original renderer wrapper described below:

```text
make CONFIG=debug restunts
```

For `pixldump-original`, `pixldump.c` and `md5.c` retain size optimization
(`-os`) and use line debugging (`-d1`) without local-variable information.
This preserves the release code generation and stack layout required by the
original rendering code. Other objects use their usual debug flags.

WLINK writes Watcom debug information; debug builds also request WASM line
information.

## The toolchain

| Purpose | Active tool |
| --- | --- |
| 16-bit DOS C compilation | Open Watcom 2 `binnt/wcc.exe` or native Linux `binl64/wcc` |
| 16-bit DOS linking | Open Watcom 2 `binnt/wlink.exe` or native Linux `binl64/wlink` |
| C headers and runtime | Open Watcom 2 `h/` and `lib286/` |
| Assembly | Open Watcom 2 `binnt/wasm.exe` or native Linux `binl64/wasm` |
| Build orchestration | GNU Make 4.3 or newer (bundled 4.4.1 on Windows) |
| Original-source preparation | Python 3.9 or newer, for the `*-original` targets with WASM |
| Running and testing | DOSBox / DOSBox-X |

Current makefiles select Watcom executables by their full installation paths.
Open Watcom supplies all C headers and runtime libraries. Regression oracles
retain their original Borland-built machine code.

## Debugging restunts.exe

`CONFIG=debug` builds Watcom debug information and writes linker map files
beside the executables. Use a debugger that supports Watcom's format.
