# Assembly-to-C migration: Phase 1, slice 36

## Exact 16-bit physics stack-word buffers

Phase 1 slice 36 gives the physics compatibility buffers the width of the DOS
stack words they model. `legacy_wheel_angle_stack_words` and
`legacy_grip_stack_words` are now shared `legacy_s16[4]` declarations rather
than translation-unit-specific `int[4]` declarations. They remain eight bytes
under Borland and are no longer widened to sixteen bytes on hosted targets.

The related saved wheel-plane angles, translated wheel-contact locals, and
penalty traversal branch stacks now use the same exact 16-bit representation.
The penalty branch arrays are particularly important because entries 114
through 117 intentionally carry four words of historical stack residue into
the following physics update.

DOS stack reads and the caller-frame pointer use `legacy_u16` for raw word
access. Values are then stored in the signed buffers with their original
two's-complement bit patterns. No calling convention or Borland stack layout
changes because both legacy aliases resolve to the original 16-bit types in
the DOS build.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU C89 test compiled with `-funsigned-char` and warnings as errors
  verifies that each four-word compatibility buffer occupies eight bytes,
  that a 128-entry branch stack occupies 256 bytes, and that signed high-bit
  word patterns are preserved.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with unchanged DOS word and stack sizing.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice36_stack_word_buffers.txt`, byte-identical to the
  clean Phase 1 slice 35 report.
