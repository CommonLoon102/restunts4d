#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "resource.h"
#include "shape2d.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "timing.h"
#include "ui_input.h"
#include "game_input.h"

#define RST_ASC_CHAR_UPPER 1
#define RST_ASC_CHAR_LOWER 2
#define RST_ASC_CONTROL    32
#define ASCII_CASE_OFFSET 32U
#define DIALOG_DEFAULT_WIDTH 32
#define DIALOG_WIDTH_PADDING 24U
#define DIALOG_ALIGNMENT_MASK 65528U
#define DIALOG_SCREEN_WIDTH 320
#define DIALOG_SCREEN_HEIGHT 200
#define DIALOG_CENTER_DIVISOR 2
#define DIALOG_CONTENT_WIDTH_REDUCTION 16
#define DIALOG_NO_SELECTION 255U
#define FILE_DIALOG_SEPARATOR_WIDTH 171
#define FILE_DIALOG_LIST_WIDTH 162
#define FILE_DIALOG_DIRECTORY_MAX_LENGTH 18
#define DIALOG_INPUT_TIMEOUT 30000UL
#define SECURITY_DIALOG_Y 120U
#define DIALOG_LINE_BUFFER_CAPACITY 128
#define DIALOG_CHOICE_BUFFER_CAPACITY 80
#define DIALOG_CHOICE_CAPACITY 20
#define DIALOG_LINE_HEIGHT_PADDING 2
#define DIALOG_PARAGRAPH_HEIGHT 4
#define DIALOG_CHOICE_PARAGRAPH_HEIGHT 3
#define DIALOG_PLACEHOLDER_POSITION_STRIDE 2U
#define DIALOG_DELAY_TICKS 8UL

static const legacy_u8 far quiz_question_suffixes[20] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'
};
static const legacy_u8 far g_ascii_props[256] = {
	32, 32, 32, 32, 32, 32, 32, 32, 32, 40, 40, 40, 40, 40, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	72, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 16, 16, 16, 16, 16, 16,
	16, 129, 129, 129, 129, 129, 129, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 16, 16, 16, 16, 16,
	16, 130, 130, 130, 130, 130, 130, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 16, 16, 16, 16, 32,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void sprite_xor_rect_outline(legacy_s16 left, legacy_s16 top, legacy_s16 right, legacy_s16 bottom, legacy_s16 color)
{
	legacy_s16 x;
	legacy_s16 y;
	legacy_s16 width;
	legacy_s16 height;

	x = LEGACY_S16_FROM_BITS(left);
	y = LEGACY_S16_FROM_BITS(top);
	width = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_SUB(right, left), 1);
	height = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_SUB(bottom, top), 1);
	if (width > 0) {
		sprite_xor_rect_clipped(x, y, width, 1, color);
		sprite_xor_rect_clipped(x, LEGACY_S16_FROM_BITS(bottom), width, 1, color);
	}
	if (height > 0) {
		y = LEGACY_S16_WRAP_ADD(y, 1);
		sprite_xor_rect_clipped(x, y, 1, height, color);
		sprite_xor_rect_clipped(LEGACY_S16_FROM_BITS(right), y,
			1, height, color);
	}
}

static legacy_u16 dialog_ascii_lower(legacy_u16 character)
{
	if (character < 256U &&
		(g_ascii_props[character] & RST_ASC_CHAR_UPPER) != 0)
		character = LEGACY_U16_WRAP_ADD(character, ASCII_CASE_OFFSET);
	return character;
}

static legacy_u16 dialog_finish(legacy_s16 result,
	legacy_s16 save_background)
{
	if (save_background != 0)
		sprite_pop_background();
	return (legacy_u16)result;
}

static legacy_s16 dialog_advance_height(legacy_s16 dialog_height,
	legacy_s16 line_height, legacy_u8 separator,
	legacy_s16 paragraph_height)
{
	if (separator == ']')
		return LEGACY_S16_WRAP_ADD(dialog_height, line_height);
	return LEGACY_S16_WRAP_ADD(dialog_height, paragraph_height);
}

