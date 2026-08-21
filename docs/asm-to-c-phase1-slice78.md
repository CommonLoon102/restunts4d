# Assembly-to-C migration: Phase 1, slice 78

## Defined opponent stack-residue word extraction

Phase 1 slice 78 converts the four opponent wheel-position words used to
reconstruct legacy player-physics stack residue to exact fixed-width
semantics. Each value now performs a wrapped signed-word/32-bit position sum,
then extracts either its low or high 16-bit word explicitly. This removes the
DOS-specific assumption that C `unsigned long` is exactly 32 bits.

The compatibility primitives expose both low- and high-word extraction while
reusing slice 73's wrapped addition. The guarded Borland path compiles to the
same native operations and adds no helper call or stack local to the
layout-sensitive routine.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test checks 3,097,152
  signed-word/32-bit sums against independent modulo-addition and word
  extraction references. It covers every signed-word bit pattern across 32
  double-word boundaries plus one million deterministic pseudo-random pairs.
  The complete production `stateply.c` also compiles with sanitizer
  instrumentation.
- Borland C++ 5.2 compiles the guarded DOS stack-residue path and links
  `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the original replay oracle. `stateply.c`
  retains the same eight pre-existing missing-prototype warnings, and its DOS
  object remains exactly 20,862 bytes.
- Exactly twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice78_opponent_stack_residue_words.txt`. It is empty
  and byte-identical to the clean Phase 1 slice 77 report.
