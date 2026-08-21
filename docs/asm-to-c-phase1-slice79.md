# Assembly-to-C migration: Phase 1, slice 79

## Defined wheel interpolation arithmetic

Phase 1 slice 79 converts the six wheel-position interpolation expressions in
`update_player_state()` to exact legacy signed double-word semantics. Each
expression wraps the 32-bit position difference, retains the low 32 bits of
the signed-word multiplication, divides signed magnitudes with truncation
toward zero, reapplies the result sign, and stores the quotient's low word.
The three word calculations that produce the interpolation factors and one
denominator now also wrap before use.

This matches the original `__aFlmul` and `__aFldiv` runtime routines, including
their two's-complement `INT32_MIN / -1` result of `0x80000000`. Hosted builds
use a static bit-pattern helper without signed-overflow undefined behavior.
Borland excludes that helper and retains the original native expression, so
the DOS build continues to call its existing long multiply/divide runtime with
no added stack local or code footprint.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test links the production
  interpolation helper and checks 2,126,975 cases against an independent
  wide-integer reference. It covers signed double-word and word boundaries,
  every nonzero 16-bit divisor pattern, modulo multiplication overflow,
  `INT32_MIN / -1`, and two million deterministic pseudo-random cases. The
  complete production `stateply.c` also compiles with sanitizer
  instrumentation.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `stateply.c` retains the same eight pre-existing
  missing-prototype warnings, and its DOS object remains exactly 20,862 bytes.
- Exactly twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory's
  `update_player_state()` source location is refreshed for the hosted helper.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice79_wheel_interpolation.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 78 report.
