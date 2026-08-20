# Assembly-to-C migration: Phase 6

## Stale fallback cleanup

Phase 6 removes three obsolete `ported_*` fallback declarations from active C
sources:

- `ported_mmgr_alloc_pages_`
- `ported_mmgr_get_chunk_by_name_`
- `ported_sub_204AE_`

None of these symbols had a live C call. The two memory-manager functions and
`sub_204AE` already execute their C implementations; the declarations were
leftovers from earlier translation work. The stale commented fallback call in
`sub_204AE` is removed with its declaration.

No executable statement, linker input, or `seg0xx.asm` source was changed.

## Verification

- The DOS compiler rebuilds `memmgr.c` and `shape3d.c`, and both ported
  executables link successfully with only their existing warnings.
- `REPLDUMP.EXE` is byte-for-byte identical before and after the cleanup, with
  SHA-256
  `2abd15ee479c2ff9507d41529cdb8210d4161a508ed7217c67ae73298330ad5a`.
- Because the executable is exactly the one already validated by Phase 5, the
  Phase 5 comprehensive remote result is also the regression proof for this
  source-only cleanup. No additional serial request is necessary.
- The Phase 0 audit reports four preserved-assembly symbol references, down
  from seven, and the checked-in inventory is current.

The four remaining preserved-symbol references are live compatibility paths:
one timing-sensitive sprite clear and three stack-sensitive calls to the
original car-speed routine.
