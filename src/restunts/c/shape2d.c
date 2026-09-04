#include <stddef.h>

#include "externs.h"
#include "memmgr.h"
#include "fileio.h"
#include "legacy.h"
#include "platform.h"
#include "resource.h"
#include "shape2d.h"
#include "shape2d_internal.h"

extern legacy_u8 far* active_font_definition;
extern legacy_u16 fontdefseg;

legacy_u16 shape2d_get_word(const legacy_u8 far* source)
{
	return LEGACY_READ_U16_LE(source);
}

void shape2d_put_word(legacy_u8 far* destination, legacy_u16 value)
{
	LEGACY_WRITE_U16_LE(destination, value);
}

legacy_u16 shape2d_get_width(const struct SHAPE2D far* shape)
{
	return shape2d_get_word((const legacy_u8 far*)shape +
		SHAPE2D_WIDTH_OFFSET);
}

legacy_u16 shape2d_get_height(const struct SHAPE2D far* shape)
{
	return shape2d_get_word((const legacy_u8 far*)shape +
		SHAPE2D_HEIGHT_OFFSET);
}

legacy_u16 shape2d_get_unk1(const struct SHAPE2D far* shape)
{
	return shape2d_get_word((const legacy_u8 far*)shape +
		SHAPE2D_UNK1_OFFSET);
}

legacy_u16 shape2d_get_unk2(const struct SHAPE2D far* shape)
{
	return shape2d_get_word((const legacy_u8 far*)shape +
		SHAPE2D_UNK2_OFFSET);
}

legacy_u16 shape2d_get_pos_x(const struct SHAPE2D far* shape)
{
	return shape2d_get_word((const legacy_u8 far*)shape +
		SHAPE2D_POS_X_OFFSET);
}

legacy_u16 shape2d_get_pos_y(const struct SHAPE2D far* shape)
{
	return shape2d_get_word((const legacy_u8 far*)shape +
		SHAPE2D_POS_Y_OFFSET);
}

legacy_u16 shape2d_get_line_offset(legacy_u16 sprite_segment,
	legacy_u16 y)
{
	legacy_u16 line_entry;
	legacy_u8 far* line_entry_ptr;

	line_entry = LEGACY_U16_WRAP_ADD(
		dos_memory_pointer_offset(sprite1.sprite_lineofs), (legacy_u16)(y << 1));
	line_entry_ptr = (legacy_u8 far*)dos_memory_make_pointer(sprite_segment, line_entry);
	return shape2d_get_word(line_entry_ptr);
}

void sprite_set_1_size(legacy_u16 left, legacy_u16 right,
	legacy_u16 top, legacy_u16 height)
{
	sprite1.sprite_left2 = left;
	sprite1.sprite_left = left;
	sprite1.sprite_widthsum = right;
	sprite1.sprite_right = right;
	sprite1.sprite_top = top;
	sprite1.sprite_height = height;
}

void nopsub_3320E(struct SPRITE far* sprite, legacy_u16 left,
	legacy_u16 right, legacy_u16 top, legacy_u16 height)
{
	sprite->sprite_left2 = left;
	sprite->sprite_left = left;
	sprite->sprite_widthsum = right;
	sprite->sprite_right = right;
	sprite->sprite_top = top;
	sprite->sprite_height = height;

	if (dos_memory_pointer_segment(sprite->sprite_bitmapptr) == dos_memory_pointer_segment(sprite1.sprite_bitmapptr))
		sprite_set_1_size(left, right, top, height);
}

void sprite_1_unk(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color)
{
	legacy_u8 far* bitmap;
	legacy_u16 offset;
	legacy_u16 row;
	legacy_u16 column;
	legacy_u16 row_count;
	legacy_u16 column_count;

	if (LEGACY_S16_FROM_BITS(width) <= 0 ||
		LEGACY_S16_FROM_BITS(height) <= 0)
		return;
	bitmap = (legacy_u8 far*)sprite1.sprite_bitmapptr;
	offset = LEGACY_U16_WRAP_ADD(
		shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1), (legacy_u16)y),
		(legacy_u16)x);
	row_count = (legacy_u16)height;
	column_count = (legacy_u16)width;
	for (row = 0; row < row_count; row++) {
		for (column = 0; column < column_count; column++)
			bitmap[LEGACY_U16_WRAP_ADD(offset, column)] =
				(legacy_u8)color;
		offset = LEGACY_U16_WRAP_ADD(offset, sprite1.sprite_pitch);
	}
}

