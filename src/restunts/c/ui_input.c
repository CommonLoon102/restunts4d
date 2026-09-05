#include "menu_internal.h"
#include "timing.h"
#include "platform.h"
#include "shape2d.h"
#include "ui_input.h"
#include "ui_text.h"

#define SPRITE_BLIT_IMMEDIATE_MODE 65534U
#define READ_LINE_CLEAR_TEXT 1U
#define READ_LINE_CURSOR_AT_START 2U
#define READ_LINE_RETAIN_INITIAL_TEXT 4U
#define READ_LINE_IGNORE_DOWN_KEY 8U
#define READ_LINE_IGNORE_TAB_KEY 16U
#define TEXT_EDIT_MINIMUM_CHARACTER 32
#define TEXT_EDIT_MAXIMUM_CHARACTER 122
#define TEXT_EDIT_CHARACTER_WIDTH 9U
#define TEXT_EDIT_INSERT_MARGIN 2U
#define TEXT_EDIT_NARROW_CURSOR_WIDTH 1U
#define TEXT_EDIT_WIDE_CURSOR_WIDTH 8U
#define TEXT_EDIT_CURSOR_BLINK_TICKS 4UL
#define SPRITE_BLIT_PHASE_COUNT 4U
#define FONT_DEFINITION_HEIGHT_OFFSET 18U
#define FONT_DEFINITION_BACKGROUND_COLOR_OFFSET 2U

static legacy_u16 text_edit_cursor_width;
static legacy_u16 text_edit_x;
static legacy_u16 text_edit_y;
static legacy_u16 text_edit_cursor_visible;
static legacy_s8* text_edit_buffer;
static legacy_u16 text_edit_max_pixels;
static legacy_u16 text_edit_cursor;

legacy_u16 audioresource_get_word(const legacy_u8 far* source);

legacy_s16 call_read_line(legacy_s8* text, legacy_s16 max_characters, legacy_s16 x, legacy_s16 y,
	legacy_u32 timeout)
{
	legacy_u16 length;
	legacy_u16 trim_index;
	legacy_u16 max_pixels;
	legacy_s16 result;

	mouse_draw_opaque_check();
	max_pixels = LEGACY_U16_WRAP_ADD(
		LEGACY_U16_WRAP_MUL(max_characters, TEXT_EDIT_CHARACTER_WIDTH),
		TEXT_EDIT_CHARACTER_WIDTH);
	result = read_line(READ_LINE_CURSOR_AT_START, text, 0, max_characters,
		max_pixels, x, y,
		&dos_kb_clear_numlock, timeout);
	mouse_draw_transparent_check();

	length = (legacy_u16)strlen(text);
	trim_index = LEGACY_U16_WRAP_SUB(length, 1U);
	while (text[trim_index] == ' ')
		trim_index = LEGACY_U16_WRAP_SUB(trim_index, 1U);
	text[LEGACY_U16_WRAP_ADD(trim_index, 1U)] = 0;
	return result;
}

legacy_s16 sprite_blit_to_video(struct SPRITE far* sprite, legacy_s16 mode)
{
	legacy_s16 result;
	legacy_u16 phase;

	sprite_copy_2_to_1_2();
	mouse_draw_opaque_check();
	if ((legacy_u16)mode == SPRITE_BLIT_IMMEDIATE_MODE) {
		sprite_putimage(sprite->sprite_bitmapptr);
		mouse_draw_transparent_check();
		return 0;
	}

	result = 0;
	for (phase = 0; phase < SPRITE_BLIT_PHASE_COUNT; ++phase) {
		result = input_do_checking((legacy_s16)timer_get_delta_alt());
		if (result != 0)
			break;
		sprite_1_unk3(sprite->sprite_bitmapptr, phase);
	}
	if (result != 0) {
		sprite_copy_2_to_1_2();
		sprite_putimage(sprite->sprite_bitmapptr);
	}
	mouse_draw_transparent_check();
	return result;
}

static void read_line_delete_character(legacy_s8* text,
	legacy_s16 max_characters)
{
	legacy_u16 index;

	index = (legacy_u16)text_edit_cursor;
	while (LEGACY_S16_FROM_BITS(index) <
		LEGACY_S16_FROM_BITS(max_characters)) {
		text[index] = text[LEGACY_U16_WRAP_ADD(index, 1U)];
		index = LEGACY_U16_WRAP_ADD(index, 1U);
	}
	text[LEGACY_U16_WRAP_SUB(max_characters, 1U)] = ' ';
	read_line_helper2();
}

static void read_line_erase_character(legacy_s8* text,
	legacy_s16 max_characters, legacy_s16 move_left)
{
	read_line_helper();
	if (move_left != 0)
		text_edit_cursor = LEGACY_U16_WRAP_SUB(text_edit_cursor, 1U);
	read_line_delete_character(text, max_characters);
	read_line_helper();
}

