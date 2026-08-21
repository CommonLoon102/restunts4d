# Assembly-to-C migration: Phase 1, slice 76

## Defined signed double-word coordinate projections

Phase 1 slice 76 converts all 33 six-bit coordinate projections in
`update_player_state()` to exact legacy double-word semantics. Plain world and
wheel coordinates now use explicit 32-bit arithmetic right shifts and retain
the original low result word. Wall and plane-relative forms wrap the following
word subtraction. The collision-coordinate form first performs the wrapped
signed-word/double-word addition from slice 73, then applies the projection.

The shared compatibility layer propagates the upper six sign bits explicitly
on hosted compilers and converts the projected low word without an out-of-range
signed cast. Borland keeps its native signed-long `>> 6` expression, preserving
the original `sar/rcr` code path without additional locals or helper calls.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test checks 3,048,576
  cases against independent projection references. It covers every 16-bit
  high word across 16 low-word boundaries, plus two million deterministic
  pseudo-random values, and checks plain projection, signed-word pre-addition,
  and wrapped word post-subtraction. The complete production `stateply.c` also
  compiles with sanitizer instrumentation.
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
  `partitions_all_phase1_slice76_signed_coordinate_projections.txt`. It is
  empty and byte-identical to the clean Phase 1 slice 75 report.
