# Archived Borland regression oracles

These executable files preserve the pre-Open Watcom regression reference. They
were copied byte for byte from the existing `stunts/` executables on 2026-09-08,
before compiler/linker migration. They contain the original disassembly and
the existing Borland-built dump wrappers. Preserve these exact archived bytes
when rebuilding development executables with Open Watcom.

| File | Bytes | Original file timestamp (local time) |
| --- | ---: | --- |
| `REPLDUMO.EXE` | 244268 | 2026-09-07 09:36 |
| `PIXLDUMO.EXE` | 248722 | 2026-09-08 00:48 |

The timestamps record the supplied local artifacts, not a claim of a
reproducible build. `SHA256SUMS` identifies the exact archived bytes. The
pre-migration files are untracked build outputs, so no source commit is claimed
as their verified build provenance.

From the repository root, verify the archived references on Linux:

```sh
(cd tools/oracles/borland && sha256sum --check SHA256SUMS)
```

`REPLDUMO.EXE` writes per-frame game state as `.BIN`; `PIXLDUMO.EXE` writes
framebuffer MD5 samples as `.PDO` for comparison with the ported `.BNI` and
`.PDD` outputs. Camera/target options must match on both sides.

The `repldump-original` and `pixldump-original` Make targets rebuild their
executables from the original assembly and dump wrapper sources with Open
Watcom, then place them in `stunts/`. Those builds leave this archive unchanged.
See [the build instructions](../../../readme.md#how-to-build) for commands.

Never replace these archived files while updating the compiler or fixing the
C port. A deliberate oracle change requires separate provenance and regression
review. CI and `validate-toolchain.py` verify these checksums and copy the
archived references into their isolated validation directories, independently
of any rebuilt `REPLDUMO.EXE` or `PIXLDUMO.EXE` under `stunts/`.

For a fresh, deterministic 100-replay comparison against these exact oracles:

```sh
python3 tools/scripts/validate-toolchain.py --output out/watcom-validation
```

This requires built `stunts/REPLDUMP.EXE` and `stunts/PIXLDUMP.EXE`, Python 3,
.NET 10, and DOSBox-X. The command copies game assets, selects 100 evenly spaced
replays from the ordinal-sorted golden ZIP, verifies the archived checksums,
and runs both physics and renderer comparisons for every selected replay.
Rendering uses camera 2 and player target 0. A new output directory is required
to exclude stale caches. `inputs.json` records SHA-256 hashes of the archive,
selected replays, assets, and executables; `summary.md` and `partitions_all.txt`
record coverage and results. Use `--count`, `--workers`, `--physics-timeout`,
`--renderer-timeout`, or `--candidate-directory` to override defaults.

The existing shared C# runner enforces `core=dynamic` and `cycles=max`, kills
owned emulator processes forcibly on timeout, and verifies byte-for-byte state
and framebuffer hash equality. Its merger checks that every selected replay
completed exactly once in both phases. `--prepare-only` creates and fingerprints
the isolated inputs without running DOSBox.

To reuse completed archived outputs from a previous invocation, add
`--oracle-cache PATH`. The preparation checks that the corpus, selected replay
hashes, asset hashes, oracle executable hashes, camera, and target match before
copying any cached `.BIN` or `.PDO` files. Incomplete outputs carrying a pending
marker are excluded. The new `inputs.json` fingerprints every copied cache
file; candidate outputs are always generated afresh.

The frozen renderer also preserves an original stack alias between primitive
rendering and stopped-wheel physics. A sampled rendering run can therefore
produce different crash-settling positions from a physics-only run or a single
requested bitmap. The C renderer's sampled replay wrapper explicitly models
that legacy call context. The oracle executable and original `.asm` sources
remain unchanged; expected hashes are never rewritten to hide a mismatch.
