#include <stddef.h>

#include "externs.h"
#include "memmgr.h"
#include "fileio.h"
#include "legacy.h"
#include "platform.h"
#include "resource.h"
#include "shape2d.h"

extern legacy_s8 aWindowdefOutOfRowTableSpa[];
extern legacy_s8 aMcgaWindow[];
extern legacy_s8 aWindowReleased[];
extern struct SPRITE far* render_window_sprite;

extern legacy_u8* far wnd_defs; // a reserved memory chunk of 0xE10 bytes in seg012. contents are SPRITE structs followed by lineoffsets. cast to a far pointer for access to the contents in other segments.
extern legacy_s8* far next_wnd_def; // near pointer relative to seg012 to the current SPRITE in wnd_defs. cast to a far pointer for access to the contents in other segments
extern struct SPRITE far sprite1; // seg012
extern struct SPRITE far sprite2; // seg012
extern struct SPRITE far* mcgawndsprite;
extern struct SPRITE far* mouse_background_sprite;
extern struct SPRITE far* mouse_medium_sprite;
extern struct SPRITE far* mouse_small_sprite;
extern legacy_s8 mouse_background_dirty;
extern legacy_u8 far* active_font_definition;
extern legacy_u16 fontdefseg;
extern legacy_u8 far incnums[];
extern legacy_u16 word_4031E;
extern legacy_u16 word_40320;
extern legacy_u8 byte_3B8FC;
extern struct SPRITE far* sprite_ptrs[4];
extern legacy_s16 word_4646A[4];
extern legacy_s16 word_46486[4];

static legacy_u16 shape2d_get_word(const legacy_u8 far* source)
{
	return LEGACY_READ_U16_LE(source);
}

static void shape2d_put_word(legacy_u8 far* destination, legacy_u16 value)
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

static legacy_u16 shape2d_get_line_offset(legacy_u16 sprite_segment,
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

void sprite_1_unk2(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color)
{
	legacy_s16 clipped_x;
	legacy_s16 clipped_y;
	legacy_s16 clipped_width;
	legacy_s16 clipped_height;
	legacy_s16 difference;

	clipped_x = LEGACY_S16_FROM_BITS(x);
	clipped_y = LEGACY_S16_FROM_BITS(y);
	clipped_width = LEGACY_S16_FROM_BITS(width);
	clipped_height = LEGACY_S16_FROM_BITS(height);
	difference = LEGACY_S16_WRAP_SUB(sprite1.sprite_left, clipped_x);
	if (difference > 0) {
		clipped_x = LEGACY_S16_FROM_BITS(sprite1.sprite_left);
		clipped_width = LEGACY_S16_WRAP_SUB(clipped_width, difference);
		if (clipped_width <= 0)
			return;
	}
	difference = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_ADD(clipped_x, clipped_width),
		sprite1.sprite_right);
	if (difference > 0) {
		clipped_width = LEGACY_S16_WRAP_SUB(clipped_width, difference);
		if (clipped_width <= 0)
			return;
	}
	difference = LEGACY_S16_WRAP_SUB(sprite1.sprite_top, clipped_y);
	if (difference > 0) {
		clipped_height = LEGACY_S16_WRAP_SUB(clipped_height, difference);
		if (clipped_height <= 0)
			return;
		clipped_y = LEGACY_S16_FROM_BITS(sprite1.sprite_top);
	}
	difference = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_ADD(clipped_y, clipped_height),
		sprite1.sprite_height);
	if (difference > 0) {
		clipped_height = LEGACY_S16_WRAP_SUB(clipped_height, difference);
		if (clipped_height <= 0)
			return;
	}
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
		remaining = count;
		do {
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_line_offset(sprite_segment, y_high), x_high);
			bitmap[destination] = color;
			x_high--;
			old_low = y_low;
			y_low = LEGACY_U16_WRAP_ADD(y_low, delta);
			if (y_low < old_low)
				y_high++;
			remaining--;
		} while (remaining != 0);
		break;
	case 8:
		remaining = count;
		do {
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_line_offset(sprite_segment, y_high), x_high);
			bitmap[destination] = color;
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
	legacy_u8 far* shape_bytes;
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

	shape_bytes = (legacy_u8 far*)shape;
	shape_segment = dos_memory_pointer_segment(shape);
	sprite_segment = dos_memory_pointer_segment(&sprite1);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	width = shape2d_get_word(shape_bytes);
	height = shape2d_get_word(shape_bytes + 2U);
	pos_x = shape2d_get_word(shape_bytes + 8U);
	pos_y = shape2d_get_word(shape_bytes + 0x0AU);
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
	legacy_u8 far* shape_bytes;
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

	shape_bytes = (legacy_u8 far*)shape;
	shape_segment = dos_memory_pointer_segment(shape);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	width = shape2d_get_word(shape_bytes);
	row_count = shape2d_get_word(shape_bytes + 2U);
	destination = LEGACY_U16_WRAP_ADD(
		shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1),
			shape2d_get_word(shape_bytes + 0x0AU)),
		shape2d_get_word(shape_bytes + 8U));
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
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	sprite_clear_shape_impl(shape,
		shape2d_get_word(shape_bytes + 8U),
		shape2d_get_word(shape_bytes + 0x0AU));
}

void nopsub_34736(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	sprite_clear_shape_impl(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_word(shape_bytes + 4U)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_word(shape_bytes + 6U)));
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
	legacy_u8 far* shape_bytes;
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
	shape_bytes = (legacy_u8 far*)shape;
	shape_segment = dos_memory_pointer_segment(shape);
	x = LEGACY_U16_WRAP_SUB(x, shape2d_scaled_anchor(
		shape2d_get_word(shape_bytes + 4U), scale));
	y = LEGACY_U16_WRAP_SUB(y, shape2d_scaled_anchor(
		shape2d_get_word(shape_bytes + 6U), scale));
	source_width = shape2d_get_word(shape_bytes);
	product = (legacy_u32)shape2d_get_word(shape_bytes + 2U) * scale;
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
	legacy_u16 x, legacy_u16 y)
{
	legacy_u8 far* shape_bytes;
	legacy_u8 far* source_ptr;
	legacy_u8 far* bitmap;
	legacy_u16 shape_segment;
	legacy_u16 source;
	legacy_u16 destination;
	legacy_u16 width;
	legacy_u16 column_count;
	legacy_u16 row_count;
	legacy_u16 old_row_count;

	shape_bytes = (legacy_u8 far*)shape;
	shape_segment = dos_memory_pointer_segment(shape);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	source = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	destination = LEGACY_U16_WRAP_ADD(
		shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1), y), x);
	width = shape2d_get_word(shape_bytes);
	row_count = shape2d_get_word(shape_bytes + 2U);
	do {
		column_count = width;
		while (column_count != 0) {
			source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				shape_segment, source);
			bitmap[destination] = *source_ptr;
			source++;
			destination++;
			column_count--;
		}
		destination = LEGACY_U16_WRAP_ADD(destination,
			LEGACY_U16_WRAP_SUB(sprite1.sprite_pitch, width));
		old_row_count = row_count;
		row_count = LEGACY_U16_WRAP_SUB(row_count, 1U);
	} while (old_row_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(row_count) > 0);
}

void sprite_shape_to_1(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_shape_to_1_impl(shape, (legacy_u16)x, (legacy_u16)y);
}

void sprite_shape_to_1_alt(struct SHAPE2D far* shape)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	sprite_shape_to_1_impl(shape,
		shape2d_get_word(shape_bytes + 8U),
		shape2d_get_word(shape_bytes + 0x0AU));
}

void nopsub_33D0C(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	sprite_shape_to_1_impl(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_word(shape_bytes + 4U)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_word(shape_bytes + 6U)));
}

