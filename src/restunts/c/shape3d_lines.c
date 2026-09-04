#include "externs.h"
#include "legacy.h"
#include "shape2d.h"
#include "shape2d_internal.h"
#include "shape3d.h"

legacy_u16 draw_line_related_impl(legacy_u16 start_x, legacy_u16 start_y,
	legacy_u16 end_x, legacy_u16 end_y, legacy_u16* line,
	legacy_u16 alternate);

/* Walk the line's far end forward by `steps` and carry the same correction
   into the span counter the caller names. */
static void draw_line_advance_end(legacy_u16* line, legacy_u16 steps,
	legacy_u16 span_index)
{
	legacy_u32 value32;
	legacy_u16 old_value;

	value32 = ((legacy_u32)line[3] << 16) | line[2];
	value32 += (legacy_u32)steps * line[6];
	value32 += 0x8000UL;
	old_value = line[5];
	line[5] = (legacy_u16)(value32 >> 16);
	line[span_index] = (legacy_u16)(line[span_index] + old_value - line[5]);
}

static legacy_u16 draw_line_round_div(legacy_u32 numerator, legacy_u16 divisor) {
	legacy_u32 quotient;
	legacy_u16 remainder;

	quotient = LEGACY_U32_DIV_OR_ZERO(numerator, divisor);
	remainder = divisor == 0U ? 0U :
		(legacy_u16)(numerator % divisor);
	if ((legacy_u16)(divisor >> 1) < remainder)
		quotient++;
	return (legacy_u16)quotient;
}

static legacy_u16 draw_line_step(legacy_u16 minor, legacy_u16 major) {
	/* The legacy code-segment table contains this truncated quotient for
	 * major values below 50.  Its first shared entry is an otherwise unused
	 * sentinel; retain it for the degenerate calls as well. */
	if (major < 2U)
		return 0x135CU;
	return (legacy_u16)LEGACY_U32_DIV_OR_ZERO(
		(legacy_u32)minor << 16, major);
}

static legacy_u16 draw_line_reject(legacy_u16* line, legacy_u16 reject)
{
	legacy_u16 clip;
	legacy_u16 bottom;
	legacy_u16 top;
	legacy_u32 value32;

	clip = reject & 0x00FFU;
	line[9] = (legacy_u16)((line[9] & 0x00FFU) | (clip << 8));
	line[7] = 0;
	if (clip & 4U) {
		line[3] = sprite1.sprite_top;
		line[2] = 0;
		line[5] = (legacy_u16)(sprite1.sprite_top - 1);
		return clip;
	}
	if (clip & 8U) {
		line[3] = sprite1.sprite_height;
		line[2] = 0;
		return clip;
	}
	bottom = line[5];
	if (LEGACY_S16_FROM_BITS(bottom) >=
		LEGACY_S16_FROM_BITS(sprite1.sprite_height)) {
		bottom = (legacy_u16)(sprite1.sprite_height - 1);
	}
	value32 = (legacy_u32)line[2] + 0x8000UL;
	top = (legacy_u16)(line[3] + (legacy_u16)(value32 >> 16));
	if (LEGACY_S16_FROM_BITS(top) <
		LEGACY_S16_FROM_BITS(sprite1.sprite_top)) {
		top = sprite1.sprite_top;
	}
	line[3] = top;
	line[2] = 0;
	bottom = (legacy_u16)(bottom - top + 1);
	line[5] = (legacy_u16)(top - 1);
	if (clip & 2U)
		line[11] = (legacy_u16)(line[11] + bottom);
	else
		line[13] = (legacy_u16)(line[13] + bottom);
	return clip;
}

