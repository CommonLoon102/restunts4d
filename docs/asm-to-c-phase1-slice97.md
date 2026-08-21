# Assembly-to-C migration: Phase 1, slice 97

## Defined gear-change frame-count byte semantics

Phase 1 slice 97 gives the gear-change timer calculation in
`update_car_speed()` explicit legacy byte semantics. The low frame-rate byte
is sign-extended, shifted arithmetically right by one, and then added to the
original byte with 8-bit wraparound before being stored in `car_fpsmul2`.

This mirrors the original `mov al`/`cbw`/`sar ax`/`add al` sequence and removes
dependencies on host word width, unsigned-word shifting, and signed narrowing.
The supported 10 and 20 FPS values still produce their original timers.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helper with `-funsigned-char` and checks all 65,536 frame-rate
  word patterns.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `statecar.c` has zero warnings and its DOS object is
  8,063 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the hosted helper.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice97_gear_change_frame_count.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 96 report.
