#include "fileio.h"
#include "game_input.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "race_resources.h"
#include "replay.h"
#include "replay_viewer.h"
#include "replay_viewer_internal.h"
#include "resource.h"
#include "shape2d.h"
#include "timing.h"
#include "ui_dialog.h"
#include "ui_input.h"
#include "ui_text.h"

extern void far* fontledresptr;
extern void far* sdgameresptr;
extern legacy_s16 camera_track_height_offset;


static void replay_controls_select(legacy_u8 selection)
{
	legacy_u16 index;

	for (index = 0; index < 9U; index++)
		byte_40E6A[index] = 0;
	byte_40E6A[selection] = 1;
}

static void replay_controls_draw(legacy_u16 recorded_frame, legacy_u16 current_frame)
{
	legacy_u16 player_index;
	legacy_u16 index;
	legacy_s16 recorded_position;
	legacy_s16 current_position;
	legacy_u16 displayed_time;
	legacy_u8 previous_selection;
	legacy_u8 state_changed;

	player_index = (legacy_u8)byte_4432A;
	if (byte_449D8[player_index] == 0) {
		byte_449D8[player_index] = 1;
		byte_40E74[player_index] = 0xFFU;
		byte_40E08[player_index] = 0xFFU;
		for (index = 0; index < 9U; index++)
			byte_40E7A[player_index + index * 2U] = 0;
		mouse_draw_opaque_check();
		shape2d_op_unk(rplyshapes[0]);
		word_40E0A[player_index] = -1;
		word_40E76[player_index] = -1;
		format_frame_as_string(&resID_byte1,
			(legacy_u16)(gameconfig.game_recordedframes + elapsed_time1),
			1);
		font_set_unk(dialog_fnt_colour, 0);
		font_set_fontdef2(fontledresptr);
		sub_345BC(&resID_byte1, 0xD8, 0xBB);
		font_set_fontdef();
	}

	displayed_time = (legacy_u16)(current_frame + elapsed_time1);
	if ((legacy_u16)word_40E0A[player_index] != displayed_time) {
		word_40E0A[player_index] = (legacy_s16)displayed_time;
		format_frame_as_string(&resID_byte1, displayed_time, 1);
		font_set_unk(dialog_fnt_colour, 0);
		mouse_draw_opaque_check();
		font_set_fontdef2(fontledresptr);
		sub_345BC(&resID_byte1, 0x98, 0xBB);
		font_set_fontdef();
	}

	if (byte_40E74[player_index] != (legacy_u8)cameramode) {
		byte_40E74[player_index] = (legacy_u8)cameramode;
		word_40E76[player_index] = -1;
		mouse_draw_opaque_check();
		shape2d_op_unk(rplyshapes[1U + (legacy_u8)cameramode]);
		if (LEGACY_S8_FROM_BITS(byte_3E9DB) > LEGACY_S8_FROM_BITS(
			game_camera_buttons_count[(legacy_u8)cameramode]))
			byte_3E9DB = game_camera_buttons_count[(legacy_u8)cameramode];
		if (byte_40E08[player_index] > 6U)
			byte_40E08[player_index] = 0xFFU;
	}

	recorded_position = (legacy_s16)replay_timeline_position(
		recorded_frame, gameconfig.game_recordedframes, 110U);
	current_position = (legacy_s16)replay_timeline_position(
		current_frame, gameconfig.game_recordedframes, 110U);
	if (word_40E76[player_index] != recorded_position ||
		word_40E04[player_index] != current_position) {
		mouse_draw_opaque_check();
		word_40E76[player_index] = recorded_position;
		word_40E04[player_index] = current_position;
		sprite_1_unk(0x9A, 0xB1, 0x74, 6, word_407FC);
		sprite_1_unk(LEGACY_S16_WRAP_ADD(0x9A, recorded_position),
			0xB1, 6, 6, dialog_fnt_colour);
		sprite_1_unk4(LEGACY_S16_WRAP_ADD(0x9A, current_position),
			0xB1, LEGACY_S16_WRAP_ADD(0x9F, current_position),
			0xB6, word_407FE);
	}

	state_changed = byte_40E08[player_index] != byte_3E9DB;
	if (state_changed == 0) {
		for (index = 0; index < 7U; index++) {
			if (byte_40E7A[player_index + index * 2U] !=
				byte_40E6A[index]) {
				state_changed = 1;
				break;
			}
		}
	}
	if (state_changed == 0) {
		mouse_draw_transparent_check();
		return;
	}

	mouse_draw_opaque_check();
	previous_selection = byte_40E08[player_index];
	if (previous_selection != 0xFFU) {
		if (byte_40E7A[player_index + previous_selection * 2U] != 0)
			shape2d_op_unk(rplyshapes[14U + previous_selection]);
		else
			shape2d_op_unk(rplyshapes[5U + previous_selection]);
		byte_40E08[player_index] = 0xFFU;
	}
	for (index = 0; index < 7U; index++) {
		if (byte_40E6A[index] == 0 &&
			byte_40E7A[player_index + index * 2U] != 0) {
			shape2d_op_unk(rplyshapes[5U + index]);
			byte_40E7A[player_index + index * 2U] = 0;
		}
	}
	for (index = 0; index < 7U; index++) {
		if (byte_40E6A[index] != 0) {
			byte_40E7A[player_index + index * 2U] = 1;
			shape2d_op_unk(rplyshapes[14U + index]);
			byte_40E7A[player_index + index * 2U] = 1;
		}
	}
	byte_40E08[player_index] = byte_3E9DB;
	if (byte_3E9DB != 0xFFU) {
		sprite_1_unk4(game_camera_buttons[byte_3E9DB].x1,
			game_camera_buttons[byte_3E9DB].y1,
			game_camera_buttons[byte_3E9DB].x2,
			game_camera_buttons[byte_3E9DB].y2, word_407FE);
	}
	mouse_draw_transparent_check();
}