static legacy_u16 draw_line_horizontal(
	legacy_u16 y,
	legacy_u16 start_x,
	legacy_u16 end_x,
	legacy_u16* line,
	legacy_u16 unclipped
) {
	legacy_u16 temporary;
	legacy_u16 amount;
	legacy_u16 reject;

	line[9] = (legacy_u16)((line[9] & 0xFF00U) | 1U);
	if (start_x == end_x)
		line[9] = (legacy_u16)((line[9] & 0xFF00U) | 9U);
	if (LEGACY_S16_FROM_BITS(start_x) > LEGACY_S16_FROM_BITS(end_x)) {
		line[9] &= 0xFF00U;
		line[1] = end_x;
		line[4] = start_x;
		temporary = start_x;
		start_x = end_x;
		end_x = temporary;
	}
	if (unclipped != 0) {
		line[7] = (legacy_u16)(end_x - start_x + 1);
		return 0;
	}
	if (LEGACY_S16_FROM_BITS(y) <
		LEGACY_S16_FROM_BITS(sprite1.sprite_top)) {
		line[3] = sprite1.sprite_top;
		line[5] = sprite1.sprite_top;
		reject = 4;
	} else if (LEGACY_S16_FROM_BITS(y) >=
		LEGACY_S16_FROM_BITS(sprite1.sprite_height)) {
		line[3] = sprite1.sprite_height;
		line[5] = sprite1.sprite_height;
		reject = 8;
	} else {
		line[7] = (legacy_u16)(end_x - start_x + 1);
		if (LEGACY_S16_FROM_BITS(end_x) <
			LEGACY_S16_FROM_BITS(sprite1.sprite_left2)) {
			line[5] = (legacy_u16)(line[5] - 1);
			line[11] = 1;
			reject = 2;
		} else if (LEGACY_S16_FROM_BITS(start_x) >=
			LEGACY_S16_FROM_BITS(sprite1.sprite_widthsum)) {
			line[5] = (legacy_u16)(line[5] - 1);
			line[13] = 1;
			reject = 1;
		} else {
			amount = (legacy_u16)(sprite1.sprite_left2 - start_x);
			if (LEGACY_S16_FROM_BITS(amount) > 0) {
				line[1] = sprite1.sprite_left2;
				line[7] = (legacy_u16)(line[7] - amount);
			}
			amount = (legacy_u16)(
				end_x - (sprite1.sprite_widthsum - 1));
			if (LEGACY_S16_FROM_BITS(amount) > 0) {
				line[7] = (legacy_u16)(line[7] - amount);
				line[4] = (legacy_u16)(sprite1.sprite_widthsum - 1);
			}
			return 0;
		}
	}
	line[9] = (legacy_u16)(
		(line[9] & 0x00FFU) | ((reject & 0xFFU) << 8));
	line[7] = 0;
	return reject & 0xFFU;
}

static legacy_u8 draw_line_clip_top(legacy_u16* line)
{
	legacy_u16 ax;
	legacy_u16 cx;
	legacy_u16 mode;
	legacy_u16 advance;
	legacy_u32 value32;
	legacy_u32 product;

	ax = line[3];
	cx = sprite1.sprite_top;
	line[3] = cx;
	cx = (legacy_u16)(cx - ax);
	mode = line[9] & 0x00FFU;
	switch (mode) {
	case 2:
	case 3:
	case 4:
		if (mode == 3)
			line[1] = (legacy_u16)(line[1] - cx);
		else if (mode == 4)
			line[1] = (legacy_u16)(line[1] + cx);
		line[7] = (legacy_u16)(line[7] - cx);
		break;
	case 5:
	case 6:
		product = (legacy_u32)line[6] * cx;
		value32 = ((legacy_u32)line[1] << 16) | line[0];
		if (mode == 5)
			value32 -= product;
		else
			value32 += product;
		line[0] = (legacy_u16)value32;
		line[1] = (legacy_u16)(value32 >> 16);
		line[7] = (legacy_u16)(line[7] - cx);
		break;
	case 7:
	case 8:
		line[3] = ax;
		advance = draw_line_round_div((legacy_u32)cx << 16, line[6]);
		if (mode == 7)
			line[1] = (legacy_u16)(line[1] - advance);
		else
			line[1] = (legacy_u16)(line[1] + advance);
		line[7] = (legacy_u16)(line[7] - advance);
		if (LEGACY_S16_FROM_BITS(line[7]) <= 0) {
			line[7] = 1;
			line[3] = sprite1.sprite_top;
			line[1] = line[4];
		} else {
			product = (legacy_u32)advance * line[6];
			value32 = ((legacy_u32)line[3] << 16) | line[2];
			value32 += product;
			line[2] = (legacy_u16)value32;
			line[3] = (legacy_u16)(value32 >> 16);
		}
		break;
	default:
		return 0;
	}
	return 1;
}

