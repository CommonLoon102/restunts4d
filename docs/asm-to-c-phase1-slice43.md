# Assembly-to-C migration: Phase 1, slice 43

## Exact car-state initializer ABI

Phase 1 slice 43 gives `init_carstate_from_simd()` an explicit legacy-width
interface. Its transmission byte is now `legacy_s8`, its three world-position
arguments are `legacy_s32`, and its track angle is `legacy_s16`. A shared
prototype in `externs.h` prevents callers and future backends from silently
selecting host-dependent parameter widths.

The original far routine receives each world coordinate as an `AX:DX` pair,
the angle as one word, and reads the low byte of the transmission argument.
The C callers now also cast their track-coordinate inputs to `legacy_s32`
before scaling them by 64, so that operation no longer widens through LP64
`long`. On Borland DOS these exact types preserve the original stack layout
and generated calling convention.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char` and warnings as errors
  verifies the complete function-pointer ABI, all legacy parameter widths,
  and a representative scale-to-world calculation.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS stack interface.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice43_car_init_coordinates.txt`, byte-identical to
  the clean Phase 1 slice 42 report.
