# Assembly-to-C migration: Phase 1, slice 91

## Defined penalty track-coordinate byte semantics

Phase 1 slice 91 makes both C penalty-route traversals decode the player's
track column and row with the original byte semantics. The column is byte 2 of
the 32-bit world X coordinate, interpreted as a signed byte. The row first
subtracts byte 2 of world Z from `0x1D` with 8-bit wraparound and only then
sign-extends the result.

This mirrors the original `mov al`/byte `sub`/`cbw` sequence. It avoids hosted
plain-`char` signedness and signed-right-shift dependencies, and fixes the
previous C row expression, which could retain a promoted value outside the
signed-byte range instead of wrapping it first.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char` and checks every possible selected
  coordinate byte and every ignored upper byte: 131,072 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `state.c` retains the same five pre-existing
  warnings and its DOS object remains exactly 15,160 bytes.
- Exactly twenty valid local replay comparisons completed. Every `.BIN`/`.BNI`
  pair is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice91_penalty_track_coordinates.txt`. It is empty
  and byte-identical to the clean Phase 1 slice 90 report.
