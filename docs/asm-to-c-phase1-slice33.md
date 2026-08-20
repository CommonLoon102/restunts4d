# Assembly-to-C migration: Phase 1, slice 33

## Recovered hidden track word tables

Phase 1 slice 33 corrects three allocated track-data regions that were still
declared as byte pointers even though the original assembly consistently
accesses them as word arrays. The opponent route (`trackdata3`) and the two
64-word calculation tables (`trackdata6` and `trackdata7`) are now
`legacy_s16 far *`.

Both `init_trackdata()` implementations explicitly cast the corresponding
byte-buffer boundaries to the recovered word types. The byte offsets and
allocated region sizes remain unchanged; indexing now advances by the intended
two-byte DOS word on every target.

The C opponent-route builder no longer casts `trackdata3` locally because its
shared declaration now carries the correct type. More importantly, active C
opponent initialization now indexes the route as words, matching the original
assembly's index shift and word load rather than reading a single byte from
the route buffer.

The new declarations resolve to the same pointer and element representation
under Borland, so the mixed DOS ABI remains unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies that all
  three recovered table pointees are signed two-byte values.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the typed buffer boundaries and word-indexed
  opponent route.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice33_hidden_track_words.txt`, byte-identical to the
  clean Phase 1 slice 32 report.
