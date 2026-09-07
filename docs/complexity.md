# Cyclomatic complexity work

The first pass prioritizes collision geometry, rendering and per-frame player
simulation. These routines run throughout a race and combine many independent
responsibilities. The project-wide scan also identifies the editor and race/UI
loops as follow-up work.

## Measurement

Use Lizard 1.24.0 with standard cyclomatic complexity, including individual
switch cases and boolean operators. Do not use modified switch counting or
macro expansion when comparing these numbers. From the repository root, in a
Python virtual environment:

```sh
python -m pip install lizard==1.24.0
lizard -l cpp -C 20 -s cyclomatic_complexity src
```

This scans project C/H files, including the DOS platform, dump tools and host
tests. Bundled tools and reference assembly are outside this measurement.
Lizard reports remaining functions above 20 as warnings and exits nonzero;
this is an audit command, not a new build requirement.

## Critical routines refactored

Baseline: commit `18ea36f8`; measurements taken with Lizard 1.24.0.

| Routine | Before | After | Highest complexity in its file after |
| --- | ---: | ---: | ---: |
| `build_track_object` (`trackobj.c`) | 310 | 17 | 40 |
| `shape3d_transform_and_queue` (`shape3d.c`) | 84 | 13 | 13 |
| `update_player_tick` (`state.c`) | 60 | 9 | 12 |
| `update_car_speed` (`statecar.c`) | 52 | 3 | 15 |

Track handling now separates tile resolution, physical-model geometry, terrain
adjustments and final collision output. Pipe/corkscrew cross sections and terrain
orientations share lookup tables. The remaining 40-complexity model dispatcher
is a flat switch; some individual geometry handlers still exceed 20.

Rendering separates the per-instance transform cache, culling, primitive clipping,
primitive emission and queue insertion. Player updates separate route penalties,
route selection, wrong-way recovery, guidance and finish detection. Car speed
updates separate transmission, pedal forces, wrapped speed arithmetic, wheel
synchronization and engine limiting.

Across these four files, excess complexity above 20 per function falls from 426
to 36. This accounts for every extracted helper, rather than only comparing the
entry points. Existing state formats, legacy arithmetic and external entry points
remain in use.

## Remaining priorities

| Priority | Routine | Complexity | Reason |
| --- | --- | ---: | --- |
| 1 | `track_setup` | 79 | Track loading, route construction and validation |
| 1 | `polygon_merge_second_edge` | 77 | Rasterization decisions and clipping boundaries |
| 1 | `update_grip` / `update_opponent_tick` | 43 / 45 | Player and opponent simulation |
| 2 | `load_tracks_menu_shapes` | 177 | Largest remaining routine; editor state and input handling |
| 2 | `run_game` / `loop_game` | 85 / 69 | Race and replay lifecycle transitions |
| 2 | `skybox_render` / `line_prepare` | 56 / 50 | Remaining renderer hotspots |
| 3 | `end_hiscore` / `run_car_menu` / `show_dialog` | 82 / 65 / 64 | UI decisions and dialog state |

Continue by extracting responsibilities with explicit inputs and preserving
observable ordering. Retain the original disassembly as the replay oracle.
Use focused boundary tests and byte-for-byte replay comparisons when changing
simulation or rendering; entry-point complexity alone is insufficient evidence.

## Validation of this pass

- All 14 host tests pass, including new car-speed, collision-geometry and
  primitive-queue regression suites. Clang-format 18.1.8 and EditorConfig pass.
- RESTUNTS, REPLDUMP and PIXLDUMP compile and link with the DOS toolchain.
- Fifteen DOS physics replays (15,346 frames; 15 cars and 14 tracks) match both
  the saved baseline and original oracle byte for byte.
- Temporary before/after differential harnesses matched 3,169,124 collision
  probes, 20,000 renderer scenes, 200,000 car-speed cases and 200,000 player
  updates. Player comparisons also checked dependency call order. Persistent
  collision fingerprints and renderer/car-speed boundary tests remain in
  `src/restunts/tests/`.
- AddressSanitizer and UndefinedBehaviorSanitizer pass the new collision and
  renderer tests.

DOS pixel comparison could not complete: both the saved baseline PIXLDUMP and
freshly built PIXLDUMP exit with errorlevel 1 before writing output in the twelve
renderer scenarios. The original renderer succeeds. This existing startup
failure remains unresolved; the renderer equivalence evidence for this pass is
from the native differential and queue tests.
