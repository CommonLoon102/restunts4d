# Assembly-to-C migration: Phase 1, slice 67

## Defined opponent-speed damping arithmetic

Phase 1 slice 67 replaces `update_car_speed()`'s host-dependent
opponent-speed damping expressions with exact legacy helpers. The damping
factor now follows the original word subtraction, word negation, logical
shift, and byte narrowing sequence for every possible opponent-speed byte.

The delta-speed helper widens the signed legacy word to an exact legacy
double word and multiplies it by the unsigned damping byte. The product is
bounded from -8,355,840 through 8,355,585, so signed overflow is impossible.
For a negative product, hosted builds divide its nonnegative magnitude by 200
and restore the sign, making truncation toward zero explicit even for pre-C99
compilers. Borland's DOS path uses its verified signed division directly to
retain the original per-frame performance. The helper then wraps the low-word
subtraction. This removes dependencies on host `int` and `long` widths while
keeping the DOS and portable formulations mathematically identical.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test compiles the
  production helpers with forced exact prototypes. It exhaustively checks all
  256 opponent-speed bytes and all 16,777,216 combinations of 65,536
  delta-speed word patterns and 256 damping factors against independent
  fixed-width references.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `statecar.c` compiles with zero warnings.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The first comprehensive run exposed 35 timeout-only regressions on longer
  replays in an exact but expensive unsigned-division formulation. Its report
  is preserved as
  `partitions_all_phase1_slice67_opponent_damping_unsigned_division_timeouts.txt`.
  A compiler-independent signed-magnitude formulation also caused 93
  timeout-only regressions and is preserved as
  `partitions_all_phase1_slice67_opponent_damping_signed_magnitude_timeouts.txt`.
  The final source gives hosted compilers that explicit signed-magnitude path
  and Borland its verified direct signed division. Its DOS executables are
  byte-for-byte identical to the clean direct-division build used by the
  final comprehensive run.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice67_opponent_damping.txt` and is byte-identical
  to the clean Phase 1 slice 66 report.
