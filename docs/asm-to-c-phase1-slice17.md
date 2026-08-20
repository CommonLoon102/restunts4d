# Assembly-to-C migration: Phase 1, slice 17

## Fixed-width legacy integer foundation

Phase 1 slice 17 introduces `legacy.h`, the first shared definition of integer
semantics inherited from the original 16-bit executable. It provides exact
signed and unsigned 8-, 16-, and 32-bit types without assuming that C `int` or
`long` has the same width on every compiler.

The header remains compatible with the C89-era Borland compiler. It:

- selects either `int` or `long` as the 32-bit storage type from the compiler's
  limits, which also handles modern LP64 hosts where `long` is 64 bits;
- enforces the byte and type widths with compile-time assertions;
- defines explicit word wrapping, arithmetic-right-shift, word-pair, and
  double-word sign helpers; and
- converts a 16-bit unsigned bit pattern to its signed value without relying on
  an implementation-defined hosted C conversion. The Borland branch retains
  the original direct cast and generated DOS semantics.

This is a foundation, not a repository-wide type conversion. Later Phase 1
slices must adopt it in simulation structures, resource decoding, serializers,
and other arithmetic paths.

## First adoption

The clipped line rasterizer translated and oracle-verified in slice 16 is the
first adopter. Its line descriptor, temporary registers, interpolation table,
and fixed-point intermediates now use explicit legacy types. All signed word
comparisons use the bit-pattern conversion helper, and double-word sign tests
no longer cast an out-of-range unsigned value to a signed C type.

On Borland, these aliases resolve to the same `unsigned char`, `short`, and
`long` widths used by slice 16. The public entry points retain their existing
DOS ABI. The algorithm and the assembly-owned interpolation table are otherwise
unchanged, so the routines remain `c_active_with_asm` in the inventory.

No `seg0xx.asm` source was changed.

## Verification

- A hosted C89 compile with strict warnings verifies the header and its LP64
  type selection and conversion helpers.
- Borland C++ 5.2 compiles and links both `RESTUNTS.EXE` and `REPLDUMP.EXE`.
- The Phase 0 audit remains current and reports zero active inline assembly,
  zero preserved-assembly calls from C, and 86 assembly link inputs.
- No ad-hoc local replay state dump was run for this slice.
- The serial comprehensive replay run returned an empty
  `partitions_all_phase1_slice17_legacy_types.txt`, byte-identical to the clean
  Phase 1 slice 16 report.
