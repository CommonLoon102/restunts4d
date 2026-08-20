# Assembly-to-C migration: Phase 1, slice 05

## Divide-by-zero vector setup

Phase 1 slice 05 replaces the call from `init_div0()` to the preserved
`ported_init_div0_` routine with its C translation. The routine had already
been transcribed in comments, but was deferred until a reviewed Borland
`intdosx` runtime boundary was available.

The C implementation uses the `_intdosx` and `_segread` modules introduced by
the earlier DOS-boundary phases. It remains conditional on `RESTUNTS_DOS`; a
non-DOS backend does not install a real-mode interrupt vector. No additional
runtime objects are required, and no `seg0xx.asm` source was changed.

The interrupt handler itself remains the existing far handler for now. Moving
that processor-specific entry point behind an external DOS driver boundary is
separate from converting the game-side vector setup.

## Behavior preserved

- DOS function `35h` still reads interrupt vector 0 and stores the returned
  `ES:BX` address in `old_intr0_handler`.
- DOS function `25h` still installs `intr0_handler` through `DS:DX`.
- The real-mode segment registers are initialized from the current process
  before the two calls.
- The SDL-side implementation is a no-op instead of depending on a DOS
  interrupt service.

## Verification

- The DOS compiler builds `restunts.c`, and both `RESTUNTS.EXE` and
  `REPLDUMP.EXE` link successfully with no new warnings.
- The Phase 0 audit reports seven preserved-assembly symbol references, down
  from eight, and the checked-in inventory is current.
- The comprehensive serial remote collection is the replay regression gate;
  no additional ad-hoc local replays were run after reaching the requested
  local-run limit in Phase 1 slice 03.
- The comprehensive remote collection reports zero mismatches and is
  byte-identical to the Phase 1 slice 04 clean baseline. Its preserved result is
  `stunts/partitions_all_phase5_init_div0_c.txt`; an empty result file means
  that every replay matched.

The serial remote replay collection is the comprehensive regression gate for
this slice, and it passed.