legacy_u16 show_dialog(
	legacy_s16 dialog_type,
	legacy_s16 save_background,
	void far* text_resource,
	legacy_u16 x_argument,
	legacy_u16 y_argument,
	legacy_s16 border_color,
	legacy_s16* disabled_choices,
	legacy_s16 initial_choice
) {
	legacy_s8 line_buffer[DIALOG_LINE_BUFFER_CAPACITY];
	legacy_s8 choice_buffer[DIALOG_CHOICE_BUFFER_CAPACITY];
	legacy_s8 far* choice_texts[DIALOG_CHOICE_CAPACITY];
	legacy_u8 choice_lengths[DIALOG_CHOICE_CAPACITY];
	struct BUTTON_AREA choices[DIALOG_CHOICE_CAPACITY];
	legacy_s8 far* cursor;
	legacy_s16 line_height;
	legacy_s16 dialog_width;
	legacy_s16 dialog_height;
	legacy_s16 measured_width;
	legacy_s16 x;
	legacy_s16 y;
	legacy_s16 left;
	legacy_s16 right;
	legacy_s16 top;
	legacy_s16 bottom;
	legacy_s16 result;
	legacy_u16 line_length;
	legacy_u16 choice_width;
	legacy_u16 character_count;
	legacy_u16 input;
	legacy_u16 first_hotkey;
	legacy_u16 second_hotkey;
	legacy_u16 index;
	legacy_u16 copied;
	legacy_s16 hit;
	legacy_u8 character;
	legacy_u8 choice_count;
	legacy_u8 placeholder_index;
	legacy_u8 selected;
	legacy_u8 previous;
	legacy_u8 active;

	line_height = LEGACY_S16_WRAP_ADD(font_glyph_height,
		DIALOG_LINE_HEIGHT_PADDING);
	dialog_height = 0;
	dialog_width = DIALOG_DEFAULT_WIDTH;
	mouse_draw_opaque_check();

	cursor = (legacy_s8 far*)text_resource;
	line_length = 0;
	while ((character = (legacy_u8)*cursor) != 0) {
		if (character == ']' || character == '}') {
			line_buffer[line_length] = 0;
			measured_width = (legacy_s16)font_text_width(line_buffer);
			if (measured_width > dialog_width)
				dialog_width = measured_width;
			line_length = 0;
			dialog_height = dialog_advance_height(dialog_height,
				line_height, character, DIALOG_PARAGRAPH_HEIGHT);
		} else {
			line_buffer[line_length++] = (legacy_s8)character;
		}
		cursor++;
	}

	dialog_width = LEGACY_S16_FROM_BITS(
		LEGACY_U16_WRAP_ADD((legacy_u16)dialog_width,
			DIALOG_WIDTH_PADDING) & DIALOG_ALIGNMENT_MASK);
	x = LEGACY_S16_FROM_BITS(x_argument);
	y = LEGACY_S16_FROM_BITS(y_argument);
	if (x == -1) {
		x = LEGACY_S16_DIV_OR_ZERO(
			LEGACY_S16_WRAP_SUB(DIALOG_SCREEN_WIDTH, dialog_width),
			DIALOG_CENTER_DIVISOR);
		x = LEGACY_S16_FROM_BITS(
			(legacy_u16)x & DIALOG_ALIGNMENT_MASK);
	}
	if (y == -1)
		y = LEGACY_S16_DIV_OR_ZERO(
			LEGACY_S16_WRAP_SUB(DIALOG_SCREEN_HEIGHT, dialog_height),
			DIALOG_CENTER_DIVISOR);

	left = x;
	right = LEGACY_S16_WRAP_ADD(x, dialog_width);
	top = LEGACY_S16_WRAP_SUB(y, 8);
	bottom = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_ADD(y, dialog_height), 8);
	x = LEGACY_S16_WRAP_ADD(x, 8);
	dialog_width = LEGACY_S16_WRAP_SUB(dialog_width,
		DIALOG_CONTENT_WIDTH_REDUCTION);
	if (save_background != 0 &&
		sprite_push_background(left, right, top, bottom) == 0)
		return DIALOG_FAILURE_RESULT;

	sprite_select_screen();
	sprite_set_target_clip_bounds(left, right, top, bottom);
	sprite_clear_target(0);
	sprite_draw_rect_outline(LEGACY_S16_WRAP_SUB(x, 4),
		LEGACY_S16_WRAP_SUB(y, 4),
		LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(x, dialog_width), 4),
		LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(y, dialog_height), 4),
		border_color);
	font_set_colors(dialog_fnt_colour, 0);
	dialog_background_color = 0;
	font_set_colors(dialog_fnt_colour, 0);

	cursor = (legacy_s8 far*)text_resource;
	line_length = 0;
	placeholder_index = 0;
	dialog_height = 1;
	while ((character = (legacy_u8)*cursor) != 0 && character != '[') {
		if (character == ']' || character == '}') {
			line_buffer[line_length] = 0;
			font_draw_text_opaque(line_buffer, x,
				LEGACY_S16_WRAP_ADD(y, dialog_height));
			line_length = 0;
			dialog_height = dialog_advance_height(dialog_height,
				line_height, character, DIALOG_PARAGRAPH_HEIGHT);
		} else if (character == '@') {
			if (dialog_type == DIALOG_TYPE_PLACEHOLDERS) {
				line_buffer[line_length] = 0;
				disabled_choices[placeholder_index] =
					LEGACY_S16_WRAP_ADD(x,
						(legacy_s16)font_text_width(line_buffer));
				disabled_choices[placeholder_index + 1U] =
					LEGACY_S16_WRAP_ADD(y, dialog_height);
				placeholder_index = (legacy_u8)(placeholder_index +
					DIALOG_PLACEHOLDER_POSITION_STRIDE);
			}
			line_buffer[line_length++] = ' ';
		} else {
			line_buffer[line_length++] = (legacy_s8)character;
		}
		cursor++;
	}

	choice_count = 0;
	while ((legacy_u8)*cursor == '[') {
		cursor++;
		choice_texts[choice_count] = cursor;
		line_buffer[line_length] = 0;
		choices[choice_count].x1 = LEGACY_S16_WRAP_ADD(
			x, (legacy_s16)font_text_width(line_buffer));
		choices[choice_count].y1 = LEGACY_S16_WRAP_ADD(y, dialog_height);
		choices[choice_count].y2 = LEGACY_S16_WRAP_ADD(
			choices[choice_count].y1, line_height);
		line_buffer[line_length++] = ' ';
		choice_width = 0;
		character_count = 0;
		while ((character = (legacy_u8)*cursor) != 0 && character != '[') {
			if (character == ']' || character == '}') {
				line_buffer[line_length] = 0;
				choice_width = (legacy_u16)font_text_width(line_buffer);
				line_length = 0;
				dialog_height = dialog_advance_height(dialog_height,
					line_height, character,
					DIALOG_CHOICE_PARAGRAPH_HEIGHT);
			} else {
				line_buffer[line_length++] = (legacy_s8)character;
				character_count++;
			}
			cursor++;
		}
		choice_lengths[choice_count] = (legacy_u8)character_count;
		line_buffer[line_length] = 0;
		if (choice_width == 0)
			choice_width = (legacy_u16)font_text_width(line_buffer);
		choices[choice_count].x2 = LEGACY_S16_WRAP_ADD(
			choices[choice_count].x1, (legacy_s16)choice_width);
		choice_count++;
	}

	if (choice_count > 2U &&
		choices[0].x1 == choices[1].x1 &&
		choices[1].x1 == choices[2].x1) {
		for (index = 0; index < choice_count; index++) {
			choices[index].x2 = LEGACY_S16_WRAP_ADD(
				choices[index].x1, dialog_width);
		}
	}
	mouse_draw_transparent_check();

	result = 1;
	if (dialog_type == DIALOG_TYPE_MESSAGE)
		return 0;
	if (dialog_type == DIALOG_TYPE_ACKNOWLEDGEMENT) {
		do {
			input = (legacy_u16)input_checking(
				(legacy_s16)timer_get_delta_alt());
		} while (input == 0);
		if (input == KEY_ESCAPE)
			result = 0;
		check_input();
		return dialog_finish(result, save_background);
	}
	if (dialog_type == DIALOG_TYPE_PLACEHOLDERS)
		return LEGACY_U16_DIV_OR_ZERO(placeholder_index,
			DIALOG_PLACEHOLDER_POSITION_STRIDE);
	if (dialog_type == DIALOG_TYPE_DELAY) {
		(void)slow_timer_wait_ticks(DIALOG_DELAY_TICKS);
		return dialog_finish(result, save_background);
	}
	if (dialog_type != DIALOG_TYPE_MENU)
		return dialog_finish(result, save_background);

	selected = (legacy_u8)initial_choice;
	previous = DIALOG_NO_SELECTION;
	(void)timer_get_delta_alt();
	mouse_draw_opaque_check();
	first_hotkey = 0;
	second_hotkey = 0;
	if (choice_count == 2U) {
		cursor = choice_texts[0];
		do {
			first_hotkey = (legacy_u8)*cursor++;
		} while (first_hotkey == ' ');
		first_hotkey = dialog_ascii_lower(first_hotkey);
		cursor = choice_texts[1];
		do {
			second_hotkey = (legacy_u8)*cursor++;
		} while (second_hotkey == ' ');
		second_hotkey = dialog_ascii_lower(second_hotkey);
	}

	active = 1;
	while (active != 0) {
		if (selected != previous) {
			mouse_draw_opaque_check();
			for (index = 0; index < choice_count; index++) {
				if (selected == (legacy_u8)index)
					font_set_colors(dialog_background_color, dialog_fnt_colour);
				else
					font_set_colors(dialog_fnt_colour, dialog_background_color);
				if (disabled_choices != 0 && disabled_choices[index] != 0)
					font_set_colors(performGraphColor, dialog_background_color);
				for (copied = 0; copied < choice_lengths[index]; copied++)
					choice_buffer[copied] = choice_texts[index][copied];
				choice_buffer[copied] = 0;
				font_draw_text_opaque(choice_buffer, choices[index].x1,
					choices[index].y1);
			}
			mouse_draw_transparent_check();
			if (previous == DIALOG_NO_SELECTION)
				check_input();
			previous = selected;
		}

		input = (legacy_u16)input_checking((legacy_s16)timer_get_delta_alt());
		hit = (legacy_s16)mouse_multi_hittest(choice_count,
			choices);
		if (hit != -1 &&
			(disabled_choices == 0 || disabled_choices[hit] == 0))
			selected = (legacy_u8)hit;

		if (choice_count == 2U && input != 0) {
			input = dialog_ascii_lower(input);
			if (input == first_hotkey) {
				selected = 0;
				input = KEY_ENTER;
			} else if (input == second_hotkey) {
				selected = 1;
				input = KEY_ENTER;
			}
		}

		if (input == 0)
			continue;
		if (input == KEY_SPACE || input == KEY_ENTER) {
			active = 0;
			check_input();
			continue;
		}
		if (input == KEY_ESCAPE) {
			selected = DIALOG_NO_SELECTION;
			active = 0;
			check_input();
			continue;
		}
		if (input == KEY_UP || input == KEY_LEFT) {
			do {
				selected = selected == 0 ?
					(legacy_u8)(choice_count - 1U) :
					(legacy_u8)(selected - 1U);
			} while (disabled_choices != 0 &&
				disabled_choices[selected] != 0);
			continue;
		}
		if (input == KEY_RIGHT || input == KEY_DOWN) {
			do {
				selected = (legacy_u8)(selected + 1U);
				if (selected >= choice_count)
					selected = 0;
			} while (disabled_choices != 0 &&
				disabled_choices[selected] != 0);
		}
	}
	result = LEGACY_S8_FROM_BITS(selected);

	return dialog_finish(result, save_background);
}

