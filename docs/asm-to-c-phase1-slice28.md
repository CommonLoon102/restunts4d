# Assembly-to-C migration: Phase 1, slice 28

## Fixed-width transformed-shape records

Phase 1 slice 28 completes the fixed-width scalar definition of the runtime
`TRANSFORMEDSHAPE3D` record. Its distance or extent field uses `legacy_u16`,
and its flags and material fields use `legacy_u8`. The two embedded vectors
already use fixed-width signed words from slice 18.

The shape and clipping-rectangle members remain native near pointers. They
occupy two bytes each in Borland's DOS memory model but must grow naturally on
a hosted target. A DOS-only C89 compile-time assertion fixes the complete
assembly-visible record at 20 bytes without imposing a segmented pointer
representation on future targets.

The structure is shared directly with assembly in the mixed build: assembly
creates stack instances, reads the global/current array, and accesses all
members by declared offsets. The 20-byte assertion therefore protects both
those member offsets and the DOS array stride.

The fixed-width aliases resolve to the same machine types as the previous
declarations under Borland, so the DOS ABI and behavior remain unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies the
  two-byte unsigned word, unsigned high-bit byte behavior, and a complete
  hosted size composed from the fixed-width vectors/scalars and two native
  pointers.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the 20-byte DOS layout assertion enabled and
  without a new relevant warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice28_transformed_shape_layout.txt`, byte-identical
  to the clean Phase 1 slice 27 report.
