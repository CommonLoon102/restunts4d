# Assembly-to-C migration: Phase 1, slice 112

## Defined game interval word arithmetic

Phase 1 slice 112 gives the two frame-rate calculations in
`init_game_state()` explicit legacy signed-word semantics. The thirty-second
replay checkpoint interval now retains the low word of the original signed
multiply. The timer interval interprets the frame-rate bit pattern as signed
before dividing 100 with truncation toward zero, matching the original
`cwd`/`idiv` sequence.

Hosted builds therefore no longer inherit unsigned or wider arithmetic from
the declared `framespersec` type. Zero remains outside the valid input
envelope, just as it triggers the original unchecked divide instruction. The
Borland definitions retain native signed-word expressions.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char` and checks the multiplication for
  every 16-bit frame-rate pattern plus signed division for every nonzero
  pattern: 131,071 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object is 44,705 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice112_game_interval_words.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 111 report.
