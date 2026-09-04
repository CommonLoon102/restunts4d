#include "externs.h"
#include "legacy.h"
#include "shape2d.h"
#include "shape2d_internal.h"
#include "shape3d.h"
#include "shape3d_internal.h"

#define SPHERE_RASTER_TABLE_LIMIT 40U

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
	legacy_u16 line[14];

	line[8] = color;
	if (draw_line_related(start_x, start_y, end_x, end_y, line) == 0 &&
			LEGACY_S16_FROM_BITS(line[7]) > 0)
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
	struct POINT2D vertices[32];
	legacy_u16 index;

	for (index = 0; index < vertex_count; index++) {
		vertices[index].px = LEGACY_S16_FROM_BITS(words[index * 2U]);
		vertices[index].py = LEGACY_S16_FROM_BITS(words[index * 2U + 1U]);
	}
	preRender_default_alt(color, vertex_count, vertices);
}

static legacy_u16 shape3d_sar1(legacy_u16 value)
{
	return (legacy_u16)((value >> 1) | (value & 0x8000U));
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
	half_x1 = shape3d_sar1((legacy_u16)destination[0]);
	quarter_x1 = shape3d_sar1(half_x1);
	three_quarters_x1 = LEGACY_U16_WRAP_ADD(half_x1, quarter_x1);
	half_y1 = shape3d_sar1((legacy_u16)destination[1]);
	quarter_y1 = shape3d_sar1(half_y1);
	three_quarters_y1 = LEGACY_U16_WRAP_ADD(half_y1, quarter_y1);
	half_x2 = shape3d_sar1((legacy_u16)destination[16]);
	quarter_x2 = shape3d_sar1(half_x2);
	three_quarters_x2 = LEGACY_U16_WRAP_ADD(half_x2, quarter_x2);
	half_y2 = shape3d_sar1((legacy_u16)destination[17]);
	quarter_y2 = shape3d_sar1(half_y2);
	three_quarters_y2 = LEGACY_U16_WRAP_ADD(half_y2, quarter_y2);

	destination[8] = sphere_scale_sum(destination[16], destination[0],
		0x2D41U);
	destination[9] = sphere_scale_sum(destination[1], destination[17],
		0x2D41U);
	destination[4] = sphere_scale_sum(destination[0], half_x2, 0x393EU);
	destination[5] = sphere_scale_sum(destination[1], half_y2, 0x393EU);
	destination[12] = sphere_scale_sum(destination[16], half_x1, 0x393EU);
	destination[13] = sphere_scale_sum(destination[17], half_y1, 0x393EU);
	destination[2] = sphere_scale_sum(destination[0], quarter_x2, 0x3E17U);
	destination[3] = sphere_scale_sum(destination[1], quarter_y2, 0x3E17U);
	destination[14] = sphere_scale_sum(destination[16], quarter_x1, 0x3E17U);
	destination[15] = sphere_scale_sum(destination[17], quarter_y1, 0x3E17U);
	destination[6] = sphere_scale_sum(destination[0], three_quarters_x2,
		0x3333U);
	destination[7] = sphere_scale_sum(destination[1], three_quarters_y2,
		0x3333U);
	destination[10] = sphere_scale_sum(destination[16], three_quarters_x1,
		0x3333U);
	destination[11] = sphere_scale_sum(destination[17], three_quarters_y1,
		0x3333U);

	negative_x1 = LEGACY_U16_WRAP_SUB(0, destination[0]);
	negative_y1 = LEGACY_U16_WRAP_SUB(0, destination[1]);
	destination[24] = sphere_scale_sum(destination[16], negative_x1,
		0x2D41U);
	destination[25] = sphere_scale_sum(destination[17], negative_y1,
		0x2D41U);
	destination[28] = sphere_scale_sum(half_x2, negative_x1, 0x393EU);
	destination[29] = sphere_scale_sum(half_y2, negative_y1, 0x393EU);
	destination[20] = sphere_scale_difference(destination[16], half_x1,
		0x393EU);
	destination[21] = sphere_scale_difference(destination[17], half_y1,
		0x393EU);
	destination[30] = sphere_scale_sum(quarter_x2, negative_x1, 0x3E17U);
	destination[31] = sphere_scale_sum(quarter_y2, negative_y1, 0x3E17U);
	destination[18] = sphere_scale_difference(destination[16], quarter_x1,
		0x3E17U);
	destination[19] = sphere_scale_difference(destination[17], quarter_y1,
		0x3E17U);
	destination[26] = sphere_scale_sum(three_quarters_x2, negative_x1,
		0x3333U);
	destination[27] = sphere_scale_sum(three_quarters_y2, negative_y1,
		0x3333U);
	destination[22] = sphere_scale_difference(destination[16],
		three_quarters_x1, 0x3333U);
	destination[23] = sphere_scale_difference(destination[17],
		three_quarters_y1, 0x3333U);

	center_x = (legacy_u16)source[0];
	center_y = (legacy_u16)source[1];
	for (index = 0; index < 16U; index++) {
		destination[32U + index * 2U] = LEGACY_U16_WRAP_SUB(
			center_x, destination[index * 2U]);
		destination[33U + index * 2U] = LEGACY_U16_WRAP_SUB(
			center_y, destination[index * 2U + 1U]);
		destination[index * 2U] = LEGACY_U16_WRAP_ADD(
			destination[index * 2U], center_x);
		destination[index * 2U + 1U] = LEGACY_U16_WRAP_ADD(
			destination[index * 2U + 1U], center_y);
	}
}

