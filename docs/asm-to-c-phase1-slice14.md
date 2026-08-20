# Assembly-to-C migration: Phase 1, slice 14

## Player speed update

Phase 1 slice 14 activates the existing C translation of `update_car_speed`
for the player simulation. `player_op` now calls the C routine directly, and
the dormant early delegation to `ported_update_car_speed_` has been removed.

The original translation contained two classes of 16-bit arithmetic errors
that were hidden while it remained unreachable:

- The acceleration path used signed long division and shifted the quotient
  before narrowing it. The original performs unsigned 32-bit division,
  stores the low 16-bit word, and then applies an arithmetic right shift to
  that word. This ordering is part of Stunts' mass-dependent Power Gear
  behavior.
- RPM comparisons against the upshift, downshift, and maximum-RPM words are
  unsigned in the original. The torque-table index also uses a logical shift
  of the RPM word. Explicit unsigned casts preserve high-RPM custom cars such
  as `GATE`, whose maximum-RPM parameter is `0xF5F8`.

Unsigned byte handling is also explicit for idle torque and the opponent
speed adjustment. These casts preserve the original byte operations without
changing any structure layout.

The protected assembly still calls `ported_update_car_speed_` from the
opponent simulation in `seg001.asm`. This slice does not modify that caller or
claim that the legacy routine can be unlinked. The C opponent-only branch is
retained for a future port of the containing caller.

No `seg0xx.asm` source was changed.

## Verification

- The DOS compiler builds both `RESTUNTS.EXE` and `REPLDUMP.EXE`.
- Borland emits its unsigned long-division helper, narrows the result to the
  existing 16-bit local, and then emits a word-sized arithmetic shift.
- The Phase 0 audit reports zero preserved-assembly symbol references from C.
  The 13 remaining inline-assembly sites are all in the two large renderer
  routines already identified in `shape3d.c`.
- No additional ad-hoc local replay was run after Phase 1 slice 03 reached the
  requested local-run limit.
- The serial comprehensive remote run returned an empty
  `partitions_all_phase1_slice14_player_speed_c.txt`, byte-identical to the
  clean Phase 1 slice 13 result.

Two rejected diagnostic reports were retained separately. The initial
translation produced widespread mismatches; correcting the unsigned division
reduced that set to 42, all using the `GATE` car. Correcting unsigned RPM
semantics removed the remaining mismatches. A delegation-only wrapper test
also showed one layout-sensitive mismatch (`0172.rpl`), confirming that the
extra wrapper frame must not be reintroduced.
