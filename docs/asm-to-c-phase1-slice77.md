# Assembly-to-C migration: Phase 1, slice 77

## Defined low-word wheel-offset differences

Phase 1 slice 77 converts the three wheel-to-centroid coordinate differences
in `update_player_state()` to exact legacy low-word semantics. The original
routine loads only each 32-bit wheel coordinate's low word, subtracts the
centroid's low word, and stores the wrapped result in a 16-bit vector. Hosted C
now follows that sequence directly instead of evaluating a potentially
overflowing signed 32-bit subtraction before narrowing it.

The shared compatibility primitive ignores the high words explicitly on
hosted compilers. Borland retains the original native narrowed signed-long
expression, preserving the DOS instruction sequence without a helper call or
additional local.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test checks 3,097,152
  low-word differences against an independent modulo-16-bit reference. It
  covers every left low-word pattern against 32 boundary right words with
  varying high words, plus one million deterministic pseudo-random double-word
  pairs. The complete production `stateply.c` also compiles with sanitizer
  instrumentation.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `stateply.c` retains the same eight pre-existing
  missing-prototype warnings, and its DOS object remains exactly 20,862 bytes.
- Exactly twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice77_low_word_wheel_offsets.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 76 report.
