# Assembly-to-C migration: Phase 1, slice 114

## Defined replay-rewind accumulator arithmetic

Phase 1 slice 114 gives the C replay-rewind controls an explicit legacy data
model. Their accumulator, maximum, and speed are now exact signed double
words instead of plain `long`, which is 32 bits in the DOS build but commonly
64 bits on hosted LP64 targets.

The maximum calculation, timer-delta multiplication, and accumulator addition
now retain their low 32 bits. The accumulated value is interpreted as signed
for the existing clamp and division, and target-frame subtraction wraps at 16
bits. This defines the same behavior on hosted compilers as the DOS C path,
including extreme timer-counter bit patterns, without changing ordinary
rewind behavior. The Borland branch retains the previous source expressions
so the layout-sensitive DOS code and relocations remain unchanged.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char`. It exhaustively checks all 65,536
  origin-frame maximums, one million deterministic wrapping accumulator
  advances, and one million signed-division/word-subtraction target cases:
  2,065,536 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object is 44,730 bytes. A separate no-debug object
  comparison confirms that its OMF code/data and relocation records are
  byte-identical to the clean Phase 1 slice 113 build; only comment metadata
  differs.
- Exactly twenty local replay comparisons completed before the
  layout-preserving correction. Every `.BIN`/`.BNI` pair was byte-identical
  and had matching MD5 hashes; there were no timeouts or incomplete
  comparisons. No additional local run was made after reaching the slice's
  twenty-replay allowance.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helpers.
- The first serial comprehensive report is preserved as
  `partitions_all_phase1_slice114_rewind_arithmetic.txt`. It records twelve
  timeouts from a Borland rewrite that grew the object by six bytes. Restoring
  the exact DOS expression layout removed that code drift while retaining the
  hosted fixed-width path.
- The corrected serial comprehensive report is preserved as
  `partitions_all_phase1_slice114_rewind_arithmetic_layout_preserving.txt`.
  It is empty and byte-identical to the clean Phase 1 slice 113 report.
