# Assembly-to-C migration: Phase 1, slice 15

## Pre-render edge merge

Phase 1 slice 15 replaces the active inline assembly in
`preRender_default_impl_helper`, known as `preRender_helper3` in the assembly
inventory, with C.

The routine merges a newly rasterized polygon edge into two 480-word scanline
arrays. The C implementation preserves the original 16-bit behavior for:

- constant, incrementing, and decrementing edge modes 2 through 4;
- 16.16 fixed-point edge modes 5 and 6;
- carry-controlled fractional span modes 7 and 8;
- the two merge policies selected by `var_A`; and
- the four clipped-padding runs selected by `var_C`.

The protected `seg012.asm` still contains the original near routine for its
two assembly callers. C renderer callers now use the verified C implementation.
No `seg0xx.asm` source was changed.

## Renderer oracle verification

Replay dumps do not execute the drawing path, so this slice used a temporary
DOS self-test before the inline body was removed. The test kept the assembly
implementation as an oracle and ran both versions over identical inputs.

- 10,400 deterministic randomized and boundary cases were checked.
- Every case compared all 960 output words, not a hash.
- Modes 2 through 8, both merge policies, padding enabled and disabled, zero
  counts, and invalid-mode exits were covered.
- The final result was zero mismatches.

A synthetic negative count exposed a discrepancy in the old inline block's manual
long-jump workaround: it used unsigned `ja`, unlike the protected original's
signed `jle`, and attempted 65,535 iterations. Real callers provide
nonnegative counts. The C implementation follows the protected original and
returns through the padding path for nonpositive counts.

The temporary oracle, test entry point, and output file are not part of the
final source or executable.

## Build and regression verification

- The DOS compiler builds both `RESTUNTS.EXE` and `REPLDUMP.EXE`.
- The Phase 0 audit reports 12 active inline-assembly sites, down from 13.
  All 12 are in the remaining `draw_line_related_impl` rasterizer.
- No additional ad-hoc replay state dump was run after Phase 1 slice 03
  reached the requested local-run limit.
- The serial comprehensive remote run returned an empty
  `partitions_all_phase1_slice15_prerender_edges_c.txt`, byte-identical to the
  clean Phase 1 slice 14 result.

The oracle verifies this edge-merge routine in isolation. Replay regression
checks simulation and executable-layout safety, but does not replace later
full-game visual and performance testing of the C renderer on real DOS.
