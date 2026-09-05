#include "externs.h"
#include "legacy.h"
#include "shape2d.h"
#include "shape2d_internal.h"
#include "shape3d.h"

#define DRAW_LINE_FIXED_ROUNDING 32768UL
#define DRAW_LINE_DEGENERATE_STEP 4956U
#define DRAW_LINE_MODE_MASK LEGACY_U8_MAX
#define DRAW_LINE_CLIP_SHIFT LEGACY_BYTE_BITS

#define DRAW_LINE_CLIP_RIGHT 1U
#define DRAW_LINE_CLIP_LEFT 2U
#define DRAW_LINE_CLIP_TOP 4U
#define DRAW_LINE_CLIP_BOTTOM 8U
#define DRAW_LINE_CLIP_MASK 15U

#define DRAW_LINE_MODE_HORIZONTAL 1U
#define DRAW_LINE_MODE_VERTICAL 2U
#define DRAW_LINE_MODE_DIAGONAL_LEFT 3U
#define DRAW_LINE_MODE_DIAGONAL_RIGHT 4U
#define DRAW_LINE_MODE_Y_MAJOR_LEFT 5U
#define DRAW_LINE_MODE_Y_MAJOR_RIGHT 6U
#define DRAW_LINE_MODE_X_MAJOR_LEFT 7U
#define DRAW_LINE_MODE_X_MAJOR_RIGHT 8U
#define DRAW_LINE_MODE_POINT 9U
#define DRAW_LINE_MODE_UNSET LEGACY_U8_MAX

#define DRAW_LINE_SUBDIVIDE_MIN (-16000)
#define DRAW_LINE_SUBDIVIDE_MAX 16000

#define DRAW_LINE_START_X_FRACTION_INDEX 0
#define DRAW_LINE_START_X_INDEX 1
#define DRAW_LINE_START_Y_FRACTION_INDEX 2
#define DRAW_LINE_START_Y_INDEX 3
#define DRAW_LINE_END_X_INDEX 4
#define DRAW_LINE_END_Y_INDEX 5
#define DRAW_LINE_STEP_INDEX 6
#define DRAW_LINE_PIXEL_COUNT_INDEX 7
#define DRAW_LINE_COLOR_INDEX 8
#define DRAW_LINE_MODE_AND_CLIP_INDEX 9
#define DRAW_LINE_START_LEFT_CLIP_COUNT_INDEX 10
#define DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX 11
#define DRAW_LINE_START_RIGHT_CLIP_COUNT_INDEX 12
#define DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX 13

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

	value32 = ((legacy_u32)line[DRAW_LINE_START_Y_INDEX] << LEGACY_WORD_BITS) | line[DRAW_LINE_START_Y_FRACTION_INDEX];
	value32 += (legacy_u32)steps * line[DRAW_LINE_STEP_INDEX];
	value32 += DRAW_LINE_FIXED_ROUNDING;
	old_value = line[DRAW_LINE_END_Y_INDEX];
	line[DRAW_LINE_END_Y_INDEX] = (legacy_u16)(value32 >> LEGACY_WORD_BITS);
	line[span_index] = (legacy_u16)(line[span_index] + old_value - line[DRAW_LINE_END_Y_INDEX]);
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
		return DRAW_LINE_DEGENERATE_STEP;
	return (legacy_u16)LEGACY_U32_DIV_OR_ZERO(
		(legacy_u32)minor << LEGACY_WORD_BITS, major);
}

