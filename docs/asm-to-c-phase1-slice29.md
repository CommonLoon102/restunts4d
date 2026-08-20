# Assembly-to-C migration: Phase 1, slice 29

## Fixed-width sprite records and line-offset tables

Phase 1 slice 29 converts all twelve word-sized scalar members of the runtime
`SPRITE` record to `legacy_u16`. The pointer to its per-row bitmap-offset table
is now explicitly a pointer to `legacy_u16` entries.

The bitmap member remains a native far pointer and the line-table member
remains a native near pointer. They occupy four and two bytes respectively in
Borland's DOS memory model but grow naturally on a hosted target. A DOS-only
C89 compile-time assertion fixes the complete assembly-visible record at 30
bytes without imposing segmented pointer widths elsewhere.

The window-sprite allocator previously sized its line-offset table with
`sizeof(unsigned int)`. That happens to be two bytes under Borland but becomes
four on common hosted targets. Allocation, initialization, access, and release
now consistently use `legacy_u16`, preserving the resource-relative word
offset format and preventing a hosted row-table stride mismatch.

The structure is copied whole and accessed directly by both C and assembly in
the mixed DOS build, so its 30-byte size and member offsets are active ABI
constraints. The fixed-width aliases resolve to the same machine types as the
previous declarations under Borland, leaving DOS behavior unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies the
  two-byte unsigned scalar and line-offset entry widths, unsigned high-bit
  behavior, and a hosted structure made from 24 fixed bytes plus two native
  pointers.
- A source audit confirms that sprite line-table sizing and pointer access no
  longer depend on hosted `unsigned int` width.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the 30-byte DOS layout assertion enabled and
  without a new relevant warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice29_sprite_layout.txt`, byte-identical to the
  clean Phase 1 slice 28 report.
