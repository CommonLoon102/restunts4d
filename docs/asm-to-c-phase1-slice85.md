# Assembly-to-C migration: Phase 1, slice 85

## Defined direct signed-word addition and subtraction

Phase 1 slice 85 converts fourteen direct word-destination additions and
subtractions in `update_player_state()` to exact 16-bit wrapping semantics.
The sites cover suspension adjustment, wall-relative coordinates, wheel
compression, plane-origin coordinates, collision displacement, steering-angle
adjustment, and four start/finish pole coordinates. The four pole calculations
also make their input-angle additions explicitly wrap as unsigned words.

Hosted helper functions accept unsigned word bit patterns, evaluate each
operand once, and reconstruct the signed result without out-of-range signed
conversion. Compound-assignment wrappers preserve the same behavior for word
fields. Under Borland these helpers expand to the original source expressions,
leaving the layout-sensitive DOS object unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production `stateply.c` helpers and checks all 65,536 input words against 32
  boundary operands, followed by 1,000,000 deterministic pseudo-random pairs:
  3,097,152 add/subtract tuples in total.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `stateply.c` retains the same eight pre-existing
  missing-prototype warnings, and its DOS object remains exactly 20,862 bytes.
- Exactly twenty local replay attempts completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. The inventory source location is
  refreshed after adding the hosted helpers.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice85_direct_word_add_subtract.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 84 report.
