#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

void do_mer_restext(void)
{
	show_dialog(1, 1, locate_text_res(mainresptr, aMer),
		-1, -1, dialogarg2, 0, 0);
}

void do_key_restext(void)
{
	input_push_status();
	dos_timer_set_callbacks_suspended(1);
	audio_suspend();
	show_dialog(4, 1, locate_text_res(mainresptr, aKey),
		-1, -1, dialogarg2, 0, 0);
	dos_joystick_set_enabled(0);
	byte_3B8F2 = 0;
	dos_timer_set_callbacks_suspended(0);
	audio_resume();
	input_pop_status();
}

static void joy_dialog_finish(void)
{
	kb_check();
	byte_3B8F2 = 0;
	audio_resume();
	dos_timer_set_callbacks_suspended(0);
	input_pop_status();
}

void do_joy_restext(void)
{
	legacy_s16 positions[15];
	legacy_s16 button_x[9];
	legacy_s16 button_y[9];
	legacy_u8 visited[9];
	legacy_s16 button_width;
	legacy_s16 button_height;
	legacy_s16 line_width;
	legacy_s16 line_height;
	legacy_s16 selected;
	legacy_s16 next_selected;
	legacy_u16 joy_flags;
	legacy_u16 i;

	input_push_status();
	dos_timer_set_callbacks_suspended(1);
	audio_suspend();
	if (LEGACY_S16_FROM_BITS(show_dialog(3, 1,
		locate_text_res(mainresptr, "joy"),
		DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
		dialogarg2, positions, 0)) <= 0) {
		dos_joystick_set_enabled(0);
		joy_dialog_finish();
		return;
	}

	for (i = 0; i < 9U; i++)
		visited[i] = 0;
	dos_joystick_set_enabled(1);
	mouse_draw_opaque_check();
	line_height = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_SUB(positions[13], positions[3]), 8);
	sprite_1_unk(LEGACY_S16_WRAP_SUB(positions[2], 4), positions[3],
		1, line_height, dialogarg2);
	sprite_1_unk(LEGACY_S16_WRAP_SUB(positions[4], 4), positions[5],
		1, line_height, dialogarg2);
	line_width = LEGACY_S16_WRAP_SUB(positions[6], positions[0]);
	sprite_1_unk(positions[0], LEGACY_S16_WRAP_SUB(positions[9], 4),
		line_width, 1, dialogarg2);
	sprite_1_unk(positions[0], LEGACY_S16_WRAP_SUB(positions[11], 4),
		line_width, 1, dialogarg2);

	button_x[0] = positions[2];
	button_x[1] = positions[2];
	button_x[5] = positions[2];
	button_x[2] = positions[4];
	button_x[3] = positions[4];
	button_x[4] = positions[4];
	button_x[6] = positions[0];
	button_x[7] = positions[0];
	button_x[8] = positions[0];
	button_y[0] = positions[9];
	button_y[3] = positions[9];
	button_y[7] = positions[9];
	button_y[1] = positions[3];
	button_y[2] = positions[3];
	button_y[8] = positions[3];
	button_y[4] = positions[11];
	button_y[5] = positions[11];
	button_y[6] = positions[11];
	button_width = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_SUB(positions[2], positions[0]), 8);
	button_height = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_SUB(positions[9], positions[1]), 8);

	selected = -1;
	sub_307B4();
	for (;;) {
		if (kb_read_char() != 0)
			break;
		joy_flags = (legacy_u16)dos_get_joy_flags();
		if ((joy_flags & 0x30U) != 0)
			break;
		next_selected = (legacy_s16)sub_307D2(joy_flags);
		if (next_selected == selected)
			continue;
		for (i = 0; i < 9U; i++)
			sprite_1_unk(button_x[i], button_y[i], button_width,
				button_height, word_3EB90);
		sprite_1_unk(button_x[next_selected], button_y[next_selected],
			button_width, button_height, dialog_fnt_colour);
		selected = next_selected;
		visited[next_selected] = 1;
	}

	for (i = 0; i < 9U; i++)
		dos_joystick_set_enabled(
			dos_joystick_is_enabled() & visited[i]);
	sub_275C6();
	if (dos_joystick_is_enabled() == 0)
		show_dialog(1, 1, locate_text_res(mainresptr, "jox"),
			DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
			dialogarg2, 0, 0);

	joy_dialog_finish();
}

void do_mou_restext(void)
{
	input_push_status();
	dos_timer_set_callbacks_suspended(1);
	audio_suspend();
	byte_3B8F2 = 1;
	show_dialog(4, 1, locate_text_res(mainresptr, aMou),
		-1, -1, dialogarg2, 0, 0);
	dos_timer_set_callbacks_suspended(0);
	audio_resume();
	input_pop_status();
}

void do_pau_restext(void)
{
	input_push_status();
	dos_timer_set_callbacks_suspended(1);
	audio_suspend();
	show_dialog(0, 1, locate_text_res(mainresptr, aPau),
		-1, -1, dialogarg2, 0, 0);
	dos_timer_set_callbacks_suspended(0);
	audio_resume();
	input_pop_status();
}

