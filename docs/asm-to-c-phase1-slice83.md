# Assembly-to-C migration: Phase 1, slice 83

## Defined shifted compound negations

Phase 1 slice 83 converts the final two raw signed-word negations in
`update_player_state()` to exact 8086 word semantics. The collision-speed
threshold now reproduces the original sequence: retain only the low word of
the signed multiply, arithmetic-shift that word by eight, wrap the subtract
and negate, take the low byte, and place it in the high byte of the result.
This differs from allowing a hosted 32-bit `int` multiplication to survive
through the shift when the product exceeds the signed-word range.

The track-row calculation now performs its arithmetic right shift by ten,
subtraction, and negation as explicit 16-bit operations. Shared unsigned-word
multiply and fixed-count arithmetic-shift primitives make both operations
defined on hosted compilers. Borland-specific forms preserve the original C
expressions and therefore the layout-sensitive DOS object.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production `stateply.c` helper and checks all 65,536 input word patterns.
  It independently models the 8086 instruction sequences and verifies
  131,072 results across the two converted expressions.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `stateply.c` retains the same eight pre-existing
  missing-prototype warnings, and its DOS object remains exactly 20,862 bytes.
- Exactly twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the hosted helper.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice83_shifted_compound_negations.txt`. It is empty
  and byte-identical to the clean Phase 1 slice 82 report.
