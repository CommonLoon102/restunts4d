# Assembly-to-C migration: Phase 0

## Purpose

Phase 0 defines a measurable boundary for removing the Restunts assembly implementation. It does not port game behavior and does not alter any `seg0xx.asm` file.

The existing original and mixed executables remain regression oracles. A future C-only target will be built alongside them and must not depend on their layout.

## Definition of C-only

The maintained Restunts game source is C-only when:

- no `seg0xx.obj`, `segments.obj`, or `dseg.obj` is linked;
- no compiled C code contains inline `asm`, `_asm`, or `__asm`;
- no C code calls a preserved `ported_*` assembly symbol;
- every required legacy routine is classified as `portable_c` in the porting inventory.

The DOS target may continue to use compiler runtime objects, standard-library code, external DOS driver binaries, and DOS-specific C APIs. Compiler extensions such as `far` and `interrupt` belong in a future DOS platform layer. They must not leak into the portable engine, resource, or renderer layers.

## Inventory states

The checked-in inventory uses these states for required routines:

- `asm_only`: no active C implementation is selected.
- `c_translation_inactive`: C translation work exists, but it is disabled or incomplete.
- `c_active_with_asm`: C is active, but still contains inline assembly, calls preserved assembly, relies on assembly-owned data, or has not passed the portability audit.
- `portable_c`: implementation and required semantics are in portable C and no assembly dependency remains.

`not_required` is reserved for entries identified by the reverse-engineering report as padding, dead code, compiler support, or otherwise outside the game-porting requirement. Such entries must be reviewed before the final C-only cutover; the old `IGNORE` label alone is not proof that an entry can safely disappear.

The initial classification is deliberately conservative. A routine marked `PORTED` by `status.html` defaults to `c_active_with_asm`, not `portable_c`. Promotion to `portable_c` requires explicit review and appropriate regression testing.

## Files

- `src/restunts/status.html` is the legacy reverse-engineering report.
- `src/restunts/porting/overrides.tsv` records reviewed exceptions and evidence.
- `src/restunts/porting/inventory.tsv` is the generated, machine-readable inventory.
- `tools/scripts/porting_audit.py` regenerates the inventory and scans current C/link inputs for assembly blockers.

Do not edit `inventory.tsv` manually. Change the source report or `overrides.tsv`, then regenerate it.

In the generated inventory, `-` in `source_evidence` or `note` means that no
reviewed value has been recorded.

## Commands

From the repository root:

```sh
python3 tools/scripts/porting_audit.py --format summary
python3 tools/scripts/porting_audit.py --format json
python3 tools/scripts/porting_audit.py --check-inventory
python3 tools/scripts/porting_audit.py --write-inventory
```

Equivalent make targets are available from `src/restunts`:

```sh
make porting-status
make porting-audit
make c-only-audit
```

`porting-audit` verifies that the checked-in inventory is current. It is expected to pass throughout the migration. `c-only-audit` is the final strict gate and is expected to fail until all required routines and source/link blockers have been removed.

The strict make target scans the path in `C_ONLY_LINK_INPUT` for assembly object or module names. During Phase 0 it defaults to the existing mixed DOS makefile, so failure is expected. When the separate C-only target is introduced, this variable must point to that target's makefile, linker script, or generated linker map. The original and mixed oracle targets may continue linking their assembly objects.

## Testing policy for later phases

Phase 0 changes only documentation and audit tooling, so replay execution is not required. Later implementation chunks must use the established local replay workflow, with no more than 20 ad-hoc local replays. Comprehensive replay testing must use the serial remote service, and each returned `partitions_all.txt` must be renamed before another request.

All local DOSBox runs of `REPLDUMP.EXE` and `REPLDUMO.EXE` must use `-silent`.
