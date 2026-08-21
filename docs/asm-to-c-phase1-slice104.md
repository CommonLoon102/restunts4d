# Assembly-to-C migration: Phase 1, slice 104

## Defined initial scene-vector word arithmetic

Phase 1 slice 104 gives the initial `game_vec1` construction in
`init_game_state()` explicit legacy word semantics. Track-angle offsets and
the two- and three-term coordinate sums now wrap after 16 bits. The starting
column is explicitly sign-extended from its byte before its word-sized
ten-bit shift, while the hill and row indices retain the original signed-byte
interpretation.

This matches the original high-byte angle additions, word `add` operations,
`cbw`, and word `shl` without signed overflow, negative signed shifting, or
plain-`char` dependencies in hosted builds. Borland definitions retain the
original direct word expressions.

No `seg0xx.asm` source was changed.

## Verification

- A hosted GNU89 address/undefined-behavior sanitizer harness compiles the
  production helpers with `-funsigned-char`. It checks two- and three-word
  sums across permutations of every word pattern and the grid offset across
  all 256 byte patterns: 131,328 results pass.
- Borland C++ 5.2 compiles and links `RESTUNTS.EXE`, `REPLDUMP.EXE`, and the
  original replay oracle. The same 41 pre-existing `restunts.c` warnings
  remain and its DOS object remains 44,612 bytes.
- Exactly twenty local replay comparisons completed. Every `.BIN`/`.BNI` pair
  is byte-identical and has matching MD5 hashes; there were no timeouts or
  incomplete comparisons.
- The refreshed Phase 0 audit reports 619 tracked routines, 479 required
  routines remaining, zero active inline assembly, zero preserved-assembly
  calls from C, and 86 assembly link inputs. Inventory source locations are
  refreshed after adding the hosted helpers.
- The separate serial comprehensive replay result is preserved as
  `partitions_all_phase1_slice104_initial_scene_vector_words.txt`. It is empty
  and byte-identical to the clean Phase 1 slice 103 report.
