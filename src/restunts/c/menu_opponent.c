#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

void run_opponent_menu(void)
{
	static legacy_s8* button_resource_ids[5] = {
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

	ensure_file_exists(4);
	miscptr = file_load_resfile(aMisc);
	opp_res = (legacy_s8 far*)file_load_resource(8, aSdosel);
	locate_many_resources(opp_res, aOpp0opp1opp2op, oppresources);
	selected = 0;
	resource_loaded = 0;
	displayed_opponent = 0xFFU;
	blit_mode = 0xFFU;
	sub_29772();
	mouse_draw_transparent_check();

	for (;;) {
		if (displayed_opponent !=
			(legacy_u8)gameconfig.game_opponenttype) {
			if (displayed_opponent != 0xFFU) {
				sprite_free_wnd(render_window_sprite);
				if (resource_loaded != 0)
					unload_resource(opponent_resource);
			}

			ensure_file_exists(4);
			if ((legacy_u8)gameconfig.game_opponenttype != 0) {
				aOpp1[3] = (legacy_s8)(
					(legacy_u8)gameconfig.game_opponenttype + '0');
				opponent_resource = (legacy_s8 far*)file_load_resfile(aOpp1);
				resource_loaded = 1;
			} else {
				resource_loaded = 0;
			}

			render_window_sprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);
			displayed_opponent =
				(legacy_u8)gameconfig.game_opponenttype;
			previous_selection = 0xFFU;
			if (video_flag5_is0 == 0)
				sprite_copy_wnd_to_1();
			else
				setup_mcgawnd2();
			sprite_clear_1_color(0);

			shape = (struct SHAPE2D far*)locate_shape_fatal(
				opp_res, aScrn_0);
			sub_34526(shape);
			for (index = 0; index < 5U; index++) {
				draw_button(locate_text_res((legacy_s8 far*)miscptr,
					button_resource_ids[index]),
					LEGACY_S16_WRAP_ADD(0x15,
						LEGACY_U16_WRAP_MUL(index, 0x38U)),
					opponentmenu_buttons_y1[0] + 1,
					0x36, 0x12, word_407F4, word_407F6,
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

			if ((legacy_u8)gameconfig.game_opponenttype != 0)
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
						font_draw_text(&resID_byte1, 0x0C,
							LEGACY_S16_WRAP_ADD(line_y, 0x21));
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
			blit_mode = 0xFEU;
			(void)timer_get_delta_alt();
			sub_29772();
		}

		elapsed = (legacy_u16)mouse_timer_sprite_unk(selected,
			opponentmenu_buttons_x1, opponentmenu_buttons_x2,
			opponentmenu_buttons_y1, opponentmenu_buttons_y2,
			word_407CE, word_407D0);
		key = (legacy_u16)input_checking(
			LEGACY_S16_FROM_BITS(elapsed));
		hit = (legacy_s16)mouse_multi_hittest(5,
			opponentmenu_buttons_x1, opponentmenu_buttons_x2,
			opponentmenu_buttons_y1, opponentmenu_buttons_y2);
		if (hit != -1 &&
			!((legacy_u8)gameconfig.game_opponenttype == 0 && hit == 3))
			selected = (legacy_u8)hit;

		if (key == 0)
			continue;
		if (key == 0x4B00U) {
			selected = selected == 0 ? 4U :
				(legacy_u8)(selected - 1U);
			if ((legacy_u8)gameconfig.game_opponenttype == 0 &&
				selected == 3)
				selected--;
			continue;
		}
		if (key == 0x4D00U) {
			selected = selected < 4U ?
				(legacy_u8)(selected + 1U) : 0U;
			if ((legacy_u8)gameconfig.game_opponenttype == 0 &&
				selected == 3)
				selected++;
			continue;
		}
		if (key != 0x0DU && key != 0x1BU && key != 0x20U)
			continue;

		if (selected == 0) {
			gameconfig.game_opponenttype = (legacy_s8)(
				(legacy_u8)gameconfig.game_opponenttype - 1U);
			if (LEGACY_S8_FROM_BITS(
				(legacy_u8)gameconfig.game_opponenttype) < 1)
				gameconfig.game_opponenttype = 6;
			continue;
		}
		if (selected == 1) {
			gameconfig.game_opponenttype = (legacy_s8)(
				(legacy_u8)gameconfig.game_opponenttype + 1U);
			if ((legacy_u8)gameconfig.game_opponenttype == 7)
				gameconfig.game_opponenttype = 1;
			continue;
		}
		if (selected == 2) {
			gameconfig.game_opponenttype = 0;
			continue;
		}
		if (selected == 3) {
			if ((legacy_u8)gameconfig.game_opponenttype == 0)
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
			displayed_opponent = 0xFFU;
			mouse_draw_transparent_check();
			continue;
		}

		if ((legacy_u8)gameconfig.game_opponenttype != 0) {
			if ((legacy_u8)gameconfig.game_opponentcarid[0] == 0xFFU) {
				for (index = 0; index < 4U; index++)
					gameconfig.game_opponentcarid[index] =
						gameconfig.game_playercarid[index];
				gameconfig.game_opponentmaterial = (legacy_s8)(
					(((legacy_u8)gameconfig.game_playermaterial & 1U) ^ 1U));
				gameconfig.game_opponenttransmission = 0;
			}
		} else {
			gameconfig.game_opponentcarid[0] = (legacy_s8)0xFFU;
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