static legacy_u8 draw_line_clip_bottom(legacy_u16* line)
{
	legacy_u16 cx;
	legacy_u16 dx;
	legacy_u16 mode;
	legacy_u16 advance;
	legacy_u32 value32;
	legacy_u32 product;

	cx = line[5];
	dx = (legacy_u16)(sprite1.sprite_height - 1);
	line[5] = dx;
	cx = (legacy_u16)(cx - dx);
	mode = line[9] & 0x00FFU;
	switch (mode) {
	case 2:
	case 3:
	case 4:
		if (mode == 3)
			line[4] = (legacy_u16)(line[4] + cx);
		else if (mode == 4)
			line[4] = (legacy_u16)(line[4] - cx);
		line[7] = (legacy_u16)(line[7] - cx);
		break;
	case 5:
	case 6:
		line[7] = (legacy_u16)(line[7] - cx);
		advance = (legacy_u16)(line[7] - 1);
		product = (legacy_u32)line[6] * advance;
		value32 = ((legacy_u32)line[1] << 16) | line[0];
		if (mode == 5)
			value32 -= product;
		else
			value32 += product;
		value32 += 0x8000UL;
		line[4] = (legacy_u16)(value32 >> 16);
		break;
	case 7:
	case 8:
		value32 = ((legacy_u32)dx << 16) -
			(((legacy_u32)line[3] << 16) | line[2]);
		if ((value32 & 0x80000000UL) != 0)
			advance = 0;
		else
			advance = draw_line_round_div(value32, line[6]);
		if (mode == 7)
			line[4] = (legacy_u16)(line[1] - advance);
		else
			line[4] = (legacy_u16)(line[1] + advance);
		line[7] = (legacy_u16)(advance + 1);
		break;
	default:
		return 0;
	}
	return 1;
}

static void draw_line_advance_secondary(legacy_u16* line,
	legacy_u16 advance, legacy_u16 counter_index)
{
	legacy_u16 old_value;
	legacy_u16 new_value;
	legacy_u32 value32;

	value32 = ((legacy_u32)line[3] << 16) | line[2];
	old_value = (legacy_u16)((value32 + 0x8000UL) >> 16);
	value32 += (legacy_u32)advance * line[6];
	line[2] = (legacy_u16)value32;
	line[3] = (legacy_u16)(value32 >> 16);
	new_value = (legacy_u16)((value32 + 0x8000UL) >> 16);
	line[counter_index] = (legacy_u16)(
		line[counter_index] + new_value - old_value);
}

static legacy_s16 draw_line_clip_left(legacy_u16* line)
{
	legacy_u16 ax;
	legacy_u16 cx;
	legacy_u16 dx;
	legacy_u16 mode;
	legacy_u16 old_value;
	legacy_u16 advance;
	legacy_u16 original_count;
	legacy_u32 value32;
	legacy_u32 product;
	legacy_s32 difference;

	mode = line[9] & 0x00FFU;
	switch (mode) {
	case 2:
		return 2;
	case 3:
		cx = sprite1.sprite_left2;
		ax = line[4];
		line[4] = cx;
		cx = (legacy_u16)(cx - ax);
		line[11] = (legacy_u16)(line[11] + cx);
		line[7] = (legacy_u16)(line[7] - cx);
		line[5] = (legacy_u16)(line[5] - cx);
		break;
	case 4:
		ax = line[1];
		cx = sprite1.sprite_left2;
		line[1] = cx;
		cx = (legacy_u16)(cx - ax);
		line[10] = (legacy_u16)(line[10] + cx);
		line[3] = (legacy_u16)(line[3] + cx);
		line[7] = (legacy_u16)(line[7] - cx);
		break;
	case 5:
		value32 = ((legacy_u32)line[1] << 16) | line[0];
		line[4] = sprite1.sprite_left2;
		difference = (legacy_s32)LEGACY_S16_FROM_BITS(line[1]) -
			(legacy_s32)LEGACY_S16_FROM_BITS(sprite1.sprite_left2);
		if (difference < 0) {
			advance = 1;
		} else {
			advance = draw_line_round_div(value32 -
				((legacy_u32)sprite1.sprite_left2 << 16), line[6]);
			advance = (legacy_u16)(advance + 1);
		}
		original_count = line[7];
		line[7] = advance;
		line[11] = (legacy_u16)(
			line[11] + original_count - advance);
		line[5] = (legacy_u16)(line[3] + advance - 1);
		break;
	case 6:
		value32 = ((legacy_u32)sprite1.sprite_left2 << 16) -
			(((legacy_u32)line[1] << 16) | line[0]);
		if ((value32 & 0x80000000UL) != 0)
			return 2;
		advance = draw_line_round_div(value32, line[6]);
		line[3] = (legacy_u16)(line[3] + advance);
		line[10] = (legacy_u16)(line[10] + advance);
		line[7] = (legacy_u16)(line[7] - advance);
		if (LEGACY_S16_FROM_BITS(line[7]) <= 0)
			return 2;
		product = (legacy_u32)advance * line[6];
		value32 = ((legacy_u32)line[1] << 16) | line[0];
		value32 += product;
		line[0] = (legacy_u16)value32;
		line[1] = (legacy_u16)(value32 >> 16);
		break;
	case 7:
		ax = line[1];
		dx = sprite1.sprite_left2;
		line[4] = dx;
		ax = (legacy_u16)(ax - dx);
		advance = (legacy_u16)(ax + 1);
		line[7] = advance;
		draw_line_advance_end(line, ax, 11);
		break;
	case 8:
		old_value = line[1];
		line[1] = sprite1.sprite_left2;
		advance = (legacy_u16)(line[1] - old_value);
		line[7] = (legacy_u16)(line[7] - advance);
		draw_line_advance_secondary(line, advance, 10U);
		break;
	default:
		return 0;
	}
	return 1;
}

