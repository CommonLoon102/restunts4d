# Assembly-to-C migration: Phase 1, slice 75

## Defined four-wheel centroid arithmetic

Phase 1 slice 75 converts `update_player_state()`'s three four-wheel centroid
calculations to exact legacy double-word semantics. Each coordinate now wraps
the four signed 32-bit wheel positions at 32 bits, then performs the original
two-bit arithmetic right shift on the resulting bit pattern.

The shared compatibility primitive uses modulo additions and explicit sign-bit
propagation on hosted compilers, avoiding signed-overflow undefined behavior
and implementation-defined negative right shifts. Borland retains the original
native signed-long sum and shift expression, preserving the DOS hot path and
its stack layout.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test checks 2,160,000
  four-value combinations against an independent modulo-sum and arithmetic
  shift reference. It covers all 160,000 combinations of 20 boundary values
  plus two million deterministic pseudo-random quadruples. The complete
  production `stateply.c` also compiles with sanitizer instrumentation.
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
  `partitions_all_phase1_slice75_four_wheel_centroid.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 74 report.
