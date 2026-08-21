# Assembly-to-C migration: Phase 1, slice 109

## Defined car initializer coordinate semantics

Phase 1 slice 109 gives the four coordinate expressions in
`init_carstate_from_simd()` explicit legacy double-word semantics. Adding the
512-unit body-height offset now wraps across the complete 32-bit coordinate.
The initial X, Y, and Z wheel positions now use an arithmetic 32-bit shift by
six and keep its low signed word, matching the original `sar`/`rcr` loops
rather than hosted signed division's truncation toward zero.

This removes signed double-word overflow and implementation-defined shift
behavior from hosted builds. Borland continues to use native signed-long
addition and arithmetic shifts through the shared compatibility primitives.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  complete production source with `-funsigned-char`. It checks both operations
  across 16 double-word boundaries and one million deterministic random
  patterns against independent modulo-addition and arithmetic-shift
  references: 2,000,032 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object is 44,609 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice109_car_initializer_coordinates.txt`. It is
  empty and byte-identical to the clean Phase 1 slice 108 report.
