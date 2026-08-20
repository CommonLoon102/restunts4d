# Assembly-to-C migration: Phase 1, slice 62

## Defined clip-rectangle rotation word semantics

Phase 1 slice 62 gives `select_cliprect_rotate()` explicit legacy word
semantics: its three rotation angles, selection parameter, and return value
are now 16-bit words on every compiler. The matching public and local
declarations use the same types, and the DOS data-segment declaration for
`select_rect_param` is represented as an exact `legacy_u16` word in C.

The reverse rotation now performs each of the original routine's three
`NEG AX` operations through `LEGACY_U16_WRAP_NEGATE()`. This defines the
`0x8000` case without relying on signed-overflow behavior or the host width of
`int`. The returned polar heading is converted through its word bit pattern
before applying the original 10-bit mask.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test exhaustively checks
  all 65,536 word patterns in each angle position. The warning-clean test TU
  verifies the exact public signature, forward and reverse matrix arguments,
  `0x8000` negation, matrix and rectangle copies, the selection word, polygon
  state reset, vector flow, polar arguments, and the masked return value.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The changed routine introduces no new warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice62_cliprect_rotation.txt` and is byte-identical
  to the clean Phase 1 slice 61 report.
