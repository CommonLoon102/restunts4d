# Assembly-to-C migration: Phase 1, slice 53

## Defined rectangle adjustment word semantics

Phase 1 slice 53 converts the two `point + 1` operations in
`rect_adjust_from_point()` to explicit signed-word behavior. Each increment now
wraps as a 16-bit bit pattern before the signed comparison and possible
rectangle-bound update, exactly matching the original `LEA AX, [word+1]` and
word `CMP` instructions.

This removes the modern-host widening that previously made `32767 + 1` compare
as `32768` instead of wrapping to the original `-32768` word value.

The Borland DOS ABI and packed rectangle layout remain unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A targeted hosted GNU89 test compiled with `-funsigned-char`, warnings as
  errors, and undefined-behavior sanitization compares 500,000 deterministic
  point/rectangle inputs with an independent word-level reference, including
  the `32767 + 1` boundary.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice53_rectangle_adjust.txt`, byte-identical to the
  clean Phase 1 slice 52 report.
