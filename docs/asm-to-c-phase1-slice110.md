# Assembly-to-C migration: Phase 1, slice 110

## Defined super-random word arithmetic

Phase 1 slice 110 gives `get_super_random()` explicit legacy word semantics.
It now evaluates `rand()`, the Kevin random generator, and the timer counter in
the original order, keeps only each low word, wraps their sum with the frame
word at 16 bits, interprets that bit pattern as signed, and applies a wrapping
word absolute value. The `0x8000` case therefore remains `0x8000`, matching the
original word `neg`.

Hosted builds no longer include the timer's high word or retain wider addition
and negation results. Borland definitions retain native word expressions.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helper with `-funsigned-char` and checks four permuted operands
  across every 16-bit pattern against an independent wrapping-sum and
  wrapping-absolute reference: 65,536 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object is 44,706 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helper.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice110_super_random_word.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 109 report.
