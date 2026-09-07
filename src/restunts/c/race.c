#include "dashboard.h"
#include "fileio.h"
#include "game_input.h"
#include "memmgr.h"
#include "platform.h"
#include "race.h"
#include "race_resources.h"
#include "replay.h"
#include "replay_record.h"
#include "replay_viewer.h"
#include "shape2d.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "shape3d.h"
#include "crash_state.h"
#include "track_objects.h"
#include "camera.h"
#include "video_frame.h"
#include "car_audio.h"
#include "audio_control.h"
#include "externs.h"
#include "keyboard.h"

#define RACE_SCREEN_WIDTH 320
#define RACE_SCREEN_HEIGHT 200
#define RACE_REPLAY_BAR_TOP 151
#define RACE_START_CAMERA_HEIGHT_OFFSET 1408L
#define RACE_REPLAY_RESTORE_WAIT_TICKS 500
#define RACE_PROJECTION_HORIZONTAL_SCALE 35
#define RACE_PROJECTION_VERTICAL_DIVISOR 6
#define RACE_OPPONENT_PROGRESS_DIALOG_Y 80
#define RACE_FRAME_LIMIT_MULTIPLIER 1500
#define RACE_FINAL_WAIT_TICKS 100
#define MOUSE_BUTTON_MASK 3
#define RACE_REPLAY_MODE_UNINITIALIZED (-1)
#define RACE_START_POSITION_DISTANCE (-240)
#define RACE_START_POSITION_SCALE_SHIFT 6U
#define RACE_RANDOM_VALUE_SHIFT 3U

struct RACE_VIEWPORT_CACHE {
	legacy_s16 roof_height;
	legacy_s16 dashboard_bottom;
};

static legacy_u16 race_prepare_mode(void)
{
	if (idle_expired == 0) {
		if (gameconfig.game_recordedframes != 0) {
			cameramode = CAMERA_MODE_COCKPIT;
			game_replay_mode = REPLAY_MODE_PLAYBACK;
			is_in_replay = 1;
		} else {
			cameramode = CAMERA_MODE_COCKPIT;
			game_replay_mode = REPLAY_MODE_PAUSED;
		}
	} else {
		cameramode++;
		if (cameramode == CAMERA_MODE_COUNT) {
			cameramode = CAMERA_MODE_COCKPIT;
		}

		game_replay_mode = REPLAY_MODE_PLAYBACK;
		if (file_load_replay(0, "default") != 0) {
			return 0;
		}
		track_setup();
	}

	return 1;
}

