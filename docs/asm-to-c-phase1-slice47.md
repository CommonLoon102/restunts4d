# Assembly-to-C migration: Phase 1, slice 47

## Defined polar-angle word semantics

Phase 1 slice 47 converts `polarAngle()` to an explicit one-word signed ABI in
both public declarations and its implementation. Its input negations,
coordinate swap, quotient rounding, and eight quadrant transformations now use
exact legacy word and double-word types with explicit wrapping operations.

The table index is derived from the original unsigned fixed-point quotient:
the high quotient byte selects the entry and the low byte rounds it upward at
`0x80`. Quadrant results reproduce the original `NEG`, high-byte add/subtract,
and final `AX` bit patterns without host-width `int` arithmetic. A denominator
check also removes C division-by-zero undefined behavior for the pathological
wrapped `-32768` case.

The original routine leaves its return register unspecified for `(0, 0)`, and
the former C translation read an uninitialized local. The portable C behavior
is now deliberately defined as zero. The complete replay collection confirms
that this deterministic choice does not alter observed game state.

The Borland DOS ABI remains unchanged because `legacy_s16` is the original
one-word signed type.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char`, warnings as errors, and
  undefined-behavior sanitization verifies the function ABI, wrapping
  `-32768` negation, quotient rounding, and all eight quadrant transforms.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice47_polar_angle.txt`, byte-identical to the clean
  Phase 1 slice 46 report.
