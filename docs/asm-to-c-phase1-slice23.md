# Assembly-to-C migration: Phase 1, slice 23

## Fixed-width car-parameter layout and pointer-safe loading

Phase 1 slice 23 converts the complete packed `SIMD` car-parameter definition
to fixed-width legacy integer types. Signed word parameters use `legacy_s16`,
gear ratios use `legacy_u16`, and every one-byte car parameter and dashboard
coordinate uses `legacy_u8`, matching the resource format's unsigned-byte
semantics. The runtime aerodynamic-resistance table is now explicitly a
pointer to `legacy_s16` values.

The `simd` resource chunk is 776 bytes (`0x308`). Its first 772 bytes (`0x304`)
contain car parameters; its last four file bytes are ignored placeholders at
the location occupied by the runtime DOS far pointer. The loader now copies
only the 772-byte parameter payload and then assigns the native
`aerorestable` pointer. This preserves DOS behavior because the old whole-
structure copy immediately overwrote those four placeholder bytes, while also
avoiding a resource over-read when a hosted pointer makes `struct SIMD` larger
than its DOS layout.

A portable C89 assertion fixes the runtime pointer offset at byte 772. A
DOS-only assertion additionally fixes the complete Borland structure size at
776 bytes, including its four-byte far pointer. The nested point and vector
types already acquired fixed-width fields in slice 18.

This slice changes stored types and resource-loading boundaries; it does not
yet rewrite all arithmetic performed on car parameters. On the Borland
compiler the new aliases resolve to the same machine types as the previous
word declarations, and unsigned byte parameters make the assembly's explicit
zero-extension behavior portable.

No `seg0xx.asm` source was changed.

## Verification

- A hosted strict C89 test compiled with `-funsigned-char` verifies the
  772-byte runtime-pointer offset, native-pointer-sized hosted structure, and
  unsigned high-bit car-parameter behavior.
- A read-only resource-header scan confirms that all 40 installed `CAR*.RES`
  files contain a 776-byte `simd` chunk.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the 776-byte DOS layout assertion enabled and
  without a new relevant warning.
- Twenty local replay tests completed using twenty distinct player-car
  resources, including original and community cars. Every `.BIN`/`.BNI` pair
  has matching MD5 hashes; there were no timeouts or incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice23_simd_layout.txt`, byte-identical to the clean
  Phase 1 slice 22 report.
