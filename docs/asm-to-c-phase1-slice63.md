# Assembly-to-C migration: Phase 1, slice 63

## Defined RPM-update word semantics

Phase 1 slice 63 gives `update_rpm_from_speed()` the exact five-word interface
used by its C and preserved-assembly callers. Current RPM, speed, gear ratio,
gear-change state, idle RPM, and the returned value now have explicit
`legacy_u16` types on every compiler.

When the car is not changing gear, the function now multiplies the two input
words with defined legacy double-word arithmetic and selects the product's
high word, reproducing the original unsigned `MUL` and `MOV CX,DX` sequence.
The idle-RPM floor remains an unsigned word comparison, matching the original
`JNB`. The result no longer depends on host `unsigned int` or `unsigned long`
widths.

The typedefs preserve the original DOS far-call ABI, including the call from
the protected assembly implementation. No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer test forces an exact
  prototype on the production translation unit and compares the function with
  an independent 64-bit reference. It exhaustively checks every speed word
  against 17 representative gear ratios, exhaustively covers clamp and
  gear-change word patterns, and checks 1,000,000 deterministic random cases.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `statecar.c` compiles with zero warnings.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice63_rpm_update.txt` and is byte-identical to the
  clean Phase 1 slice 62 report.
