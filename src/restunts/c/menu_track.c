#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

#define TRACK_EDITOR_RESOURCE_FILE_INDEX 3
#define TRACK_MENU_BUTTON_COUNT 3
#define TRACK_MENU_LOAD_BUTTON 0U
#define TRACK_MENU_EDIT_BUTTON 1U
#define TRACK_MENU_EXIT_BUTTON 2U
#define TRACK_MENU_NO_SELECTION 255U
#define TRACK_MENU_INITIAL_BLIT_MODE 255U
#define TRACK_MENU_REFRESH_BLIT_MODE 254U
#define TRACK_MENU_SETUP_WAIT_TICKS 130
#define TRACK_MENU_PREVIEW_WAIT_TICKS 155
#define TRACK_MENU_IDLE_LIMIT_TICKS 6000
#define TRACK_MENU_SCREEN_WIDTH 320U
#define TRACK_MENU_SCREEN_HEIGHT 200U
#define TRACK_MENU_TRANSPARENT_COLOR 15U
#define TRACK_SKYBOX_ELEMENT_INDEX 900U
#define TRACK_PREVIEW_PROJECTION_SCALE 40
#define TRACK_PREVIEW_GAME_STATE (-2)
#define TRACK_MENU_NAME_Y 6
#define TRACK_MENU_HIGHSCORE_LABEL_Y 18
#define TRACK_MENU_HIGHSCORE_ENTRY_Y 30
#define TRACK_MENU_HIGHSCORE_FIELD_COUNT 4U
#define TRACK_MENU_HIGHSCORE_NAME_X 16
#define TRACK_MENU_HIGHSCORE_CAR_X 120
#define TRACK_MENU_HIGHSCORE_OPPONENT_X 224
#define TRACK_MENU_HIGHSCORE_TIME_X 272
#define HIGHSCORE_UNSET_TIME 65535U
#define TRACK_MENU_FIRST_BUTTON_X 17
#define TRACK_MENU_BUTTON_SPACING 96
#define TRACK_MENU_BUTTON_Y 172
#define TRACK_MENU_BUTTON_WIDTH 94
#define TRACK_MENU_BUTTON_HEIGHT 24

