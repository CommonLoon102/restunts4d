# Assembly-to-C migration: Phase 1, slice 108

## Defined paused replay projection arithmetic

Phase 1 slice 108 gives the paused replay steering-distance calculation in
`update_gamestate()` explicit legacy semantics. Each 32-bit player coordinate
is arithmetic-shifted by six, narrowed to its low signed word, and subtracted
from the signed-byte-indexed track center with word wraparound. The two
fixed-point projections then add with word wraparound before the original
signed comparison against 228.

This matches the original six `sar`/`rcr` pairs, word `sub`, word `add`, and
signed `jle` without implementation-defined signed shifts, signed overflow,
or plain-`char` dependencies in hosted builds. Borland definitions retain the
native expressions.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helper with `-funsigned-char`. It checks every track-center word
  across 16 double-word shift boundaries and one million deterministic random
  pairs against an independent arithmetic-shift reference: 2,048,576 results
  pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object remains 44,615 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helper.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice108_paused_replay_projection.txt`. It is empty
  and byte-identical to the clean Phase 1 slice 107 report.
