# Assembly-to-C migration: Phase 1, slice 86

## Defined four-vector word sums

Phase 1 slice 86 converts six chained four-vector expressions in
`update_player_state()` to exact word arithmetic. These expressions derive the
car's orientation from four transformed wheel coordinates before sign tests
and `polarAngle()` calls. Each now reproduces the original sequence of one
16-bit addition followed by two 16-bit subtractions, including wraparound at
every intermediate step.

Hosted builds compose the existing fixed-width word primitives. The Borland
definition expands to the original four-operand expression, preserving code
generation in the layout-sensitive DOS object.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test checks all 65,536
  first-operand word patterns against 32 derived boundary triples, followed by
  1,000,000 deterministic pseudo-random quadruples: 3,097,152 cases in total.
  The complete production `stateply.c` also compiles and links with the test.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `stateply.c` retains the same eight pre-existing
  missing-prototype warnings, and its DOS object remains exactly 20,862 bytes.
- Exactly twenty local replay attempts completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice86_four_vector_word_sums.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 85 report.
