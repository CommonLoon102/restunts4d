#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"
#include "ui_text.h"
#include "timing.h"
#include "game_input.h"
#include "ui_input.h"
#include "ui_dialog.h"
#include "car_resources.h"
#include "menu_common.h"
#include "externs.h"
#include "keyboard.h"

#define OPPONENT_RESOURCE_FILE_INDEX 4
#define OPPONENT_NONE 0U
#define OPPONENT_FIRST 1U
#define OPPONENT_LAST 6U
#define OPPONENT_AFTER_LAST 7U
#define OPPONENT_ID_DIGIT_INDEX 3U
#define OPPONENT_MENU_BUTTON_COUNT 5U
#define OPPONENT_MENU_NO_SELECTION 255U
#define OPPONENT_MENU_SCREEN_WIDTH 320U
#define OPPONENT_MENU_SCREEN_HEIGHT 200U
#define OPPONENT_MENU_TRANSPARENT_COLOR 15U
#define OPPONENT_MENU_BUTTON_FIRST_X 21
#define OPPONENT_MENU_BUTTON_SPACING 56U
#define OPPONENT_MENU_BUTTON_WIDTH 54
#define OPPONENT_MENU_BUTTON_HEIGHT 18
#define OPPONENT_DESCRIPTION_X 12
#define OPPONENT_DESCRIPTION_FIRST_LINE_Y 33

enum OPPONENT_MENU_BUTTON {
	OPPONENT_MENU_PREVIOUS_BUTTON = 0,
	OPPONENT_MENU_NEXT_BUTTON = 1,
	OPPONENT_MENU_NONE_BUTTON = 2,
	OPPONENT_MENU_CAR_BUTTON = 3,
	OPPONENT_MENU_DONE_BUTTON = 4
};

#define CAR_ID_LENGTH 4U
#define CAR_MATERIAL_VARIANT_MASK 1U

struct OPPONENT_MENU_STATE {
	legacy_s8 far *opponent_resource;
	legacy_u8 selected;
	legacy_u8 previous_selection;
	legacy_u8 displayed_opponent;
	legacy_u8 blit_mode;
	legacy_u8 resource_loaded;
};

static void opponent_menu_draw_description(struct OPPONENT_MENU_STATE *menu)
{
	legacy_s8 far *description;
	legacy_u8 character;
	legacy_u16 line_length;
	legacy_s16 line_y;
	if ((legacy_u8)gameconfig.game_opponenttype != OPPONENT_NONE) {
		description = locate_text_res(menu->opponent_resource, opponent_description_id);
	} else {
		description = locate_text_res((legacy_s8 far *)miscptr, opponent_racing_car_label_id);
	}
	font_set_fontdef2(fontnptr);
	font_set_colors(0, dialog_fnt_colour);
	line_length = 0;
	line_y = 0;
	for (;;) {
		character = (legacy_u8)*description++;
		if (character == ']') {
			if (line_length != 0) {
				*(&resID_byte1 + line_length) = 0;
				font_draw_text(&resID_byte1, OPPONENT_DESCRIPTION_X,
							   LEGACY_S16_WRAP_ADD(line_y, OPPONENT_DESCRIPTION_FIRST_LINE_Y));
			}
			line_length = 0;
			line_y = LEGACY_S16_WRAP_ADD(line_y, font_glyph_height);
		} else {
			*(&resID_byte1 + line_length++) = (legacy_s8)character;
		}
		if (*description == 0) {
			break;
		}
	}
	font_set_fontdef();
}

static void opponent_menu_draw_background(void)
{
	static legacy_s8 *button_resource_ids[OPPONENT_MENU_BUTTON_COUNT] = {
		opponent_previous_button_id, opponent_next_button_id, opponent_none_button_id,
		opponent_car_button_id, opponent_done_button_id};
	struct SHAPE2D far *shape;
	legacy_u16 index;
	if (video_uses_page_flipping == 0) {
		sprite_select_render_window();
	} else {
		sprite_select_mcga_backbuffer();
	}
	sprite_clear_target(0);

	shape = (struct SHAPE2D far *)locate_shape_fatal(opp_res, opponent_menu_background_id);
	sprite_draw_palette_mapped(shape);
	for (index = 0; index < OPPONENT_MENU_BUTTON_COUNT; index++) {
		draw_button(locate_text_res((legacy_s8 far *)miscptr, button_resource_ids[index]),
					LEGACY_S16_WRAP_ADD(OPPONENT_MENU_BUTTON_FIRST_X,
										LEGACY_U16_WRAP_MUL(index, OPPONENT_MENU_BUTTON_SPACING)),
					opponentmenu_buttons[0].y1 + 1, OPPONENT_MENU_BUTTON_WIDTH,
					OPPONENT_MENU_BUTTON_HEIGHT, button_top_color, button_bottom_color,
					button_fill_color, 0);
	}

	sprite_draw_palette_mapped(
		(struct SHAPE2D far *)oppresources[(legacy_u8)gameconfig.game_opponenttype]);
	shape = (struct SHAPE2D far *)locate_shape_fatal(opp_res, opponent_portrait_clip_id);
	sprite_draw_palette_mapped(shape);
	if (video_uses_page_flipping != 0) {
		sprite_clear_shape_alt(render_window_sprite->sprite_bitmapptr, 0, 0);
		sprite_select_render_window();
	}
}