static void race_initialize_state(void)
{
	kbormouse = 0;
	replay_playback_speed = REPLAY_PLAYBACK_NORMAL;
	race_exit_request = 1;
	set_frame_callback();
	game_replay_mode_copy = RACE_REPLAY_MODE_UNINITIALIZED;
	frame_buffer_index = 0;
	dashboard_buffer_index = 0;
	recording_limit_warning_requested = 0;
	dashb_toggle = 0;

	if (idle_expired != 0) {
		framespersec = gameconfig.game_framespersec;

		init_game_state(GAMESTATE_INIT_RESET_CHECKPOINTS);
	} else {
		if (is_in_replay == 0) {
			cameramode = CAMERA_MODE_COCKPIT;
			dashb_toggle = 1;
			show_penalty_counter = 0;
			init_game_state_with_frame_rate(configured_frame_rate);
			reserved_race_word = 0;
			replay_overflow_acknowledged_word = LEGACY_S16_FROM_BITS(
				LEGACY_U16_REPLACE_LOW_BYTE(replay_overflow_acknowledged_word, 0U));
			race_start_sequence_state = RACE_START_SEQUENCE_FLAG_ANIMATION;
			mouse_minmax_position(mouse_driving_enabled);
			game_replay_mode = REPLAY_MODE_PAUSED;

			state.playerstate.car_position.lx = LEGACY_S32_WRAP_ADD(
				state.playerstate.car_position.lx,
				LEGACY_S32_SHL((legacy_s32)multiply_and_scale(sin_fast(track_angle),
															  RACE_START_POSITION_DISTANCE),
							   RACE_START_POSITION_SCALE_SHIFT));
			state.playerstate.car_position.lz = LEGACY_S32_WRAP_ADD(
				state.playerstate.car_position.lz,
				LEGACY_S32_SHL((legacy_s32)multiply_and_scale(cos_fast(track_angle),
															  RACE_START_POSITION_DISTANCE),
							   RACE_START_POSITION_SCALE_SHIFT));
			state.playerstate.car_position.ly = LEGACY_S32_WRAP_ADD(
				state.playerstate.car_position.ly, RACE_START_CAMERA_HEIGHT_OFFSET);
			replay_recording_flags = REPLAY_RECORDING_ACTIVE_FLAG;
		} else {
			cameramode = CAMERA_MODE_COCKPIT;
			game_replay_mode = REPLAY_MODE_PLAYBACK;
			start_flag_animation = RACE_REPLAY_RESTORE_WAIT_TICKS;
			framespersec = gameconfig.game_framespersec;
			restore_gamestate(0);
			restore_gamestate(gameconfig.game_recordedframes);

			while (gameconfig.game_recordedframes != state.game_frame) {
				if (input_do_checking(1) == KEY_ESCAPE) {
					break;
				}
				update_gamestate();
			}

			elapsed_time2 = gameconfig.game_recordedframes;
		}
	}
}

static void race_check_recording_limit(void)
{
	legacy_s16 dialog_choice;

	if (recording_limit_warning_requested != 0) {
		input_push_status();
		audio_suspend();
		dialog_choice =
			show_dialog(DIALOG_TYPE_MENU, DIALOG_SAVE_BACKGROUND,
						locate_text_res(gameresptr, "rbf"), -1, -1, dialog_border_color, 0, 0);
		if (dialog_choice == -1) {
			dialog_choice = 0;
		}

		audio_resume();
		dos_timer_set_callbacks_suspended(0);
		input_pop_status();
		if (dialog_choice != 0) {
			update_crash_state(CRASH_EVENT_EXIT, PLAYER_CAR_INDEX);
			race_exit_request = 1;
		}

		recording_limit_warning_requested = 0;
	}
}

static void race_update_dashboard_layout(void)
{
	game_replay_mode_copy = game_replay_mode;
	dashb_toggle_copy = dashb_toggle;
	replaybar_toggle_copy = replaybar_toggle;
	is_in_replay_copy = is_in_replay;
	followOpponentFlag_copy = followOpponentFlag;
	roofbmpheight_copy = 0;
	dashboard_visible = 0;

	if (game_replay_mode != REPLAY_MODE_PLAYBACK || idle_expired != 0 ||
		(replaybar_toggle == 0 && is_in_replay == 0)) {
		replaybar_enabled = 0;
	} else {
		replaybar_enabled = 1;
	}

	if (idle_expired != 0) {
		dashbmp_y_copy = RACE_SCREEN_HEIGHT;
	} else if (dashb_toggle == 0 || followOpponentFlag != 0) {
		if (game_replay_mode == REPLAY_MODE_PLAYBACK) {
			if (replaybar_enabled != 0) {
				dashbmp_y_copy = RACE_REPLAY_BAR_TOP;
			} else {
				dashbmp_y_copy = RACE_SCREEN_HEIGHT;
			}
		} else {
			dashbmp_y_copy = RACE_SCREEN_HEIGHT;
		}
	} else {
		if (game_replay_mode != REPLAY_MODE_PLAYBACK || replaybar_enabled == 0) {
			height_above_replaybar = RACE_SCREEN_HEIGHT;
		} else {
			height_above_replaybar = RACE_REPLAY_BAR_TOP;
		}

		dashboard_visible = 1;
		roofbmpheight_copy = roofbmpheight;
		dashbmp_y_copy = dashbmp_y;
	}
}