legacy_s16 read_line(legacy_s16 flags, legacy_s8* text, legacy_s16 initial_key, legacy_s16 max_characters,
	legacy_s16 max_pixels, legacy_s16 x, legacy_s16 y, void (far* callback)(void),
	legacy_u32 timeout)
{
	legacy_u8 input_flags;
	legacy_u16 key;
	legacy_u16 length;
	legacy_u16 index;
	legacy_u16 old_cursor_state;
	legacy_s16 insert_mode;
	legacy_s16 first_key;

	input_flags = (legacy_u8)flags;
	sprite_copy_2_to_1();
	text_edit_x = (legacy_u16)x;
	text_edit_y = (legacy_u16)y;
	text_edit_buffer = text;
	text_edit_max_pixels = (legacy_u16)max_pixels;
	text[(legacy_u16)max_characters] = 0;
	if ((input_flags & READ_LINE_CLEAR_TEXT) != 0)
		text[0] = 0;
	if ((input_flags & READ_LINE_CURSOR_AT_START) != 0)
		text_edit_cursor = 0;
	else
		text_edit_cursor = (legacy_u16)strlen(text);

	length = (legacy_u16)strlen(text);
	while (LEGACY_S16_FROM_BITS(length) <
		LEGACY_S16_FROM_BITS(max_characters)) {
		text[length] = ' ';
		length = LEGACY_U16_WRAP_ADD(length, 1U);
	}
	read_line_helper2();
	text_edit_cursor_width = TEXT_EDIT_NARROW_CURSOR_WIDTH;
	text_edit_cursor_visible = 1;
	insert_mode = 0;
	read_line_helper();
	timer_copy_counter(timeout);
	set_add_value(TEXT_EDIT_CURSOR_BLINK_TICKS);
	first_key = 1;

	for (;;) {
		if ((legacy_u16)initial_key != 0) {
			key = (legacy_u16)initial_key;
			initial_key = 0;
		} else {
			do {
				callback();
				key = (legacy_u16)kb_call_readchar_callback();
				if (key != 0)
					break;
			} while (sub_2EB07() == 0);
		}

		if (key == 0) {
			set_add_value(TEXT_EDIT_CURSOR_BLINK_TICKS);
			old_cursor_state = (legacy_u16)text_edit_cursor_visible;
			text_edit_cursor_visible = 1;
			read_line_helper();
			text_edit_cursor_visible = old_cursor_state != 0 ? 0 : 1;
			if (timeout != 0 && timer_compare_dx()) {
				read_line_helper();
				return 0;
			}
			continue;
		}

		timer_copy_counter(timeout);
		if (key == KEY_ENTER || key == KEY_ESCAPE || key == KEY_UP ||
			(key == KEY_DOWN &&
				(input_flags & READ_LINE_IGNORE_DOWN_KEY) == 0) ||
			(key == KEY_TAB &&
				(input_flags & READ_LINE_IGNORE_TAB_KEY) == 0)) {
			read_line_helper();
			return key;
		}

		if (key == KEY_RIGHT) {
			read_line_helper();
			if (LEGACY_S16_FROM_BITS(max_characters) >
				LEGACY_S16_FROM_BITS(text_edit_cursor))
				text_edit_cursor = LEGACY_U16_WRAP_ADD(text_edit_cursor, 1U);
			read_line_helper();
			first_key = 0;
			continue;
		}

		if (key == KEY_LEFT) {
			read_line_helper();
			if (text_edit_cursor != 0)
				text_edit_cursor = LEGACY_U16_WRAP_SUB(text_edit_cursor, 1U);
			read_line_helper();
			first_key = 0;
			continue;
		}

		if (key == KEY_HOME) {
			read_line_helper();
			text_edit_cursor = 0;
			read_line_helper();
			first_key = 0;
			continue;
		}

		if (key == KEY_END) {
			read_line_helper();
			text_edit_cursor = (legacy_u16)strlen(text);
			read_line_helper();
			first_key = 0;
			continue;
		}

		if (key == KEY_INSERT) {
			read_line_helper();
			insert_mode = !insert_mode;
			text_edit_cursor_width = insert_mode ?
				TEXT_EDIT_WIDE_CURSOR_WIDTH : TEXT_EDIT_NARROW_CURSOR_WIDTH;
			read_line_helper();
			first_key = 0;
			continue;
		}

		if (key == KEY_DELETE) {
			if (LEGACY_S16_FROM_BITS(max_characters) >
				LEGACY_S16_FROM_BITS(text_edit_cursor) &&
				text[(legacy_u16)text_edit_cursor] != 0)
				read_line_erase_character(text, max_characters, 0);
			first_key = 0;
			continue;
		}

		if (key == KEY_BACKSPACE) {
			if (text_edit_cursor != 0)
				read_line_erase_character(text, max_characters, 1);
			first_key = 0;
			continue;
		}

		if (LEGACY_S16_FROM_BITS(key) >= TEXT_EDIT_MINIMUM_CHARACTER &&
			LEGACY_S16_FROM_BITS(key) <= TEXT_EDIT_MAXIMUM_CHARACTER &&
			LEGACY_S16_FROM_BITS(max_characters) >
				LEGACY_S16_FROM_BITS(text_edit_cursor)) {
			read_line_helper();
			if (first_key &&
				(input_flags & READ_LINE_RETAIN_INITIAL_TEXT) == 0) {
				text_edit_cursor = 0;
				for (index = 0;
					LEGACY_S16_FROM_BITS(index) <
						LEGACY_S16_FROM_BITS(max_characters);
					index = LEGACY_U16_WRAP_ADD(index, 1U))
					text[index] = ' ';
			}

			index = (legacy_u16)text_edit_cursor;
			if (text[index] == 0)
				text[LEGACY_U16_WRAP_ADD(index, 1U)] = 0;
			if (insert_mode) {
				legacy_u16 move_index;
				move_index = LEGACY_U16_WRAP_SUB(max_characters,
					TEXT_EDIT_INSERT_MARGIN);
				while (LEGACY_S16_FROM_BITS(move_index) >=
					LEGACY_S16_FROM_BITS(text_edit_cursor)) {
					text[LEGACY_U16_WRAP_ADD(move_index, 1U)] =
						text[move_index];
					move_index = LEGACY_U16_WRAP_SUB(move_index, 1U);
				}
			}
			text[index] = (legacy_s8)(legacy_u8)key;
			if (LEGACY_S16_FROM_BITS(max_characters) >
				LEGACY_S16_FROM_BITS(text_edit_cursor))
				text_edit_cursor = LEGACY_U16_WRAP_ADD(text_edit_cursor, 1U);
			read_line_helper2();
			read_line_helper();
		}
		first_key = 0;
	}
}

