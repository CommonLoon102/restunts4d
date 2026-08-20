# Assembly-to-C migration: Phase 1, slice 21

## Fixed-width car-state layout

Phase 1 slice 21 converts the complete packed `CARSTATE` definition to the
fixed-width legacy types. Signed word fields use `legacy_s16`; speed and ratio
fields which were already declared unsigned use `legacy_u16`; and the trailing
flags, counters, surface codes, and unknown bytes use `legacy_s8` to retain the
Borland DOS plain-`char` semantics on hosts with a different default.

The nested vectors already acquired fixed-width fields in slice 18. Together,
these declarations now describe the complete assembly-owned structure without
depending on host `char` or `short` behavior. A C89 compile-time assertion
enforces the assembly layout size of 208 bytes (`0xD0`, with `field_CF` at the
last byte).

This slice changes stored types and validates layout; it does not yet rewrite
all arithmetic performed on car-state fields. On the Borland compiler the new
aliases resolve to the same machine types as the previous declarations, so the
DOS ABI and assembly offsets remain unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies the
  208-byte structure size and signed trailing-byte behavior.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle without a layout assertion failure or new relevant
  warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair has matching
  MD5 hashes; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports zero active inline assembly,
  zero preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice21_carstate_layout.txt`, byte-identical to the
  clean Phase 1 slice 20 report.
