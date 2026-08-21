# Assembly-to-C migration: Phase 1, slice 96

## Defined route byte-index semantics

Phase 1 slice 96 makes the route-sample indices in `player_op()` explicit
byte values. The local sample index and completion flag are stored as unsigned
bytes when passed to `sub_18D60()`. The previous-sample index decrements with
8-bit wraparound and is then sign-extended, matching the original `dec al` plus
`cbw`. Consuming `field_CE` returns its old unsigned byte while incrementing the
stored byte bit pattern.

This removes dependencies on plain-`char` signedness, integer promotion, and
signed-byte post-increment behavior. All affected locals remain one byte, so
the DOS stack layout is unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char` and checks all 256 byte patterns
  for the old unsigned value, wrapped signed decrement, and wrapped stored
  increment: 768 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `state.c` retains the same five pre-existing
  warnings; its DOS object is 15,169 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice96_route_byte_indices.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 95 report.
