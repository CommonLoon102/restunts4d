# Assembly-to-C migration: Phase 1, slice 20

## Explicit replay-header byte signedness

Phase 1 slice 20 removes the replay header's remaining dependency on whether a
compiler treats plain `char` as signed or unsigned. The player and opponent car
IDs now use `legacy_s8`; material, transmission, and opponent-selection bytes
use `legacy_u8`. The track name remains a C character string, but its decoder
copies the underlying unsigned-byte representation directly.

The opponent car ID uses `FF 00 00 00` when there is no opponent. Existing car
loading logic tests its first element against `-1`. `LEGACY_S8_FROM_BITS` now
performs that conversion explicitly, so byte `FF` becomes `-1` even on targets
such as ARM configurations where plain `char` can be unsigned. Default game
setup likewise assigns the semantic value `-1` instead of relying on conversion
from the out-of-range integer constant `0xFF`.

The C declarations for `shape3d_load_car_shapes` and the byte-pointer arguments
of `run_car_menu` now reflect these exact types. Their DOS calling conventions
and pointer representations are unchanged. `GAMEINFO` remains exactly 26 bytes,
and its serialized byte representation is unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies the 26-byte
  layout and the conversions `00 -> 0`, `7F -> 127`, `80 -> -128`, and
  `FF -> -1`.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle without a new warning tied to this slice.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair has matching
  MD5 hashes; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports zero active inline assembly,
  zero preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice20_gameinfo_signedness.txt`, byte-identical to
  the clean Phase 1 slice 19 report.
