# Assembly-to-C migration: Phase 1, slice 40

## Exact memory-manager byte-count APIs

Phase 1 slice 40 converts the memory manager's double-word size interfaces
from host `long` spellings to the exact legacy types used by their DOS ABIs.
`mmgr_get_res_ofs_diff_scaled()`, `mmgr_get_chunk_size_bytes()`, and
`mmgr_prepare_fullscreen_window()` now return `legacy_u32`; the original
implementations return these byte counts in `DX:AX` after shifting a 16-bit
paragraph count left by four.

`mmgr_alloc_resbytes()` now accepts `legacy_s32`, matching its two-word signed
division by 16 before the allocator reserves one additional paragraph. Active
memory checks and 3D car-resource callers carry the exact 32-bit values through
their local variables, and the redundant conflicting declaration in
`shape3d.c` has been removed.

The changes preserve the Borland ABI because `legacy_u32` and `legacy_s32`
resolve to the original four-byte `unsigned long` and `long` types there. On
LP64 hosts, they prevent paragraph-derived byte counts from silently widening
to eight bytes and changing function interfaces or wrap behavior.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU C89 test compiled with `-funsigned-char` and warnings as errors
  verifies all three return widths and confirms that scaling the maximum
  16-bit paragraph count produces the exact 32-bit value `0x000FFFF0`.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged two-word DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice40_memory_byte_counts.txt`, byte-identical to the
  clean Phase 1 slice 39 report.
