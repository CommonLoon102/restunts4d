# Assembly-to-C migration roadmap

This file distinguishes the agreed top-level roadmap from the small,
independently verified implementation slices used to make progress within a
phase. Slice numbers never advance the top-level phase number.

## Status

| Top-level phase | Status | Notes |
| --- | --- | --- |
| Phase 0 | Complete | Established the inventory, audits, and regression workflow. |
| Phase 1 | In progress | Thirty-one implementation slices are committed and comprehensively replay-tested. |
| Phase 2 | Not started | No Phase 2 implementation has been committed. |
| Phase 3 | Not started | No Phase 3 implementation has been committed. |
| Phase 4 | Not started | No Phase 4 implementation has been committed. |
| Phase 5 | Not started | No Phase 5 implementation has been committed. |
| Phase 6 | Not started | No Phase 6 implementation has been committed. |

## Agreed top-level scope

The seven top-level phases are:

0. Define and measure what a C-only build means, including the assembly
   inventory and regression gates.
1. Establish exact legacy C semantics: fixed-width values, explicit wrapping,
   shifts and rotations, multiply/divide behavior, little-endian decoding,
   explicit serialization, deterministic legacy state, and checked divisions.
2. Produce a C-only headless replay engine.
3. Port the renderer.
4. Port menus, the editor, and input handling.
5. Isolate audio and low-level DOS platform services. External DOS driver
   binaries may still contain assembly.
6. Complete the C-only cutover.

An SDL backend is a later platform-porting project, outside this DOS-focused
assembly-to-C roadmap. It can discard the DOS service layer once the engine is
portable enough to support it.

## Phase 0

- [Phase 0 baseline and audit](asm-to-c-phase0.md)

## Phase 1 implementation slices

1. [Timer counters](asm-to-c-phase1-slice01.md)
2. [DOS upper-memory calls](asm-to-c-phase1-slice02.md)
3. [DOS file I/O](asm-to-c-phase1-slice03.md)
4. [DOS conventional-memory calls](asm-to-c-phase1-slice04.md)
5. [Divide-by-zero vector setup](asm-to-c-phase1-slice05.md)
6. [Obsolete preserved-assembly declarations](asm-to-c-phase1-slice06.md)
7. [Particle primitive branch](asm-to-c-phase1-slice07.md)
8. [Startup sprite clear](asm-to-c-phase1-slice08.md)
9. [Memory-manager register ABI](asm-to-c-phase1-slice09.md)
10. [Stale polygon-edge assembly](asm-to-c-phase1-slice10.md)
11. [Defined penalty traversal](asm-to-c-phase1-slice11.md)
12. [Caller-frame access](asm-to-c-phase1-slice12.md)
13. [Grip stack-residue capture](asm-to-c-phase1-slice13.md)
14. [Player speed update](asm-to-c-phase1-slice14.md)
15. [Pre-render edge merge](asm-to-c-phase1-slice15.md)
16. [Clipped line rasterizer](asm-to-c-phase1-slice16.md)
17. [Fixed-width legacy integer foundation](asm-to-c-phase1-slice17.md)
18. [Fixed-width geometry layouts](asm-to-c-phase1-slice18.md)
19. [Explicit replay-header serialization](asm-to-c-phase1-slice19.md)
20. [Explicit replay-header byte signedness](asm-to-c-phase1-slice20.md)
21. [Fixed-width car-state layout](asm-to-c-phase1-slice21.md)
22. [Fixed-width game-state layout](asm-to-c-phase1-slice22.md)
23. [Fixed-width car-parameter layout and pointer-safe loading](asm-to-c-phase1-slice23.md)
24. [Fixed-width memory-manager records](asm-to-c-phase1-slice24.md)
25. [Fixed-width 2D-shape headers](asm-to-c-phase1-slice25.md)
26. [Fixed-width 3D-shape resource headers](asm-to-c-phase1-slice26.md)
27. [Fixed-width 3D-shape runtime records](asm-to-c-phase1-slice27.md)
28. [Fixed-width transformed-shape records](asm-to-c-phase1-slice28.md)
29. [Fixed-width sprite records and line-offset tables](asm-to-c-phase1-slice29.md)
30. [Fixed-width track-object records](asm-to-c-phase1-slice30.md)
31. [Fixed-width track-object-info records](asm-to-c-phase1-slice31.md)

The historical replay-result filenames still contain labels such as
`phase7` or `phase13`. They are preserved test artifacts and are not roadmap
phase identifiers. Git commit history is likewise left intact; this index and
the renamed documents are the authoritative terminology going forward.
