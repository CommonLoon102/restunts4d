# Assembly-to-C migration: Phase 1, slice 66

## Defined torque/mass acceleration arithmetic

Phase 1 slice 66 replaces `update_car_speed()`'s host-dependent torque/mass
block with an exact legacy helper. It preserves the original sequence rather
than reducing it to a mathematically similar host expression.

The helper keeps the low word of the unsigned gear-ratio/torque product,
logically shifts that word by four, wraps its addition into the signed delta
word, sign-extends that result to a double word, multiplies by 25 modulo
32 bits, divides the resulting bit pattern by the zero-extended mass as an
unsigned double word, and finally arithmetic-shifts the quotient's low word.
This includes the original routine's unusual signed-multiply/unsigned-divide
combination and its word-narrowing points.

A zero mass remains outside the helper's valid envelope, matching the
original unchecked division. No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test compiles the
  production helper with a forced exact prototype. It exhaustively checks all
  65,536 delta words across 12 ratio/torque/mass boundary tuples, sweeps every
  ratio word and every nonzero mass word, and checks 1,000,000 deterministic
  random cases against an independent signed/unsigned 64-bit reference.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `statecar.c` compiles with zero warnings.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice66_acceleration_mass.txt` and is byte-identical
  to the clean Phase 1 slice 65 report.
