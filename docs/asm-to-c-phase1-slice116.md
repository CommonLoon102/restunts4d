# Assembly-to-C migration: Phase 1, slice 116

## Defined pre-render merge fixed-point words

Phase 1 slice 116 gives `preRender_default_impl_helper()` exact 32-bit
fixed-point state. Its carry sum, 16.16 accumulator, and per-scanline delta no
longer use plain `unsigned long`, which is 32 bits in the DOS build but
commonly 64 bits on hosted LP64 systems. Accumulator updates now wrap modulo
2^32 on every target before their signed high word is written to an edge.

The Borland build still resolves `legacy_u32` to the same underlying
`unsigned long` type, so its generated code and relocations remain unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production `shape3d.c` with `-funsigned-char` and warnings as errors. It
  exercises both merge fixed-point modes across every low-word pattern, with
  permuted high words and steps: 1,048,576 edge steps pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The existing 41 `restunts.c` and 65 `shape3d.c`
  warnings remain; `shape3d.obj` is unchanged at 43,127 bytes. A separate
  no-debug object comparison confirms that its OMF code/data and relocation
  records are byte-identical to the clean Phase 1 slice 115 build; only
  comment metadata differs.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. No inventory source location
  changed because the replacement preserves the source line count.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice116_prerender_merge_fixed_points.txt`. It is
  empty and byte-identical to the clean Phase 1 slice 115 report.
