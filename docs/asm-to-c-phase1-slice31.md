# Assembly-to-C migration: Phase 1, slice 31

## Fixed-width track-object-info records

Phase 1 slice 31 converts the runtime `TRKOBJINFO` record to fixed-width legacy
types and corrects an incomplete reverse-engineered field interpretation. Its
block count, connectivity values, arrow type, and opponent speed-table index
are `legacy_u8`; its arrow orientation is `legacy_s16`.

The two bytes formerly declared separately as `si_opp1` and `si_opp2` are
accessed together by the original assembly as a word. When nonzero, that word
is used as an alternate near pointer to camera-coordinate data. The C record
now models it as `si_opponentCameraDataOffset`, parallel to the existing
`si_cameraDataOffset`; both point to `legacy_s16` coordinate words and remain
native near pointers.

The remaining opponent-routing byte is `legacy_s8`. Assembly explicitly sign
extends it before indexing direction tables and also recognizes `0xFF` as a
sentinel. The speed-code byte is zero-extended before table indexing and is
therefore `legacy_u8`.

Both near pointers occupy two bytes in Borland's DOS memory model but grow
naturally on a hosted target. A DOS-only C89 compile-time assertion fixes the
complete assembly-visible record at 14 bytes without imposing DOS pointer
widths elsewhere. The fixed-width aliases and corrected pointer occupy the
same DOS bytes as the previous declarations, so the mixed-build ABI remains
unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies the
  unsigned count/index bytes, signed `0xFF` opponent sentinel, signed word
  coordinates, and a hosted structure made from ten fixed bytes plus two
  native pointers.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the corrected pointer and 14-byte DOS layout
  assertion enabled, without a new relevant warning.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice31_trkobjinfo_layout.txt`, byte-identical to the
  clean Phase 1 slice 30 report.
