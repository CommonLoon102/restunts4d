# Assembly-to-C migration: Phase 1, slice 72

## Defined RPM-limiter word arithmetic

Phase 1 slice 72 converts `update_car_speed()`'s RPM-limiter block to exact
legacy word semantics. Both RPM differences now wrap at 16 bits before their
signed comparisons with 2,000 RPM.

The idle-torque/gearing test now retains only the low word of the original
unsigned multiplication and interprets that word as signed for the following
comparison. The limiter's `car_speed2` reduction wraps explicitly as an
unsigned word. Hosted builds use the fixed-width legacy primitives, while
Borland retains native compound assignment for the original DOS performance.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test exhaustively checks
  all 16,777,216 idle-torque/gear-ratio combinations against an independent
  low-word signed-product reference. It also checks 2,572,864 RPM boundary
  pairs for both wrapped difference directions and wrapped speed reduction.
  The complete production `statecar.c` compiles under the same sanitizers.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `statecar.c` compiles with zero warnings and retains
  the clean 8,062-byte DOS object footprint.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice72_rpm_limiter.txt` and is byte-identical to the
  clean Phase 1 slice 71 report.