static void putpixel_icon_combine(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y, legacy_s16 combine_or)
{
	legacy_u8 far* shape_bytes;
	legacy_u8 far* source_ptr;
	legacy_u8 far* bitmap;
	legacy_u16 shape_segment;
	legacy_u16 source;
	legacy_u16 destination;
	legacy_u16 width;
	legacy_u16 column_count;
	legacy_u16 pair_count;
	legacy_u16 row_count;
	legacy_u16 old_row_count;

	shape_bytes = (legacy_u8 far*)shape;
	shape_segment = dos_memory_pointer_segment(shape);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	source = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	destination = LEGACY_U16_WRAP_ADD(
		shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1), y), x);
	width = shape2d_get_word(shape_bytes);
	row_count = shape2d_get_word(shape_bytes + 2U);
	do {
		if (width == 0) {
			pair_count = 0;
			do {
				source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
					shape_segment, source);
				if (combine_or != 0)
					bitmap[destination] |= *source_ptr;
				else
					bitmap[destination] &= *source_ptr;
				source++;
				destination++;
				source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
					shape_segment, source);
				if (combine_or != 0)
					bitmap[destination] |= *source_ptr;
				else
					bitmap[destination] &= *source_ptr;
				source++;
				destination++;
				pair_count--;
			} while (pair_count != 0);
		} else {
			column_count = width;
			do {
				source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
					shape_segment, source);
				if (combine_or != 0)
					bitmap[destination] |= *source_ptr;
				else
					bitmap[destination] &= *source_ptr;
				source++;
				destination++;
				column_count--;
			} while (column_count != 0);
			if (width == 1 && combine_or == 0)
				bitmap[destination] = 0;
		}
		destination = LEGACY_U16_WRAP_ADD(destination,
			LEGACY_U16_WRAP_SUB(sprite1.sprite_pitch, width));
		old_row_count = row_count;
		row_count = LEGACY_U16_WRAP_SUB(row_count, 1U);
	} while (old_row_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(row_count) > 0);
}

void putpixel_iconMask(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	putpixel_icon_combine(shape, (legacy_u16)x, (legacy_u16)y, 0);
}

void nopsub_339FA(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	putpixel_icon_combine(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_word(shape_bytes + 4U)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_word(shape_bytes + 6U)), 0);
}

void putpixel_iconFillings(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	putpixel_icon_combine(shape, (legacy_u16)x, (legacy_u16)y, 1);
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
	legacy_s16 difference;
	legacy_u16 destination;
	legacy_u16 row_count;
	legacy_u16 old_row_count;
	legacy_u16 column_count;
	legacy_u8 color_bits;

	clipped_x = LEGACY_S16_FROM_BITS(x);
	clipped_y = LEGACY_S16_FROM_BITS(y);
	clipped_width = LEGACY_S16_FROM_BITS(width);
	clipped_height = LEGACY_S16_FROM_BITS(height);
	difference = LEGACY_S16_WRAP_SUB(sprite1.sprite_left, clipped_x);
	if (difference > 0) {
		clipped_x = LEGACY_S16_FROM_BITS(sprite1.sprite_left);
		clipped_width = LEGACY_S16_WRAP_SUB(clipped_width, difference);
		if (clipped_width <= 0)
			return;
	}
	difference = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_ADD(clipped_x, clipped_width),
		sprite1.sprite_right);
	if (difference > 0) {
		clipped_width = LEGACY_S16_WRAP_SUB(clipped_width, difference);
		if (clipped_width <= 0)
			return;
	}
	difference = LEGACY_S16_WRAP_SUB(sprite1.sprite_top, clipped_y);
	if (difference > 0) {
		clipped_height = LEGACY_S16_WRAP_SUB(clipped_height, difference);
		if (clipped_height <= 0)
			return;
		clipped_y = LEGACY_S16_FROM_BITS(sprite1.sprite_top);
	}
	difference = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_ADD(clipped_y, clipped_height),
		sprite1.sprite_height);
	if (difference > 0) {
		clipped_height = LEGACY_S16_WRAP_SUB(clipped_height, difference);
		if (clipped_height <= 0)
			return;
	}
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

legacy_u32 parse_shape2d_helper(void far* data)
{
	return ((legacy_u32)dos_memory_pointer_segment(data) << 4) + dos_memory_pointer_offset(data);
}

void far* parse_shape2d_helper2(legacy_u32 linear_address)
{
	return dos_memory_make_pointer((legacy_u16)(linear_address >> 4),
		(legacy_u16)linear_address & 0x0FU);
}

legacy_s16 parse_shape2d_helper3(void far* data)
{
	legacy_u8 far* source_ptr;
	legacy_u16 source_segment;
	legacy_u16 source;
	legacy_u16 count;
	legacy_u8 value;

	source_segment = dos_memory_pointer_segment(data);
	source = dos_memory_pointer_offset(data);
	source_ptr = (legacy_u8 far*)dos_memory_make_pointer(source_segment, source);
	value = *source_ptr;
	count = 0;
	for (;;) {
		source_ptr = (legacy_u8 far*)dos_memory_make_pointer(source_segment, source);
		source++;
		if (*source_ptr != value)
			return count;
		count++;
	}
}

static legacy_u8 shape2d_far_read_byte(legacy_u16 segment,
	legacy_u16 offset)
{
	return *(legacy_u8 far*)dos_memory_make_pointer(segment, offset);
}

static void shape2d_far_write_byte(legacy_u16 segment,
	legacy_u16 offset, legacy_u8 value)
{
	*(legacy_u8 far*)dos_memory_make_pointer(segment, offset) = value;
}

static void shape2d_far_write_dword(legacy_u16 segment,
	legacy_u16 offset, legacy_u32 value)
{
	shape2d_far_write_byte(segment, offset, (legacy_u8)value);
	offset++;
	shape2d_far_write_byte(segment, offset, (legacy_u8)(value >> 8));
	offset++;
	shape2d_far_write_byte(segment, offset, (legacy_u8)(value >> 16));
	offset++;
	shape2d_far_write_byte(segment, offset, (legacy_u8)(value >> 24));
}

static void shape2d_copy_wrapped(legacy_u16 source_segment,
	legacy_u16* source, legacy_u16 destination_segment,
	legacy_u16* destination, legacy_u16 count)
{
	legacy_u16 copied;

	copied = 0;
	while (LEGACY_S16_FROM_BITS(copied) <
		LEGACY_S16_FROM_BITS(count)) {
		shape2d_far_write_byte(destination_segment, *destination,
			shape2d_far_read_byte(source_segment, *source));
		(*source)++;
		(*destination)++;
		copied++;
	}
}

