# Assembly-to-C migration: Phase 1, slice 58

## Defined rectangle-list byte semantics

Phase 1 slice 58 gives `rectlist_add_rect()` and `rectlist_add_rects()` the
byte widths and signedness used by the original executable. Rectangle-list
lengths and the local shift positions are explicit signed bytes, while the
combined-list count and per-rectangle flag bytes are explicit unsigned bytes.
The combined-list routine still reproduces the original signed extension of
its byte counter when it forms an array offset.

The shift-index and list-length byte `INC`/`DEC` operations in
`rectlist_add_rect()` now wrap through an unsigned byte bit pattern before
converting back to a signed byte. This removes dependence on host `int` width,
plain-`char` signedness, and signed-overflow behavior. The rectangle-list
declarations are exposed in `math.h` with the same byte-level ABI, and an
unused pointer local was removed.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char`, warnings treated as
  errors, and address/undefined-behavior sanitizers compares 100,000 direct
  rectangle-list additions and 100,000 combined/clipped additions with an
  independent reference. Typed function pointers also verify both public
  signatures.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS byte ABI and no warnings from
  `math.c`.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice58_rectangle_list_bytes.txt` and is
  byte-identical to the clean Phase 1 slice 57 report.
