# Assembly-to-C migration: Phase 1, slice 50

## Defined projection setup word semantics

Phase 1 slice 50 converts `set_projection()` and
`projectiondata9_times_ratio()` to explicit legacy word and double-word
arithmetic. Their declarations now expose the original unsigned one-word ABI
instead of depending on Borland's 16-bit `int` and `unsigned` widths.

Projection angle construction reproduces the original 32-bit left shift,
unsigned word division, and logical word shift. Screen dimensions and centers
use logical shifts and explicit wrapping additions. Projection scales and the
ratio helper use unsigned 16-by-16 multiplication followed by unsigned
division, preserving the bit patterns returned by the signed trig functions.
The zero-vertical-angle fallback now performs both subtractions with explicit
word wrapping before passing signed word views to `polarAngle()`.

The Borland DOS ABI and data layout remain unchanged because each
`legacy_u16` parameter and result is one word in the original memory model.

No `seg0xx.asm` source was changed.

## Verification

- A targeted hosted GNU89 test compiled with `-funsigned-char`, warnings as
  errors, and undefined-behavior sanitization checks 200,000 deterministic
  randomized projection setups plus all 65,535 nonzero ratio numerators. It
  covers both vertical-angle branches, logical shifts, word wrapping,
  unsigned multiplication/division, and signed trig return bit patterns.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice50_projection_setup.txt`, byte-identical to the
  clean Phase 1 slice 49 report.