static legacy_s16 sprite_clip_rectangle(legacy_s16 x, legacy_s16 y,
	legacy_s16 width, legacy_s16 height, legacy_s16* clipped_x,
	legacy_s16* clipped_y, legacy_s16* clipped_width,
	legacy_s16* clipped_height)
{
	legacy_s16 difference;

	*clipped_x = LEGACY_S16_FROM_BITS(x);
	*clipped_y = LEGACY_S16_FROM_BITS(y);
	*clipped_width = LEGACY_S16_FROM_BITS(width);
	*clipped_height = LEGACY_S16_FROM_BITS(height);
	difference = LEGACY_S16_WRAP_SUB(sprite1.sprite_left, *clipped_x);
	if (difference > 0) {
		*clipped_x = LEGACY_S16_FROM_BITS(sprite1.sprite_left);
		*clipped_width = LEGACY_S16_WRAP_SUB(*clipped_width, difference);
		if (*clipped_width <= 0)
			return 0;
	}
	difference = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_ADD(*clipped_x, *clipped_width),
		sprite1.sprite_right);
	if (difference > 0) {
		*clipped_width = LEGACY_S16_WRAP_SUB(*clipped_width, difference);
		if (*clipped_width <= 0)
			return 0;
	}
	difference = LEGACY_S16_WRAP_SUB(sprite1.sprite_top, *clipped_y);
	if (difference > 0) {
		*clipped_height = LEGACY_S16_WRAP_SUB(*clipped_height, difference);
		if (*clipped_height <= 0)
			return 0;
		*clipped_y = LEGACY_S16_FROM_BITS(sprite1.sprite_top);
	}
	difference = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_ADD(*clipped_y, *clipped_height),
		sprite1.sprite_height);
	if (difference > 0) {
		*clipped_height = LEGACY_S16_WRAP_SUB(*clipped_height, difference);
		if (*clipped_height <= 0)
			return 0;
	}
	return 1;
}

void sprite_1_unk2(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color)
{
	legacy_s16 clipped_x;
	legacy_s16 clipped_y;
	legacy_s16 clipped_width;
	legacy_s16 clipped_height;

	if (!sprite_clip_rectangle(x, y, width, height, &clipped_x,
		&clipped_y, &clipped_width, &clipped_height))
		return;
	sprite_1_unk(clipped_x, clipped_y, clipped_width, clipped_height, color);
}

void sprite_1_unk4(legacy_s16 x1, legacy_s16 y1, legacy_s16 x2, legacy_s16 y2, legacy_s16 color)
{
	legacy_s16 width;
	legacy_s16 height;

	width = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_SUB(x2, x1), 1);
	height = LEGACY_S16_WRAP_SUB(y2, y1);
	if (width > 0) {
		sprite_1_unk2(x1, y1, width, 1, color);
		sprite_1_unk2(x1, y2, width, 1, color);
	}
	if (height > 0) {
		sprite_1_unk2(x1, y1, 1, height, color);
		sprite_1_unk2(x2, y1, 1, height, color);
	}
}

static void font_draw_text_impl(const legacy_s8* text, legacy_s16 x, legacy_s16 y, legacy_s16 opaque)
{
	legacy_u8 far* font_definition;
	legacy_u8 far* glyph_data;
	legacy_u8 far* bitmap;
	legacy_u16 glyph_offset;
	legacy_u16 current_x;
	legacy_u16 current_y;
	legacy_u16 destination;
	legacy_u16 glyph_width;
	legacy_u16 row_index;
	legacy_u8 character;
	legacy_u8 color;
	legacy_u8 background;
	legacy_u8 bits;
	legacy_u8 bit;
	legacy_s8 byte_count;
	legacy_s8 old_byte_count;
	legacy_s16 row_count;
	legacy_s16 old_row_count;

	font_definition = active_font_definition;
	shape2d_put_word(font_definition + 8U, (legacy_u16)x);
	shape2d_put_word(font_definition + 0x0AU, (legacy_u16)y);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	while ((character = (legacy_u8)*text++) != 0) {
		glyph_offset = shape2d_get_word(font_definition + 0x16U +
			(legacy_u16)character * 2U);
		if (glyph_offset == 0) {
			if (character == '\r' || character == '\n') {
				shape2d_put_word(font_definition + 8U,
					shape2d_get_word(font_definition + 4U));
				shape2d_put_word(font_definition + 0x0AU,
					LEGACY_U16_WRAP_ADD(
						shape2d_get_word(font_definition + 0x0AU),
						shape2d_get_word(font_definition + 0x12U)));
			}
			continue;
		}
		glyph_data = font_definition + glyph_offset;
		current_x = shape2d_get_word(font_definition + 8U);
		if (font_definition[0x14U] != 0) {
			glyph_width = *glyph_data++;
			shape2d_put_word(font_definition + 0x10U, glyph_width);
			font_definition[0x0CU] = (legacy_u8)((glyph_width + 7U) >> 3);
		}
		color = font_definition[0];
		background = font_definition[2];
		current_y = shape2d_get_word(font_definition + 0x0AU);
		row_index = current_y;
		row_count = LEGACY_S16_FROM_BITS(
			shape2d_get_word(font_definition + 0x0EU));
		do {
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1), row_index),
				current_x);
			byte_count = LEGACY_S8_FROM_BITS(font_definition[0x0CU]);
			do {
				bits = *glyph_data++;
				for (bit = 0; bit < 8U; bit++) {
					if ((bits & 0x80U) != 0)
						bitmap[destination] = color;
					else if (opaque != 0)
						bitmap[destination] = background;
					bits <<= 1;
					destination++;
				}
				old_byte_count = byte_count;
				byte_count = LEGACY_S8_FROM_BITS(
					(legacy_u8)((legacy_u8)byte_count - 1U));
			} while (old_byte_count != LEGACY_S8_FROM_BITS(0x80U) &&
				byte_count > 0);
			row_index++;
			old_row_count = row_count;
			row_count = LEGACY_S16_WRAP_SUB(row_count, 1);
		} while (old_row_count != LEGACY_S16_FROM_BITS(0x8000U) &&
			row_count > 0);
		shape2d_put_word(font_definition + 8U,
			LEGACY_U16_WRAP_ADD(current_x,
				shape2d_get_word(font_definition + 0x10U)));
	}
}

