# Assembly-to-C migration: Phase 1, slice 45

## Defined fixed-point multiply and scale

Phase 1 slice 45 rewrites `multiply_and_scale()` as explicit legacy bit
operations. The original routine performs a signed 16-by-16 `IMUL`, shifts
the double-word product left twice, shifts the low word once more to obtain a
rounding carry, adds that carry to the high word, and returns the resulting
16-bit pattern.

The C implementation now reproduces that sequence with exact legacy types,
modulo-32-bit multiplication, explicit high/low-word extraction, 16-bit
wrapping addition, and defined bit-pattern-to-signed conversion. It no longer
depends on host `long` width, signed overflow, or implementation-defined
right shifting of negative values. The public declaration also uses
`legacy_s16` for both inputs and the return value.

The Borland DOS ABI remains unchanged because the exact types resolve to the
same one-word arguments and return value used by the original routine.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char`, warnings as errors, and
  undefined-behavior sanitization compares the new bit algorithm with an
  independent 64-bit reference across all pairs from a boundary-value set and
  100,000 deterministic pseudo-random input pairs.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice45_multiply_scale.txt`, byte-identical to the
  clean Phase 1 slice 44 report.