static void opponent_menu_refresh(struct OPPONENT_MENU_STATE *menu)
{
	if (menu->displayed_opponent != (legacy_u8)gameconfig.game_opponenttype) {
		if (menu->displayed_opponent != OPPONENT_MENU_NO_SELECTION) {
			sprite_free_wnd(render_window_sprite);
			if (menu->resource_loaded != 0) {
				unload_resource(menu->opponent_resource);
			}
		}

		ensure_file_exists(OPPONENT_RESOURCE_FILE_INDEX);
		if ((legacy_u8)gameconfig.game_opponenttype != OPPONENT_NONE) {
			opponent_resource_name[OPPONENT_ID_DIGIT_INDEX] =
				(legacy_s8)((legacy_u8)gameconfig.game_opponenttype + '0');
			menu->opponent_resource = (legacy_s8 far *)file_load_resfile(opponent_resource_name);
			menu->resource_loaded = 1;
		} else {
			menu->resource_loaded = 0;
		}

		render_window_sprite =
			sprite_make_wnd(OPPONENT_MENU_SCREEN_WIDTH, OPPONENT_MENU_SCREEN_HEIGHT,
							OPPONENT_MENU_TRANSPARENT_COLOR);
		menu->displayed_opponent = (legacy_u8)gameconfig.game_opponenttype;
		menu->previous_selection = OPPONENT_MENU_NO_SELECTION;
		opponent_menu_draw_background();

		opponent_menu_draw_description(menu);
	}
}

static legacy_u16 opponent_menu_poll_input(struct OPPONENT_MENU_STATE *menu)
{
	legacy_u16 elapsed;
	legacy_u16 key;
	legacy_s16 hit;
	if (menu->selected != menu->previous_selection) {
		menu->previous_selection = menu->selected;
		sprite_blit_to_video(render_window_sprite, LEGACY_S8_FROM_BITS(menu->blit_mode));
		menu->blit_mode = MENU_BLIT_MODE_REFRESH;
		(void)timer_get_delta_alt();
		menu_reset_animation_timers();
	}

	elapsed = (legacy_u16)menu_animate_button_highlight(menu->selected, opponentmenu_buttons,
														menu_highlight_second_color,
														menu_highlight_first_color);
	key = (legacy_u16)input_checking(LEGACY_S16_FROM_BITS(elapsed));
	hit = (legacy_s16)mouse_multi_hittest(OPPONENT_MENU_BUTTON_COUNT, opponentmenu_buttons);
	if (hit != -1 && !((legacy_u8)gameconfig.game_opponenttype == OPPONENT_NONE &&
					   hit == OPPONENT_MENU_CAR_BUTTON)) {
		menu->selected = (legacy_u8)hit;
	}
	return key;
}

static legacy_u8 opponent_menu_activate_key(struct OPPONENT_MENU_STATE *menu, legacy_u16 key)
{
	if (key == 0) {
		return 0;
	}
	if (key == KEY_LEFT) {
		menu->selected = menu->selected == OPPONENT_MENU_PREVIOUS_BUTTON
							 ? OPPONENT_MENU_DONE_BUTTON
							 : (legacy_u8)(menu->selected - 1U);
		if ((legacy_u8)gameconfig.game_opponenttype == OPPONENT_NONE &&
			menu->selected == OPPONENT_MENU_CAR_BUTTON) {
			menu->selected--;
		}
		return 0;
	}
	if (key == KEY_RIGHT) {
		menu->selected = menu->selected < OPPONENT_MENU_DONE_BUTTON
							 ? (legacy_u8)(menu->selected + 1U)
							 : OPPONENT_MENU_PREVIOUS_BUTTON;
		if ((legacy_u8)gameconfig.game_opponenttype == OPPONENT_NONE &&
			menu->selected == OPPONENT_MENU_CAR_BUTTON) {
			menu->selected++;
		}
		return 0;
	}
	if (key != KEY_ENTER && key != KEY_ESCAPE && key != KEY_SPACE) {
		return 0;
	}
	return 1;
}

