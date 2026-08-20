# Assembly-to-C migration: Phase 1, slice 38

## Portable 2D-shape table serialization

Phase 1 slice 38 removes host `long` size, alignment, and byte-order
assumptions from active 2D-shape resource-table access. The format stores its
shape count and identifiers as little-endian words and its offsets and total
size as little-endian double words. Those fields are now read and written as
explicit bytes rather than through cast `unsigned short *`, `long *`, or
`unsigned long *` accesses.

`legacy.h` now provides C89-compatible `LEGACY_READ_U32_LE` and
`LEGACY_WRITE_U32_LE` helpers alongside the existing word helpers. The 2D
loader uses them for source offsets, expanded offset tables, allocation sizes,
and the final expanded-resource size. This guarantees four-byte serialized
double words on LP64 hosts and permits unaligned buffers on CPU architectures
that reject unaligned word or double-word loads.

The repeated bitmap word pattern is explicitly `legacy_u16`, expanded sizes
are accumulated as `legacy_u32`, and the original 16-bit intermediate wrap in
the per-shape bitmap-size calculation is retained. Under Borland these exact
types resolve to the original DOS widths.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU C89 test compiled with `-funsigned-char` and warnings as errors
  writes and reads 16- and 32-bit little-endian values at unaligned byte
  offsets, verifies the exact byte order, and checks the recovered values.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the byte-wise helpers applied to far pointers;
  no new `shape2d.c` warning is emitted.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice38_shape2d_serialization.txt`, byte-identical to
  the clean Phase 1 slice 37 report.