static legacy_u16 draw_line_clip_right(legacy_u16* line)
{
	legacy_u16 ax;
	legacy_u16 cx;
	legacy_u16 mode;
	legacy_u16 old_value;
	legacy_u16 advance;
	legacy_u16 original_count;
	legacy_u32 value32;
	legacy_u32 product;

	mode = line[9] & 0x00FFU;
	switch (mode) {
	case 2:
		return draw_line_reject(line, 1U);
	case 3:
		cx = line[1];
		ax = (legacy_u16)(sprite1.sprite_widthsum - 1);
		line[1] = ax;
		cx = (legacy_u16)(cx - ax);
		line[3] = (legacy_u16)(line[3] + cx);
		line[7] = (legacy_u16)(line[7] - cx);
		line[12] = (legacy_u16)(line[12] + cx);
		return 0;
	case 4:
		cx = (legacy_u16)(sprite1.sprite_widthsum - 1);
		ax = line[4];
		line[4] = cx;
		ax = (legacy_u16)(ax - cx);
		line[13] = (legacy_u16)(line[13] + ax);
		line[7] = (legacy_u16)(line[7] - ax);
		line[5] = (legacy_u16)(line[5] - ax);
		return 0;
	case 5:
		value32 = ((legacy_u32)line[1] << 16) | line[0];
		value32 -= (legacy_u32)(sprite1.sprite_widthsum - 1) << 16;
		advance = draw_line_round_div(value32, line[6]);
		line[7] = (legacy_u16)(line[7] - advance);
		if (LEGACY_S16_FROM_BITS(line[7]) <= 0)
			return draw_line_reject(line, 1U);
		line[3] = (legacy_u16)(line[3] + advance);
		line[12] = (legacy_u16)(line[12] + advance);
		product = (legacy_u32)advance * line[6];
		value32 = ((legacy_u32)line[1] << 16) | line[0];
		value32 -= product;
		line[0] = (legacy_u16)value32;
		line[1] = (legacy_u16)(value32 >> 16);
		return 0;
	case 6:
		line[4] = (legacy_u16)(sprite1.sprite_widthsum - 1);
		value32 = ((legacy_u32)line[4] << 16) -
			(((legacy_u32)line[1] << 16) | line[0]);
		if ((value32 & 0x80000000UL) != 0)
			return draw_line_reject(line, 1U);
		advance = (legacy_u16)(
			draw_line_round_div(value32, line[6]) + 1);
		original_count = line[7];
		line[7] = advance;
		line[13] = (legacy_u16)(
			line[13] + original_count - advance);
		line[5] = (legacy_u16)(line[3] + advance - 1);
		return 0;
	case 7:
		ax = line[1];
		cx = (legacy_u16)(sprite1.sprite_widthsum - 1);
		ax = (legacy_u16)(ax - cx);
		line[1] = cx;
		line[7] = (legacy_u16)(line[7] - ax);
		draw_line_advance_secondary(line, ax, 12U);
		return 0;
	case 8:
		ax = sprite1.sprite_widthsum;
		cx = (legacy_u16)(ax - 1);
		line[4] = cx;
		ax = (legacy_u16)(ax - line[1]);
		line[7] = ax;
		advance = (legacy_u16)(ax - 1);
		draw_line_advance_end(line, advance, 13);
		return 0;
	default:
		return 0;
	}
}

