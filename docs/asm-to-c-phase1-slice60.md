# Assembly-to-C migration: Phase 1, slice 60

## Defined plane-rotation word semantics

Phase 1 slice 60 rewrites `plane_rotate_op()` as the two structured branches
of the original routine: selected-plane rotation and the no-plane fallback.
The obsolete embedded assembly transcript and goto labels are removed, while
the original plane-orientation shortcut, inverse-rotation path, two rotation
caches, and matrix/vector operation order are preserved.

The selected plane index, three saved orientation values, wheel angle, local
angle, and both cache keys are now explicit signed legacy words. The original
word `ADD` and `NEG` instructions use wrapping helpers, including the
`0x8000` boundary. The wheel-angle producer in `stateply.c` now reproduces the
two arithmetic right shifts and wrapping subtraction explicitly, and the
matching declarations in `stateply.c` and `frame.c` use the same one-word ABI.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char`, warnings treated as
  errors, and address/undefined-behavior sanitizers compares 100,000 random
  selected-plane/no-plane, orientation-match/mismatch, and cache-hit/miss
  cases with an independent structured reference. Directed cases cover
  `0x7FFF + 1`, `0x8000 + 0x8000`, zero-angle bypasses, and cached `0x8000`
  rotation keys.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. `math.c` has no warnings and the other changed
  translation units introduce no new warnings.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice60_plane_rotation.txt` and is byte-identical to
  the clean Phase 1 slice 59 report.