void preRender_sphere_helper(legacy_u16* source, legacy_u16 color)
{
	legacy_u16 vertices[64];

	preRender_sphere_helper2(source, vertices);
	preRender_default_alt_words(color, 0x20U, vertices);
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
		0x2D41U);
	destination[5] = sphere_scale_sum(destination[1], destination[9],
		0x2D41U);
	half_x2 = shape3d_sar1((legacy_u16)destination[8]);
	half_y2 = shape3d_sar1((legacy_u16)destination[9]);
	destination[2] = sphere_scale_sum(destination[0], half_x2, 0x393EU);
	destination[3] = sphere_scale_sum(destination[1], half_y2, 0x393EU);
	half_x1 = shape3d_sar1((legacy_u16)destination[0]);
	half_y1 = shape3d_sar1((legacy_u16)destination[1]);
	destination[6] = sphere_scale_sum(destination[8], half_x1, 0x393EU);
	destination[7] = sphere_scale_sum(destination[9], half_y1, 0x393EU);
	destination[12] = sphere_scale_difference(destination[8],
		destination[0], 0x2D41U);
	destination[13] = sphere_scale_difference(destination[9],
		destination[1], 0x2D41U);
	destination[14] = sphere_scale_difference(half_x2,
		destination[0], 0x393EU);
	destination[15] = sphere_scale_difference(half_y2,
		destination[1], 0x393EU);
	destination[10] = sphere_scale_difference(destination[8], half_x1,
		0x393EU);
	destination[11] = sphere_scale_difference(destination[9], half_y1,
		0x393EU);

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
	preRender_wheel_helper3(inner_source, destination + 32U);
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
	for (index = 0; index < 16U; index++) {
		destination[64U + index * 2U] = LEGACY_U16_WRAP_ADD(
			destination[index * 2U], offset_x);
		destination[65U + index * 2U] = LEGACY_U16_WRAP_ADD(
			destination[index * 2U + 1U], offset_y);
	}
}

static void preRender_wheel_side(const legacy_u16* wheel_points,
	legacy_u16 minimum_index, legacy_u16 reverse, legacy_u16 color)
{
	legacy_u16 side[36];
	legacy_u16 step;
	legacy_u16 point_index;
	legacy_u16 destination_index;

	for (step = 0; step < 9U; step++) {
		if (reverse == 0U)
			point_index = (legacy_u16)((minimum_index + step) & 0x0FU);
		else
			point_index = (legacy_u16)((minimum_index + 16U - step) & 0x0FU);
		destination_index = (legacy_u16)(step * 2U);
		side[destination_index] = wheel_points[point_index * 2U];
		side[destination_index + 1U] =
			wheel_points[point_index * 2U + 1U];
		destination_index = (legacy_u16)((17U - step) * 2U);
		side[destination_index] = wheel_points[32U + point_index * 2U];
		side[destination_index + 1U] =
			wheel_points[33U + point_index * 2U];
	}
	preRender_default_alt_words(color, 18U, side);
}

