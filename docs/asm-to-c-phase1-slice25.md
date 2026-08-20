# Assembly-to-C migration: Phase 1, slice 25

## Fixed-width 2D-shape headers

Phase 1 slice 25 converts the pointer-free packed `SHAPE2D` resource header to
the fixed-width legacy integer types. Its six dimension, position, and unknown
word fields use `legacy_u16`; its four flag and palette bytes use `legacy_u8`.

The 2D-shape loader and drawing code repeatedly uses `sizeof(struct SHAPE2D)`
to locate bitmap data and advance between serialized or expanded shapes. A
C89 compile-time assertion now fixes that boundary at the assembly layout size
of 16 bytes on every compiler. This prevents native integer widths from
silently changing resource pointer arithmetic.

The pointer-bearing `SPRITE` runtime record is intentionally outside this
slice. It needs separate treatment because its DOS far and near pointers must
remain native pointers on a hosted build rather than being forced into the
serialized-header representation.

The fixed-width aliases resolve to the same machine types as the previous
declarations under Borland, so the DOS ABI and assembly offsets remain
unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies the
  16-byte header size, unsigned 16-bit dimension range, and unsigned high-bit
  flag behavior.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle without a layout assertion failure or new relevant
  warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair has matching
  MD5 hashes; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice25_shape2d_layout.txt`, byte-identical to the
  clean Phase 1 slice 24 report.
