# Assembly-to-C migration: Phase 1, slice 61

## Restored non-unit-pixel rectangle semantics

Phase 1 slice 61 restores the original non-unit horizontal pixel-scale path
in `rect_union()` and `rectlist_add_rect()`. These functions no longer abort
when `video_flag2_is1` differs from 1. Instead, the selected right edge is
adjusted by the scale minus one with word wrapping and then masked by
`video_flag3_isFFFF`, exactly as the original `ADD`, `DEC`, and `AND`
instructions specify.

The scale and mask globals now use explicit signed legacy words. A shared
helper performs the arithmetic through unsigned word bit patterns before
converting the result back to the rectangle's signed coordinate type. The
unit-scale path remains unchanged, while recursive rectangle-list additions
retain the original repeated adjustment behavior.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char`, warnings treated as
  errors, and address/undefined-behavior sanitizers exhaustively verifies all
  65,536 right-edge bit patterns across 25 scale/mask combinations. It also
  verifies input mutation and output for empty lists, repeated recursive
  adjustment, combined-list union adjustment, exact public signatures, and
  100,000 random unions.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with no warnings from `math.c`.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice61_video_rectangles.txt` and is byte-identical to
  the clean Phase 1 slice 60 report.
