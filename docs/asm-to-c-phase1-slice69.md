# Assembly-to-C migration: Phase 1, slice 69

## Defined signed speed-difference word semantics

Phase 1 slice 69 replaces `update_car_speed()`'s host-dependent
`car_speed2 - car_lastspeed` assignment with a reusable exact word-difference
macro. The original subtraction occurs in a 16-bit register and stores that
bit pattern in the signed `car_speeddiff` word.

The shared macro wraps the subtraction explicitly at 16 bits and converts the
resulting two's-complement bit pattern with `LEGACY_S16_FROM_BITS()`. This
avoids both the hosted integer-promotion difference and an implementation-
defined out-of-range conversion to a signed short. Keeping this primitive as
an expression also avoids adding a function call to the per-frame physics
path.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test checks the
  production macro for 2,572,864 input pairs against an independent
  modulo-16-bit signed reference: two complete 65,536-word sweeps for each of
  12 boundary values, plus 1,000,000 deterministic pairs.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `statecar.c` compiles with zero warnings.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- An initial out-of-line formulation produced no mismatches but made 93 long
  replays exceed the service's ten-second limit. Its diagnostic report is
  preserved as
  `partitions_all_phase1_slice69_signed_speed_difference_helper_timeouts.txt`.
  The final inline macro restores the original hot-path performance.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice69_signed_speed_difference.txt` and is
  byte-identical to the clean Phase 1 slice 68 report.
