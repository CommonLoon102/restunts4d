# Assembly-to-C migration: Phase 10

## Polygon-edge helper control flow

Phase 10 removes two active inline-assembly remnants from
`generate_poly_edges`.

The function represents the original adjacent `preRender_helper` and
`preRender_helper2` routines as one C implementation with a `mode` argument.
Mode 0 performs the four boundary-fill cases before continuing through the
shared edge generator. Mode 1 enters directly at that shared portion. The
previous mode-1 dispatch used an assembly jump to a C label; it now uses a C
`goto` to the same label.

The other inline block set `ES` to `SS` and loaded `SI` from `regsi`. Those
registers were used only by an assembly transcription that had already been
commented out after its operations were translated into the C loops above it.
The active code following the block is C and reads `regsi` directly, so the
register setup had no remaining consumer and is removed with the stale
transcription.

No live rasterizer arithmetic was changed, and no `seg0xx.asm` source was
changed.

## Behavior preserved

- Mode 0 still executes all four initial boundary-fill cases in their
  existing order.
- Mode 1 still skips those cases and enters at `preRender_helper2`.
- Both modes still use the same count, offset, and edge-type switch after the
  shared label.
- The removed register setup performed no memory writes and supplied no value
  read by the active C implementation.

## Verification

- The implementation was checked against the original adjacent
  `preRender_helper` and `preRender_helper2` control flow.
- The DOS compiler builds `shape3d.c`, and all original and ported game/replay
  targets link successfully. The changed function adds no compiler error.
- The Phase 0 audit reports 29 active inline-assembly sites, down from 31.
- The checked-in inventory is current.
- The comprehensive serial remote collection is the replay regression gate;
  no additional ad-hoc local replays were run after reaching the requested
  local-run limit in Phase 3.

- The comprehensive remote collection reports zero mismatches and is
  byte-identical to the Phase 9 clean baseline. Its preserved result is
  `stunts/partitions_all_phase10_poly_edges_c.txt`; an empty result file means
  that every replay matched.

The serial remote replay collection is the comprehensive regression gate for
this slice, and it passed.
