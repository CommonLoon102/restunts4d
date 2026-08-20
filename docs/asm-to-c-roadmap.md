# Assembly-to-C migration roadmap

This file distinguishes the agreed top-level roadmap from the small,
independently verified implementation slices used to make progress within a
phase. Slice numbers never advance the top-level phase number.

## Status

| Top-level phase | Status | Notes |
| --- | --- | --- |
| Phase 0 | Complete | Established the inventory, audits, and regression workflow. |
| Phase 1 | In progress | Thirteen implementation slices are committed and comprehensively replay-tested. |
| Phase 2 | Not started | No Phase 2 implementation has been committed. |
| Phase 3 | Not started | No Phase 3 implementation has been committed. |
| Phase 4 | Not started | No Phase 4 implementation has been committed. |
| Phase 5 | Not started | No Phase 5 implementation has been committed. |
| Phase 6 | Not started | No Phase 6 implementation has been committed. |

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

The historical replay-result filenames still contain labels such as
`phase7` or `phase13`. They are preserved test artifacts and are not roadmap
phase identifiers. Git commit history is likewise left intact; this index and
the renamed documents are the authoritative terminology going forward.