static void race_update_viewport(struct RACE_VIEWPORT_CACHE *cache)
{
	if (game_replay_mode != game_replay_mode_copy || dashb_toggle != dashb_toggle_copy ||
		replaybar_toggle != replaybar_toggle_copy || is_in_replay != is_in_replay_copy ||
		followOpponentFlag != followOpponentFlag_copy) {
		race_update_dashboard_layout();
		if (cache->roof_height != roofbmpheight_copy || dashbmp_y_copy != viewport_bottom_cache ||
			cache->dashboard_bottom != height_above_replaybar) {
			full_redraw_frames_remaining = video_page_count;
			set_projection(RACE_PROJECTION_HORIZONTAL_SCALE,
						   LEGACY_S16_DIV_OR_ZERO(dashbmp_y_copy, RACE_PROJECTION_VERTICAL_DIVISOR),
						   RACE_SCREEN_WIDTH, dashbmp_y_copy);
			rect_windshield.top = roofbmpheight_copy;
			rect_windshield.bottom = dashbmp_y_copy;
			cache->roof_height = roofbmpheight_copy;
			viewport_bottom_cache = dashbmp_y_copy;
			cache->dashboard_bottom = height_above_replaybar;
		}
	}
}

static void race_draw_frame(void)
{
	struct RECTANGLE dashboard_mask_rect;

	if (full_redraw_frames_remaining != 0) {
		replay_controls_drawn[dashboard_buffer_index] = 0;
		if (dashboard_visible != 0) {
			sprite_set_target_clip_bounds(0, RACE_SCREEN_WIDTH, dashbmp_y_copy,
										  height_above_replaybar);
			setup_car_shapes(DASHBOARD_OPERATION_REDRAW_STATIC);
		}

		if (replaybar_enabled != 0) {
			sprite_set_target_clip_bounds(0, RACE_SCREEN_WIDTH, 0, RACE_SCREEN_HEIGHT);
			loop_game(REPLAY_LOOP_DRAW_CONTROLS, state.game_frame, state.game_frame);
		}
	} else {
		if (replaybar_enabled == 0) {
			replay_controls_drawn[dashboard_buffer_index] = 0;
		}
	}

	update_frame(frame_buffer_index, &rect_windshield);
	if (dastbmp_y != 0 && dashboard_visible != 0) {
		if (slow_video_mgmt_copy != 0) {
			dashboard_mask_rect.left = 0;
			dashboard_mask_rect.right = RACE_SCREEN_WIDTH;
			dashboard_mask_rect.top = dastbmp_y;
			dashboard_mask_rect.bottom = dashbmp_y_copy;
			if (active_frame_rects != 0) {
				rect_union(active_frame_rects, &dashboard_mask_rect, active_frame_rects);
			}
		}

		shape2d_render_bmp_as_mask(dasmshapeptr);
		shape2d_rle_or_far_pointer(dastbmp_y2, dastseg);
	}

	frame_present(&rect_windshield);
	if (dashboard_visible != 0) {
		sprite_set_target_clip_bounds(0, RACE_SCREEN_WIDTH, dashbmp_y_copy, height_above_replaybar);
		setup_car_shapes(DASHBOARD_OPERATION_UPDATE);
		sprite_set_target_clip_bounds(0, RACE_SCREEN_WIDTH, 0, RACE_SCREEN_HEIGHT);
	}

	if (full_redraw_frames_remaining != 0) {
		full_redraw_frames_remaining--;
	}

	if (video_uses_page_flipping != 0) {
		mouse_draw_opaque_check();
		sprite_present_mcga_backbuffer();
		frame_buffer_index ^= 1;
		dashboard_buffer_index = frame_buffer_index;
		mouse_draw_transparent_check();
	}
}

