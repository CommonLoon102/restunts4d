#include "fileio.h"
#include "game_input.h"
#include "math.h"
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

#define REPLAY_PLAYER_COUNT 2U
#define REPLAY_CONTROL_COUNT 9U
#define REPLAY_ACTION_CONTROL_COUNT 7U
#define REPLAY_LAST_ACTION_CONTROL (REPLAY_ACTION_CONTROL_COUNT - 1U)
#define REPLAY_CONTROL_PLAYER_STRIDE REPLAY_PLAYER_COUNT
#define REPLAY_NO_SELECTION 255U

#define REPLAY_SHAPE_BACKGROUND 0U
#define REPLAY_SHAPE_CAMERA_FIRST 1U
#define REPLAY_SHAPE_CONTROL_INACTIVE_FIRST 5U
#define REPLAY_SHAPE_CONTROL_ACTIVE_FIRST 14U

#define REPLAY_TOTAL_TIME_X 216
#define REPLAY_CURRENT_TIME_X 152
#define REPLAY_TIME_Y 187
#define REPLAY_TIMELINE_X 154
#define REPLAY_TIMELINE_Y 177
#define REPLAY_TIMELINE_WIDTH 116
#define REPLAY_TIMELINE_POSITION_RANGE 110U
#define REPLAY_TIMELINE_HEIGHT 6
#define REPLAY_TIMELINE_CURSOR_EDGE_OFFSET 5
#define REPLAY_TIMELINE_CURSOR_RIGHT_X \
	(REPLAY_TIMELINE_X + REPLAY_TIMELINE_CURSOR_EDGE_OFFSET)
#define REPLAY_TIMELINE_CURSOR_BOTTOM_Y \
	(REPLAY_TIMELINE_Y + REPLAY_TIMELINE_CURSOR_EDGE_OFFSET)
#define REPLAY_WAITING_TEXT_Y 100
#define REPLAY_TIME_INCLUDE_FRACTION 1

#define REPLAY_PAUSE_OPTION_COUNT 8U
#define REPLAY_MODE_OPTION_COUNT 5U
#define CAR_ID_LENGTH 4U
#define TRACK_SKYBOX_ELEMENT_INDEX 900U
#define REPLAY_LOAD_WAIT_VALUE 150

#define REPLAY_SCRUB_ACCELERATION_DIVISOR 50L
#define REPLAY_SCRUB_INITIAL_SPEED 3
#define REPLAY_SCRUB_FIXED_SCALE 20L
#define REPLAY_SCRUB_MAX_SPEED 100
#define REPLAY_INPUT_SETTLE_DELTA 1000
#define REPLAY_RESTART_WAIT_TICKS 50UL

#define CAMERA_MODE_COCKPIT 0
#define CAMERA_MODE_CUSTOM 2
#define CAMERA_MODE_TRACKSIDE 3
#define CAMERA_MODE_COUNT 4U
#define CAMERA_MODE_MASK (CAMERA_MODE_COUNT - 1U)
#define REPLAY_CAMERA_ZOOM_STEP 30
#define REPLAY_CUSTOM_CAMERA_MIN_DISTANCE 120
#define REPLAY_CUSTOM_CAMERA_MAX_DISTANCE 1500
#define REPLAY_TRACK_CAMERA_MAX_HEIGHT 900
#define REPLAY_CAMERA_ANGLE_STEP 16
#define REPLAY_CUSTOM_CAMERA_ELEVATION_LIMIT ANGLE_QUARTER_TURN
#define REPLAY_CUSTOM_CAMERA_MODIFIER_SCAN_CODE 29

#define REPLAY_DIRECTION_ANGLE_SHIFT 8U
#define REPLAY_DIRECTION_MASK 3U
#define REPLAY_DIRECTION_UP 0U
#define REPLAY_DIRECTION_RIGHT 1U
#define REPLAY_DIRECTION_DOWN 2U

#define REPLAY_CONTROL_FAST_FORWARD 0U
#define REPLAY_CONTROL_REWIND 1U
#define REPLAY_CONTROL_FAST_PLAY 2U
#define REPLAY_CONTROL_PLAY 3U
#define REPLAY_CONTROL_PAUSE 4U
#define REPLAY_CONTROL_RESTART 5U
#define REPLAY_CONTROL_MENU 6U
#define REPLAY_CONTROL_ZOOM 7U
#define REPLAY_CONTROL_PAN 8U

