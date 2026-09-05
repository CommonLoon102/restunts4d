#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

void run_tracks_menu(legacy_s16 reload_track)
{
	legacy_s8 far* text_resource;
	legacy_s8 far* prompt;
	struct HIGHSCORE_ENTRY far* scores;
	legacy_u8 text_offsets[4];
	legacy_u8 selected;
	legacy_u8 previous;
	legacy_u8 blit_mode;
	legacy_u16 elapsed;
	legacy_u16 key;
	legacy_u16 score;
	legacy_s16 hit;
	legacy_s8 chosen;
	legacy_s16 needs_track_setup;

	ensure_file_exists(3);
	needs_track_setup = reload_track != 0;
	for (;;) {
		if (needs_track_setup != 0) {
			check_input();
			show_waiting();
			waitflag = 0x82;
			track_setup();
			load_tracks_menu_shapes();
			needs_track_setup = 0;
		}

		selected = 0;
		previous = 0xFFU;
		blit_mode = 0xFFU;
		show_waiting();
		waitflag = 0x9B;
		render_window_sprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);
		load_skybox((legacy_s8)td14_elem_map_main[0x384]);
		shape3d_load_all();
		set_projection(0x28, 0x28, 0x140, 0xC8);
		init_game_state(-2);
		sprite_copy_wnd_to_1();
		sprite_clear_1_color((legacy_u8)skybox.ground_color);
		sprite_set_1_size(0, 0x140, 0, 0xC8);
		draw_track_preview();
		shape3d_free_all();
		unload_skybox();

		sprite_copy_wnd_to_1();
		strcpy(&resID_byte1, "'");
		strcat(&resID_byte1, gameconfig.game_trackname);
		strcat(&resID_byte1, "'");
		intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1), 6,
			dialog_fnt_colour, 0);
		if (highscore_write_a(0) == 0) {
			scores = (struct HIGHSCORE_ENTRY far*)td11_highscores;
			score = scores[ranking_entry_order[0]].time;
			if (score != 0xFFFFU) {
				copy_string(&resID_byte1,
					locate_text_res(mainresptr, "hs0"));
				intro_draw_text(&resID_byte1,
					font_op2_alt(&resID_byte1), 0x12,
					dialog_fnt_colour, 0);
				font_set_fontdef2(fontnptr);
				print_highscore_entry(0, text_offsets);
				font_set_unk(0, 0);
				font_draw_text(&resID_byte1 + text_offsets[0],
					0x10, 0x1E);
				font_draw_text(&resID_byte1 + text_offsets[1],
					0x78, 0x1E);
				font_draw_text(&resID_byte1 + text_offsets[2],
					0xE0, 0x1E);
				font_draw_text(&resID_byte1 + text_offsets[3],
					0x110, 0x1E);
				font_set_fontdef();
			}
		}

		text_resource = (legacy_s8 far*)file_load_resfile("tedit");
		draw_button(locate_text_res(text_resource, "bmt"),
			0x11, 0xAC, 0x5E, 0x18, word_407F4, word_407F6,
			word_407F8, 0);
		draw_button(locate_text_res(text_resource, "bet"),
			0x71, 0xAC, 0x5E, 0x18, word_407F4, word_407F6,
			word_407F8, 0);
		draw_button(locate_text_res(text_resource, "bmm"),
			0xD1, 0xAC, 0x5E, 0x18, word_407F4, word_407F6,
			word_407F8, 0);
		unload_resource(text_resource);

		for (;;) {
			if (selected != previous) {
				previous = selected;
				sprite_blit_to_video(render_window_sprite,
					LEGACY_S8_FROM_BITS(blit_mode));
				blit_mode = 0xFEU;
				sprite_copy_2_to_1_2();
				sub_29772();
			}

			elapsed = (legacy_u16)mouse_timer_sprite_unk(selected,
				trackmenu_buttons, word_407CE, word_407D0);
			menu_update_idle_counter(elapsed, 0x1770);
			key = (legacy_u16)input_checking(
				LEGACY_S16_FROM_BITS(elapsed));
			hit = (legacy_s16)mouse_multi_hittest(3, trackmenu_buttons);
			if (hit != -1)
				selected = (legacy_u8)hit;
			if (idle_expired != 0) {
				selected = 2;
				key = 0x0DU;
			}

			if (key == 0)
				continue;
			if (key == 0x4B00U) {
				selected = selected == 0 ? 2U :
					(legacy_u8)(selected - 1U);
				continue;
			}
			if (key == 0x4D00U) {
				selected = selected >= 2U ? 0U :
					(legacy_u8)(selected + 1U);
				continue;
			}
			if (key == 0x1BU)
				selected = 0xFFU;
			else if (key != 0x0DU && key != 0x20U)
				continue;

			if (selected == 0) {
				prompt = locate_text_res(mainresptr, "trk");
				chosen = do_fileselect_dialog(byte_3B80C,
					gameconfig.game_trackname, ".trk", prompt);
				file_build_path(byte_3B80C,
					gameconfig.game_trackname, ".trk", g_path_buf);
				if (chosen != 0) {
					file_read_fatal(g_path_buf, td14_elem_map_main);
					sprite_free_wnd(render_window_sprite);
					break;
				}
				previous = 0xFFU;
				continue;
			}

			sprite_free_wnd(render_window_sprite);
			if (selected == 1)
				needs_track_setup = 1;
			else
				return;
			break;
		}
	}
}
