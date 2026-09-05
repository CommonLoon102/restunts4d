#include "menu_internal.h"
#include "ui_text.h"

legacy_u8 far* active_font_definition;

legacy_u16 audioresource_get_word(const legacy_u8 far* source);

#define UI_SCREEN_WIDTH 320
#define FONT_FIXED_GLYPH_WIDTH_OFFSET 16U
#define FONT_GLYPH_WIDTHS_FLAG_OFFSET 20U
#define FONT_GLYPH_OFFSET_TABLE_OFFSET 22U
#define FONT_GLYPH_OFFSET_ENTRY_SIZE 2U

legacy_s16 font_op2_alt(const legacy_s8* text)
{
	legacy_s16 centered;

	centered = LEGACY_S16_WRAP_NEGATE(
		LEGACY_S16_WRAP_SUB(font_op2(text), UI_SCREEN_WIDTH));
	return LEGACY_S16_DIV_OR_ZERO(centered, 2);
}

legacy_u16 legacy_near_string_length(const legacy_s8* text)
{
	legacy_u16 length;

	length = 0;
	while (*text++ != 0)
		length = LEGACY_U16_WRAP_ADD(length, 1U);
	return length;
}

void print_int_as_string_maybe(legacy_s8* destination, legacy_s16 value, legacy_s16 zero_pad,
	legacy_s16 width)
{
	legacy_s8 digits[5];
	legacy_s16 signed_value;
	legacy_u16 magnitude;
	legacy_u16 digit_count;
	legacy_u16 length;
	legacy_u16 index;

	signed_value = LEGACY_S16_FROM_BITS((legacy_u16)value);
	magnitude = signed_value < 0 ?
		(legacy_u16)(0U - (legacy_u16)signed_value) :
		(legacy_u16)signed_value;
	digit_count = 0;
	do {
		digits[digit_count++] = (legacy_s8)('0' + magnitude % 10U);
		magnitude = LEGACY_U16_DIV_OR_ZERO(magnitude, 10U);
	} while (magnitude != 0);

	index = 0;
	if (signed_value < 0)
		destination[index++] = '-';
	while (digit_count != 0)
		destination[index++] = digits[--digit_count];
	destination[index] = 0;
	length = index;

	if (width != 0) {
		while (LEGACY_S16_FROM_BITS((legacy_u16)width) <
			LEGACY_S16_FROM_BITS(length)) {
			for (index = 0; index < length; index++)
				destination[index] = destination[index + 1U];
			length--;
		}
		while (LEGACY_S16_FROM_BITS((legacy_u16)width) >
			LEGACY_S16_FROM_BITS(length)) {
			index = length;
			do {
				destination[index + 1U] = destination[index];
			} while (index-- != 0);
			destination[0] = ' ';
			length++;
		}
	}
	if (zero_pad != 0) {
		index = 0;
		while (destination[index] == ' ')
			destination[index++] = '0';
	}
}

static legacy_s8* legacy_near_string_copy(legacy_s8* destination, const legacy_s8* source)
{
	while ((*destination = *source) != 0) {
		destination++;
		source++;
	}
	return destination;
}

void format_frame_as_string(legacy_s8* destination, legacy_s16 frame_count,
	legacy_s16 include_hundredths)
{
	legacy_s8 number[18];
	legacy_s8* output;
	legacy_u16 frames;
	legacy_u16 frame_rate;
	legacy_u16 frames_per_minute;
	legacy_u16 minutes;
	legacy_u16 seconds;
	legacy_u16 hundredths;

	frames = (legacy_u16)frame_count;
	frame_rate = (legacy_u16)framespersec;
	frames_per_minute = LEGACY_U16_WRAP_MUL(60U, frame_rate);
	minutes = LEGACY_U16_DIV_OR_ZERO(frames, frames_per_minute);
	frames = LEGACY_U16_WRAP_SUB(frames,
		LEGACY_U16_WRAP_MUL(frames_per_minute, minutes));
	seconds = LEGACY_U16_DIV_OR_ZERO(frames, frame_rate);
	frames = LEGACY_U16_WRAP_SUB(frames,
		LEGACY_U16_WRAP_MUL(frame_rate, seconds));

	print_int_as_string_maybe(number, minutes, 0, 2);
	output = legacy_near_string_copy(destination, number);
	*output++ = ':';
	print_int_as_string_maybe(number, seconds, 1, 2);
	output = legacy_near_string_copy(output, number);
	if (include_hundredths != 0) {
		*output++ = '.';
		hundredths = LEGACY_U16_WRAP_MUL(
			LEGACY_U16_DIV_OR_ZERO(100U, frame_rate), frames);
		print_int_as_string_maybe(number, hundredths, 1, 2);
		legacy_near_string_copy(output, number);
	}
}

