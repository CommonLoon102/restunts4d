# Assembly-to-C migration: Phase 1, slice 24

## Fixed-width memory-manager records

Phase 1 slice 24 converts the three numeric fields in the packed `MEMCHUNK`
memory-manager record from plain `unsigned` to `legacy_u16`. Resource size,
paragraph offset, and allocation-state flags are all assembly-owned DOS words;
the 12-byte resource name remains character data.

Plain `unsigned` is 16 bits in the Borland DOS data model but normally 32 bits
on hosted compilers. Consequently, the previous C declaration was the required
18 bytes under DOS but expanded to 24 bytes on a modern host, changing every
array stride and field offset after the resource name. A C89 compile-time
assertion now enforces the assembly layout size of 18 bytes on every compiler.

The fixed-width aliases resolve to the same machine type as the old fields
under Borland, so the DOS ABI, assembly offsets, and resource-table layout are
unchanged. On hosted builds, the declaration now retains the DOS word ranges
and record stride rather than inheriting the native width of `unsigned`.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies the
  18-byte record size and the full unsigned 16-bit ranges of its numeric
  fields.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle without a layout assertion failure or new relevant
  warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair has matching
  MD5 hashes; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice24_memchunk_layout.txt`, byte-identical to the
  clean Phase 1 slice 23 report.
