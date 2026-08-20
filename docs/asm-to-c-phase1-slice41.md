# Assembly-to-C migration: Phase 1, slice 41

## Exact physics and culling double-word state

Phase 1 slice 41 converts a bounded group of assembly-owned signed
double-word values from host `long` spellings to `legacy_s32`. The centralized
declarations now cover the 32-entry `invpow2tbl` culling-mask table, the four
cached sine/cosine values, the travelled-distance accumulator, and the three
player world-position components. Each corresponding symbol is declared with
`dd` storage in `dseg.asm`.

The active C paths now use the same exact type for the directly dependent
double-word intermediates in `vector_op_unk2()` and
`transformed_shape_op()`. This includes the two culling-mask pointers, both
working masks, and the transformed-depth accumulator. Redundant local `long`
declarations were removed in favor of the shared declarations in `externs.h`.

The change preserves the Borland DOS ABI because `legacy_s32` resolves to the
original four-byte `long` there. On LP64 hosts it prevents these assembly-owned
values and their arithmetic carriers from widening to eight bytes. The
generated porting inventory was refreshed because removing local declarations
changed source-evidence line numbers; no routine classifications changed.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char` and warnings as errors
  verifies that every centralized global is exactly four bytes and that
  `invpow2tbl` remains exactly 128 bytes.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice41_physics_dwords.txt`, byte-identical to the
  clean Phase 1 slice 40 report.
