# Assembly-to-C migration: Phase 1, slice 51

## Defined vector interpolation arithmetic

Phase 1 slice 51 converts `vector_op_unk()` to explicit legacy word and
double-word arithmetic and gives its final parameter the original signed
one-word ABI.

The two depth differences and component differences now use wrapping word
subtraction. When the depth denominator is negative, both operands undergo the
original logical word shift rather than a host-dependent signed shift. Each
interpolated component uses an exact signed 16-by-16-to-32 product, signed
division, and wrapping word addition to the second endpoint.

This removes the routine's dependency on host `short` and `long` widths while
preserving the original nontrapping `IMUL`/`IDIV` behavior and output bit
patterns.

The Borland DOS ABI and vector layout remain unchanged because `legacy_s16` is
the original one-word signed type.

No `seg0xx.asm` source was changed.

## Verification

- A targeted hosted GNU89 test compiled with `-funsigned-char`, warnings as
  errors, and undefined-behavior sanitization compares the implementation with
  an independent instruction-level reference over 396,031 valid nontrapping
  deterministic random cases. It covers both denominator paths and both
  numerator signs.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice51_vector_interpolation.txt`, byte-identical to
  the clean Phase 1 slice 50 report.