static void replay_draw_waiting(void)
{
	struct RECTANGLE* text_rectangle;

	copy_string(&resID_byte1, locate_text_res(gameresptr, "wai"));
	text_rectangle = intro_draw_text(&resID_byte1,
		font_op2_alt(&resID_byte1), 0x64, dialog_fnt_colour, 0);
	if (slow_video_mgmt_copy != 0)
		rect_union(rectptr_unk2, text_rectangle, rectptr_unk2);
}

static void replay_pause_menu(void)
{
	struct GAMEINFO saved_config;
	legacy_s16 options[8];
	legacy_s16 mode_options[5];
	legacy_s16 dialog_result;
	legacy_s8 menu_result;
	legacy_s8 save_status;
	legacy_u16 index;
	legacy_u8 saved_track;
	legacy_s16 resources_changed;
	legacy_s16 opponent_changed;

	is_in_replay = 1;
	audio_carstate();
	replay_controls_select(4);
	replay_controls_draw(state.game_frame, state.game_frame);
	for (index = 0; index < 8U; index++)
		options[index] = 0;
	if (state.playerstate.car_crashBmpFlag != 0)
		options[3] = 1;
	if (gameconfig.game_recordedframes == 0 || elapsed_time1 != 0)
		options[5] = 1;
	if (passed_security == 0) {
		options[2] = 1;
		options[3] = 1;
	}
	if (((legacy_u8)byte_43966 & 4U) == 0)
		options[1] = 1;
	byte_454A4 = (legacy_u8)video_flag6_is1;
	menu_result = LEGACY_S8_FROM_BITS(show_dialog(2, 0,
		locate_text_res(gameresptr, aMen_0),
		DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
		dialogarg2, options, 0));

	switch (menu_result) {
	case 1:
		update_crash_state(4, 0);
		byte_449DA = 2;
		break;

	case 2:
		check_input();
		init_game_state_with_frame_rate_byte(framespersec2);
		elapsed_time2 = 0;
		gameconfig.game_recordedframes = 0;
		word_45D3E = LEGACY_S16_FROM_BITS(
			LEGACY_U16_REPLACE_LOW_BYTE(word_45D3E, 0U));
		byte_43966 = 1;
		/* fall through */

	case 3:
		if (menu_result == 3) {
			if (((legacy_u8)byte_43966 & 2U) != 0) {
				byte_43966 = 3;
			} else if (gameconfig.game_recordedframes != elapsed_time2) {
				dialog_result = LEGACY_S16_FROM_BITS(show_dialog(2, 0,
					locate_text_res(gameresptr, aCon_0),
					DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
					performGraphColor, 0, 0));
				if (dialog_result < 1)
					break;
				byte_43966 = 3;
			} else {
				byte_43966 = 1;
			}
			elapsed_time2 = (legacy_u16)state.game_frame;
			gameconfig.game_recordedframes = (legacy_u16)state.game_frame;
		}
		dashb_toggle = 1;
		show_penalty_counter = 0;
		followOpponentFlag = 0;
		game_replay_mode = 0;
		cameramode = 0;
		state.game_3F6autoLoadEvalFlag = 0;
		state.game_frame_in_sec = 0;
		byte_449E6 = 0;
		replay_controls_select(3);
		is_in_replay = 0;
		mouse_minmax_position(LEGACY_S8_FROM_BITS(byte_3B8F2));
		check_input();
		kbormouse = 0;
		break;

	case 4:
		byte_43966 = 0;
		audio_carstate();
		if (do_fileselect_dialog(byte_3B85E, aDefault_1, ".rpl",
			locate_text_res(mainresptr, "rep")) == 0)
			break;
		waitflag = 0x96;
		show_waiting();
		saved_config = gameconfig;
		saved_track = td14_elem_map_main[0x384];
		if ((legacy_u8)file_load_replay(byte_3B85E, aDefault_1) != 0)
			gameconfig.game_recordedframes = 0;
		dashb_toggle = 0;
		track_setup();
		resources_changed = td14_elem_map_main[0x384] != saved_track;
		for (index = 0; index < 4U; index++) {
			if (saved_config.game_playercarid[index] !=
				gameconfig.game_playercarid[index])
				resources_changed = 1;
		}
		if (saved_config.game_opponenttype !=
			gameconfig.game_opponenttype) {
			resources_changed = 1;
		} else if (gameconfig.game_opponenttype != 0) {
			opponent_changed = 0;
			for (index = 0; index < 4U; index++) {
				if (saved_config.game_opponentcarid[index] !=
					gameconfig.game_opponentcarid[index]) {
					resources_changed = 1;
					opponent_changed = 1;
				}
			}
			if (opponent_changed == 0) {
				ensure_file_exists(2);
				load_opponent_data();
			}
		}
		if (resources_changed != 0) {
			free_player_cars();
			setup_player_cars();
		}
		framespersec = (legacy_s16)LEGACY_S8_FROM_BITS(
			LEGACY_U16_LOW_BYTE(gameconfig.game_framespersec));
		init_game_state(-1);
		break;

	case 5:
		audio_carstate();
		for (;;) {
			save_status = 0;
			if (do_savefile_dialog(byte_3B85E, aDefault_1,
				locate_text_res(mainresptr, aRep_1)) == 0) {
				save_status = -1;
			} else {
				file_build_path(byte_3B85E, aDefault_1, a_rpl_2,
					g_path_buf);
				save_status = 1;
				g_is_busy = 1;
				if (file_find(g_path_buf) != 0) {
					dialog_result = LEGACY_S16_FROM_BITS(show_dialog(
						2, 0, locate_text_res(mainresptr, aFex_0),
						DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
						performGraphColor, 0, 0));
					if (dialog_result == -1)
						save_status = -1;
					else if (dialog_result == 0)
						save_status = 0;
				}
				g_is_busy = 0;
			}
			if (save_status != 1)
				break;
			if ((legacy_u8)file_write_replay(g_path_buf) == 0)
				break;
			show_dialog(1, 0, locate_text_res(mainresptr, aSer_0),
				DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
				performGraphColor, 0, 0);
		}
		break;

	case 6:
		for (index = 0; index < 5U; index++)
			mode_options[index] = 0;
		if (gameconfig.game_opponenttype == 0)
			mode_options[4] = 1;
		menu_result = LEGACY_S8_FROM_BITS(show_dialog(2, 0,
			locate_text_res(gameresptr, aMdo),
			DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
			dialogarg2, mode_options, 0));
		switch (menu_result) {
		case 0:
			dashb_toggle ^= 1;
			break;
		case 1:
			replaybar_toggle ^= 1;
			break;
		case 2:
			cameramode = (legacy_s8)(((legacy_u8)cameramode + 1U) & 3U);
			break;
		case 3:
			show_graphic_levels_menu();
			break;
		case 4:
			followOpponentFlag ^= 1;
			break;
		}
		break;

	case 7:
		update_crash_state(4, 0);
		byte_43966 = 0;
		byte_449DA = 2;
		break;
	}
	check_input();
}