static legacy_u16 draw_line_reject(legacy_u16* line, legacy_u16 reject)
{
	legacy_u16 clip;
	legacy_u16 bottom;
	legacy_u16 top;
	legacy_u32 value32;

	clip = reject & DRAW_LINE_MODE_MASK;
	line[DRAW_LINE_MODE_AND_CLIP_INDEX] = (legacy_u16)((line[DRAW_LINE_MODE_AND_CLIP_INDEX] & DRAW_LINE_MODE_MASK) |
		(clip << DRAW_LINE_CLIP_SHIFT));
	line[DRAW_LINE_PIXEL_COUNT_INDEX] = 0;
	if (clip & DRAW_LINE_CLIP_TOP) {
		line[DRAW_LINE_START_Y_INDEX] = sprite1.sprite_top;
		line[DRAW_LINE_START_Y_FRACTION_INDEX] = 0;
		line[DRAW_LINE_END_Y_INDEX] = (legacy_u16)(sprite1.sprite_top - 1);
		return clip;
	}
	if (clip & DRAW_LINE_CLIP_BOTTOM) {
		line[DRAW_LINE_START_Y_INDEX] = sprite1.sprite_height;
		line[DRAW_LINE_START_Y_FRACTION_INDEX] = 0;
		return clip;
	}
	bottom = line[DRAW_LINE_END_Y_INDEX];
	if (LEGACY_S16_FROM_BITS(bottom) >=
		LEGACY_S16_FROM_BITS(sprite1.sprite_height)) {
		bottom = (legacy_u16)(sprite1.sprite_height - 1);
	}
	value32 = (legacy_u32)line[DRAW_LINE_START_Y_FRACTION_INDEX] + DRAW_LINE_FIXED_ROUNDING;
	top = (legacy_u16)(line[DRAW_LINE_START_Y_INDEX] +
		(legacy_u16)(value32 >> LEGACY_WORD_BITS));
	if (LEGACY_S16_FROM_BITS(top) <
		LEGACY_S16_FROM_BITS(sprite1.sprite_top)) {
		top = sprite1.sprite_top;
	}
	line[DRAW_LINE_START_Y_INDEX] = top;
	line[DRAW_LINE_START_Y_FRACTION_INDEX] = 0;
	bottom = (legacy_u16)(bottom - top + 1);
	line[DRAW_LINE_END_Y_INDEX] = (legacy_u16)(top - 1);
	if (clip & DRAW_LINE_CLIP_LEFT)
		line[DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX] + bottom);
	else
		line[DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX] + bottom);
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

	line[DRAW_LINE_MODE_AND_CLIP_INDEX] = (legacy_u16)((line[DRAW_LINE_MODE_AND_CLIP_INDEX] & LEGACY_U16_HIGH_BYTE_MASK) |
		DRAW_LINE_MODE_HORIZONTAL);
	if (start_x == end_x)
		line[DRAW_LINE_MODE_AND_CLIP_INDEX] = (legacy_u16)((line[DRAW_LINE_MODE_AND_CLIP_INDEX] &
			LEGACY_U16_HIGH_BYTE_MASK) | DRAW_LINE_MODE_POINT);
	if (LEGACY_S16_FROM_BITS(start_x) > LEGACY_S16_FROM_BITS(end_x)) {
		line[DRAW_LINE_MODE_AND_CLIP_INDEX] &= LEGACY_U16_HIGH_BYTE_MASK;
		line[DRAW_LINE_START_X_INDEX] = end_x;
		line[DRAW_LINE_END_X_INDEX] = start_x;
		temporary = start_x;
		start_x = end_x;
		end_x = temporary;
	}
	if (unclipped != 0) {
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(end_x - start_x + 1);
		return 0;
	}
	if (LEGACY_S16_FROM_BITS(y) <
		LEGACY_S16_FROM_BITS(sprite1.sprite_top)) {
		line[DRAW_LINE_START_Y_INDEX] = sprite1.sprite_top;
		line[DRAW_LINE_END_Y_INDEX] = sprite1.sprite_top;
		reject = DRAW_LINE_CLIP_TOP;
	} else if (LEGACY_S16_FROM_BITS(y) >=
		LEGACY_S16_FROM_BITS(sprite1.sprite_height)) {
		line[DRAW_LINE_START_Y_INDEX] = sprite1.sprite_height;
		line[DRAW_LINE_END_Y_INDEX] = sprite1.sprite_height;
		reject = DRAW_LINE_CLIP_BOTTOM;
	} else {
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(end_x - start_x + 1);
		if (LEGACY_S16_FROM_BITS(end_x) <
			LEGACY_S16_FROM_BITS(sprite1.sprite_left2)) {
			line[DRAW_LINE_END_Y_INDEX] = (legacy_u16)(line[DRAW_LINE_END_Y_INDEX] - 1);
			line[DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX] = 1;
			reject = DRAW_LINE_CLIP_LEFT;
		} else if (LEGACY_S16_FROM_BITS(start_x) >=
			LEGACY_S16_FROM_BITS(sprite1.sprite_widthsum)) {
			line[DRAW_LINE_END_Y_INDEX] = (legacy_u16)(line[DRAW_LINE_END_Y_INDEX] - 1);
			line[DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX] = 1;
			reject = DRAW_LINE_CLIP_RIGHT;
		} else {
			amount = (legacy_u16)(sprite1.sprite_left2 - start_x);
			if (LEGACY_S16_FROM_BITS(amount) > 0) {
				line[DRAW_LINE_START_X_INDEX] = sprite1.sprite_left2;
				line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - amount);
			}
			amount = (legacy_u16)(
				end_x - (sprite1.sprite_widthsum - 1));
			if (LEGACY_S16_FROM_BITS(amount) > 0) {
				line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - amount);
				line[DRAW_LINE_END_X_INDEX] = (legacy_u16)(sprite1.sprite_widthsum - 1);
			}
			return 0;
		}
	}
	line[DRAW_LINE_MODE_AND_CLIP_INDEX] = (legacy_u16)((line[DRAW_LINE_MODE_AND_CLIP_INDEX] & DRAW_LINE_MODE_MASK) |
		((reject & DRAW_LINE_MODE_MASK) << DRAW_LINE_CLIP_SHIFT));
	line[DRAW_LINE_PIXEL_COUNT_INDEX] = 0;
	return reject & DRAW_LINE_MODE_MASK;
}

