# Assembly-to-C migration: Phase 1, slice 81

## Defined direct signed-word negations

Phase 1 slice 81 converts 18 direct signed-word negations in
`update_player_state()` to exact legacy word semantics. The converted sites
cover rotation arguments, pseudo-gravity, vector-axis swaps, polar-angle
arguments, and two control-flow comparisons. Negating the `0x8000` bit pattern
now consistently returns `0x8000`, including in those comparisons, instead of
depending on the hosted integer-promotion width.

Compound expressions that combine negation with another add, subtract, or
shift remain outside this bounded slice. A stateply-specific compatibility
macro maps hosted builds to the fixed-width word primitive while retaining the
original unary-minus expression under Borland, so no DOS helper or local is
introduced.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test exhaustively checks
  all 65,536 input word patterns against an independent two's-complement
  negation reference. It also checks the affected signed comparison and the
  double-negation round trip. The complete production `stateply.c` compiles
  with sanitizer instrumentation.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `stateply.c` retains the same eight pre-existing
  missing-prototype warnings, and its DOS object remains exactly 20,862 bytes.
- Exactly twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the compatibility macro.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice81_signed_word_negations.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 80 report.
