#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../c/externs.h"
#include "../c/shape2d.h"
#include "../c/shape2d_internal.h"
#include "../c/shape3d.h"
#include "../c/shape3d_internal.h"

static legacy_u32 callback_hash;
static unsigned span_calls, line_calls, point_calls;
static legacy_u16 last_top, last_count;

static void hash_word(legacy_u16 value)
{
	callback_hash = (callback_hash ^ value) * 16777619UL;
}

static void record_spans(legacy_s16 *left, legacy_s16 *right, legacy_u16 top, legacy_u16 count,
						 legacy_u16 color, legacy_u16 kind)
{
	unsigned i;

	assert(top < 480 && count <= 480 - top);
	span_calls++;
	last_top = top;
	last_count = count;
	hash_word(kind);
	hash_word(top);
	hash_word(count);
	hash_word(color);
	for (i = 0; i < count; i++) {
		hash_word((legacy_u16)left[i]);
		hash_word((legacy_u16)right[i]);
	}
}

void draw_filled_lines(legacy_s16 *left, legacy_s16 *right, legacy_u16 top, legacy_u16 count,
					   legacy_u16 color)
{
	record_spans(left, right, top, count, color, 1);
}

void draw_two_color_lines(legacy_s16 *left, legacy_s16 *right, legacy_u16 top, legacy_u16 count,
						  legacy_u16 color)
{
	hash_word(raster_fill_pattern);
	hash_word(raster_alternate_color);
	record_spans(left, right, top, count, color, 2);
}

void draw_patterned_lines(legacy_s16 *left, legacy_s16 *right, legacy_u16 top, legacy_u16 count,
						  legacy_u16 color)
{
	hash_word(raster_fill_pattern);
	record_spans(left, right, top, count, color, 3);
}

void sprite_draw_line_from_setup(const legacy_u16 *line)
{
	legacy_u16 mode = line[DRAW_LINE_MODE_AND_CLIP_INDEX] & DRAW_LINE_MODE_MASK;

	line_calls++;
	hash_word(4);
	hash_word(mode);
	hash_word(line[DRAW_LINE_START_X_INDEX]);
	hash_word(line[DRAW_LINE_START_Y_INDEX]);
	hash_word(line[DRAW_LINE_END_X_INDEX]);
	hash_word(line[DRAW_LINE_END_Y_INDEX]);
	hash_word(line[DRAW_LINE_PIXEL_COUNT_INDEX]);
	hash_word(line[DRAW_LINE_COLOR_INDEX]);
	/* Integer modes deliberately leave unused fractional setup words unchanged. */
	if (mode >= DRAW_LINE_MODE_Y_MAJOR_LEFT && mode <= DRAW_LINE_MODE_X_MAJOR_RIGHT) {
		hash_word(line[DRAW_LINE_STEP_INDEX]);
		hash_word(line[DRAW_LINE_START_X_FRACTION_INDEX]);
		hash_word(line[DRAW_LINE_START_Y_FRACTION_INDEX]);
	}
}

void sprite_putpixel_clipped(legacy_s16 x, legacy_s16 y, legacy_s16 color)
{
	point_calls++;
	hash_word(5);
	hash_word((legacy_u16)x);
	hash_word((legacy_u16)y);
	hash_word((legacy_u16)color);
}

static void reset_raster(void)
{
	drawing_sprite.sprite_raster_left = 10;
	drawing_sprite.sprite_raster_right = 310;
	drawing_sprite.sprite_top = 20;
	drawing_sprite.sprite_bottom = 180;
	callback_hash = 2166136261UL;
	span_calls = line_calls = point_calls = 0;
}

static void test_raster_boundaries(void)
{
	struct POINT2D rectangle[4] = {{10, 20}, {20, 20}, {20, 30}, {10, 30}};
	struct POINT2D vertical[2] = {{309, 20}, {309, 30}};

	reset_raster();
	preRender_default(37, 0, rectangle);
	assert(span_calls == 0 && line_calls == 0);
	preRender_default(37, 1, rectangle);
	assert(span_calls == 0 && line_calls == 1);
	preRender_default(37, 4, rectangle);
	assert(span_calls == 1 && last_top == 20 && last_count == 11);
	/* The original polygon rejection excludes a vertical edge at right - 1. */
	preRender_default(37, 2, vertical);
	assert(span_calls == 1 && line_calls == 1);
	vertical[0].px = vertical[1].px = 308;
	preRender_default(37, 2, vertical);
	assert(line_calls == 2);
	preRender_sphere(160, 100, 0, 3);
	assert(point_calls == 0);
	preRender_sphere(160, 100, 1, 3);
	assert(point_calls == 1);
}

