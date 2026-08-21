# Assembly-to-C migration: Phase 1, slice 65

## Defined player-physics speed scaling

Phase 1 slice 65 replaces both host-`long` speed-scaling expressions in
`update_player_state()` with one exact legacy helper. The initialization path
selects the original `0x1E00` or `0x3C00` divisor from the frame rate, and the
collision path uses the same `0x3C00` calculation before calling
`state_op_unk()`.

The helper zero-extends the car's speed word, multiplies it by `0x0580` with
defined modulo-32-bit arithmetic, performs the original unsigned double-word
division, and returns the quotient's low signed word. The layout-sensitive
`var_pSpeed2Scaled` local is now explicitly `legacy_s16`; this remains the same
single stack word in the Borland DOS build while preventing hosted `int` width
from changing later arithmetic.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test compiles the
  production helper with a forced exact prototype. It exhaustively checks all
  65,536 speed words against 14 nonzero divisors, including both real
  constants, and checks 1,000,000 deterministic random speed/divisor pairs
  against an independent unsigned 64-bit reference.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The eight pre-existing `stateply.c` warnings are
  unchanged, and the new helper adds no warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice65_player_speed_scaling.txt` and is
  byte-identical to the clean Phase 1 slice 64 report.
