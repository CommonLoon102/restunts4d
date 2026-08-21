# Assembly-to-C migration: Phase 1, slice 93

## Defined projected-coordinate subtraction

Phase 1 slice 93 gives the twelve player-route world-coordinate projections in
`player_op()` explicit legacy semantics. Each 32-bit coordinate is shifted
arithmetically right by six, only the low result word is retained, and that
word is subtracted from a vector or track-center word with 16-bit wraparound.
The special route-height case is represented as the same wrapped subtraction
from zero.

This matches the original repeated `sar`/`rcr` sequence followed by word
`sub` or `neg`. Hosted builds no longer depend on signed-right-shift behavior,
host `int` width, or signed narrowing. The Borland macro retains the original
direct expression.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helper with `-funsigned-char` and checks 4,521,984 results. It
  covers every low 22-bit coordinate pattern with varied upper bits, then all
  65,536 left words against five signed and boundary coordinates.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `state.c` retains the same five pre-existing
  warnings; its DOS object is 15,162 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the hosted helper.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice93_projected_coordinate_subtraction.txt`. It is
  empty and byte-identical to the clean Phase 1 slice 92 report.
