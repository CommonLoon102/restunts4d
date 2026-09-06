#include "audio_internal.h"
#include "dashboard.h"
#include "fileio.h"
#include "game_input.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "race.h"
#include "race_resources.h"
#include "replay.h"
#include "replay_record.h"
#include "replay_viewer.h"
#include "shape2d.h"
#include "ui_dialog.h"

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

legacy_s16 get_0(void);
void do_mer_restext(void);

void run_game(void) {
	legacy_s16 var_16[2];
	legacy_s16 var_12, var_E, var_C;
	struct RECTANGLE var_rect;
	legacy_s16 var_2;
	legacy_s16 regsi;

	var_C = -1;
	rect_windshield.left = 0;
	rect_windshield.right = RACE_SCREEN_WIDTH;
	var_2 = -1;
	word_449EA = -1;
	run_game_random = LEGACY_S16_SHL(get_kevinrandom(),
		RACE_RANDOM_VALUE_SHIFT);
	replaybar_toggle = 1;
	is_in_replay = 0;
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
			return ;
		}
		track_setup();
	}

	if (setup_player_cars() != 0) {
		free_player_cars();
		do_mer_restext();
	} else {

		kbormouse = 0;
		byte_449E6 = REPLAY_PLAYBACK_NORMAL;
		byte_449DA = 1;
		set_frame_callback();
		game_replay_mode_copy = RACE_REPLAY_MODE_UNINITIALIZED;
		byte_44346 = 0;
		byte_4432A = 0;
		byte_46467 = 0;
		dashb_toggle = 0;

		if (idle_expired != 0) {
			framespersec = gameconfig.game_framespersec;

			init_game_state(GAMESTATE_INIT_RESET_CHECKPOINTS);
		} else {
			if (is_in_replay == 0) {
				cameramode = CAMERA_MODE_COCKPIT;
				dashb_toggle = 1;
				show_penalty_counter = 0;
				init_game_state_with_frame_rate(framespersec2);
				word_45D94 = 0;
				word_45D3E = LEGACY_S16_FROM_BITS(
					LEGACY_U16_REPLACE_LOW_BYTE(word_45D3E, 0U));
				byte_4393C = RACE_START_SEQUENCE_FLAG_ANIMATION;
				mouse_minmax_position(byte_3B8F2);
				game_replay_mode = REPLAY_MODE_PAUSED;

				state.playerstate.car_posWorld1.lx = LEGACY_S32_WRAP_ADD(
					state.playerstate.car_posWorld1.lx,
					LEGACY_S32_SHL((legacy_s32)multiply_and_scale(
						sin_fast(track_angle), RACE_START_POSITION_DISTANCE),
						RACE_START_POSITION_SCALE_SHIFT));
				state.playerstate.car_posWorld1.lz = LEGACY_S32_WRAP_ADD(
					state.playerstate.car_posWorld1.lz,
					LEGACY_S32_SHL((legacy_s32)multiply_and_scale(
						cos_fast(track_angle), RACE_START_POSITION_DISTANCE),
						RACE_START_POSITION_SCALE_SHIFT));
				state.playerstate.car_posWorld1.ly = LEGACY_S32_WRAP_ADD(
					state.playerstate.car_posWorld1.ly,
					RACE_START_CAMERA_HEIGHT_OFFSET);
				byte_43966 = REPLAY_RECORDING_ACTIVE_FLAG;
			} else {
				cameramode = CAMERA_MODE_COCKPIT;
				game_replay_mode = REPLAY_MODE_PLAYBACK;
				word_44DCA = RACE_REPLAY_RESTORE_WAIT_TICKS;
				framespersec = gameconfig.game_framespersec;
				restore_gamestate(0);
				restore_gamestate(gameconfig.game_recordedframes);

				while (gameconfig.game_recordedframes != state.game_frame) {
					if (input_do_checking(1) == KEY_ESCAPE)
						break;
					update_gamestate();
				}

				elapsed_time2 = gameconfig.game_recordedframes;
			}
		}

		while (1) {

			if (state.game_frame != elapsed_time2) {
				if ((byte_3B8F2 != 0 || dos_joystick_is_enabled() != 0) &&
					game_replay_mode == REPLAY_MODE_LIVE) {
					replay_unk();
				}
				update_gamestate();
				continue;
			}


			if (game_replay_mode == REPLAY_MODE_LIVE && byte_449DA == 0 &&
				state.game_inputmode != GAME_INPUT_MODE_WAITING) {
				if (var_C == state.game_frame)
					continue;
				var_C = state.game_frame;
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

			if (byte_46467 != 0) {
				input_push_status();
				audio_suspend();
				regsi = show_dialog(DIALOG_TYPE_MENU,
					DIALOG_SAVE_BACKGROUND,
					locate_text_res(gameresptr, "rbf"), -1, -1,
					dialogarg2, 0, 0);
				if (regsi == -1)
					regsi = 0;

				audio_resume();
				dos_timer_set_callbacks_suspended(0);
				input_pop_status();
				if (regsi != 0) {
					update_crash_state(
						CRASH_EVENT_EXIT, PLAYER_CAR_INDEX);
					byte_449DA = 1;
				}

				byte_46467 = 0;
			}

			if (video_flag5_is0 != 0) {
				setup_mcgawnd2();
				byte_4432A = byte_44346;
			} else {
				sprite_copy_wnd_to_1();
			}

			if (game_replay_mode != game_replay_mode_copy || dashb_toggle != dashb_toggle_copy || replaybar_toggle != replaybar_toggle_copy || is_in_replay != is_in_replay_copy || followOpponentFlag != followOpponentFlag_copy) {
				game_replay_mode_copy = game_replay_mode;
				dashb_toggle_copy = dashb_toggle;
				replaybar_toggle_copy = replaybar_toggle;
				is_in_replay_copy = is_in_replay;
				followOpponentFlag_copy = followOpponentFlag;
				roofbmpheight_copy = 0;
				byte_449E2 = 0;

				if (game_replay_mode != REPLAY_MODE_PLAYBACK ||
					idle_expired != 0 ||
					(replaybar_toggle == 0 && is_in_replay == 0)) {
					replaybar_enabled = 0;
				} else {
					replaybar_enabled = 1;
				}

				if (idle_expired != 0) {
					dashbmp_y_copy = RACE_SCREEN_HEIGHT;
				} else
				if (dashb_toggle == 0 || followOpponentFlag != 0) {
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
					if (game_replay_mode != REPLAY_MODE_PLAYBACK ||
						replaybar_enabled == 0) {
						height_above_replaybar = RACE_SCREEN_HEIGHT;
					} else {
						height_above_replaybar = RACE_REPLAY_BAR_TOP;
					}

					byte_449E2 = 1;
					roofbmpheight_copy = roofbmpheight;
					dashbmp_y_copy = dashbmp_y;
				}

				if (var_2 != roofbmpheight_copy || dashbmp_y_copy != word_449EA || var_E != height_above_replaybar) {
					byte_454A4 = video_flag6_is1;
					set_projection(RACE_PROJECTION_HORIZONTAL_SCALE,
						LEGACY_S16_DIV_OR_ZERO(dashbmp_y_copy,
							RACE_PROJECTION_VERTICAL_DIVISOR),
						RACE_SCREEN_WIDTH, dashbmp_y_copy);
					rect_windshield.top = roofbmpheight_copy;
					rect_windshield.bottom = dashbmp_y_copy;
					var_2 = roofbmpheight_copy;
					word_449EA = dashbmp_y_copy;
					var_E = height_above_replaybar;
				}
			}

			if (byte_454A4 != 0) {
				byte_449D8[byte_4432A] = 0;
				if (byte_449E2 != 0) {
					sprite_set_1_size(0, RACE_SCREEN_WIDTH, dashbmp_y_copy,
						height_above_replaybar);
					setup_car_shapes(DASHBOARD_OPERATION_REDRAW_STATIC);
				}

				if (replaybar_enabled != 0) {
					sprite_set_1_size(0, RACE_SCREEN_WIDTH, 0,
						RACE_SCREEN_HEIGHT);
					loop_game(REPLAY_LOOP_DRAW_CONTROLS,
						state.game_frame, state.game_frame);
				}
			} else {
				if (replaybar_enabled == 0) {
					byte_449D8[byte_4432A] = 0;
				}
			}

			update_frame(byte_44346, &rect_windshield);
			if (dastbmp_y != 0 && byte_449E2 != 0) {
				if (slow_video_mgmt_copy != 0) {
					var_rect.left = 0;
					var_rect.right = RACE_SCREEN_WIDTH;
					var_rect.top = dastbmp_y;
					var_rect.bottom = dashbmp_y_copy;
					if (rectptr_unk != 0) {
						rect_union(rectptr_unk, &var_rect, rectptr_unk);
					}
				}

				shape2d_render_bmp_as_mask(dasmshapeptr);
				shape2d_op_unk4(dastbmp_y2, dastseg);
			}

			sub_19F14(&rect_windshield);
			if (byte_449E2 != 0) {
				sprite_set_1_size(0, RACE_SCREEN_WIDTH, dashbmp_y_copy,
					height_above_replaybar);
				setup_car_shapes(DASHBOARD_OPERATION_UPDATE);
				sprite_set_1_size(0, RACE_SCREEN_WIDTH, 0,
					RACE_SCREEN_HEIGHT);
			}

			if (byte_454A4 != 0) {
				byte_454A4--;
			}

			if (video_flag5_is0 != 0) {
				mouse_draw_opaque_check();
				setup_mcgawnd1();
				byte_44346 ^= 1;
				byte_4432A = byte_44346;
				mouse_draw_transparent_check();
			}

			if (game_replay_mode == REPLAY_MODE_PAUSED &&
				byte_4393C == RACE_START_SEQUENCE_INACTIVE) {
				game_replay_mode = REPLAY_MODE_LIVE;
				init_game_state_with_frame_rate(framespersec2);
			}

			if (idle_expired == 0) {
				if (byte_449DA != 0) {

					if ((game_replay_mode != REPLAY_MODE_LIVE ||
						state.game_3F6autoLoadEvalFlag == CRASH_EVENT_EXIT) &&
						byte_449DA != REPLAY_EXIT_REQUESTED) {
						byte_449DA = 0;
						game_replay_mode = REPLAY_MODE_PLAYBACK;
						mouse_minmax_position(0);
						loop_game(REPLAY_LOOP_LOAD_RESOURCES,
							REPLAY_LOOP_UNUSED_ARGUMENT,
							REPLAY_LOOP_UNUSED_ARGUMENT);
						loop_game(REPLAY_LOOP_SELECT_CONTROL,
							REPLAY_CONTROL_PAUSE,
							REPLAY_LOOP_UNUSED_ARGUMENT);
						is_in_replay = 1;
						audio_carstate();
					} else {
						break;
					}
				}

				if (game_replay_mode == REPLAY_MODE_PLAYBACK) {
					loop_game(REPLAY_LOOP_HANDLE_INPUT,
						REPLAY_LOOP_UNUSED_ARGUMENT,
						REPLAY_LOOP_UNUSED_ARGUMENT);
					continue;
				}

				do {
					var_12 = dos_kb_get_char();
					if (var_12 != 0) {
						handle_ingame_kb_shortcuts(var_12);
					}

				} while (var_12 == KEY_UP || var_12 == KEY_LEFT ||
					var_12 == KEY_RIGHT || var_12 == KEY_DOWN);

				if (game_replay_mode == REPLAY_MODE_PAUSED) {
					dos_mouse_get_state(&mouse_butstate, &mouse_xpos, &mouse_ypos);
					if (((mouse_butstate & MOUSE_BUTTON_MASK) != 0) ||
						((get_kb_or_joy_flags() &
							INPUT_ACTION_BUTTON_MASK) != 0)) {
						game_replay_mode = REPLAY_MODE_LIVE;
						byte_4393C = RACE_START_SEQUENCE_INACTIVE;
						init_game_state_with_frame_rate(framespersec2);
					}
				}

			} else {
				if (dos_kb_get_char() != 0 || byte_449DA != 0 || get_kb_or_joy_flags() != 0) {
					break;
				}
			}
		}

		if (video_flag5_is0 != 0 && get_0() != 0) {
			mouse_draw_opaque_check();
			setup_mcgawnd2();
			sub_35C4E(0, 0, RACE_SCREEN_WIDTH, RACE_SCREEN_HEIGHT, 0);
			setup_mcgawnd1();
			mouse_draw_transparent_check();
		}

		sprite_copy_2_to_1_2();
		is_in_replay = 1;
		audio_carstate();
		audio_remove_driver_timer();
		if (game_replay_mode == REPLAY_MODE_LIVE &&
			gameconfig.game_opponenttype != 0 &&
			state.opponentstate.car_crashBmpFlag == CRASH_EVENT_NONE) {
			show_dialog(DIALOG_TYPE_PLACEHOLDERS,
				DIALOG_NO_BACKGROUND_SAVE,
				locate_text_res(gameresptr, "cop"), -1,
				RACE_OPPONENT_PROGRESS_DIALOG_Y, performGraphColor,
				var_16, 0);
			word_45D3E = LEGACY_S16_FROM_BITS(
				LEGACY_U16_REPLACE_LOW_BYTE(word_45D3E, 1U));
			regsi = framespersec;
			regsi--;

			while (1) {
				replay_unk2(1);
				update_gamestate();
				regsi++;
				if (regsi == framespersec) {
					regsi = 0;
					format_frame_as_string(&resID_byte1, state.game_frame + elapsed_time1, 1);
					mouse_draw_opaque_check();
					sub_345BC(&resID_byte1, font_op2_alt(&resID_byte1), var_16[1]);
					mouse_draw_transparent_check();
				}

				if (input_do_checking(1) == KEY_ESCAPE)
					break;
				if (state.opponentstate.car_crashBmpFlag != CRASH_EVENT_NONE)
					break;
				if (RACE_FRAME_LIMIT_MULTIPLIER * framespersec ==
					state.game_frame + elapsed_time1)
					break;
			}
		}

		word_45D3E = LEGACY_S16_FROM_BITS(
			LEGACY_U16_REPLACE_LOW_BYTE(word_45D3E, 0U));
		mouse_minmax_position(0);
		remove_frame_callback();
		free_player_cars();
	}

	waitflag = RACE_FINAL_WAIT_TICKS;
	check_input();
	show_waiting();

	return ;
}
