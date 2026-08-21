# Assembly-to-C migration: Phase 1, slice 95

## Defined route-angle word arithmetic

Phase 1 slice 95 gives the remaining route-angle arithmetic in `player_op()`
explicit legacy-word semantics. The two route-vector differences and the
player-versus-route angle difference now subtract with 16-bit wraparound, the
matrix X component is negated as a complete word bit pattern, and the final
two projected side values are added with word wraparound before their sign is
tested.

Hosted builds no longer depend on promoted host-width arithmetic, signed
narrowing, or signed overflow. Borland definitions retain the original direct
word expressions and compound addition.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char` and checks 720,896 results: all
  65,536 word values against five boundary operands for addition and
  subtraction, plus every word negation.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `state.c` retains the same five pre-existing
  warnings; its DOS object is 15,165 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice95_route_angle_word_arithmetic.txt`. It is empty
  and byte-identical to the clean Phase 1 slice 94 report.
