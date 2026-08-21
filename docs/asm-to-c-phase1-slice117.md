# Assembly-to-C migration: Phase 1, slice 117

## Defined explosion-scale quotient arithmetic

Phase 1 slice 117 gives the crash-explosion scale calculation the original
signed word/double-word semantics. Hosted builds now sign-extend the rectangle
extent, multiply its bit pattern by 256 with 32-bit wrapping, divide by the
sign-extended animation width with truncation toward zero, and retain the
quotient's low signed word.

This matches the original `cwd`, byte-shuffle left shift, and signed long
division sequence. Hosted builds no longer use an LP64 `long` calculation or
retain a wider `int` quotient. A zero animation width remains outside the
valid input envelope, matching the original unchecked division. The Borland
branch retains its previous expression verbatim to preserve DOS layout.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production `frame.c` with `-funsigned-char` and warnings as errors. It tests
  every 16-bit extent against nine positive and negative nonzero width
  patterns: 589,824 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The existing two `frame.c` and 41 `restunts.c`
  warnings remain; `frame.obj` is 31,290 bytes. A separate no-debug object
  comparison confirms that its OMF code/data and relocation records are
  byte-identical to the clean Phase 1 slice 116 build; only comment metadata
  differs.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helper.
- The first serial comprehensive result is preserved as
  `partitions_all_phase1_slice117_explosion_scale_word.txt`. It contains 32
  timeout records split between the unchanged oracle and current executable,
  with no state-file mismatch. Because the oracle also timed out and the OMF
  code is unchanged, the same binaries were submitted again after the serial
  service became available; no code change or additional local replay run was
  made.
- The unchanged-binary retry is preserved as
  `partitions_all_phase1_slice117_explosion_scale_word_retry.txt`. It also
  contains 32 timeouts, but for a different set of replays, again split across
  both executables (13 oracle and 19 current) with no mismatch. This confirms
  transient service capacity rather than deterministic replay behavior.
- The second unchanged-binary retry is preserved as
  `partitions_all_phase1_slice117_explosion_scale_word_retry2.txt`. It is
  empty and byte-identical to the clean Phase 1 slice 116 report.
