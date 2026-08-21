# Assembly-to-C migration: Phase 1, slice 74

## Defined compound wheel-position arithmetic

Phase 1 slice 74 converts the seven remaining compound wheel-position
additions in `update_player_state()` to their exact legacy evaluation order.
Three coordinates now wrap two vector components at 16 bits before
sign-extending that word into a 32-bit world-coordinate addition. Three other
coordinates perform two independently sign-extended word additions in
sequence. The suspension expression wraps `var_EE + 0x180` at 16 bits before
adding it to the wheel's 32-bit vertical position.

The implementation composes the fixed-width compatibility primitives from
slices 70 and 73. Borland therefore retains native word and signed-long
operations, with no new local variables or helper calls in the layout-sensitive
physics routine.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test checks 9,388,608
  compound cases against independent references for both word-sum-first and
  sequential signed-word evaluation. It includes every first-word bit pattern
  across 16 double-word boundaries and eight second-word boundaries, including
  `0x0180`, plus one million deterministic pseudo-random triples. The complete
  production `stateply.c` also compiles with sanitizer instrumentation.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `stateply.c` retains the same eight pre-existing
  missing-prototype warnings, with no new warning category or count; its DOS
  object grows by six bytes.
- Exactly twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice74_compound_wheel_position_arithmetic.txt`. It is
  empty and byte-identical to the clean Phase 1 slice 73 report.
