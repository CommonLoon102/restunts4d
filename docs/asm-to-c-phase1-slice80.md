# Assembly-to-C migration: Phase 1, slice 80

## Defined wrapped vector-delta scaling

Phase 1 slice 80 converts all eight six-bit word left shifts in
`update_player_state()` to exact legacy bit-pattern semantics. Six collision
vector components now wrap their 16-bit subtraction before shifting. The
plane-offset case wraps its negation before shifting, and the collision-point
case shifts the source word directly. Hosted C no longer left-shifts negative
promoted integers or relies on narrowing an out-of-range result.

The compatibility layer provides dedicated direct, subtract-then-shift, and
negate-then-shift forms. Hosted builds compose unsigned word operations.
Borland keeps each original native source expression so the layout-sensitive
DOS path retains its instruction footprint.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test checks 3,097,152
  cases against independent modulo-16-bit references for direct shift,
  wrapped subtraction then shift, and wrapped negation then shift. It covers
  every input word against 32 boundary words plus one million deterministic
  pseudo-random pairs. The complete production `stateply.c` also compiles with
  sanitizer instrumentation.
- An initial generic Borland composition grew `stateply.obj` by one byte. The
  final dedicated Borland expressions restore the exact 20,862-byte slice 79
  footprint. Borland C++ 5.2 links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle, with the same eight pre-existing `stateply.c`
  missing-prototype warnings.
- Exactly twenty local replay tests completed using the final restored-footprint
  binary. Every `.BIN`/`.BNI` pair is byte-identical and has matching MD5
  hashes; there were no timeouts or incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice80_wrapped_vector_scaling.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 79 report.
