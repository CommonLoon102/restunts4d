# Assembly-to-C migration: Phase 1, slice 64

## Defined aerodynamic-table arithmetic

Phase 1 slice 64 makes `setup_aero_trackdata()` reproduce the original
signed double-word table calculation without depending on host `long` width
or implementation-defined right shifting. Its opponent selector is also an
explicit legacy word, preserving the original far-call interface.

Each resistance value is sign-extended from its resource word, multiplied
twice by the zero-extended speed word with modulo-32-bit arithmetic, shifted
arithmetically right nine times, and stored from the low signed word. This is
the original pair of signed long multiplies followed by nine `SAR DX`/`RCR AX`
steps. `legacy.h` now provides `LEGACY_U32_SAR1()` for that defined bit-level
operation and later translations.

Both the player and opponent table paths share the same calculation. Resource
copying, the far table pointers, car-name loading, and the 64-entry table size
remain unchanged. No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test forces the exact
  production function prototype and checks both complete setup branches. It
  exhaustively covers all 65,536 resistance words at every speed from 0
  through 63, comparing 8,388,608 generated entries with an independent
  signed 64-bit floor-division reference. It also verifies the SIMD resource
  copy, table pointer, and selected name resource.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The 41 pre-existing `restunts.c` warnings are
  unchanged, and neither changed routine adds a warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice64_aerodynamic_table.txt` and is byte-identical
  to the clean Phase 1 slice 63 report.
