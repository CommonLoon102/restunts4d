# Assembly-to-C migration: Phase 1, slice 105

## Defined initial-car coordinate arithmetic

Phase 1 slice 105 gives the player and opponent coordinate construction in
`init_game_state()` explicit legacy semantics. Trigonometric offset pairs and
track-center additions now wrap as words before being sign-extended and
scaled by 64 into double-word coordinates. The initial heading uses wrapped
word negation, and the transmission, track indices, hill index, and stored
start coordinates now explicitly preserve the original signed-byte
interpretation.

This matches the original word `add`, `cwd`, six-bit double-word shift,
word `neg`, and `cbw` sequences without hosted signed-overflow or
plain-`char` dependencies. Borland definitions retain the native word and
signed-long expressions.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char`. It checks direct and summed
  word-to-double-word scaling plus wrapped word negation for every 16-bit
  pattern, and all 256 byte sign extensions: 196,864 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object is 44,614 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helper.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice105_initial_car_coordinates.txt`. It is empty
  and byte-identical to the clean Phase 1 slice 104 report.