void read_line_helper(void)
{
	static const legacy_s8 space[] = " ";
	legacy_u8 far* font_definition;
	legacy_u16 length;
	legacy_u16 cursor;
	legacy_u16 cursor_width;
	legacy_u16 x;
	legacy_u16 y;
	legacy_u16 color;

	if (text_edit_cursor_visible == 0)
		return;
	length = legacy_near_string_length(text_edit_buffer);
	cursor = (legacy_u16)text_edit_cursor;
	if (LEGACY_S16_FROM_BITS(length) < LEGACY_S16_FROM_BITS(cursor)) {
		cursor = length;
		text_edit_cursor = cursor;
	}
	cursor_width = (legacy_u16)font_op(text_edit_buffer + cursor, 1);
	if (cursor_width == 0)
		cursor_width = (legacy_u16)font_op2(space);
	x = LEGACY_U16_WRAP_ADD(font_op(text_edit_buffer, cursor), text_edit_x);
	font_definition = active_font_definition;
	y = LEGACY_U16_WRAP_ADD(
		audioresource_get_word(font_definition +
			FONT_DEFINITION_HEIGHT_OFFSET), text_edit_y);
	y = LEGACY_U16_WRAP_SUB(y, text_edit_cursor_width);
	color = audioresource_get_word(font_definition);
	sub_35B76(LEGACY_S16_FROM_BITS(x), LEGACY_S16_FROM_BITS(y),
		LEGACY_S16_FROM_BITS(cursor_width),
		LEGACY_S16_FROM_BITS(text_edit_cursor_width),
		LEGACY_S16_FROM_BITS(color));
}

void read_line_helper2(void)
{
	legacy_u8 far* font_definition;
	legacy_u16 length;
	legacy_u16 text_width;
	legacy_u16 remaining_width;

	if (text_edit_max_pixels != 0) {
		while (LEGACY_S16_FROM_BITS(font_op2(text_edit_buffer)) >
			LEGACY_S16_FROM_BITS(text_edit_max_pixels)) {
			length = legacy_near_string_length(text_edit_buffer);
			if (length == 0)
				break;
			text_edit_buffer[length - 1U] = 0;
		}
	}
	length = legacy_near_string_length(text_edit_buffer);
	if (LEGACY_S16_FROM_BITS(length) <
		LEGACY_S16_FROM_BITS(text_edit_cursor))
		text_edit_cursor = length;
	sub_345BC(text_edit_buffer, LEGACY_S16_FROM_BITS(text_edit_x),
		LEGACY_S16_FROM_BITS(text_edit_y));
	if (text_edit_max_pixels == 0)
		return;

	text_width = (legacy_u16)font_op2(text_edit_buffer);
	remaining_width = LEGACY_U16_WRAP_SUB(text_edit_max_pixels, text_width);
	if (LEGACY_S16_FROM_BITS(remaining_width) <= 0)
		return;
	font_definition = active_font_definition;
	sprite_1_unk2(LEGACY_S16_FROM_BITS(
			LEGACY_U16_WRAP_ADD(text_width, text_edit_x)),
		LEGACY_S16_FROM_BITS(text_edit_y),
		LEGACY_S16_FROM_BITS(remaining_width),
		LEGACY_S16_FROM_BITS(
			audioresource_get_word(font_definition +
				FONT_DEFINITION_HEIGHT_OFFSET)),
		LEGACY_S16_FROM_BITS(
			audioresource_get_word(font_definition +
				FONT_DEFINITION_BACKGROUND_COLOR_OFFSET)));
}
