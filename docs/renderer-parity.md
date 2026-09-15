# Renderer parity with the original game

Pixel comparisons use the original assembly built by `pixldump-original`.
The archived Borland renderer retains its historical full-redraw capture mode.
Rendering and simulation are not independent in the original executable: its
physics reads values left in stack slots by earlier rendering calls. The C port
represents those values explicitly instead of depending on undefined C locals,
adjacent allocations, or the host compiler's stack layout.

## Unsupported track skybox selectors

A track's byte 900 selects one of five horizon resources: 0 desert, 1 tropical,
2 alpine, 3 city, or 4 country. In a replay, the embedded track follows the
26-byte header, so its selector is at file offset 926 (`0x39E`).

ZCT77 and its six golden replays contain `0xFF` here. Original `load_skybox`
checks bit `0x08` before indexing the resource table. When that bit is set,
it skips image loading and leaves the previous image pointers and heights
untouched. On a fresh replay start, the pointers and heights are zero. With
incremental redraws, the zero maximum height creates an empty horizon rectangle;
`rectlist_add_rect` can then split and merge recursively without making progress,
corrupting the stack and hanging. The captured original-engine failure in 0691
occurs at frame 122, with horizon rectangle `[0,320,28,28]`.
The missing images do not guarantee a hang: interactive `restunts.exe` can play
unmodified 0707 from a fresh start with plain blue sky. Recursion requires a
particular combination of dirty rectangles, which varies with the render path.

Visiting a normal track preview first loads and then frees its skybox. The loader
for ZCT77 skips loading replacements, while the preview draws through the stale
pointers. Reused image memory explains the multicolored horizon. Retained nonzero
heights can avoid the empty-rectangle failure, but the result depends on earlier
menu activity. This is not a stable initialization workaround.

Keep the original assembly and executable intact by repairing unsupported
selectors in the renderer's input copies. `tools/scripts/normalize-track-skybox.py`
retains values 0 through 4 and maps every other value to desert by default. It
changes only the scenery selector, preserving the geometry, terrain, replay
header, and recorded controls byte for byte. This deliberately compares a
supported scenery selection; it does not reproduce the undefined appearance of
the original invalid input. Other invalid values can also index outside the
five resource names, so merely clearing bit `0x08` is insufficient.

For interactive playback, make repaired copies of both the track and replay:

```sh
python3 tools/scripts/normalize-track-skybox.py \
    --output-directory out/skybox-fixed stunts/ZCT77.TRK stunts/0707.rpl
```

Load these repaired files in a game directory. Repairing the `.TRK` alone does
not repair the track embedded in an existing `.RPL`. `--skybox 1` through
`--skybox 4` select another fallback; already valid scenery is preserved.
`--check` audits files or directories without changing them. The tool accepts
1802-byte tracks and this branch's 26-byte-header replay format and rejects
inconsistent file lengths before modifying a batch.

CI runs the normalizer only on its temporary renderer game directory, before
either executable starts. The archived corpus and physics phase retain their
original inputs. A changed replay gets a `.PDO.pending` marker so the shared
runner regenerates an old renderer cache. Valid inputs and their caches are
untouched. Run `python3 tools/scripts/test-normalize-track-skybox.py` for the
all-selector and corpus-preservation checks.

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
`legacy_execution_residue` buffers during replay rendering. Rasterizer
and skybox paths retain the values written by their corresponding original
instructions. The existing stopped-wheel physics then consumes these buffers.
This restores the original simulation after stops and crashes as well as the
immediately rendered image.

Camera setup also writes the opponent's fourth heading word, at
`get_a_poly_info` BP-58. `select_cliprect_rotate` calls `mat_rot_zxy` for the
forward and inverse view rotations. With multiple rotation axes, its nested
`mat_multiply` leaves the far return code segment in that word. A single-axis
constructor leaves its return instruction offset instead. Identity and cardinal
yaw reuse existing matrices and leave the word unchanged. Incremental sky
rendering can skip assigning its local rectangle, so its top coordinate no
longer replaces this camera value. `shape3d_retain_legacy_view_rotation` models
these calls before the skybox and polygon paths apply their own writes.