legacy_s8 do_fileselect_dialog(
	legacy_s8* directory,
	legacy_s8* filename,
	legacy_s8* extension,
	legacy_s8 far* prompt
) {
	legacy_s16 positions[40];
	struct BUTTON_AREA hit_areas[10];
	legacy_s8 filenames[128][13];
	const legacy_s8* found_path;
	legacy_u16 index;
	legacy_u16 compare_index;
	legacy_u16 visible_row;
	legacy_u16 text_width;
	legacy_u16 key;
	legacy_s16 dialog_result;
	legacy_s16 hit;
	legacy_s16 candidate;
	legacy_s8 selected;
	legacy_s8 scroll;
	legacy_s8 previous_selected;
	legacy_s8 previous_scroll;
	legacy_s8 result;
	legacy_u8 file_count;
	legacy_u8 saved_busy;
	legacy_u8 character;
	legacy_u8 search_again;

	dialog_result = LEGACY_S16_FROM_BITS(show_dialog(
		DIALOG_TYPE_PLACEHOLDERS, DIALOG_SAVE_BACKGROUND,
		locate_text_res(mainresptr, file_load_dialog_id),
		DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
		dialog_border_color, positions, 0));
	if (dialog_result < 0)
		return 0;

	saved_busy = g_is_busy;
	g_is_busy = 1;
	preRender_line(positions[4] - 4, positions[5] + 4,
		positions[4] + FILE_DIALOG_SEPARATOR_WIDTH,
		positions[5] + 4, dialog_border_color);
	font_set_colors(dialog_fnt_colour, dialog_background_color);
	copy_string(&resID_byte1, prompt);
	font_draw_text_opaque(&resID_byte1, positions[0], positions[1]);

	for (index = 0; index < 10U; index++) {
		hit_areas[index].x1 = positions[2];
		hit_areas[index].x2 = positions[2] + FILE_DIALOG_LIST_WIDTH;
		if (index == 9U)
			hit_areas[index].y1 = hit_areas[index - 1U].y1 + 10;
		else
			hit_areas[index].y1 = positions[3U + index * 2U];
		hit_areas[index].y2 = hit_areas[index].y1 + 10;
	}
	font_set_colors(dialog_fnt_colour, dialog_background_color);
	font_draw_text_opaque(directory, positions[2], positions[3]);

	for (;;) {
	mouse_draw_transparent_check();
	file_count = 0;
	found_path = file_combine_and_find(directory, "*", extension);
	if (found_path == 0) {
		font_set_colors(dialog_fnt_colour, dialog_background_color);
		key = (legacy_u16)call_read_line(directory,
			FILE_DIALOG_DIRECTORY_MAX_LENGTH, positions[2], positions[3],
			DIALOG_INPUT_TIMEOUT);
		if (key == KEY_ESCAPE) {
			result = 0;
			break;
		}
		continue;
	}

	parse_filepath_separators(filenames[file_count++], found_path);
	while (file_count < 128U && (found_path = file_find_next_alt()) != 0)
		parse_filepath_separators(filenames[file_count++], found_path);

	for (index = 0; index + 1U < file_count; index++) {
		for (compare_index = index + 1U;
			compare_index < file_count; compare_index++) {
			if (strcmp(filenames[index], filenames[compare_index]) > 0) {
				strcpy(&resID_byte1, filenames[index]);
				strcpy(filenames[index], filenames[compare_index]);
				strcpy(filenames[compare_index], &resID_byte1);
			}
		}
	}

	if (file_count > 7U) {
		copy_string(&resID_byte1, locate_text_res(mainresptr, file_scroll_up_label_id));
		font_draw_text_opaque(&resID_byte1, font_centered_text_x(&resID_byte1), hit_areas[1].y1);
		copy_string(&resID_byte1, locate_text_res(mainresptr, file_scroll_down_label_id));
		font_draw_text_opaque(&resID_byte1, font_centered_text_x(&resID_byte1),
			hit_areas[9].y1 - 1);
	}

	selected = 0;
	scroll = 0;
	previous_selected = -1;
	previous_scroll = -1;
	(void)timer_get_delta_alt();
	result = 0;
	search_again = 0;
	for (;;) {
		if (selected != previous_selected || scroll != previous_scroll) {
			previous_selected = selected;
			previous_scroll = scroll;
			mouse_draw_opaque_check();
			for (visible_row = 0; visible_row < 7U; visible_row++) {
				candidate = (legacy_s16)(scroll + (legacy_s16)visible_row);
				if (candidate == selected)
					font_set_colors(dialog_background_color, dialog_fnt_colour);
				else
					font_set_colors(dialog_fnt_colour, dialog_background_color);
				if (candidate < (legacy_s16)file_count) {
					strcpy(&resID_byte1, filenames[(legacy_u8)candidate]);
					font_draw_text_opaque(&resID_byte1, positions[2],
						hit_areas[visible_row + 2U].y1);
				} else {
					font_draw_text_opaque("        ", positions[2],
						hit_areas[visible_row + 2U].y1);
				}
				text_width = (legacy_u16)font_text_width(&resID_byte1);
				sprite_fill_rect(positions[2] + text_width,
					hit_areas[visible_row + 2U].y1,
					positions[2] + FILE_DIALOG_LIST_WIDTH - text_width -
						positions[2],
					8, dialog_background_color);
			}
			mouse_draw_transparent_check();
		}

		key = (legacy_u16)input_checking((legacy_s16)timer_get_delta_alt());
		hit = (legacy_s16)mouse_multi_hittest(10,
			hit_areas);
		if (hit != -1) {
			if (hit == 0) {
				if ((mouse_butstate & 3U) != 0) {
					selected = 0;
					scroll = -1;
					key = 0;
				}
			} else if (hit == 1) {
				if ((mouse_butstate & 3U) != 0) {
					if ((legacy_s16)selected + scroll != 0)
						selected--;
					if (selected < scroll)
						scroll = selected;
					key = 0;
				}
			} else if (hit == 9) {
				if ((mouse_butstate & 3U) != 0) {
					if (selected != (legacy_s8)(file_count - 1U))
						selected++;
					key = 0;
				}
			} else {
				candidate = (legacy_s16)(scroll + hit - 2);
				if (candidate < (legacy_s16)file_count)
					selected = (legacy_s8)candidate;
			}
		}

		if (key == KEY_ENTER || key == KEY_SPACE) {
			result = 1;
		} else if (key == KEY_ESCAPE) {
			result = -1;
		} else if (key == KEY_UP) {
			selected--;
		} else if (key == KEY_DOWN) {
			if (selected != (legacy_s8)(file_count - 1U))
				selected++;
		} else if (key < 256U &&
			(g_ascii_props[key] &
				(RST_ASC_CHAR_UPPER | RST_ASC_CHAR_LOWER)) != 0) {
			character = (legacy_u8)dialog_ascii_lower(key);
			for (index = 0; index < file_count; index++) {
				if ((legacy_u8)dialog_ascii_lower(
					(legacy_u8)filenames[index][0]) == character) {
					selected = (legacy_s8)index;
					break;
				}
			}
		}

		if (selected < scroll)
			scroll = selected;
		if (scroll < 0) {
			font_set_colors(dialog_fnt_colour, dialog_background_color);
			key = (legacy_u16)call_read_line(directory,
				FILE_DIALOG_DIRECTORY_MAX_LENGTH, positions[2], positions[3],
				DIALOG_INPUT_TIMEOUT);
			if (key == KEY_ESCAPE) {
				result = 0;
			} else {
				search_again = 1;
			}
			break;
		}
		while ((legacy_s16)(scroll + 6) < selected)
			scroll++;

		if (result == 0)
			continue;
		if (result < 0) {
			result = 0;
			break;
		}
		strcpy(filename, filenames[(legacy_u8)selected]);
		result = 1;
		break;
	}
	if (search_again == 0)
		break;
	}

	sprite_pop_background();
	g_is_busy = saved_busy;
	return result;
}