void run_tracks_menu(legacy_s16 reload_track)
{
	legacy_s8 far* text_resource;
	legacy_s8 far* prompt;
	struct HIGHSCORE_ENTRY far* scores;
	legacy_u8 text_offsets[TRACK_MENU_HIGHSCORE_FIELD_COUNT];
	legacy_u8 selected;
	legacy_u8 previous;
	legacy_u8 blit_mode;
	legacy_u16 elapsed;
	legacy_u16 key;
	legacy_u16 score;
	legacy_s16 hit;
	legacy_s8 chosen;
	legacy_s16 needs_track_setup;

	ensure_file_exists(TRACK_EDITOR_RESOURCE_FILE_INDEX);
	needs_track_setup = reload_track != 0;
	for (;;) {
		if (needs_track_setup != 0) {
			check_input();
			show_waiting();
			waitflag = TRACK_MENU_SETUP_WAIT_TICKS;
			track_setup();
			load_tracks_menu_shapes();
			needs_track_setup = 0;
		}

		selected = 0;
		previous = TRACK_MENU_NO_SELECTION;
		blit_mode = TRACK_MENU_INITIAL_BLIT_MODE;
		show_waiting();
		waitflag = TRACK_MENU_PREVIEW_WAIT_TICKS;
		render_window_sprite = sprite_make_wnd(TRACK_MENU_SCREEN_WIDTH,
			TRACK_MENU_SCREEN_HEIGHT, TRACK_MENU_TRANSPARENT_COLOR);
		load_skybox((legacy_s8)
			td14_elem_map_main[TRACK_SKYBOX_ELEMENT_INDEX]);
		shape3d_load_all();
		set_projection(TRACK_PREVIEW_PROJECTION_SCALE,
			TRACK_PREVIEW_PROJECTION_SCALE, TRACK_MENU_SCREEN_WIDTH,
			TRACK_MENU_SCREEN_HEIGHT);
		init_game_state(TRACK_PREVIEW_GAME_STATE);
		sprite_copy_wnd_to_1();
		sprite_clear_1_color((legacy_u8)skybox.ground_color);
		sprite_set_1_size(0, TRACK_MENU_SCREEN_WIDTH, 0,
			TRACK_MENU_SCREEN_HEIGHT);
		draw_track_preview();
		shape3d_free_all();
		unload_skybox();

		sprite_copy_wnd_to_1();
		strcpy(&resID_byte1, "'");
		strcat(&resID_byte1, gameconfig.game_trackname);
		strcat(&resID_byte1, "'");
		intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1),
			TRACK_MENU_NAME_Y,
			dialog_fnt_colour, 0);
		if (highscore_write_a(0) == 0) {
			scores = (struct HIGHSCORE_ENTRY far*)td11_highscores;
			score = scores[ranking_entry_order[0]].time;
			if (score != HIGHSCORE_UNSET_TIME) {
				copy_string(&resID_byte1,
					locate_text_res(mainresptr, "hs0"));
				intro_draw_text(&resID_byte1,
					font_op2_alt(&resID_byte1),
					TRACK_MENU_HIGHSCORE_LABEL_Y,
					dialog_fnt_colour, 0);
				font_set_fontdef2(fontnptr);
				print_highscore_entry(0, text_offsets);
				font_set_unk(0, 0);
				font_draw_text(&resID_byte1 + text_offsets[0],
					TRACK_MENU_HIGHSCORE_NAME_X,
					TRACK_MENU_HIGHSCORE_ENTRY_Y);
				font_draw_text(&resID_byte1 + text_offsets[1],
					TRACK_MENU_HIGHSCORE_CAR_X,
					TRACK_MENU_HIGHSCORE_ENTRY_Y);
				font_draw_text(&resID_byte1 + text_offsets[2],
					TRACK_MENU_HIGHSCORE_OPPONENT_X,
					TRACK_MENU_HIGHSCORE_ENTRY_Y);
				font_draw_text(&resID_byte1 + text_offsets[3],
					TRACK_MENU_HIGHSCORE_TIME_X,
					TRACK_MENU_HIGHSCORE_ENTRY_Y);
				font_set_fontdef();
			}
		}

		text_resource = (legacy_s8 far*)file_load_resfile("tedit");
		draw_button(locate_text_res(text_resource, "bmt"),
			TRACK_MENU_FIRST_BUTTON_X, TRACK_MENU_BUTTON_Y,
			TRACK_MENU_BUTTON_WIDTH, TRACK_MENU_BUTTON_HEIGHT,
			word_407F4, word_407F6,
			word_407F8, 0);
		draw_button(locate_text_res(text_resource, "bet"),
			TRACK_MENU_FIRST_BUTTON_X + TRACK_MENU_BUTTON_SPACING,
			TRACK_MENU_BUTTON_Y, TRACK_MENU_BUTTON_WIDTH,
			TRACK_MENU_BUTTON_HEIGHT, word_407F4, word_407F6,
			word_407F8, 0);
		draw_button(locate_text_res(text_resource, "bmm"),
			TRACK_MENU_FIRST_BUTTON_X + TRACK_MENU_BUTTON_SPACING * 2,
			TRACK_MENU_BUTTON_Y, TRACK_MENU_BUTTON_WIDTH,
			TRACK_MENU_BUTTON_HEIGHT, word_407F4, word_407F6,
			word_407F8, 0);
		unload_resource(text_resource);

		for (;;) {
			if (selected != previous) {
				previous = selected;
				sprite_blit_to_video(render_window_sprite,
					LEGACY_S8_FROM_BITS(blit_mode));
				blit_mode = TRACK_MENU_REFRESH_BLIT_MODE;
				sprite_copy_2_to_1_2();
				sub_29772();
			}

			elapsed = (legacy_u16)mouse_timer_sprite_unk(selected,
				trackmenu_buttons, word_407CE, word_407D0);
			menu_update_idle_counter(elapsed, TRACK_MENU_IDLE_LIMIT_TICKS);
			key = (legacy_u16)input_checking(
				LEGACY_S16_FROM_BITS(elapsed));
			hit = (legacy_s16)mouse_multi_hittest(TRACK_MENU_BUTTON_COUNT,
				trackmenu_buttons);
			if (hit != -1)
				selected = (legacy_u8)hit;
			if (idle_expired != 0) {
				selected = TRACK_MENU_EXIT_BUTTON;
				key = KEY_ENTER;
			}

			if (key == 0)
				continue;
			if (key == KEY_LEFT) {
				selected = selected == TRACK_MENU_LOAD_BUTTON ?
					TRACK_MENU_EXIT_BUTTON :
					(legacy_u8)(selected - 1U);
				continue;
			}
			if (key == KEY_RIGHT) {
				selected = selected >= TRACK_MENU_EXIT_BUTTON ?
					TRACK_MENU_LOAD_BUTTON :
					(legacy_u8)(selected + 1U);
				continue;
			}
			if (key == KEY_ESCAPE)
				selected = TRACK_MENU_NO_SELECTION;
			else if (key != KEY_ENTER && key != KEY_SPACE)
				continue;

			if (selected == TRACK_MENU_LOAD_BUTTON) {
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
				previous = TRACK_MENU_NO_SELECTION;
				continue;
			}

			sprite_free_wnd(render_window_sprite);
			if (selected == TRACK_MENU_EDIT_BUTTON)
				needs_track_setup = 1;
			else
				return;
			break;
		}
	}
}
