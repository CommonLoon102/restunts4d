# Assembly-to-C migration: Phase 1, slice 42

## Exact file byte-count APIs

Phase 1 slice 42 converts the file writer and decompression byte-count paths
from host `unsigned long` spellings to `legacy_u32`. The three public
`file_write*()` interfaces now accept exact four-byte lengths, matching the
two-word length passed by their DOS callers. The RLE and VLE decompression
passes likewise return exact four-byte lengths and carry their input, output,
and intermediate byte counts through `legacy_u32` locals.

Compressed-resource sizes are stored as one high byte and one low word. Those
values are now assembled with `LEGACY_U32_FROM_WORDS()` in the paragraph-size,
RLE, and VLE paths instead of depending on the width of a cast to `long`.
`fileio.h` includes the legacy integer contract directly so callers receive
the same prototype independently of include order.

The standard `fseek()` and `ftell()` signatures remain unchanged; their
`long` type belongs to the C runtime interface rather than a serialized or
assembly-defined game ABI. On Borland DOS, `legacy_u32` remains the original
four-byte `unsigned long`, so the generated calling convention is unchanged.
On LP64 hosts, file and decompression counts no longer widen to eight bytes.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 test compiled with `-funsigned-char` and warnings as errors
  verifies the exact write-function prototypes, the four-byte `legacy_u32`
  contract, and reconstruction of a representative 24-bit compressed size.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged two-word DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice42_file_byte_counts.txt`, byte-identical to the
  clean Phase 1 slice 41 report.
