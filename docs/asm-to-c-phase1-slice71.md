# Assembly-to-C migration: Phase 1, slice 71

## Defined speed-integration word arithmetic

Phase 1 slice 71 gives `update_car_speed()`'s coupled integration locals their
exact legacy widths: signed words for step, difference, and acceleration, an
unsigned word for accumulated speed, and an unsigned byte for torque.

On hosted and modern compilers, the complete integration path explicitly
wraps aerodynamic and braking subtraction, doubled braking, frame-rate
acceleration doubling, free-rev boost, RPM decrements, unsigned speed
accumulation, and the collision speed difference and negation. Borland uses
equivalent native 16-bit expressions and compound assignments to retain the
original per-frame DOS performance. The negative-delta stopping test compares
the original wrapped word magnitude as unsigned, including the `0x8000` case.
This removes hosted integer-promotion and signed-conversion dependencies from
the block while retaining the original control flow.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test checks 2,572,864
  speed/delta boundary pairs for accumulation, doubling, boost, and the
  unsigned stopping comparison. It also checks 1,000,000 deterministic
  integration tuples for doubled braking and collision-difference magnitude
  against independent register-bit references. The complete production
  `statecar.c` compiles under the same sanitizers.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `statecar.c` compiles with zero warnings.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- An initial formulation used the portable expression macros under Borland as
  well. It produced no mismatches but made 35 long replays exceed the
  service's ten-second limit; the diagnostic report is preserved as
  `partitions_all_phase1_slice71_speed_integration_portable_macro_timeouts.txt`.
  The final Borland compatibility expressions restore `statecar.obj` and the
  DOS executables to their clean Slice 70 sizes.
- Two subsequent identical-binary submissions encountered remote-worker load.
  Their failures moved between unrelated replay partitions and included 11
  and 33 timeouts respectively in the unchanged original oracle, alongside
  21 and 31 in the ported executable. These service-noise reports are
  preserved as
  `partitions_all_phase1_slice71_speed_integration_native_macro_timeouts.txt`
  and
  `partitions_all_phase1_slice71_speed_integration_service_noise_rerun.txt`.
  After a cooldown, the identical executable hashes completed cleanly,
  confirming that the mixed timeouts were infrastructure noise.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice71_speed_integration.txt` and is byte-identical
  to the clean Phase 1 slice 70 report.