static void race_handle_driving_input(void)
{
	legacy_s16 input_key;

	do {
		input_key = dos_kb_get_char();
		if (input_key != 0) {
			handle_ingame_kb_shortcuts(input_key);
		}

	} while (input_key == KEY_UP || input_key == KEY_LEFT || input_key == KEY_RIGHT ||
			 input_key == KEY_DOWN);

	if (game_replay_mode == REPLAY_MODE_PAUSED) {
		dos_mouse_get_state(&mouse_butstate, &mouse_xpos, &mouse_ypos);
		if (((mouse_butstate & MOUSE_BUTTON_MASK) != 0) ||
			((get_kb_or_joy_flags() & INPUT_ACTION_BUTTON_MASK) != 0)) {
			game_replay_mode = REPLAY_MODE_LIVE;
			race_start_sequence_state = RACE_START_SEQUENCE_INACTIVE;
			init_game_state_with_frame_rate(configured_frame_rate);
		}
	}
}

static legacy_u16 race_handle_exit_request(void)
{
	if (race_exit_request != 0) {

		if ((game_replay_mode != REPLAY_MODE_LIVE || state.game_end_event == CRASH_EVENT_EXIT) &&
			race_exit_request != REPLAY_EXIT_REQUESTED) {
			race_exit_request = 0;
			game_replay_mode = REPLAY_MODE_PLAYBACK;
			mouse_minmax_position(0);
			loop_game(REPLAY_LOOP_LOAD_RESOURCES, REPLAY_LOOP_UNUSED_ARGUMENT,
					  REPLAY_LOOP_UNUSED_ARGUMENT);
			loop_game(REPLAY_LOOP_SELECT_CONTROL, REPLAY_CONTROL_PAUSE,
					  REPLAY_LOOP_UNUSED_ARGUMENT);
			is_in_replay = 1;
			audio_carstate();
		} else {
			return 1;
		}
	}
	return 0;
}

static legacy_u16 race_frame_is_ready(legacy_s16 *last_processed_frame)
{
	if (state.game_frame != elapsed_time2) {
		if ((mouse_driving_enabled != 0 || dos_joystick_is_enabled() != 0) &&
			game_replay_mode == REPLAY_MODE_LIVE) {
			replay_apply_analog_steering_history();
		}
		update_gamestate();
		return 0;
	}

	if (game_replay_mode == REPLAY_MODE_LIVE && race_exit_request == 0 &&
		state.game_inputmode != GAME_INPUT_MODE_WAITING) {
		if (*last_processed_frame == state.game_frame) {
			return 0;
		}
		*last_processed_frame = state.game_frame;
	}

	return 1;
}

static legacy_s16 race_process_frame_input(void)
{
	if (idle_expired == 0) {
		if (race_handle_exit_request() != 0) {
			return 1;
		}

		if (game_replay_mode == REPLAY_MODE_PLAYBACK) {
			loop_game(REPLAY_LOOP_HANDLE_INPUT, REPLAY_LOOP_UNUSED_ARGUMENT,
					  REPLAY_LOOP_UNUSED_ARGUMENT);
			return 0;
		}

		race_handle_driving_input();

	} else {
		if (dos_kb_get_char() != 0 || race_exit_request != 0 || get_kb_or_joy_flags() != 0) {
			return 1;
		}
	}
	return 0;
}

static void race_run_frames(struct RACE_VIEWPORT_CACHE *cache)
{
	legacy_s16 last_processed_frame = -1;

	while (1) {

		if (race_frame_is_ready(&last_processed_frame) == 0) {
			continue;
		}
		if (state.game_inputmode == GAME_INPUT_MODE_WAITING &&
			game_replay_mode == REPLAY_MODE_LIVE) {
			elapsed_time2 = 0;
			gameconfig.game_recordedframes = 0;
			state.game_frame = 0;
		}

		if (slow_video_mgmt_copy != slow_video_mgmt) {
			slow_video_mgmt_copy = slow_video_mgmt;
			init_rect_arrays();
		}

		race_check_recording_limit();
		if (video_uses_page_flipping != 0) {
			sprite_select_mcga_backbuffer();
			dashboard_buffer_index = frame_buffer_index;
		} else {
			sprite_select_render_window();
		}

		race_update_viewport(cache);
		race_draw_frame();
		if (game_replay_mode == REPLAY_MODE_PAUSED &&
			race_start_sequence_state == RACE_START_SEQUENCE_INACTIVE) {
			game_replay_mode = REPLAY_MODE_LIVE;
			init_game_state_with_frame_rate(configured_frame_rate);
		}

		if (race_process_frame_input() != 0) {
			break;
		}
	}
}