void ensure_file_exists(legacy_s16 file_index)
{
	static legacy_s8* const message_ids[] = { missing_disk1_message_id, missing_disk2_message_id, missing_disk3_message_id, missing_disk4_message_id };
	legacy_s8* message_id;

	message_id = message_ids[file_index - 1];
	while (file_find(findfilenames[file_index]) == 0) {
		show_dialog(DIALOG_TYPE_ACKNOWLEDGEMENT, DIALOG_SAVE_BACKGROUND,
			locate_text_res(mainresptr, message_id),
			-1, -1, dialog_border_color, 0, 0);
		mouse_draw_opaque_check();
		kbormouse = 0;
	}
}

void show_waiting(void)
{
	show_dialog(DIALOG_TYPE_MESSAGE, DIALOG_NO_BACKGROUND_SAVE,
		locate_text_res(mainresptr, waiting_message_id),
		-1, waitflag, dialog_border_color, 0, 0);
	mouse_draw_opaque_check();
}

legacy_s16 do_savefile_dialog(legacy_s8* primary, legacy_s8* secondary, legacy_s8 far* prompt)
{
	legacy_s16 positions[6];
	legacy_s16 character_index;
	legacy_s16 key;
	legacy_s16 result;

	result = LEGACY_S16_FROM_BITS(show_dialog(DIALOG_TYPE_PLACEHOLDERS,
		DIALOG_SAVE_BACKGROUND,
		locate_text_res(mainresptr, file_save_dialog_id), -1, -1, dialog_border_color,
		positions, 0));
	if (result < 0)
		return 0;

	font_set_colors(dialog_fnt_colour, dialog_background_color);
	copy_string(&resID_byte1, prompt);
	font_draw_text_opaque(&resID_byte1, positions[0], positions[1]);
	font_set_colors(dialog_fnt_colour, dialog_background_color);
	font_draw_text_opaque(primary, positions[2], positions[3]);
	font_draw_text_opaque(secondary, positions[4], positions[5]);
	mouse_draw_transparent_check();

	result = 0;
	for (;;) {
		key = LEGACY_S16_FROM_BITS(call_read_line(secondary, 8,
			positions[4], positions[5], DIALOG_INPUT_TIMEOUT));
		for (character_index = 0; secondary[character_index] != 0;
			character_index++) {
			if (secondary[character_index] == ' ')
				secondary[character_index] = '_';
		}
		if (key == KEY_ESCAPE)
			break;
		if (key == KEY_ENTER) {
			result = 1;
			break;
		}
		key = LEGACY_S16_FROM_BITS(call_read_line(primary,
			FILE_DIALOG_DIRECTORY_MAX_LENGTH, positions[2], positions[3],
			DIALOG_INPUT_TIMEOUT));
		if (key == KEY_ESCAPE)
			break;
	}

	sprite_pop_background();
	return result;
}

