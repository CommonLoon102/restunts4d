# Assembly-to-C migration: Phase 1, slice 48

## Defined matrix fixed-point arithmetic

Phase 1 slice 48 makes the fixed-point arithmetic in `mat_mul_vector()` and
`mat_multiply()` reproduce the original 16-bit implementation independently
of host integer width and signed right-shift behavior.

Each matrix product now follows the original signed `IMUL` followed by two
`SHL`/`RCL` pairs and returns the resulting high word. The shared helper uses
explicit 32-bit bit-pattern multiplication and extraction, while each
three-term dot product accumulates with explicit 16-bit wrapping just like the
original word `ADD` instructions. `mat_multiply()` also addresses matrix
storage through `legacy_s16` pointers instead of host-width `int` pointers.

The Borland DOS ABI and matrix layout remain unchanged because `legacy_s16` is
the original one-word signed type.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char`, warnings as errors, and
  undefined-behavior sanitization verifies boundary products, 100,000 random
  input pairs, and an overflowing three-term accumulation against an
  independent bit-pattern reference.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice48_matrix_fixed_point.txt`, byte-identical to the
  clean Phase 1 slice 47 report.