void font_draw_text(const legacy_s8* text, legacy_s16 x, legacy_s16 y)
{
	font_draw_text_impl(text, x, y, 0);
}

void sub_345BC(const legacy_s8* text, legacy_s16 x, legacy_s16 y)
{
	font_draw_text_impl(text, x, y, 1);
}

void draw_filled_lines(legacy_s16* x1arr, legacy_s16* x2arr, legacy_u16 y,
	legacy_u16 numlines, legacy_u16 color)
{
	legacy_u8 far* bitmap;
	legacy_u16 current_y;
	legacy_u16 line_count;
	legacy_u16 old_line_count;
	legacy_u16 left;
	legacy_u16 right;
	legacy_u16 width;
	legacy_u16 destination;

	line_count = (legacy_u16)numlines;
	if (line_count == 0)
		return;
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	current_y = (legacy_u16)y;
	do {
		left = (legacy_u16)*x1arr++;
		right = (legacy_u16)*x2arr++;
		width = LEGACY_U16_WRAP_ADD(
			LEGACY_U16_WRAP_SUB(right, left), 1U);
		if (width != 0 && width <= 0x8000U) {
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1), current_y),
				left);
			do {
				bitmap[destination] = (legacy_u8)color;
				destination++;
				width--;
			} while (width != 0);
		}
		current_y++;
		old_line_count = line_count;
		line_count = LEGACY_U16_WRAP_SUB(line_count, 1U);
	} while (old_line_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(line_count) > 0);
}

static legacy_u8 shape2d_rotate_left_8(legacy_u8 value, legacy_u8 count)
{
	count &= 7U;
	if (count == 0)
		return value;
	return (legacy_u8)((value << count) | (value >> (8U - count)));
}

static void draw_pattern_lines(legacy_s16* x1arr, legacy_s16* x2arr, legacy_u16 y,
	legacy_u16 numlines, legacy_u16 color, legacy_s16 two_colors)
{
	legacy_u8 far* bitmap;
	legacy_u8 far* line_entry_ptr;
	legacy_u16 sprite_segment;
	legacy_u16 line_entry;
	legacy_u16 line_count;
	legacy_u16 old_line_count;
	legacy_u16 left;
	legacy_u16 right;
	legacy_u16 width;
	legacy_u16 destination;
	legacy_u8 pattern;
	legacy_u8 alternate_color;
	legacy_u16 swapped_pattern;

	if (((legacy_u16)y & 1U) == 0) {
		word_4031E = (legacy_u16)((word_4031E << 8) |
			(word_4031E >> 8));
	}
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	sprite_segment = dos_memory_pointer_segment(&sprite1);
	line_entry = LEGACY_U16_WRAP_ADD(
		dos_memory_pointer_offset(sprite1.sprite_lineofs),
		(legacy_u16)((legacy_u16)y << 1));
	line_count = (legacy_u16)numlines;
	do {
		left = (legacy_u16)*x1arr++;
		right = (legacy_u16)*x2arr++;
		pattern = (legacy_u8)word_4031E;
		pattern = shape2d_rotate_left_8(pattern, (legacy_u8)left);
		alternate_color = (legacy_u8)word_40320;
		width = LEGACY_U16_WRAP_ADD(
			LEGACY_U16_WRAP_SUB(right, left), 1U);
		if (width != 0 && width <= 0x8000U) {
			line_entry_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				sprite_segment, line_entry);
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_word(line_entry_ptr), left);
			do {
				pattern = shape2d_rotate_left_8(pattern, 1U);
				if ((pattern & 1U) != 0) {
					if (two_colors != 0)
						bitmap[destination] = alternate_color;
					else
						bitmap[destination] = (legacy_u8)color;
				} else if (two_colors != 0) {
					bitmap[destination] = (legacy_u8)color;
				}
				destination++;
				width--;
			} while (width != 0);
		}
		line_entry = LEGACY_U16_WRAP_ADD(line_entry, 2U);
		swapped_pattern = (legacy_u16)((word_4031E << 8) |
			(word_4031E >> 8));
		word_4031E = swapped_pattern;
		old_line_count = line_count;
		line_count = LEGACY_U16_WRAP_SUB(line_count, 1U);
	} while (old_line_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(line_count) > 0);
}

void draw_unknown_lines(legacy_s16* x1arr, legacy_s16* x2arr, legacy_u16 y,
	legacy_u16 numlines, legacy_u16 color)
{
	draw_pattern_lines(x1arr, x2arr, y, numlines, color, 1);
}

void nopsub_33330(legacy_s16* x1arr, legacy_s16* x2arr, legacy_u16 y,
	legacy_u16 numlines, legacy_u16 color, legacy_u16 alternate_color,
	legacy_u16 pattern)
{
	word_4031E = (legacy_u16)pattern;
	word_40320 = (legacy_u16)((word_40320 & 0xFF00U) |
		(legacy_u8)alternate_color);
	draw_unknown_lines(x1arr, x2arr, y, numlines, color);
}

void draw_patterned_lines(legacy_s16* x1arr, legacy_s16* x2arr, legacy_u16 y,
	legacy_u16 numlines, legacy_u16 color)
{
	draw_pattern_lines(x1arr, x2arr, y, numlines, color, 0);
}

