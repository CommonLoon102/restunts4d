#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"
#include "ui_input.h"
#include "ui_dialog.h"
#include "menu_common.h"
#include "externs.h"
#include "keyboard.h"

#define MAIN_MENU_BUTTON_COUNT 5U
#define MAIN_MENU_NO_SELECTION 255U
#define MAIN_MENU_MOUSE_HIT_NONE (-1)
#define MAIN_MENU_IDLE_NOT_EXPIRED 0
#define MAIN_MENU_KEY_NONE 0U
#define MAIN_MENU_WAIT_TICKS 180
#define MAIN_MENU_IDLE_LIMIT_TICKS 6000
#define MAIN_MENU_SCREEN_WIDTH 320U
#define MAIN_MENU_SCREEN_HEIGHT 200U
#define MAIN_MENU_TRANSPARENT_COLOR 15U

enum MAIN_MENU_SELECTION {
	MAIN_MENU_DRIVE = 0,
	MAIN_MENU_CAR = 1,
	MAIN_MENU_OPPONENT = 2,
	MAIN_MENU_TRACK = 3,
	MAIN_MENU_OPTIONS = 4
};

legacy_s8 run_menu(void)
{
	static const legacy_u8 previous_selection[MAIN_MENU_BUTTON_COUNT] = {
		MAIN_MENU_CAR, MAIN_MENU_OPPONENT, MAIN_MENU_OPTIONS, MAIN_MENU_DRIVE, MAIN_MENU_TRACK};
	static const legacy_u8 next_selection[MAIN_MENU_BUTTON_COUNT] = {
		MAIN_MENU_TRACK, MAIN_MENU_DRIVE, MAIN_MENU_CAR, MAIN_MENU_OPTIONS, MAIN_MENU_OPPONENT};

	legacy_u8 selected = MAIN_MENU_DRIVE;
	legacy_u8 previous = MAIN_MENU_NO_SELECTION;
	legacy_u8 blit_mode = MENU_BLIT_MODE_INITIAL;
	show_waiting();
	waitflag = MAIN_MENU_WAIT_TICKS;
	render_window_sprite = sprite_make_wnd(MAIN_MENU_SCREEN_WIDTH, MAIN_MENU_SCREEN_HEIGHT,
										   MAIN_MENU_TRANSPARENT_COLOR);
	legacy_s8 far *resource =
		(legacy_s8 far *)file_load_resource(FILE_RESOURCE_SHAPE2D, main_menu_shapes_name);
	sprite_select_render_window();
	struct SHAPE2D far *shape =
		(struct SHAPE2D far *)locate_shape_fatal(resource, main_menu_background_data);
	sprite_shape_to_1_alt(shape);
	mmgr_free(resource);

	for (;;) {
		if (selected != previous) {
			previous = selected;
			sprite_select_render_window();
			sprite_blit_to_video(render_window_sprite, LEGACY_S8_FROM_BITS(blit_mode));
			blit_mode = MENU_BLIT_MODE_REFRESH;
			sprite_select_screen_compat();
			menu_reset_animation_timers();
		}

		legacy_u16 elapsed = (legacy_u16)menu_animate_button_highlight(
			selected, menu_buttons, menu_highlight_second_color, menu_highlight_first_color);
		legacy_u16 key = (legacy_u16)input_checking(LEGACY_S16_FROM_BITS(elapsed));
		legacy_s16 hit = (legacy_s16)mouse_multi_hittest(MAIN_MENU_BUTTON_COUNT, menu_buttons);
		if (hit != MAIN_MENU_MOUSE_HIT_NONE) {
			selected = (legacy_u8)hit;
		}

		menu_update_idle_counter(elapsed, MAIN_MENU_IDLE_LIMIT_TICKS);
		if (idle_expired != MAIN_MENU_IDLE_NOT_EXPIRED) {
			selected = MAIN_MENU_DRIVE;
			key = KEY_ENTER;
		}

		if (key == MAIN_MENU_KEY_NONE) {
			continue;
		}
		if (key == KEY_ENTER || key == KEY_SPACE) {
			break;
		}
		if (key == KEY_ESCAPE) {
			selected = MAIN_MENU_NO_SELECTION;
			break;
		}
		if (key == KEY_LEFT) {
			selected = previous_selection[selected];
		} else if (key == KEY_RIGHT) {
			selected = next_selection[selected];
		}
	}

	sprite_free_wnd(render_window_sprite);
	return LEGACY_S8_FROM_BITS(selected);
}