This dependency affects simulation, even while the stopped opponent is outside
the camera view. In `r0136.rpl`, the opponent stops at frame 434 and the first
visible hash mismatch was at frame 807 in the Borland comparison. `0696.rpl`
also exposed the missing camera value after a crash. The fix uses
rotation operations and the relocated code segment, independent of replay
names, frame numbers, cars, or tracks.

The renderer handoff also takes precedence at the first stopped player tick.
The physics-only caller reconstructs headings from opponent wheel coordinates
at that transition. When rendering has overwritten those slots, repeating that
reconstruction discards the actual values. In `0701.rpl`, the original player
physics enters frame 1138 with the preceding line rasterizer's retained words;
the C fallback instead substituted opponent coordinates and changed the player's
pose. `legacy_render_player_headings_active` selects the supplied renderer
buffer while it is bound, preserving the physics-only caller's existing behavior.

A separate reused stack word holds the SI register saved by `update_grip`.
After a capped collision scan, physics can consume it as the fourth wheel's
contact distance. It is not always 80: the sampled dump loop supplies its output
handle, the single-image loop supplies its original argv offset, and checkpoint
and race-start paths overwrite it with their own index or distance. Explicit
caller parameters carry these values through the C simulation.

## Caller and address calculation

`pixldump/legacy_context.c` describes the original Borland executable's ABI, not a
particular replay. It derives the load segment from the DOS PSP and calculates
original stack placement from the executable path and decoded argument lengths.
The Borland hash loop places `get_a_poly_info` BP 558 bytes below argv; the BMP
loop places it 556 bytes below argv. BMP passes one more argument word but saves
two fewer registers. The wrapper selects the matching depth for each mode.
The polygon buffer's logical segment follows the original allocation order,
using the sizes of the loaded resources. Its retained image size is 39E6 DOS
paragraphs for the incremental Borland oracle (the earlier sampled Murmur32
build used 39E3). That three-paragraph difference shifts the polygon buffer's
segment, which is itself consumed as the opponent's third stopped-wheel
heading. It accounts for the `0698.rpl` mismatch even when visible geometry
initially agrees. Wrapper changes must keep this model synchronized with the
original link map. CI runs `tools/scripts/test-pixldump-legacy-layout.py` against
the freshly built oracle to check the retained image size, initial CRT stack
pointer, and polygon code segment.
Thus replay names, optional extensions, DOS directories, and environment
placement do not require special cases.

The Watcom original renderer must retain the same heap boundary. Its linker
places `ENDSEG` at physical offset `0x39E50`; original startup uses
`seg endseg + 1`, retaining `0x39E6` paragraphs. The former MD5 boundary retained
`0x3A1A` paragraphs and shifted resource addresses by 52 paragraphs, or 832 bytes.
Polygon-buffer segment words left on the stack can then become different
stopped-opponent wheel headings, even with the same original game assembly.
WLINK rejects code that grows past the fixed boundary. Keep this linker setting
and the C address model synchronized when the reference wrapper changes.

The original engine assembly remains unchanged. Ordinary C
game callers keep their existing default simulation contract; the pixel-dump
wrapper selects its own original caller context. Both BMP and hash modes render
frame 0 and every subsequent frame with incremental redraws. Hash mode records
every frame; BMP mode records only the requested frame. Their caller
register contexts still differ as described above.

The wrapper initializes the changed-region arrays and requests one full redraw
for the first frame, then decrements that request after presentation. This keeps
both erasing and presenting dirty regions active throughout capture. Hash dumps
start with `PIXLDUMP 2` and CRLF so the regression runner regenerates caches from
the old full-redraw, sampled-rendering wrapper.

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

Use `tools/scripts/pixelcheck.sh REPLAY.rpl false 2 0` for a complete incremental
comparison with existing executables, or append a frame for a BMP comparison.
Both sides must use the same camera, target, arguments and DOS environment.
A match requires complete output through the replay's final frame; equal
partial files are not evidence of renderer parity.
