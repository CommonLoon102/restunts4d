# Assembly-to-C migration: Phase 1, slice 01

## First conversion slice

Phase 1 slice 01 removes the active inline assembly from
`timer_get_counter` and `timer_get_delta` in `src/restunts/c/restunts.c`.
These routines were selected because they form a small, cohesive boundary and
do not participate in replay physics calculations.

Both routines now use the DOS C compiler's `disable()` and `enable()` services
around the shared 32-bit timer read. This preserves the important behavior of
the former `cli`/`sti` sequence: the timer interrupt cannot update one 16-bit
half of the counter while the foreground code reads the other half.

No `seg0xx.asm` source was changed. The original and mixed assembly objects
remain available as regression oracles.

## Current classification

The two routines remain `c_active_with_asm` in the inventory even though their
own inline assembly is gone. They still use DOS-specific interrupt masking and
timer storage owned by the legacy data segment. They can become final C-only
platform routines after that storage and the timer interrupt backend have been
moved behind the DOS platform boundary.

## Verification

- The DOS compiler builds both routines without the former missing-return
  warnings.
- Ten short local replays produce byte-identical `.BIN` and `.BNI` files.
- The Phase 0 audit reports 55 active inline-assembly sites, down from 57.
- The comprehensive remote collection reports zero mismatches. Its preserved
  result is `stunts/partitions_all_phase1_timer_c.txt`; an empty result file
  means that every replay matched. The older checked-in report lists five
  mismatches, but it is not a same-commit pre-change baseline, so no individual
  fix is attributed to this timer slice.

The serial remote replay collection is the comprehensive regression gate for
this slice, and it passed.
