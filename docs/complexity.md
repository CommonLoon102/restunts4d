# Cyclomatic complexity work

Two completed passes cover the critical collision, rendering and player
simulation routines, followed by all twelve routines in the original remaining
priorities list. Each follow-up routine has its own refactor and regression-test
commit. The wider audit still identifies other functions above the threshold.

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

## Follow-up priorities completed

Baseline: commit `608a854a`; measurements taken with Lizard 1.24.0.
All twelve routines from the remaining priorities list are complete.

| Priority | Routine | Before | After | Highest new helper |
| --- | --- | ---: | ---: | ---: |
| 1 | `track_setup` | 79 | 8 | 16 |
| 1 | `polygon_merge_second_edge` | 77 | 10 | 15 |
| 1 | `update_grip` | 43 | 5 | 9 |
| 1 | `update_opponent_tick` | 45 | 5 | 12 |
| 2 | `load_tracks_menu_shapes` | 177 | 5 | 20 |
| 2 | `run_game` | 85 | 3 | 15 |
| 2 | `loop_game` | 69 | 5 | 20 |
| 2 | `skybox_render` | 56 | 6 | 13 |
| 2 | `line_prepare` | 50 | 7 | 19 |
| 3 | `end_hiscore` | 82 | 3 | 15 |
| 3 | `run_car_menu` | 65 | 6 | 18 |
| 3 | `show_dialog` | 64 | 9 | 14 |

Track setup separates traversal, deferred branches, validation and scenery.
Grip and opponent updates separate surface forces, steering, route progress,
overtaking and speed control. Polygon, line and skybox rendering separate edge
stepping, clipping, projection and drawing while preserving legacy arithmetic.

The editor separates session resources, navigation, drawing and tile edits.
Race and replay loops separate scheduling, input, camera controls and lifecycle
transitions. The remaining UI routines separate layout, animation, resource
management and user interaction. External entry points and observable call
ordering remain intact.

All 135 new production helpers measure at most 20. Excess complexity above 20
for these twelve entry points and their extracted helpers falls from 652 to 0.
Across the complete `src` scan, the number of functions above 20 falls from 41
to 29, and total excess complexity falls from 933 to 281. Those 29 functions
were outside the completed priority list; the whole codebase is not yet below
20 per function.

## Validation of the first pass

- All 14 host tests pass, including new car-speed, collision-geometry and
  primitive-queue regression suites. Clang-format 18.1.8 and EditorConfig pass.
- restunts, repldump and pixldump compile and link with the DOS toolchain.
- Fifteen DOS physics replays (15,346 frames; 15 cars and 14 tracks) match both
  the saved baseline and original oracle byte for byte.
- Temporary before/after differential harnesses matched 3,169,124 collision
  probes, 20,000 renderer scenes, 200,000 car-speed cases and 200,000 player
  updates. Player comparisons also checked dependency call order. Persistent
  collision fingerprints and renderer/car-speed boundary tests remain in
  `src/restunts/tests/`.
- AddressSanitizer and UndefinedBehaviorSanitizer pass the new collision and
  renderer tests.

DOS pixel comparison could not complete: both the saved baseline pixldump and
freshly built pixldump exit with errorlevel 1 before writing output in the twelve
renderer scenarios. The original renderer succeeds. This existing startup
failure remains unresolved; the renderer equivalence evidence for this pass is
from the native differential and queue tests.

## Validation of the completed priorities

- All 26 host regression suites pass via
  `bash src/restunts/tests/run-host-tests.sh`, including one new suite for each
  of the twelve refactored routines.
- Clang-format 18.1.8, EditorConfig, CRLF checks and `git diff --check` pass.
- restunts, repldump and pixldump compile and link with the DOS toolchain.
- All fifteen DOS physics replays (15,346 frames; 15 cars, 14 tracks and four
  opponent races) match the saved baseline, first-pass result and original
  disassembly oracle byte for byte. Each run uses the final repldump executable.
- Original-code comparisons cover 2,592 rectangular track configurations,
  branch and validation boundaries, the fifteen replay tracks, 200,000 grip
  states and 200,000 opponent updates. Opponent comparisons include full game
  state and dependency call order. Persistent fingerprints retain coverage.
- Renderer regression fingerprints match the original C implementations across
  32,768 polygon-edge cases, 200,000 line cases and 32,768 skybox scenes, plus
  focused clipping and horizon boundaries.
- Full-entry UI trace fingerprints match the original implementations across
  420 dialog, 102 car-menu and 360 end-screen scenarios. They cover drawing,
  input, state changes and resource lifetime. Focused tests also cover race
  scheduling, replay camera controls and editor placement/navigation boundaries.
- AddressSanitizer and UndefinedBehaviorSanitizer pass all twelve new suites.

The DOS pixel limitation persists: all twelve fresh renderer cases exit with
errorlevel 1 before creating a PDD file, using the final pixldump executable.
The saved baseline and first-pass executable fail the same cases; the original
renderer produces valid output in all twelve. DOS pixel equivalence therefore
remains unverified. Native renderer comparisons and boundary tests provide the
rendering evidence for these refactors.