void parse_shape2d(void far* memchunk, void far* mempages)
{
	struct SHAPE2D far* shape;
	void far* output_pointer;
	legacy_u32 initial_output_linear;
	legacy_u32 output_linear;
	legacy_u32 output_size;
	legacy_u16 chunk_segment;
	legacy_u16 chunk_offset;
	legacy_u16 pages_segment;
	legacy_u16 pages_offset;
	legacy_u16 offsets_offset;
	legacy_u16 output_segment;
	legacy_u16 output_offset;
	legacy_u16 source_segment;
	legacy_u16 source_offset;
	legacy_u16 scan_offset;
	legacy_u16 literal_offset;
	legacy_u16 shape_count;
	legacy_u16 shape_index;
	legacy_u16 header_size;
	legacy_u16 copied;
	legacy_u16 remaining;
	legacy_u16 literal_count;
	legacy_u16 run_count;
	legacy_u8 value;

	chunk_segment = dos_memory_pointer_segment(memchunk);
	chunk_offset = dos_memory_pointer_offset(memchunk);
	pages_segment = dos_memory_pointer_segment(mempages);
	pages_offset = dos_memory_pointer_offset(mempages);
	shape_count = file_get_res_shape_count(memchunk);
	offsets_offset = LEGACY_U16_WRAP_ADD(pages_offset,
		LEGACY_U16_WRAP_ADD(LEGACY_U16_WRAP_MUL(shape_count, 4U), 6U));
	header_size = LEGACY_U16_WRAP_ADD(
		LEGACY_U16_WRAP_MUL(shape_count, 4U), 6U);
	copied = 0;
	while (LEGACY_S16_FROM_BITS(header_size) >
		LEGACY_S16_FROM_BITS(copied)) {
		shape2d_far_write_byte(pages_segment, pages_offset,
			shape2d_far_read_byte(chunk_segment, chunk_offset));
		chunk_offset++;
		pages_offset++;
		copied++;
	}
	output_segment = dos_memory_pointer_segment(mempages);
	output_offset = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(mempages),
		LEGACY_U16_WRAP_ADD(LEGACY_U16_WRAP_MUL(shape_count, 8U), 6U));
	initial_output_linear = parse_shape2d_helper(
		dos_memory_make_pointer(output_segment, output_offset));

	shape_index = 0;
	while (LEGACY_S16_FROM_BITS(shape_index) <
		LEGACY_S16_FROM_BITS(shape_count)) {
		shape = file_get_shape2d((legacy_u8 far*)memchunk, shape_index);
		output_linear = parse_shape2d_helper(
			dos_memory_make_pointer(output_segment, output_offset));
		output_pointer = parse_shape2d_helper2(output_linear);
		output_segment = dos_memory_pointer_segment(output_pointer);
		output_offset = dos_memory_pointer_offset(output_pointer);
		shape2d_far_write_dword(pages_segment, offsets_offset,
			output_linear - initial_output_linear);
		offsets_offset = LEGACY_U16_WRAP_ADD(offsets_offset, 4U);

		source_segment = dos_memory_pointer_segment(shape);
		source_offset = dos_memory_pointer_offset(shape);
		shape2d_copy_wrapped(source_segment, &source_offset,
			output_segment, &output_offset,
			SHAPE2D_HEADER_SIZE);
		scan_offset = source_offset;
		literal_offset = scan_offset;
		literal_count = 0;
		remaining = LEGACY_U16_WRAP_MUL(
			shape2d_get_word((legacy_u8 far*)shape),
			shape2d_get_word((legacy_u8 far*)shape + 2U));
		scan_offset++;
		literal_count++;

		if (remaining != 0) {
			for (;;) {
				run_count = (legacy_u16)parse_shape2d_helper3(
					dos_memory_make_pointer(source_segment, scan_offset));
				if (LEGACY_S16_FROM_BITS(run_count) <= 3 &&
					literal_count < remaining) {
					scan_offset++;
					literal_count++;
					continue;
				}

				while (LEGACY_S16_FROM_BITS(literal_count) > 0x7F) {
					literal_count = LEGACY_U16_WRAP_SUB(
						literal_count, 0x7FU);
					remaining = LEGACY_U16_WRAP_SUB(
						remaining, 0x7FU);
					shape2d_far_write_byte(output_segment,
						output_offset, 0x81U);
					output_offset++;
					shape2d_copy_wrapped(source_segment,
						&literal_offset, output_segment,
						&output_offset, 0x7FU);
				}
				if (literal_count != 0) {
					shape2d_far_write_byte(output_segment,
						output_offset,
						(legacy_u8)(0U - literal_count));
					output_offset++;
					remaining = LEGACY_U16_WRAP_SUB(
						remaining, literal_count);
					shape2d_copy_wrapped(source_segment,
						&literal_offset, output_segment,
						&output_offset, literal_count);
				}

				if (run_count > remaining)
					run_count = remaining;
				while (LEGACY_S16_FROM_BITS(run_count) > 0x7F) {
					run_count = LEGACY_U16_WRAP_SUB(
						run_count, 0x7FU);
					remaining = LEGACY_U16_WRAP_SUB(
						remaining, 0x7FU);
					shape2d_far_write_byte(output_segment,
						output_offset, 0x7FU);
					output_offset++;
					value = shape2d_far_read_byte(
						source_segment, scan_offset);
					shape2d_far_write_byte(output_segment,
						output_offset, value);
					output_offset++;
					scan_offset = LEGACY_U16_WRAP_ADD(
						scan_offset, 0x7FU);
				}
				if (LEGACY_S16_FROM_BITS(run_count) > 3) {
					shape2d_far_write_byte(output_segment,
						output_offset, (legacy_u8)run_count);
					output_offset++;
					remaining = LEGACY_U16_WRAP_SUB(
						remaining, run_count);
					value = shape2d_far_read_byte(
						source_segment, scan_offset);
					shape2d_far_write_byte(output_segment,
						output_offset, value);
					output_offset++;
					scan_offset = LEGACY_U16_WRAP_ADD(
						scan_offset, run_count);
				}

				literal_offset = scan_offset;
				literal_count = 0;
				scan_offset++;
				literal_count++;
				if (remaining == 0)
					break;
			}
		}
		shape2d_far_write_byte(output_segment, output_offset, 0);
		output_offset++;
		shape_index++;
	}

	output_size = parse_shape2d_helper(
		dos_memory_make_pointer(output_segment, output_offset)) -
		parse_shape2d_helper(mempages);
	if ((legacy_u8)output_size & 0x0FU)
		output_size = (output_size >> 4) + 1UL;
	else
		output_size >>= 4;
	mmgr_resize_memory(dos_memory_pointer_offset(mempages), dos_memory_pointer_segment(mempages),
		(legacy_u16)output_size);
}

#define SHAPE2D_RLE_AND 0
#define SHAPE2D_RLE_OR 1
#define SHAPE2D_RLE_COPY 2

static void shape2d_render_rle(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y, legacy_s16 operation)
{
	legacy_u8 far* shape_bytes;
	legacy_u8 far* source_ptr;
	legacy_u8 far* bitmap;
	legacy_u16 shape_segment;
	legacy_u16 source;
	legacy_u16 line_entry;
	legacy_u16 destination;
	legacy_u16 width;
	legacy_u16 remaining;
	legacy_u16 old_remaining;
	legacy_u16 count;
	legacy_u8 control_bits;
	legacy_s8 control;
	legacy_u8 value;
	legacy_s16 literal;

	shape_bytes = (legacy_u8 far*)shape;
	shape_segment = dos_memory_pointer_segment(shape);
	source = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	width = shape2d_get_word(shape_bytes);
	line_entry = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(sprite1.sprite_lineofs),
		(legacy_u16)(y << 1));
	destination = LEGACY_U16_WRAP_ADD(shape2d_get_word(
		(legacy_u8 far*)dos_memory_make_pointer(dos_memory_pointer_segment(&sprite1), line_entry)), x);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	remaining = width;
	for (;;) {
		source_ptr = (legacy_u8 far*)dos_memory_make_pointer(shape_segment, source);
		control_bits = *source_ptr;
		source++;
		control = LEGACY_S8_FROM_BITS(control_bits);
		if (control == 0)
			return;
		literal = control < 0;
		if (literal != 0) {
			count = (legacy_u8)(0U - control_bits);
		} else {
			count = control_bits;
			source_ptr = (legacy_u8 far*)dos_memory_make_pointer(shape_segment, source);
			value = *source_ptr;
			source++;
		}
		do {
			if (literal != 0) {
				source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
					shape_segment, source);
				value = *source_ptr;
				source++;
			}
			if (operation == SHAPE2D_RLE_OR)
				bitmap[destination] |= value;
			else if (operation == SHAPE2D_RLE_COPY)
				bitmap[destination] = value;
			else
				bitmap[destination] &= value;
			destination++;
			old_remaining = remaining;
			remaining = LEGACY_U16_WRAP_SUB(remaining, 1U);
			if (old_remaining == 0x8000U ||
				LEGACY_S16_FROM_BITS(remaining) <= 0) {
				line_entry = LEGACY_U16_WRAP_ADD(line_entry, 2U);
				destination = LEGACY_U16_WRAP_ADD(shape2d_get_word(
					(legacy_u8 far*)dos_memory_make_pointer(
						dos_memory_pointer_segment(&sprite1), line_entry)), x);
				remaining = width;
			}
			count--;
		} while (count != 0);
	}
}

void shape2d_render_bmp_as_mask(struct SHAPE2D far* shape)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle(shape,
		shape2d_get_word(shape_bytes + 8U),
		shape2d_get_word(shape_bytes + 0x0AU), SHAPE2D_RLE_AND);
}

void nopsub_33AC0(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_word(shape_bytes + 4U)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_word(shape_bytes + 6U)),
		SHAPE2D_RLE_AND);
}

void nopsub_33AE4(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	shape2d_render_rle(shape, (legacy_u16)x, (legacy_u16)y,
		SHAPE2D_RLE_AND);
}

void shape2d_op_unk4(legacy_u16 offset, legacy_u16 segment)
{
	struct SHAPE2D far* shape;
	legacy_u8 far* shape_bytes;

	shape = (struct SHAPE2D far*)dos_memory_make_pointer(segment, offset);
	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle(shape,
		shape2d_get_word(shape_bytes + 8U),
		shape2d_get_word(shape_bytes + 0x0AU), SHAPE2D_RLE_OR);
}

void shape2d_op_unk5(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	shape2d_render_rle(shape, (legacy_u16)x, (legacy_u16)y,
		SHAPE2D_RLE_COPY);
}

void shape2d_op_unk(struct SHAPE2D far* shape)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle(shape,
		shape2d_get_word(shape_bytes + 8U),
		shape2d_get_word(shape_bytes + 0x0AU), SHAPE2D_RLE_COPY);
}

void nopsub_33DBE(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_word(shape_bytes + 4U)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_word(shape_bytes + 6U)),
		SHAPE2D_RLE_COPY);
}

