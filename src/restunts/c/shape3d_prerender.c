#include "externs.h"
#include "legacy.h"
#include "shape2d.h"
#include "shape2d_internal.h"
#include "shape3d.h"
#include "shape3d_internal.h"

#define PRERENDER_POINT_WORD_COUNT 2U
#define PRERENDER_POINT_Y_OFFSET 1U
#define SPHERE_RASTER_TABLE_LIMIT 40U
#define SPHERE_PERIMETER_POINT_COUNT 16U
#define SPHERE_VERTEX_COUNT 32U
#define SPHERE_VERTEX_WORD_COUNT 64U
#define SPHERE_MIRROR_WORD_OFFSET 32U
#define SPHERE_DIAGONAL_NORMALIZATION 11585U
#define SPHERE_HALF_STEP_NORMALIZATION 14654U
#define SPHERE_QUARTER_STEP_NORMALIZATION 15895U
#define SPHERE_THREE_QUARTER_STEP_NORMALIZATION 13107U
#define WHEEL_SOURCE_POINT_COUNT 4U
#define WHEEL_SOURCE_WORD_COUNT 8U
#define WHEEL_PERIMETER_POINT_COUNT 16U
#define WHEEL_PERIMETER_INDEX_MASK 15U
#define WHEEL_INNER_WORD_OFFSET 32U
#define WHEEL_DEPTH_WORD_OFFSET 64U
#define WHEEL_POINT_WORD_COUNT 96U
#define WHEEL_SIDE_HALF_POINT_COUNT 9U
#define WHEEL_SIDE_LAST_POINT_INDEX 17U
#define WHEEL_SIDE_VERTEX_COUNT 18U
#define WHEEL_SIDE_WORD_COUNT 36U
#define WHEEL_QUAD_POINT_COUNT 4U
#define WHEEL_QUAD_WORD_COUNT 8U
#define PRERENDER_EDGE_ROW_CAPACITY 480
#define PRERENDER_EDGE_TABLE_COUNT 2U
#define PRERENDER_LINE_BUFFER_COUNT 2U
#define PRERENDER_EDGE_CLIPPED_MODE 0
#define PRERENDER_EDGE_UNCLIPPED_MODE 1
#define PRERENDER_FIXED_CARRY_LIMIT ((legacy_u32)LEGACY_U16_MAX)

static void preRender_default_impl(legacy_u16 arg_color,
	legacy_u16 arg_vertlinecount, const struct POINT2D* arg_vertlines,
	legacy_u16 mode);
void generate_poly_edges(legacy_s16* edges, const legacy_u16* line,
	legacy_s16 mode);
void preRender_default_impl_helper(const legacy_u16* line,
	legacy_u16 color, legacy_u16 mode, legacy_s16* edges);

void preRender_line(
	legacy_u16 start_x,
	legacy_u16 start_y,
	legacy_u16 end_x,
	legacy_u16 end_y,
	legacy_u16 color
) {
	legacy_u16 line[DRAW_LINE_WORD_COUNT];

	line[DRAW_LINE_COLOR_INDEX] = color;
	if (draw_line_related(start_x, start_y, end_x, end_y, line) == 0 &&
			LEGACY_S16_FROM_BITS(line[DRAW_LINE_PIXEL_COUNT_INDEX]) > 0)
		putpixel_line1_maybe(line);
}

void draw_beveled_border(legacy_s16 x, legacy_s16 y,
	legacy_s16 width, legacy_s16 height,
	legacy_s16 top_outer_color, legacy_s16 top_inner_color,
	legacy_s16 bottom_outer_color, legacy_s16 bottom_inner_color)
{
	legacy_s16 right = x + width;
	legacy_s16 bottom = y + height;

	preRender_line(x, y, right, y, top_outer_color);
	preRender_line(x + 1, y + 1, right - 1, y + 1, top_outer_color);
	preRender_line(x + 2, y + 2, right - 2, y + 2, top_inner_color);

	preRender_line(x, y, x, bottom, top_outer_color);
	preRender_line(x + 1, y + 1, x + 1, bottom - 1, top_outer_color);
	preRender_line(x + 2, y + 2, x + 2, bottom - 2, top_inner_color);

	preRender_line(x, bottom, right, bottom, bottom_outer_color);
	preRender_line(x + 1, bottom - 1, right - 1, bottom - 1,
		bottom_outer_color);
	preRender_line(x + 2, bottom - 2, right - 2, bottom - 2,
		bottom_inner_color);

	preRender_line(right, y, right, bottom, bottom_outer_color);
	preRender_line(right - 1, y + 1, right - 1, bottom - 1,
		bottom_outer_color);
	preRender_line(right - 2, y + 2, right - 2, bottom - 2,
		bottom_inner_color);
}

void draw_lines_unk(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height,
	legacy_s16 outer_color, legacy_s16 inner_color, legacy_s16 opposite_color)
{
	draw_beveled_border(x, y, width, height,
		outer_color, inner_color, opposite_color, inner_color);
}

void preRender_default_alt(legacy_u16 arg_color,
	legacy_u16 arg_vertlinecount, const struct POINT2D* arg_vertlines) {
	//return ported_preRender_default_alt_(arg_color, arg_vertlinecount, arg_vertlines);

	spritefunc = &draw_filled_lines;
	imagefunc = &preRender_line;
	preRender_default_impl(arg_color, arg_vertlinecount, arg_vertlines, 0);
}

void preRender_default(legacy_u16 arg_color,
	legacy_u16 arg_vertlinecount, const struct POINT2D* arg_vertlines) {
	//return ported_preRender_default_(arg_color, arg_vertlinecount, arg_vertlines);

	spritefunc = &draw_filled_lines;
	imagefunc = &preRender_line;
	preRender_default_impl(arg_color, arg_vertlinecount, arg_vertlines, 1);
}

static void preRender_default_alt_words(legacy_u16 color,
	legacy_u16 vertex_count, const legacy_u16* words)
{
	struct POINT2D vertices[SPHERE_VERTEX_COUNT];
	legacy_u16 index;

	for (index = 0; index < vertex_count; index++) {
		vertices[index].px = LEGACY_S16_FROM_BITS(
			words[index * PRERENDER_POINT_WORD_COUNT]);
		vertices[index].py = LEGACY_S16_FROM_BITS(
			words[index * PRERENDER_POINT_WORD_COUNT +
				PRERENDER_POINT_Y_OFFSET]);
	}
	preRender_default_alt(color, vertex_count, vertices);
}

static legacy_u16 sphere_scale_sum(legacy_u16 left, legacy_u16 right,
	legacy_u16 scale)
{
	return (legacy_u16)multiply_and_scale(
		LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(left, right)),
		LEGACY_S16_FROM_BITS(scale));
}

static legacy_u16 sphere_scale_difference(legacy_u16 left,
	legacy_u16 right, legacy_u16 scale)
{
	return (legacy_u16)multiply_and_scale(
		LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB(left, right)),
		LEGACY_S16_FROM_BITS(scale));
}

