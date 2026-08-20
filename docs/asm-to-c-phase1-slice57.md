# Assembly-to-C migration: Phase 1, slice 57

## Restored legacy sorting semantics

Phase 1 slice 57 restores the original implementation of
`heapsort_by_order()` and converts its count, key array, and order array to
explicit signed legacy words. Despite its name, the original executable uses a
gap-based descending sort. The former C replacement used a heap sort, which
could produce different order-array results when keys were equal.

The restored algorithm uses the original halving gap sequence and swaps only
when the later signed-word key is strictly greater. `rect_array_sort_by_top()`
now uses a signed-byte count, signed-word key/order buffers, and explicit word
negation of rectangle tops. The transformed-shape key and index declarations
are likewise fixed to their actual one-word storage, and the shared prototype
now lives in `math.h`.

The Borland DOS ABI and data layout remain unchanged because these explicit
types match the original byte and word objects.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test with `-funsigned-char`, new warnings treated as errors,
  and undefined-behavior sanitization compares 100,000 direct word-array sorts
  and 100,000 rectangle-index sorts with an independent instruction-level
  reference. Duplicate-heavy keys verify the original strict-comparison tie
  behavior.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice57_legacy_sorting.txt`, byte-identical to the
  clean Phase 1 slice 56 report.
