#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "resource.h"
#include "shape2d.h"
#include "ui_dialog.h"
#include "ui_text.h"

#define RST_ASC_CHAR_UPPER 0x01
#define RST_ASC_CHAR_LOWER 0x02
#define RST_ASC_CONTROL    0x20

extern legacy_s8 aId1[];
extern legacy_s8 aId2[];
extern legacy_s8 aId3[];
extern legacy_s8 aId4[];
extern legacy_s8 aDea[];
extern legacy_s8 aDer[];
extern legacy_s8 aSav[];
extern legacy_s8 aWai[];
extern legacy_s8 aLoa[];
extern legacy_s8 aLsu[];
extern legacy_s8 aLsd[];
extern legacy_s8* findfilenames[];
extern legacy_s8 gnam_string[];
extern legacy_s8 gsna_string[];
extern legacy_s8 unk_46464[];
extern legacy_s8 byte_459E0[];

legacy_s16 call_read_line(legacy_s8* text, legacy_s16 max_characters,
	legacy_s16 x, legacy_s16 y, legacy_u32 timeout);

static const legacy_u8 far quiz_question_suffixes[20] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'
};
static const legacy_u8 far g_ascii_props[256] = {
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x28, 0x28, 0x28, 0x28, 0x28, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x48, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x10, 0x10, 0x10, 0x10, 0x20,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void sub_3702E(legacy_s16 left, legacy_s16 top, legacy_s16 right, legacy_s16 bottom, legacy_s16 color)
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
		sub_35B76(x, y, width, 1, color);
		sub_35B76(x, LEGACY_S16_FROM_BITS(bottom), width, 1, color);
	}
	if (height > 0) {
		y = LEGACY_S16_WRAP_ADD(y, 1);
		sub_35B76(x, y, 1, height, color);
		sub_35B76(LEGACY_S16_FROM_BITS(right), y,
			1, height, color);
	}
}

static legacy_u16 dialog_ascii_lower(legacy_u16 character)
{
	if (character < 256U &&
		(g_ascii_props[character] & RST_ASC_CHAR_UPPER) != 0)
		character = LEGACY_U16_WRAP_ADD(character, 0x20U);
	return character;
}

