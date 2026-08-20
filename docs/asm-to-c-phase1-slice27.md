# Assembly-to-C migration: Phase 1, slice 27

## Fixed-width 3D-shape runtime records

Phase 1 slice 27 converts the three word-sized count fields in the runtime
`SHAPE3D` record to `legacy_u16`. The vertex, primitive, and paint counts are
unsigned DOS words in the assembly declaration and now retain that width on
hosted targets.

The four vertex, primitive, and culling data members remain native far
pointers. They occupy four bytes each in Borland's DOS memory model but grow
naturally with the target pointer representation in a future hosted build. A
DOS-only C89 compile-time assertion fixes the complete assembly-visible record
at 22 bytes without imposing the DOS pointer layout on other targets.

This size is an active ABI constraint. Assembly accesses the members by their
declared offsets, and both C and assembly select entries from the
`game3dshapes` array using byte displacements derived from the 22-byte record.
The assertion therefore protects the member offsets and array stride used by
the mixed DOS build.

The fixed-width aliases resolve to the same machine types as the previous
declarations under Borland, so the DOS ABI and behavior remain unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies the three
  two-byte unsigned count fields, unsigned high-bit behavior, and a complete
  structure size composed of six fixed bytes plus four native pointers.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the 22-byte DOS layout assertion enabled and
  without a new relevant warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice27_shape3d_runtime_layout.txt`, byte-identical to
  the clean Phase 1 slice 26 report.
