# Assembly-to-C migration: Phase 1, slice 39

## Portable generic resource-directory parsing

Phase 1 slice 39 removes the raw C-struct cast from `locate_resource()`. The
resource format begins with a four-byte size and a little-endian two-byte entry
count, followed by four-byte names and a parallel table of little-endian
four-byte offsets. The parser now reads the count and selected offset directly
from those serialized bytes.

Previously, the private header struct used `unsigned long`. Its DOS layout was
six bytes, but on an LP64 host the field becomes eight bytes and moves the
entry count away from byte 4. The selected offset was also read through an
unaligned `unsigned long far *`. Removing both casts makes the parser
independent of host `long` width, struct padding, alignment rules, and native
byte order.

The moving name pointer is retained: after the search advances to entry `j`,
adding the full name-table size reaches offset `j` in the parallel offset
table. Header/table-size arithmetic explicitly retains the original 16-bit
DOS intermediate behavior, while the selected serialized offset is an exact
`legacy_u32` used with the huge result pointer.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU C89 test compiled with `-funsigned-char` and warnings as errors
  constructs a two-entry resource directory, looks up the second offset via
  the moving name pointer, and verifies the computed data address.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the byte-wise parser in the far/huge DOS memory
  model.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice39_resource_directory.txt`, byte-identical to the
  clean Phase 1 slice 38 report.