void preRender_wheel(const struct POINT2D* source, legacy_u16 scale,
	legacy_u16 outer_color, legacy_u16 side_color, legacy_u16 inner_color)
{
	legacy_u16 source_words[8];
	legacy_u16 wheel_points[96];
	legacy_u16 quad[8];
	legacy_u16 index;
	legacy_u16 next_index;
	legacy_u16 minimum_index;
	legacy_u16 minimum_y;

	for (index = 0; index < 4U; index++) {
		source_words[index * 2U] = (legacy_u16)source[index].px;
		source_words[index * 2U + 1U] = (legacy_u16)source[index].py;
	}
	preRender_wheel_helper(source_words, wheel_points, scale);
	for (index = 0; index < 16U; index++) {
		next_index = (legacy_u16)((index + 1U) & 0x0FU);
		quad[0] = wheel_points[index * 2U];
		quad[1] = wheel_points[index * 2U + 1U];
		quad[2] = wheel_points[next_index * 2U];
		quad[3] = wheel_points[next_index * 2U + 1U];
		quad[4] = wheel_points[64U + next_index * 2U];
		quad[5] = wheel_points[65U + next_index * 2U];
		quad[6] = wheel_points[64U + index * 2U];
		quad[7] = wheel_points[65U + index * 2U];
		preRender_default_alt_words(outer_color, 4U, quad);
	}

	minimum_index = 0;
	minimum_y = (legacy_u16)wheel_points[1];
	for (index = 1; index < 16U; index++) {
		if (LEGACY_S16_FROM_BITS(wheel_points[index * 2U + 1U]) <
			LEGACY_S16_FROM_BITS(minimum_y)) {
			minimum_y = (legacy_u16)wheel_points[index * 2U + 1U];
			minimum_index = index;
		}
	}

	preRender_wheel_side(wheel_points, minimum_index, 0U, side_color);
	preRender_wheel_side(wheel_points, minimum_index, 1U, side_color);
	preRender_default_alt_words(inner_color, 16U, &wheel_points[32]);
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
	//return ported_skybox_op_helper_(arg_color, arg_vertlinecount, &arg_vertlines);

	spritefunc = &draw_filled_lines;
	imagefunc = &preRender_line;
	preRender_default_impl(arg_color, arg_vertlinecount, arg_vertlines, 1);
}

void preRender_wheel_helper4(legacy_u16 arg_color, legacy_u16 arg_vertlinecount, struct POINT2D arg_vertlines[]) {
	//return ported_preRender_wheel_helper4_(arg_color, arg_vertlinecount, &arg_vertlines);

	spritefunc = &draw_filled_lines;
	imagefunc = &preRender_line;
	preRender_default_impl(arg_color, arg_vertlinecount, arg_vertlines, 0);
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
	legacy_s16 var_798[480 + 480];
	legacy_u16 var_7D0[28];

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
				generate_poly_edges(var_18, var_7D0, 0);
			} else {
				draw_line_related_alt(temp0x, temp0y, temp1x, temp1y, var_7D0);
				generate_poly_edges(var_18, var_7D0, 1);
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

	spritefunc(&var_798[temp1y], &var_798[480 + temp1y], temp1y, temp0x, arg_color);

}

static void generate_poly_edge_fill(legacy_s16* edges,
	legacy_s16 offset, legacy_s16 count, legacy_s16 boundary)
{
	legacy_s16 index;

	for (index = 0; index < count; index++) {
		edges[offset + index] = boundary;
		edges[480 + offset + index] = boundary - 1;
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
			LEGACY_S16_FROM_BITS(setup[3]), count);
		if (LEGACY_S16_FROM_BITS(setup[2]) < 0)
			offset = LEGACY_S16_WRAP_ADD(offset, 1);
	} else {
		offset = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_FROM_BITS(setup[5]), 1);
	}
	generate_poly_edge_fill(edges, offset, count, boundary);
}

