# Assembly-to-C migration: Phase 1, slice 101

## Defined runtime byte-index updates

Phase 1 slice 101 gives the remaining byte updates in `init_game_state()` and
`run_game()` explicit legacy semantics. Each initial car-route index is
consumed as its old unsigned byte while its stored signed-byte bit pattern is
incremented before `sub_18D60()` runs, matching the original `mov al`, byte
`inc`, and zero-extension sequence. The idle camera mode and dashboard redraw
countdown now also use explicit 8-bit wrapping updates.

This removes dependencies on plain-`char` signedness, signed-byte overflow,
and function-call ordering in hosted builds. Borland definitions retain the
original direct post-increment and post-decrement expressions.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char` and checks increment, decrement,
  unsigned old-value consumption, and stored post-increment across all 256
  byte patterns: 1,024 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object is 44,594 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice101_runtime_byte_indices.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 100 report.
