#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

#define OPPONENT_RESOURCE_FILE_INDEX 4
#define OPPONENT_NONE 0U
#define OPPONENT_FIRST 1U
#define OPPONENT_LAST 6U
#define OPPONENT_AFTER_LAST 7U
#define OPPONENT_ID_DIGIT_INDEX 3U
#define OPPONENT_MENU_PREVIOUS_BUTTON 0U
#define OPPONENT_MENU_NEXT_BUTTON 1U
#define OPPONENT_MENU_NONE_BUTTON 2U
#define OPPONENT_MENU_CAR_BUTTON 3U
#define OPPONENT_MENU_DONE_BUTTON 4U
#define OPPONENT_MENU_BUTTON_COUNT 5U
#define OPPONENT_MENU_NO_SELECTION 255U
#define OPPONENT_MENU_INITIAL_BLIT_MODE 255U
#define OPPONENT_MENU_REFRESH_BLIT_MODE 254U
#define OPPONENT_MENU_SCREEN_WIDTH 320U
#define OPPONENT_MENU_SCREEN_HEIGHT 200U
#define OPPONENT_MENU_TRANSPARENT_COLOR 15U
#define OPPONENT_MENU_BUTTON_FIRST_X 21
#define OPPONENT_MENU_BUTTON_SPACING 56U
#define OPPONENT_MENU_BUTTON_WIDTH 54
#define OPPONENT_MENU_BUTTON_HEIGHT 18
#define OPPONENT_DESCRIPTION_X 12
#define OPPONENT_DESCRIPTION_FIRST_LINE_Y 33
#define CAR_ID_LENGTH 4U
#define CAR_MATERIAL_VARIANT_MASK 1U

