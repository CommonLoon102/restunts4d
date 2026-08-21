# Assembly-to-C migration: Phase 1, slice 68

## Defined unsigned speed-word averaging

Phase 1 slice 68 replaces `update_car_speed()`'s host-`long` speed average
with an exact legacy helper. The original code adds two unsigned speed words
into a zero-initialized `DX:AX` pair, propagates the carry into `DX`, and then
right-shifts the resulting 17-bit sum through `DX` and `AX`.

The C helper reproduces that sequence with an explicit legacy double-word
sum, a logical right shift, and low-word narrowing. Consequently sums above
65,535 retain their carry bit rather than wrapping before the division by two,
and no host `long` width or signed-shift behavior is involved.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test compiles the
  production helper with a forced exact prototype. It checks 2,572,864 input
  pairs against both an independent `DX:AX` carry/rotate reference and a wide
  arithmetic reference: two complete 65,536-word sweeps for each of 12
  boundary values, plus 1,000,000 deterministic pairs.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `statecar.c` compiles with zero warnings.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice68_speed_average.txt` and is byte-identical to
  the clean Phase 1 slice 67 report.