void putpixel_line1_maybe(const legacy_u16* line)
{
	legacy_u8 far* bitmap;
	legacy_u16 sprite_segment;
	legacy_u16 x_low;
	legacy_u16 x_high;
	legacy_u16 y_low;
	legacy_u16 y_high;
	legacy_u16 original_y_high;
	legacy_u16 delta;
	legacy_u16 count;
	legacy_u16 remaining;
	legacy_u16 destination;
	legacy_u16 old_low;
	legacy_u16 mode;
	legacy_u8 color;

	x_low = (legacy_u16)line[0];
	x_high = (legacy_u16)line[1];
	old_low = x_low;
	x_low = LEGACY_U16_WRAP_ADD(x_low, 0x8000U);
	if (x_low < old_low)
		x_high++;
	y_low = (legacy_u16)line[2];
	y_high = (legacy_u16)line[3];
	old_low = y_low;
	y_low = LEGACY_U16_WRAP_ADD(y_low, 0x8000U);
	if (y_low < old_low)
		y_high++;
	original_y_high = (legacy_u16)line[3];
	delta = (legacy_u16)line[6];
	count = (legacy_u16)line[7];
	color = (legacy_u8)line[8];
	mode = (legacy_u16)line[9];
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	sprite_segment = dos_memory_pointer_segment(&sprite1);

	switch (mode) {
	case 0:
	case 1:
		destination = LEGACY_U16_WRAP_ADD(
			shape2d_get_line_offset(sprite_segment, y_high), x_high);
		remaining = count;
		while (remaining != 0) {
			bitmap[destination] = color;
			destination++;
			remaining--;
		}
		break;
	case 2:
		remaining = count;
		do {
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_line_offset(sprite_segment, original_y_high),
				x_high);
			bitmap[destination] = color;
			original_y_high++;
			remaining--;
		} while (remaining != 0);
		break;
	case 3:
		remaining = count;
		do {
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_line_offset(sprite_segment, original_y_high),
				x_high);
			bitmap[destination] = color;
			x_high--;
			original_y_high++;
			remaining--;
		} while (remaining != 0);
		break;
	case 4:
		remaining = count;
		do {
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_line_offset(sprite_segment, original_y_high),
				x_high);
			bitmap[destination] = color;
			x_high++;
			original_y_high++;
			remaining--;
		} while (remaining != 0);
		break;
	case 5:
		remaining = count;
		do {
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_line_offset(sprite_segment, original_y_high),
				x_high);
			bitmap[destination] = color;
			original_y_high++;
			old_low = x_low;
			x_low = LEGACY_U16_WRAP_SUB(x_low, delta);
			if (old_low < delta)
				x_high--;
			remaining--;
		} while (remaining != 0);
		break;
	case 6:
		remaining = count;
		do {
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_line_offset(sprite_segment, original_y_high),
				x_high);
			bitmap[destination] = color;
			original_y_high++;
			old_low = x_low;
			x_low = LEGACY_U16_WRAP_ADD(x_low, delta);
			if (x_low < old_low)
				x_high++;
			remaining--;
		} while (remaining != 0);
		break;
	case 7:
	case 8:
		remaining = count;
		do {
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_line_offset(sprite_segment, y_high), x_high);
			bitmap[destination] = color;
			if (mode == 7U)
				x_high--;
			else
				x_high++;
			old_low = y_low;
			y_low = LEGACY_U16_WRAP_ADD(y_low, delta);
			if (y_low < old_low)
				y_high++;
			remaining--;
		} while (remaining != 0);
		break;
	case 9:
		destination = LEGACY_U16_WRAP_ADD(
			shape2d_get_line_offset(sprite_segment, y_high), x_high);
		bitmap[destination] = color;
		break;
	}
}

void sprite_1_unk3(struct SHAPE2D far* shape, legacy_u16 phase)
{
	static const legacy_u8 row_order[12] = {
		11, 5, 8, 2, 10, 4, 7, 1, 9, 3, 6, 0
	};
	static const legacy_u8 skip_count[4] = { 1, 3, 0, 2 };
	static const legacy_u8 advance_count[4] = { 3, 1, 4, 2 };
	legacy_u8 far* bitmap;
	legacy_u8 far* line_entry_ptr;
	legacy_u8 far* source_ptr;
	legacy_u16 shape_segment;
	legacy_u16 sprite_segment;
	legacy_u16 line_table_start;
	legacy_u16 line_table_end;
	legacy_u16 line_entry;
	legacy_u16 data_start;
	legacy_u16 row_source;
	legacy_u16 source;
	legacy_u16 source_row_step;
	legacy_u16 destination;
	legacy_u16 width;
	legacy_u16 height;
	legacy_u16 pos_x;
	legacy_u16 pos_y;
	legacy_u16 remaining;
	legacy_u16 row_phase;
	legacy_u16 pattern;
	legacy_u16 selector;
	legacy_u16 skip;
	legacy_u16 advance;
	legacy_s16 order_index;

	shape_segment = dos_memory_pointer_segment(shape);
	sprite_segment = dos_memory_pointer_segment(&sprite1);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	width = shape2d_get_width(shape);
	height = shape2d_get_height(shape);
	pos_x = shape2d_get_pos_x(shape);
	pos_y = shape2d_get_pos_y(shape);
	line_table_start = LEGACY_U16_WRAP_ADD(
		dos_memory_pointer_offset(sprite1.sprite_lineofs), (legacy_u16)(pos_y << 1));
	line_table_end = LEGACY_U16_WRAP_ADD(
		line_table_start, (legacy_u16)(height << 1));
	data_start = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	source_row_step = (legacy_u16)((legacy_u32)width * 12UL);
	for (order_index = 11; order_index >= 0; order_index--) {
		selector = row_order[order_index];
		line_entry = LEGACY_U16_WRAP_ADD(line_table_start,
			(legacy_u16)(selector << 1));
		source = LEGACY_U16_WRAP_ADD(data_start,
			(legacy_u16)((legacy_u32)width * selector));
		row_phase = (legacy_u16)phase;
		while (line_entry < line_table_end) {
			line_entry_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				sprite_segment, line_entry);
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_word(line_entry_ptr), pos_x);
			remaining = width;
			row_source = source;
			pattern = row_phase;
			for (;;) {
				pattern &= 3U;
				skip = skip_count[pattern];
				if (LEGACY_S16_FROM_BITS(remaining) <=
					(legacy_s16)skip)
					break;
				remaining = LEGACY_U16_WRAP_SUB(remaining, skip);
				source = LEGACY_U16_WRAP_ADD(source, skip);
				destination = LEGACY_U16_WRAP_ADD(destination, skip);
				source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
					shape_segment, source);
				bitmap[destination] = *source_ptr;
				advance = advance_count[pattern];
				source = LEGACY_U16_WRAP_ADD(source, advance);
				destination = LEGACY_U16_WRAP_ADD(
					destination, advance);
				remaining = LEGACY_U16_WRAP_SUB(
					remaining, advance);
				pattern++;
			}
			row_phase++;
			line_entry = LEGACY_U16_WRAP_ADD(line_entry, 24U);
			source = LEGACY_U16_WRAP_ADD(
				row_source, source_row_step);
		}
		phase++;
	}
}

