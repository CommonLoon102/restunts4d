#include <assert.h>
#include <string.h>

#include "../c/externs.h"
#include "../c/shape2d.h"
#include "../c/shape2d_internal.h"
#include "../c/shape3d_internal.h"

static void reset_clip(void)
{
	drawing_sprite.sprite_top = 20;
	drawing_sprite.sprite_bottom = 180;
	drawing_sprite.sprite_raster_left = 10;
	drawing_sprite.sprite_raster_right = 310;
}

static void test_line_modes(void)
{
	static const struct {
		legacy_s16 dx, dy;
		legacy_u16 mode, count;
	} cases[] = {{-10, 0, DRAW_LINE_MODE_HORIZONTAL_REVERSED, 11},
				 {10, 0, DRAW_LINE_MODE_HORIZONTAL, 11},
				 {0, 0, DRAW_LINE_MODE_POINT, 1},
				 {0, 10, DRAW_LINE_MODE_VERTICAL, 11},
				 {-10, 10, DRAW_LINE_MODE_DIAGONAL_LEFT, 11},
				 {10, 10, DRAW_LINE_MODE_DIAGONAL_RIGHT, 11},
				 {-3, 10, DRAW_LINE_MODE_Y_MAJOR_LEFT, 11},
				 {3, 10, DRAW_LINE_MODE_Y_MAJOR_RIGHT, 11},
				 {-10, 3, DRAW_LINE_MODE_X_MAJOR_LEFT, 11},
				 {10, 3, DRAW_LINE_MODE_X_MAJOR_RIGHT, 11}};
	legacy_u16 line[DRAW_LINE_WORD_COUNT];
	unsigned i;

	reset_clip();
	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		memset(line, 0, sizeof(line));
		line[DRAW_LINE_COLOR_INDEX] = 37;
		assert(line_prepare_clipped(100, 100, 100 + cases[i].dx, 100 + cases[i].dy, line) == 0);
		assert(line[DRAW_LINE_MODE_AND_CLIP_INDEX] == cases[i].mode);
		assert(line[DRAW_LINE_PIXEL_COUNT_INDEX] == cases[i].count);
		assert(line[DRAW_LINE_COLOR_INDEX] == 37);
		if (cases[i].mode >= DRAW_LINE_MODE_Y_MAJOR_LEFT &&
			cases[i].mode <= DRAW_LINE_MODE_X_MAJOR_RIGHT) {
			assert(line[DRAW_LINE_STEP_INDEX] == 19660);
		}
	}
}

static void test_clipped_endpoints(void)
{
	legacy_u16 line[DRAW_LINE_WORD_COUNT] = {0};

	reset_clip();
	assert(line_prepare_clipped(100, 190, 100, 10, line) == 0);
	assert(line[DRAW_LINE_START_Y_INDEX] == 20);
	assert(line[DRAW_LINE_END_Y_INDEX] == 179);
	assert(line[DRAW_LINE_PIXEL_COUNT_INDEX] == 160);
	assert(line_prepare_clipped(0, 100, 319, 100, line) == 0);
	assert(line[DRAW_LINE_START_X_INDEX] == 10);
	assert(line[DRAW_LINE_END_X_INDEX] == 309);
	assert(line[DRAW_LINE_PIXEL_COUNT_INDEX] == 300);
	assert(line_prepare_clipped(400, 30, 500, 170, line) == DRAW_LINE_CLIP_RIGHT);
	assert(line[DRAW_LINE_PIXEL_COUNT_INDEX] == 0);
	assert(line_prepare_unclipped(100, 190, 100, 10, line) == 0);
	assert(line[DRAW_LINE_START_Y_INDEX] == 10);
	assert(line[DRAW_LINE_END_Y_INDEX] == 190);
	assert(line[DRAW_LINE_PIXEL_COUNT_INDEX] == 181);
}

static legacy_u16 random_word(legacy_u32 *seed)
{
	*seed = *seed * 1664525UL + 1013904223UL;
	return (legacy_u16)(*seed >> 16);
}

static legacy_u32 line_fingerprint(legacy_u16 unclipped, legacy_u16 wide_coordinates)
{
	static const legacy_u16 boundaries[] = {0,	   1,	  9,	 10,	19,	  20,	 179,
											180,   309,	  310,	 319,	320,  15999, 16000,
											32767, 32768, 49536, 49537, 65535};
	legacy_u16 line[DRAW_LINE_WORD_COUNT];
	legacy_u16 coordinates[4];
	legacy_u16 result;
	legacy_u32 seed = 271828UL;
	legacy_u32 hash = 2166136261UL;
	unsigned iteration, i;

	reset_clip();
	for (iteration = 0; iteration < 50000; iteration++) {
		for (i = 0; i < 4; i++) {
			coordinates[i] = random_word(&seed);
			if (wide_coordinates == 0) {
				coordinates[i] = (legacy_u16)(coordinates[i] % 1000 - 300);
			} else if (iteration % 2 == 0) {
				coordinates[i] =
					boundaries[coordinates[i] % (sizeof(boundaries) / sizeof(boundaries[0]))];
			}
		}
		/* Unclipped subdivision assumes the x range intersects the viewport. */
		if (unclipped != 0 && wide_coordinates != 0) {
			coordinates[0] %= 320;
			coordinates[2] %= 320;
		}
		for (i = 0; i < DRAW_LINE_WORD_COUNT; i++) {
			line[i] = random_word(&seed);
		}
		if (unclipped != 0) {
			result = line_prepare_unclipped(coordinates[0], coordinates[1], coordinates[2],
											coordinates[3], line);
		} else {
			result = line_prepare_clipped(coordinates[0], coordinates[1], coordinates[2],
										  coordinates[3], line);
		}
		hash = (hash ^ result) * 16777619UL;
		for (i = 0; i < DRAW_LINE_WORD_COUNT; i++) {
			hash = (hash ^ line[i]) * 16777619UL;
		}
	}
	return hash;
}

int main(void)
{
	/* Captured before refactoring: all setup words, including words retained
	 * on rejection, contribute to each deterministic baseline fingerprint. */
	static const legacy_u32 expected[] = {2459404900UL, 19184828UL, 2708284857UL, 1954427655UL};
	unsigned i;

	test_line_modes();
	test_clipped_endpoints();
	for (i = 0; i < 4; i++) {
		assert(line_fingerprint(i & 1U, i >> 1U) == expected[i]);
	}
	return 0;
}