void preRender_sphere_helper2(legacy_u16* source, legacy_u16* destination)
{
	legacy_u16 half_x1;
	legacy_u16 quarter_x1;
	legacy_u16 three_quarters_x1;
	legacy_u16 half_y1;
	legacy_u16 quarter_y1;
	legacy_u16 three_quarters_y1;
	legacy_u16 half_x2;
	legacy_u16 quarter_x2;
	legacy_u16 three_quarters_x2;
	legacy_u16 half_y2;
	legacy_u16 quarter_y2;
	legacy_u16 three_quarters_y2;
	legacy_u16 negative_x1;
	legacy_u16 negative_y1;
	legacy_u16 index;
	legacy_u16 center_x;
	legacy_u16 center_y;

	destination[0] = LEGACY_U16_WRAP_SUB(source[2], source[0]);
	destination[1] = LEGACY_U16_WRAP_SUB(source[3], source[1]);
	destination[16] = LEGACY_U16_WRAP_SUB(source[4], source[0]);
	destination[17] = LEGACY_U16_WRAP_SUB(source[5], source[1]);
	half_x1 = sar1_word((legacy_u16)destination[0]);
	quarter_x1 = sar1_word(half_x1);
	three_quarters_x1 = LEGACY_U16_WRAP_ADD(half_x1, quarter_x1);
	half_y1 = sar1_word((legacy_u16)destination[1]);
	quarter_y1 = sar1_word(half_y1);
	three_quarters_y1 = LEGACY_U16_WRAP_ADD(half_y1, quarter_y1);
	half_x2 = sar1_word((legacy_u16)destination[16]);
	quarter_x2 = sar1_word(half_x2);
	three_quarters_x2 = LEGACY_U16_WRAP_ADD(half_x2, quarter_x2);
	half_y2 = sar1_word((legacy_u16)destination[17]);
	quarter_y2 = sar1_word(half_y2);
	three_quarters_y2 = LEGACY_U16_WRAP_ADD(half_y2, quarter_y2);

	destination[8] = sphere_scale_sum(destination[16], destination[0],
		SPHERE_DIAGONAL_NORMALIZATION);
	destination[9] = sphere_scale_sum(destination[1], destination[17],
		SPHERE_DIAGONAL_NORMALIZATION);
	destination[4] = sphere_scale_sum(destination[0], half_x2,
		SPHERE_HALF_STEP_NORMALIZATION);
	destination[5] = sphere_scale_sum(destination[1], half_y2,
		SPHERE_HALF_STEP_NORMALIZATION);
	destination[12] = sphere_scale_sum(destination[16], half_x1,
		SPHERE_HALF_STEP_NORMALIZATION);
	destination[13] = sphere_scale_sum(destination[17], half_y1,
		SPHERE_HALF_STEP_NORMALIZATION);
	destination[2] = sphere_scale_sum(destination[0], quarter_x2,
		SPHERE_QUARTER_STEP_NORMALIZATION);
	destination[3] = sphere_scale_sum(destination[1], quarter_y2,
		SPHERE_QUARTER_STEP_NORMALIZATION);
	destination[14] = sphere_scale_sum(destination[16], quarter_x1,
		SPHERE_QUARTER_STEP_NORMALIZATION);
	destination[15] = sphere_scale_sum(destination[17], quarter_y1,
		SPHERE_QUARTER_STEP_NORMALIZATION);
	destination[6] = sphere_scale_sum(destination[0], three_quarters_x2,
		SPHERE_THREE_QUARTER_STEP_NORMALIZATION);
	destination[7] = sphere_scale_sum(destination[1], three_quarters_y2,
		SPHERE_THREE_QUARTER_STEP_NORMALIZATION);
	destination[10] = sphere_scale_sum(destination[16], three_quarters_x1,
		SPHERE_THREE_QUARTER_STEP_NORMALIZATION);
	destination[11] = sphere_scale_sum(destination[17], three_quarters_y1,
		SPHERE_THREE_QUARTER_STEP_NORMALIZATION);

	negative_x1 = LEGACY_U16_WRAP_SUB(0, destination[0]);
	negative_y1 = LEGACY_U16_WRAP_SUB(0, destination[1]);
	destination[24] = sphere_scale_sum(destination[16], negative_x1,
		SPHERE_DIAGONAL_NORMALIZATION);
	destination[25] = sphere_scale_sum(destination[17], negative_y1,
		SPHERE_DIAGONAL_NORMALIZATION);
	destination[28] = sphere_scale_sum(half_x2, negative_x1,
		SPHERE_HALF_STEP_NORMALIZATION);
	destination[29] = sphere_scale_sum(half_y2, negative_y1,
		SPHERE_HALF_STEP_NORMALIZATION);
	destination[20] = sphere_scale_difference(destination[16], half_x1,
		SPHERE_HALF_STEP_NORMALIZATION);
	destination[21] = sphere_scale_difference(destination[17], half_y1,
		SPHERE_HALF_STEP_NORMALIZATION);
	destination[30] = sphere_scale_sum(quarter_x2, negative_x1,
		SPHERE_QUARTER_STEP_NORMALIZATION);
	destination[31] = sphere_scale_sum(quarter_y2, negative_y1,
		SPHERE_QUARTER_STEP_NORMALIZATION);
	destination[18] = sphere_scale_difference(destination[16], quarter_x1,
		SPHERE_QUARTER_STEP_NORMALIZATION);
	destination[19] = sphere_scale_difference(destination[17], quarter_y1,
		SPHERE_QUARTER_STEP_NORMALIZATION);
	destination[26] = sphere_scale_sum(three_quarters_x2, negative_x1,
		SPHERE_THREE_QUARTER_STEP_NORMALIZATION);
	destination[27] = sphere_scale_sum(three_quarters_y2, negative_y1,
		SPHERE_THREE_QUARTER_STEP_NORMALIZATION);
	destination[22] = sphere_scale_difference(destination[16],
		three_quarters_x1, SPHERE_THREE_QUARTER_STEP_NORMALIZATION);
	destination[23] = sphere_scale_difference(destination[17],
		three_quarters_y1, SPHERE_THREE_QUARTER_STEP_NORMALIZATION);

	center_x = (legacy_u16)source[0];
	center_y = (legacy_u16)source[1];
	for (index = 0; index < SPHERE_PERIMETER_POINT_COUNT; index++) {
		destination[SPHERE_MIRROR_WORD_OFFSET +
			index * PRERENDER_POINT_WORD_COUNT] = LEGACY_U16_WRAP_SUB(
			center_x, destination[index * PRERENDER_POINT_WORD_COUNT]);
		destination[SPHERE_MIRROR_WORD_OFFSET +
			index * PRERENDER_POINT_WORD_COUNT + PRERENDER_POINT_Y_OFFSET] =
			LEGACY_U16_WRAP_SUB(center_y,
				destination[index * PRERENDER_POINT_WORD_COUNT +
					PRERENDER_POINT_Y_OFFSET]);
		destination[index * PRERENDER_POINT_WORD_COUNT] =
			LEGACY_U16_WRAP_ADD(
				destination[index * PRERENDER_POINT_WORD_COUNT], center_x);
		destination[index * PRERENDER_POINT_WORD_COUNT +
			PRERENDER_POINT_Y_OFFSET] = LEGACY_U16_WRAP_ADD(
				destination[index * PRERENDER_POINT_WORD_COUNT +
					PRERENDER_POINT_Y_OFFSET], center_y);
	}
}

void preRender_sphere_helper(legacy_u16* source, legacy_u16 color)
{
	legacy_u16 vertices[SPHERE_VERTEX_WORD_COUNT];

	preRender_sphere_helper2(source, vertices);
	preRender_default_alt_words(color, SPHERE_VERTEX_COUNT, vertices);
}

