# Assembly-to-C migration: Phase 1, slice 73

## Defined signed-word/double-word wheel-position arithmetic

Phase 1 slice 73 converts the direct wheel-position additions and subtractions
in `update_player_state()` to exact legacy word/double-word semantics. The 23
converted operations now sign-extend each 16-bit vector or suspension value,
perform the operation through an unsigned 32-bit bit pattern, wrap at 32 bits,
and convert the result back without relying on hosted signed-overflow behavior.

The shared compatibility layer provides the sign-extension and wrapped
addition/subtraction primitives. Borland retains its native signed-long and
signed-word expressions so the DOS hot path does not acquire helper calls or
additional compatibility temporaries. Expressions that first combine two
16-bit operands remain outside this bounded slice because they require a
separate word-wrap step before the double-word operation.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test checks 2,048,576
  signed-word/double-word cases against an independent sign-extension and
  modulo-32-bit reference. The complete production `stateply.c` also compiles
  with sanitizer instrumentation.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `stateply.c` retains the same eight pre-existing
  missing-prototype warnings as slice 72, with no new warning category or
  count.
- Exactly twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The first serial comprehensive response contained 19 original-oracle and
  13 ported-executable timeouts in contiguous replay bands. It is preserved as
  `partitions_all_phase1_slice73_wheel_position_service_noise.txt`. After a
  three-minute cooldown, the identical executable produced the empty final
  report `partitions_all_phase1_slice73_wheel_position_arithmetic.txt`, which
  is byte-identical to the clean Phase 1 slice 72 report.