void sub_34526(struct SHAPE2D far* shape)
{
	legacy_u8 far* bitmap;
	legacy_u8 far* source_ptr;
	legacy_u16 shape_segment;
	legacy_u16 source;
	legacy_u16 destination;
	legacy_u16 destination_advance;
	legacy_u16 width;
	legacy_u16 row_count;
	legacy_u16 old_row_count;
	legacy_u16 column_count;
	legacy_u8 source_color;
	legacy_u8 mapped_color;

	shape_segment = dos_memory_pointer_segment(shape);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	width = shape2d_get_width(shape);
	row_count = shape2d_get_height(shape);
	destination = LEGACY_U16_WRAP_ADD(
		shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1),
			shape2d_get_pos_y(shape)),
		shape2d_get_pos_x(shape));
	destination_advance = LEGACY_U16_WRAP_SUB(
		sprite1.sprite_pitch, width);
	source = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	do {
		column_count = width;
		do {
			source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				shape_segment, source);
			source_color = *source_ptr;
			source++;
			mapped_color = incnums[source_color];
			if (mapped_color != 0xFFU)
				bitmap[destination] = mapped_color;
			destination++;
			column_count--;
		} while (column_count != 0);
		destination = LEGACY_U16_WRAP_ADD(
			destination, destination_advance);
		old_row_count = row_count;
		row_count = LEGACY_U16_WRAP_SUB(row_count, 1U);
	} while (old_row_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(row_count) > 0);
}

static void sprite_clear_shape_impl(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y)
{
	legacy_u8 far* bitmap;
	legacy_u8 far* destination_ptr;
	legacy_u8 far* line_entry_ptr;
	legacy_u16 shape_segment;
	legacy_u16 sprite_segment;
	legacy_u16 line_entry;
	legacy_u16 destination;
	legacy_u16 source;
	legacy_u16 width;
	legacy_u16 row_count;
	legacy_u16 old_row_count;
	legacy_u16 column_count;

	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	shape_segment = dos_memory_pointer_segment(shape);
	sprite_segment = dos_memory_pointer_segment(&sprite1);
	line_entry = LEGACY_U16_WRAP_ADD(
		dos_memory_pointer_offset(sprite1.sprite_lineofs), (legacy_u16)(y << 1));
	destination = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	width = shape2d_get_word((legacy_u8 far*)shape);
	row_count = shape2d_get_word((legacy_u8 far*)shape + 2U);
	do {
		line_entry_ptr = (legacy_u8 far*)dos_memory_make_pointer(
			sprite_segment, line_entry);
		source = LEGACY_U16_WRAP_ADD(
			shape2d_get_word(line_entry_ptr), x);
		column_count = width;
		while (column_count != 0) {
			destination_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				shape_segment, destination);
			*destination_ptr = bitmap[source];
			destination++;
			source++;
			column_count--;
		}
		line_entry = LEGACY_U16_WRAP_ADD(line_entry, 2U);
		old_row_count = row_count;
		row_count = LEGACY_U16_WRAP_SUB(row_count, 1U);
	} while (old_row_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(row_count) > 0);
}

void sprite_clear_shape_alt(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	shape2d_put_word(shape_bytes + 8U, (legacy_u16)x);
	shape2d_put_word(shape_bytes + 0x0AU, (legacy_u16)y);
	sprite_clear_shape_impl(shape, (legacy_u16)x, (legacy_u16)y);
}

void sprite_clear_shape(struct SHAPE2D far* shape)
{
	sprite_clear_shape_impl(shape,
		shape2d_get_pos_x(shape),
		shape2d_get_pos_y(shape));
}

void nopsub_34736(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_clear_shape_impl(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_unk1(shape)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_unk2(shape)));
}

