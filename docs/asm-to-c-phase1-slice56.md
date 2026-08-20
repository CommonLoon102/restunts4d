# Assembly-to-C migration: Phase 1, slice 56

## Corrected rectangle predicate semantics

Phase 1 slice 56 gives `rect_compare_point()`, `rect_intersect()`,
`rect_is_inside()`, `rect_is_overlapping()`, and `rect_is_adjacent()` explicit
signed one-word results. The point-classification flag is now an explicit
unsigned byte before its original word return.

Instruction-level comparison also identified and corrected two translation
errors. The original `rect_intersect()` rejects the first rectangle only when
`right < left`; the C translation used `right <= left` and therefore rejected
a zero-width first rectangle too early. The reverse horizontal adjacency test
now correctly compares `r2->right` with `r1->left`; it previously compared
`r2->right` with `r2->left`.

The Borland DOS ABI and packed rectangle layout remain unchanged because the
explicit result type is the original signed word.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test with `-funsigned-char`, new warnings treated as errors,
  and undefined-behavior sanitization compares 500,000 deterministic inputs
  with independent references for point classification, intersection and its
  mutation, containment, overlap, and adjacency. Directed cases cover both
  corrected translation errors.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice56_rectangle_predicates.txt`, byte-identical to
  the clean Phase 1 slice 55 report.
