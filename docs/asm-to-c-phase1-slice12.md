# Assembly-to-C migration: Phase 1, slice 12

## Caller-frame access expressed in C

Phase 1 slice 12 removes the last two active inline-assembly statements from
`math.c`. They loaded `mat_rot_zxy`'s saved caller frame pointer into a local
word before invoking the existing DOS compatibility helper.

For the DOS build, `mat_rot_zxy` now derives the same word with the address of
its own stack local:

```c
caller_bp = *(unsigned short far*)MK_FP(
	_SS, FP_OFF(&caller_bp) + sizeof(caller_bp)
);
```

Borland places `caller_bp` at `BP-2`, so adding its two-byte size addresses
the saved BP at `SS:BP`. The generated listing was inspected and contains the
same two instructions as the removed inline assembly:

```asm
mov ax, word ptr [bp]
mov word ptr [bp-2], ax
```

The local frame size, helper argument, and call sequence are unchanged. This
is intentionally a transitional DOS compatibility path: it remains dependent
on Borland's 16-bit stack layout, but it is ordinary C and is excluded when
`RESTUNTS_DOS` is not defined. A future portable backend must replace the
underlying legacy stack-residue behavior explicitly rather than carry this
DOS frame convention forward.

No `seg0xx.asm` source was changed.

## Rejected semantic replacements

Several explicit-state prototypes were tested before retaining the
instruction-equivalent C expression. Each produced the same four remote
replay mismatches (`0034`, `0664`, `0696`, and `0701`):

- a scoped pointer to the presumed wheel-angle local;
- synchronization limited to the six direct matrix calls;
- an active flag plus direct local initialization;
- restored helper and matrix-call stack depths without caller BP;
- explicit writes to the compiler locals occupying `BP-320..BP-314`, both
  early and at all six call sites.

These results show that the compatibility dependency is broader than the
initial wheel-angle interpretation and cannot yet be replaced safely in
isolation. The reports were preserved under phase-specific names in the
ignored `stunts` test directory.

## Verification

- The DOS compiler builds both `RESTUNTS.EXE` and `REPLDUMP.EXE` with no new
  warning in the final changed code.
- The generated DOS instructions at the replaced site match the Phase 1 slice 11
  sequence.
- The Phase 0 audit reports 24 active inline-assembly sites, down from 26,
  and the checked-in inventory is current.
- No ad-hoc local replay was run for this slice.
- The serial comprehensive remote run returned an empty
  `partitions_all_phase12_c_frame_expression.txt`, byte-identical to the clean
  Phase 1 slice 11 result.
