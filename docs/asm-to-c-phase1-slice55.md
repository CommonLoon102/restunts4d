# Assembly-to-C migration: Phase 1, slice 55

## Defined vector classification arithmetic

Phase 1 slice 55 converts `vector_op_unk2()` to explicit legacy word,
double-word, and byte arithmetic and gives it the original signed one-word
result ABI.

Word absolute values now reproduce 16-bit `abs()` behavior, including the
`-32768` bit pattern. The polar radius is zero-extended exactly as in the
original local double word instead of being sign-extended by the hosted C
translation. The optional sine/cosine weighting uses wrapped 32-bit products
and signed double-word comparison.

The angular classification path now performs word negations and the
quarter-turn correction with explicit wrapping, carries out the multiply-by-15
transform in a double-word bit pattern, and reproduces the original low-byte
addition followed by signed-byte extension.

The Borland DOS ABI remains unchanged because `legacy_s16` is the original
one-word signed result type.

No `seg0xx.asm` source was changed.

## Verification

- A targeted hosted GNU89 test compiled with `-funsigned-char`, warnings as
  errors, and undefined-behavior sanitization compares 500,000 deterministic
  cases with an independent instruction-level reference. The cases are split
  evenly between equal and unequal weighting paths and include `-32768`.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle with the unchanged DOS ABI.
- Twenty local replay tests completed. Every `.BIN`/`.BNI` pair is
  byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice55_vector_classification.txt`, byte-identical to
  the clean Phase 1 slice 54 report.