struct SPRITE far* sprite_make_wnd(legacy_u16 width, legacy_u16 height, legacy_u16 unk) {
	legacy_s16 pages, i;
	legacy_s8* wnd;
	legacy_s8* nextwnd;
	struct SPRITE far * farwnd;
	legacy_s8 far* shapebuf;
	legacy_u8 far* header;
	legacy_u16 lineofs;
	legacy_u8* lineofsptr;
	legacy_u8 far* farlineofsptr;
	legacy_u16 wnddefseg;

	(void)unk;

	wnddefseg = dos_memory_pointer_segment(&wnd_defs);

	pages = ((width * height + SHAPE2D_HEADER_SIZE) >> 4) + 1;
	shapebuf = mmgr_alloc_pages("MCGA WINDOW", pages);

	header = (legacy_u8 far*)dos_memory_make_pointer(dos_memory_pointer_segment(shapebuf), 0);
	shape2d_put_word(header + SHAPE2D_WIDTH_OFFSET, width);
	shape2d_put_word(header + SHAPE2D_HEIGHT_OFFSET, height);
	shape2d_put_word(header + SHAPE2D_POS_X_OFFSET, 0U);
	shape2d_put_word(header + SHAPE2D_POS_Y_OFFSET, 0U);
	shape2d_put_word(header + SHAPE2D_UNK1_OFFSET, 0U);
	shape2d_put_word(header + SHAPE2D_UNK2_OFFSET, 0U);

	// it is safe to read/write the pointers to next_wnd_def/wnd_defs, but not the contents
	wnd = next_wnd_def;
	nextwnd = next_wnd_def + sizeof(struct SPRITE) + height * sizeof(legacy_u16);
	if (dos_memory_pointer_offset(nextwnd) >= dos_memory_pointer_offset(&wnd_defs) + 0xE10) {
		fatal_error(aWindowdefOutOfRowTableSpa);
	}
	next_wnd_def = nextwnd;

	// get a writable far pointer to the render_window_sprite
	farwnd = dos_memory_make_pointer(wnddefseg, dos_memory_pointer_offset(wnd));

	lineofsptr = (legacy_u8*)(wnd + sizeof(struct SPRITE));
	farwnd->sprite_bitmapptr = (struct SHAPE2D far*)header;
	farwnd->sprite_lineofs = lineofsptr;
	farwnd->sprite_left = 0;
	farwnd->sprite_left2 = 0;
	farwnd->sprite_right = width;
	farwnd->sprite_pitch = width;	// ??
	farwnd->sprite_top = 0;
	farwnd->sprite_height = height;
	farwnd->sprite_width2 = width;
	farwnd->sprite_widthsum = width;

	// create a writable far pointer to the line offsets
	farlineofsptr = (legacy_u8 far*)dos_memory_make_pointer(
		wnddefseg, dos_memory_pointer_offset(lineofsptr));
	lineofs = SHAPE2D_HEADER_SIZE;
	// One of several counted loops where the original uses `loop`, which runs
	// 65536 times on a count of zero while this runs none. Reaching it needs a
	// zero-height window; the same applies to the unflip and palette loops in
	// this file.
	for (i = 0; i < height; i++) {
		shape2d_put_word(farlineofsptr, lineofs);
		farlineofsptr += 2U;
		lineofs += width;
	}

	return farwnd;
}

void sprite_free_wnd(struct SPRITE far* render_window_sprite) {
	legacy_u16 spritesize;
	// The height comes from the bitmap header, not from the SPRITE: the
	// original walks through sprite_bitmapptr to reach SHAPE2D.s2d_height.
	// sprite_make_wnd initializes both heights alike, and normal clipping edits
	// the sprite1 working copy rather than the stored window SPRITE, so using
	// the bitmap field here is structural parity rather than a clipping repair.
	spritesize = sizeof(struct SPRITE) + shape2d_get_height(
		render_window_sprite->sprite_bitmapptr) * sizeof(legacy_u16);
	if (dos_memory_pointer_offset(render_window_sprite) + spritesize != dos_memory_pointer_offset(next_wnd_def)) {
		fatal_error(aWindowReleased);
	}
	next_wnd_def = next_wnd_def - spritesize;
	mmgr_release((void far*)render_window_sprite->sprite_bitmapptr);
}

void sprite_set_1_from_argptr(struct SPRITE far* argsprite) {
	fmemcpy(&sprite1, argsprite, sizeof(struct SPRITE));
}

void sprite_copy_2_to_1(void) {
	sprite_set_1_from_argptr(&sprite2);
}

void sprite_copy_2_to_1_2(void) {
	sprite_set_1_from_argptr(&sprite2);
}

void sprite_copy_2_to_1_clear(void) {
	sprite_set_1_from_argptr(&sprite2);
	sprite_clear_1_color(0);
}

void sprite_copy_wnd_to_1(void) {
	sprite_set_1_from_argptr(render_window_sprite);
}

void sprite_copy_wnd_to_1_clear(void) {
	sprite_set_1_from_argptr(render_window_sprite);
	sprite_clear_1_color(0);
}

void sprite_copy_both_to_arg(struct SPRITE* argsprite) {
	fmemcpy(argsprite, &sprite1, sizeof(struct SPRITE) * 2);
}

void sprite_copy_arg_to_both(struct SPRITE* argsprite) {
	fmemcpy(&sprite1, argsprite, sizeof(struct SPRITE) * 2);
}

legacy_s16 sub_274B0(legacy_s16 left, legacy_s16 right, legacy_s16 top, legacy_s16 bottom)
{
	struct SPRITE saved_sprites[2];
	struct SPRITE far* window;
	legacy_u16 index;
	legacy_s16 width;
	legacy_s16 height;
	legacy_s32 required;

	width = LEGACY_S16_WRAP_SUB(right, left);
	height = LEGACY_S16_WRAP_SUB(bottom, top);
	required = ((legacy_s32)width * height) /
		((legacy_s32)video_flag1_is1 * video_flag4_is1) + 0x12L;
	if (mmgr_get_res_ofs_diff_scaled() <= (legacy_u32)required)
		return 0;

	mouse_draw_opaque_check();
	window = sprite_make_wnd((legacy_u16)width, (legacy_u16)height, 0x0FU);
	index = byte_3B8FC;
	sprite_ptrs[index] = window;
	word_4646A[index] = left;
	word_46486[index] = top;
	sprite_copy_both_to_arg(saved_sprites);
	fmemcpy(trackdata12 + index * sizeof(saved_sprites),
		saved_sprites, sizeof(saved_sprites));
	sprite_copy_2_to_1();
	sprite_clear_shape_alt(window->sprite_bitmapptr, left, top);
	byte_3B8FC++;
	return 1;
}

void sub_275C6(void)
{
	struct SPRITE saved_sprites[2];
	legacy_u16 index;

	if (byte_3B8FC == 0)
		return;
	byte_3B8FC--;
	index = byte_3B8FC;
	mouse_draw_opaque_check();
	sprite_shape_to_1(sprite_ptrs[index]->sprite_bitmapptr,
		word_4646A[index], word_46486[index]);
	fmemcpy(saved_sprites,
		trackdata12 + index * sizeof(saved_sprites),
		sizeof(saved_sprites));
	sprite_copy_arg_to_both(saved_sprites);
	sprite_free_wnd(sprite_ptrs[index]);
	mouse_draw_transparent_check();
}

void mouse_draw_opaque(void) {
	struct SPRITE saved_sprites[2];

	sprite_copy_both_to_arg(saved_sprites);
	sprite_copy_2_to_1();
	sprite_putimage(mouse_background_sprite->sprite_bitmapptr);
	sprite_copy_arg_to_both(saved_sprites);
	mouse_background_dirty = 0;
}

void mouse_draw_transparent(void) {
	struct SPRITE saved_sprites[2];
	legacy_s16 aligned_x;

	aligned_x = mouse_xpos - mouse_xpos % video_flag2_is1;
	sprite_copy_both_to_arg(saved_sprites);
	sprite_copy_2_to_1();
	sprite_clear_shape_alt(
		mouse_background_sprite->sprite_bitmapptr,
		aligned_x,
		mouse_ypos);
	sprite_putimage_and(
		mouse_medium_sprite->sprite_bitmapptr,
		mouse_xpos,
		mouse_ypos);
	sprite_putimage_or(
		mouse_small_sprite->sprite_bitmapptr,
		mouse_xpos,
		mouse_ypos);
	sprite_copy_arg_to_both(saved_sprites);
	mouse_background_dirty = 1;
}