static legacy_s32 replay_scrub_accumulate(legacy_s32 accumulated,
	legacy_s16 speed, legacy_u16 delta)
{
	legacy_s16 increment;

	increment = LEGACY_S16_WRAP_MUL(LEGACY_S16_FROM_BITS(delta), speed);
	return LEGACY_S32_WRAP_ADD_S16(accumulated, increment);
}

static legacy_s16 replay_scrub_speed(legacy_s32 accumulated)
{
	legacy_s32 quotient;

	quotient = LEGACY_S32_DIV_OR_ZERO(accumulated, 50L);
	return LEGACY_S16_WRAP_ADD(
		LEGACY_S16_FROM_BITS((legacy_u16)quotient), 3);
}

static legacy_u16 replay_scrub_amount(legacy_s32 accumulated)
{
	return (legacy_u16)LEGACY_S32_DIV_OR_ZERO(accumulated, 20L);
}

static legacy_s32 replay_scrub_begin(legacy_u8 selection)
{
	is_in_replay = 1;
	audio_carstate();
	replay_controls_select(selection);
	(void)timer_get_delta_alt();
	return 20L;
}

static legacy_s32 replay_scrub_advance(legacy_s32 accumulated,
	legacy_u16* delta)
{
	legacy_s16 speed;

	speed = replay_scrub_speed(accumulated);
	if (speed > 100)
		speed = 100;
	*delta = (legacy_u16)timer_get_delta_alt();
	return replay_scrub_accumulate(accumulated, speed, *delta);
}

