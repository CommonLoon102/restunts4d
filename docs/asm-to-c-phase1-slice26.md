# Assembly-to-C migration: Phase 1, slice 26

## Fixed-width 3D-shape resource headers

Phase 1 slice 26 converts the pointer-free serialized `SHAPE3DHEADER` resource
record to fixed-width legacy byte types. The vertex, primitive, paint, and
reserved fields all use `legacy_u8`.

`shape3d_init_shape()` reads these counts directly from a loaded shape resource
and uses them to calculate the vertex, culling, and primitive-block offsets. A
C89 compile-time assertion now fixes the header at its four-byte resource
layout on every compiler, preventing host integer or plain-character behavior
from changing those offsets.

The pointer-bearing runtime `SHAPE3D` structure is intentionally outside this
slice. Its DOS far pointers must remain native pointers, so it needs separate
layout treatment rather than being conflated with the serialized header.

The fixed-width aliases resolve to the same machine types as the previous
declarations under Borland, so the DOS ABI and resource layout remain
unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies the
  four-byte header size and unsigned high-bit behavior.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle without a layout assertion failure or new relevant
  warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice26_shape3d_header_layout.txt`, byte-identical to
  the clean Phase 1 slice 25 report.