void sprite_clear_1_color(legacy_u8 color) {

	legacy_s16 height, top, left, right, pitch, lines, width, widthdiff, i, j;
	legacy_u16 ofs;
	legacy_u8 far* bitmapptr;

	top = sprite1.sprite_top;
	left = sprite1.sprite_left;
	right = sprite1.sprite_right;
	pitch = sprite1.sprite_pitch;
	bitmapptr = (legacy_u8 far*)sprite1.sprite_bitmapptr;

	lines = sprite1.sprite_height - top;
	if (lines <= 0) return;

	ofs = LEGACY_U16_WRAP_ADD(
		shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1), (legacy_u16)top),
		(legacy_u16)left);

	width = right - left;
	if (width <= 0) return ;

	widthdiff = pitch - width;

	for (i = 0; i < lines; i++) {
		for (j = 0; j < width; j++) {
			bitmapptr[ofs ++] = color;
		}
		ofs += widthdiff;
	}
}

struct SHAPE2D_CLIP {
	legacy_u16 source;
	legacy_u16 source_advance;
	legacy_u16 destination;
	legacy_u16 destination_advance;
	legacy_u16 width;
	legacy_u16 rows;
};

static legacy_s16 shape2d_clip_blit(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y, struct SHAPE2D_CLIP* clip)
{
	legacy_u8 far* shape_bytes;
	legacy_u16 width;
	legacy_u16 height;
	legacy_u16 source;
	legacy_u16 clipped_rows;
	legacy_u16 visible;
	legacy_u16 overflow;
	legacy_u16 sprite_width;
	legacy_u16 sum;

	shape_bytes = (legacy_u8 far*)shape;
	width = shape2d_get_word(shape_bytes);
	height = shape2d_get_word(shape_bytes + 2U);
	source = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	clipped_rows = height;
	if (LEGACY_S16_FROM_BITS(y) <
		LEGACY_S16_FROM_BITS(sprite1.sprite_top)) {
		sum = LEGACY_U16_WRAP_ADD(y, clipped_rows);
		if (LEGACY_S16_FROM_BITS(sum) <=
			LEGACY_S16_FROM_BITS(sprite1.sprite_top))
			return 0;
		visible = LEGACY_U16_WRAP_SUB(sum, sprite1.sprite_top);
		overflow = LEGACY_U16_WRAP_SUB(clipped_rows, visible);
		source = LEGACY_U16_WRAP_ADD(source,
			(legacy_u16)((legacy_u32)overflow * width));
		clipped_rows = visible;
		y = sprite1.sprite_top;
		sum = LEGACY_U16_WRAP_ADD(y, clipped_rows);
		if (LEGACY_S16_FROM_BITS(sum) >
			LEGACY_S16_FROM_BITS(sprite1.sprite_height)) {
			overflow = LEGACY_U16_WRAP_SUB(
				sum, sprite1.sprite_height);
			if (LEGACY_S16_FROM_BITS(clipped_rows) <=
				LEGACY_S16_FROM_BITS(overflow))
				return 0;
			clipped_rows = LEGACY_U16_WRAP_SUB(
				clipped_rows, overflow);
		}
	} else {
		sum = LEGACY_U16_WRAP_ADD(y, clipped_rows);
		if (LEGACY_S16_FROM_BITS(sum) >
			LEGACY_S16_FROM_BITS(sprite1.sprite_height)) {
			overflow = LEGACY_U16_WRAP_SUB(
				sum, sprite1.sprite_height);
			if (LEGACY_S16_FROM_BITS(clipped_rows) <=
				LEGACY_S16_FROM_BITS(overflow))
				return 0;
			clipped_rows = LEGACY_U16_WRAP_SUB(
				clipped_rows, overflow);
		}
	}

	visible = width;
	clip->source_advance = 0;
	if (LEGACY_S16_FROM_BITS(x) <
		LEGACY_S16_FROM_BITS(sprite1.sprite_left)) {
		sum = LEGACY_U16_WRAP_ADD(x, visible);
		if (LEGACY_S16_FROM_BITS(sum) <=
			LEGACY_S16_FROM_BITS(sprite1.sprite_left))
			return 0;
		visible = LEGACY_U16_WRAP_SUB(sum, sprite1.sprite_left);
		overflow = LEGACY_U16_WRAP_SUB(width, visible);
		source = LEGACY_U16_WRAP_ADD(source, overflow);
		sprite_width = LEGACY_U16_WRAP_SUB(
			sprite1.sprite_right, sprite1.sprite_left);
		if (LEGACY_S16_FROM_BITS(sprite1.sprite_right) <=
			LEGACY_S16_FROM_BITS(sprite1.sprite_left))
			return 0;
		if (LEGACY_S16_FROM_BITS(visible) >=
			LEGACY_S16_FROM_BITS(sprite_width))
			visible = sprite_width;
		clip->source_advance = LEGACY_U16_WRAP_SUB(width, visible);
		x = sprite1.sprite_left;
	} else {
		sum = LEGACY_U16_WRAP_ADD(x, visible);
		if (LEGACY_S16_FROM_BITS(sum) >=
			LEGACY_S16_FROM_BITS(sprite1.sprite_right)) {
			overflow = LEGACY_U16_WRAP_SUB(
				sum, sprite1.sprite_right);
			if (LEGACY_S16_FROM_BITS(visible) <=
				LEGACY_S16_FROM_BITS(overflow))
				return 0;
			visible = LEGACY_U16_WRAP_SUB(visible, overflow);
			clip->source_advance = overflow;
		}
	}
	if (LEGACY_S16_FROM_BITS(visible) <= 0)
		return 0;

	clip->source = source;
	clip->destination = LEGACY_U16_WRAP_ADD(
		shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1), y), x);
	clip->destination_advance = LEGACY_U16_WRAP_SUB(
		sprite1.sprite_pitch, visible);
	clip->width = visible;
	clip->rows = clipped_rows;
	return 1;
}

struct SHAPE2D_RLE_CURSOR {
	legacy_u16 shape_segment;
	legacy_u16 source;
	legacy_u16 remaining;
	legacy_u8 value;
	legacy_s16 literal;
};

static legacy_s16 shape2d_rle_next(struct SHAPE2D_RLE_CURSOR* cursor,
	legacy_u8* value)
{
	legacy_u8 far* source_ptr;
	legacy_u8 control_bits;
	legacy_s8 control;

	if (cursor->remaining == 0) {
		source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
			cursor->shape_segment, cursor->source);
		control_bits = *source_ptr;
		cursor->source++;
		control = LEGACY_S8_FROM_BITS(control_bits);
		if (control == 0)
			return 0;
		cursor->literal = control < 0;
		if (cursor->literal != 0) {
			cursor->remaining = (legacy_u8)(0U - control_bits);
		} else {
			cursor->remaining = control_bits;
			source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				cursor->shape_segment, cursor->source);
			cursor->value = *source_ptr;
			cursor->source++;
		}
	}
	if (cursor->literal != 0) {
		source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
			cursor->shape_segment, cursor->source);
		cursor->value = *source_ptr;
		cursor->source++;
	}
	*value = cursor->value;
	cursor->remaining--;
	return 1;
}

static void shape2d_render_rle_clipped(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y)
{
	struct SHAPE2D_CLIP clip;
	struct SHAPE2D_RLE_CURSOR cursor;
	legacy_u8 far* shape_bytes;
	legacy_u8 far* bitmap;
	legacy_u16 data_start;
	legacy_u16 skip;
	legacy_u16 count;
	legacy_u16 rows;
	legacy_u16 destination;
	legacy_u16 width;
	legacy_u16 height;
	legacy_u8 value;

	if (!shape2d_clip_blit(shape, x, y, &clip))
		return;
	shape_bytes = (legacy_u8 far*)shape;
	width = shape2d_get_word(shape_bytes);
	height = shape2d_get_word(shape_bytes + 2U);
	data_start = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	if (clip.source == data_start && clip.source_advance == 0 &&
		clip.width == width && clip.rows == height) {
		shape2d_render_rle(shape, x, y, SHAPE2D_RLE_COPY);
		return;
	}
	cursor.shape_segment = dos_memory_pointer_segment(shape);
	cursor.source = data_start;
	cursor.remaining = 0;
	cursor.value = 0;
	cursor.literal = 0;
	skip = LEGACY_U16_WRAP_SUB(clip.source, data_start);
	while (skip != 0) {
		if (!shape2d_rle_next(&cursor, &value))
			return;
		skip--;
	}
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	destination = clip.destination;
	rows = clip.rows;
	do {
		count = clip.width;
		do {
			if (!shape2d_rle_next(&cursor, &value))
				return;
			bitmap[destination] = value;
			destination++;
			count--;
		} while (count != 0);
		rows--;
		if (rows == 0)
			return;
		skip = clip.source_advance;
		while (skip != 0) {
			if (!shape2d_rle_next(&cursor, &value))
				return;
			skip--;
		}
		destination = LEGACY_U16_WRAP_ADD(
			destination, clip.destination_advance);
	} while (1);
}

