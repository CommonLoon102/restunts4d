# Archived Borland regression oracles

These executable files use the original game disassembly and Borland-built dump
wrappers as independent regression references. They must not be rebuilt with
Watcom or replaced while fixing the C port.

| File | Bytes | Purpose |
| --- | ---: | --- |
| `repldumo.exe` | 244284 | Per-frame game state (`.BIN`) |
| `pixldumo.exe` | 247847 | MurmurHash3_x86_32 framebuffer samples (`.PDO`) |

`SHA256SUMS` identifies the exact archived bytes. The initial references were
copied from `stunts/` before compiler/linker migration. The physics oracle is
unchanged by the Murmur32 migration.

The renderer oracle was deliberately rebuilt with Borland on 2026-09-13 to
replace MD5 with MurmurHash3_x86_32, seed 0. Its pre-change rebuild matched the
previous archived executable byte for byte. Source fingerprints, tool hashes,
and regression results are recorded in [pixldumo-provenance.json](pixldumo-provenance.json).
The original engine assembly is unchanged. The C port's legacy memory model
tracks the new oracle's smaller retained image size; see
[renderer parity](../../../docs/renderer-parity.md).

From the repository root, verify and restore the references on Linux:

```sh
(cd tools/oracles/borland && sha256sum --check SHA256SUMS)
cp tools/oracles/borland/repldumo.exe tools/oracles/borland/pixldumo.exe stunts/
```

Compare `.BIN` against ported `.BNI`, and `.PDO` against ported `.PDD`.
Framebuffer rows contain a decimal frame number, one space, eight lowercase
hexadecimal hash digits (most significant first), and CRLF. Camera and target
options must match on both sides. Old MD5 `.PDO`/`.PDD` files are incompatible;
the shared regression runner rejects and regenerates those caches.

See the [shared regression runner](../../scripts/dumpsrv/README.md) for running
and merging replay comparisons. CI uses these checked-in oracles, generates
Murmur32 renderer references locally, and continues using the published physics
cache. A deliberate oracle change requires provenance and regression review.
