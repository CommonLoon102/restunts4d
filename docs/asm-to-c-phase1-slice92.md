# Assembly-to-C migration: Phase 1, slice 92

## Defined penalty accumulator arithmetic

Phase 1 slice 92 gives the penalty update in `player_op()` explicit legacy
word and byte semantics. The route penalty count is multiplied by the frame
rate, the low product word is multiplied by three with 16-bit wraparound, the
display duration is the low byte of frame rate times four, and the accumulated
game penalty is a wrapped 16-bit sum.

The assembly-backed `penalty_time` declaration is now explicitly signed
16-bit and `show_penalty_counter` is explicitly unsigned 8-bit. These match the
existing DOS storage and remove hosted `int`-width and plain-`char` dependencies.
Borland definitions retain the original direct expressions and compound
assignment.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char` and checks 524,288 results: every
  penalty-count word at both supported frame rates, every frame-rate word for
  the display byte, and five boundary addends across every accumulated penalty
  word.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `state.c` retains the same five pre-existing
  warnings and its DOS object remains exactly 15,160 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice92_penalty_accumulator_arithmetic.txt`. It is
  empty and byte-identical to the clean Phase 1 slice 91 report.