static legacy_u8 draw_line_clip_top(legacy_u16* line)
{
	legacy_u16 ax;
	legacy_u16 cx;
	legacy_u16 mode;
	legacy_u16 advance;
	legacy_u32 value32;
	legacy_u32 product;

	ax = line[DRAW_LINE_START_Y_INDEX];
	cx = sprite1.sprite_top;
	line[DRAW_LINE_START_Y_INDEX] = cx;
	cx = (legacy_u16)(cx - ax);
	mode = line[DRAW_LINE_MODE_AND_CLIP_INDEX] & DRAW_LINE_MODE_MASK;
	switch (mode) {
	case DRAW_LINE_MODE_VERTICAL:
	case DRAW_LINE_MODE_DIAGONAL_LEFT:
	case DRAW_LINE_MODE_DIAGONAL_RIGHT:
		if (mode == DRAW_LINE_MODE_DIAGONAL_LEFT)
			line[DRAW_LINE_START_X_INDEX] = (legacy_u16)(line[DRAW_LINE_START_X_INDEX] - cx);
		else if (mode == DRAW_LINE_MODE_DIAGONAL_RIGHT)
			line[DRAW_LINE_START_X_INDEX] = (legacy_u16)(line[DRAW_LINE_START_X_INDEX] + cx);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - cx);
		break;
	case DRAW_LINE_MODE_Y_MAJOR_LEFT:
	case DRAW_LINE_MODE_Y_MAJOR_RIGHT:
		product = (legacy_u32)line[DRAW_LINE_STEP_INDEX] * cx;
		value32 = ((legacy_u32)line[DRAW_LINE_START_X_INDEX] << LEGACY_WORD_BITS) | line[DRAW_LINE_START_X_FRACTION_INDEX];
		if (mode == DRAW_LINE_MODE_Y_MAJOR_LEFT)
			value32 -= product;
		else
			value32 += product;
		line[DRAW_LINE_START_X_FRACTION_INDEX] = (legacy_u16)value32;
		line[DRAW_LINE_START_X_INDEX] = (legacy_u16)(value32 >> LEGACY_WORD_BITS);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - cx);
		break;
	case DRAW_LINE_MODE_X_MAJOR_LEFT:
	case DRAW_LINE_MODE_X_MAJOR_RIGHT:
		line[DRAW_LINE_START_Y_INDEX] = ax;
		advance = draw_line_round_div(
			(legacy_u32)cx << LEGACY_WORD_BITS, line[DRAW_LINE_STEP_INDEX]);
		if (mode == DRAW_LINE_MODE_X_MAJOR_LEFT)
			line[DRAW_LINE_START_X_INDEX] = (legacy_u16)(line[DRAW_LINE_START_X_INDEX] - advance);
		else
			line[DRAW_LINE_START_X_INDEX] = (legacy_u16)(line[DRAW_LINE_START_X_INDEX] + advance);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - advance);
		if (LEGACY_S16_FROM_BITS(line[DRAW_LINE_PIXEL_COUNT_INDEX]) <= 0) {
			line[DRAW_LINE_PIXEL_COUNT_INDEX] = 1;
			line[DRAW_LINE_START_Y_INDEX] = sprite1.sprite_top;
			line[DRAW_LINE_START_X_INDEX] = line[DRAW_LINE_END_X_INDEX];
		} else {
			product = (legacy_u32)advance * line[DRAW_LINE_STEP_INDEX];
			value32 = ((legacy_u32)line[DRAW_LINE_START_Y_INDEX] << LEGACY_WORD_BITS) |
				line[DRAW_LINE_START_Y_FRACTION_INDEX];
			value32 += product;
			line[DRAW_LINE_START_Y_FRACTION_INDEX] = (legacy_u16)value32;
			line[DRAW_LINE_START_Y_INDEX] = (legacy_u16)(value32 >> LEGACY_WORD_BITS);
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

	cx = line[DRAW_LINE_END_Y_INDEX];
	dx = (legacy_u16)(sprite1.sprite_height - 1);
	line[DRAW_LINE_END_Y_INDEX] = dx;
	cx = (legacy_u16)(cx - dx);
	mode = line[DRAW_LINE_MODE_AND_CLIP_INDEX] & DRAW_LINE_MODE_MASK;
	switch (mode) {
	case DRAW_LINE_MODE_VERTICAL:
	case DRAW_LINE_MODE_DIAGONAL_LEFT:
	case DRAW_LINE_MODE_DIAGONAL_RIGHT:
		if (mode == DRAW_LINE_MODE_DIAGONAL_LEFT)
			line[DRAW_LINE_END_X_INDEX] = (legacy_u16)(line[DRAW_LINE_END_X_INDEX] + cx);
		else if (mode == DRAW_LINE_MODE_DIAGONAL_RIGHT)
			line[DRAW_LINE_END_X_INDEX] = (legacy_u16)(line[DRAW_LINE_END_X_INDEX] - cx);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - cx);
		break;
	case DRAW_LINE_MODE_Y_MAJOR_LEFT:
	case DRAW_LINE_MODE_Y_MAJOR_RIGHT:
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - cx);
		advance = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - 1);
		product = (legacy_u32)line[DRAW_LINE_STEP_INDEX] * advance;
		value32 = ((legacy_u32)line[DRAW_LINE_START_X_INDEX] << LEGACY_WORD_BITS) | line[DRAW_LINE_START_X_FRACTION_INDEX];
		if (mode == DRAW_LINE_MODE_Y_MAJOR_LEFT)
			value32 -= product;
		else
			value32 += product;
		value32 += DRAW_LINE_FIXED_ROUNDING;
		line[DRAW_LINE_END_X_INDEX] = (legacy_u16)(value32 >> LEGACY_WORD_BITS);
		break;
	case DRAW_LINE_MODE_X_MAJOR_LEFT:
	case DRAW_LINE_MODE_X_MAJOR_RIGHT:
		value32 = ((legacy_u32)dx << LEGACY_WORD_BITS) -
			(((legacy_u32)line[DRAW_LINE_START_Y_INDEX] << LEGACY_WORD_BITS) | line[DRAW_LINE_START_Y_FRACTION_INDEX]);
		if ((value32 & LEGACY_U32_SIGN_BIT) != 0)
			advance = 0;
		else
			advance = draw_line_round_div(value32, line[DRAW_LINE_STEP_INDEX]);
		if (mode == DRAW_LINE_MODE_X_MAJOR_LEFT)
			line[DRAW_LINE_END_X_INDEX] = (legacy_u16)(line[DRAW_LINE_START_X_INDEX] - advance);
		else
			line[DRAW_LINE_END_X_INDEX] = (legacy_u16)(line[DRAW_LINE_START_X_INDEX] + advance);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(advance + 1);
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

	value32 = ((legacy_u32)line[DRAW_LINE_START_Y_INDEX] << LEGACY_WORD_BITS) | line[DRAW_LINE_START_Y_FRACTION_INDEX];
	old_value = (legacy_u16)((value32 + DRAW_LINE_FIXED_ROUNDING) >>
		LEGACY_WORD_BITS);
	value32 += (legacy_u32)advance * line[DRAW_LINE_STEP_INDEX];
	line[DRAW_LINE_START_Y_FRACTION_INDEX] = (legacy_u16)value32;
	line[DRAW_LINE_START_Y_INDEX] = (legacy_u16)(value32 >> LEGACY_WORD_BITS);
	new_value = (legacy_u16)((value32 + DRAW_LINE_FIXED_ROUNDING) >>
		LEGACY_WORD_BITS);
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

	mode = line[DRAW_LINE_MODE_AND_CLIP_INDEX] & DRAW_LINE_MODE_MASK;
	switch (mode) {
	case DRAW_LINE_MODE_VERTICAL:
		return DRAW_LINE_CLIP_LEFT;
	case DRAW_LINE_MODE_DIAGONAL_LEFT:
		cx = sprite1.sprite_left2;
		ax = line[DRAW_LINE_END_X_INDEX];
		line[DRAW_LINE_END_X_INDEX] = cx;
		cx = (legacy_u16)(cx - ax);
		line[DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX] + cx);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - cx);
		line[DRAW_LINE_END_Y_INDEX] = (legacy_u16)(line[DRAW_LINE_END_Y_INDEX] - cx);
		break;
	case DRAW_LINE_MODE_DIAGONAL_RIGHT:
		ax = line[DRAW_LINE_START_X_INDEX];
		cx = sprite1.sprite_left2;
		line[DRAW_LINE_START_X_INDEX] = cx;
		cx = (legacy_u16)(cx - ax);
		line[DRAW_LINE_START_LEFT_CLIP_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_START_LEFT_CLIP_COUNT_INDEX] + cx);
		line[DRAW_LINE_START_Y_INDEX] = (legacy_u16)(line[DRAW_LINE_START_Y_INDEX] + cx);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - cx);
		break;
	case DRAW_LINE_MODE_Y_MAJOR_LEFT:
		value32 = ((legacy_u32)line[DRAW_LINE_START_X_INDEX] << LEGACY_WORD_BITS) | line[DRAW_LINE_START_X_FRACTION_INDEX];
		line[DRAW_LINE_END_X_INDEX] = sprite1.sprite_left2;
		difference = (legacy_s32)LEGACY_S16_FROM_BITS(line[DRAW_LINE_START_X_INDEX]) -
			(legacy_s32)LEGACY_S16_FROM_BITS(sprite1.sprite_left2);
		if (difference < 0) {
			advance = 1;
		} else {
			advance = draw_line_round_div(value32 -
				((legacy_u32)sprite1.sprite_left2 << LEGACY_WORD_BITS),
				line[DRAW_LINE_STEP_INDEX]);
			advance = (legacy_u16)(advance + 1);
		}
		original_count = line[DRAW_LINE_PIXEL_COUNT_INDEX];
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = advance;
		line[DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX] = (legacy_u16)(
			line[DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX] + original_count - advance);
		line[DRAW_LINE_END_Y_INDEX] = (legacy_u16)(line[DRAW_LINE_START_Y_INDEX] + advance - 1);
		break;
	case DRAW_LINE_MODE_Y_MAJOR_RIGHT:
		value32 = ((legacy_u32)sprite1.sprite_left2 <<
			LEGACY_WORD_BITS) -
			(((legacy_u32)line[DRAW_LINE_START_X_INDEX] << LEGACY_WORD_BITS) | line[DRAW_LINE_START_X_FRACTION_INDEX]);
		if ((value32 & LEGACY_U32_SIGN_BIT) != 0)
			return DRAW_LINE_CLIP_LEFT;
		advance = draw_line_round_div(value32, line[DRAW_LINE_STEP_INDEX]);
		line[DRAW_LINE_START_Y_INDEX] = (legacy_u16)(line[DRAW_LINE_START_Y_INDEX] + advance);
		line[DRAW_LINE_START_LEFT_CLIP_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_START_LEFT_CLIP_COUNT_INDEX] + advance);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - advance);
		if (LEGACY_S16_FROM_BITS(line[DRAW_LINE_PIXEL_COUNT_INDEX]) <= 0)
			return DRAW_LINE_CLIP_LEFT;
		product = (legacy_u32)advance * line[DRAW_LINE_STEP_INDEX];
		value32 = ((legacy_u32)line[DRAW_LINE_START_X_INDEX] << LEGACY_WORD_BITS) | line[DRAW_LINE_START_X_FRACTION_INDEX];
		value32 += product;
		line[DRAW_LINE_START_X_FRACTION_INDEX] = (legacy_u16)value32;
		line[DRAW_LINE_START_X_INDEX] = (legacy_u16)(value32 >> LEGACY_WORD_BITS);
		break;
	case DRAW_LINE_MODE_X_MAJOR_LEFT:
		ax = line[DRAW_LINE_START_X_INDEX];
		dx = sprite1.sprite_left2;
		line[DRAW_LINE_END_X_INDEX] = dx;
		ax = (legacy_u16)(ax - dx);
		advance = (legacy_u16)(ax + 1);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = advance;
		draw_line_advance_end(line, ax, 11);
		break;
	case DRAW_LINE_MODE_X_MAJOR_RIGHT:
		old_value = line[DRAW_LINE_START_X_INDEX];
		line[DRAW_LINE_START_X_INDEX] = sprite1.sprite_left2;
		advance = (legacy_u16)(line[DRAW_LINE_START_X_INDEX] - old_value);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - advance);
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

	mode = line[DRAW_LINE_MODE_AND_CLIP_INDEX] & DRAW_LINE_MODE_MASK;
	switch (mode) {
	case DRAW_LINE_MODE_VERTICAL:
		return draw_line_reject(line, DRAW_LINE_CLIP_RIGHT);
	case DRAW_LINE_MODE_DIAGONAL_LEFT:
		cx = line[DRAW_LINE_START_X_INDEX];
		ax = (legacy_u16)(sprite1.sprite_widthsum - 1);
		line[DRAW_LINE_START_X_INDEX] = ax;
		cx = (legacy_u16)(cx - ax);
		line[DRAW_LINE_START_Y_INDEX] = (legacy_u16)(line[DRAW_LINE_START_Y_INDEX] + cx);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - cx);
		line[DRAW_LINE_START_RIGHT_CLIP_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_START_RIGHT_CLIP_COUNT_INDEX] + cx);
		return 0;
	case DRAW_LINE_MODE_DIAGONAL_RIGHT:
		cx = (legacy_u16)(sprite1.sprite_widthsum - 1);
		ax = line[DRAW_LINE_END_X_INDEX];
		line[DRAW_LINE_END_X_INDEX] = cx;
		ax = (legacy_u16)(ax - cx);
		line[DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX] + ax);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - ax);
		line[DRAW_LINE_END_Y_INDEX] = (legacy_u16)(line[DRAW_LINE_END_Y_INDEX] - ax);
		return 0;
	case DRAW_LINE_MODE_Y_MAJOR_LEFT:
		value32 = ((legacy_u32)line[DRAW_LINE_START_X_INDEX] << LEGACY_WORD_BITS) | line[DRAW_LINE_START_X_FRACTION_INDEX];
		value32 -= (legacy_u32)(sprite1.sprite_widthsum - 1) <<
			LEGACY_WORD_BITS;
		advance = draw_line_round_div(value32, line[DRAW_LINE_STEP_INDEX]);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - advance);
		if (LEGACY_S16_FROM_BITS(line[DRAW_LINE_PIXEL_COUNT_INDEX]) <= 0)
			return draw_line_reject(line, DRAW_LINE_CLIP_RIGHT);
		line[DRAW_LINE_START_Y_INDEX] = (legacy_u16)(line[DRAW_LINE_START_Y_INDEX] + advance);
		line[DRAW_LINE_START_RIGHT_CLIP_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_START_RIGHT_CLIP_COUNT_INDEX] + advance);
		product = (legacy_u32)advance * line[DRAW_LINE_STEP_INDEX];
		value32 = ((legacy_u32)line[DRAW_LINE_START_X_INDEX] << LEGACY_WORD_BITS) | line[DRAW_LINE_START_X_FRACTION_INDEX];
		value32 -= product;
		line[DRAW_LINE_START_X_FRACTION_INDEX] = (legacy_u16)value32;
		line[DRAW_LINE_START_X_INDEX] = (legacy_u16)(value32 >> LEGACY_WORD_BITS);
		return 0;
	case DRAW_LINE_MODE_Y_MAJOR_RIGHT:
		line[DRAW_LINE_END_X_INDEX] = (legacy_u16)(sprite1.sprite_widthsum - 1);
		value32 = ((legacy_u32)line[DRAW_LINE_END_X_INDEX] << LEGACY_WORD_BITS) -
			(((legacy_u32)line[DRAW_LINE_START_X_INDEX] << LEGACY_WORD_BITS) | line[DRAW_LINE_START_X_FRACTION_INDEX]);
		if ((value32 & LEGACY_U32_SIGN_BIT) != 0)
			return draw_line_reject(line, DRAW_LINE_CLIP_RIGHT);
		advance = (legacy_u16)(
			draw_line_round_div(value32, line[DRAW_LINE_STEP_INDEX]) + 1);
		original_count = line[DRAW_LINE_PIXEL_COUNT_INDEX];
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = advance;
		line[DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX] = (legacy_u16)(
			line[DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX] + original_count - advance);
		line[DRAW_LINE_END_Y_INDEX] = (legacy_u16)(line[DRAW_LINE_START_Y_INDEX] + advance - 1);
		return 0;
	case DRAW_LINE_MODE_X_MAJOR_LEFT:
		ax = line[DRAW_LINE_START_X_INDEX];
		cx = (legacy_u16)(sprite1.sprite_widthsum - 1);
		ax = (legacy_u16)(ax - cx);
		line[DRAW_LINE_START_X_INDEX] = cx;
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_PIXEL_COUNT_INDEX] - ax);
		draw_line_advance_secondary(line, ax, 12U);
		return 0;
	case DRAW_LINE_MODE_X_MAJOR_RIGHT:
		ax = sprite1.sprite_widthsum;
		cx = (legacy_u16)(ax - 1);
		line[DRAW_LINE_END_X_INDEX] = cx;
		ax = (legacy_u16)(ax - line[DRAW_LINE_START_X_INDEX]);
		line[DRAW_LINE_PIXEL_COUNT_INDEX] = ax;
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

	ax = (legacy_u16)(line[DRAW_LINE_START_Y_INDEX] + vertical_step);
	line[DRAW_LINE_START_Y_INDEX] = ax;
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
			if (LEGACY_S16_FROM_BITS(line[DRAW_LINE_START_X_INDEX]) <
				LEGACY_S16_FROM_BITS(sprite1.sprite_left2)) {
				line[DRAW_LINE_START_LEFT_CLIP_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_START_LEFT_CLIP_COUNT_INDEX] + ax);
			} else {
				line[DRAW_LINE_START_RIGHT_CLIP_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_START_RIGHT_CLIP_COUNT_INDEX] + ax);
			}
		}
	}
	line[DRAW_LINE_START_X_INDEX] = (legacy_u16)(line[DRAW_LINE_START_X_INDEX] + horizontal_step);
}

