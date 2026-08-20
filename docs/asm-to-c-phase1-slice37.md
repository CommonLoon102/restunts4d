# Assembly-to-C migration: Phase 1, slice 37

## Exact opponent-route work-state widths

Phase 1 slice 37 converts the portable opponent-route rebuild's scratch state
to the integer widths used by the DOS implementation. Path nodes, saved branch
nodes, branch path lengths, indices, and control values are now
`legacy_s16`; accumulated route costs and their saved branch values are
`legacy_u32`.

The route bounds make these values fit their respective types, but explicit
widths are still required to keep the hosted algorithm's storage and
modulo-arithmetic model independent of the compiler data model. The path now
occupies 1,802 bytes and each 256-entry word branch stack occupies 512 bytes,
matching the intended DOS word representation rather than doubling on a host
where `int` is 32 bits.

This slice also centralizes two assembly-backed declarations. The signed
`track_pieces_counter` is a data-segment word, while `oppnentSped` is the full
16-byte unsigned speed table. This removes the previous conflicting local
declaration in `statecar.c` that exposed only ten table entries.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU C89 test compiled with `-funsigned-char` and warnings as errors
  verifies the assembly-backed table and counter widths plus the complete
  path, word branch stacks, and double-word cost stack sizes.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with unchanged DOS storage and calling behavior.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice37_opponent_route_widths.txt`, byte-identical to
  the clean Phase 1 slice 36 report.
