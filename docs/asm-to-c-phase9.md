# Assembly-to-C migration: Phase 9

## DOS compiler-ABI boundary

Phase 9 removes all four active inline-assembly sites from `memmgr.c`. The
memory-manager behavior remains implemented in C; a small DOS-only external
shim now preserves the register behavior on which legacy assembly callers
depend.

The Borland C calling convention permits the compiler to clobber `DX`, while
the original memory-manager routines happened to preserve it. The original
`mmgr_release` routine also preserved `BX`. Removing the inline pushes without
an ABI boundary therefore would be source-correct C but could change the
machine-level contract seen by callers that have not yet been ported.

`dos/regshim.asm` contains only forwarding wrappers:

- `mmgr_path_to_name`, `copy_paras_reverse`, `mmgr_find_free`, and
  `mmgr_resize_memory` preserve `DX`;
- `mmgr_release` preserves both `BX` and `DX`;
- arguments are forwarded to C implementations using the compiler's medium
  memory-model convention, and return values pass through unchanged;
- saved registers and arguments live only on the CPU stack, so the wrappers
  do not add shared or non-reentrant state.

The shim is linked only into the ported DOS targets. Non-DOS builds export the
C functions directly, and the original replay oracle's link set remains
unchanged. This is a temporary DOS compiler-ABI boundary, not game logic; it
can disappear with the DOS backend when a later SDL/native target no longer
has assembly callers.

No `seg0xx.asm` source was changed.

## C cleanup

`copy_paras_reverse` now indexes the source and destination through explicit
far addresses while walking the offset backwards. This avoids compiler
register-allocation differences caused by keeping two far pointer locals,
without changing the copied range or overlap-safe reverse order.

The other four C bodies retain their existing control flow. Only the inline
register-save operations and their now-empty non-DOS substitutes were
removed.

## Verification

- The DOS compiler and assembler build the C implementations and ABI shim,
  and both `RESTUNTS.EXE` and `REPLDUMP.EXE` link successfully with no new
  warnings in the changed code.
- The Phase 0 audit reports 31 active inline-assembly sites, down from 35.
- The checked-in inventory is current.
- The comprehensive serial remote collection is the replay regression gate;
  no additional ad-hoc local replays were run after reaching the requested
  local-run limit in Phase 3.

- The comprehensive remote collection reports zero mismatches and is
  byte-identical to the Phase 8 clean baseline. Its preserved result is
  `stunts/partitions_all_phase9_memmgr_abi_c.txt`; an empty result file means
  that every replay matched.

The serial remote replay collection is the comprehensive regression gate for
this slice, and it passed.