static void draw_line_subdivide_advance_start(legacy_u16* line,
	legacy_u16 vertical_step, legacy_u16 horizontal_step)
{
	legacy_u16 ax;
	legacy_u16 bx;
	legacy_u8 update_counter;

	ax = (legacy_u16)(line[3] + vertical_step);
	line[3] = ax;
	bx = ax;
	ax = (legacy_u16)(ax - sprite1.sprite_top);
	if (LEGACY_S16_FROM_BITS(ax) > 0) {
		if (LEGACY_S16_FROM_BITS(ax) >
			LEGACY_S16_FROM_BITS(vertical_step))
			ax = vertical_step;
		update_counter = 1;
		bx = (legacy_u16)(bx - sprite1.sprite_height);
		if (LEGACY_S16_FROM_BITS(bx) > 0) {
			ax = (legacy_u16)(ax - bx);
			if (LEGACY_S16_FROM_BITS(ax) <= 0)
				update_counter = 0;
		}
		if (update_counter != 0) {
			if (LEGACY_S16_FROM_BITS(line[1]) <
				LEGACY_S16_FROM_BITS(sprite1.sprite_left2)) {
				line[10] = (legacy_u16)(line[10] + ax);
			} else {
				line[12] = (legacy_u16)(line[12] + ax);
			}
		}
	}
	line[1] = (legacy_u16)(line[1] + horizontal_step);
}

static void draw_line_subdivide_advance_end(legacy_u16* line,
	legacy_u16 vertical_step, legacy_u16 horizontal_step)
{
	legacy_u16 ax;
	legacy_u16 bx;
	legacy_u8 update_counter;

	ax = (legacy_u16)(line[5] - vertical_step);
	line[5] = ax;
	bx = ax;
	ax = (legacy_u16)(ax - sprite1.sprite_height + 1);
	if (LEGACY_S16_FROM_BITS(ax) < 0) {
		ax = (legacy_u16)(0U - ax);
		if (LEGACY_S16_FROM_BITS(ax) >
			LEGACY_S16_FROM_BITS(vertical_step))
			ax = vertical_step;
		update_counter = 1;
		bx = (legacy_u16)(bx - sprite1.sprite_top + 1);
		if (LEGACY_S16_FROM_BITS(bx) < 0) {
			ax = (legacy_u16)(ax + bx);
			if (LEGACY_S16_FROM_BITS(ax) <= 0)
				update_counter = 0;
		}
		if (update_counter != 0) {
			if (LEGACY_S16_FROM_BITS(line[4]) <
				LEGACY_S16_FROM_BITS(sprite1.sprite_left2)) {
				line[11] = (legacy_u16)(line[11] + ax);
			} else {
				line[13] = (legacy_u16)(line[13] + ax);
			}
		}
	}
	line[4] = (legacy_u16)(line[4] - horizontal_step);
}

static void draw_line_subdivide(legacy_u16* line)
{
	legacy_u16 ax;
	legacy_u16 cx;
	legacy_u16 dx;

	cx = sar1_word(line[5]);
	ax = sar1_word(line[3]);
	cx = sar1_word((legacy_u16)(cx - ax));
	dx = sar1_word(line[4]);
	ax = sar1_word(line[1]);
	dx = sar1_word((legacy_u16)(dx - ax));

	for (;;) {
		while (LEGACY_S16_FROM_BITS(line[3]) <=
			LEGACY_S16_FROM_BITS(0xC180U)) {
			draw_line_subdivide_advance_start(line, cx, dx);
		}

		if (LEGACY_S16_FROM_BITS(line[5]) >=
			LEGACY_S16_FROM_BITS(0x3E80U)) {
			draw_line_subdivide_advance_end(line, cx, dx);
			continue;
		}
		if (LEGACY_S16_FROM_BITS(line[1]) <=
			LEGACY_S16_FROM_BITS(0xC180U) ||
			LEGACY_S16_FROM_BITS(line[1]) >=
			LEGACY_S16_FROM_BITS(0x3E80U)) {
			draw_line_subdivide_advance_start(line, cx, dx);
			continue;
		}
		if (LEGACY_S16_FROM_BITS(line[4]) <=
			LEGACY_S16_FROM_BITS(0xC180U) ||
			LEGACY_S16_FROM_BITS(line[4]) >=
			LEGACY_S16_FROM_BITS(0x3E80U)) {
			draw_line_subdivide_advance_end(line, cx, dx);
			continue;
		}
		return;
	}
}

