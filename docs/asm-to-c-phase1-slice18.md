# Assembly-to-C migration: Phase 1, slice 18

## Fixed-width geometry layouts

Phase 1 slice 18 applies the fixed-width legacy integer foundation to the
packed geometry structures shared by C, assembly, and binary resources.
`RECTANGLE`, `VECTOR`, `VECTORLONG`, `POINT2D`, `MATRIX`, and `PLANE` now use
explicit 16- or 32-bit fields instead of host-dependent `int` and `long`
fields.

The public function declarations remain unchanged, so this slice does not
alter the 16-bit DOS calling convention. It only makes the stored layouts
portable. On a modern LP64 host the previous declarations produced incorrect
sizes for four of the six structures; for example, `VECTORLONG` was 24 bytes
instead of 12 and `PLANE` was 56 bytes instead of 34.

## Layout checks

C89-compatible compile-time assertions now enforce the assembly and resource
format sizes:

| Structure | Required size |
| --- | ---: |
| `RECTANGLE` | 8 bytes |
| `VECTOR` | 6 bytes |
| `VECTORLONG` | 12 bytes |
| `POINT2D` | 4 bytes |
| `MATRIX` | 18 bytes |
| `PLANE` | 34 bytes |

The C makefile declares `math.h` and `legacy.h` as dependencies of every C
object. This is deliberately conservative: the geometry header is included
through several other headers, and the old makefile does not generate header
dependencies automatically.

The hosted implementation of `LEGACY_S16_FROM_BITS` was also changed from a
header-local function to a C89 expression macro. This prevents strict hosted
builds from reporting an unused static helper in translation units which use
the types and layout assertions but not the conversion operation.

No `seg0xx.asm` source was changed.

## Verification

- A hosted C89 compile with strict warnings reports the required structure
  sizes: 8, 6, 12, 4, 18, and 34 bytes respectively.
- Borland C++ 5.2 compiles and links both `RESTUNTS.EXE` and `REPLDUMP.EXE`.
- The Phase 0 audit remains current and reports zero active inline assembly,
  zero preserved-assembly calls from C, and 86 assembly link inputs.
- No additional ad-hoc replay state dump was run after Phase 1 slice 03 reached
  the requested local-run limit.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice18_geometry_layouts.txt`, byte-identical to the
  clean Phase 1 slice 17 report.