#define REPLAY_LOOP_LOAD_RESOURCES 0
#define REPLAY_LOOP_DRAW_CONTROLS 1
#define REPLAY_LOOP_SELECT_CONTROL 2
#define REPLAY_LOOP_HANDLE_INPUT 3

#define REPLAY_PAUSE_ACTION_FINISH 1
#define REPLAY_PAUSE_ACTION_RESTART 2
#define REPLAY_PAUSE_ACTION_CONTINUE 3
#define REPLAY_PAUSE_ACTION_LOAD 4
#define REPLAY_PAUSE_ACTION_SAVE 5
#define REPLAY_PAUSE_ACTION_DISPLAY_OPTIONS 6
#define REPLAY_PAUSE_ACTION_EXIT 7

#define REPLAY_MODE_ACTION_DASHBOARD 0
#define REPLAY_MODE_ACTION_BAR 1
#define REPLAY_MODE_ACTION_CAMERA 2
#define REPLAY_MODE_ACTION_DETAIL 3
#define REPLAY_MODE_ACTION_FOLLOW_OPPONENT 4

#define REPLAY_DIALOG_INITIAL_CHOICE 0
#define REPLAY_DIALOG_ACCEPTED_MINIMUM 1

#define REPLAY_SAVE_CANCELLED (-1)
#define REPLAY_SAVE_RETRY 0
#define REPLAY_SAVE_READY 1
#define REPLAY_FILE_CHECK_ARGUMENT 2
#define REPLAY_INITIAL_GAME_FRAME (-1)

#define REPLAY_PLAYBACK_NORMAL 0
#define REPLAY_PLAYBACK_FAST 3
#define REPLAY_EXIT_CRASH_STATE 4
#define REPLAY_EXIT_REQUESTED 2
#define PLAYER_CAR_INDEX 0
#define REPLAY_RECORDING_ACTIVE_FLAG 1U
#define REPLAY_RECORDING_MODIFIED_FLAG 2U
#define REPLAY_RECORDING_RESTARTABLE_FLAG 4U
#define REPLAY_BAR_HIDDEN_STATE (-1)
#define REPLAY_FIRST_FRAME 0
#define REPLAY_SINGLE_FRAME_DELTA 1

extern void far* fontledresptr;
extern void far* sdgameresptr;
extern legacy_s16 camera_track_height_offset;


