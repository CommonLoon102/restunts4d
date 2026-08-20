# Assembly-to-C migration: Phase 1, slice 35

## Exact 32-bit timer state and APIs

Phase 1 slice 35 converts the legacy timer state and its complete C API from
plain `unsigned long` to `legacy_u32`. The original data segment declares
`timer_callback_counter`, `last_timer_callback_counter`, and `timer_copy_unk`
as double words, and the original assembly reads, writes, and returns each
value as a `DX:AX` pair.

The distinction is invisible in the 16-bit Borland build, where `unsigned
long` is already four bytes, but it is required for hosted portability because
`unsigned long` is eight bytes on common LP64 targets. Explicit 32-bit types
retain the original modulo-2^32 addition and subtraction when a counter wraps.

All timer declarations and definitions now share the exact type, including
the two-word timer arguments accepted by assembly callers. Timer-derived
locals used by replay rewind and startup video calibration are also explicitly
32-bit. The comparison behavior itself is unchanged and remains identical to
the original unsigned high-word/low-word assembly comparisons.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU C89 test compiled with `-funsigned-char` and warnings as errors
  verifies the three counter widths, the timer return width, and 32-bit
  addition/subtraction wraparound.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the exact-width timer declarations and API.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The Phase 0 audit remains current and reports 619 tracked routines, 479
  required routines remaining, zero active inline assembly, zero
  preserved-assembly calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice35_timer_u32.txt`, byte-identical to the clean
  Phase 1 slice 34 report.
