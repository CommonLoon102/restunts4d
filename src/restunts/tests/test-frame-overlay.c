#include <assert.h>
#include <string.h>

#include "../c/frame_internal.h"
#include "../c/externs.h"
#include "../c/shape3d.h"
#include "../c/shape3d_internal.h"
#include "../c/shape2d.h"
#include "../c/shape2d_internal.h"
#include "../c/residue.h"
#include "../c/platform.h"
#include "../c/ui_text.h"

#undef strcmp
#undef memcpy

struct LEGACY_EXECUTION_RESIDUE legacy_execution_residue;
legacy_s16 legacy_render_player_headings_active;

static legacy_u8 crack_lines[16];
static legacy_u8 crack_info[6];
static legacy_u16 crack_offset;
static legacy_u16 last_line[DRAW_LINE_WORD_COUNT];
static unsigned drawn_lines;
static legacy_s16 player[LEGACY_RESIDUE_WORD_COUNT];
static legacy_s16 opponent[LEGACY_RESIDUE_WORD_COUNT];

legacy_s8 far *locate_shape_alt(legacy_s8 far *resource, const legacy_s8 *name)
{
	assert(resource == gameresptr);
	if (strcmp((const char *)name, "crak") == 0) {
		return (legacy_s8 far *)crack_lines;
	}
	assert(strcmp((const char *)name, "cinf") == 0);
	return (legacy_s8 far *)crack_info;
}

legacy_u16 dos_memory_pointer_offset(const void far *pointer)
{
	assert(pointer == crack_lines);
	return crack_offset;
}

void sprite_draw_line_from_setup(const legacy_u16 *line)
{
	memcpy(last_line, line, sizeof(last_line));
	drawn_lines++;
}

static void assert_words(const legacy_s16 *words, legacy_s16 a, legacy_s16 b, legacy_s16 c,
						 legacy_s16 d)
{
	assert(words[0] == a && words[1] == b && words[2] == c && words[3] == d);
}

static void set_line(unsigned index, legacy_s16 x1, legacy_s16 y1, legacy_s16 x2, legacy_s16 y2)
{
	LEGACY_WRITE_U16_LE(crack_lines + index * 8, x1);
	LEGACY_WRITE_U16_LE(crack_lines + index * 8 + 2, y1);
	LEGACY_WRITE_U16_LE(crack_lines + index * 8 + 4, x2);
	LEGACY_WRITE_U16_LE(crack_lines + index * 8 + 6, y2);
}

static void reset_overlay(legacy_s16 dirty_rects)
{
	struct SHAPE3D_LEGACY_OPPONENT_RENDER_CONTEXT context;
	unsigned i;

	memset(&context, 0, sizeof(context));
	context.wheel_headings = opponent;
	shape3d_set_legacy_render_stack(player, 0x9000, 0x4000, &context);
	for (i = 0; i < LEGACY_RESIDUE_WORD_COUNT; i++) {
		player[i] = 100 + i;
		opponent[i] = 200 + i;
	}
	drawing_sprite.sprite_raster_left = 0;
	drawing_sprite.sprite_raster_right = 320;
	drawing_sprite.sprite_top = 0;
	drawing_sprite.sprite_bottom = 200;
	slow_video_mgmt_copy = dirty_rects;
	framespersec = 20;
	dialog_fnt_colour = 7;
	drawn_lines = 0;
	crack_offset = 11;
	LEGACY_WRITE_U16_LE(crack_info, 2);
	LEGACY_WRITE_U16_LE(crack_info + 2, 1);
	LEGACY_WRITE_U16_LE(crack_info + 4, 2);
	set_line(0, 20, 40, 100, 160);
	set_line(1, 140, 40, 200, 100);
}

static void test_incremental_crack_overlay(void)
{
	struct RECTANGLE *bounds;

	reset_overlay(1);
	bounds = init_crak(0, 10, 100);
	assert(drawn_lines == 3);
	assert(last_line[DRAW_LINE_COLOR_INDEX] == 7);
	assert(last_line[DRAW_LINE_START_Y_INDEX] == 30);
	assert(last_line[DRAW_LINE_END_Y_INDEX] == 90);
	assert(bounds->left == 20 && bounds->right == 101);
	assert(bounds->top == 29 && bounds->bottom == 92);
	/* SEG003 init_crak -> SEG012 preRender_line places the first four line
	 * words over player headings; the later bounds calls save SI/DI over the
	 * last two opponent words. These replace the preceding scene's residue. */
	assert_words(player, 0, 20, 0, 30);
	assert_words(opponent, 0, 0, 0, 11);

	/* A later animation frame retains the last line's index and coordinates.
	 * Clamp to the final frame and vary the normalized resource offset. */
	crack_offset = 3;
	init_crak(300, 10, 100);
	assert(drawn_lines == 9);
	assert_words(player, 0, 140, 0, 30);
	assert_words(opponent, 0, 0, 1, 3);
}

static void test_rejected_crack_lines(void)
{
	reset_overlay(1);
	set_line(0, 20, -10, 100, -10);
	init_crak(0, 0, 200);
	assert(drawn_lines == 0);
	assert_words(player, 0, 20, 0, 0);
	assert_words(opponent, 0, 0, 0, 11);

	reset_overlay(1);
	set_line(0, -20, 10, -10, 20);
	init_crak(0, 0, 200);
	assert(drawn_lines == 0);
	assert(opponent[0] == 0 && opponent[1] == 11);
	assert(opponent[2] == 0 && opponent[3] == 11);
}

static void test_direct_redraw_stack(void)
{
	reset_overlay(0);
	init_crak(0, 10, 100);
	assert(drawn_lines == 3);
	/* Without pending rect_union arguments, the line buffer is four bytes
	 * higher. A drawn line leaves SEG012's return CS and the buffer address. */
	assert_words(player, 0x49cc, (legacy_s16)0x8fb0, 0, 20);
	assert_words(opponent, 7, DRAW_LINE_MODE_X_MAJOR_RIGHT, 0, 0);

	reset_overlay(0);
	set_line(0, 20, -10, 100, -10);
	init_crak(0, 0, 200);
	assert(drawn_lines == 0);
	/* A rejected line never makes the pixel call: its setup argument stays. */
	assert_words(player, -10, (legacy_s16)0x8fb0, 0, 20);
	assert_words(opponent, 7,
				 DRAW_LINE_MODE_HORIZONTAL | (DRAW_LINE_CLIP_TOP << DRAW_LINE_CLIP_SHIFT), 0, 0);
}

static void test_explicit_crack_context(void)
{
	reset_overlay(1);
	preRender_line(20, 30, 100, 90, 7);
	assert_words(player, 100, 101, 102, 103);
	assert_words(opponent, 200, 201, 202, 203);

	shape3d_set_legacy_render_stack(0, 0, 0, 0);
	init_crak(0, 10, 100);
	assert(drawn_lines == 4);
	assert_words(player, 100, 101, 102, 103);
	assert_words(opponent, 200, 201, 202, 203);
}

int main(void)
{
	test_incremental_crack_overlay();
	test_rejected_crack_lines();
	test_direct_redraw_stack();
	test_explicit_crack_context();
	return 0;
}
