# Assembly-to-C migration: Phase 1, slice 113

## Defined fullscreen-window threshold arithmetic

Phase 1 slice 113 gives the fullscreen-window memory threshold the original
signed double-word semantics. The two video scale words are now interpreted as
signed and multiplied in an explicit 32-bit domain before dividing 64,000.
This preserves the full `DX:AX` product consumed by the original long-division
runtime instead of allowing a 16-bit DOS `int` product to wrap first.

The available memory and computed threshold are also compared as signed
32-bit values, matching the original signed high-word and unsigned low-word
comparison sequence. Zero scale products remain outside the valid input
envelope, just as they trigger the original unchecked division.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char`. It checks every nonzero 16-bit
  first factor against nine positive and negative second factors, then checks
  one million deterministic signed double-word comparisons: 1,589,815 results
  pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object is 44,730 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice113_fullscreen_window_threshold.txt`. It is
  empty and byte-identical to the clean Phase 1 slice 112 report.