static legacy_u16 shape2d_scaled_anchor(legacy_u16 anchor,
	legacy_u16 scale)
{
	legacy_s32 product;

	product = (legacy_s32)LEGACY_S16_FROM_BITS(anchor) *
		(legacy_s32)LEGACY_S16_FROM_BITS(scale);
	return (legacy_u16)((legacy_u32)product >> 8);
}

static void shape2d_scale_transparent_impl(struct SHAPE2D far* shape,
	legacy_u16 scale, legacy_u16 x, legacy_u16 y, legacy_s16 clipped)
{
	legacy_u8 far* source_ptr;
	legacy_u8 far* bitmap;
	legacy_u32 product;
	legacy_u16 shape_segment;
	legacy_u16 source;
	legacy_u16 row_source;
	legacy_u16 destination;
	legacy_u16 source_width;
	legacy_u16 scaled_width;
	legacy_u16 scaled_height;
	legacy_u16 destination_advance;
	legacy_u16 step;
	legacy_u16 horizontal_start;
	legacy_u16 horizontal_fraction;
	legacy_u16 vertical_fraction;
	legacy_u16 center_skip;
	legacy_u16 skipped;
	legacy_u16 overflow;
	legacy_u16 column_count;
	legacy_u16 row_count;
	legacy_u16 old_row_count;
	legacy_u16 source_rows;
	legacy_u8 color;

	if (scale < 2U)
		return;
	shape_segment = dos_memory_pointer_segment(shape);
	x = LEGACY_U16_WRAP_SUB(x, shape2d_scaled_anchor(
		shape2d_get_unk1(shape), scale));
	y = LEGACY_U16_WRAP_SUB(y, shape2d_scaled_anchor(
		shape2d_get_unk2(shape), scale));
	source_width = shape2d_get_width(shape);
	product = (legacy_u32)shape2d_get_height(shape) * scale;
	scaled_height = (legacy_u16)(product >> 8);
	if (scaled_height == 0)
		return;
	product = (legacy_u32)source_width * scale;
	scaled_width = (legacy_u16)(product >> 8);
	if (scaled_width == 0)
		return;
	source = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	step = (legacy_u16)LEGACY_U32_DIV_OR_ZERO(0x10000UL, scale);
	horizontal_start = 0;
	vertical_fraction = 0;
	center_skip = (legacy_u16)((step >> 8) >> 1);
	source = LEGACY_U16_WRAP_ADD(source,
		(legacy_u16)((legacy_u32)center_skip *
		((legacy_u32)source_width + 1UL)));

	if (clipped != 0) {
		if (LEGACY_S16_FROM_BITS(x) <
			LEGACY_S16_FROM_BITS(sprite1.sprite_left2)) {
			overflow = LEGACY_U16_WRAP_SUB(
				LEGACY_U16_WRAP_ADD(x, scaled_width),
				sprite1.sprite_left2);
			if (LEGACY_S16_FROM_BITS(overflow) <= 0)
				return;
			skipped = LEGACY_U16_WRAP_SUB(scaled_width, overflow);
			scaled_width = overflow;
			x = sprite1.sprite_left2;
			product = (legacy_u32)skipped * step;
			horizontal_start = (legacy_u8)product;
			source = LEGACY_U16_WRAP_ADD(source,
				(legacy_u16)(product >> 8));
		}
		overflow = LEGACY_U16_WRAP_SUB(
			LEGACY_U16_WRAP_ADD(x, scaled_width),
			sprite1.sprite_widthsum);
		if (LEGACY_S16_FROM_BITS(overflow) >= 0) {
			scaled_width = LEGACY_U16_WRAP_SUB(
				scaled_width, overflow);
			if (LEGACY_S16_FROM_BITS(scaled_width) <= 0)
				return;
		}
		if (LEGACY_S16_FROM_BITS(y) <
			LEGACY_S16_FROM_BITS(sprite1.sprite_top)) {
			overflow = LEGACY_U16_WRAP_SUB(
				LEGACY_U16_WRAP_ADD(y, scaled_height),
				sprite1.sprite_top);
			if (LEGACY_S16_FROM_BITS(overflow) <= 0)
				return;
			skipped = LEGACY_U16_WRAP_SUB(scaled_height, overflow);
			scaled_height = overflow;
			y = sprite1.sprite_top;
			product = (legacy_u32)skipped * step;
			vertical_fraction = (legacy_u8)product;
			source_rows = (legacy_u16)(product >> 8);
			source = LEGACY_U16_WRAP_ADD(source,
				LEGACY_U16_WRAP_MUL(source_rows, source_width));
		}
		overflow = LEGACY_U16_WRAP_SUB(
			LEGACY_U16_WRAP_ADD(y, scaled_height),
			sprite1.sprite_height);
		if (LEGACY_S16_FROM_BITS(overflow) >= 0) {
			scaled_height = LEGACY_U16_WRAP_SUB(
				scaled_height, overflow);
			if (LEGACY_S16_FROM_BITS(scaled_height) <= 0)
				return;
		}
	}

	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	destination = LEGACY_U16_WRAP_ADD(
		shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1), y), x);
	destination_advance = LEGACY_U16_WRAP_SUB(
		sprite1.sprite_pitch, scaled_width);
	row_source = source;
	row_count = scaled_height;
	do {
		column_count = scaled_width;
		horizontal_fraction = horizontal_start;
		do {
			source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				shape_segment, source);
			color = *source_ptr;
			if (color != 0xFFU)
				bitmap[destination] = color;
			destination++;
			horizontal_fraction = LEGACY_U16_WRAP_ADD(
				horizontal_fraction, step);
			source = LEGACY_U16_WRAP_ADD(source,
				horizontal_fraction >> 8);
			horizontal_fraction &= 0x00FFU;
			column_count--;
		} while (column_count != 0);
		old_row_count = row_count;
		row_count = LEGACY_U16_WRAP_SUB(row_count, 1U);
		if (old_row_count == 0x8000U ||
			LEGACY_S16_FROM_BITS(row_count) <= 0)
			break;
		destination = LEGACY_U16_WRAP_ADD(
			destination, destination_advance);
		source = row_source;
		vertical_fraction = LEGACY_U16_WRAP_ADD(
			vertical_fraction, step);
		source_rows = vertical_fraction >> 8;
		if (source_rows != 0) {
			source = LEGACY_U16_WRAP_ADD(source,
				LEGACY_U16_WRAP_MUL(source_rows, source_width));
			vertical_fraction &= 0x00FFU;
			row_source = source;
		}
	} while (1);
}