static void race_finish_opponent(void)
{
	legacy_s16 opponent_progress_text_position[2];
	legacy_s16 frame_counter;

	if (game_replay_mode == REPLAY_MODE_LIVE && gameconfig.game_opponenttype != 0 &&
		state.opponentstate.car_crashBmpFlag == CRASH_EVENT_NONE) {
		show_dialog(DIALOG_TYPE_PLACEHOLDERS, DIALOG_NO_BACKGROUND_SAVE,
					locate_text_res(gameresptr, "cop"), -1, RACE_OPPONENT_PROGRESS_DIALOG_Y,
					performGraphColor, opponent_progress_text_position, 0);
		replay_overflow_acknowledged_word = LEGACY_S16_FROM_BITS(
			LEGACY_U16_REPLACE_LOW_BYTE(replay_overflow_acknowledged_word, 1U));
		frame_counter = framespersec;
		frame_counter--;

		while (1) {
			replay_update_input_tick(1);
			update_gamestate();
			frame_counter++;
			if (frame_counter == framespersec) {
				frame_counter = 0;
				format_frame_as_string(&resID_byte1, state.game_frame + elapsed_time1, 1);
				mouse_draw_opaque_check();
				font_draw_text_opaque(&resID_byte1, font_centered_text_x(&resID_byte1),
									  opponent_progress_text_position[1]);
				mouse_draw_transparent_check();
			}

			if (input_do_checking(1) == KEY_ESCAPE) {
				break;
			}
			if (state.opponentstate.car_crashBmpFlag != CRASH_EVENT_NONE) {
				break;
			}
			if (RACE_FRAME_LIMIT_MULTIPLIER * framespersec == state.game_frame + elapsed_time1) {
				break;
			}
		}
	}
}

static void race_release_resources(void)
{
	if (video_uses_page_flipping != 0 && video_backbuffer_copy_required() != 0) {
		mouse_draw_opaque_check();
		sprite_select_mcga_backbuffer();
		sprite_copy_rect_shifted(0, 0, RACE_SCREEN_WIDTH, RACE_SCREEN_HEIGHT, 0);
		sprite_present_mcga_backbuffer();
		mouse_draw_transparent_check();
	}

	sprite_select_screen_compat();
	is_in_replay = 1;
	audio_carstate();
	audio_remove_driver_timer();
	race_finish_opponent();

	replay_overflow_acknowledged_word =
		LEGACY_S16_FROM_BITS(LEGACY_U16_REPLACE_LOW_BYTE(replay_overflow_acknowledged_word, 0U));
	mouse_minmax_position(0);
	remove_frame_callback();
	free_player_cars();
}

void run_game(void)
{
	struct RACE_VIEWPORT_CACHE cache;

	rect_windshield.left = 0;
	rect_windshield.right = RACE_SCREEN_WIDTH;
	cache.roof_height = -1;
	cache.dashboard_bottom = -1;
	viewport_bottom_cache = -1;
	run_game_random = LEGACY_S16_SHL(get_kevinrandom(), RACE_RANDOM_VALUE_SHIFT);
	replaybar_toggle = 1;
	is_in_replay = 0;
	if (race_prepare_mode() == 0) {
		return;
	}
	if (setup_player_cars() != 0) {
		free_player_cars();
		show_insufficient_memory_dialog();
	} else {
		race_initialize_state();
		race_run_frames(&cache);
		race_release_resources();
	}
	waitflag = RACE_FINAL_WAIT_TICKS;
	check_input();
	show_waiting();
}
