# Assembly-to-C migration: Phase 1, slice 111

## Defined random-wait counter semantics

Phase 1 slice 111 gives `random_wait()` the original 16-bit `DI` counter
semantics. The video-status loop count now wraps as an unsigned word. When the
special count is reached, the replacement byte from `aMisc_1` is explicitly
interpreted as signed and sign-extended to the counter word, matching the
original `cbw` instruction.

Both post-decrement delay loops now operate on that word value. A completed
loop therefore leaves the counter at `0xFFFF`; masking it to the low byte before
the second loop reproduces the original register sequence without depending on
the host compiler's plain-`char` signedness or unbounded `int` arithmetic.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helper with `-funsigned-char` and checks 1,536 counter states and
  8,453,760 loop iterations against an independent 16-bit reference.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object is 44,706 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after the C change.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice111_random_wait_counter.txt`. It is empty and
  byte-identical to the clean Phase 1 slice 110 report.
