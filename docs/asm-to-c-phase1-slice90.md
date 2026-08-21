# Assembly-to-C migration: Phase 1, slice 90

## Defined hidden call-operand arithmetic

Phase 1 slice 90 makes four call operands in `update_player_state()` use
explicit legacy-word arithmetic. Three opponent fallback rotation arguments
now negate their complete 16-bit bit patterns with defined wraparound, and the
collision-state call now defines both the wrapped `si + 2` selector and the
wrapped negation of the player X rotation.

The Borland definitions remain direct expressions so the DOS compiler retains
the original calling and stack behavior. Hosted builds use fixed-width helpers
and do not depend on integer-promotion or signed-overflow behavior.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers and exhaustively checks all 65,536 input bit patterns for
  both operations: 131,072 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `stateply.c` retains the same eight pre-existing
  missing-prototype warnings and its DOS object remains exactly 20,862 bytes.
- Exactly twenty local replay attempts were made. Nineteen completed with
  byte-identical `.BIN`/`.BNI` pairs and matching MD5 hashes. One attempt,
  `0698.rpl`, did not complete; it produced no mismatching pair and was not
  retried under the twenty-attempt cap.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the hosted helper.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice90_hidden_call_operands.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 89 report.