static void draw_line_subdivide_advance_end(legacy_u16* line,
	legacy_u16 vertical_step, legacy_u16 horizontal_step)
{
	legacy_u16 ax;
	legacy_u16 bx;
	legacy_u8 update_counter;

	ax = (legacy_u16)(line[DRAW_LINE_END_Y_INDEX] - vertical_step);
	line[DRAW_LINE_END_Y_INDEX] = ax;
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
			if (LEGACY_S16_FROM_BITS(line[DRAW_LINE_END_X_INDEX]) <
				LEGACY_S16_FROM_BITS(sprite1.sprite_left2)) {
				line[DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX] + ax);
			} else {
				line[DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX] = (legacy_u16)(line[DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX] + ax);
			}
		}
	}
	line[DRAW_LINE_END_X_INDEX] = (legacy_u16)(line[DRAW_LINE_END_X_INDEX] - horizontal_step);
}

static void draw_line_subdivide(legacy_u16* line)
{
	legacy_u16 ax;
	legacy_u16 cx;
	legacy_u16 dx;

	cx = sar1_word(line[DRAW_LINE_END_Y_INDEX]);
	ax = sar1_word(line[DRAW_LINE_START_Y_INDEX]);
	cx = sar1_word((legacy_u16)(cx - ax));
	dx = sar1_word(line[DRAW_LINE_END_X_INDEX]);
	ax = sar1_word(line[DRAW_LINE_START_X_INDEX]);
	dx = sar1_word((legacy_u16)(dx - ax));

	for (;;) {
		while (LEGACY_S16_FROM_BITS(line[DRAW_LINE_START_Y_INDEX]) <=
			DRAW_LINE_SUBDIVIDE_MIN) {
			draw_line_subdivide_advance_start(line, cx, dx);
		}

		if (LEGACY_S16_FROM_BITS(line[DRAW_LINE_END_Y_INDEX]) >=
			DRAW_LINE_SUBDIVIDE_MAX) {
			draw_line_subdivide_advance_end(line, cx, dx);
			continue;
		}
		if (LEGACY_S16_FROM_BITS(line[DRAW_LINE_START_X_INDEX]) <= DRAW_LINE_SUBDIVIDE_MIN ||
			LEGACY_S16_FROM_BITS(line[DRAW_LINE_START_X_INDEX]) >= DRAW_LINE_SUBDIVIDE_MAX) {
			draw_line_subdivide_advance_start(line, cx, dx);
			continue;
		}
		if (LEGACY_S16_FROM_BITS(line[DRAW_LINE_END_X_INDEX]) <= DRAW_LINE_SUBDIVIDE_MIN ||
			LEGACY_S16_FROM_BITS(line[DRAW_LINE_END_X_INDEX]) >= DRAW_LINE_SUBDIVIDE_MAX) {
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

	line[DRAW_LINE_MODE_AND_CLIP_INDEX] = DRAW_LINE_MODE_UNSET;
	line[DRAW_LINE_START_X_FRACTION_INDEX] = 0;
	line[DRAW_LINE_START_Y_FRACTION_INDEX] = 0;
	line[DRAW_LINE_START_LEFT_CLIP_COUNT_INDEX] = 0;
	line[DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX] = 0;
	line[DRAW_LINE_START_RIGHT_CLIP_COUNT_INDEX] = 0;
	line[DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX] = 0;

	ax = (legacy_u16)arg_startY;
	bx = (legacy_u16)arg_endY;
	cx = (legacy_u16)arg_startX;
	dx = (legacy_u16)arg_endX;
	if (LEGACY_S16_FROM_BITS(ax) <= LEGACY_S16_FROM_BITS(bx)) {
		line[DRAW_LINE_START_X_INDEX] = cx;
		line[DRAW_LINE_START_Y_INDEX] = ax;
		line[DRAW_LINE_END_X_INDEX] = dx;
		line[DRAW_LINE_END_Y_INDEX] = bx;
	} else {
		line[DRAW_LINE_START_X_INDEX] = dx;
		line[DRAW_LINE_START_Y_INDEX] = bx;
		line[DRAW_LINE_END_X_INDEX] = cx;
		line[DRAW_LINE_END_Y_INDEX] = ax;
	}
	if (ax == bx)
		return draw_line_horizontal(ax, cx, dx, line, var_4);

	for (;;) {
	dx = 0;
	if ((legacy_u16)var_4 == 0) {
		ax = line[DRAW_LINE_START_Y_INDEX];
		bx = sprite1.sprite_top;
		cx = sprite1.sprite_height;
		if (LEGACY_S16_FROM_BITS(ax) >= LEGACY_S16_FROM_BITS(cx)) {
			dx = DRAW_LINE_CLIP_BOTTOM;
			return draw_line_reject(line, dx);
		}
		if (LEGACY_S16_FROM_BITS(ax) < LEGACY_S16_FROM_BITS(bx))
			dx |= DRAW_LINE_CLIP_TOP << DRAW_LINE_CLIP_SHIFT;

		ax = line[DRAW_LINE_END_Y_INDEX];
		if (LEGACY_S16_FROM_BITS(ax) < LEGACY_S16_FROM_BITS(bx)) {
			dx = DRAW_LINE_CLIP_TOP;
			return draw_line_reject(line, dx);
		}
		if (LEGACY_S16_FROM_BITS(ax) >= LEGACY_S16_FROM_BITS(cx))
			dx |= DRAW_LINE_CLIP_BOTTOM;

		bx = sprite1.sprite_left2;
		cx = sprite1.sprite_widthsum;
		ax = line[DRAW_LINE_START_X_INDEX];
		if (LEGACY_S16_FROM_BITS(ax) < LEGACY_S16_FROM_BITS(bx))
			dx |= DRAW_LINE_CLIP_LEFT << DRAW_LINE_CLIP_SHIFT;
		if (LEGACY_S16_FROM_BITS(ax) >= LEGACY_S16_FROM_BITS(cx))
			dx |= DRAW_LINE_CLIP_RIGHT << DRAW_LINE_CLIP_SHIFT;
		ax = line[DRAW_LINE_END_X_INDEX];
		if (LEGACY_S16_FROM_BITS(ax) < LEGACY_S16_FROM_BITS(bx))
			dx |= DRAW_LINE_CLIP_LEFT;
		if (LEGACY_S16_FROM_BITS(ax) >= LEGACY_S16_FROM_BITS(cx))
			dx |= DRAW_LINE_CLIP_RIGHT;
		if ((legacy_u8)dx & (legacy_u8)(dx >> LEGACY_BYTE_BITS)) {
			dx = (legacy_u8)dx &
				(legacy_u8)(dx >> LEGACY_BYTE_BITS);
			return draw_line_reject(line, dx);
		}
	}

	dx = (legacy_u16)((legacy_u8)dx |
		(legacy_u8)(dx >> LEGACY_BYTE_BITS));
	clip = dx;
	subdivide_required = 0;
	difference = (legacy_s32)LEGACY_S16_FROM_BITS(line[DRAW_LINE_END_Y_INDEX]) -
		(legacy_s32)LEGACY_S16_FROM_BITS(line[DRAW_LINE_START_Y_INDEX]);
	if (difference < -32768L || difference > 32767L) {
		subdivide_required = 1;
	} else {
		cx = (legacy_u16)difference;
		difference = (legacy_s32)LEGACY_S16_FROM_BITS(line[DRAW_LINE_END_X_INDEX]) -
			(legacy_s32)LEGACY_S16_FROM_BITS(line[DRAW_LINE_START_X_INDEX]);
		if (difference < -32768L || difference > 32767L) {
			subdivide_required = 1;
		} else {
			dx = (legacy_u16)difference;
			compute_step = 0;
			if (dx == 0) {
				cx = (legacy_u16)(cx + 1);
				line[DRAW_LINE_PIXEL_COUNT_INDEX] = cx;
				line[DRAW_LINE_MODE_AND_CLIP_INDEX] = (legacy_u16)((line[DRAW_LINE_MODE_AND_CLIP_INDEX] &
					LEGACY_U16_HIGH_BYTE_MASK) | DRAW_LINE_MODE_VERTICAL);
			} else if (LEGACY_S16_FROM_BITS(dx) >= 0) {
				if (dx < cx) {
					line[DRAW_LINE_MODE_AND_CLIP_INDEX] = (legacy_u16)((line[DRAW_LINE_MODE_AND_CLIP_INDEX] &
						LEGACY_U16_HIGH_BYTE_MASK) |
						DRAW_LINE_MODE_Y_MAJOR_RIGHT);
					compute_step = 1;
				} else if (dx == cx) {
					line[DRAW_LINE_MODE_AND_CLIP_INDEX] = (legacy_u16)((line[DRAW_LINE_MODE_AND_CLIP_INDEX] &
						LEGACY_U16_HIGH_BYTE_MASK) |
						DRAW_LINE_MODE_DIAGONAL_RIGHT);
					line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(cx + 1);
				} else {
					line[DRAW_LINE_MODE_AND_CLIP_INDEX] = (legacy_u16)((line[DRAW_LINE_MODE_AND_CLIP_INDEX] &
						LEGACY_U16_HIGH_BYTE_MASK) |
						DRAW_LINE_MODE_X_MAJOR_RIGHT);
					bx = cx;
					cx = dx;
					dx = bx;
					compute_step = 1;
				}
			} else if (dx == LEGACY_U16_SIGN_BIT) {
				subdivide_required = 1;
			} else {
				dx = (legacy_u16)(0U - dx);
				if (dx < cx) {
					line[DRAW_LINE_MODE_AND_CLIP_INDEX] = (legacy_u16)((line[DRAW_LINE_MODE_AND_CLIP_INDEX] &
						LEGACY_U16_HIGH_BYTE_MASK) |
						DRAW_LINE_MODE_Y_MAJOR_LEFT);
					compute_step = 1;
				} else if (dx == cx) {
					line[DRAW_LINE_MODE_AND_CLIP_INDEX] = (legacy_u16)((line[DRAW_LINE_MODE_AND_CLIP_INDEX] &
						LEGACY_U16_HIGH_BYTE_MASK) |
						DRAW_LINE_MODE_DIAGONAL_LEFT);
					line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(cx + 1);
				} else {
					line[DRAW_LINE_MODE_AND_CLIP_INDEX] = (legacy_u16)((line[DRAW_LINE_MODE_AND_CLIP_INDEX] &
						LEGACY_U16_HIGH_BYTE_MASK) |
						DRAW_LINE_MODE_X_MAJOR_LEFT);
					bx = cx;
					cx = dx;
					dx = bx;
					compute_step = 1;
				}
			}
			if (compute_step != 0) {
				line[DRAW_LINE_STEP_INDEX] = draw_line_step(dx, cx);
				if (cx == LEGACY_S16_MAX)
					subdivide_required = 1;
				else
					line[DRAW_LINE_PIXEL_COUNT_INDEX] = (legacy_u16)(cx + 1);
			}
		}
	}

	if (subdivide_required != 0) {
		draw_line_subdivide(line);
		continue;
	}

	for (;;) {
		switch (clip & DRAW_LINE_CLIP_MASK) {
		case 0:
			return 0;
		case DRAW_LINE_CLIP_RIGHT:
			return draw_line_clip_right(line);
		case DRAW_LINE_CLIP_LEFT:
		case DRAW_LINE_CLIP_LEFT | DRAW_LINE_CLIP_RIGHT:
			clip_result = draw_line_clip_left(line);
			if (clip_result == DRAW_LINE_CLIP_LEFT)
				return draw_line_reject(line, DRAW_LINE_CLIP_LEFT);
			if (clip_result == 0)
				return 0;
			if (clip & DRAW_LINE_CLIP_RIGHT)
				return draw_line_clip_right(line);
			return 0;
		case DRAW_LINE_CLIP_BOTTOM:
		case DRAW_LINE_CLIP_BOTTOM | DRAW_LINE_CLIP_RIGHT:
		case DRAW_LINE_CLIP_BOTTOM | DRAW_LINE_CLIP_LEFT:
		case DRAW_LINE_CLIP_BOTTOM | DRAW_LINE_CLIP_LEFT |
			DRAW_LINE_CLIP_RIGHT:
			if (draw_line_clip_bottom(line) == 0)
				return 0;
			break;
		default:
			if (draw_line_clip_top(line) == 0)
				return 0;
			if ((clip & DRAW_LINE_CLIP_BOTTOM) != 0 &&
				draw_line_clip_bottom(line) == 0) {
				return 0;
			}
			break;
		}

		dx = 0;
		ax = line[DRAW_LINE_START_X_INDEX];
		if (LEGACY_S16_FROM_BITS(ax) <
			LEGACY_S16_FROM_BITS(sprite1.sprite_left2)) {
			dx |= DRAW_LINE_CLIP_LEFT << DRAW_LINE_CLIP_SHIFT;
		}
		value32 = (legacy_u32)line[DRAW_LINE_START_X_FRACTION_INDEX] + DRAW_LINE_FIXED_ROUNDING;
		ax = (legacy_u16)(ax +
			(legacy_u16)(value32 >> LEGACY_WORD_BITS));
		if (LEGACY_S16_FROM_BITS(ax) >=
			LEGACY_S16_FROM_BITS(sprite1.sprite_widthsum)) {
			dx |= DRAW_LINE_CLIP_RIGHT << DRAW_LINE_CLIP_SHIFT;
		}
		ax = line[DRAW_LINE_END_X_INDEX];
		if (LEGACY_S16_FROM_BITS(ax) <
			LEGACY_S16_FROM_BITS(sprite1.sprite_left2)) {
			dx |= DRAW_LINE_CLIP_LEFT;
		}
		if (LEGACY_S16_FROM_BITS(ax) >=
			LEGACY_S16_FROM_BITS(sprite1.sprite_widthsum)) {
			dx |= DRAW_LINE_CLIP_RIGHT;
		}
		if ((legacy_u8)dx & (legacy_u8)(dx >> LEGACY_BYTE_BITS)) {
			dx = (legacy_u8)dx &
				(legacy_u8)(dx >> LEGACY_BYTE_BITS);
			return draw_line_reject(line, dx);
		}
		dx = (legacy_u16)((legacy_u8)dx |
			(legacy_u8)(dx >> LEGACY_BYTE_BITS));
		if (dx == 0)
			return 0;
		clip = dx;
	}
	}
}