void shape_op_explosion(legacy_s16 scale, struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	shape2d_scale_transparent_impl(shape, (legacy_u16)scale,
		(legacy_u16)x, (legacy_u16)y, 1);
}

void sub_35E08(legacy_s16 scale, struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	shape2d_scale_transparent_impl(shape, (legacy_u16)scale,
		(legacy_u16)x, (legacy_u16)y, 0);
}

static void sprite_shape_to_1_impl(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y, legacy_s16 operation)
{
	legacy_u8 far* source_ptr;
	legacy_u8 far* bitmap;
	legacy_u16 shape_segment;
	legacy_u16 source;
	legacy_u16 destination;
	legacy_u16 width;
	legacy_u16 row_count;
	legacy_u16 old_row_count;
	legacy_u32 pixel_count;

	shape_segment = dos_memory_pointer_segment(shape);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	source = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	destination = LEGACY_U16_WRAP_ADD(
		shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1), y), x);
	width = shape2d_get_width(shape);
	row_count = shape2d_get_height(shape);
	do {
		if (width == 0 && operation != SHAPE2D_RASTER_COPY)
			pixel_count = 0x20000UL;
		else
			pixel_count = width;
		while (pixel_count != 0) {
			source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				shape_segment, source);
			if (operation == SHAPE2D_RASTER_OR)
				bitmap[destination] |= *source_ptr;
			else if (operation == SHAPE2D_RASTER_COPY)
				bitmap[destination] = *source_ptr;
			else
				bitmap[destination] &= *source_ptr;
			source++;
			destination++;
			pixel_count--;
		}
		if (width == 1 && operation == SHAPE2D_RASTER_AND)
			bitmap[destination] = 0;
		destination = LEGACY_U16_WRAP_ADD(destination,
			LEGACY_U16_WRAP_SUB(sprite1.sprite_pitch, width));
		old_row_count = row_count;
		row_count = LEGACY_U16_WRAP_SUB(row_count, 1U);
	} while (old_row_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(row_count) > 0);
}

void sprite_shape_to_1(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_shape_to_1_impl(shape, (legacy_u16)x, (legacy_u16)y,
		SHAPE2D_RASTER_COPY);
}

void sprite_shape_to_1_alt(struct SHAPE2D far* shape)
{
	sprite_shape_to_1_impl(shape,
		shape2d_get_pos_x(shape),
		shape2d_get_pos_y(shape), SHAPE2D_RASTER_COPY);
}

static void sprite_shape_to_1_at_anchor(struct SHAPE2D far* shape,
	legacy_s16 x, legacy_s16 y, legacy_s16 operation)
{
	sprite_shape_to_1_impl(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_unk1(shape)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_unk2(shape)),
		operation);
}

void nopsub_33D0C(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_shape_to_1_at_anchor(shape, x, y, SHAPE2D_RASTER_COPY);
}

void putpixel_iconMask(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_shape_to_1_impl(shape, (legacy_u16)x, (legacy_u16)y,
		SHAPE2D_RASTER_AND);
}

void nopsub_339FA(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_shape_to_1_at_anchor(shape, x, y, SHAPE2D_RASTER_AND);
}

void putpixel_iconFillings(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_shape_to_1_impl(shape, (legacy_u16)x, (legacy_u16)y,
		SHAPE2D_RASTER_OR);
}

void putpixel_single_maybe(legacy_s16 x, legacy_s16 y, legacy_s16 color)
{
	legacy_u8 far* bitmap;
	legacy_u16 x_bits;
	legacy_u16 y_bits;
	legacy_u16 destination;

	x_bits = (legacy_u16)x;
	y_bits = (legacy_u16)y;
	if (LEGACY_S16_FROM_BITS(x_bits) <
		LEGACY_S16_FROM_BITS(sprite1.sprite_left) ||
		LEGACY_S16_FROM_BITS(x_bits) >=
		LEGACY_S16_FROM_BITS(sprite1.sprite_right) ||
		LEGACY_S16_FROM_BITS(y_bits) <
		LEGACY_S16_FROM_BITS(sprite1.sprite_top) ||
		LEGACY_S16_FROM_BITS(y_bits) >=
		LEGACY_S16_FROM_BITS(sprite1.sprite_height))
		return;
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	destination = LEGACY_U16_WRAP_ADD(
		shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1), y_bits), x_bits);
	bitmap[destination] = (legacy_u8)color;
}

