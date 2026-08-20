# Assembly-to-C migration: Phase 1, slice 08

## Optimized C sprite clear

Phase 1 slice 08 removes the startup timing loop's last call to the preserved
`ported_sprite_clear_1_color_` routine. All callers now use the existing
`sprite_clear_1_color` C implementation.

The previous C implementation wrote every pixel with a nested, unoptimized C
loop. That was functionally simple but slow enough to distort the startup
video-speed measurement. The new implementation delegates each fill to
Borland's medium-model `fmemset` runtime module, which uses a word-oriented far
memory fill. A rectangle whose width equals the sprite pitch is cleared in one
call; a clipped rectangle is cleared one scanline at a time.

`FMEMSET.OBJ` is extracted from the already checked-in `tools/lib/cm.lib` and
is linked only into the ported DOS targets. It is compiler runtime support, not
preserved game assembly. A future non-DOS backend can map this operation to its
normal memory or surface fill. The original replay oracle's link set remains
unchanged, and no `seg0xx.asm` source was changed.

## Behavior preserved

- Empty vertical or horizontal rectangles still return without writing.
- The first destination byte still comes from the active sprite's line-offset
  table plus its left edge.
- Exactly `right - left` bytes are filled on each of `height - top` rows.
- Partial-width rectangles still advance by the sprite pitch between rows.
- The startup benchmark still performs fifteen clears of the same 320-by-120
  sprite region before calculating `slow_video_mgmt`.

## Verification

- The DOS compiler builds `shape2d.c` and `restunts.c`, and both
  `RESTUNTS.EXE` and `REPLDUMP.EXE` link successfully with no new warnings in
  the changed code.
- The Phase 0 audit reports three preserved-assembly symbol references, down
  from four. All three remaining references call the same stack-sensitive
  original car-speed routine.
- The checked-in inventory is current.
- The comprehensive serial remote collection is the replay regression gate;
  no additional ad-hoc local replays were run after reaching the requested
  local-run limit in Phase 1 slice 03.
- The comprehensive remote collection reports zero mismatches and is
  byte-identical to the Phase 1 slice 07 clean baseline. Its preserved result is
  `stunts/partitions_all_phase8_sprite_clear_c.txt`; an empty result file means
  that every replay matched.

The serial remote replay collection is the comprehensive regression gate for
this slice, and it passed.