void shape2d_op_unk2(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	shape2d_render_rle_clipped(shape, (legacy_u16)x, (legacy_u16)y);
}

void shape2d_op_unk3(struct SHAPE2D far* shape)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle_clipped(shape,
		shape2d_get_word(shape_bytes + 8U),
		shape2d_get_word(shape_bytes + 0x0AU));
}

void nopsub_33E90(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle_clipped(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_word(shape_bytes + 4U)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_word(shape_bytes + 6U)));
}

static void sprite_putimage_at(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y)
{
	struct SHAPE2D_CLIP clip;
	legacy_u8 far* source_ptr;
	legacy_u8 far* bitmap;
	legacy_u16 shape_segment;
	legacy_u16 column_count;
	legacy_u16 row_count;
	legacy_u16 old_row_count;

	if (!shape2d_clip_blit(shape, x, y, &clip))
		return;
	shape_segment = dos_memory_pointer_segment(shape);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	row_count = clip.rows;
	do {
		column_count = clip.width;
		do {
			source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				shape_segment, clip.source);
			bitmap[clip.destination] = *source_ptr;
			clip.source++;
			clip.destination++;
			column_count--;
		} while (column_count != 0);
		clip.source = LEGACY_U16_WRAP_ADD(
			clip.source, clip.source_advance);
		clip.destination = LEGACY_U16_WRAP_ADD(
			clip.destination, clip.destination_advance);
		old_row_count = row_count;
		row_count = LEGACY_U16_WRAP_SUB(row_count, 1U);
	} while (old_row_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(row_count) > 0);
}

void sprite_putimage(struct SHAPE2D far* shape)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	sprite_putimage_at(shape,
		shape2d_get_word(shape_bytes + 8U),
		shape2d_get_word(shape_bytes + 0x0AU));
}

void nopsub_33B98(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	sprite_putimage_at(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_word(shape_bytes + 4U)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_word(shape_bytes + 6U)));
}

static void sprite_putimage_combine(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y, legacy_s16 combine_or)
{
	struct SHAPE2D_CLIP clip;
	legacy_u8 far* source_ptr;
	legacy_u8 far* bitmap;
	legacy_u16 shape_segment;
	legacy_u16 column_count;
	legacy_u16 row_count;
	legacy_u16 old_row_count;

	if (!shape2d_clip_blit(shape, x, y, &clip))
		return;
	shape_segment = dos_memory_pointer_segment(shape);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	row_count = clip.rows;
	do {
		column_count = clip.width;
		do {
			source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				shape_segment, clip.source);
			if (combine_or != 0)
				bitmap[clip.destination] |= *source_ptr;
			else
				bitmap[clip.destination] &= *source_ptr;
			clip.source++;
			clip.destination++;
			column_count--;
		} while (column_count != 0);
		clip.source = LEGACY_U16_WRAP_ADD(
			clip.source, clip.source_advance);
		clip.destination = LEGACY_U16_WRAP_ADD(
			clip.destination, clip.destination_advance);
		old_row_count = row_count;
		row_count = LEGACY_U16_WRAP_SUB(row_count, 1U);
	} while (old_row_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(row_count) > 0);
}

void sprite_putimage_and(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y)
{
	sprite_putimage_combine(shape, x, y, 0);
}

void sprite_putimage_or(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y)
{
	sprite_putimage_combine(shape, x, y, 1);
}

void sprite_putimage_and_alt(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_putimage_at(shape, (legacy_u16)x, (legacy_u16)y);
}

void sprite_putimage_and_alt2(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	sprite_putimage_combine(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_word(shape_bytes + 4U)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_word(shape_bytes + 6U)), 0);
}

void sprite_putimage_or_alt(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	sprite_putimage_combine(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_word(shape_bytes + 4U)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_word(shape_bytes + 6U)), 1);
}

void sprite_putimage_transparent(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	struct SHAPE2D_CLIP clip;
	legacy_u8 far* source_ptr;
	legacy_u8 far* bitmap;
	legacy_u16 shape_segment;
	legacy_u16 column_count;
	legacy_u16 row_count;
	legacy_u16 old_row_count;
	legacy_u8 mapped_color;

	if (!shape2d_clip_blit(shape, (legacy_u16)x, (legacy_u16)y, &clip))
		return;
	shape_segment = dos_memory_pointer_segment(shape);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	row_count = clip.rows;
	do {
		column_count = clip.width;
		do {
			source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				shape_segment, clip.source);
			mapped_color = incnums[*source_ptr];
			if (mapped_color != 0xFFU)
				bitmap[clip.destination] = mapped_color;
			clip.source++;
			clip.destination++;
			column_count--;
		} while (column_count != 0);
		clip.source = LEGACY_U16_WRAP_ADD(
			clip.source, clip.source_advance);
		clip.destination = LEGACY_U16_WRAP_ADD(
			clip.destination, clip.destination_advance);
		old_row_count = row_count;
		row_count = LEGACY_U16_WRAP_SUB(row_count, 1U);
	} while (old_row_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(row_count) > 0);
}

void setup_mcgawnd1(void) {
	if (!mcgawndsprite) {
		mcgawndsprite = sprite_make_wnd(320, 200, 0x0F);
	}

	sprite_set_1_from_argptr(&sprite2);
	sprite_putimage(mcgawndsprite->sprite_bitmapptr);
}

void setup_mcgawnd2(void) {
	if (!mcgawndsprite) {
		mcgawndsprite = sprite_make_wnd(320, 200, 0x0F);
	}

	sprite_set_1_from_argptr(mcgawndsprite);
}

static legacy_u8 far* file_get_shape2d_bytes(legacy_u8 far* memchunk,
	legacy_s16 index)
{
	return resource_file_data(memchunk, (legacy_u16)index);
}

// like locate_resource_by_index()
struct SHAPE2D far* file_get_shape2d(legacy_u8 far* memchunk,
	legacy_s16 index)
{
	return (struct SHAPE2D far*)file_get_shape2d_bytes(memchunk, index);
}

void nopsub_326BA(legacy_u8 far* memchunk, legacy_u16 index, legacy_u32* result) {
	*result = LEGACY_READ_U32_LE(resource_file_identifier(memchunk, index));
}

legacy_u16 file_get_res_shape_count(void far* memchunk) {
	return resource_file_count((const legacy_u8 far*)memchunk);
}