void preRender_wheel_helper3(legacy_u16* source, legacy_u16* destination)
{
	legacy_u16 half_x1;
	legacy_u16 half_y1;
	legacy_u16 half_x2;
	legacy_u16 half_y2;
	legacy_u16 index;
	legacy_u16 center_x;
	legacy_u16 center_y;

	destination[0] = LEGACY_U16_WRAP_SUB(source[2], source[0]);
	destination[1] = LEGACY_U16_WRAP_SUB(source[3], source[1]);
	destination[8] = LEGACY_U16_WRAP_SUB(source[4], source[0]);
	destination[9] = LEGACY_U16_WRAP_SUB(source[5], source[1]);
	destination[4] = sphere_scale_sum(destination[8], destination[0],
		SPHERE_DIAGONAL_NORMALIZATION);
	destination[5] = sphere_scale_sum(destination[1], destination[9],
		SPHERE_DIAGONAL_NORMALIZATION);
	half_x2 = sar1_word((legacy_u16)destination[8]);
	half_y2 = sar1_word((legacy_u16)destination[9]);
	destination[2] = sphere_scale_sum(destination[0], half_x2,
		SPHERE_HALF_STEP_NORMALIZATION);
	destination[3] = sphere_scale_sum(destination[1], half_y2,
		SPHERE_HALF_STEP_NORMALIZATION);
	half_x1 = sar1_word((legacy_u16)destination[0]);
	half_y1 = sar1_word((legacy_u16)destination[1]);
	destination[6] = sphere_scale_sum(destination[8], half_x1,
		SPHERE_HALF_STEP_NORMALIZATION);
	destination[7] = sphere_scale_sum(destination[9], half_y1,
		SPHERE_HALF_STEP_NORMALIZATION);
	destination[12] = sphere_scale_difference(destination[8],
		destination[0], SPHERE_DIAGONAL_NORMALIZATION);
	destination[13] = sphere_scale_difference(destination[9],
		destination[1], SPHERE_DIAGONAL_NORMALIZATION);
	destination[14] = sphere_scale_difference(half_x2,
		destination[0], SPHERE_HALF_STEP_NORMALIZATION);
	destination[15] = sphere_scale_difference(half_y2,
		destination[1], SPHERE_HALF_STEP_NORMALIZATION);
	destination[10] = sphere_scale_difference(destination[8], half_x1,
		SPHERE_HALF_STEP_NORMALIZATION);
	destination[11] = sphere_scale_difference(destination[9], half_y1,
		SPHERE_HALF_STEP_NORMALIZATION);

	center_x = (legacy_u16)source[0];
	center_y = (legacy_u16)source[1];
	for (index = 0; index < 8U; index++) {
		destination[16U + index * 2U] = LEGACY_U16_WRAP_SUB(
			center_x, destination[index * 2U]);
		destination[17U + index * 2U] = LEGACY_U16_WRAP_SUB(
			center_y, destination[index * 2U + 1U]);
		destination[index * 2U] = LEGACY_U16_WRAP_ADD(
			destination[index * 2U], center_x);
		destination[index * 2U + 1U] = LEGACY_U16_WRAP_ADD(
			destination[index * 2U + 1U], center_y);
	}
}

static legacy_u16 wheel_interpolate(legacy_u16 center,
	legacy_u16 target, legacy_u16 scale)
{
	return LEGACY_U16_WRAP_ADD(center, (legacy_u16)multiply_and_scale(
		LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB(target, center)),
		LEGACY_S16_FROM_BITS(scale)));
}

void preRender_wheel_helper2(legacy_u16* source, legacy_u16* destination,
	legacy_u16 scale)
{
	legacy_u16 inner_source[6];
	legacy_u16 center_x;
	legacy_u16 center_y;
	legacy_u16 scale_bits;

	center_x = (legacy_u16)source[0];
	center_y = (legacy_u16)source[1];
	scale_bits = (legacy_u16)scale;
	inner_source[0] = center_x;
	inner_source[1] = center_y;
	inner_source[2] = wheel_interpolate(center_x,
		(legacy_u16)source[2], scale_bits);
	inner_source[3] = wheel_interpolate(center_y,
		(legacy_u16)source[3], scale_bits);
	inner_source[4] = wheel_interpolate(center_x,
		(legacy_u16)source[4], scale_bits);
	inner_source[5] = wheel_interpolate(center_y,
		(legacy_u16)source[5], scale_bits);
	preRender_wheel_helper3(source, destination);
	preRender_wheel_helper3(inner_source,
		destination + WHEEL_INNER_WORD_OFFSET);
}

void preRender_wheel_helper(legacy_u16* source, legacy_u16* destination,
	legacy_u16 scale)
{
	legacy_u16 offset_x;
	legacy_u16 offset_y;
	legacy_u16 index;

	preRender_wheel_helper2(source, destination, scale);
	offset_x = LEGACY_U16_WRAP_SUB(source[6], source[0]);
	offset_y = LEGACY_U16_WRAP_SUB(source[7], source[1]);
	for (index = 0; index < WHEEL_PERIMETER_POINT_COUNT; index++) {
		destination[WHEEL_DEPTH_WORD_OFFSET +
			index * PRERENDER_POINT_WORD_COUNT] = LEGACY_U16_WRAP_ADD(
			destination[index * PRERENDER_POINT_WORD_COUNT], offset_x);
		destination[WHEEL_DEPTH_WORD_OFFSET +
			index * PRERENDER_POINT_WORD_COUNT + PRERENDER_POINT_Y_OFFSET] =
			LEGACY_U16_WRAP_ADD(
				destination[index * PRERENDER_POINT_WORD_COUNT +
					PRERENDER_POINT_Y_OFFSET], offset_y);
	}
}

static void preRender_wheel_side(const legacy_u16* wheel_points,
	legacy_u16 minimum_index, legacy_u16 reverse, legacy_u16 color)
{
	legacy_u16 side[WHEEL_SIDE_WORD_COUNT];
	legacy_u16 step;
	legacy_u16 point_index;
	legacy_u16 destination_index;

	for (step = 0; step < WHEEL_SIDE_HALF_POINT_COUNT; step++) {
		if (reverse == 0U)
			point_index = (legacy_u16)((minimum_index + step) &
				WHEEL_PERIMETER_INDEX_MASK);
		else
			point_index = (legacy_u16)((minimum_index +
				WHEEL_PERIMETER_POINT_COUNT - step) &
				WHEEL_PERIMETER_INDEX_MASK);
		destination_index = (legacy_u16)(
			step * PRERENDER_POINT_WORD_COUNT);
		side[destination_index] = wheel_points[
			point_index * PRERENDER_POINT_WORD_COUNT];
		side[destination_index + PRERENDER_POINT_Y_OFFSET] =
			wheel_points[point_index * PRERENDER_POINT_WORD_COUNT +
				PRERENDER_POINT_Y_OFFSET];
		destination_index = (legacy_u16)((WHEEL_SIDE_LAST_POINT_INDEX -
			step) * PRERENDER_POINT_WORD_COUNT);
		side[destination_index] = wheel_points[WHEEL_INNER_WORD_OFFSET +
			point_index * PRERENDER_POINT_WORD_COUNT];
		side[destination_index + PRERENDER_POINT_Y_OFFSET] =
			wheel_points[WHEEL_INNER_WORD_OFFSET +
				point_index * PRERENDER_POINT_WORD_COUNT +
				PRERENDER_POINT_Y_OFFSET];
	}
	preRender_default_alt_words(color, WHEEL_SIDE_VERTEX_COUNT, side);
}

