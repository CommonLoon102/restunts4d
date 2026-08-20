# Assembly-to-C migration: Phase 1, slice 30

## Fixed-width track-object records

Phase 1 slice 30 converts the scalar fields of the runtime `TRACKOBJECT`
record to fixed-width legacy types. Its horizontal rotation is `legacy_s16`.
The overlay index, Z-bias flags, multi-tile flags, physical-model value, and
reserved byte are `legacy_u8`.

The surface/material byte is deliberately `legacy_s8`: the value `0xFF` is a
negative sentinel that tells the renderer to choose an alternate material,
and active C code tests the field against zero before using it as a material
index. This makes that behavior independent of the hosted compiler's plain
`char` signedness.

The track-info and two shape members remain native near pointers. They occupy
two bytes each in Borland's DOS memory model but grow naturally on a hosted
target. A DOS-only C89 compile-time assertion fixes the complete
assembly-visible record at 14 bytes without imposing DOS pointer widths on
future targets.

The structure is indexed and accessed directly by C and assembly in the mixed
DOS build, so its member offsets and 14-byte array stride are active ABI
constraints. The fixed-width aliases resolve to the same machine types as the
previous declarations under Borland, leaving DOS behavior unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies the signed
  `0xFF` surface sentinel, unsigned high-bit flag/index behavior, the two-byte
  rotation, and a hosted structure made from eight fixed bytes plus three
  native pointers.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the 14-byte DOS layout assertion enabled and
  without a new relevant warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice30_trackobject_layout.txt`, byte-identical to the
  clean Phase 1 slice 29 report.
