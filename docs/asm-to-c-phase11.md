# Assembly-to-C migration: Phase 11

## Penalty-route traversal

Phase 11 moves the defined portion of `player_op`'s penalty-route traversal
from the preserved `detect_penalty` assembly routine to C. The translation
keeps the original depth-first route walk, alternate-branch stack, closest
positive distance, start-tile updates, and wrong-way results.

The original routine allocated a 128-entry `branch_pieces` array on the CPU
stack. Entries 114 through 117 later occupied the same addresses as four
uninitialized opponent wheel-angle words. Earlier compatibility work had to
seed and recover those entries by reaching into the future assembly stack
frame with inline assembly.

The C traversal now models that accidental state flow directly:

- `branch_pieces[114..117]` are seeded from
  `legacy_wheel_angle_stack_words`;
- the primary traversal retains those four entries after it returns;
- the special terminal-route retry does not retain them, matching the
  existing one-way contamination from the primary call;
- no code reads or writes another function's physical stack frame.

## Terminal route handling

All track-data tables share one allocation, so the bytes immediately before
`td17`, `td21`, and `td22` are still defined data inside that allocation and
are read explicitly when the original next-piece sentinel is `-1`.

There is no defined route word before `td01` in the ported C allocator, and
the replay collection demonstrates that neither a universal zero wrap nor a
universal backtrack reproduces every historical heap-layout outcome. The C
routine therefore reports when traversal reaches that boundary. `player_op`
then restores the pre-call start tile and delegates only that bounded terminal
case to the preserved DOS routine for traversal outputs. The C branch array
still supplies the retained legacy words.

The preserved assembly symbol also remains linked for assembly callers that
have not yet been ported. Removing this terminal fallback requires an explicit
portable policy for the old heap-before-`td01` read and is left as a later
slice. No `seg0xx.asm` source was changed.

## Verification

- The DOS compiler builds `state.c`, and both ported game and replay targets
  link successfully with no new warning in the translated traversal.
- The Phase 0 audit reports 26 active inline-assembly sites, down from 29.
- A diagnostic comprehensive run using original traversal outputs and the C
  branch-residue model reported zero mismatches, independently confirming
  the explicit entries 114 through 117.
- Prototype runs showed four mismatches with universal zero wrapping and ten
  with universal terminal backtracking. That evidence defines the narrow
  fallback above and avoids track- or replay-specific fingerprints.
- The checked-in inventory is current.
- No additional ad-hoc local replays were run after reaching the requested
  local-run limit in Phase 3.

- The final hybrid comprehensive collection reports zero mismatches and is
  byte-identical to the Phase 10 clean baseline. Its preserved result is
  `stunts/partitions_all_phase11_detect_penalty_hybrid_c.txt`; an empty result
  file means that every replay matched.

The serial remote replay collection is the comprehensive regression gate for
this slice, and it passed.