static legacy_u16 dialog_finish(legacy_s16 result,
	legacy_s16 save_background)
{
	if (save_background != 0)
		sub_275C6();
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
	legacy_s8 line_buffer[128];
	legacy_s8 choice_buffer[80];
	legacy_s8 far* choice_texts[20];
	legacy_u8 choice_lengths[20];
	struct BUTTON_AREA choices[20];
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

	line_height = LEGACY_S16_WRAP_ADD(fontdef_unk_0E, 2);
	dialog_height = 0;
	dialog_width = 0x20;
	mouse_draw_opaque_check();

	cursor = (legacy_s8 far*)text_resource;
	line_length = 0;
	while ((character = (legacy_u8)*cursor) != 0) {
		if (character == ']' || character == '}') {
			line_buffer[line_length] = 0;
			measured_width = (legacy_s16)font_op2(line_buffer);
			if (measured_width > dialog_width)
				dialog_width = measured_width;
			line_length = 0;
			dialog_height = dialog_advance_height(dialog_height,
				line_height, character, 4);
		} else {
			line_buffer[line_length++] = (legacy_s8)character;
		}
		cursor++;
	}

	dialog_width = LEGACY_S16_FROM_BITS(
		LEGACY_U16_WRAP_ADD((legacy_u16)dialog_width, 0x18U) & 0xFFF8U);
	x = LEGACY_S16_FROM_BITS(x_argument);
	y = LEGACY_S16_FROM_BITS(y_argument);
	if (x == -1) {
		x = LEGACY_S16_DIV_OR_ZERO(
			LEGACY_S16_WRAP_SUB(0x140, dialog_width), 2);
		x = LEGACY_S16_FROM_BITS((legacy_u16)x & 0xFFF8U);
	}
	if (y == -1)
		y = LEGACY_S16_DIV_OR_ZERO(
			LEGACY_S16_WRAP_SUB(0xC8, dialog_height), 2);

	left = x;
	right = LEGACY_S16_WRAP_ADD(x, dialog_width);
	top = LEGACY_S16_WRAP_SUB(y, 8);
	bottom = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_ADD(y, dialog_height), 8);
	x = LEGACY_S16_WRAP_ADD(x, 8);
	dialog_width = LEGACY_S16_WRAP_SUB(dialog_width, 0x10);
	if (save_background != 0 &&
		sub_274B0(left, right, top, bottom) == 0)
		return 0xFFFFU;

	sprite_copy_2_to_1();
	sprite_set_1_size(left, right, top, bottom);
	sprite_clear_1_color(0);
	sprite_1_unk4(LEGACY_S16_WRAP_SUB(x, 4),
		LEGACY_S16_WRAP_SUB(y, 4),
		LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(x, dialog_width), 4),
		LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(y, dialog_height), 4),
		border_color);
	font_set_unk(dialog_fnt_colour, 0);
	word_3EB90 = 0;
	font_set_unk(dialog_fnt_colour, 0);

	cursor = (legacy_s8 far*)text_resource;
	line_length = 0;
	placeholder_index = 0;
	dialog_height = 1;
	while ((character = (legacy_u8)*cursor) != 0 && character != '[') {
		if (character == ']' || character == '}') {
			line_buffer[line_length] = 0;
			sub_345BC(line_buffer, x,
				LEGACY_S16_WRAP_ADD(y, dialog_height));
			line_length = 0;
			dialog_height = dialog_advance_height(dialog_height,
				line_height, character, 4);
		} else if (character == '@') {
			if (dialog_type == 3) {
				line_buffer[line_length] = 0;
				disabled_choices[placeholder_index] =
					LEGACY_S16_WRAP_ADD(x,
						(legacy_s16)font_op2(line_buffer));
				disabled_choices[placeholder_index + 1U] =
					LEGACY_S16_WRAP_ADD(y, dialog_height);
				placeholder_index = (legacy_u8)(placeholder_index + 2U);
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
			x, (legacy_s16)font_op2(line_buffer));
		choices[choice_count].y1 = LEGACY_S16_WRAP_ADD(y, dialog_height);
		choices[choice_count].y2 = LEGACY_S16_WRAP_ADD(
			choices[choice_count].y1, line_height);
		line_buffer[line_length++] = ' ';
		choice_width = 0;
		character_count = 0;
		while ((character = (legacy_u8)*cursor) != 0 && character != '[') {
			if (character == ']' || character == '}') {
				line_buffer[line_length] = 0;
				choice_width = (legacy_u16)font_op2(line_buffer);
				line_length = 0;
				dialog_height = dialog_advance_height(dialog_height,
					line_height, character, 3);
			} else {
				line_buffer[line_length++] = (legacy_s8)character;
				character_count++;
			}
			cursor++;
		}
		choice_lengths[choice_count] = (legacy_u8)character_count;
		line_buffer[line_length] = 0;
		if (choice_width == 0)
			choice_width = (legacy_u16)font_op2(line_buffer);
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
	if (dialog_type == 0)
		return 0;
	if (dialog_type == 1) {
		do {
			input = (legacy_u16)input_checking(
				(legacy_s16)timer_get_delta_alt());
		} while (input == 0);
		if (input == 0x1BU)
			result = 0;
		check_input();
		return dialog_finish(result, save_background);
	}
	if (dialog_type == 3)
		return LEGACY_U16_DIV_OR_ZERO(placeholder_index, 2U);
	if (dialog_type == 4) {
		(void)sub_2EB1E(8UL);
		return dialog_finish(result, save_background);
	}
	if (dialog_type != 2)
		return dialog_finish(result, save_background);

	selected = (legacy_u8)initial_choice;
	previous = 0xFFU;
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
					font_set_unk(word_3EB90, dialog_fnt_colour);
				else
					font_set_unk(dialog_fnt_colour, word_3EB90);
				if (disabled_choices != 0 && disabled_choices[index] != 0)
					font_set_unk(performGraphColor, word_3EB90);
				for (copied = 0; copied < choice_lengths[index]; copied++)
					choice_buffer[copied] = choice_texts[index][copied];
				choice_buffer[copied] = 0;
				sub_345BC(choice_buffer, choices[index].x1,
					choices[index].y1);
			}
			mouse_draw_transparent_check();
			if (previous == 0xFFU)
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
				input = 0x0DU;
			} else if (input == second_hotkey) {
				selected = 1;
				input = 0x0DU;
			}
		}

		if (input == 0)
			continue;
		if (input == 0x20U || input == 0x0DU) {
			active = 0;
			check_input();
			continue;
		}
		if (input == 0x1BU) {
			selected = 0xFFU;
			active = 0;
			check_input();
			continue;
		}
		if (input == 0x4800U || input == 0x4B00U) {
			do {
				selected = selected == 0 ?
					(legacy_u8)(choice_count - 1U) :
					(legacy_u8)(selected - 1U);
			} while (disabled_choices != 0 &&
				disabled_choices[selected] != 0);
			continue;
		}
		if (input == 0x4D00U || input == 0x5000U) {
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

	dialog_result = LEGACY_S16_FROM_BITS(show_dialog(3, 1,
		locate_text_res(mainresptr, aLoa),
		DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
		dialogarg2, positions, 0));
	if (dialog_result < 0)
		return 0;

	saved_busy = g_is_busy;
	g_is_busy = 1;
	preRender_line(positions[4] - 4, positions[5] + 4,
		positions[4] + 0xAB, positions[5] + 4, dialogarg2);
	font_set_unk(dialog_fnt_colour, word_3EB90);
	copy_string(&resID_byte1, prompt);
	sub_345BC(&resID_byte1, positions[0], positions[1]);

	for (index = 0; index < 10U; index++) {
		hit_areas[index].x1 = positions[2];
		hit_areas[index].x2 = positions[2] + 0xA2;
		if (index == 9U)
			hit_areas[index].y1 = hit_areas[index - 1U].y1 + 10;
		else
			hit_areas[index].y1 = positions[3U + index * 2U];
		hit_areas[index].y2 = hit_areas[index].y1 + 10;
	}
	font_set_unk(dialog_fnt_colour, word_3EB90);
	sub_345BC(directory, positions[2], positions[3]);

	for (;;) {
	mouse_draw_transparent_check();
	file_count = 0;
	found_path = file_combine_and_find(directory, "*", extension);
	if (found_path == 0) {
		font_set_unk(dialog_fnt_colour, word_3EB90);
		key = (legacy_u16)call_read_line(directory, 0x12,
			positions[2], positions[3], 0x7530UL);
		if (key == 0x1BU) {
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
		copy_string(&resID_byte1, locate_text_res(mainresptr, aLsu));
		sub_345BC(&resID_byte1, font_op2_alt(&resID_byte1), hit_areas[1].y1);
		copy_string(&resID_byte1, locate_text_res(mainresptr, aLsd));
		sub_345BC(&resID_byte1, font_op2_alt(&resID_byte1),
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
					font_set_unk(word_3EB90, dialog_fnt_colour);
				else
					font_set_unk(dialog_fnt_colour, word_3EB90);
				if (candidate < (legacy_s16)file_count) {
					strcpy(&resID_byte1, filenames[(legacy_u8)candidate]);
					sub_345BC(&resID_byte1, positions[2],
						hit_areas[visible_row + 2U].y1);
				} else {
					sub_345BC("        ", positions[2],
						hit_areas[visible_row + 2U].y1);
				}
				text_width = (legacy_u16)font_op2(&resID_byte1);
				sprite_1_unk(positions[2] + text_width,
					hit_areas[visible_row + 2U].y1,
					positions[2] + 0xA2 - text_width - positions[2],
					8, word_3EB90);
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

		if (key == 0x0DU || key == 0x20U) {
			result = 1;
		} else if (key == 0x1BU) {
			result = -1;
		} else if (key == 0x4800U) {
			selected--;
		} else if (key == 0x5000U) {
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
			font_set_unk(dialog_fnt_colour, word_3EB90);
			key = (legacy_u16)call_read_line(directory, 0x12,
				positions[2], positions[3], 0x7530UL);
			if (key == 0x1BU) {
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

	sub_275C6();
	g_is_busy = saved_busy;
	return result;
}

void ensure_file_exists(legacy_s16 file_index)
{
	static legacy_s8* const message_ids[] = { aId1, aId2, aId3, aId4 };
	legacy_s8* message_id;

	message_id = message_ids[file_index - 1];
	while (file_find(findfilenames[file_index]) == 0) {
		show_dialog(1, 1, locate_text_res(mainresptr, message_id),
			-1, -1, dialogarg2, 0, 0);
		mouse_draw_opaque_check();
		kbormouse = 0;
	}
}

void show_waiting(void)
{
	show_dialog(0, 0, locate_text_res(mainresptr, aWai),
		-1, waitflag, dialogarg2, 0, 0);
	mouse_draw_opaque_check();
}

legacy_s16 do_savefile_dialog(legacy_s8* primary, legacy_s8* secondary, legacy_s8 far* prompt)
{
	legacy_s16 positions[6];
	legacy_s16 character_index;
	legacy_s16 key;
	legacy_s16 result;

	result = LEGACY_S16_FROM_BITS(show_dialog(3, 1,
		locate_text_res(mainresptr, aSav), -1, -1, dialogarg2,
		positions, 0));
	if (result < 0)
		return 0;

	font_set_unk(dialog_fnt_colour, word_3EB90);
	copy_string(&resID_byte1, prompt);
	sub_345BC(&resID_byte1, positions[0], positions[1]);
	font_set_unk(dialog_fnt_colour, word_3EB90);
	sub_345BC(primary, positions[2], positions[3]);
	sub_345BC(secondary, positions[4], positions[5]);
	mouse_draw_transparent_check();

	result = 0;
	for (;;) {
		key = LEGACY_S16_FROM_BITS(call_read_line(secondary, 8,
			positions[4], positions[5], 0x7530UL));
		for (character_index = 0; secondary[character_index] != 0;
			character_index++) {
			if (secondary[character_index] == ' ')
				secondary[character_index] = '_';
		}
		if (key == 0x1B)
			break;
		if (key == 0x0D) {
			result = 1;
			break;
		}
		key = LEGACY_S16_FROM_BITS(call_read_line(primary, 0x12,
			positions[2], positions[3], 0x7530UL));
		if (key == 0x1B)
			break;
	}

	sub_275C6();
	return result;
}

legacy_s16 do_dea_textres(void)
{
	legacy_s16 result;

	input_push_status();
	if (g_is_busy != 0) {
		result = show_dialog(2, 1,
			locate_text_res(mainresptr, aDea),
			-1, -1, dialogarg2, 0, 0) == 0;
	} else {
		show_dialog(0, 1, locate_text_res(mainresptr, aDer),
			-1, -1, dialogarg2, 0, 0);
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
	strcat(question_text, unk_463EA);
	for (i = 0; i < 6U; i++)
		question_parts[i] = (legacy_u8)(&resID_byte1)[i];

	show_dialog(3, 1, (void far*)question_text,
		DIALOG_AUTO_POSITION, 0x78U,
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
			0x7530UL);
		for (i = 0; answer[i] != 0; i++) {
			legacy_u8 character = (legacy_u8)answer[i];

			if ((g_ascii_props[character] & RST_ASC_CHAR_UPPER) != 0)
				answer[i] = (legacy_s8)(character + 0x20U);
		}
		if (strcmp(answer, &resID_byte1) == 0) {
			passed_security = 1;
			break;
		}
		attempts++;
		if (passed_security != 0 || attempts == 3U)
			break;
	}

	sub_275C6();
	mouse_draw_transparent_check();
	unload_resource(resource);
}