static void replay_controls_select(legacy_u8 selection)
{
	legacy_u16 index;

	for (index = 0; index < REPLAY_CONTROL_COUNT; index++)
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
		byte_40E74[player_index] = REPLAY_NO_SELECTION;
		byte_40E08[player_index] = REPLAY_NO_SELECTION;
		for (index = 0; index < REPLAY_CONTROL_COUNT; index++)
			byte_40E7A[player_index +
				index * REPLAY_CONTROL_PLAYER_STRIDE] = 0;
		mouse_draw_opaque_check();
		shape2d_op_unk(rplyshapes[REPLAY_SHAPE_BACKGROUND]);
		word_40E0A[player_index] = -1;
		word_40E76[player_index] = -1;
		format_frame_as_string(&resID_byte1,
			(legacy_u16)(gameconfig.game_recordedframes + elapsed_time1),
			REPLAY_TIME_INCLUDE_FRACTION);
		font_set_unk(dialog_fnt_colour, 0);
		font_set_fontdef2(fontledresptr);
		sub_345BC(&resID_byte1, REPLAY_TOTAL_TIME_X, REPLAY_TIME_Y);
		font_set_fontdef();
	}

	displayed_time = (legacy_u16)(current_frame + elapsed_time1);
	if ((legacy_u16)word_40E0A[player_index] != displayed_time) {
		word_40E0A[player_index] = (legacy_s16)displayed_time;
		format_frame_as_string(&resID_byte1, displayed_time,
			REPLAY_TIME_INCLUDE_FRACTION);
		font_set_unk(dialog_fnt_colour, 0);
		mouse_draw_opaque_check();
		font_set_fontdef2(fontledresptr);
		sub_345BC(&resID_byte1, REPLAY_CURRENT_TIME_X, REPLAY_TIME_Y);
		font_set_fontdef();
	}

	if (byte_40E74[player_index] != (legacy_u8)cameramode) {
		byte_40E74[player_index] = (legacy_u8)cameramode;
		word_40E76[player_index] = -1;
		mouse_draw_opaque_check();
		shape2d_op_unk(rplyshapes[
			REPLAY_SHAPE_CAMERA_FIRST + (legacy_u8)cameramode]);
		if (LEGACY_S8_FROM_BITS(byte_3E9DB) > LEGACY_S8_FROM_BITS(
			game_camera_buttons_count[(legacy_u8)cameramode]))
			byte_3E9DB = game_camera_buttons_count[(legacy_u8)cameramode];
		if (byte_40E08[player_index] > REPLAY_LAST_ACTION_CONTROL)
			byte_40E08[player_index] = REPLAY_NO_SELECTION;
	}

	recorded_position = (legacy_s16)replay_timeline_position(
		recorded_frame, gameconfig.game_recordedframes,
		REPLAY_TIMELINE_POSITION_RANGE);
	current_position = (legacy_s16)replay_timeline_position(
		current_frame, gameconfig.game_recordedframes,
		REPLAY_TIMELINE_POSITION_RANGE);
	if (word_40E76[player_index] != recorded_position ||
		word_40E04[player_index] != current_position) {
		mouse_draw_opaque_check();
		word_40E76[player_index] = recorded_position;
		word_40E04[player_index] = current_position;
		sprite_1_unk(REPLAY_TIMELINE_X, REPLAY_TIMELINE_Y,
			REPLAY_TIMELINE_WIDTH, REPLAY_TIMELINE_HEIGHT, word_407FC);
		sprite_1_unk(LEGACY_S16_WRAP_ADD(
			REPLAY_TIMELINE_X, recorded_position),
			REPLAY_TIMELINE_Y, REPLAY_TIMELINE_HEIGHT,
			REPLAY_TIMELINE_HEIGHT, dialog_fnt_colour);
		sprite_1_unk4(LEGACY_S16_WRAP_ADD(
			REPLAY_TIMELINE_X, current_position),
			REPLAY_TIMELINE_Y,
			LEGACY_S16_WRAP_ADD(
				REPLAY_TIMELINE_CURSOR_RIGHT_X, current_position),
			REPLAY_TIMELINE_CURSOR_BOTTOM_Y,
			word_407FE);
	}

	state_changed = byte_40E08[player_index] != byte_3E9DB;
	if (state_changed == 0) {
		for (index = 0; index < REPLAY_ACTION_CONTROL_COUNT; index++) {
			if (byte_40E7A[player_index +
				index * REPLAY_CONTROL_PLAYER_STRIDE] !=
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
	if (previous_selection != REPLAY_NO_SELECTION) {
		if (byte_40E7A[player_index +
			previous_selection * REPLAY_CONTROL_PLAYER_STRIDE] != 0)
			shape2d_op_unk(rplyshapes[
				REPLAY_SHAPE_CONTROL_ACTIVE_FIRST + previous_selection]);
		else
			shape2d_op_unk(rplyshapes[
				REPLAY_SHAPE_CONTROL_INACTIVE_FIRST + previous_selection]);
		byte_40E08[player_index] = REPLAY_NO_SELECTION;
	}
	for (index = 0; index < REPLAY_ACTION_CONTROL_COUNT; index++) {
		if (byte_40E6A[index] == 0 &&
			byte_40E7A[player_index +
				index * REPLAY_CONTROL_PLAYER_STRIDE] != 0) {
			shape2d_op_unk(rplyshapes[
				REPLAY_SHAPE_CONTROL_INACTIVE_FIRST + index]);
			byte_40E7A[player_index +
				index * REPLAY_CONTROL_PLAYER_STRIDE] = 0;
		}
	}
	for (index = 0; index < REPLAY_ACTION_CONTROL_COUNT; index++) {
		if (byte_40E6A[index] != 0) {
			byte_40E7A[player_index +
				index * REPLAY_CONTROL_PLAYER_STRIDE] = 1;
			shape2d_op_unk(rplyshapes[
				REPLAY_SHAPE_CONTROL_ACTIVE_FIRST + index]);
			byte_40E7A[player_index +
				index * REPLAY_CONTROL_PLAYER_STRIDE] = 1;
		}
	}
	byte_40E08[player_index] = byte_3E9DB;
	if (byte_3E9DB != REPLAY_NO_SELECTION) {
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
		font_op2_alt(&resID_byte1), REPLAY_WAITING_TEXT_Y,
		dialog_fnt_colour, 0);
	if (slow_video_mgmt_copy != 0)
		rect_union(rectptr_unk2, text_rectangle, rectptr_unk2);
}

static void replay_pause_menu(void)
{
	struct GAMEINFO saved_config;
	legacy_s16 options[REPLAY_PAUSE_OPTION_COUNT];
	legacy_s16 mode_options[REPLAY_MODE_OPTION_COUNT];
	legacy_s16 dialog_result;
	legacy_s8 menu_result;
	legacy_s8 save_status;
	legacy_u16 index;
	legacy_u8 saved_track;
	legacy_s16 resources_changed;
	legacy_s16 opponent_changed;

	is_in_replay = 1;
	audio_carstate();
	replay_controls_select(REPLAY_CONTROL_PAUSE);
	replay_controls_draw(state.game_frame, state.game_frame);
	for (index = 0; index < REPLAY_PAUSE_OPTION_COUNT; index++)
		options[index] = 0;
	if (state.playerstate.car_crashBmpFlag != 0)
		options[REPLAY_PAUSE_ACTION_CONTINUE] = 1;
	if (gameconfig.game_recordedframes == 0 || elapsed_time1 != 0)
		options[REPLAY_PAUSE_ACTION_SAVE] = 1;
	if (passed_security == 0) {
		options[REPLAY_PAUSE_ACTION_RESTART] = 1;
		options[REPLAY_PAUSE_ACTION_CONTINUE] = 1;
	}
	if (((legacy_u8)byte_43966 & REPLAY_RECORDING_RESTARTABLE_FLAG) == 0)
		options[REPLAY_PAUSE_ACTION_FINISH] = 1;
	byte_454A4 = (legacy_u8)video_flag6_is1;
	menu_result = LEGACY_S8_FROM_BITS(show_dialog(DIALOG_TYPE_MENU,
		DIALOG_NO_BACKGROUND_SAVE,
		locate_text_res(gameresptr, aMen_0),
		DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
		dialogarg2, options, REPLAY_DIALOG_INITIAL_CHOICE));

	switch (menu_result) {
	case REPLAY_PAUSE_ACTION_FINISH:
		update_crash_state(REPLAY_EXIT_CRASH_STATE, PLAYER_CAR_INDEX);
		byte_449DA = REPLAY_EXIT_REQUESTED;
		break;

	case REPLAY_PAUSE_ACTION_RESTART:
		check_input();
		init_game_state_with_frame_rate_byte(framespersec2);
		elapsed_time2 = 0;
		gameconfig.game_recordedframes = 0;
		word_45D3E = LEGACY_S16_FROM_BITS(
			LEGACY_U16_REPLACE_LOW_BYTE(word_45D3E, 0U));
		byte_43966 = REPLAY_RECORDING_ACTIVE_FLAG;
		/* fall through */

	case REPLAY_PAUSE_ACTION_CONTINUE:
		if (menu_result == REPLAY_PAUSE_ACTION_CONTINUE) {
			if (((legacy_u8)byte_43966 &
				REPLAY_RECORDING_MODIFIED_FLAG) != 0) {
				byte_43966 = REPLAY_RECORDING_ACTIVE_FLAG |
					REPLAY_RECORDING_MODIFIED_FLAG;
			} else if (gameconfig.game_recordedframes != elapsed_time2) {
				dialog_result = LEGACY_S16_FROM_BITS(show_dialog(
					DIALOG_TYPE_MENU, DIALOG_NO_BACKGROUND_SAVE,
					locate_text_res(gameresptr, aCon_0),
					DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
					performGraphColor, 0, REPLAY_DIALOG_INITIAL_CHOICE));
				if (dialog_result < REPLAY_DIALOG_ACCEPTED_MINIMUM)
					break;
				byte_43966 = REPLAY_RECORDING_ACTIVE_FLAG |
					REPLAY_RECORDING_MODIFIED_FLAG;
			} else {
				byte_43966 = REPLAY_RECORDING_ACTIVE_FLAG;
			}
			elapsed_time2 = (legacy_u16)state.game_frame;
			gameconfig.game_recordedframes = (legacy_u16)state.game_frame;
		}
		dashb_toggle = 1;
		show_penalty_counter = 0;
		followOpponentFlag = 0;
		game_replay_mode = 0;
		cameramode = CAMERA_MODE_COCKPIT;
		state.game_3F6autoLoadEvalFlag = 0;
		state.game_frame_in_sec = 0;
		byte_449E6 = REPLAY_PLAYBACK_NORMAL;
		replay_controls_select(REPLAY_CONTROL_PLAY);
		is_in_replay = 0;
		mouse_minmax_position(LEGACY_S8_FROM_BITS(byte_3B8F2));
		check_input();
		kbormouse = 0;
		break;

	case REPLAY_PAUSE_ACTION_LOAD:
		byte_43966 = 0;
		audio_carstate();
		if (do_fileselect_dialog(byte_3B85E, aDefault_1, ".rpl",
			locate_text_res(mainresptr, "rep")) == 0)
			break;
		waitflag = REPLAY_LOAD_WAIT_VALUE;
		show_waiting();
		saved_config = gameconfig;
		saved_track = td14_elem_map_main[TRACK_SKYBOX_ELEMENT_INDEX];
		if ((legacy_u8)file_load_replay(byte_3B85E, aDefault_1) != 0)
			gameconfig.game_recordedframes = 0;
		dashb_toggle = 0;
		track_setup();
		resources_changed =
			td14_elem_map_main[TRACK_SKYBOX_ELEMENT_INDEX] != saved_track;
		for (index = 0; index < CAR_ID_LENGTH; index++) {
			if (saved_config.game_playercarid[index] !=
				gameconfig.game_playercarid[index])
				resources_changed = 1;
		}
		if (saved_config.game_opponenttype !=
			gameconfig.game_opponenttype) {
			resources_changed = 1;
		} else if (gameconfig.game_opponenttype != 0) {
			opponent_changed = 0;
			for (index = 0; index < CAR_ID_LENGTH; index++) {
				if (saved_config.game_opponentcarid[index] !=
					gameconfig.game_opponentcarid[index]) {
					resources_changed = 1;
					opponent_changed = 1;
				}
			}
			if (opponent_changed == 0) {
				ensure_file_exists(REPLAY_FILE_CHECK_ARGUMENT);
				load_opponent_data();
			}
		}
		if (resources_changed != 0) {
			free_player_cars();
			setup_player_cars();
		}
		framespersec = (legacy_s16)LEGACY_S8_FROM_BITS(
			LEGACY_U16_LOW_BYTE(gameconfig.game_framespersec));
		init_game_state(REPLAY_INITIAL_GAME_FRAME);
		break;

	case REPLAY_PAUSE_ACTION_SAVE:
		audio_carstate();
		for (;;) {
			save_status = REPLAY_SAVE_RETRY;
			if (do_savefile_dialog(byte_3B85E, aDefault_1,
				locate_text_res(mainresptr, aRep_1)) == 0) {
				save_status = REPLAY_SAVE_CANCELLED;
			} else {
				file_build_path(byte_3B85E, aDefault_1, a_rpl_2,
					g_path_buf);
				save_status = REPLAY_SAVE_READY;
				g_is_busy = 1;
				if (file_find(g_path_buf) != 0) {
					dialog_result = LEGACY_S16_FROM_BITS(show_dialog(
						DIALOG_TYPE_MENU, DIALOG_NO_BACKGROUND_SAVE,
						locate_text_res(mainresptr, aFex_0),
						DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
						performGraphColor, 0,
						REPLAY_DIALOG_INITIAL_CHOICE));
					if (dialog_result == REPLAY_SAVE_CANCELLED)
						save_status = REPLAY_SAVE_CANCELLED;
					else if (dialog_result == REPLAY_SAVE_RETRY)
						save_status = REPLAY_SAVE_RETRY;
				}
				g_is_busy = 0;
			}
			if (save_status != REPLAY_SAVE_READY)
				break;
			if ((legacy_u8)file_write_replay(g_path_buf) == 0)
				break;
			show_dialog(DIALOG_TYPE_ACKNOWLEDGEMENT,
				DIALOG_NO_BACKGROUND_SAVE,
				locate_text_res(mainresptr, aSer_0),
				DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
				performGraphColor, 0, REPLAY_DIALOG_INITIAL_CHOICE);
		}
		break;

	case REPLAY_PAUSE_ACTION_DISPLAY_OPTIONS:
		for (index = 0; index < REPLAY_MODE_OPTION_COUNT; index++)
			mode_options[index] = 0;
		if (gameconfig.game_opponenttype == 0)
			mode_options[REPLAY_MODE_ACTION_FOLLOW_OPPONENT] = 1;
		menu_result = LEGACY_S8_FROM_BITS(show_dialog(DIALOG_TYPE_MENU,
			DIALOG_NO_BACKGROUND_SAVE,
			locate_text_res(gameresptr, aMdo),
			DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
			dialogarg2, mode_options, REPLAY_DIALOG_INITIAL_CHOICE));
		switch (menu_result) {
		case REPLAY_MODE_ACTION_DASHBOARD:
			dashb_toggle ^= 1;
			break;
		case REPLAY_MODE_ACTION_BAR:
			replaybar_toggle ^= 1;
			break;
		case REPLAY_MODE_ACTION_CAMERA:
			cameramode = (legacy_s8)(((legacy_u8)cameramode + 1U) &
				CAMERA_MODE_MASK);
			break;
		case REPLAY_MODE_ACTION_DETAIL:
			show_graphic_levels_menu();
			break;
		case REPLAY_MODE_ACTION_FOLLOW_OPPONENT:
			followOpponentFlag ^= 1;
			break;
		}
		break;

	case REPLAY_PAUSE_ACTION_EXIT:
		update_crash_state(REPLAY_EXIT_CRASH_STATE, PLAYER_CAR_INDEX);
		byte_43966 = 0;
		byte_449DA = REPLAY_EXIT_REQUESTED;
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

	quotient = LEGACY_S32_DIV_OR_ZERO(
		accumulated, REPLAY_SCRUB_ACCELERATION_DIVISOR);
	return LEGACY_S16_WRAP_ADD(
		LEGACY_S16_FROM_BITS((legacy_u16)quotient),
		REPLAY_SCRUB_INITIAL_SPEED);
}

static legacy_u16 replay_scrub_amount(legacy_s32 accumulated)
{
	return (legacy_u16)LEGACY_S32_DIV_OR_ZERO(
		accumulated, REPLAY_SCRUB_FIXED_SCALE);
}

static legacy_s32 replay_scrub_begin(legacy_u8 selection)
{
	is_in_replay = 1;
	audio_carstate();
	replay_controls_select(selection);
	(void)timer_get_delta_alt();
	return REPLAY_SCRUB_FIXED_SCALE;
}

static legacy_s32 replay_scrub_advance(legacy_s32 accumulated,
	legacy_u16* delta)
{
	legacy_s16 speed;

	speed = replay_scrub_speed(accumulated);
	if (speed > REPLAY_SCRUB_MAX_SPEED)
		speed = REPLAY_SCRUB_MAX_SPEED;
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

	accumulated = replay_scrub_begin(REPLAY_CONTROL_FAST_FORWARD);
	while (((legacy_u8)input_combined_flags & INPUT_ACTION_BUTTON_MASK) != 0) {
		accumulated = replay_scrub_advance(accumulated, &delta);
		remaining = LEGACY_U16_WRAP_SUB(
			gameconfig.game_recordedframes, elapsed_time2);
		amount = replay_scrub_amount(accumulated);
		if (amount > remaining)
			accumulated = LEGACY_S32_WRAP_MUL(
				(legacy_s32)remaining, REPLAY_SCRUB_FIXED_SCALE);
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
			(legacy_s32)remaining, REPLAY_SCRUB_FIXED_SCALE);
		amount = remaining;
	}
	target = LEGACY_U16_WRAP_ADD(elapsed_time2, amount);
	if (LEGACY_S16_FROM_BITS(target) >
		LEGACY_S16_FROM_BITS(gameconfig.game_recordedframes))
		target = gameconfig.game_recordedframes;
	restore_gamestate(target);
	elapsed_time2 = target;
	replay_controls_select(REPLAY_CONTROL_PAUSE);
	replay_draw_waiting();
	while ((legacy_u16)state.game_frame != elapsed_time2) {
		update_gamestate();
		replay_controls_draw(state.game_frame, elapsed_time2);
	}
	input_do_checking(REPLAY_INPUT_SETTLE_DELTA);
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

	accumulated = replay_scrub_begin(REPLAY_CONTROL_REWIND);
	while (((legacy_u8)input_combined_flags & INPUT_ACTION_BUTTON_MASK) != 0) {
		accumulated = replay_scrub_advance(accumulated, &delta);
		amount = replay_scrub_amount(accumulated);
		if (amount > elapsed_time2)
			accumulated = LEGACY_S32_WRAP_MUL(
				(legacy_s32)elapsed_time2, REPLAY_SCRUB_FIXED_SCALE);
		amount = replay_scrub_amount(accumulated);
		replay_controls_draw(state.game_frame,
			LEGACY_U16_WRAP_SUB(elapsed_time2, amount));
		input_do_checking(LEGACY_S16_FROM_BITS(delta));
	}

	amount = replay_scrub_amount(accumulated);
	if (amount > elapsed_time2)
		amount = elapsed_time2;
	replay_controls_select(REPLAY_CONTROL_PAUSE);
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
			frames_remaining = LEGACY_S16_WRAP_SUB(
				frames_remaining, REPLAY_SINGLE_FRAME_DELTA);
			displayed_frame = LEGACY_U16_WRAP_ADD(elapsed_time2,
				replay_rewind_interpolate(amount,
					(legacy_u16)frames_remaining,
					(legacy_u16)frames_to_catch_up));
			replay_controls_draw(displayed_frame, elapsed_time2);
			input_do_checking(REPLAY_SINGLE_FRAME_DELTA);
		}
	}
	replay_controls_draw(state.game_frame, state.game_frame);
	input_do_checking(REPLAY_INPUT_SETTLE_DELTA);
}