legacy_s16 show_disk_error_dialog(void)
{
	legacy_s16 result;

	input_push_status();
	if (g_is_busy != 0) {
		result = show_dialog(DIALOG_TYPE_MENU, DIALOG_SAVE_BACKGROUND,
			locate_text_res(mainresptr, disk_retry_dialog_id),
			-1, -1, dialog_border_color, 0, 0) == 0;
	} else {
		show_dialog(DIALOG_TYPE_MESSAGE, DIALOG_SAVE_BACKGROUND,
			locate_text_res(mainresptr, disk_error_dialog_id),
			-1, -1, dialog_border_color, 0, 0);
		result = 1;
	}
	input_pop_status();
	return result;
}

void security_check(legacy_s16 question_index)
{
	legacy_s8 question_id[4] = "q00";
	legacy_s8 answer_id[4] = "a00";
	legacy_s8 question_text[1024];
	legacy_s8 answer[22];
	legacy_u8 question_parts[6];
	legacy_s16 positions[8];
	void far* resource;
	legacy_u16 answer_length;
	legacy_u16 attempts;
	legacy_u16 i;

	question_id[2] = quiz_question_suffixes[(legacy_u16)question_index];
	answer_id[2] = question_id[2];
	resource = file_load_resfile("misc");
	copy_string(question_text, locate_text_res(resource, "cop"));
	copy_string(&resID_byte1, locate_text_res(resource, question_id));
	strcat(question_text, resource_text_payload);
	for (i = 0; i < 6U; i++)
		question_parts[i] = (legacy_u8)(&resID_byte1)[i];

	show_dialog(DIALOG_TYPE_PLACEHOLDERS, DIALOG_SAVE_BACKGROUND,
		(void far*)question_text,
		DIALOG_AUTO_POSITION, SECURITY_DIALOG_Y,
		performGraphColor, positions, 0);
	(&resID_byte1)[2] = 0;
	(&resID_byte1)[0] = question_parts[0];
	(&resID_byte1)[1] = question_parts[1];
	font_draw_text(&resID_byte1, positions[0], positions[1]);
	(&resID_byte1)[0] = question_parts[2];
	(&resID_byte1)[1] = question_parts[3];
	font_draw_text(&resID_byte1, positions[2], positions[3]);
	(&resID_byte1)[0] = question_parts[4];
	(&resID_byte1)[1] = question_parts[5];
	font_draw_text(&resID_byte1, positions[4], positions[5]);

	copy_string(&resID_byte1, locate_text_res(resource, answer_id));
	answer_length = (legacy_u16)strlen(&resID_byte1);
	answer[0] = 0;
	attempts = 0;
	for (;;) {
		call_read_line(answer, answer_length, positions[6], positions[7],
			DIALOG_INPUT_TIMEOUT);
		for (i = 0; answer[i] != 0; i++) {
			legacy_u8 character = (legacy_u8)answer[i];

			if ((g_ascii_props[character] & RST_ASC_CHAR_UPPER) != 0)
				answer[i] = (legacy_s8)(character + ASCII_CASE_OFFSET);
		}
		if (strcmp(answer, &resID_byte1) == 0) {
			passed_security = 1;
			break;
		}
		attempts++;
		if (passed_security != 0 || attempts == 3U)
			break;
	}

	sprite_pop_background();
	mouse_draw_transparent_check();
	unload_resource(resource);
}