void generate_poly_edges(legacy_s16* var_18, const legacy_u16* regsi, legacy_s16 mode) {

	legacy_s16 sprite1_sprite_left2 = sprite1.sprite_left2;
	legacy_s16 sprite1_sprite_widthsum = sprite1.sprite_widthsum;
	legacy_s16 i, count, ofs;
	legacy_u32 value;
	legacy_u32 temp;

	if (mode != 1) {
		generate_poly_edge_padding(var_18, regsi, 10,
			sprite1_sprite_left2, 1);
		generate_poly_edge_padding(var_18, regsi, 12,
			sprite1_sprite_widthsum, 1);
		generate_poly_edge_padding(var_18, regsi, 11,
			sprite1_sprite_left2, 0);
		generate_poly_edge_padding(var_18, regsi, 13,
			sprite1_sprite_widthsum, 0);
	}

	count = LEGACY_S16_FROM_BITS(regsi[7]);
	if (count <= 0) return ;

	ofs = LEGACY_S16_FROM_BITS(regsi[3]);

	switch ((legacy_u8)regsi[9]) {
		case 0:
		case 1:
			return;
		case 2:
			for (i = 0; i < count; i++) {
				var_18[ofs + i] = LEGACY_S16_FROM_BITS(regsi[1]);
				var_18[480 + ofs + i] =
					LEGACY_S16_FROM_BITS(regsi[1]);
			}
			return ;
		case 3:
			for (i = 0; i < count; i++) {
				var_18[ofs + i] = LEGACY_S16_WRAP_SUB(
					LEGACY_S16_FROM_BITS(regsi[1]), i);
				var_18[480 + ofs + i] = LEGACY_S16_WRAP_SUB(
					LEGACY_S16_FROM_BITS(regsi[1]), i);
			}
			return ;
		case 4:
			for (i = 0; i < count; i++) {
				var_18[ofs + i] = LEGACY_S16_WRAP_ADD(
					LEGACY_S16_FROM_BITS(regsi[1]), i);
				var_18[480 + ofs + i] = LEGACY_S16_WRAP_ADD(
					LEGACY_S16_FROM_BITS(regsi[1]), i);
			}
			return ;
		case 5:
		case 6:
			value = LEGACY_U32_WRAP_ADD(
				LEGACY_U32_FROM_WORDS(regsi[0], regsi[1]), 0x8000UL);
			for (i = 0; i < count; i++) {
				var_18[ofs + i] = LEGACY_S16_FROM_BITS(
					(legacy_u16)(value >> 16));
				var_18[480 + ofs + i] = LEGACY_S16_FROM_BITS(
					(legacy_u16)(value >> 16));
				if ((legacy_u8)regsi[9] == 5U)
					value = LEGACY_U32_WRAP_SUB(value,
						(legacy_u16)regsi[6]);
				else
					value = LEGACY_U32_WRAP_ADD(value,
						(legacy_u16)regsi[6]);
			}
			return ;
		case 7:
			value = (legacy_u16)regsi[1];
			temp = (legacy_u16)regsi[2];
			if (temp + 0x8000 > USHRT_MAX)
				ofs++;
			temp = (temp + 0x8000) & 0xFFFF;
			var_18[480 + ofs] = value;
			for (i = 0; i < count; i++) {
				if (temp + (legacy_u16)regsi[6] <= USHRT_MAX) {
					value--;
					if (i == count - 1) {
						var_18[ofs] = value + 1;
					}
				} else {
					var_18[ofs] = value;
					value--;
					ofs++;
					var_18[480 + ofs] = value;
				}
				temp = (temp + (legacy_u16)regsi[6])  & 0xFFFF;
			}
			return ;

		case 8:
			value = (legacy_u16)regsi[1];
			temp = (legacy_u16)regsi[2];
			if (temp + 0x8000 > USHRT_MAX)
				ofs++;
			temp = (temp + 0x8000) & 0xFFFF;
			var_18[ofs] = value;
			for (i = 0; i < count; i++) {
				if (temp + (legacy_u16)regsi[6] <= USHRT_MAX) {
					value++;
					if (i == count - 1) {
						var_18[480+ofs] = value - 1;
					}
				} else {
					var_18[480+ofs] = value;
					value++;
					ofs++;
					var_18[ofs] = value;
				}
				temp = (temp + (legacy_u16)regsi[6])  & 0xFFFF;
				}
				return ;
			case 9:
			default:
				return ;
	}
}




