# Assembly-to-C migration: Phase 1, slice 54

## Defined trigonometric and axis-rotation word semantics

Phase 1 slice 54 gives `sin_fast()`, `cos_fast()`, `mat_rot_x()`,
`mat_rot_y()`, and `mat_rot_z()` explicit one-word parameter and result types.
The sine table also uses the exact signed-word element type.

Cosine's quarter-turn offset and all negative sine matrix entries now use
explicit wrapping word operations. `sin_fast()` has a defined defensive return
after its exhaustive four-way quadrant switch, removing the hosted compiler's
missing-return warning without changing any reachable angle result.

The Borland DOS ABI and matrix layout remain unchanged because the explicit
types are the original word types.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test with `-funsigned-char`, new warnings treated as errors,
  and undefined-behavior sanitization exhaustively checks all 65,536 angle bit
  patterns for sine, cosine, and the complete X, Y, and Z rotation matrices.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice54_trig_rotations.txt`, byte-identical to the
  clean Phase 1 slice 53 report.