void preRender_wheel(const struct POINT2D* source, legacy_u16 scale,
	legacy_u16 outer_color, legacy_u16 side_color, legacy_u16 inner_color)
{
	legacy_u16 source_words[WHEEL_SOURCE_WORD_COUNT];
	legacy_u16 wheel_points[WHEEL_POINT_WORD_COUNT];
	legacy_u16 quad[WHEEL_QUAD_WORD_COUNT];
	legacy_u16 index;
	legacy_u16 next_index;
	legacy_u16 minimum_index;
	legacy_u16 minimum_y;

	for (index = 0; index < WHEEL_SOURCE_POINT_COUNT; index++) {
		source_words[index * PRERENDER_POINT_WORD_COUNT] =
			(legacy_u16)source[index].px;
		source_words[index * PRERENDER_POINT_WORD_COUNT +
			PRERENDER_POINT_Y_OFFSET] = (legacy_u16)source[index].py;
	}
	preRender_wheel_helper(source_words, wheel_points, scale);
	for (index = 0; index < WHEEL_PERIMETER_POINT_COUNT; index++) {
		next_index = (legacy_u16)((index + 1U) &
			WHEEL_PERIMETER_INDEX_MASK);
		quad[0] = wheel_points[index * PRERENDER_POINT_WORD_COUNT];
		quad[1] = wheel_points[index * PRERENDER_POINT_WORD_COUNT +
			PRERENDER_POINT_Y_OFFSET];
		quad[2] = wheel_points[next_index * PRERENDER_POINT_WORD_COUNT];
		quad[3] = wheel_points[next_index * PRERENDER_POINT_WORD_COUNT +
			PRERENDER_POINT_Y_OFFSET];
		quad[4] = wheel_points[WHEEL_DEPTH_WORD_OFFSET +
			next_index * PRERENDER_POINT_WORD_COUNT];
		quad[5] = wheel_points[WHEEL_DEPTH_WORD_OFFSET +
			next_index * PRERENDER_POINT_WORD_COUNT +
			PRERENDER_POINT_Y_OFFSET];
		quad[6] = wheel_points[WHEEL_DEPTH_WORD_OFFSET +
			index * PRERENDER_POINT_WORD_COUNT];
		quad[7] = wheel_points[WHEEL_DEPTH_WORD_OFFSET +
			index * PRERENDER_POINT_WORD_COUNT +
			PRERENDER_POINT_Y_OFFSET];
		preRender_default_alt_words(outer_color,
			WHEEL_QUAD_POINT_COUNT, quad);
	}

	minimum_index = 0;
	minimum_y = (legacy_u16)wheel_points[1];
	for (index = 1; index < WHEEL_PERIMETER_POINT_COUNT; index++) {
		if (LEGACY_S16_FROM_BITS(wheel_points[
			index * PRERENDER_POINT_WORD_COUNT +
			PRERENDER_POINT_Y_OFFSET]) <
			LEGACY_S16_FROM_BITS(minimum_y)) {
			minimum_y = (legacy_u16)wheel_points[
				index * PRERENDER_POINT_WORD_COUNT +
				PRERENDER_POINT_Y_OFFSET];
			minimum_index = index;
		}
	}

	preRender_wheel_side(wheel_points, minimum_index, 0U, side_color);
	preRender_wheel_side(wheel_points, minimum_index, 1U, side_color);
	preRender_default_alt_words(inner_color, WHEEL_PERIMETER_POINT_COUNT,
		&wheel_points[WHEEL_INNER_WORD_OFFSET]);
}

void preRender_sphere(legacy_s16 x, legacy_s16 y, legacy_u16 size, legacy_u16 color)
{
	legacy_s16 left_edges[SPHERE_RASTER_TABLE_LIMIT * 2U];
	legacy_s16 right_edges[SPHERE_RASTER_TABLE_LIMIT * 2U];
	legacy_u16 helper_points[6];
	legacy_u16 x_bits;
	legacy_u16 y_bits;
	legacy_u16 size_bits;
	legacy_u16 effective_height;
	legacy_u16 half_height;
	legacy_u16 half_width;
	legacy_u16 top;
	legacy_u16 line_count;
	legacy_u16 left_bound;
	legacy_u16 right_bound;
	legacy_u16 left;
	legacy_u16 right;
	legacy_u16 skip_lines;
	legacy_u16 output_index;
	legacy_u8* radii;
	legacy_u8 radius;
	legacy_s16 mirror_offset;
	legacy_s16 clip_delta;

	x_bits = (legacy_u16)x;
	y_bits = (legacy_u16)y;
	size_bits = (legacy_u16)size;
	effective_height = LEGACY_U16_WRAP_SUB(size_bits,
		(legacy_u16)(size_bits >> 2));
	effective_height = LEGACY_U16_WRAP_ADD(effective_height,
		(legacy_u16)(size_bits >> 4));
	if (LEGACY_S16_FROM_BITS(effective_height) <= 0)
		return;

	half_height = (legacy_u16)(effective_height >> 1);
	if (half_height == 0) {
		putpixel_single_maybe(LEGACY_S16_FROM_BITS(x_bits),
			LEGACY_S16_FROM_BITS(y_bits), color);
		return;
	}
	half_width = LEGACY_U16_WRAP_SUB(effective_height, half_height);
	left_bound = sprite1.sprite_left2;
	right_bound = LEGACY_U16_WRAP_SUB(sprite1.sprite_widthsum, 1U);
	top = LEGACY_U16_WRAP_SUB(y_bits, half_height);
	if (LEGACY_S16_FROM_BITS(top) >=
		LEGACY_S16_FROM_BITS(sprite1.sprite_height))
		return;
	if (LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(y_bits, half_width)) <=
		LEGACY_S16_FROM_BITS(sprite1.sprite_top))
		return;
	half_width = LEGACY_U16_WRAP_ADD(half_width,
		(legacy_u16)(half_width >> 2));
	if (LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB(x_bits, half_width)) >
		LEGACY_S16_FROM_BITS(right_bound))
		return;
	if (LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(x_bits, half_width)) <
		LEGACY_S16_FROM_BITS(left_bound))
		return;

	half_width = LEGACY_U16_WRAP_SUB(effective_height, half_height);
	if (half_width >= SPHERE_RASTER_TABLE_LIMIT) {
		helper_points[0] = x_bits;
		helper_points[1] = y_bits;
		helper_points[2] = x_bits;
		helper_points[3] = LEGACY_U16_WRAP_ADD(y_bits, half_height);
		helper_points[4] = LEGACY_U16_WRAP_ADD(x_bits,
			(legacy_u16)(size_bits >> 1));
		helper_points[5] = y_bits;
		preRender_sphere_helper(helper_points, color);
		return;
	}

	radii = off_3F3C8[half_width];
	line_count = effective_height;
	mirror_offset = (legacy_s16)((effective_height - 1U) << 1);
	output_index = 0;
	for (;;) {
		radius = *radii++;
		left = LEGACY_U16_WRAP_SUB(x_bits, radius);
		right = LEGACY_U16_WRAP_ADD(x_bits, radius);
		if (LEGACY_S16_FROM_BITS(left) >
				LEGACY_S16_FROM_BITS(right_bound) ||
			LEGACY_S16_FROM_BITS(right) <
				LEGACY_S16_FROM_BITS(left_bound)) {
			top = LEGACY_U16_WRAP_ADD(top, 1U);
			line_count = LEGACY_U16_WRAP_SUB(line_count, 2U);
			mirror_offset -= 4;
			if (mirror_offset < 0)
				return;
			continue;
		}
		if (LEGACY_S16_FROM_BITS(left) <
			LEGACY_S16_FROM_BITS(left_bound))
			left = left_bound;
		if (LEGACY_S16_FROM_BITS(right) >
			LEGACY_S16_FROM_BITS(right_bound))
			right = right_bound;
		left_edges[output_index] = left;
		right_edges[output_index] = right;
		left_edges[output_index + (legacy_u16)(mirror_offset >> 1)] = left;
		right_edges[output_index + (legacy_u16)(mirror_offset >> 1)] = right;
		output_index++;
		mirror_offset -= 4;
		if (mirror_offset < 0)
			break;
	}

	skip_lines = 0;
	clip_delta = LEGACY_S16_WRAP_SUB(sprite1.sprite_top, top);
	if (clip_delta > 0) {
		line_count = LEGACY_U16_WRAP_SUB(line_count, clip_delta);
		skip_lines = (legacy_u16)clip_delta;
		top = sprite1.sprite_top;
	}
	clip_delta = LEGACY_S16_WRAP_SUB(
		LEGACY_U16_WRAP_ADD(top, line_count), sprite1.sprite_height);
	if (clip_delta > 0)
		line_count = LEGACY_U16_WRAP_SUB(line_count, clip_delta);
	draw_filled_lines(&left_edges[skip_lines], &right_edges[skip_lines],
		top, line_count, color);
}