static void replay_fast_forward(void)
{
	legacy_s32 accumulated;
	legacy_u16 delta;
	legacy_u16 remaining;
	legacy_u16 amount;
	legacy_u16 target;

	accumulated = replay_scrub_begin(0);
	while (((legacy_u8)input_combined_flags & 0x30U) != 0) {
		accumulated = replay_scrub_advance(accumulated, &delta);
		remaining = LEGACY_U16_WRAP_SUB(
			gameconfig.game_recordedframes, elapsed_time2);
		amount = replay_scrub_amount(accumulated);
		if (amount > remaining)
			accumulated = LEGACY_S32_WRAP_MUL(
				(legacy_s32)remaining, 20L);
		amount = replay_scrub_amount(accumulated);
		replay_controls_draw(state.game_frame,
			LEGACY_U16_WRAP_ADD(elapsed_time2, amount));
		input_do_checking(LEGACY_S16_FROM_BITS(delta));
	}

	remaining = LEGACY_U16_WRAP_SUB(
		gameconfig.game_recordedframes, elapsed_time2);
	amount = replay_scrub_amount(accumulated);
	if (amount > remaining) {
		accumulated = LEGACY_S32_WRAP_MUL(
			(legacy_s32)remaining, 20L);
		amount = remaining;
	}
	target = LEGACY_U16_WRAP_ADD(elapsed_time2, amount);
	if (LEGACY_S16_FROM_BITS(target) >
		LEGACY_S16_FROM_BITS(gameconfig.game_recordedframes))
		target = gameconfig.game_recordedframes;
	restore_gamestate(target);
	elapsed_time2 = target;
	replay_controls_select(4);
	replay_draw_waiting();
	while ((legacy_u16)state.game_frame != elapsed_time2) {
		update_gamestate();
		replay_controls_draw(state.game_frame, elapsed_time2);
	}
	input_do_checking(1000);
}

static void replay_rewind(void)
{
	legacy_s32 accumulated;
	legacy_s16 frames_to_catch_up;
	legacy_s16 frames_remaining;
	legacy_u16 delta;
	legacy_u16 amount;
	legacy_u16 target;
	legacy_u16 displayed_frame;

	accumulated = replay_scrub_begin(1);
	while (((legacy_u8)input_combined_flags & 0x30U) != 0) {
		accumulated = replay_scrub_advance(accumulated, &delta);
		amount = replay_scrub_amount(accumulated);
		if (amount > elapsed_time2)
			accumulated = LEGACY_S32_WRAP_MUL(
				(legacy_s32)elapsed_time2, 20L);
		amount = replay_scrub_amount(accumulated);
		replay_controls_draw(state.game_frame,
			LEGACY_U16_WRAP_SUB(elapsed_time2, amount));
		input_do_checking(LEGACY_S16_FROM_BITS(delta));
	}

	amount = replay_scrub_amount(accumulated);
	if (amount > elapsed_time2)
		amount = elapsed_time2;
	replay_controls_select(4);
	if (amount != 0) {
		replay_draw_waiting();
		target = LEGACY_U16_WRAP_SUB(elapsed_time2, amount);
		restore_gamestate(target);
		elapsed_time2 = target;
		frames_to_catch_up = LEGACY_S16_WRAP_SUB(
			LEGACY_S16_FROM_BITS(target), state.game_frame);
		frames_remaining = frames_to_catch_up;
		while ((legacy_u16)state.game_frame != elapsed_time2) {
			update_gamestate();
			frames_remaining = LEGACY_S16_WRAP_SUB(frames_remaining, 1);
			displayed_frame = LEGACY_U16_WRAP_ADD(elapsed_time2,
				replay_rewind_interpolate(amount,
					(legacy_u16)frames_remaining,
					(legacy_u16)frames_to_catch_up));
			replay_controls_draw(displayed_frame, elapsed_time2);
			input_do_checking(1);
		}
	}
	replay_controls_draw(state.game_frame, state.game_frame);
	input_do_checking(1000);
}