static legacy_u32 polygon_fingerprint(void)
{
	static const legacy_s16 xs[] = {-50, 9, 10, 11, 160, 308, 309, 310, 350};
	static const legacy_s16 ys[] = {-50, 19, 20, 21, 100, 178, 179, 180, 230};
	struct POINT2D vertices[4], ordered[4];
	unsigned x, y, shape, reverse, start, i;
	legacy_u16 count;

	reset_raster();
	for (x = 0; x < sizeof(xs) / sizeof(xs[0]); x++) {
		for (y = 0; y < sizeof(ys) / sizeof(ys[0]); y++) {
			for (shape = 0; shape < 4; shape++) {
				vertices[0].px = xs[x];
				vertices[0].py = ys[y];
				vertices[1].px = xs[x] + 30;
				vertices[1].py = ys[y] + (shape == 1 ? 7 : 0);
				vertices[2].px = xs[x] + (shape == 2 ? 60 : 30);
				vertices[2].py = ys[y] + 40;
				vertices[3].px = xs[x];
				vertices[3].py = ys[y] + 40;
				count = shape == 3 ? 3 : 4;
				for (reverse = 0; reverse < 2; reverse++) {
					for (start = 0; start < count; start++) {
						for (i = 0; i < count; i++) {
							ordered[i] = vertices[(start + (reverse ? count - i : i)) % count];
						}
						preRender_default(37, count, ordered);
						preRender_default_alt(43, count, ordered);
						preRender_two_color(0x5a, 7, 9, count, ordered);
						preRender_patterned(0xa5, 11, count, ordered);
					}
				}
			}
		}
	}
	return callback_hash;
}

static legacy_u32 sphere_fingerprint(void)
{
	static const legacy_s16 xs[] = {-100, 9, 10, 11, 160, 308, 309, 310, 400};
	static const legacy_s16 ys[] = {-100, 19, 20, 21, 100, 178, 179, 180, 300};
	unsigned x, y, size;

	reset_raster();
	for (x = 0; x < sizeof(xs) / sizeof(xs[0]); x++) {
		for (y = 0; y < sizeof(ys) / sizeof(ys[0]); y++) {
			for (size = 0; size <= 160; size++) {
				preRender_sphere(xs[x], ys[y], size, 37);
			}
		}
	}
	return callback_hash;
}

static legacy_u32 perimeter_fingerprint(void)
{
	legacy_u16 source[6], destination[64];
	legacy_u32 seed = 314159UL;
	unsigned sample, i;

	callback_hash = 2166136261UL;
	for (sample = 0; sample < 8192; sample++) {
		for (i = 0; i < 6; i++) {
			seed = seed * 1664525UL + 1013904223UL;
			source[i] = (legacy_u16)(seed >> 16);
		}
		sphere_build_perimeter(source, destination);
		for (i = 0; i < 64; i++) {
			hash_word(destination[i]);
		}
	}
	return callback_hash;
}

int main(void)
{
	legacy_u32 polygon_hash, sphere_hash, perimeter_hash;

	test_raster_boundaries();
	polygon_hash = polygon_fingerprint();
	sphere_hash = sphere_fingerprint();
	perimeter_hash = perimeter_fingerprint();
#ifdef PRERENDER_RECORD_BASELINE
	fprintf(stdout, "%08lx %08lx %08lx\n", (unsigned long)polygon_hash, (unsigned long)sphere_hash,
			(unsigned long)perimeter_hash);
#else
	/* Captured from the original implementation with both -O0 and -O2. */
	assert(polygon_hash == 0x1322835bUL);
	assert(sphere_hash == 0x63a0e6c4UL);
	assert(perimeter_hash == 0x28c59b4bUL);
#endif
	return 0;
}
