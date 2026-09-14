# Renderer parity with the original game

Pixel comparisons use the independent Borland oracle in `tools/oracles/borland`.
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
using the sizes of the loaded resources. Its retained image size is 39E3 DOS
paragraphs for the Murmur32 oracle (the previous MD5 build used 3A1A). Hash
wrapper changes must keep this model synchronized with the original link map.
Thus replay names, optional extensions, DOS directories, and environment
placement do not require special cases.

The original engine assembly remains unchanged. Ordinary C
game callers keep their existing default simulation contract; the pixel-dump
wrapper selects its own original caller context. BMP mode simulates without
intermediate rendering, while hash mode renders frame 0 and every fifth frame.
Those modes can therefore reach different states in the original game itself.

## Deterministic original dump capture

Both original dump wrappers (`repldumo` and `pixldumo`) mask timer IRQ0
immediately after `init_main`, before replay initialization and capture.
The shared `platform/dos/dump_timer.h` emits inline instructions for Watcom and
Borland without adding a stack frame. It preserves other PIC mask bits and the
CPU interrupt flag. Normal shutdown restores the timer interrupt and BIOS
handler; physics failure returns run the registered exit handlers too.

During interactive gameplay, the timer advances pacing and timeout counters,
chains to the BIOS clock, advances audio callbacks and schedules frame/input
recording. Offline dump tools advance the replay explicitly by recorded frames;
these real-time services are unnecessary during capture. Interactive gameplay
retains its timer.

Timer activity can affect the original simulation through two different forms
of legacy memory reuse. Interrupt handlers leave saved bytes in reused stack
slots, as in stopped-wheel physics. Audio callbacks also mutate driver memory
which unchecked route indexing can read. Both mechanisms are removed during
offline capture by masking IRQ0, without changing the original engine code.

#### Physics-only reproduction: `1947.rpl`

At frame 1021, `detect_penalty` in `asmorig/seg001.asm` follows the route
sentinel `-1`. The wrapped primary-route read (`td01[-1]`) yields 6681 for this
replay, beyond its 134 route pieces. The original lookup then reads
`visited[6681]` using `SS:BP + 6681 - 0x5a2`, outside the local visited array.
With `repldumo.exe "1947" 1` and the historical Borland layout, this aliases
byte `0x1c5` in the loaded `PC15.DRV` audio driver.

The route search writes 1 to this byte when visiting the invalid index. The
registered `audiodriver_timer` in `asmorig/seg028.asm` calls `sub_39700`, which
calls the driver's entry at offset `0x30`. That entry jumps to the PC-speaker
channel update loop at `0x5df`; it rewrites the aliased channel-state byte to 0.
The next route search then revisits invalid route 6681, follows its zero link,
and searches from route 0 to route 58 instead of backtracking to route 125.
The first dump difference is at frame 1021, and 48 of the 2,604 state records
differ. This is a live audio-data dependency, even without rendering or a stop.

The recorded investigation used these controls to separate timer activity from
memory layout:

| Executable/control | Timer | Result for CI's arguments |
| --- | --- | --- |
| Historical Borland executable | Enabled | 40/40 repeats produce the route mismatch |
| Historical executable, only IRQ-enable immediate changed | Masked | 5/5 match all C states |
| Refreshed Borland executable | Masked | 40/40 match all C states |
| Refreshed executable, mask instruction neutralized | Enabled | 20/20 match; changed heap placement hides this particular alias |

The historical-layout diagnostic records the same BP, SS and driver address
in both timer cases. The aliased byte changes from 0 with IRQ0 enabled to 1
with IRQ0 masked, changing the route result from confirmed/previous 85/58 to
125/125. Thus recompilation alone is insufficient proof of the fix: it can
move the accidental read to another driver byte.

Adding `.rpl` to the argument shifts the original argument stack by four bytes.
The same invalid lookup then lands at driver offset `0x1c1`, initialized to 127,
which hides the mismatch. Stdout redirection did not change this result in the
controlled four-variant comparison. No replay-specific physics adjustment is
needed; the shared capture policy prevents the timer from changing either
aliased driver data or stack residue on every replay.

The refreshed Borland oracles use the same helper as the current original
dump wrappers. Their frozen source/toolchain reconstruction first reproduces
the old binaries byte for byte, then applies the capture changes. See the
[oracle provenance guide](../tools/oracles/borland/README.md).

### C dump behavior

The C `pixldump` keeps its existing timer behavior. Its legacy simulation values
are represented explicitly in `legacy_execution_residue` arrays, independent of
CPU interrupt stack writes. It bypasses interactive race initialization and does
not register `frame_callback`; the registered audio callbacks do not write the
modeled simulation residue. The headless C `repldump` does not initialize the
interactive timer services.

### Verification

The standalone DOS timer regression checks that capture masks only IRQ0, preserves IF
whether initially set or cleared, and remains idempotent. The replay-dump
exit regression verifies that an output-creation error restores the timer
vector, unmasks IRQ0 and returns with interrupts enabled.

Compare complete dumps using the same invocation as CI: a quoted replay basename
without its extension, and no DOS stdout redirection. Argument variants can
change the original stack placement and hide the failure. Preserve
failed outputs and reference hashes when repeating a test. Historical unmasked
BIN/PDO caches must be regenerated or checked against the refreshed oracle;
complete length alone does not prove deterministic contents.

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
