# Assembly-to-C migration: Phase 1, slice 49

## Defined vector projection word semantics

Phase 1 slice 49 replaces the host-width projection arithmetic in
`vector_to_point()` with the exact word and double-word operations used by the
original routine. Projection coordinates and projection state are now declared
with explicit legacy word types on every host.

The shared coordinate helper reproduces the original unsigned `MUL`, the
rounded doubled high-word limit, its signed depth comparison, unsigned `DIV`,
word negation and addition, and the signed-overflow clamp selection. It also
handles `-32768` by its 16-bit bit pattern instead of overflowing a signed C
negation. Nonpositive depth returns the original `0x8000` sentinel in both
coordinates.

This removes the previous implementation-defined signed shifts, host `long`
dependency, signed-overflow exposure, and four dead `fatal_error()` conditions
whose comparison precedence did not match the original sign-bit test.

The Borland DOS ABI and data layout remain unchanged because `legacy_u16` is
the original one-word unsigned type.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char`, warnings as errors for
  the tested projection path, and undefined-behavior sanitization checks
  directed boundary cases plus 250,000 deterministic randomized cases against
  an independent instruction-level reference. Both clamp directions, signed
  addition overflow, ordinary projection, `-32768`, and nonpositive depth are
  covered.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice49_vector_projection.txt`, byte-identical to the
  clean Phase 1 slice 48 report.