static legacy_s16 replay_try_zoom(legacy_u16 input)
{
	if (input == '-') {
		if (cameramode == CAMERA_MODE_TRACKSIDE) {
			if (camera_track_height_offset <= 0)
				return 0;
			camera_track_height_offset = LEGACY_S16_WRAP_SUB(
				camera_track_height_offset, REPLAY_CAMERA_ZOOM_STEP);
		} else {
			if (custom_camera.distance >= REPLAY_CUSTOM_CAMERA_MAX_DISTANCE)
				return 0;
			custom_camera.distance = LEGACY_S16_WRAP_ADD(
				custom_camera.distance, REPLAY_CAMERA_ZOOM_STEP);
		}
	} else {
		if (cameramode == CAMERA_MODE_TRACKSIDE) {
			if (camera_track_height_offset >= REPLAY_TRACK_CAMERA_MAX_HEIGHT)
				return 0;
			camera_track_height_offset = LEGACY_S16_WRAP_ADD(
				camera_track_height_offset, REPLAY_CAMERA_ZOOM_STEP);
		} else {
			if (custom_camera.distance <= REPLAY_CUSTOM_CAMERA_MIN_DISTANCE)
				return 0;
			custom_camera.distance = LEGACY_S16_WRAP_SUB(
				custom_camera.distance, REPLAY_CAMERA_ZOOM_STEP);
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

	if (operation == REPLAY_LOOP_LOAD_RESOURCES) {
		locate_many_resources((legacy_s8 far*)sdgameresptr,
			aRplyrpicrpacrpmcrptcbof6bof5b,
			(legacy_s8 far**)rplyshapes);
		replay_controls_select(REPLAY_CONTROL_PAUSE);
		return;
	}
	if (operation == REPLAY_LOOP_DRAW_CONTROLS) {
		replay_controls_draw(recorded_frame, current_frame);
		return;
	}
	if (operation == REPLAY_LOOP_SELECT_CONTROL) {
		replay_controls_select((legacy_u8)recorded_frame);
		return;
	}
	if (operation != REPLAY_LOOP_HANDLE_INPUT)
		return;

	if (LEGACY_S8_FROM_BITS(byte_3E9DB) > LEGACY_S8_FROM_BITS(
		game_camera_buttons_count[(legacy_u8)cameramode]) &&
		cameramode != CAMERA_MODE_CUSTOM)
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
	if (hit != REPLAY_NO_SELECTION) {
		if (hit != byte_3E9DB && input == 0)
			input = 1;
		byte_3E9DB = hit;
		if ((input == KEY_ENTER || input == KEY_SPACE) &&
			byte_3E9DB >= REPLAY_CONTROL_ZOOM) {
			if (byte_3E9DB == REPLAY_CONTROL_ZOOM) {
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
				switch (((angle + ANGLE_EIGHTH_TURN) >>
					REPLAY_DIRECTION_ANGLE_SHIFT) & REPLAY_DIRECTION_MASK) {
				case REPLAY_DIRECTION_UP:
					input = KEY_UP;
					break;
				case REPLAY_DIRECTION_RIGHT:
					input = KEY_RIGHT;
					break;
				case REPLAY_DIRECTION_DOWN:
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
		is_in_replay_copy = (legacy_s8)REPLAY_BAR_HIDDEN_STATE;
		word_449EA = -1;
	}
	if (is_in_replay != 0 && (byte_40E6D != 0 || byte_40E6C != 0))
		replay_controls_select(REPLAY_CONTROL_PAUSE);
	replay_controls_draw(state.game_frame, state.game_frame);

	custom_camera_active = 0;
	if (kb_get_key_state(REPLAY_CUSTOM_CAMERA_MODIFIER_SCAN_CODE) != 0 ||
		(byte_3E9DB == REPLAY_CONTROL_PAN &&
			((legacy_u8)input_combined_flags & INPUT_ACTION_BUTTON_MASK) != 0))
		custom_camera_active = 1;
	if (custom_camera_active != 0) {
		switch (input) {
		case KEY_RIGHT:
			custom_camera.azimuth_angle = LEGACY_S16_WRAP_ADD(
				custom_camera.azimuth_angle, REPLAY_CAMERA_ANGLE_STEP);
			return;
		case KEY_LEFT:
			custom_camera.azimuth_angle = LEGACY_S16_WRAP_SUB(
				custom_camera.azimuth_angle, REPLAY_CAMERA_ANGLE_STEP);
			return;
		case KEY_UP:
			if (LEGACY_S16_WRAP_ADD(custom_camera.elevation_angle,
				REPLAY_CAMERA_ANGLE_STEP) <
				REPLAY_CUSTOM_CAMERA_ELEVATION_LIMIT) {
				custom_camera.elevation_angle = LEGACY_S16_WRAP_ADD(
					custom_camera.elevation_angle,
					REPLAY_CAMERA_ANGLE_STEP);
				return;
			}
			input = 0;
			break;
		case KEY_DOWN:
			if (LEGACY_S16_WRAP_SUB(custom_camera.elevation_angle,
				REPLAY_CAMERA_ANGLE_STEP) >
				-REPLAY_CUSTOM_CAMERA_ELEVATION_LIMIT) {
				custom_camera.elevation_angle = LEGACY_S16_WRAP_SUB(
					custom_camera.elevation_angle,
					REPLAY_CAMERA_ANGLE_STEP);
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
		if (byte_3E9DB > REPLAY_LAST_ACTION_CONTROL)
			break;
		switch (byte_3E9DB) {
		case REPLAY_CONTROL_FAST_FORWARD:
			replay_fast_forward();
			return;
		case REPLAY_CONTROL_REWIND:
			replay_rewind();
			return;
		case REPLAY_CONTROL_FAST_PLAY:
			replay_controls_select(REPLAY_CONTROL_FAST_PLAY);
			byte_449E6 = REPLAY_PLAYBACK_FAST;
			is_in_replay = 0;
			break;
		case REPLAY_CONTROL_PLAY:
			byte_449E6 = REPLAY_PLAYBACK_NORMAL;
			replay_controls_select(REPLAY_CONTROL_PLAY);
			is_in_replay = 0;
			break;
		case REPLAY_CONTROL_PAUSE:
			is_in_replay = 1;
			audio_carstate();
			replay_controls_select(REPLAY_CONTROL_PAUSE);
			replay_controls_draw(state.game_frame, state.game_frame);
			break;
		case REPLAY_CONTROL_RESTART:
			is_in_replay = 1;
			audio_carstate();
			replay_controls_select(REPLAY_CONTROL_RESTART);
			replay_controls_draw(state.game_frame, state.game_frame);
			restore_gamestate(REPLAY_FIRST_FRAME);
			(void)timer_get_counter_unk(REPLAY_RESTART_WAIT_TICKS);
			replay_controls_select(REPLAY_CONTROL_PAUSE);
			replay_controls_draw(state.game_frame, state.game_frame);
			return;
		case REPLAY_CONTROL_MENU:
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
		if (byte_3E9DB == REPLAY_CONTROL_ZOOM) {
			if (replay_try_zoom('+') != 0)
				return;
			break;
		}
		byte_3E9DB = byte_3E9F0[byte_3E9DB];
		break;

	case KEY_DOWN:
		if (byte_3E9DB == REPLAY_CONTROL_ZOOM) {
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
