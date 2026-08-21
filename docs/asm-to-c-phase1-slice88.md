# Assembly-to-C migration: Phase 1, slice 88

## Defined orientation word offsets

Phase 1 slice 88 converts four orientation-offset expressions in
`update_player_state()` to exact 16-bit wrapping arithmetic. The converted
sites cover the masked wall orientation, the reflected collision angle, and
the two `polarAngle()` results adjusted by `0x100`. Each expression now
reproduces the original word `ADD` or `SUB` before masking, assignment, or
signed comparison.

Hosted builds use the existing fixed-width state helpers. Those helpers expand
to the original source operations under Borland, preserving the
layout-sensitive DOS object.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers and exhaustively checks all 65,536 input word patterns
  across masked add, reflected subtract, and angle-offset subtract:
  196,608 results in total.
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
  `partitions_all_phase1_slice88_orientation_word_offsets.txt`. It is empty
  and byte-identical to the clean Phase 1 slice 87 report.
