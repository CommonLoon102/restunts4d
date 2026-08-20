# Assembly-to-C migration: Phase 1, slice 34

## Explicit unsigned track byte buffers

Phase 1 slice 34 converts every remaining one-byte region in the allocated
track-data block to `legacy_u8 far *`. These regions contain raw high-score and
sprite snapshots, the serialized replay header, track/terrain maps, replay
input, ordered track-element indices, connection flags, the appended track
file, path coordinates, and track-object indices.

These are byte patterns, nonnegative indices, coordinates, or bit flags; none
should inherit the hosted compiler's plain-`char` signedness. The aliases also
unify fields previously split between `char *` and `unsigned char *` spellings.

Both `init_trackdata()` implementations now use a `legacy_u8 far *` cursor for
the single `0x6BF3`-byte allocation. All partition offsets are still expressed
in bytes, while the word regions retain the explicit `legacy_s16 far *` casts
introduced in slices 32 and 33.

Because the replay-header pointer now carries its actual byte type,
`gameinfo_encode()` and `gameinfo_decode()` no longer need local corrective
casts. Under Borland, `legacy_u8` has the same one-byte representation and the
far pointers retain their DOS ABI.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies that all
  thirteen byte-buffer pointees are one-byte unsigned values, including the
  full high-bit range.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the byte-typed allocation cursor and buffer
  declarations.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice34_track_byte_buffers.txt`, byte-identical to the
  clean Phase 1 slice 33 report.
