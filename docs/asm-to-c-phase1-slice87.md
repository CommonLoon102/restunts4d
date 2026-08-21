# Assembly-to-C migration: Phase 1, slice 87

## Defined surface sums and jump-counter increment

Phase 1 slice 87 converts three surface-type sums in
`update_player_state()` to exact 8-bit wrapping arithmetic. The front and rear
wheel pairs and their combined total now reproduce the original `ADD AL`
behavior for every byte pattern instead of relying on hosted narrowing to a
signed character. The stack temporary is also explicitly a legacy signed
byte.

The player jump counter now increments with explicit 16-bit wrapping, matching
the original `INC` word instruction when its value crosses `0x7FFF` or
`0xFFFF`. Borland definitions retain the original addition and compound
assignment expressions, preserving the layout-sensitive DOS object.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production word helper and exhaustively checks all 65,536 byte pairs plus
  all 65,536 jump-counter word patterns: 131,072 results in total.
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
  `partitions_all_phase1_slice87_surface_sum_jump_counter.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 86 report.