void file_unflip_shape2d(legacy_u8 far* memchunk, legacy_s8 far* mempages) {

	legacy_s16 shapecount, counter, width, height;
	legacy_s16 evenrows, oddrows;
	legacy_u8 far* memshape;
	legacy_s8 far* membitmapptr;
	legacy_u8 flag;
	legacy_s16 i, j;

	shapecount = resource_file_count(memchunk);
	counter = 0;
	do {
		memshape = file_get_shape2d_bytes(memchunk, counter);
		membitmapptr = (legacy_s8 far*)memshape + SHAPE2D_HEADER_SIZE;
		flag = memshape[SHAPE2D_UNK6_OFFSET];
		if ((flag & 0xF0) == 0) {
			flag = memshape[SHAPE2D_UNK5_OFFSET] >> 4;
			if (flag != 0) {
				// The original does not merely skip an unknown flip type, it
				// gives up on the whole resource, so the shapes after this
				// one are left flipped and the
				// caller is told. This port skips the shape and carries on,
				// and is declared void. Harmless: the only call site
				// (asmorig/seg034.asm:237) does `add sp, 8` and goes straight
				// on to mmgr_release without ever reading ax, and the three
				// arms below cover every flip type the resources use.
				if (flag < 4) {
					width = shape2d_get_word(
						memshape + SHAPE2D_WIDTH_OFFSET);
					height = shape2d_get_word(
						memshape + SHAPE2D_HEIGHT_OFFSET);
					switch (flag - 1) {
						case 0:
							// regular flip
							for (j = 0; j < height; j++) { // height
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[j + i * height];
								}
							}
							break;
						case 1:
							// interlaced: the even rows first, then the odd
							// ones. loc_32BBA walks the second pass with
							// dx = 1, 3, .. while dx < height, so an odd
							// height gets one fewer odd row than even rows.
							for (j = 0; j < height; j += 2) { // even rows
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[
										LEGACY_S16_DIV_OR_ZERO(j, 2) + i * height];
								}
							}
							for (j = 1; j < height; j += 2) { // odd rows
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[
										LEGACY_S16_DIV_OR_ZERO(
											LEGACY_S16_WRAP_ADD(height, j), 2) +
										i * height];
								}
							}
							break;
						case 2:
							// loc_32BDE. Even and odd rows are stored as two
							// separate column-major blocks: the even one holds
							// ceil(height/2) samples per column from offset 0,
							// the odd one holds height/2 per column starting at
							// width * ceil(height/2). The original never reloads
							// bx between the two halves of a pass, which is what
							// puts the odd rows at that offset.
							evenrows = LEGACY_S16_DIV_OR_ZERO(
								LEGACY_S16_WRAP_ADD(height, 1), 2);
							oddrows = LEGACY_S16_DIV_OR_ZERO(height, 2);
							for (j = 0; j < height; j += 2) { // even rows
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[
										LEGACY_S16_DIV_OR_ZERO(j, 2) + i * evenrows];
								}
							}
							for (j = 1; j < height; j += 2) { // odd rows
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[
										width * evenrows +
										LEGACY_S16_DIV_OR_ZERO(j, 2) + i * oddrows];
								}
							}
							break;
					}

					// copy flipped bits from mempages -> subres
					for (j = 0; j < height; j++) { // height
						for (i = 0; i < width; i++) { // width
							membitmapptr[i + j * width] = mempages[i + j * width];
						}
					}
				}
			}
		}
		counter++;
		shapecount--;
	} while (shapecount > 0);
}

void file_unflip_shape2d_pes(legacy_u8 far* memchunk, legacy_s8 far* mempages) {
	legacy_s16 shapecount, width, height, i, j, x, y;
	legacy_u8 val;
	legacy_u8 far* membitmapptr;
	legacy_u8 far* memshape;

	shapecount = file_get_res_shape_count(memchunk);

	for (i = 0; i < shapecount; ++i) {
		memshape = file_get_shape2d_bytes(memchunk, i);

		if (!(memshape[SHAPE2D_UNK6_OFFSET] & 0xF0)) {
			val = (memshape[SHAPE2D_UNK5_OFFSET] >> 4) & 0x0F;

			if (val) {
				width = shape2d_get_word(
					memshape + SHAPE2D_WIDTH_OFFSET);
				height = shape2d_get_word(
					memshape + SHAPE2D_HEIGHT_OFFSET);
				membitmapptr = memshape + SHAPE2D_HEADER_SIZE;

				for (j = 0; j < 4; ++j) {
					if (val & 0x01) {
						for (y = 0; y < height; ++y) {
							for (x = 0; x < width; ++x) {
								mempages[y * width + x] = membitmapptr[x * height + y];
							}
						}

						// Copy flipped data from mempages -> subres
						for (y = 0; y < height; ++y) {
							for (x = 0; x < width; ++x) {
								membitmapptr[y * width + x] = mempages[y * width + x];
							}
						}
					}
					membitmapptr += width * height;
					val >>= 1;
				}
			}
		}
	}
}

void file_load_shape2d_expand(legacy_u8 far* memchunk, legacy_u8 far* mempages) {
	legacy_s16 shapecount, length, i, j, k, l;
	legacy_u8 far* memchunkptr, far* mempagesptr, px, pat;
	legacy_u32 val;
	legacy_u32 product;
	legacy_u16 lowterm;
	legacy_u16 directory_prefix_size;
	legacy_u32 nextoffset;
	legacy_u8 far* srcshape;
	legacy_u8 far* dstshape;

	shapecount = file_get_res_shape_count(memchunk);

	// Skip size.
	memchunkptr = memchunk + RESOURCE_FILE_COUNT_OFFSET;
	mempagesptr = mempages + RESOURCE_FILE_COUNT_OFFSET;

	// Copy count and ids.
	directory_prefix_size = LEGACY_U16_WRAP_ADD(RESOURCE_FILE_COUNT_SIZE,
		LEGACY_U16_WRAP_MUL(shapecount,
			RESOURCE_FILE_IDENTIFIER_SIZE));
	fmemcpy(mempagesptr, memchunkptr, directory_prefix_size);
	nextoffset = 0;

	for (i = 0; i < shapecount; ++i) {
		srcshape = file_get_shape2d_bytes(memchunk, i);
		product = (legacy_u32)shape2d_get_word(
			srcshape + SHAPE2D_WIDTH_OFFSET) *
			shape2d_get_word(srcshape + SHAPE2D_HEIGHT_OFFSET);
		length = (legacy_s16)(legacy_u16)product;

		// dx:ax at this point is HIWORD(w*h) : (LOWORD(w*h)*8 + 16), each
		// half 16 bits wide and wrapping on its own - the three shl's and
		// the `add ax, size SHAPE2D` never carry into dx. Only the
		// `add ax, bx / adc dx, cx` that folds in the running offset does.
		lowterm = (legacy_u16)length * 8 + SHAPE2D_HEADER_SIZE;

		resource_file_set_offset((legacy_u8 far*)mempages,
			(legacy_u16)shapecount, (legacy_u16)i, nextoffset);
		nextoffset += (legacy_u32)lowterm
					+ ((legacy_u32)(legacy_u16)(product >> 16) << 16);

		dstshape = file_get_shape2d_bytes((legacy_u8 far*)mempages, i);
		// `mov cx, 6 / rep movsw` - the first six words only, up to and
		// including s2d_pos_y. s2d_unk3..s2d_unk6 hold the pattern and flip
		// nibbles and are deliberately left alone in the destination.
		fmemcpy(dstshape, srcshape, 6 * sizeof(legacy_u16));

		shape2d_put_word(dstshape + SHAPE2D_WIDTH_OFFSET,
			LEGACY_U16_WRAP_MUL(shape2d_get_word(
				dstshape + SHAPE2D_WIDTH_OFFSET), 8U));

		if (length && length <= 8000) {
			mempagesptr = dstshape + SHAPE2D_HEADER_SIZE;

			val = srcshape[SHAPE2D_UNK4_OFFSET] >> 4;
			val |= val << 8;

			for (j = 0; j < length * 4; ++j) {
				shape2d_put_word(mempagesptr, val);
				mempagesptr += 2U;
			}
			memchunkptr = srcshape + SHAPE2D_HEADER_SIZE;

			for (j = 0; j < 4; ++j) {
				pat = srcshape[SHAPE2D_UNK3_OFFSET + j] & 0x0F;

				if (pat) {
					mempagesptr = dstshape + SHAPE2D_HEADER_SIZE;
					for (k = 0; k < length; ++k) {
						px = *memchunkptr++;
						for (l = 0; l < 8; ++l) {
							if (px & 0x80) {
								*mempagesptr |= pat;
							}
							px <<= 1;
							mempagesptr++;
						}
					}
				}
				else {
					break;
				}
			}
		}
	}

	// Final size. The original folds this in as a 16-bit term too
	// (bx = shapecount*8 + 6, then `add ax, bx / adc dx, 0`).
	resource_file_set_size((legacy_u8 far*)mempages,
		(legacy_u32)resource_file_data_start(
			(legacy_u16)shapecount) + nextoffset);
}

legacy_u16 file_get_unflip_size(legacy_s8 far* memchunk) {
	legacy_u16 i, shapecount, size, maxsize;
	legacy_u8 far* memshape;

	shapecount = file_get_res_shape_count(memchunk);
	maxsize = 0;

	for (i = 0; i < shapecount; i++) {
		memshape = file_get_shape2d_bytes((legacy_u8 far*)memchunk, i);
		size = (shape2d_get_word(memshape + SHAPE2D_WIDTH_OFFSET) *
			shape2d_get_word(memshape + SHAPE2D_HEIGHT_OFFSET) +
			0x20) >> 4;
		if (size > maxsize)
			maxsize = size;
	}
	return maxsize;
}

