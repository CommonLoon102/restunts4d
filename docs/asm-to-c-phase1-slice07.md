# Assembly-to-C migration: Phase 1, slice 07

## Particle primitive branch

Phase 1 slice 07 removes the active inline-assembly block for primitive type 5 in
`transformed_shape_op`. The original listing identifies this type as a
particle. The branch is compact and maps directly onto C data structures that
the surrounding translated branches already use.

No `seg0xx.asm` source was changed.

## Behavior preserved

- The primitive's first byte still selects the vertex index.
- A vertex marked by `var_vertflagtbl` still skips the primitive through
  `loc_25801`.
- The selected signed 16-bit Z coordinate is still sign-extended into the
  32-bit `var_18` depth accumulator.
- The first projected point is still copied into the poly-info record at byte
  offset 6.
- The dirty rectangle is still expanded only when transformed-shape flag 8 is
  set.
- The branch still records one output vertex and rejoins the common
  `loc_25988` path.

The earlier duplicate commented assembly transcription is removed with the
active block so there is one authoritative implementation.

## Verification

- The DOS compiler builds `shape3d.c`, and both `RESTUNTS.EXE` and
  `REPLDUMP.EXE` link successfully with no new warning in the converted
  branch.
- The Phase 0 audit reports 35 active inline-assembly sites, down from 36, and
  the checked-in inventory is current.
- The comprehensive serial remote collection is the replay regression gate;
  no additional ad-hoc local replays were run after reaching the requested
  local-run limit in Phase 1 slice 03.

The replay dumper guards simulation state and broad integration but does not
guarantee pixel-level execution of this rendering branch. The direct mapping
above and the DOS compiler's type checking are therefore also material parts
of this slice's verification.
- The comprehensive remote collection reports zero mismatches and is
  byte-identical to the Phase 1 slices 05/06 clean baseline. Its preserved result is
  `stunts/partitions_all_phase7_particle_primitive_c.txt`; an empty result file
  means that every replay matched.

The serial remote replay collection is the comprehensive integration gate for
this slice, and it passed.
