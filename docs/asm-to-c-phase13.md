# Assembly-to-C migration: Phase 13

## Grip stack-residue capture expressed in C

Phase 13 removes the eleven active inline-assembly statements from
`player_op` in `state.c`. The statements captured four words left below the
current stack pointer by the preceding preserved `update_grip` call and saved
them in `legacy_grip_stack_words` for `update_player_state`.

The DOS build now reads the same words with C far-pointer expressions based
on the address of the existing `var_terminalPenalty` local. Borland places
that local at `BP-28`; the four residue words occupy `BP-118`, `BP-116`,
`BP-114`, and `BP-112`. Subtracting 90, 88, 86, and 84 bytes respectively
therefore addresses the original words without introducing another local.

The generated listing was inspected after the change:

- `player_op` retains its `enter 76,0` frame;
- the four loads use `SS` and resolve to the same frame offsets;
- no call or stack write occurs between `update_grip` and the capture;
- the values are stored in the same four `legacy_grip_stack_words` entries.

This is a transitional DOS compatibility path, like Phase 12. It is ordinary
C but still deliberately depends on Borland's 16-bit frame layout. The code
is guarded by `RESTUNTS_DOS`; a future portable backend must replace the
underlying uninitialized-stack behavior with explicit physics state.

No `seg0xx.asm` source was changed.

## Verification

- The DOS compiler builds both `RESTUNTS.EXE` and `REPLDUMP.EXE`. The emitted
  `state.c` warnings are the same pre-existing pointer-conversion and missing
  prototype warnings.
- The Phase 0 audit reports 13 active inline-assembly sites, down from 24,
  and the checked-in inventory is current.
- No additional ad-hoc local replays were run after reaching the requested
  local-run limit in Phase 3.
- The serial comprehensive remote run returned an empty
  `partitions_all_phase13_grip_residue_c.txt`, byte-identical to the clean
  Phase 12 result.
