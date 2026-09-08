# DOS assembly builds

The default assembler is Open Watcom WASM from the pinned installation in
`tools/watcom`. The makefiles use native `binl64/wasm` on Linux and
`binnt/wasm.exe` on Windows, with `-zcm=tasm` and 8086 code generation.
`ASSEMBLER=tasm32` selects the bundled fallback; that fallback needs Wine on
Linux. Compiler and linker selection remains WCC and WLINK in both cases.

GNU Make 4.3 or newer is required. Windows includes GNU Make 4.4.1.
Building the original game with WASM also requires Python 3.9 or newer.
The makefiles select `python3` on Linux and `python` on Windows; `PYTHON`
can override that command. The adapter uses only Python's standard library.

The original assembly uses `-cx` to match TASM's case-insensitive symbol
handling. Platform assembly keeps case-sensitive symbols to match the C
interfaces. Release builds use `-d0`; debug builds use `-d1` for line records.

## Original-source preparation

Files under `src/restunts/asmorig` remain the authoritative, unchanged
assembly sources. The WASM build invokes
[`prepare-wasm-original.py`](../tools/scripts/prepare-wasm-original.py) and
assembles its generated copies. To prepare an independent directory:

```sh
python3 tools/scripts/prepare-wasm-original.py \
    --source-dir src/restunts/asmorig --output-dir out/wasm-original-source
```

The adapter emits `.asm` and `.inc` files plus `manifest.json`. The manifest
records the adapter hash, each source and generated-file hash, and counts of
the transformations applied. Source bytes use their original DOS encoding;
generated assembly retains CRLF line endings. Preparation validates and
translates all inputs before writing generated files. It rejects unsupported
structure layouts or unresolved typed stack expressions instead of dropping
their type information.

The adapter supports the syntax in the preserved disassembly. New constructs
require both translation checks and artifact comparisons. The compatibility
transformations preserve the original object layout:

- Typed stack aliases and nested structure/union fields retain their offsets
  and scalar operand widths.
- Unsupported union declarations become storage with the same size; member
  references are resolved to their original overlay offsets.
- `SMART`/`NOSMART`, immediate instructions, register direction encodings,
  explicit string prefixes, and near/far transfers retain the encodings
  needed by the preserved disassembly.

The standard generated directory is
`src/restunts/asmorig/build/watcom/<configuration>/wasm/source/`.
Generated files are disposable: edit the adapter when its translation needs
correction, then let Make regenerate them. A grouped Make rule regenerates
the complete set when an input, include, adapter, or toolchain stamp changes,
or when any generated source/include is missing. Release/debug and each
assembler use separate assembly object directories. Platform C objects remain
shared between assembler choices within a configuration.

## Original executable layout

The original target's WLINK response file preserves code/data segment order,
disables automatic segment packing, and retains the original 8000-byte stack.
It uses `NOFARCALLS` to prevent WLINK from rewriting a far call into a
push-CS/near-call sequence. Although those instructions can have equivalent
control flow, preserving the original instruction bytes and relocation
layout is required for the original executable.

The archived `REPLDUMO.EXE` and `PIXLDUMO.EXE` are independent reference
binaries. Their build targets only copy the archived files; neither this
adapter nor a new compiler rebuilds them.

## Comparing artifacts

[`compare-dos-artifacts.py`](../tools/scripts/compare-dos-artifacts.py)
compares existing artifacts and prints a JSON summary. `--output` also saves
the detailed report. Replace the example paths with the files being checked:

```sh
python3 tools/scripts/compare-dos-artifacts.py mz \
    out/reference/RESTUNTO.EXE out/candidate/RESTUNTO.EXE \
    --output out/restunto-comparison.json

python3 tools/scripts/compare-dos-artifacts.py omf \
    out/reference/seg000.obj out/candidate/seg000.obj \
    --ignore-case --output out/seg000-comparison.json
```

MZ comparison checks the loaded image, loader settings, and relocation
semantics. It reports differences in nonloaded headers/trailers separately.
OMF comparison checks segment order/layout, initialized bytes, names, public
offsets, groups, and fixups while ignoring record partitioning and
comment/debug records. `--ignore-case` only folds ASCII names; use it when
the target linker resolves names without case sensitivity. It never changes
the compared data bytes. Unknown record types, unsupported relocation forms,
or malformed input fail the comparison.
Compiler objects with conflicting overlapping debug data are also rejected;
compare their linked MZ executables instead.

OMF equivalence is limited to emitted object contents; the final MZ
comparison is the stronger check because comments can affect linker behavior.
Exit status is 0 for equivalence, 1 for differences, and 2 for invalid or
unsupported input. DOS runtime and replay tests remain necessary alongside
artifact comparisons.

Run the helper unit tests with:

```sh
python3 -m unittest discover -s tools/scripts/tests -p test_prepare_wasm_original.py
python3 tools/scripts/test-compare-dos-artifacts.py
```