void skybox_op_helper(legacy_u16 arg_color, legacy_u16 arg_vertlinecount, struct POINT2D arg_vertlines[]) {
	preRender_default(arg_color, arg_vertlinecount, arg_vertlines);
}

void preRender_wheel_helper4(legacy_u16 arg_color, legacy_u16 arg_vertlinecount, struct POINT2D arg_vertlines[]) {
	preRender_default_alt(arg_color, arg_vertlinecount, arg_vertlines);
}


void preRender_unk(legacy_u16 unk, legacy_u16 arg_color, legacy_u16 unk2,
	legacy_u16 arg_vertlinecount, const struct POINT2D* arg_vertlines) {
	spritefunc = &draw_unknown_lines;
	imagefunc = &preRender_line;

	word_4031E = unk;
	word_40320 = unk2;
	preRender_default_impl(arg_color, arg_vertlinecount, arg_vertlines, 1);
}

void preRender_patterned(legacy_u16 unk, legacy_u16 arg_color,
	legacy_u16 arg_vertlinecount, const struct POINT2D* arg_vertlines) {
	//return ported_preRender_patterned_(unk, arg_color, arg_vertlinecount, arg_vertlines);

	spritefunc = &draw_patterned_lines;
	imagefunc = &preRender_line;
	word_4031E = unk;

	preRender_default_impl(arg_color, arg_vertlinecount, arg_vertlines, 1);
}

static void preRender_default_impl(legacy_u16 arg_color,
	legacy_u16 arg_vertlinecount, const struct POINT2D* arg_vertlines,
	legacy_u16 var_A) {
	legacy_s16 var_798[
		PRERENDER_EDGE_ROW_CAPACITY * PRERENDER_EDGE_TABLE_COUNT];
	legacy_u16 var_7D0[
		DRAW_LINE_WORD_COUNT * PRERENDER_LINE_BUFFER_COUNT];

	legacy_s16* var_18;
	const struct POINT2D* var_16;
	const struct POINT2D* var_14;
	const struct POINT2D* var_10;
	legacy_s16 var_E, var_12;
	legacy_u16 var_C;
	const struct POINT2D* var_8;
	legacy_s16 var_4, var_2;

	const struct POINT2D* var_vertlineptr;
	legacy_s16 minx, maxx, i;
	legacy_s16 temp0x, temp0y, temp1x, temp1y;

	legacy_s16 sprite1_sprite_left2 = sprite1.sprite_left2;
	legacy_s16 sprite1_sprite_widthsum = sprite1.sprite_widthsum;
	legacy_s16 sprite1_sprite_top = sprite1.sprite_top;
	legacy_s16 sprite1_sprite_height = sprite1.sprite_height;

	if (arg_vertlinecount == 0U)
		return;
	var_vertlineptr = arg_vertlines;
	var_8 = var_vertlineptr + arg_vertlinecount - 1U;
	var_18 = var_798;
	var_2 = sprite1_sprite_left2;
	var_4 = sprite1_sprite_widthsum - 1;
	var_12 = var_E = var_vertlineptr->py;
	maxx = minx = var_vertlineptr->px;
	var_10 = arg_vertlines;
	var_14 = arg_vertlines;
	if (arg_vertlinecount == 1U) {
		imagefunc(var_vertlineptr->px, var_vertlineptr->py,
			var_vertlineptr->px, var_vertlineptr->py, arg_color);
		return ;
	}

	for (i = 1; i < arg_vertlinecount; i++) {
		if (arg_vertlines[i].py <= var_E) {
			var_E = arg_vertlines[i].py;
			var_10 = &arg_vertlines[i];
		}
		if (arg_vertlines[i].py > var_12) {
			var_12 = arg_vertlines[i].py;
			var_14 = &arg_vertlines[i];
		}

		if (arg_vertlines[i].px < minx) {
			minx = arg_vertlines[i].px;
		}
		if (arg_vertlines[i].px > maxx) {
			maxx = arg_vertlines[i].px;
		}

	}

	if (maxx < var_2) return;
	if (minx >= var_4) return ;
	if (var_12 < sprite1_sprite_top) return ;
	if (var_E >= sprite1_sprite_height) return ;
	var_C = 0;

	if (maxx > var_4 || minx < var_2 || var_12 >= sprite1_sprite_height || var_E < sprite1_sprite_top) {
		var_C = 1;
	}
	if (var_12 == var_E || maxx == minx) {
		imagefunc(minx, var_E, maxx, var_12, arg_color);
		return ;
	}

	var_16 = var_10;

	do {
		temp0x = var_16->px;
		temp0y = var_16->py;
		var_16++;
		if (var_16 > var_8)
			var_16 = var_vertlineptr;

		temp1x = var_16->px;
		temp1y = var_16->py;
		if (temp1y > temp0y) {

			if (var_C != 0) {
				draw_line_related(temp0x, temp0y, temp1x, temp1y, var_7D0);
				generate_poly_edges(var_18, var_7D0,
					PRERENDER_EDGE_CLIPPED_MODE);
			} else {
				draw_line_related_alt(temp0x, temp0y, temp1x, temp1y, var_7D0);
				generate_poly_edges(var_18, var_7D0,
					PRERENDER_EDGE_UNCLIPPED_MODE);
			}
		}

	} while (var_16 != var_14);

	var_16 = var_10;
	do {
		temp0x = var_16->px;
		temp0y = var_16->py;
		if (var_16 == var_vertlineptr)
			var_16 = var_8;
		else
			var_16--;
		temp1x = var_16->px;
		temp1y = var_16->py;
		if (temp1y > temp0y) {

			if (var_C != 0) {
				draw_line_related(temp0x, temp0y, temp1x, temp1y, var_7D0);
			} else {
				draw_line_related_alt(temp0x, temp0y, temp1x, temp1y, var_7D0);
			}
			preRender_default_impl_helper(var_7D0, var_A, var_C, var_18);
		}
	} while (var_16 != var_14);

	temp0y = var_12;

	if (temp0y >= sprite1_sprite_height)
		temp0y = sprite1_sprite_height - 1;
	temp1y = var_E;
	if (temp1y < sprite1_sprite_top)
		temp1y = sprite1_sprite_top;

	temp0x = temp0y - temp1y;
	if (temp0x <= 0) return ;
	temp0x++;

	spritefunc(&var_798[temp1y],
		&var_798[PRERENDER_EDGE_ROW_CAPACITY + temp1y],
		temp1y, temp0x, arg_color);

}

