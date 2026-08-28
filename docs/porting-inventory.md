# Assembly porting inventory

`porting-inventory.json` is the generated, machine-readable inventory for the
assembly-to-C port. It distinguishes these states for every procedure found in
`src/restunts/asm/seg*.asm`:

- `asm_only`
- `c_translation_inactive`
- `c_active_asm_dependent`
- `portable_c`

Reachability is evaluated for the non-original DOS builds
(`RESTUNTS_DOS` enabled and `RESTUNTS_ORIGINAL` disabled), including both the
game and headless replay entry points. The original build remains the oracle;
its deliberately selected assembly branches do not make a C translation
inactive.

Regenerate and verify it from the repository root with:

```sh
python3 tools/scripts/porting-inventory.py --write docs/porting-inventory.json
python3 tools/scripts/porting-inventory.py --check-generated docs/porting-inventory.json
```

The future C-only target can use the strict gate below. It intentionally fails
until the link drops all project assembly objects, active C contains no inline
assembly or `ported_*` references, and game data no longer comes from
`dseg.asm`:

```sh
python3 tools/scripts/porting-inventory.py --check-c-only
```

Compiler runtime libraries and DOS platform services are reported separately
from the portable procedure state; they can remain platform dependencies for a
real-mode DOS build.
