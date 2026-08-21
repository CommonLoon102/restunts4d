# Assembly-to-C migration: Phase 1, slice 82

## Defined compound signed-word negations

Phase 1 slice 82 converts eight compound signed-word negation expressions in
`update_player_state()` to exact legacy word semantics. The converted sites
cover three suspension-coordinate calculations, wall and car orientation
deltas, the doubled steering response, and the inverted path index. Each
operation now explicitly wraps at the same 16-bit boundaries as the original
8086 instructions, including the `0x8000` negation case and intermediate
addition overflow.

The two remaining negations in `update_player_state()` combine word
multiplication or arithmetic right shift with byte extraction. They remain
outside this bounded slice so their exact instruction sequences can be
modelled and tested independently. Hosted builds use fixed-width primitives;
the Borland definitions retain the original source forms so the layout-sensitive
DOS object does not grow.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test checks all 65,536
  input word patterns against 32 boundary operands, followed by 1,000,000
  deterministic pseudo-random triples: 3,097,152 tuples in total. It covers
  negate/add, negate/subtract, negate-sum/add, and negate/shift-left-one. The
  complete production `stateply.c` also compiles and runs with the harness.
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
  `partitions_all_phase1_slice82_compound_word_negations.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 81 report.
