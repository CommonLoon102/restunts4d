# Assembly-to-C migration: Phase 1, slice 16

## Clipped line rasterizer

Phase 1 slice 16 replaces the final active inline-assembly implementation in
compiled C, `draw_line_related_impl`, with C. The routine sorts line endpoints,
selects one of eight edge modes, calculates fixed-point steps, clips against all
four viewport edges, and records clipped scanline padding for the pre-renderer.

The translation explicitly preserves the original 16-bit behavior for:

- signed endpoint comparisons and wrapped word arithmetic;
- signed-overflow detection and emergency line subdivision;
- the interpolation-table and long-division step paths;
- quotient rounding based on the division remainder;
- fixed-point carry and borrow propagation;
- all mode and clipping dispatch combinations; and
- the normal and alternate entry-point policies.

The interpolation lookup is still owned by `seg012`, so the two routines remain
`c_active_with_asm`. Protected callers in `seg003` and `seg012` also continue to
use the original assembly entry points. A later slice must move the lookup data
to C and redirect those callers before these routines can become `portable_c`.
No `seg0xx.asm` source was changed.

## Renderer oracle verification

Replay dumps do not execute the drawing path, so a temporary DOS self-test
compared the C implementation directly with the protected
`ported_draw_line_related_` and `ported_draw_line_related_alt_` routines.

- 20,000 deterministic directed and randomized lines were tested in both modes,
  for 40,000 C-versus-assembly calls.
- Every call compared the return value and all 14 output words.
- Four viewport layouts, clipping boundaries, all line directions, exact
  diagonals, fixed-point modes, and overflow subdivision were covered.
- The tested coordinate envelope was x = -15,000 through 15,000 and y = -20,000
  through 20,000, substantially wider than a real viewport.
- The final result was zero mismatches.

An initial full-word boundary matrix found synthetic extreme lines for which the
protected original never terminates in its emergency subdivision loop. Those
invalid cases were excluded from the oracle envelope; the terminating overflow
paths remain covered. The temporary test entry point and output file are not part
of the final source or executable.

## Build and regression verification

- The DOS compiler builds both `RESTUNTS.EXE` and `REPLDUMP.EXE`.
- The Phase 0 audit reports zero active inline-assembly sites, down from 12, and
  zero preserved-assembly symbol calls from C.
- No additional ad-hoc replay state dump was run after Phase 1 slice 03 reached
  the requested local-run limit.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice16_draw_line_c.txt`, byte-identical to the clean
  Phase 1 slice 15 result.

The oracle verifies descriptor generation rather than final pixels. Full-game
visual and performance testing on real DOS remains necessary before treating the
C renderer as a complete replacement.
