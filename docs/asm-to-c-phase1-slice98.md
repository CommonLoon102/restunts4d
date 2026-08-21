# Assembly-to-C migration: Phase 1, slice 98

## Defined statecar byte-counter updates

Phase 1 slice 98 gives four byte updates in `update_car_speed()` explicit
legacy semantics. The engine-limiter timer, current gear, and gear-change
timer now increment or decrement their complete 8-bit patterns with
wraparound, matching the original byte `inc` and `dec` instructions.

Hosted builds no longer risk signed-byte overflow. Borland definitions retain
the original direct post-increment and post-decrement expressions, and no
structure or stack layout changes.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char` and checks increment and decrement
  across all 256 byte patterns: 512 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `statecar.c` has zero warnings and its DOS object
  remains exactly 8,063 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice98_statecar_byte_counters.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 97 report.
