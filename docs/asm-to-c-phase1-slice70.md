# Assembly-to-C migration: Phase 1, slice 70

## Defined gear-knob word arithmetic

Phase 1 slice 70 converts the complete gear-knob motion block in
`update_car_speed()` to explicit legacy signed-word arithmetic. Its three
coordinate differences now wrap at 16 bits before signed interpretation, and
the four incremental coordinate updates use wrapped signed add or subtract.

The shared legacy foundation now also provides wrapped signed addition,
negation, and word absolute value. The absolute operation deliberately leaves
`0x8000` as the signed word `-32768`, matching the original 16-bit `_abs`
sequence and its following signed comparison. These primitives remain inline
expressions, avoiding calls in the per-frame physics path.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test exhaustively checks
  wrapped negation and word absolute value for all 65,536 input patterns. It
  also checks wrapped signed add and subtract for 2,572,864 operand pairs: two
  complete 65,536-word sweeps for each of 12 boundary values plus 1,000,000
  deterministic pairs. The complete production `statecar.c` compiles under
  the same sanitizers.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `statecar.c` compiles with zero warnings.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice70_gear_knob_arithmetic.txt` and is
  byte-identical to the clean Phase 1 slice 69 report.