static void generate_poly_edge_fill(legacy_s16* edges,
	legacy_s16 offset, legacy_s16 count, legacy_s16 boundary)
{
	legacy_s16 index;

	for (index = 0; index < count; index++) {
		edges[offset + index] = boundary;
		edges[PRERENDER_EDGE_ROW_CAPACITY + offset + index] = boundary - 1;
	}
}

static void generate_poly_edge_padding(legacy_s16* edges,
	const legacy_u16* setup, legacy_u16 count_index,
	legacy_s16 boundary, legacy_s16 leading)
{
	legacy_s16 count;
	legacy_s16 offset;

	count = LEGACY_S16_FROM_BITS(setup[count_index]);
	if (count <= 0)
		return;
	if (leading != 0) {
		offset = LEGACY_S16_WRAP_SUB(
			LEGACY_S16_FROM_BITS(setup[DRAW_LINE_START_Y_INDEX]), count);
		if (LEGACY_S16_FROM_BITS(
			setup[DRAW_LINE_START_Y_FRACTION_INDEX]) < 0)
			offset = LEGACY_S16_WRAP_ADD(offset, 1);
	} else {
		offset = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_FROM_BITS(setup[DRAW_LINE_END_Y_INDEX]), 1);
	}
	generate_poly_edge_fill(edges, offset, count, boundary);
}

void generate_poly_edges(legacy_s16* var_18, const legacy_u16* regsi, legacy_s16 mode) {

	legacy_s16 sprite1_sprite_left2 = sprite1.sprite_left2;
	legacy_s16 sprite1_sprite_widthsum = sprite1.sprite_widthsum;
	legacy_s16 i, count, ofs;
	legacy_u32 value;
	legacy_u32 temp;

	if (mode != PRERENDER_EDGE_UNCLIPPED_MODE) {
		generate_poly_edge_padding(var_18, regsi,
			DRAW_LINE_START_LEFT_CLIP_COUNT_INDEX,
			sprite1_sprite_left2, 1);
		generate_poly_edge_padding(var_18, regsi,
			DRAW_LINE_START_RIGHT_CLIP_COUNT_INDEX,
			sprite1_sprite_widthsum, 1);
		generate_poly_edge_padding(var_18, regsi,
			DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX,
			sprite1_sprite_left2, 0);
		generate_poly_edge_padding(var_18, regsi,
			DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX,
			sprite1_sprite_widthsum, 0);
	}

	count = LEGACY_S16_FROM_BITS(regsi[DRAW_LINE_PIXEL_COUNT_INDEX]);
	if (count <= 0) return ;

	ofs = LEGACY_S16_FROM_BITS(regsi[DRAW_LINE_START_Y_INDEX]);

	switch ((legacy_u8)regsi[DRAW_LINE_MODE_AND_CLIP_INDEX]) {
		case DRAW_LINE_MODE_HORIZONTAL_REVERSED:
		case DRAW_LINE_MODE_HORIZONTAL:
			return;
		case DRAW_LINE_MODE_VERTICAL:
			for (i = 0; i < count; i++) {
				var_18[ofs + i] = LEGACY_S16_FROM_BITS(
					regsi[DRAW_LINE_START_X_INDEX]);
				var_18[PRERENDER_EDGE_ROW_CAPACITY + ofs + i] =
					LEGACY_S16_FROM_BITS(
						regsi[DRAW_LINE_START_X_INDEX]);
			}
			return ;
		case DRAW_LINE_MODE_DIAGONAL_LEFT:
			for (i = 0; i < count; i++) {
				var_18[ofs + i] = LEGACY_S16_WRAP_SUB(
					LEGACY_S16_FROM_BITS(
						regsi[DRAW_LINE_START_X_INDEX]), i);
				var_18[PRERENDER_EDGE_ROW_CAPACITY + ofs + i] =
					LEGACY_S16_WRAP_SUB(LEGACY_S16_FROM_BITS(
						regsi[DRAW_LINE_START_X_INDEX]), i);
			}
			return ;
		case DRAW_LINE_MODE_DIAGONAL_RIGHT:
			for (i = 0; i < count; i++) {
				var_18[ofs + i] = LEGACY_S16_WRAP_ADD(
					LEGACY_S16_FROM_BITS(
						regsi[DRAW_LINE_START_X_INDEX]), i);
				var_18[PRERENDER_EDGE_ROW_CAPACITY + ofs + i] =
					LEGACY_S16_WRAP_ADD(LEGACY_S16_FROM_BITS(
						regsi[DRAW_LINE_START_X_INDEX]), i);
			}
			return ;
		case DRAW_LINE_MODE_Y_MAJOR_LEFT:
		case DRAW_LINE_MODE_Y_MAJOR_RIGHT:
			value = LEGACY_U32_WRAP_ADD(
				LEGACY_U32_FROM_WORDS(
					regsi[DRAW_LINE_START_X_FRACTION_INDEX],
					regsi[DRAW_LINE_START_X_INDEX]),
				DRAW_LINE_FIXED_ROUNDING);
			for (i = 0; i < count; i++) {
				var_18[ofs + i] = LEGACY_S16_FROM_BITS(
					(legacy_u16)(value >> LEGACY_WORD_BITS));
				var_18[PRERENDER_EDGE_ROW_CAPACITY + ofs + i] =
					LEGACY_S16_FROM_BITS(
						(legacy_u16)(value >> LEGACY_WORD_BITS));
				if ((legacy_u8)regsi[DRAW_LINE_MODE_AND_CLIP_INDEX] ==
					DRAW_LINE_MODE_Y_MAJOR_LEFT)
					value = LEGACY_U32_WRAP_SUB(value,
						(legacy_u16)regsi[DRAW_LINE_STEP_INDEX]);
				else
					value = LEGACY_U32_WRAP_ADD(value,
						(legacy_u16)regsi[DRAW_LINE_STEP_INDEX]);
			}
			return ;
		case DRAW_LINE_MODE_X_MAJOR_LEFT:
			value = (legacy_u16)regsi[DRAW_LINE_START_X_INDEX];
			temp = (legacy_u16)regsi[DRAW_LINE_START_Y_FRACTION_INDEX];
			if (temp + DRAW_LINE_FIXED_ROUNDING >
				PRERENDER_FIXED_CARRY_LIMIT)
				ofs++;
			temp = (temp + DRAW_LINE_FIXED_ROUNDING) & LEGACY_U16_MAX;
			var_18[PRERENDER_EDGE_ROW_CAPACITY + ofs] = value;
			for (i = 0; i < count; i++) {
				if (temp + (legacy_u16)regsi[DRAW_LINE_STEP_INDEX] <=
					PRERENDER_FIXED_CARRY_LIMIT) {
					value--;
					if (i == count - 1) {
						var_18[ofs] = value + 1;
					}
				} else {
					var_18[ofs] = value;
					value--;
					ofs++;
					var_18[PRERENDER_EDGE_ROW_CAPACITY + ofs] = value;
				}
				temp = (temp +
					(legacy_u16)regsi[DRAW_LINE_STEP_INDEX]) &
					LEGACY_U16_MAX;
			}
			return ;

		case DRAW_LINE_MODE_X_MAJOR_RIGHT:
			value = (legacy_u16)regsi[DRAW_LINE_START_X_INDEX];
			temp = (legacy_u16)regsi[DRAW_LINE_START_Y_FRACTION_INDEX];
			if (temp + DRAW_LINE_FIXED_ROUNDING >
				PRERENDER_FIXED_CARRY_LIMIT)
				ofs++;
			temp = (temp + DRAW_LINE_FIXED_ROUNDING) & LEGACY_U16_MAX;
			var_18[ofs] = value;
			for (i = 0; i < count; i++) {
				if (temp + (legacy_u16)regsi[DRAW_LINE_STEP_INDEX] <=
					PRERENDER_FIXED_CARRY_LIMIT) {
					value++;
					if (i == count - 1) {
						var_18[PRERENDER_EDGE_ROW_CAPACITY + ofs] =
							value - 1;
					}
				} else {
					var_18[PRERENDER_EDGE_ROW_CAPACITY + ofs] = value;
					value++;
					ofs++;
					var_18[ofs] = value;
				}
				temp = (temp +
					(legacy_u16)regsi[DRAW_LINE_STEP_INDEX]) &
					LEGACY_U16_MAX;
				}
				return ;
			case DRAW_LINE_MODE_POINT:
			default:
				return ;
	}
}




