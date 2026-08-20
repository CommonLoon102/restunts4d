# Assembly-to-C migration: Phase 1, slice 22

## Fixed-width game-state layout

Phase 1 slice 22 converts the complete packed `GAMESTATE` definition to the
fixed-width legacy types. Position-history arrays use `legacy_s32`; signed
word fields use `legacy_s16`; impact and top-speed fields which were already
declared unsigned use `legacy_u16`; and byte arrays, flags, counters, and the
random seed use `legacy_s8` to retain the Borland DOS plain-`char` semantics on
hosts with a different default.

The nested vectors acquired fixed-width fields in slice 18 and the two nested
car states in slice 21. Together, these declarations now describe the complete
assembly-owned game-state structure without depending on host `char`, `short`,
or `long` widths. A C89 compile-time assertion enforces the assembly layout
size of 1120 bytes (`0x460`, with `field_45F` at the last byte).

The assembly declaration stores the 48 bytes at `field_38E` as raw bytes. The
C declaration retains the existing 24-word interpretation because its users
treat the data as words; both representations occupy the same 48 bytes and
therefore preserve all following offsets.

The Kevin random-seed helpers now accept explicit `legacy_s8` pointers. This
does not change their DOS pointer ABI, but makes the six stored seed bytes'
signed behavior independent of the hosted compiler's plain-`char` default.

This slice changes stored types and validates layout; it does not yet rewrite
all arithmetic performed on game-state fields. On the Borland compiler the new
aliases resolve to the same machine types as the previous declarations, so the
DOS ABI and assembly offsets remain unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies the
  1120-byte structure size and signed trailing-byte behavior.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle without a layout assertion failure or new relevant
  warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair has matching
  MD5 hashes; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice22_gamestate_layout.txt`, byte-identical to the
  clean Phase 1 slice 21 report.
