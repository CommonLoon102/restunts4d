# Renderer parity with the original game

Pixel comparisons use the unchanged Borland oracle in `tools/oracles/borland`.
Rendering and simulation are not independent in the original executable: its
physics reads values left in stack slots by earlier rendering calls. The C port
represents those values explicitly instead of depending on undefined C locals,
adjacent allocations, or the host compiler's stack layout.

## Sphere bounds

In `asmorig/seg006.asm`, `loc_25C92` first submits `(x - radius, y - radius)`
to `rect_adjust_from_point`. For its second point it assigns `x + radius` to X,
then overwrites that same X with `y + radius`, leaving Y unchanged. The C sphere
path previously used a conventional bounding square. Crash explosions scale
from this rectangle, so their placement and size differed even with identical
simulation state and source pixels. `shape3d_adjust_sphere_bounds` reproduces
these writes; wheel bounds retain their distinct original calculation.

## Renderer values consumed by physics

The original `get_a_poly_info` calls leave saved frame pointers, far return
addresses, rasterizer locals, and arguments in the stopped player's wheel-heading
slots. Its polygon-data pointer and pattern temporary overlap the opponent's
heading slots. The preceding skybox writes some of those slots too, including
on branches that clip away the whole sky rectangle. A slot retains its earlier
value on branches that do not write it.

`shape3d_set_legacy_render_stack` binds those logical slots to the existing
`legacy_execution_residue` buffers during sampled replay rendering. Rasterizer
and skybox paths retain the values written by their corresponding original
instructions. The existing stopped-wheel physics then consumes these buffers.
This restores the original simulation after stops and crashes as well as the
immediately rendered image.

A separate reused stack word holds the SI register saved by `update_grip`.
After a capped collision scan, physics can consume it as the fourth wheel's
contact distance. It is not always 80: the sampled dump loop supplies its output
handle, the single-image loop supplies its original argv offset, and checkpoint
and race-start paths overwrite it with their own index or distance. Explicit
caller parameters carry these values through the C simulation.

## Caller and address calculation

`pixldump/legacy_context.c` describes the archived executable's ABI, not a
particular replay. It derives the load segment from the DOS PSP and calculates
original stack placement from the executable path and decoded argument lengths.
The polygon buffer's logical segment follows the original allocation order,
using the sizes of the loaded resources. Thus replay names, optional extensions,
DOS directories, and environment placement do not require special cases.

The original assembly and archived executable remain unchanged. Ordinary C
game callers keep their existing default simulation contract; the pixel-dump
wrapper selects its own original caller context. BMP mode simulates without
intermediate rendering, while hash mode renders frame 0 and every fifth frame.
Those modes can therefore reach different states in the original game itself.

## Deterministic original dump capture

The Watcom original pixel-dump wrapper masks timer IRQ0 after `init_main`.
The existing `legacy_timer_shutdown` restores the timer interrupt and its BIOS
handler on exit.

During interactive gameplay, the timer provides real-time timing and background
services:

- It advances counters used for delays, timeouts and pacing, and periodically
  calls the original BIOS timer handler for system timekeeping.
- Audio callbacks advance music, sound effects and sound-driver updates.
- The frame callback schedules input recording and plays queued car-audio
  updates.

`pixldumo` advances the recorded replay explicitly, one simulation frame at a
time. It captures images without needing real-time pacing, live input recording
or audio playback, so these timer services are unnecessary during offline
capture. The interactive game still needs them; masking IRQ0 is specific to the
Watcom original pixel-dump wrapper.

The interrupt handler saves CPU registers on the interrupted renderer's stack,
runs its callbacks, then restores the registers. Restoring the registers does
not erase their saved bytes from stack memory. Stopped-car physics can later
reuse those slots without initializing its wheel headings and read the leftover
values instead. Masking IRQ0 prevents these timing-dependent writes while
preserving the ordinary renderer residue required by the cached PDOs, without
changing the original engine's code, stack frames, or physics calculations.
The rule applies throughout hash and BMP capture, independently of the replay,
frame number, car, or crash state.

Repeat checks must retain the cached PDO: regenerating it with the unchanged
Borland executable can encounter the same interrupt-driven variation.

### Why the C dump keeps the timer enabled

The C-based `pixldump` does not need IRQ0 masking to avoid this failure.
`restore_stopped_wheel_headings` reads the explicit `legacy_execution_residue`
arrays, which have defined initial values. The renderer updates those arrays
through `shape3d_set_legacy_render_stack`; physics does not read leftover CPU
stack bytes. Interrupt register pushes therefore cannot supply wheel headings
through the original stack-reuse mechanism.

The C dump also bypasses interactive race initialization and does not register
`frame_callback`. That callback cannot advance `frame_callback_count`, which
the renderer uses for the material animation phase at frame zero. The registered
audio callbacks do not write `legacy_execution_residue`, and replay simulation
advances explicitly by recorded frames rather than timer ticks.

Ten repeated runs of the unchanged C executable, with IRQ0 enabled, produced
identical complete output for `0696.rpl` and matched the cached PDO every time.
No equivalent C-renderer failure was reproduced. IRQ0 masking therefore remains
specific to the Watcom original `pixldumo`; the C `pixldump` keeps its timer
enabled.

## Track-boundary values

A multi-tile track object at the map edge can read index 30 of the original
30-word position tables. In the archived data segment, the following row word
is the closed hi-hat resource pointer's offset; the following column word is
the live elapsed replay time. Watcom places globals differently, so indexing
past the C arrays cannot reproduce those values reliably.

`track_row_position` and `track_column_position` model the boundary words
explicitly for collision, route, and renderer callers. Audio resource mapping
retains the original hi-hat offset across replay initialization. The fix applies
to every multi-tile boundary object, including the false collision in `a0457.rpl`.
Host tests cover row and column boundaries and the audio offset's lifetime.

## Regression checks

Run `bash src/restunts/tests/run-host-tests.sh` for the host tests. Coverage
includes sphere versus wheel bounds, rasterizer primitive types, retained values
on skipped branches, skybox clipping, player and opponent stopped-wheel motion,
caller-register suspension effects, and DOS argv/address calculations.

Use `tools/scripts/pixelcheck.sh REPLAY.rpl false 2 0` for a complete sampled
comparison with existing executables, or append a frame for a BMP comparison.
Both sides must use the same camera, target, arguments and DOS environment.
A match requires complete output through the replay's last sampled frame; equal
partial files are not evidence of renderer parity.
