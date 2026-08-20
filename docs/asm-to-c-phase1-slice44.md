# Assembly-to-C migration: Phase 1, slice 44

## Defined polygon-facing determinant

Phase 1 slice 44 makes `is_facing_camera()` reproduce the original integer
machine semantics without relying on host `long`. The X deltas are exact
signed 32-bit differences. As in the original instruction sequence, each Y
delta first wraps as a 16-bit subtraction and is then sign-extended.

The two determinant products and their subtraction now operate on
`legacy_u32` bit patterns through explicit modulo-32-bit helpers. The result
is classified by its zero and sign bits, exactly matching the original
`DX:AX` comparison. This avoids both LP64 widening and undefined signed
overflow. It also covers valid extreme point coordinates where the
mathematical determinant exceeds `INT32_MAX` but the DOS routine intentionally
uses its wrapped 32-bit result.

`legacy.h` now provides reusable `LEGACY_U32_WRAP_SUB()` and
`LEGACY_U32_WRAP_MUL()` operations for later word-level translations.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char`, warnings as errors, and
  undefined-behavior sanitization verifies normal, reversed, 16-bit Y-wrap,
  and overflowing determinant cases. The overflow case produces the expected
  legacy bit pattern `0xFFFE0001` without a sanitizer failure.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice44_facing_wrap.txt`, byte-identical to the clean
  Phase 1 slice 43 report.