legacy_u16 draw_line_related(legacy_u16 arg_startX, legacy_u16 arg_startY, legacy_u16 arg_endX, legacy_u16 arg_endY, legacy_u16* line) {
	return draw_line_related_impl(arg_startX, arg_startY, arg_endX, arg_endY, line, 0);
}

legacy_u16 draw_line_related_alt(legacy_u16 arg_startX, legacy_u16 arg_startY, legacy_u16 arg_endX, legacy_u16 arg_endY, legacy_u16* line) {
	return draw_line_related_impl(arg_startX, arg_startY, arg_endX, arg_endY, line, 1);
}

legacy_u16 draw_line_related_impl(legacy_u16 arg_startX, legacy_u16 arg_startY, legacy_u16 arg_endX, legacy_u16 arg_endY, legacy_u16* line, legacy_u16 var_4) {
	legacy_u16 ax;
	legacy_u16 bx;
	legacy_u16 cx;
	legacy_u16 dx;
	legacy_u16 mode;
	legacy_u16 clip;
	legacy_u16 old_value;
	legacy_u16 advance;
	legacy_u16 original_count;
	legacy_u32 value32;
	legacy_u32 product;
	legacy_s32 difference;
	legacy_s16 clip_result;
	legacy_u8 compute_step;
	legacy_u8 subdivide_required;

	line[9] = 0x00FFU;
	line[0] = 0;
	line[2] = 0;
	line[10] = 0;
	line[11] = 0;
	line[12] = 0;
	line[13] = 0;

	ax = (legacy_u16)arg_startY;
	bx = (legacy_u16)arg_endY;
	cx = (legacy_u16)arg_startX;
	dx = (legacy_u16)arg_endX;
	if (LEGACY_S16_FROM_BITS(ax) <= LEGACY_S16_FROM_BITS(bx)) {
		line[1] = cx;
		line[3] = ax;
		line[4] = dx;
		line[5] = bx;
	} else {
		line[1] = dx;
		line[3] = bx;
		line[4] = cx;
		line[5] = ax;
	}
	if (ax == bx)
		return draw_line_horizontal(ax, cx, dx, line, var_4);

	for (;;) {
	dx = 0;
	if ((legacy_u16)var_4 == 0) {
		ax = line[3];
		bx = sprite1.sprite_top;
		cx = sprite1.sprite_height;
		if (LEGACY_S16_FROM_BITS(ax) >= LEGACY_S16_FROM_BITS(cx)) {
			dx = 8;
			return draw_line_reject(line, dx);
		}
		if (LEGACY_S16_FROM_BITS(ax) < LEGACY_S16_FROM_BITS(bx))
			dx |= 0x0400U;

		ax = line[5];
		if (LEGACY_S16_FROM_BITS(ax) < LEGACY_S16_FROM_BITS(bx)) {
			dx = 4;
			return draw_line_reject(line, dx);
		}
		if (LEGACY_S16_FROM_BITS(ax) >= LEGACY_S16_FROM_BITS(cx))
			dx |= 8;

		bx = sprite1.sprite_left2;
		cx = sprite1.sprite_widthsum;
		ax = line[1];
		if (LEGACY_S16_FROM_BITS(ax) < LEGACY_S16_FROM_BITS(bx))
			dx |= 0x0200U;
		if (LEGACY_S16_FROM_BITS(ax) >= LEGACY_S16_FROM_BITS(cx))
			dx |= 0x0100U;
		ax = line[4];
		if (LEGACY_S16_FROM_BITS(ax) < LEGACY_S16_FROM_BITS(bx))
			dx |= 2;
		if (LEGACY_S16_FROM_BITS(ax) >= LEGACY_S16_FROM_BITS(cx))
			dx |= 1;
		if ((legacy_u8)dx & (legacy_u8)(dx >> 8)) {
			dx = (legacy_u8)dx & (legacy_u8)(dx >> 8);
			return draw_line_reject(line, dx);
		}
	}

	dx = (legacy_u16)((legacy_u8)dx | (legacy_u8)(dx >> 8));
	clip = dx;
	subdivide_required = 0;
	difference = (legacy_s32)LEGACY_S16_FROM_BITS(line[5]) -
		(legacy_s32)LEGACY_S16_FROM_BITS(line[3]);
	if (difference < -32768L || difference > 32767L) {
		subdivide_required = 1;
	} else {
		cx = (legacy_u16)difference;
		difference = (legacy_s32)LEGACY_S16_FROM_BITS(line[4]) -
			(legacy_s32)LEGACY_S16_FROM_BITS(line[1]);
		if (difference < -32768L || difference > 32767L) {
			subdivide_required = 1;
		} else {
			dx = (legacy_u16)difference;
			compute_step = 0;
			if (dx == 0) {
				cx = (legacy_u16)(cx + 1);
				line[7] = cx;
				line[9] = (legacy_u16)(
					(line[9] & 0xFF00U) | 2U);
			} else if (LEGACY_S16_FROM_BITS(dx) >= 0) {
				if (dx < cx) {
					line[9] = (legacy_u16)(
						(line[9] & 0xFF00U) | 6U);
					compute_step = 1;
				} else if (dx == cx) {
					line[9] = (legacy_u16)(
						(line[9] & 0xFF00U) | 4U);
					line[7] = (legacy_u16)(cx + 1);
				} else {
					line[9] = (legacy_u16)(
						(line[9] & 0xFF00U) | 8U);
					bx = cx;
					cx = dx;
					dx = bx;
					compute_step = 1;
				}
			} else if (dx == 0x8000U) {
				subdivide_required = 1;
			} else {
				dx = (legacy_u16)(0U - dx);
				if (dx < cx) {
					line[9] = (legacy_u16)(
						(line[9] & 0xFF00U) | 5U);
					compute_step = 1;
				} else if (dx == cx) {
					line[9] = (legacy_u16)(
						(line[9] & 0xFF00U) | 3U);
					line[7] = (legacy_u16)(cx + 1);
				} else {
					line[9] = (legacy_u16)(
						(line[9] & 0xFF00U) | 7U);
					bx = cx;
					cx = dx;
					dx = bx;
					compute_step = 1;
				}
			}
			if (compute_step != 0) {
				line[6] = draw_line_step(dx, cx);
				if (cx == 0x7FFFU)
					subdivide_required = 1;
				else
					line[7] = (legacy_u16)(cx + 1);
			}
		}
	}

	if (subdivide_required != 0) {
		draw_line_subdivide(line);
		continue;
	}

	for (;;) {
		switch (clip & 0x0FU) {
		case 0:
			return 0;
		case 1:
			return draw_line_clip_right(line);
		case 2:
		case 3:
			clip_result = draw_line_clip_left(line);
			if (clip_result == 2)
				return draw_line_reject(line, 2U);
			if (clip_result == 0)
				return 0;
			if (clip & 1U)
				return draw_line_clip_right(line);
			return 0;
		case 8:
		case 9:
		case 10:
		case 11:
			if (draw_line_clip_bottom(line) == 0)
				return 0;
			break;
		default:
			if (draw_line_clip_top(line) == 0)
				return 0;
			if ((clip & 8U) != 0 &&
				draw_line_clip_bottom(line) == 0) {
				return 0;
			}
			break;
		}

		dx = 0;
		ax = line[1];
		if (LEGACY_S16_FROM_BITS(ax) <
			LEGACY_S16_FROM_BITS(sprite1.sprite_left2)) {
			dx |= 0x0200U;
		}
		value32 = (legacy_u32)line[0] + 0x8000UL;
		ax = (legacy_u16)(ax + (legacy_u16)(value32 >> 16));
		if (LEGACY_S16_FROM_BITS(ax) >=
			LEGACY_S16_FROM_BITS(sprite1.sprite_widthsum)) {
			dx |= 0x0100U;
		}
		ax = line[4];
		if (LEGACY_S16_FROM_BITS(ax) <
			LEGACY_S16_FROM_BITS(sprite1.sprite_left2)) {
			dx |= 2;
		}
		if (LEGACY_S16_FROM_BITS(ax) >=
			LEGACY_S16_FROM_BITS(sprite1.sprite_widthsum)) {
			dx |= 1;
		}
		if ((legacy_u8)dx & (legacy_u8)(dx >> 8)) {
			dx = (legacy_u8)dx & (legacy_u8)(dx >> 8);
			return draw_line_reject(line, dx);
		}
		dx = (legacy_u16)((legacy_u8)dx | (legacy_u8)(dx >> 8));
		if (dx == 0)
			return 0;
		clip = dx;
	}
	}
}