/* right_edges keeps the largest x seen on a row, left_edges the smallest. */
static void prerender_extend_edge(legacy_s16* left_edges,
	legacy_s16* right_edges, legacy_s16 index, legacy_s16 value,
	legacy_s16 to_left)
{
	if (to_left) {
		if (left_edges[index] > value)
			left_edges[index] = value;
	} else if (right_edges[index] < value) {
		right_edges[index] = value;
	}
}

void preRender_default_impl_helper(const legacy_u16* regsi, legacy_u16 var_A,
	legacy_u16 var_C, legacy_s16* var_18)
{
	const legacy_u16* line = regsi;
	legacy_s16* left_edges = var_18;
	legacy_s16* right_edges = left_edges + PRERENDER_EDGE_ROW_CAPACITY;
	legacy_s16 value;
	legacy_u16 fraction;
	legacy_u16 step;
	legacy_u16 mode;
	legacy_u32 fixed;
	legacy_u32 sum;
	legacy_s16 count;
	legacy_s16 index;
	legacy_s16 target;
	legacy_s16 carry;
	legacy_s16 to_left;
	legacy_s16 value_step;

	count = LEGACY_S16_FROM_BITS(line[DRAW_LINE_PIXEL_COUNT_INDEX]);
	index = LEGACY_S16_FROM_BITS(line[DRAW_LINE_START_Y_INDEX]);
	mode = (legacy_u8)line[DRAW_LINE_MODE_AND_CLIP_INDEX];

	if (count > 0 && mode >= DRAW_LINE_MODE_VERTICAL &&
		mode <= DRAW_LINE_MODE_X_MAJOR_RIGHT) {
		value = LEGACY_S16_FROM_BITS(line[DRAW_LINE_START_X_INDEX]);

		if (var_A != 0U) {
			switch (mode) {
			case DRAW_LINE_MODE_VERTICAL:
			case DRAW_LINE_MODE_DIAGONAL_LEFT:
			case DRAW_LINE_MODE_DIAGONAL_RIGHT:
				while (count-- > 0) {
					if (right_edges[index] < value)
						right_edges[index] = value;
					else if (left_edges[index] > value)
						left_edges[index] = value;
					index++;
					if (mode == DRAW_LINE_MODE_DIAGONAL_LEFT)
						value = LEGACY_S16_WRAP_SUB(value, 1);
					else if (mode == DRAW_LINE_MODE_DIAGONAL_RIGHT)
						value = LEGACY_S16_WRAP_ADD(value, 1);
				}
				break;

			case DRAW_LINE_MODE_Y_MAJOR_LEFT:
			case DRAW_LINE_MODE_Y_MAJOR_RIGHT:
				fixed = ((legacy_u32)(legacy_u16)
					line[DRAW_LINE_START_X_INDEX] << LEGACY_WORD_BITS) |
					(legacy_u16)line[DRAW_LINE_START_X_FRACTION_INDEX];
				fixed += DRAW_LINE_FIXED_ROUNDING;
				step = (legacy_u16)line[DRAW_LINE_STEP_INDEX];
				while (count-- > 0) {
					value = LEGACY_S16_FROM_BITS(
						(legacy_u16)(fixed >> LEGACY_WORD_BITS));
					if (right_edges[index] < value)
						right_edges[index] = value;
					else if (left_edges[index] > value)
						left_edges[index] = value;
					index++;
					if (mode == DRAW_LINE_MODE_Y_MAJOR_LEFT)
						fixed -= step;
					else
						fixed += step;
				}
				break;

			case DRAW_LINE_MODE_X_MAJOR_LEFT:
			case DRAW_LINE_MODE_X_MAJOR_RIGHT:
				/* Mode 7 walks the span right-to-left and mode 8
				   left-to-right, which only swaps which edge array
				   opens a row and which one closes it. */
				to_left = mode == DRAW_LINE_MODE_X_MAJOR_LEFT ? 1 : 0;
				value_step = to_left ? -1 : 1;
				step = (legacy_u16)line[DRAW_LINE_STEP_INDEX];
				sum = (legacy_u32)(legacy_u16)
					line[DRAW_LINE_START_Y_FRACTION_INDEX] +
					DRAW_LINE_FIXED_ROUNDING;
				fraction = (legacy_u16)sum;
				if (sum > PRERENDER_FIXED_CARRY_LIMIT)
					index++;
				prerender_extend_edge(left_edges, right_edges, index,
					value, !to_left);
				while (count > 0) {
					sum = (legacy_u32)fraction + step;
					fraction = (legacy_u16)sum;
					carry = sum > PRERENDER_FIXED_CARRY_LIMIT;
					if (carry) {
						prerender_extend_edge(left_edges,
							right_edges, index, value, to_left);
						index++;
						value = LEGACY_S16_WRAP_ADD(value,
							value_step);
						count--;
						if (count > 0)
							prerender_extend_edge(left_edges,
								right_edges, index, value,
								!to_left);
					} else {
						value = LEGACY_S16_WRAP_ADD(value,
							value_step);
						count--;
						if (count == 0) {
							value = LEGACY_S16_WRAP_SUB(value,
								value_step);
							prerender_extend_edge(left_edges,
								right_edges, index, value,
								to_left);
						}
					}
				}
				break;
			}
		} else if (mode <= DRAW_LINE_MODE_Y_MAJOR_RIGHT) {
			fixed = ((legacy_u32)(legacy_u16)
				line[DRAW_LINE_START_X_INDEX] << LEGACY_WORD_BITS) |
				(legacy_u16)line[DRAW_LINE_START_X_FRACTION_INDEX];
			fixed += DRAW_LINE_FIXED_ROUNDING;
			step = (legacy_u16)line[DRAW_LINE_STEP_INDEX];
			target = 0;

			while (count > 0) {
				value = mode >= DRAW_LINE_MODE_Y_MAJOR_LEFT ?
					LEGACY_S16_FROM_BITS(
						(legacy_u16)(fixed >> LEGACY_WORD_BITS)) :
					LEGACY_S16_FROM_BITS(line[DRAW_LINE_START_X_INDEX]);
				if (mode == DRAW_LINE_MODE_DIAGONAL_LEFT)
					value = LEGACY_S16_WRAP_SUB(value,
						LEGACY_S16_WRAP_SUB(
							LEGACY_S16_FROM_BITS(
								line[DRAW_LINE_PIXEL_COUNT_INDEX]), count));
				else if (mode == DRAW_LINE_MODE_DIAGONAL_RIGHT)
					value = LEGACY_S16_WRAP_ADD(value,
						LEGACY_S16_WRAP_SUB(
							LEGACY_S16_FROM_BITS(
								line[DRAW_LINE_PIXEL_COUNT_INDEX]), count));

				if (target == 0) {
					if (left_edges[index] > value)
						target = 1;
					else if (right_edges[index] < value)
						target = 2;
				}
				if (target == 1)
					left_edges[index] = value;
				else if (target == 2)
					right_edges[index] = value;
				index++;
				count--;
				if (mode == DRAW_LINE_MODE_Y_MAJOR_LEFT)
					fixed -= step;
				else if (mode == DRAW_LINE_MODE_Y_MAJOR_RIGHT)
					fixed += step;
			}
		} else if (mode == DRAW_LINE_MODE_X_MAJOR_LEFT) {
			step = (legacy_u16)line[DRAW_LINE_STEP_INDEX];
			sum = (legacy_u32)(legacy_u16)
				line[DRAW_LINE_START_Y_FRACTION_INDEX] +
				DRAW_LINE_FIXED_ROUNDING;
			fraction = (legacy_u16)sum;
			if (sum > PRERENDER_FIXED_CARRY_LIMIT)
				index++;

			while (count > 0) {
				if (right_edges[index] < value) {
					right_edges[index++] = value;
					while (count > 0) {
						value = LEGACY_S16_WRAP_SUB(value, 1);
						sum = (legacy_u32)fraction + step;
						fraction = (legacy_u16)sum;
						count--;
						if (count > 0 &&
							sum > PRERENDER_FIXED_CARRY_LIMIT)
							right_edges[index++] = value;
					}
					break;
				}

				sum = (legacy_u32)fraction + step;
				fraction = (legacy_u16)sum;
				carry = sum > PRERENDER_FIXED_CARRY_LIMIT;
				if (carry && left_edges[index] > value) {
					left_edges[index++] = value;
					value = LEGACY_S16_WRAP_SUB(value, 1);
					count--;
					while (count > 0) {
						sum = (legacy_u32)fraction + step;
						fraction = (legacy_u16)sum;
						carry = sum > PRERENDER_FIXED_CARRY_LIMIT;
						if (carry)
							left_edges[index++] = value;
						value = LEGACY_S16_WRAP_SUB(value, 1);
						count--;
						if (count == 0 && !carry) {
							value = LEGACY_S16_WRAP_ADD(value, 1);
							left_edges[index] = value;
						}
					}
					break;
				}
				if (carry)
					index++;
				value = LEGACY_S16_WRAP_SUB(value, 1);
				count--;
			}
		} else {
			step = (legacy_u16)line[DRAW_LINE_STEP_INDEX];
			sum = (legacy_u32)(legacy_u16)
				line[DRAW_LINE_START_Y_FRACTION_INDEX] +
				DRAW_LINE_FIXED_ROUNDING;
			fraction = (legacy_u16)sum;
			if (sum > PRERENDER_FIXED_CARRY_LIMIT)
				index++;

			while (count > 0) {
				if (left_edges[index] > value) {
					left_edges[index++] = value;
					value = LEGACY_S16_WRAP_ADD(value, 1);
					count--;
					while (count > 0) {
						sum = (legacy_u32)fraction + step;
						fraction = (legacy_u16)sum;
						if (sum > PRERENDER_FIXED_CARRY_LIMIT)
							left_edges[index++] = value;
						value = LEGACY_S16_WRAP_ADD(value, 1);
						count--;
					}
					break;
				}

				sum = (legacy_u32)fraction + step;
				fraction = (legacy_u16)sum;
				carry = sum > PRERENDER_FIXED_CARRY_LIMIT;
				if (carry && right_edges[index] < value) {
					right_edges[index] = value;
					while (count > 0) {
						value = LEGACY_S16_WRAP_ADD(value, 1);
						sum = (legacy_u32)fraction + step;
						fraction = (legacy_u16)sum;
						carry = sum > PRERENDER_FIXED_CARRY_LIMIT;
						if (carry)
							index++;
						count--;
						if (count > 0 && carry)
							right_edges[index] = value;
						else if (count == 0) {
							if (!carry)
								index++;
							value = LEGACY_S16_WRAP_SUB(value, 1);
							right_edges[index] = value;
						}
					}
					break;
				}
				if (carry)
					index++;
				value = LEGACY_S16_WRAP_ADD(value, 1);
				count--;
			}
		}
	}

	if (var_C == 0U)
		return;

	sum = (legacy_u32)(legacy_u16)
		line[DRAW_LINE_START_Y_FRACTION_INDEX] +
		DRAW_LINE_FIXED_ROUNDING;
	carry = sum > PRERENDER_FIXED_CARRY_LIMIT;

	count = LEGACY_S16_FROM_BITS(
		line[DRAW_LINE_START_LEFT_CLIP_COUNT_INDEX]);
	if (count > 0) {
		index = LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_ADD(
			LEGACY_S16_FROM_BITS(line[DRAW_LINE_START_Y_INDEX]), carry),
			count);
		while (count-- > 0)
			left_edges[index++] =
				LEGACY_S16_FROM_BITS(sprite1.sprite_left2);
	}

	count = LEGACY_S16_FROM_BITS(
		line[DRAW_LINE_START_RIGHT_CLIP_COUNT_INDEX]);
	if (count > 0) {
		index = LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_ADD(
			LEGACY_S16_FROM_BITS(line[DRAW_LINE_START_Y_INDEX]), carry),
			count);
		while (count-- > 0)
			right_edges[index++] = LEGACY_S16_WRAP_SUB(
				LEGACY_S16_FROM_BITS(sprite1.sprite_widthsum), 1);
	}

	count = LEGACY_S16_FROM_BITS(
		line[DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX]);
	if (count > 0) {
		index = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_FROM_BITS(line[DRAW_LINE_END_Y_INDEX]), 1);
		while (count-- > 0)
			left_edges[index++] =
				LEGACY_S16_FROM_BITS(sprite1.sprite_left2);
	}

	count = LEGACY_S16_FROM_BITS(
		line[DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX]);
	if (count > 0) {
		index = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_FROM_BITS(line[DRAW_LINE_END_Y_INDEX]), 1);
		while (count-- > 0)
			right_edges[index++] = LEGACY_S16_WRAP_SUB(
				LEGACY_S16_FROM_BITS(sprite1.sprite_widthsum), 1);
	}
}
