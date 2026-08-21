# Assembly-to-C migration: Phase 1, slice 115

## Defined generated-edge fixed-point words

Phase 1 slice 115 gives `generate_poly_edges()` exact 32-bit fixed-point
temporaries. Hosted builds now compose the initial 16.16 value explicitly from
the two legacy words instead of reading through an `unsigned long *`, which
reads four bytes under the DOS data model but eight bytes on common LP64
hosts. Subsequent mode 5 and mode 6 additions and subtractions therefore wrap
at 32 bits on every target.

The Borland branch retains the original pointer expression verbatim. A
surrounding vertex-array comparison now uses matching pointer types; both
pointers address the same array and the cast emits no DOS operation.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production `shape3d.c` with `-funsigned-char` and warnings as errors. It
  exercises both fixed-point edge modes across every low-word pattern, with
  permuted high words and steps: 1,048,576 edge steps pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The existing 41 `restunts.c` and 65 `shape3d.c`
  warnings remain; `shape3d.obj` is unchanged at 43,127 bytes. A separate
  no-debug object comparison confirms that its OMF code/data and relocation
  records are byte-identical to the clean Phase 1 slice 114 build; only
  comment metadata differs.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after the renderer changes.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice115_generated_edge_fixed_points.txt`. It is
  empty and byte-identical to the corrected clean Phase 1 slice 114 report.
