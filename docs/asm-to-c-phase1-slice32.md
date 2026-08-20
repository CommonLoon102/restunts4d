# Assembly-to-C migration: Phase 1, slice 32

## Fixed-width track work tables

Phase 1 slice 32 converts the shared track row/position arrays and all
word-based regions of the allocated track-data block to `legacy_s16`. These
tables were declared as DOS `int` or `short` values, both of which are two
bytes under Borland, but hosted `int` is commonly four bytes.

The converted allocated regions are the main and alternate path tables, the
player and opponent aerodynamic tables, the direction table, camera-position
table, and relative track-coordinate table. They contain signed values such
as the `-1` path sentinel and signed spatial coordinates, so `legacy_s16`
preserves both their width and interpretation.

Both `init_trackdata()` implementations now cast the byte-buffer boundaries
to `legacy_s16 far *` explicitly. The byte offsets that partition the single
`0x6BF3`-byte allocation are unchanged; subsequent indexing now advances by
the intended two-byte word on every compiler. This also removes the seven
Borland suspicious-pointer-conversion warnings previously emitted for those
assignments in the replay-dump initializer.

The declarations resolve to the same two-byte machine type under Borland, so
the assembly-owned arrays and far pointers retain their DOS ABI.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies that all
  eight row/position arrays and seven allocated table pointees have two-byte
  signed elements.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The prior seven track-table pointer-conversion
  warnings are absent from the build log.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice32_track_word_tables.txt`, byte-identical to the
  clean Phase 1 slice 31 report.
