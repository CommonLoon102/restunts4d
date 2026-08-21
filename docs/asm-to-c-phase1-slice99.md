# Assembly-to-C migration: Phase 1, slice 99

## Defined crash timing word arithmetic

Phase 1 slice 99 gives the timing updates in `update_crash_state()` explicit
legacy-word semantics. Crash display time now multiplies the frame-rate word
by four with 16-bit wraparound. Player finish time adds frame, penalty, and
elapsed-time words sequentially with wraparound, and the opponent finish field
uses the corresponding wrapped two-word sum.

This mirrors the original word `shl` and `add` instructions and removes
dependencies on host `int` width, signed overflow, and signed narrowing.
Borland definitions retain the original direct expressions.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char` and checks 2,031,616 results across
  every base word and five boundary operands for the two- and three-word sums.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `statecrs.c` retains its five pre-existing warnings
  and its DOS object is 4,841 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice99_crash_timing_words.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 98 report.