void preRender_default_impl_helper(const legacy_u16* regsi, legacy_u16 var_A,
	legacy_u16 var_C, legacy_s16* var_18)
{
	const legacy_u16* line = regsi;
	legacy_s16* left_edges = var_18;
	legacy_s16* right_edges = left_edges + 480;
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

	count = LEGACY_S16_FROM_BITS(line[7]);
	index = LEGACY_S16_FROM_BITS(line[3]);
	mode = (legacy_u8)line[9];

	if (count > 0 && mode >= 2U && mode <= 8U) {
		value = LEGACY_S16_FROM_BITS(line[1]);

		if (var_A != 0U) {
			switch (mode) {
			case 2:
			case 3:
			case 4:
				while (count-- > 0) {
					if (right_edges[index] < value)
						right_edges[index] = value;
					else if (left_edges[index] > value)
						left_edges[index] = value;
					index++;
					if (mode == 3U)
						value = LEGACY_S16_WRAP_SUB(value, 1);
					else if (mode == 4U)
						value = LEGACY_S16_WRAP_ADD(value, 1);
				}
				break;

			case 5:
			case 6:
				fixed = ((legacy_u32)(legacy_u16)line[1] << 16) |
					(legacy_u16)line[0];
				fixed += 0x8000UL;
				step = (legacy_u16)line[6];
				while (count-- > 0) {
					value = LEGACY_S16_FROM_BITS(
						(legacy_u16)(fixed >> 16));
					if (right_edges[index] < value)
						right_edges[index] = value;
					else if (left_edges[index] > value)
						left_edges[index] = value;
					index++;
					if (mode == 5U)
						fixed -= step;
					else
						fixed += step;
				}
				break;

			case 7:
				step = (legacy_u16)line[6];
				sum = (legacy_u32)(legacy_u16)line[2] + 0x8000UL;
				fraction = (legacy_u16)sum;
				if (sum > 0xFFFFUL)
					index++;
				if (right_edges[index] < value)
					right_edges[index] = value;
				while (count > 0) {
					sum = (legacy_u32)fraction + step;
					fraction = (legacy_u16)sum;
					carry = sum > 0xFFFFUL;
					if (carry) {
						if (left_edges[index] > value)
							left_edges[index] = value;
						index++;
						value = LEGACY_S16_WRAP_SUB(value, 1);
						count--;
						if (count > 0 &&
							right_edges[index] < value)
							right_edges[index] = value;
					} else {
						value = LEGACY_S16_WRAP_SUB(value, 1);
						count--;
						if (count == 0) {
							value = LEGACY_S16_WRAP_ADD(value, 1);
							if (left_edges[index] > value)
								left_edges[index] = value;
						}
					}
				}
				break;

			case 8:
				step = (legacy_u16)line[6];
				sum = (legacy_u32)(legacy_u16)line[2] + 0x8000UL;
				fraction = (legacy_u16)sum;
				if (sum > 0xFFFFUL)
					index++;
				if (left_edges[index] > value)
					left_edges[index] = value;
				while (count > 0) {
					sum = (legacy_u32)fraction + step;
					fraction = (legacy_u16)sum;
					carry = sum > 0xFFFFUL;
					if (carry) {
						if (right_edges[index] < value)
							right_edges[index] = value;
						index++;
						value = LEGACY_S16_WRAP_ADD(value, 1);
						count--;
						if (count > 0 &&
							left_edges[index] > value)
							left_edges[index] = value;
					} else {
						value = LEGACY_S16_WRAP_ADD(value, 1);
						count--;
						if (count == 0) {
							value = LEGACY_S16_WRAP_SUB(value, 1);
							if (right_edges[index] < value)
								right_edges[index] = value;
						}
					}
				}
				break;
			}
		} else if (mode <= 6U) {
			fixed = ((legacy_u32)(legacy_u16)line[1] << 16) |
				(legacy_u16)line[0];
			fixed += 0x8000UL;
			step = (legacy_u16)line[6];
			target = 0;

			while (count > 0) {
				value = mode >= 5U ?
					LEGACY_S16_FROM_BITS((legacy_u16)(fixed >> 16)) :
					LEGACY_S16_FROM_BITS(line[1]);
				if (mode == 3U)
					value = LEGACY_S16_WRAP_SUB(value,
						LEGACY_S16_WRAP_SUB(
							LEGACY_S16_FROM_BITS(line[7]), count));
				else if (mode == 4U)
					value = LEGACY_S16_WRAP_ADD(value,
						LEGACY_S16_WRAP_SUB(
							LEGACY_S16_FROM_BITS(line[7]), count));

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
				if (mode == 5U)
					fixed -= step;
				else if (mode == 6U)
					fixed += step;
			}
		} else if (mode == 7U) {
			step = (legacy_u16)line[6];
			sum = (legacy_u32)(legacy_u16)line[2] + 0x8000UL;
			fraction = (legacy_u16)sum;
			if (sum > 0xFFFFUL)
				index++;

			while (count > 0) {
				if (right_edges[index] < value) {
					right_edges[index++] = value;
					while (count > 0) {
						value = LEGACY_S16_WRAP_SUB(value, 1);
						sum = (legacy_u32)fraction + step;
						fraction = (legacy_u16)sum;
						count--;
						if (count > 0 && sum > 0xFFFFUL)
							right_edges[index++] = value;
					}
					break;
				}

				sum = (legacy_u32)fraction + step;
				fraction = (legacy_u16)sum;
				carry = sum > 0xFFFFUL;
				if (carry && left_edges[index] > value) {
					left_edges[index++] = value;
					value = LEGACY_S16_WRAP_SUB(value, 1);
					count--;
					while (count > 0) {
						sum = (legacy_u32)fraction + step;
						fraction = (legacy_u16)sum;
						carry = sum > 0xFFFFUL;
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
			step = (legacy_u16)line[6];
			sum = (legacy_u32)(legacy_u16)line[2] + 0x8000UL;
			fraction = (legacy_u16)sum;
			if (sum > 0xFFFFUL)
				index++;

			while (count > 0) {
				if (left_edges[index] > value) {
					left_edges[index++] = value;
					value = LEGACY_S16_WRAP_ADD(value, 1);
					count--;
					while (count > 0) {
						sum = (legacy_u32)fraction + step;
						fraction = (legacy_u16)sum;
						if (sum > 0xFFFFUL)
							left_edges[index++] = value;
						value = LEGACY_S16_WRAP_ADD(value, 1);
						count--;
					}
					break;
				}

				sum = (legacy_u32)fraction + step;
				fraction = (legacy_u16)sum;
				carry = sum > 0xFFFFUL;
				if (carry && right_edges[index] < value) {
					right_edges[index] = value;
					while (count > 0) {
						value = LEGACY_S16_WRAP_ADD(value, 1);
						sum = (legacy_u32)fraction + step;
						fraction = (legacy_u16)sum;
						carry = sum > 0xFFFFUL;
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

	sum = (legacy_u32)(legacy_u16)line[2] + 0x8000UL;
	carry = sum > 0xFFFFUL;

	count = LEGACY_S16_FROM_BITS(line[10]);
	if (count > 0) {
		index = LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_ADD(
			LEGACY_S16_FROM_BITS(line[3]), carry), count);
		while (count-- > 0)
			left_edges[index++] =
				LEGACY_S16_FROM_BITS(sprite1.sprite_left2);
	}

	count = LEGACY_S16_FROM_BITS(line[12]);
	if (count > 0) {
		index = LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_ADD(
			LEGACY_S16_FROM_BITS(line[3]), carry), count);
		while (count-- > 0)
			right_edges[index++] = LEGACY_S16_WRAP_SUB(
				LEGACY_S16_FROM_BITS(sprite1.sprite_widthsum), 1);
	}

	count = LEGACY_S16_FROM_BITS(line[11]);
	if (count > 0) {
		index = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_FROM_BITS(line[5]), 1);
		while (count-- > 0)
			left_edges[index++] =
				LEGACY_S16_FROM_BITS(sprite1.sprite_left2);
	}

	count = LEGACY_S16_FROM_BITS(line[13]);
	if (count > 0) {
		index = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_FROM_BITS(line[5]), 1);
		while (count-- > 0)
			right_edges[index++] = LEGACY_S16_WRAP_SUB(
				LEGACY_S16_FROM_BITS(sprite1.sprite_widthsum), 1);
	}
}