static legacy_u8 opponent_menu_activate_selection(struct OPPONENT_MENU_STATE *menu)
{
	if (menu->selected == OPPONENT_MENU_PREVIOUS_BUTTON) {
		gameconfig.game_opponenttype = LEGACY_S8_WRAP_SUB(gameconfig.game_opponenttype, 1U);
		/* Preserve the signed comparison: Clock (0) decrements to -1 before wrapping. */
		if (LEGACY_S8_FROM_BITS((legacy_u8)gameconfig.game_opponenttype) <
			(legacy_s16)OPPONENT_FIRST) {
			gameconfig.game_opponenttype = OPPONENT_LAST;
		}
		return 0;
	}
	if (menu->selected == OPPONENT_MENU_NEXT_BUTTON) {
		gameconfig.game_opponenttype = (legacy_s8)((legacy_u8)gameconfig.game_opponenttype + 1U);
		if ((legacy_u8)gameconfig.game_opponenttype == OPPONENT_AFTER_LAST) {
			gameconfig.game_opponenttype = OPPONENT_FIRST;
		}
		return 0;
	}
	if (menu->selected == OPPONENT_MENU_NONE_BUTTON) {
		gameconfig.game_opponenttype = OPPONENT_NONE;
		return 0;
	}
	if (menu->selected == OPPONENT_MENU_CAR_BUTTON) {
		if ((legacy_u8)gameconfig.game_opponenttype == OPPONENT_NONE) {
			return 0;
		}
		check_input();
		mouse_draw_opaque_check();
		sprite_free_wnd(render_window_sprite);
		unload_resource(menu->opponent_resource);
		show_waiting();
		run_car_menu(&gameconfig.game_opponentcarid[0], &gameconfig.game_opponentmaterial,
					 &gameconfig.game_opponenttransmission,
					 (legacy_u8)gameconfig.game_opponenttype);
		menu->displayed_opponent = OPPONENT_MENU_NO_SELECTION;
		mouse_draw_transparent_check();
		return 0;
	}
	return 1;
}

static void opponent_menu_release(struct OPPONENT_MENU_STATE *menu)
{
	legacy_u16 index;
	if ((legacy_u8)gameconfig.game_opponenttype != OPPONENT_NONE) {
		if ((legacy_u8)gameconfig.game_opponentcarid[0] == OPPONENT_MENU_NO_SELECTION) {
			for (index = 0; index < CAR_ID_LENGTH; index++) {
				gameconfig.game_opponentcarid[index] = gameconfig.game_playercarid[index];
			}
			gameconfig.game_opponentmaterial = (legacy_s8)((
				((legacy_u8)gameconfig.game_playermaterial & CAR_MATERIAL_VARIANT_MASK) ^
				CAR_MATERIAL_VARIANT_MASK));
			gameconfig.game_opponenttransmission = TRANSMISSION_MANUAL;
		}
	} else {
		gameconfig.game_opponentcarid[0] = LEGACY_S8_FROM_BITS(OPPONENT_MENU_NO_SELECTION);
	}

	sprite_free_wnd(render_window_sprite);
	if (menu->resource_loaded != 0) {
		unload_resource(menu->opponent_resource);
	}
	mmgr_free(opp_res);
	unload_resource(miscptr);
	mouse_draw_opaque_check();
}

void run_opponent_menu(void)
{
	struct OPPONENT_MENU_STATE session;
	struct OPPONENT_MENU_STATE *menu = &session;
	legacy_u16 key;

	ensure_file_exists(OPPONENT_RESOURCE_FILE_INDEX);
	miscptr = file_load_resfile(opponent_misc_resource_name);
	opp_res = (legacy_s8 far *)file_load_resource(FILE_RESOURCE_SHAPE2D_ALTERNATE,
												  opponent_menu_shapes_name);
	locate_many_resources(opp_res, opponent_portrait_shape_ids, oppresources);
	menu->selected = OPPONENT_MENU_PREVIOUS_BUTTON;
	menu->resource_loaded = 0;
	menu->displayed_opponent = OPPONENT_MENU_NO_SELECTION;
	menu->blit_mode = MENU_BLIT_MODE_INITIAL;
	menu_reset_animation_timers();
	mouse_draw_transparent_check();

	for (;;) {
		opponent_menu_refresh(menu);

		key = opponent_menu_poll_input(menu);

		if (opponent_menu_activate_key(menu, key) == 0) {
			continue;
		}

		if (opponent_menu_activate_selection(menu) == 0) {
			continue;
		}

		opponent_menu_release(menu);
		return;
	}
}