void run_opponent_menu(void)
{
	static legacy_s8* button_resource_ids[OPPONENT_MENU_BUTTON_COUNT] = {
		aBla, aBnx, aBcl, aBca, aBdo
	};
	legacy_s8 far* opponent_resource;
	legacy_s8 far* description;
	struct SHAPE2D far* shape;
	legacy_u8 selected;
	legacy_u8 previous_selection;
	legacy_u8 displayed_opponent;
	legacy_u8 blit_mode;
	legacy_u8 resource_loaded;
	legacy_u8 character;
	legacy_u16 line_length;
	legacy_s16 line_y;
	legacy_u16 elapsed;
	legacy_u16 key;
	legacy_s16 hit;
	legacy_u16 index;

	ensure_file_exists(OPPONENT_RESOURCE_FILE_INDEX);
	miscptr = file_load_resfile(aMisc);
	opp_res = (legacy_s8 far*)file_load_resource(
		FILE_RESOURCE_SHAPE2D_ALTERNATE, aSdosel);
	locate_many_resources(opp_res, aOpp0opp1opp2op, oppresources);
	selected = OPPONENT_MENU_PREVIOUS_BUTTON;
	resource_loaded = 0;
	displayed_opponent = OPPONENT_MENU_NO_SELECTION;
	blit_mode = OPPONENT_MENU_INITIAL_BLIT_MODE;
	sub_29772();
	mouse_draw_transparent_check();

	for (;;) {
		if (displayed_opponent !=
			(legacy_u8)gameconfig.game_opponenttype) {
			if (displayed_opponent != OPPONENT_MENU_NO_SELECTION) {
				sprite_free_wnd(render_window_sprite);
				if (resource_loaded != 0)
					unload_resource(opponent_resource);
			}

			ensure_file_exists(OPPONENT_RESOURCE_FILE_INDEX);
			if ((legacy_u8)gameconfig.game_opponenttype != OPPONENT_NONE) {
				aOpp1[OPPONENT_ID_DIGIT_INDEX] = (legacy_s8)(
					(legacy_u8)gameconfig.game_opponenttype + '0');
				opponent_resource = (legacy_s8 far*)file_load_resfile(aOpp1);
				resource_loaded = 1;
			} else {
				resource_loaded = 0;
			}

			render_window_sprite = sprite_make_wnd(OPPONENT_MENU_SCREEN_WIDTH,
				OPPONENT_MENU_SCREEN_HEIGHT,
				OPPONENT_MENU_TRANSPARENT_COLOR);
			displayed_opponent =
				(legacy_u8)gameconfig.game_opponenttype;
			previous_selection = OPPONENT_MENU_NO_SELECTION;
			if (video_flag5_is0 == 0)
				sprite_copy_wnd_to_1();
			else
				setup_mcgawnd2();
			sprite_clear_1_color(0);

			shape = (struct SHAPE2D far*)locate_shape_fatal(
				opp_res, aScrn_0);
			sub_34526(shape);
			for (index = 0; index < OPPONENT_MENU_BUTTON_COUNT; index++) {
				draw_button(locate_text_res((legacy_s8 far*)miscptr,
					button_resource_ids[index]),
					LEGACY_S16_WRAP_ADD(OPPONENT_MENU_BUTTON_FIRST_X,
						LEGACY_U16_WRAP_MUL(index,
							OPPONENT_MENU_BUTTON_SPACING)),
					opponentmenu_buttons[0].y1 + 1,
					OPPONENT_MENU_BUTTON_WIDTH,
					OPPONENT_MENU_BUTTON_HEIGHT, word_407F4, word_407F6,
					word_407F8, 0);
			}

			sub_34526((struct SHAPE2D far*)
				oppresources[(legacy_u8)gameconfig.game_opponenttype]);
			shape = (struct SHAPE2D far*)locate_shape_fatal(
				opp_res, aClip);
			sub_34526(shape);
			if (video_flag5_is0 != 0) {
				sprite_clear_shape_alt(
					render_window_sprite->sprite_bitmapptr, 0, 0);
				sprite_copy_wnd_to_1();
			}

			if ((legacy_u8)gameconfig.game_opponenttype != OPPONENT_NONE)
				description = locate_text_res(
					opponent_resource, aDes_0);
			else
				description = locate_text_res(
					(legacy_s8 far*)miscptr, aRac);
			font_set_fontdef2(fontnptr);
			font_set_unk(0, dialog_fnt_colour);
			line_length = 0;
			line_y = 0;
			for (;;) {
				character = (legacy_u8)*description++;
				if (character == ']') {
					if (line_length != 0) {
						*(&resID_byte1 + line_length) = 0;
						font_draw_text(&resID_byte1, OPPONENT_DESCRIPTION_X,
							LEGACY_S16_WRAP_ADD(line_y,
								OPPONENT_DESCRIPTION_FIRST_LINE_Y));
					}
					line_length = 0;
					line_y = LEGACY_S16_WRAP_ADD(
						line_y, fontdef_unk_0E);
				} else {
					*(&resID_byte1 + line_length++) = (legacy_s8)character;
				}
				if (*description == 0)
					break;
			}
			font_set_fontdef();
		}

		if (selected != previous_selection) {
			previous_selection = selected;
			sprite_blit_to_video(render_window_sprite,
				LEGACY_S8_FROM_BITS(blit_mode));
			blit_mode = OPPONENT_MENU_REFRESH_BLIT_MODE;
			(void)timer_get_delta_alt();
			sub_29772();
		}

		elapsed = (legacy_u16)mouse_timer_sprite_unk(selected,
			opponentmenu_buttons, word_407CE, word_407D0);
		key = (legacy_u16)input_checking(
			LEGACY_S16_FROM_BITS(elapsed));
		hit = (legacy_s16)mouse_multi_hittest(OPPONENT_MENU_BUTTON_COUNT,
			opponentmenu_buttons);
		if (hit != -1 &&
			!((legacy_u8)gameconfig.game_opponenttype == OPPONENT_NONE &&
				hit == OPPONENT_MENU_CAR_BUTTON))
			selected = (legacy_u8)hit;

		if (key == 0)
			continue;
		if (key == KEY_LEFT) {
			selected = selected == OPPONENT_MENU_PREVIOUS_BUTTON ?
				OPPONENT_MENU_DONE_BUTTON :
				(legacy_u8)(selected - 1U);
			if ((legacy_u8)gameconfig.game_opponenttype == OPPONENT_NONE &&
				selected == OPPONENT_MENU_CAR_BUTTON)
				selected--;
			continue;
		}
		if (key == KEY_RIGHT) {
			selected = selected < OPPONENT_MENU_DONE_BUTTON ?
				(legacy_u8)(selected + 1U) : OPPONENT_MENU_PREVIOUS_BUTTON;
			if ((legacy_u8)gameconfig.game_opponenttype == OPPONENT_NONE &&
				selected == OPPONENT_MENU_CAR_BUTTON)
				selected++;
			continue;
		}
		if (key != KEY_ENTER && key != KEY_ESCAPE && key != KEY_SPACE)
			continue;

		if (selected == OPPONENT_MENU_PREVIOUS_BUTTON) {
			gameconfig.game_opponenttype = (legacy_s8)(
				(legacy_u8)gameconfig.game_opponenttype - 1U);
			if (LEGACY_S8_FROM_BITS(
				(legacy_u8)gameconfig.game_opponenttype) < OPPONENT_FIRST)
				gameconfig.game_opponenttype = OPPONENT_LAST;
			continue;
		}
		if (selected == OPPONENT_MENU_NEXT_BUTTON) {
			gameconfig.game_opponenttype = (legacy_s8)(
				(legacy_u8)gameconfig.game_opponenttype + 1U);
			if ((legacy_u8)gameconfig.game_opponenttype == OPPONENT_AFTER_LAST)
				gameconfig.game_opponenttype = OPPONENT_FIRST;
			continue;
		}
		if (selected == OPPONENT_MENU_NONE_BUTTON) {
			gameconfig.game_opponenttype = OPPONENT_NONE;
			continue;
		}
		if (selected == OPPONENT_MENU_CAR_BUTTON) {
			if ((legacy_u8)gameconfig.game_opponenttype == OPPONENT_NONE)
				continue;
			check_input();
			mouse_draw_opaque_check();
			sprite_free_wnd(render_window_sprite);
			unload_resource(opponent_resource);
			show_waiting();
			run_car_menu(&gameconfig.game_opponentcarid[0],
				&gameconfig.game_opponentmaterial,
				&gameconfig.game_opponenttransmission,
				(legacy_u8)gameconfig.game_opponenttype);
			displayed_opponent = OPPONENT_MENU_NO_SELECTION;
			mouse_draw_transparent_check();
			continue;
		}

		if ((legacy_u8)gameconfig.game_opponenttype != OPPONENT_NONE) {
			if ((legacy_u8)gameconfig.game_opponentcarid[0] ==
				OPPONENT_MENU_NO_SELECTION) {
				for (index = 0; index < CAR_ID_LENGTH; index++)
					gameconfig.game_opponentcarid[index] =
						gameconfig.game_playercarid[index];
				gameconfig.game_opponentmaterial = (legacy_s8)(
					(((legacy_u8)gameconfig.game_playermaterial &
						CAR_MATERIAL_VARIANT_MASK) ^
						CAR_MATERIAL_VARIANT_MASK));
				gameconfig.game_opponenttransmission = 0;
			}
		} else {
			gameconfig.game_opponentcarid[0] =
				LEGACY_S8_FROM_BITS(OPPONENT_MENU_NO_SELECTION);
		}

		sprite_free_wnd(render_window_sprite);
		if (resource_loaded != 0)
			unload_resource(opponent_resource);
		mmgr_free(opp_res);
		unload_resource(miscptr);
		mouse_draw_opaque_check();
		return;
	}
}