void set_fontdefseg(void far* data)
{
	fontdefseg = dos_memory_pointer_segment(data);
	active_font_definition = (legacy_u8 far*)data;
}

void sub_35B76(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color)
{
	legacy_u8 far* bitmap;
	legacy_s16 clipped_x;
	legacy_s16 clipped_y;
	legacy_s16 clipped_width;
	legacy_s16 clipped_height;
	legacy_u16 destination;
	legacy_u16 row_count;
	legacy_u16 old_row_count;
	legacy_u16 column_count;
	legacy_u8 color_bits;

	if (!sprite_clip_rectangle(x, y, width, height, &clipped_x,
		&clipped_y, &clipped_width, &clipped_height))
		return;
	if (clipped_width <= 0 || clipped_height <= 0)
		return;
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	destination = LEGACY_U16_WRAP_ADD(shape2d_get_line_offset(
		dos_memory_pointer_segment(&sprite1), (legacy_u16)clipped_y),
		(legacy_u16)clipped_x);
	row_count = (legacy_u16)clipped_height;
	color_bits = (legacy_u8)color;
	do {
		column_count = (legacy_u16)clipped_width;
		do {
			bitmap[destination] ^= color_bits;
			destination++;
			column_count--;
		} while (column_count != 0);
		destination = LEGACY_U16_WRAP_ADD(destination,
			LEGACY_U16_WRAP_SUB(sprite1.sprite_pitch,
				(legacy_u16)clipped_width));
		old_row_count = row_count;
		row_count = LEGACY_U16_WRAP_SUB(row_count, 1U);
	} while (old_row_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(row_count) > 0);
}

void sub_35C4E(legacy_s16 source_x, legacy_s16 source_y, legacy_s16 width, legacy_s16 height,
	legacy_s16 destination_shift)
{
	legacy_u8 far* source_bitmap;
	legacy_u8 far* destination_bitmap;
	legacy_u16 source;
	legacy_u16 destination;
	legacy_u16 source_line;
	legacy_u16 destination_line;
	legacy_u16 column_count;
	legacy_u16 row_count;
	legacy_u16 old_row_count;
	legacy_s16 dividend;
	legacy_s16 divisor;
	legacy_s16 quotient;
	legacy_s16 remainder;

	dividend = LEGACY_S16_WRAP_ADD(source_x, destination_shift);
	divisor = LEGACY_S16_FROM_BITS(sprite1.sprite_width2);
	if ((legacy_u16)divisor == 0U ||
		((legacy_u16)dividend == 0x8000U && divisor == -1)) {
		quotient = 0;
		remainder = 0;
	} else {
		quotient = LEGACY_S16_DIV_OR_ZERO(dividend, divisor);
		remainder = (legacy_s16)(dividend % divisor);
	}
	source_line = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(sprite2.sprite_lineofs),
		(legacy_u16)((legacy_u16)source_y << 1));
	destination_line = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(sprite1.sprite_lineofs),
		(legacy_u16)(LEGACY_U16_WRAP_ADD(source_y, quotient) << 1));
	source_bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite2.sprite_bitmapptr), 0);
	destination_bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	row_count = (legacy_u16)height;
	do {
		source = LEGACY_U16_WRAP_ADD(shape2d_get_word(
			(legacy_u8 far*)dos_memory_make_pointer(dos_memory_pointer_segment(&sprite2), source_line)),
			(legacy_u16)source_x);
		destination = LEGACY_U16_WRAP_ADD(shape2d_get_word(
			(legacy_u8 far*)dos_memory_make_pointer(dos_memory_pointer_segment(&sprite1), destination_line)),
			(legacy_u16)remainder);
		column_count = (legacy_u16)width;
		while (column_count != 0) {
			destination_bitmap[destination] = source_bitmap[source];
			source++;
			destination++;
			column_count--;
		}
		source_line = LEGACY_U16_WRAP_ADD(source_line, 2U);
		destination_line = LEGACY_U16_WRAP_ADD(destination_line, 2U);
		old_row_count = row_count;
		row_count = LEGACY_U16_WRAP_SUB(row_count, 1U);
	} while (old_row_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(row_count) > 0);
}

void sub_35DE6(legacy_s16 destination_index, legacy_s16 count, void far* source_data)
{
	legacy_u8 far* source_ptr;
	legacy_u8 far* destination_ptr;
	legacy_u16 source_segment;
	legacy_u16 source;
	legacy_u16 destination_segment;
	legacy_u16 destination;
	legacy_u16 remaining;

	source_segment = dos_memory_pointer_segment(source_data);
	source = dos_memory_pointer_offset(source_data);
	destination_segment = dos_memory_pointer_segment(incnums);
	destination = LEGACY_U16_WRAP_ADD(
		dos_memory_pointer_offset(incnums), (legacy_u16)destination_index);
	remaining = (legacy_u16)count;
	while (remaining != 0) {
		source_ptr = (legacy_u8 far*)dos_memory_make_pointer(source_segment, source);
		destination_ptr = (legacy_u8 far*)dos_memory_make_pointer(
			destination_segment, destination);
		*destination_ptr = *source_ptr;
		source++;
		destination++;
		remaining--;
	}
}

// like locate_resource_by_index()
