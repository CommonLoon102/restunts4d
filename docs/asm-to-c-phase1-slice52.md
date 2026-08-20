# Assembly-to-C migration: Phase 1, slice 52

## Defined plane arithmetic word semantics

Phase 1 slice 52 converts `vec_normalInnerProduct()` and `plane_origin_op()`
to explicit legacy word and double-word arithmetic. Their coordinate,
index, global-state, and result declarations now expose the original signed
one-word representation on every host.

The inner product performs three signed 16-by-16 multiplications, accumulates
their bit patterns with explicit 32-bit wrapping, interprets the wrapped sum as
a signed double word, divides by `0x2000`, and returns the quotient's low signed
word. The plane-origin calculation now wraps every origin/offset addition and
relative-coordinate subtraction exactly like the original word instructions.

`legacy.h` gains reusable 32-bit wrapping addition and portable signed
double-word bit interpretation helpers for this and later translations.

The Borland DOS ABI and packed geometry layout remain unchanged because the
new public types are the same original word types.

No `seg0xx.asm` source was changed.

## Verification

- A targeted hosted GNU89 test compiled with `-funsigned-char`, warnings as
  errors, and undefined-behavior sanitization compares 500,000 inner products
  and 250,000 plane-origin calculations with an independent instruction-level
  reference. Both signs of the wrapped 32-bit sum are covered.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice52_plane_arithmetic.txt`, byte-identical to the
  clean Phase 1 slice 51 report.
