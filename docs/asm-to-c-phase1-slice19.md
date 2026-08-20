# Assembly-to-C migration: Phase 1, slice 19

## Explicit replay-header serialization

Phase 1 slice 19 replaces the replay header's packed-structure casts with
explicit byte decoding and encoding. The on-disk `GAMEINFO` header is exactly
26 bytes. Its byte fields are copied at named offsets, and its two word fields
are read and written explicitly in little-endian order.

`legacy.h` now supplies C89-compatible little-endian 16-bit accessors. The
`game_framespersec` and `game_recordedframes` fields use `legacy_u16`, and a
compile-time assertion keeps `sizeof(struct GAMEINFO)` at the 26-byte assembly
and replay-format size. Replay writes use the serialized-format constant
instead of deriving the disk header length from the host structure size.

This removes native endianness, alignment, and structure-assignment behavior
from the replay file boundary. The byte fields deliberately retain their
current `char` declarations in this slice; making their signedness explicit,
including the opponent-car `-1` sentinel, is a separate semantic conversion.

The external DOS function ABIs are unchanged. No `seg0xx.asm` source was
changed.

## Verification

- A hosted strict C89 test verifies the 26-byte layout and round-trips the
  little-endian test word `0xA15C` through bytes `5C A1`.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle.
- Twenty local replay attempts were made for this slice. Eighteen completed
  and produced matching `.BIN`/`.BNI` MD5 hashes. `1654.rpl` and `1118.rpl`
  exceeded the local harness's fixed ten-second ported-executable timeout, so
  they produced no valid comparison and are recorded as incomplete rather
  than mismatches.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice19_replay_header_serialization.txt`,
  byte-identical to the clean Phase 1 slice 18 report.
- The Phase 0 audit remains current and reports zero active inline assembly,
  zero preserved-assembly calls from C, and 86 assembly link inputs.