static legacy_s16 replay_try_zoom(legacy_u16 input)
{
	if (input == '-') {
		if (cameramode == 3) {
			if (camera_track_height_offset <= 0)
				return 0;
			camera_track_height_offset = LEGACY_S16_WRAP_SUB(
				camera_track_height_offset, 0x1E);
		} else {
			if (custom_camera.distance >= 0x5DC)
				return 0;
			custom_camera.distance = LEGACY_S16_WRAP_ADD(
				custom_camera.distance, 0x1E);
		}
	} else {
		if (cameramode == 3) {
			if (camera_track_height_offset >= 0x384)
				return 0;
			camera_track_height_offset = LEGACY_S16_WRAP_ADD(
				camera_track_height_offset, 0x1E);
		} else {
			if (custom_camera.distance <= 0x78)
				return 0;
			custom_camera.distance = LEGACY_S16_WRAP_SUB(
				custom_camera.distance, 0x1E);
		}
	}
	return 1;
}

void loop_game(legacy_s16 operation, legacy_s16 recorded_frame, legacy_s16 current_frame)
{
	legacy_u16 input;
	legacy_s16 delta;
	legacy_s16 midpoint;
	legacy_s16 x_delta;
	legacy_s16 y_delta;
	legacy_u16 angle;
	legacy_u8 hit;
	legacy_u8 next_selection;
	legacy_u8 custom_camera_active;

	if (operation == 0) {
		locate_many_resources((legacy_s8 far*)sdgameresptr,
			aRplyrpicrpacrpmcrptcbof6bof5b,
			(legacy_s8 far**)rplyshapes);
		replay_controls_select(4);
		return;
	}
	if (operation == 1) {
		replay_controls_draw(recorded_frame, current_frame);
		return;
	}
	if (operation == 2) {
		replay_controls_select((legacy_u8)recorded_frame);
		return;
	}
	if (operation != 3)
		return;

	if (LEGACY_S8_FROM_BITS(byte_3E9DB) > LEGACY_S8_FROM_BITS(
		game_camera_buttons_count[(legacy_u8)cameramode]) &&
		cameramode != 2)
		byte_3E9DB = game_camera_buttons_count[(legacy_u8)cameramode];
	sprite_copy_2_to_1();
	if (video_flag5_is0 != 0)
		byte_4432A = byte_44346 ^ 1;

	for (;;) {
	delta = LEGACY_S16_FROM_BITS((legacy_u16)timer_get_delta_alt());
	input = (legacy_u16)input_checking(delta);
	hit = (legacy_u8)mouse_multi_hittest(
		(legacy_u8)(game_camera_buttons_count[(legacy_u8)cameramode] + 1U),
		game_camera_buttons);
	if (hit != 0xFFU) {
		if (hit != byte_3E9DB && input == 0)
			input = 1;
		byte_3E9DB = hit;
		if ((input == KEY_ENTER || input == KEY_SPACE) && byte_3E9DB >= 7U) {
			if (byte_3E9DB == 7U) {
				midpoint = LEGACY_S16_SAR(LEGACY_S16_WRAP_ADD(
					word_3EA3A, word_3EA4C), 1U);
				input = midpoint < mouse_ypos ? KEY_DOWN : KEY_UP;
			} else {
				y_delta = LEGACY_S16_WRAP_SUB(
					LEGACY_S16_SAR(LEGACY_S16_WRAP_ADD(
						word_3EA3C, word_3EA4E), 1U),
					(legacy_s16)mouse_ypos);
				x_delta = LEGACY_S16_WRAP_SUB((legacy_s16)mouse_xpos,
					LEGACY_S16_SAR(LEGACY_S16_WRAP_ADD(
						word_3EA18, word_3EA2A), 1U));
				angle = (legacy_u16)polarAngle(x_delta, y_delta);
				switch (((angle + 0x80U) >> 8) & 3U) {
				case 0:
					input = KEY_UP;
					break;
				case 1:
					input = KEY_RIGHT;
					break;
				case 2:
					input = KEY_DOWN;
					break;
				default:
					input = KEY_LEFT;
					break;
				}
			}
		}
	} else {
		hit = (legacy_u8)mouse_multi_hittest(1, &gameunk_button);
		if (hit == 0 && (input == KEY_ENTER || input == KEY_SPACE))
			input = 'c';
	}

	if (input != 0 && input != KEY_ESCAPE &&
		(legacy_u8)handle_ingame_kb_shortcuts(input) != 0)
		return;
	if (is_in_replay == 0 && input == 0) {
		if (replaybar_enabled != 0)
			replay_controls_draw(state.game_frame, state.game_frame);
		return;
	}
	if (replaybar_enabled == 0) {
		is_in_replay_copy = (legacy_s8)0xFF;
		word_449EA = -1;
	}
	if (is_in_replay != 0 && (byte_40E6D != 0 || byte_40E6C != 0))
		replay_controls_select(4);
	replay_controls_draw(state.game_frame, state.game_frame);

	custom_camera_active = 0;
	if (kb_get_key_state(0x1D) != 0 ||
		(byte_3E9DB == 8U && ((legacy_u8)input_combined_flags & 0x30U) != 0))
		custom_camera_active = 1;
	if (custom_camera_active != 0) {
		switch (input) {
		case KEY_RIGHT:
			custom_camera.azimuth_angle = LEGACY_S16_WRAP_ADD(
				custom_camera.azimuth_angle, 0x10);
			return;
		case KEY_LEFT:
			custom_camera.azimuth_angle = LEGACY_S16_WRAP_SUB(
				custom_camera.azimuth_angle, 0x10);
			return;
		case KEY_UP:
			if (LEGACY_S16_WRAP_ADD(custom_camera.elevation_angle,
				0x10) < 0x100) {
				custom_camera.elevation_angle = LEGACY_S16_WRAP_ADD(
					custom_camera.elevation_angle, 0x10);
				return;
			}
			input = 0;
			break;
		case KEY_DOWN:
			if (LEGACY_S16_WRAP_SUB(custom_camera.elevation_angle,
				0x10) > -0x100) {
				custom_camera.elevation_angle = LEGACY_S16_WRAP_SUB(
					custom_camera.elevation_angle, 0x10);
				return;
			}
			input = 0;
			break;
		case '+':
		case '-':
			break;
		default:
			input = 0;
			break;
		}
	}

	if ((input == '-' || input == '+') && replay_try_zoom(input) != 0)
		return;

	switch (input) {
	case KEY_ENTER:
	case KEY_SPACE:
		if (byte_3E9DB > 6U)
			break;
		switch (byte_3E9DB) {
		case 0:
			replay_fast_forward();
			return;
		case 1:
			replay_rewind();
			return;
		case 2:
			replay_controls_select(2);
			byte_449E6 = 3;
			is_in_replay = 0;
			break;
		case 3:
			byte_449E6 = 0;
			replay_controls_select(3);
			is_in_replay = 0;
			break;
		case 4:
			is_in_replay = 1;
			audio_carstate();
			replay_controls_select(4);
			replay_controls_draw(state.game_frame, state.game_frame);
			break;
		case 5:
			is_in_replay = 1;
			audio_carstate();
			replay_controls_select(5);
			replay_controls_draw(state.game_frame, state.game_frame);
			restore_gamestate(0);
			(void)timer_get_counter_unk(50UL);
			replay_controls_select(4);
			replay_controls_draw(state.game_frame, state.game_frame);
			return;
		case 6:
			replay_pause_menu();
			return;
		}
		break;

	case KEY_ESCAPE:
		replay_pause_menu();
		return;

	case KEY_LEFT:
		next_selection = byte_3E9DC[byte_3E9DB];
		if (next_selection <=
			game_camera_buttons_count[(legacy_u8)cameramode])
			byte_3E9DB = next_selection;
		break;

	case KEY_RIGHT:
		byte_3E9DB = byte_3E9E6[byte_3E9DB];
		break;

	case KEY_UP:
		if (byte_3E9DB == 7U) {
			if (replay_try_zoom('+') != 0)
				return;
			break;
		}
		byte_3E9DB = byte_3E9F0[byte_3E9DB];
		break;

	case KEY_DOWN:
		if (byte_3E9DB == 7U) {
			if (replay_try_zoom('-') != 0)
				return;
			break;
		}
		byte_3E9DB = byte_3E9FA[byte_3E9DB];
		break;
	}

	replay_controls_draw(state.game_frame, state.game_frame);
	}
}
