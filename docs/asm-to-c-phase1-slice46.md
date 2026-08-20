# Assembly-to-C migration: Phase 1, slice 46

## Exact polar-radius word semantics

Phase 1 slice 46 converts `polarRadius2D()` and `polarRadius3D()` to explicit
one-word signed interfaces. Their arguments and return values now use
`legacy_s16`, matching the original stack words and `AX` return value. Hosted
callers therefore truncate coordinate differences at the same boundary as the
16-bit DOS ABI instead of silently passing host-width `int` values.

`polarRadius2D()` now performs its absolute-value and angle-folding steps with
explicit 16-bit wrapping operations. It constructs the unsigned division
numerator as a modulo-32-bit multiplication by `0x4000`, reproducing the
original `DX:AX` bit pattern without an LP64 `unsigned long` or undefined
negation of `-32768`. The quotient is converted back through an explicit
16-bit signed bit pattern.

The Borland DOS ABI and ordinary-input results remain unchanged; the new types
and operations make those semantics explicit for future hosted builds.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char`, warnings as errors, and
  undefined-behavior sanitization verifies both function-pointer ABIs,
  wrapping absolute values, and representative positive, negative, and
  `-32768` `DX:AX` numerator patterns.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice46_polar_radius.txt`, byte-identical to the clean
  Phase 1 slice 45 report.
