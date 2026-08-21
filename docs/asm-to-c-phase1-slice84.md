# Assembly-to-C migration: Phase 1, slice 84

## Defined direct signed-word shifts

Phase 1 slice 84 converts the two remaining direct word shifts in active
`update_player_state()` code to exact legacy semantics. The steering response
now wraps its left shift at 16 bits, including negative inputs and overflow.
The track-column calculation now performs a defined 16-bit arithmetic right
shift by ten rather than relying on the hosted compiler's treatment of a
negative signed right operand.

The shared primitives use unsigned bit operations in hosted builds. Their
Borland definitions retain the original source expressions, so they do not
alter code generation in the layout-sensitive DOS object.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test exhaustively checks
  all 65,536 input word patterns against independent shift references:
  131,072 results in total. The complete production `stateply.c` also compiles
  with sanitizer instrumentation and links with the test harness.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `stateply.c` retains the same eight pre-existing
  missing-prototype warnings, and its DOS object remains exactly 20,862 bytes.
- Exactly twenty local replay attempts were made without retries. Seventeen
  completed with byte-identical `.BIN`/`.BNI` files and matching MD5 hashes;
  `0092.rpl`, `1408.rpl`, and `3648.rpl` were killed by the harness's fixed
  ten-second local timeout. No completed comparison differed, and all partial
  artifacts were removed.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice84_direct_word_shifts.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 83 report, confirming the local
  incomplete attempts were host-side timeout noise.
