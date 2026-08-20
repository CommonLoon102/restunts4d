# Assembly-to-C migration: Phase 1, slice 59

## Restored matrix-composition semantics

Phase 1 slice 59 restores the original eight-way axis-selection behavior of
`mat_rot_zxy()`. The function now builds only the nonzero Z, X, and Y axes,
uses the four shared precomputed Y matrices, preserves the normalized Y-angle
cache, skips identity multiplications, follows the original forward/reverse
composition order, and returns the same shared matrix object for each axis
combination. Avoiding the extra identity products also avoids introducing
additional fixed-point rounding.

The three angle arguments are explicit unsigned legacy words and the order
flag is an explicit byte. The matrix multiplication counter, transpose swap
word, and Y-cache word now have their original widths. The DOS-only
wheel-angle compatibility path uses an explicit unsigned double-word speed
product/division, defined arithmetic right shifts, wrapping word subtraction,
and exact word-sized stack offsets. Its caller-frame probe remains in a
one-local public wrapper so the validated Borland stack relationship is not
changed by the restored composition logic.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char`, warnings treated as
  errors, and address/undefined-behavior sanitizers verifies all eight active
  axis combinations in both composition orders, cached and uncached Y paths,
  all three quarter-turn presets, ignored high angle bits, shared-pointer
  identity, and 100,000 randomized cases. It also checks the exact public
  function signature through a typed function pointer.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with no warnings from `math.c`.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice59_matrix_composition.txt` and is byte-identical
  to the clean Phase 1 slice 58 report.