void parse_filepath_separators(legacy_s8* destination, const legacy_s8* path)
{
	legacy_u16 path_index;
	legacy_u16 output_index;
	legacy_s8 current;

	path_index = legacy_near_string_length(path);
	while (path_index != 0) {
		current = path[path_index - 1U];
		if (current == '\\' || current == ':')
			break;
		path_index--;
	}
	output_index = 0;
	do {
		current = path[path_index++];
		destination[output_index++] = current;
	} while (current != '.');
	destination[output_index - 1U] = 0;
}

void font_set_unk(legacy_s16 color, legacy_s16 unknown)
{
	legacy_u8 far* font_definition;

	font_definition = active_font_definition;
	font_definition[0] = (legacy_u8)color;
	font_definition[1] = 0;
	font_definition[2] = (legacy_u8)unknown;
	font_definition[3] = 0;
}

struct RECTANGLE* intro_draw_text(legacy_s8* text, legacy_s16 x, legacy_s16 y, legacy_s16 color,
	legacy_s16 shadow_color)
{
	word_42248.left = LEGACY_S16_FROM_BITS((legacy_u16)x);
	word_42248.right = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_ADD(x, font_op2(text)), 1);
	word_42248.top = LEGACY_S16_FROM_BITS((legacy_u16)y);
	word_42248.bottom = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_ADD(y, fontdef_unk_0E), 1);
	font_set_unk(shadow_color, 0);
	font_draw_text(text, LEGACY_S16_WRAP_ADD(x, 1),
		LEGACY_S16_WRAP_ADD(y, 1));
	font_set_unk(color, 0);
	font_draw_text(text, LEGACY_S16_FROM_BITS((legacy_u16)x),
		LEGACY_S16_FROM_BITS((legacy_u16)y));
	return &word_42248;
}

static legacy_s16 font_measure(const legacy_s8* text, legacy_u16 remaining, legacy_s16 bounded)
{
	legacy_u8 far* font_definition;
	legacy_u16 glyph_offset;
	legacy_u16 glyph_width;
	legacy_u16 total_width;
	legacy_u8 character;
	legacy_u8 has_glyph_widths;

	if (bounded != 0 && remaining == 0)
		return 0;
	font_definition = active_font_definition;
	has_glyph_widths = font_definition[FONT_GLYPH_WIDTHS_FLAG_OFFSET];
	glyph_width = audioresource_get_word(
		font_definition + FONT_FIXED_GLYPH_WIDTH_OFFSET);
	total_width = 0;
	while ((character = (legacy_u8)*text++) != 0) {
		glyph_offset = audioresource_get_word(
			font_definition + FONT_GLYPH_OFFSET_TABLE_OFFSET +
			(legacy_u16)character * FONT_GLYPH_OFFSET_ENTRY_SIZE);
		if (glyph_offset == 0)
			continue;
		if (has_glyph_widths != 0)
			glyph_width = font_definition[glyph_offset];
		total_width = LEGACY_U16_WRAP_ADD(total_width, glyph_width);
		remaining--;
		if (remaining == 0)
			break;
	}
	return LEGACY_S16_FROM_BITS(total_width);
}

legacy_s16 font_op(const legacy_s8* text, legacy_s16 glyph_count)
{
	return font_measure(text, (legacy_u16)glyph_count, 1);
}

legacy_s16 font_op2(const legacy_s8* text)
{
	return font_measure(text, 0, 0);
}
