# Assembly-to-C migration: Phase 1, slice 89

## Defined signed collision-table decoding

Phase 1 slice 89 converts the start/finish collision-table lookup in
`update_player_state()` to an explicit signed-byte decode. `trackdata19` is an
unsigned-byte resource, but the original code sign-extends its selected byte
and treats `0xFF` as the `-1` sentinel. Casting through plain `char` does not
provide that behavior in hosted builds compiled with `-funsigned-char`.

The hosted helper now reconstructs every signed 8-bit value explicitly and
returns it as a legacy word. Under Borland it expands to the original `(char)`
cast, so the DOS object and executable are unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production decoder and exhaustively checks all 256 byte patterns, including
  the `0xFF` to `-1` sentinel conversion and byte round trips.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `stateply.c` retains the same eight pre-existing
  missing-prototype warnings, its DOS object remains exactly 20,862 bytes, and
  `REPLDUMP.EXE` is SHA-256 identical to the Slice 88 executable.
- Exactly twenty local replay attempts completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the hosted decoder.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice89_signed_collision_table_decode.txt`. It is
  empty and byte-identical to the clean Phase 1 slice 88 report.
