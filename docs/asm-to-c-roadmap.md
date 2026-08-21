# Assembly-to-C migration roadmap

This file distinguishes the agreed top-level roadmap from the small,
independently verified implementation slices used to make progress within a
phase. Slice numbers never advance the top-level phase number.

## Status

| Top-level phase | Status | Notes |
| --- | --- | --- |
| Phase 0 | Complete | Established the inventory, audits, and regression workflow. |
| Phase 1 | In progress | Eighty-nine implementation slices are committed and comprehensively replay-tested. |
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
32. [Fixed-width track work tables](asm-to-c-phase1-slice32.md)
33. [Recovered hidden track word tables](asm-to-c-phase1-slice33.md)
34. [Explicit unsigned track byte buffers](asm-to-c-phase1-slice34.md)
35. [Exact 32-bit timer state and APIs](asm-to-c-phase1-slice35.md)
36. [Exact 16-bit physics stack-word buffers](asm-to-c-phase1-slice36.md)
37. [Exact opponent-route work-state widths](asm-to-c-phase1-slice37.md)
38. [Portable 2D-shape table serialization](asm-to-c-phase1-slice38.md)
39. [Portable generic resource-directory parsing](asm-to-c-phase1-slice39.md)
40. [Exact memory-manager byte-count APIs](asm-to-c-phase1-slice40.md)
41. [Exact physics and culling double-word state](asm-to-c-phase1-slice41.md)
42. [Exact file byte-count APIs](asm-to-c-phase1-slice42.md)
43. [Exact car-state initializer ABI](asm-to-c-phase1-slice43.md)
44. [Defined polygon-facing determinant](asm-to-c-phase1-slice44.md)
45. [Defined fixed-point multiply and scale](asm-to-c-phase1-slice45.md)
46. [Exact polar-radius word semantics](asm-to-c-phase1-slice46.md)
47. [Defined polar-angle word semantics](asm-to-c-phase1-slice47.md)
48. [Defined matrix fixed-point arithmetic](asm-to-c-phase1-slice48.md)
49. [Defined vector projection word semantics](asm-to-c-phase1-slice49.md)
50. [Defined projection setup word semantics](asm-to-c-phase1-slice50.md)
51. [Defined vector interpolation arithmetic](asm-to-c-phase1-slice51.md)
52. [Defined plane arithmetic word semantics](asm-to-c-phase1-slice52.md)
53. [Defined rectangle adjustment word semantics](asm-to-c-phase1-slice53.md)
54. [Defined trigonometric and axis-rotation word semantics](asm-to-c-phase1-slice54.md)
55. [Defined vector classification arithmetic](asm-to-c-phase1-slice55.md)
56. [Corrected rectangle predicate semantics](asm-to-c-phase1-slice56.md)
57. [Restored legacy sorting semantics](asm-to-c-phase1-slice57.md)
58. [Defined rectangle-list byte semantics](asm-to-c-phase1-slice58.md)
59. [Restored matrix-composition semantics](asm-to-c-phase1-slice59.md)
60. [Defined plane-rotation word semantics](asm-to-c-phase1-slice60.md)
61. [Restored non-unit-pixel rectangle semantics](asm-to-c-phase1-slice61.md)
62. [Defined clip-rectangle rotation word semantics](asm-to-c-phase1-slice62.md)
63. [Defined RPM-update word semantics](asm-to-c-phase1-slice63.md)
64. [Defined aerodynamic-table arithmetic](asm-to-c-phase1-slice64.md)
65. [Defined player-physics speed scaling](asm-to-c-phase1-slice65.md)
66. [Defined torque/mass acceleration arithmetic](asm-to-c-phase1-slice66.md)
67. [Defined opponent-speed damping arithmetic](asm-to-c-phase1-slice67.md)
68. [Defined unsigned speed-word averaging](asm-to-c-phase1-slice68.md)
69. [Defined signed speed-difference word semantics](asm-to-c-phase1-slice69.md)
70. [Defined gear-knob word arithmetic](asm-to-c-phase1-slice70.md)
71. [Defined speed-integration word arithmetic](asm-to-c-phase1-slice71.md)
72. [Defined RPM-limiter word arithmetic](asm-to-c-phase1-slice72.md)
73. [Defined signed-word/double-word wheel-position arithmetic](asm-to-c-phase1-slice73.md)
74. [Defined compound wheel-position arithmetic](asm-to-c-phase1-slice74.md)
75. [Defined four-wheel centroid arithmetic](asm-to-c-phase1-slice75.md)
76. [Defined signed double-word coordinate projections](asm-to-c-phase1-slice76.md)
77. [Defined low-word wheel-offset differences](asm-to-c-phase1-slice77.md)
78. [Defined opponent stack-residue word extraction](asm-to-c-phase1-slice78.md)
79. [Defined wheel interpolation arithmetic](asm-to-c-phase1-slice79.md)
80. [Defined wrapped vector-delta scaling](asm-to-c-phase1-slice80.md)
81. [Defined direct signed-word negations](asm-to-c-phase1-slice81.md)
82. [Defined compound signed-word negations](asm-to-c-phase1-slice82.md)
83. [Defined shifted compound negations](asm-to-c-phase1-slice83.md)
84. [Defined direct signed-word shifts](asm-to-c-phase1-slice84.md)
85. [Defined direct signed-word addition and subtraction](asm-to-c-phase1-slice85.md)
86. [Defined four-vector word sums](asm-to-c-phase1-slice86.md)
87. [Defined surface sums and jump-counter increment](asm-to-c-phase1-slice87.md)
88. [Defined orientation word offsets](asm-to-c-phase1-slice88.md)
89. [Defined signed collision-table decoding](asm-to-c-phase1-slice89.md)

The historical replay-result filenames still contain labels such as
`phase7` or `phase13`. They are preserved test artifacts and are not roadmap
phase identifiers. Git commit history is likewise left intact; this index and
the renamed documents are the authoritative terminology going forward.
