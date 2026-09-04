#include "audio_internal.h"
#include "dashboard.h"
#include "fileio.h"
#include "game_input.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "race.h"
#include "race_resources.h"
#include "replay_record.h"
#include "replay_viewer.h"
#include "shape2d.h"
#include "ui_dialog.h"

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
	rect_windshield.right = 320;
	var_2 = -1;
	word_449EA = -1;
	run_game_random = LEGACY_S16_SHL(get_kevinrandom(), 3U);
	replaybar_toggle = 1;
	is_in_replay = 0;
	if (idle_expired == 0) {
		if (gameconfig.game_recordedframes != 0) {
			cameramode = 0;
			game_replay_mode = 2;
			is_in_replay = 1;
		} else {
			cameramode = 0;
			game_replay_mode = 1;
		}
	} else {
		cameramode++;
		if (cameramode == 4) {
			cameramode = 0;
		}

		game_replay_mode = 2;
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
		byte_449E6 = 0;
		byte_449DA = 1;
		set_frame_callback();
		game_replay_mode_copy = -1;
		byte_44346 = 0;
		byte_4432A = 0;
		byte_46467 = 0;
		dashb_toggle = 0;

		if (idle_expired != 0) {
			framespersec = gameconfig.game_framespersec;

			init_game_state(-1);
		} else {
			if (is_in_replay == 0) {
				cameramode = 0;
				dashb_toggle = 1;
				show_penalty_counter = 0;
				init_game_state_with_frame_rate(framespersec2);
				word_45D94 = 0;
				word_45D3E = LEGACY_S16_FROM_BITS(
					LEGACY_U16_REPLACE_LOW_BYTE(word_45D3E, 0U));
				byte_4393C = 1;
				mouse_minmax_position(byte_3B8F2);
				game_replay_mode = 1;

				state.playerstate.car_posWorld1.lx = LEGACY_S32_WRAP_ADD(
					state.playerstate.car_posWorld1.lx,
					LEGACY_S32_SHL((legacy_s32)multiply_and_scale(
						sin_fast(track_angle), -240), 6U));
				state.playerstate.car_posWorld1.lz = LEGACY_S32_WRAP_ADD(
					state.playerstate.car_posWorld1.lz,
					LEGACY_S32_SHL((legacy_s32)multiply_and_scale(
						cos_fast(track_angle), -240), 6U));
				state.playerstate.car_posWorld1.ly = LEGACY_S32_WRAP_ADD(
					state.playerstate.car_posWorld1.ly, 0x580L);
				byte_43966 = 1;
			} else {
				cameramode = 0;
				game_replay_mode = 2;
				word_44DCA = 0x1F4;
				framespersec = gameconfig.game_framespersec;
				restore_gamestate(0);
				restore_gamestate(gameconfig.game_recordedframes);

				while (gameconfig.game_recordedframes != state.game_frame) {
					if (input_do_checking(1) == 27)
						break;
					update_gamestate();
				}

				elapsed_time2 = gameconfig.game_recordedframes;
			}
		}

		while (1) {

			if (state.game_frame != elapsed_time2) {
				if ((byte_3B8F2 != 0 || dos_joystick_is_enabled() != 0) && game_replay_mode == 0) {
					replay_unk();
				}
				update_gamestate();
				continue;
			}


			if (game_replay_mode == 0 && byte_449DA == 0 && state.game_inputmode != 0) {
				if (var_C == state.game_frame)
					continue;
				var_C = state.game_frame;
			}

			if (state.game_inputmode == 0 && game_replay_mode == 0) {
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
				regsi = show_dialog(2, 1, locate_text_res(gameresptr, "rbf"), -1, -1, dialogarg2, 0, 0);
				if (regsi == -1)
					regsi = 0;

				audio_resume();
				dos_timer_set_callbacks_suspended(0);
				input_pop_status();
				if (regsi != 0) {
					update_crash_state(4, 0);
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

				if (game_replay_mode != 2 || idle_expired != 0 || (replaybar_toggle == 0 && is_in_replay == 0)) {
					replaybar_enabled = 0;
				} else {
					replaybar_enabled = 1;
				}

				if (idle_expired != 0) {
					dashbmp_y_copy = 0xC8;
				} else
				if (dashb_toggle == 0 || followOpponentFlag != 0) {
					if (game_replay_mode == 2) {
						if (replaybar_enabled != 0) {
							dashbmp_y_copy = 0x97;
						} else {
							dashbmp_y_copy = 0xC8;
						}
					} else {
						dashbmp_y_copy = 0xC8;
					}
				} else {
					if (game_replay_mode != 2 || replaybar_enabled == 0) {
						height_above_replaybar = 200;
					} else {
						height_above_replaybar = 151;
					}

					byte_449E2 = 1;
					roofbmpheight_copy = roofbmpheight;
					dashbmp_y_copy = dashbmp_y;
				}

				if (var_2 != roofbmpheight_copy || dashbmp_y_copy != word_449EA || var_E != height_above_replaybar) {
					byte_454A4 = video_flag6_is1;
					set_projection(0x23,
						LEGACY_S16_DIV_OR_ZERO(dashbmp_y_copy, 6),
						0x140, dashbmp_y_copy);
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
					sprite_set_1_size(0, 0x140, dashbmp_y_copy, height_above_replaybar);
					setup_car_shapes(1);
				}

				if (replaybar_enabled != 0) {
					sprite_set_1_size(0, 0x140, 0, 0xC8);
					loop_game(1, state.game_frame, state.game_frame);
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
					var_rect.right = 0x140;
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
				sprite_set_1_size(0, 0x140, dashbmp_y_copy, height_above_replaybar);
				setup_car_shapes(2);
				sprite_set_1_size(0, 0x140, 0, 0xC8);
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

			if (game_replay_mode == 1 && byte_4393C == 0) {
				game_replay_mode = 0;
				init_game_state_with_frame_rate(framespersec2);
			}

			if (idle_expired == 0) {
				if (byte_449DA != 0) {

					if ((game_replay_mode != 0 || state.game_3F6autoLoadEvalFlag == 4) && byte_449DA != 2) {
						byte_449DA = 0;
						game_replay_mode = 2;
						mouse_minmax_position(0);
						loop_game(0, 0, 0);
						loop_game(2, 4, 0);
						is_in_replay = 1;
						audio_carstate();
					} else {
						break;
					}
				}

				if (game_replay_mode == 2) {
					loop_game(3, 0, 0);
					continue;
				}

				do {
					var_12 = dos_kb_get_char();
					if (var_12 != 0) {
						handle_ingame_kb_shortcuts(var_12);
					}

				} while (var_12 == 0x4800 || var_12 == 0x4B00 || var_12 == 0x4D00 || var_12 == 0x5000);

				if (game_replay_mode == 1) {
					dos_mouse_get_state(&mouse_butstate, &mouse_xpos, &mouse_ypos);
					if (((mouse_butstate & 3) != 0) || ((get_kb_or_joy_flags() & 0x30) != 0)) {
						game_replay_mode = 0;
						byte_4393C = 0;
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
			sub_35C4E(0, 0, 0x140, 0xC8, 0);
			setup_mcgawnd1();
			mouse_draw_transparent_check();
		}

		sprite_copy_2_to_1_2();
		is_in_replay = 1;
		audio_carstate();
		audio_remove_driver_timer();
		if (game_replay_mode == 0 && gameconfig.game_opponenttype != 0 && state.opponentstate.car_crashBmpFlag == 0) {
			show_dialog(3, 0, locate_text_res(gameresptr, "cop"), -1, 0x50, performGraphColor, var_16, 0);
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

				if (input_do_checking(1) == 27)
					break;
				if (state.opponentstate.car_crashBmpFlag != 0)
					break;
				if (0x5DC * framespersec == state.game_frame + elapsed_time1)
					break;
			}
		}

		word_45D3E = LEGACY_S16_FROM_BITS(
			LEGACY_U16_REPLACE_LOW_BYTE(word_45D3E, 0U));
		mouse_minmax_position(0);
		remove_frame_callback();
		free_player_cars();
	}

	waitflag = 0x64;
	check_input();
	show_waiting();

	return ;
}
