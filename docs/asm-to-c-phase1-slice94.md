# Assembly-to-C migration: Phase 1, slice 94

## Defined player progress counters

Phase 1 slice 94 gives three progress updates in `player_op()` explicit legacy
semantics. The unsigned 16-bit player speed is added to the 32-bit travel
distance with double-word wraparound, matching the original word `add` plus
zero-extended `adc`. The missed-route counter and route-confirmation counter
now increment their byte bit patterns with 8-bit wraparound.

Hosted builds no longer risk signed 32-bit overflow or signed-byte increment
overflow. Borland definitions retain the original compound addition and byte
increments.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char` and checks 327,936 results: every
  speed word against five signed and boundary travel distances, plus all 256
  byte increment patterns.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `state.c` retains the same five pre-existing
  warnings and its DOS object remains exactly 15,162 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice94_player_progress_counters.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 93 report.
