# Assembly-to-C migration: Phase 1, slice 100

## Defined game-state counter updates

Phase 1 slice 100 gives the runtime counters in `update_gamestate()` explicit
legacy semantics. Game frame, frame-within-second, and crash-delay words now
increment with 16-bit wraparound. The paused-replay delay adds eight with word
wraparound, and its byte state increments with 8-bit wraparound.

Hosted builds no longer risk signed word or byte overflow. Borland definitions
retain the original direct `inc` and compound-add expressions.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char` and checks every word increment,
  every word-plus-eight result, and every byte increment: 131,328 results
  pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object is 44,600 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice100_game_state_counters.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 99 report.