void do_mof_restext(void)
{
	legacy_s8* message_id;

	input_push_status();
	dos_timer_set_callbacks_suspended(1);
	message_id = audio_toggle_flag2() != 0 ? aMon : aMof;
	show_dialog(4, 1, locate_text_res(mainresptr, message_id),
		-1, -1, dialogarg2, 0, 0);
	dos_timer_set_callbacks_suspended(0);
	input_pop_status();
}

void do_sonsof_restext(void)
{
	legacy_s8* message_id;

	input_push_status();
	dos_timer_set_callbacks_suspended(1);
	message_id = audio_toggle_flag6() != 0 ? aSon : aSof;
	show_dialog(4, 1, locate_text_res(mainresptr, message_id),
		-1, -1, dialogarg2, 0, 0);
	dos_timer_set_callbacks_suspended(0);
	input_pop_status();
}

void do_dos_restext(void)
{
	legacy_s16 result;

	input_push_status();
	dos_timer_set_callbacks_suspended(1);
	audio_suspend();
	result = show_dialog(2, 1, locate_text_res(mainresptr, aDos_0),
		-1, -1, dialogarg2, 0, 0);
	if (result == 1)
		call_exitlist2();
	dos_timer_set_callbacks_suspended(0);
	audio_resume();
	input_pop_status();
}

void show_graphic_levels_menu(void)
{
	legacy_s8 selected_options[9];
	legacy_s8 menu_text[512];
	legacy_u16 original_frame_rate;
	legacy_u16 option_index;
	legacy_u16 text_index;
	legacy_s8 selected;

	input_push_status();
	dos_timer_set_callbacks_suspended(1);
	audio_suspend();
	original_frame_rate = framespersec2;
	selected = 0;
	for (;;) {
		copy_string(menu_text, locate_text_res(mainresptr, aMrl));
		for (option_index = 0; option_index < 9U; option_index++)
			selected_options[option_index] = 0;
		selected_options[detail_level] = 1;
		selected_options[5U + slow_video_mgmt] = 1;
		selected_options[framespersec2 == 10U ? 7 : 8] = 1;

		text_index = 0;
		for (option_index = 0; option_index < 9U; option_index++) {
			while (menu_text[text_index] != '[')
				text_index++;
			if (selected_options[option_index] != 0)
				menu_text[text_index + 1U] = '*';
			text_index++;
		}

		selected = LEGACY_S8_FROM_BITS(show_dialog(2, 1,
			(void far*)menu_text, -1, -1, performGraphColor, 0,
			(legacy_s16)selected));
		if (selected == -1 || selected == 9)
			break;
		switch (selected) {
		case 5:
			slow_video_mgmt = 0;
			break;
		case 6:
			slow_video_mgmt = 1;
			break;
		case 7:
			framespersec2 = 10;
			break;
		case 8:
			framespersec2 = 20;
			break;
		default:
			detail_level = (legacy_u8)selected;
			break;
		}
	}

	if (original_frame_rate != framespersec2)
		show_dialog(1, 1, locate_text_res(mainresptr, aMrs),
			-1, -1, dialogarg2, 0, 0);
	dos_timer_set_callbacks_suspended(0);
	audio_resume();
	input_pop_status();
}

legacy_u16 run_option_menu(void)
{
	legacy_s8 selected;
	legacy_s8 initial_input;
	legacy_u8 menu_active;
	legacy_s8 far* prompt;

	miscptr = file_load_resfile("misc");
	sprite_copy_2_to_1_2();
	sprite_clear_1_color((legacy_u8)word_407FA);
	copy_string(&resID_byte1, locate_shape_alt(miscptr, "gstu"));
	intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1), 6,
		dialog_fnt_colour, 0);
	copy_string(&resID_byte1, locate_shape_alt(miscptr, "gver"));
	intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1), 0x10,
		dialog_fnt_colour, 0);

	menu_active = 1;
	while (menu_active != 0) {
		selected = LEGACY_S8_FROM_BITS(show_dialog(2, 1,
			locate_text_res(miscptr, "mop"),
			DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
			dialogarg2, 0, 0));
		switch (selected) {
		case -1:
		case 6:
			menu_active = 0;
			break;

		case 0:
			if (byte_3B8F2 != 0)
				initial_input = 2;
			else if (dos_joystick_is_enabled() != 0)
				initial_input = 1;
			else
				initial_input = 0;
			selected = LEGACY_S8_FROM_BITS(show_dialog(2, 1,
				locate_text_res(miscptr, "mid"),
				DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
				performGraphColor, 0, initial_input));
			if (selected == 0)
				do_key_restext();
			else if (selected == 1)
				do_joy_restext();
			else if (selected == 2)
				do_mou_restext();
			break;

		case 1:
			do_mof_restext();
			break;

		case 2:
			do_sonsof_restext();
			break;

		case 3:
			prompt = locate_text_res(mainresptr, "rep");
			if (do_fileselect_dialog(byte_3B85E, aDefault_1,
				".rpl", prompt) != 0) {
				waitflag = 0x96;
				show_waiting();
				file_load_replay(byte_3B85E, aDefault_1);
				menu_active = 1;
				unload_resource(miscptr);
				return menu_active;
			}
			break;

		case 4:
			show_graphic_levels_menu();
			break;

		case 5:
			do_dos_restext();
			break;
		}
	}

	unload_resource(miscptr);
	return menu_active;
}
