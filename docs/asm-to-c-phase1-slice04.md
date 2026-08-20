# Assembly-to-C migration: Phase 1, slice 04

## DOS conventional-memory boundary

Phase 1 slice 04 removes three active inline-assembly blocks from
`src/restunts/c/memmgr.c`: querying the program segment prefix, allocating a
DOS paragraph block, and resizing a DOS paragraph block.

The C implementation uses Borland's `_intdos`, `_intdosx`, and `_segread`
runtime interfaces already linked by the earlier DOS-boundary phases. These
modules remain DOS platform support and can be replaced with the allocator of
a future non-DOS backend. No additional runtime objects are needed, and no
`seg0xx.asm` source was changed.

This deliberately does not convert `pushregs`/`popregs` or the register
preservation code around memory release. Those sites encode compatibility with
callers that still cross the original assembly boundary and belong in a later
ABI-focused chunk.

## Behavior preserved

- The PSP helper still constructs the same `DS:BX` far-pointer value as the
  previous wrapper after DOS function `62h`.
- Allocation still returns DOS's raw `AX` value after function `48h`, including
  the existing failure behavior.
- SETBLOCK still supplies the memory segment through `ES` and returns the raw
  `BX` maximum-block value after function `4Ah`.
- The allocator's two-call probe-and-resize sequence is unchanged.

## Verification

- The DOS compiler builds `memmgr.c`, and both `RESTUNTS.EXE` and
  `REPLDUMP.EXE` link successfully. The only compiler warning is the existing
  incompatible `abs` declaration in `externs.h`.
- The Phase 0 audit reports 36 active inline-assembly sites, down from 39, and
  the checked-in inventory is current.
- No ad-hoc local replay was run for this slice; the comprehensive serial
  remote collection is its replay regression gate.
- The comprehensive remote collection reports zero mismatches and is
  byte-identical to the Phase 1 slice 03 clean baseline. Its preserved result is
  `stunts/partitions_all_phase4_memmgr_dos_c.txt`; an empty result file means
  that every replay matched.

The serial remote replay collection is the comprehensive regression gate for
this slice, and it passed.