legacy_u16 file_load_shape2d_expandedsize(void far* memchunk) {
	legacy_u16 shapecount, i;
	legacy_s32 size;
	legacy_u8 far* memshape;

	shapecount = file_get_res_shape_count(memchunk);

	// The original forms this seed in AX, then uses CWD: both the shift and
	// header addition wrap to 16 bits before the result is sign-extended.
	size = (legacy_s16)(legacy_u16)((shapecount * 8) +
		SHAPE2D_HEADER_SIZE);

	for (i = 0; i < shapecount; ++i) {
		memshape = file_get_shape2d_bytes((legacy_u8 far*)memchunk, i);
		// `shl ax, 3` then `sub dx, dx / adc`: the per-shape term is a
		// 16-bit value ZERO-extended into the accumulator, and the header
		// size is folded in afterwards with its own carry.
		size += (legacy_u32)(legacy_u16)(shape2d_get_word(
			memshape + SHAPE2D_WIDTH_OFFSET) * shape2d_get_word(
			memshape + SHAPE2D_HEIGHT_OFFSET) * 8)
			  + SHAPE2D_HEADER_SIZE;
	}

	return (size + SHAPE2D_HEADER_SIZE) >> 4;
}

void file_load_shape2d_palmap_init(legacy_u8 far* pal) {
	legacy_s16 i;

	for (i = 0; i < 0x10; ++i) {
		palmap[i] = pal[i];
	}
}

void file_load_shape2d_palmap_apply(legacy_u8 far* memchunk, legacy_u8 palmap[]) {
	legacy_u16 shapecount, length, i, j;
	legacy_u8 far* memchunkptr;
	legacy_u8 far* memshape;

	shapecount = file_get_res_shape_count(memchunk);

	for (i = 0; i < shapecount; ++i) {
		memshape = file_get_shape2d_bytes(memchunk, i);
		length = shape2d_get_word(memshape + SHAPE2D_WIDTH_OFFSET) *
			shape2d_get_word(memshape + SHAPE2D_HEIGHT_OFFSET);

		memchunkptr = memshape + SHAPE2D_HEADER_SIZE;

		for (j = 0; j < length; ++j) {
			// `mov bl, es:[di] / mov al, [bx+si] / stosb` - the lookup reads
			// the byte di is on, and only stosb advances di afterwards.
			*memchunkptr = palmap[*memchunkptr];
			memchunkptr++;
		}
	}
}

void far* file_load_shape2d_esh(void far* memchunk, const legacy_s8* str) {
	legacy_u16 expandedsize;
	void far* mempages;
	void far* palmapres;

	expandedsize = file_load_shape2d_expandedsize(memchunk);

	palmapres = locate_shape_nofatal(memchunk, "!MGA");

	if (palmapres) {
		file_load_shape2d_palmap_init((legacy_u8 far*)palmapres +
			SHAPE2D_HEADER_SIZE);
	}

	mempages = mmgr_alloc_pages(str, expandedsize);

	resource_file_set_size((legacy_u8 far*)mempages,
		(legacy_u32)expandedsize * 16U);

	file_load_shape2d_expand(memchunk, mempages);
	mmgr_release(memchunk);
	memchunk = mmgr_op_unk(mempages);
	file_load_shape2d_palmap_apply(memchunk, palmap);

	return memchunk;
}

void far* file_load_shape2d(const legacy_s8* shapename, legacy_s16 fatal) {
	legacy_s8 str[100];
	legacy_s8* strptr;
	legacy_s16 counter;
	void far* memchunk;
	void far* mempages;
	legacy_s16 unflipsize;

	strcpy(str, shapename);
	strptr = str;

	while (*strptr != '.' && *strptr) {
		strptr++;
	}

	if (*strptr != 0) {
		memchunk = mmgr_get_chunk_by_name(str);
		if (memchunk) return memchunk; // return existing chunk with same name
	}
	else {
		for (counter = 0; *shapeexts[counter] != 0; counter++) {
			strcpy(strptr, shapeexts[counter]);
			memchunk = mmgr_get_chunk_by_name(str);
			if (memchunk) return memchunk; // return existing chunk with same name

			if (file_find(str)) {
				break;
			}
		}
		// list exhausted: fall through to the dispatch with the last extension
		// (".ESH") still in str, like the original loc_3AA53 `jz _try_load_pvs`
	}

	if (stricmp(strptr, ".PVS") == 0) {
		memchunk = file_decomp(str, fatal);
		if (!memchunk) return dos_memory_make_pointer(0, 0);

		unflipsize = file_get_unflip_size(memchunk);
		mempages = mmgr_alloc_pages("UNFLIP", unflipsize);
		file_unflip_shape2d(memchunk, mempages);
		mmgr_release(mempages);

		return memchunk;
	}
	else if (stricmp(strptr, ".XVS") == 0) {
		return file_decomp(str, fatal);
	}
	else if (stricmp(strptr, ".PES") == 0) {
		memchunk = file_decomp(str, fatal);
		if (!memchunk) return dos_memory_make_pointer(0, 0);

		mempages = mmgr_alloc_pages("UNFLIP", 1000);
		file_unflip_shape2d_pes(memchunk, mempages);
		mmgr_release(mempages);

		return file_load_shape2d_esh(memchunk, str);
	}
	else if (stricmp(strptr, ".ESH") == 0) {
		memchunk = file_load_binary(str, fatal);
		if (!memchunk) return dos_memory_make_pointer(0, 0);

		return file_load_shape2d_esh(memchunk, str);
	}
	else { // .VSH or an explicit unknown extension
		return file_load_binary(str, fatal);
	}
}

void far* file_load_shape2d_fatal(const legacy_s8* shapename) {
	return file_load_shape2d(shapename, 1);
}

void far* file_load_shape2d_nofatal(const legacy_s8* shapename) {
	return file_load_shape2d(shapename, 0);
}

void far* file_load_shape2d_nofatal2(const legacy_s8* shapename) {
	return file_load_shape2d(shapename, 0);
}

void far* file_load_shape2d_res(const legacy_s8* resname, legacy_s16 fatal) {
	legacy_s16 chunksize;
	const legacy_s8* shapename = mmgr_path_to_name(resname);
	void far* mempages;
	void far* memchunk = mmgr_get_chunk_by_name(shapename);
	legacy_u16 freeparas, margin, rawseg;

	if (memchunk) return memchunk;

	memchunk = file_load_shape2d(shapename, fatal);
	if (!memchunk) return 0;

	chunksize = mmgr_get_chunk_size(memchunk);

	// Parsing normally needs a second buffer as large as the loaded one, and
	// the largest custom dashboards leave no room for that in the arena.
	// Upper memory is the first choice for the second buffer; only when the
	// destination really has to come out of the arena, and does not fit, is
	// the chunk grown instead so the raw data can slide up inside it and
	// parse_shape2d write downwards into the same chunk.
	//
	// That overlap is safe: both cursors run forwards with the writer
	// starting a margin below the reader, and across the stock and custom
	// cars the writer leads by at most 38% of the resource against a margin
	// of 60% or more. Compared shape by shape against the two-buffer output
	// the bytes are identical everywhere parse_shape2d writes; only the tail
	// past the last shape differs, and the parser leaves that region alone
	// in either case. What is left afterwards has the same size, position
	// and name as the two-buffer path would have produced.
	freeparas = mmgr_get_ofs_diff();
	if (freeparas < (legacy_u16)chunksize + 2 &&
		!(highpool_route(resname, (legacy_u16)chunksize) &&
		  highpool_can_fit((legacy_u16)chunksize))) {
		margin = ((legacy_u16)chunksize >> 1) + ((legacy_u16)chunksize >> 2);
		if (margin > freeparas - (freeparas >> 3))
			margin = freeparas - (freeparas >> 3);

		if (margin >= ((legacy_u16)chunksize >> 1)) {
			rawseg = dos_memory_pointer_segment(memchunk);
			mmgr_resize_memory(0, rawseg, chunksize + margin);
			copy_paras_reverse(rawseg, rawseg + margin, chunksize);
			parse_shape2d(dos_memory_make_pointer(rawseg + margin, 0), dos_memory_make_pointer(rawseg, 0));
			mmgr_resize_memory(0, rawseg, chunksize);
			mmgr_rename_chunk(dos_memory_make_pointer(rawseg, 0), resname);
			return dos_memory_make_pointer(rawseg, 0);
		}
	}

	mempages = mmgr_alloc_pages(resname, chunksize);

	parse_shape2d(memchunk, mempages);

	mmgr_release(memchunk);
	return mmgr_op_unk(mempages);
}

void far* file_load_shape2d_res_fatal(const legacy_s8* resname) {
	return file_load_shape2d_res(resname, 1);
}

void far* file_load_shape2d_res_nofatal(const legacy_s8* resname) {
	return file_load_shape2d_res(resname, 0);
}
