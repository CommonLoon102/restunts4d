# Assembly-to-C migration: Phase 1, slice 102

## Defined replay-frame word arithmetic

Phase 1 slice 102 gives the replay result screen's displayed frame and
opponent timeout explicit legacy word semantics. The current game frame plus
elapsed replay time now wraps to the low 16 bits, as does the timeout's
`1500 * framespersec` product. This matches the original word `add` and the
low word left in `AX` by `imul`.

Hosted builds no longer retain wider intermediate results where the DOS build
uses a single word. Borland definitions retain the original direct arithmetic
expressions.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char`. It traverses every 16-bit frame
  and FPS pattern, with elapsed time following a permutation of every 16-bit
  pattern: 131,072 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object remains 44,594 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice102_replay_frame_words.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 101 report.
