# Assembly-to-C migration: Phase 1, slice 02

## DOS upper-memory boundary

Phase 1 slice 02 removes the six active inline-assembly blocks from
`src/restunts/c/memupper.c`. The converted routines cover DOS allocation
strategy queries and updates, UMB link-state queries and updates, paragraph
allocation, and paragraph release.

The C implementation uses Borland's `intdos`, `_dos_allocmem`, and
`_dos_freemem` interfaces. Because the DOS build uses `/u-` and cannot link the
complete runtime library without colliding with routines preserved in the
original executable, only the required medium-model runtime modules and their
dependencies are linked into the ported targets. They are extracted from the
already checked-in `tools/lib/cm.lib`, following the repository's existing
selected-runtime-object convention. The original oracle target does not link
these new objects.

These runtime modules are DOS platform support, not preserved game assembly.
A future non-DOS target will replace this boundary with its platform allocator.
No `seg0xx.asm` source was changed.

## Behavior preserved

- Failed allocation-strategy or UMB-state queries still return `0xFFFF`.
- Setter calls still report success only when DOS clears the carry flag.
- Paragraph allocation still returns zero on failure and the allocated segment
  on success.
- The allocation result uses DOS-only data-segment storage because the
  medium-model runtime receives this output through a near pointer. Phase 1
  slice 03's
  ABI audit corrected the initial literal-`far` declaration and stack-local
  result pointer.
- The original upper-memory-only, first-fit strategy and restoration order are
  unchanged.

## Verification

- The DOS compiler builds `memupper.c` without warnings.
- Ten short local replays produce byte-identical `.BIN` and `.BNI` files:
  `0610`, `1246`, `2176`, `2096`, `2832`, `0722`, `1126`, `1115`, `1118`, and
  `0714`.
- The Phase 0 audit reports 49 active inline-assembly sites, down from 55.
- The checked-in porting inventory remains current.
- The comprehensive remote collection reports zero mismatches and is
  byte-identical to the Phase 1 slice 01 clean baseline. Its preserved result is
  `stunts/partitions_all_phase2_memupper_c.txt`; an empty result file means
  that every replay matched.

The serial remote replay collection is the comprehensive regression gate for
this slice, and it passed.
