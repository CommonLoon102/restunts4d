# Assembly-to-C migration: Phase 1, slice 106

## Defined replay checkpoint word semantics

Phase 1 slice 106 gives `update_gamestate()` explicit replay-buffer and
checkpoint indexing semantics. The game frame is interpreted as an unsigned
16-bit offset into the replay buffer and for checkpoint quotient/remainder,
matching the original `BX` address and word `div` instructions. The fetched
control byte is explicitly reinterpreted as signed before it reaches
`player_op()`, matching the original `cbw`.

Hosted builds no longer depend on plain-`char` signedness or on the signed
type of the stored game-frame word. A zero checkpoint interval remains outside
the valid input envelope, matching the original unchecked divide. Borland
definitions perform the native unsigned-word divisions.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char`. It checks every frame word
  against ten nonzero checkpoint intervals for both quotient and remainder,
  plus all 256 replay-input byte patterns: 1,310,976 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object is 44,616 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice106_replay_checkpoint_words.txt`. It is empty
  and byte-identical to the clean Phase 1 slice 105 report.
