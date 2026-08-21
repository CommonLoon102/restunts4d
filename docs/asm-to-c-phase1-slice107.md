# Assembly-to-C migration: Phase 1, slice 107

## Defined restore-checkpoint word semantics

Phase 1 slice 107 gives `restore_gamestate()` explicit legacy checkpoint
arithmetic. Its checkpoint slot now comes from signed 16-bit division, the
slot-to-frame calculation keeps the low word of the signed product, and the
requested and checkpoint frames compare as unsigned 16-bit patterns. This
matches the original `cwd`/`idiv`, `imul`, and unsigned `jb`/`ja` branches.

Hosted builds no longer let wider integer promotions change the quotient,
product, or comparison domain. Zero divisors and the original
`-32768 / -1` quotient-overflow trap remain outside the valid input envelope,
matching the DOS divide instruction. Borland definitions retain native word
operations.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char`. It checks every frame word
  against ten positive and negative nonzero interval patterns for low-word
  products and all non-trapping quotients, plus a full unsigned comparison
  permutation: 1,376,255 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object is 44,615 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice107_restore_checkpoint_words.txt`. It is empty
  and byte-identical to the clean Phase 1 slice 106 report.
