# Assembly-to-C migration: Phase 1, slice 103

## Defined player start-position arithmetic

Phase 1 slice 103 gives the three player start-position adjustments in
`run_game()` explicit legacy word/double-word semantics. The horizontal
offsets now sign-extend the 16-bit trigonometric result, scale that 32-bit bit
pattern by 64, and wrap the addition into the world coordinate. The vertical
offset likewise sign-extends its word addend and wraps the 32-bit addition.

This matches the original `cwd`, six `shl`/`rcl` pairs, and low/high-word
`add`/`adc` sequences without signed shifts or signed double-word overflow in
hosted builds. Borland definitions retain native signed-long expressions.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char`. It checks every signed-word bit
  pattern across 16 double-word boundary values for both scaled and unscaled
  additions: 2,097,152 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object is 44,612 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice103_player_start_position.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 102 report.
