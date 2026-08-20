# Assembly-to-C migration: Phase 3

## DOS file-I/O boundary

Phase 3 removes all ten active inline-assembly blocks from
`src/restunts/c/fileio.c`. The converted operations are create/open, close,
read, write, seek, tell, remove, find-first, and find-next.

The implementation keeps the existing small stdio-like interface used by the
game while delegating DOS calls to Borland's `_dos_*` and `_int86` C
interfaces. Seven additional medium-model runtime modules are extracted from
the checked-in `tools/lib/cm.lib` and linked only into the ported targets. The
original replay oracle continues to use its original link set.

No `seg0xx.asm` source was changed.

## Medium-model ABI

The DOS build disables automatic C symbol underscores with `/u-`, so the source
uses reviewed aliases for the selected runtime entry points. In the medium
memory model, the runtime header's `_FAR` macro is empty for path, register, and
result pointers; those arguments are near pointers. Read and write buffers are
the exception and remain explicit far pointers so page-sized transfers can
cross the normal data segment.

During incremental testing, literal `far` declarations for the near runtime
arguments caused deterministic replay-dumper timeouts. The declarations were
corrected to match the actual medium-model ABI. The same correction was applied
to the Phase 2 upper-memory aliases. Runtime result values that must be passed
by near pointer use DOS-only data-segment storage rather than stack locals.

## Behavior preserved

- `fopen` still clears `g_errno`, creates for write mode, and otherwise opens
  read-only.
- Read and write still return byte counts rather than element counts and set
  `g_errno` on DOS errors.
- Seek and tell preserve the original 32-bit `CX:DX` input and `DX:AX` output
  handling.
- Remove still exposes the raw DOS error through `g_errno`.
- File enumeration still accepts normal, hidden, and system files and returns
  the matched name combined with the query path.

## Verification

- A clean DOS rebuild produces `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the original
  oracle executable without new warnings in the converted wrappers.
- The final local `0610` replay produces byte-identical `.BIN` and `.BNI`
  files. Additional `0610` runs were used only to isolate the ABI mismatch; no
  further local replay sampling was performed after the full-build pass.
- The Phase 0 audit reports 39 active inline-assembly sites, down from 49.
- The comprehensive remote collection reports zero mismatches and is
  byte-identical to the Phase 2 clean baseline. Its preserved result is
  `stunts/partitions_all_phase3_fileio_c.txt`; an empty result file means that
  every replay matched.

The serial remote replay collection is the comprehensive regression gate for
this slice, and it passed.
