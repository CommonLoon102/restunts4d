#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

#define MAIN_MENU_DRIVE 0U
#define MAIN_MENU_CAR 1U
#define MAIN_MENU_OPPONENT 2U
#define MAIN_MENU_TRACK 3U
#define MAIN_MENU_OPTIONS 4U
#define MAIN_MENU_BUTTON_COUNT 5U
#define MAIN_MENU_NO_SELECTION 255U
#define MAIN_MENU_INITIAL_BLIT_MODE 255U
#define MAIN_MENU_REFRESH_BLIT_MODE 254U
#define MAIN_MENU_WAIT_TICKS 180
#define MAIN_MENU_IDLE_LIMIT_TICKS 6000
#define MAIN_MENU_SCREEN_WIDTH 320U
#define MAIN_MENU_SCREEN_HEIGHT 200U
#define MAIN_MENU_TRANSPARENT_COLOR 15U

legacy_s8 run_menu(void)
{
	static const legacy_u8 previous_selection[MAIN_MENU_BUTTON_COUNT] = {
		MAIN_MENU_CAR, MAIN_MENU_OPPONENT, MAIN_MENU_OPTIONS,
		MAIN_MENU_DRIVE, MAIN_MENU_TRACK
	};
	static const legacy_u8 next_selection[MAIN_MENU_BUTTON_COUNT] = {
		MAIN_MENU_TRACK, MAIN_MENU_DRIVE, MAIN_MENU_CAR,
		MAIN_MENU_OPTIONS, MAIN_MENU_OPPONENT
	};
	legacy_s8 far* resource;
	struct SHAPE2D far* shape;
	legacy_u8 selected;
	legacy_u8 previous;
	legacy_u8 blit_mode;
	legacy_u16 elapsed;
	legacy_u16 key;
	legacy_s16 hit;

	selected = MAIN_MENU_DRIVE;
	previous = MAIN_MENU_NO_SELECTION;
	blit_mode = MAIN_MENU_INITIAL_BLIT_MODE;
	show_waiting();
	waitflag = MAIN_MENU_WAIT_TICKS;
	render_window_sprite = sprite_make_wnd(MAIN_MENU_SCREEN_WIDTH,
		MAIN_MENU_SCREEN_HEIGHT, MAIN_MENU_TRANSPARENT_COLOR);
	resource = (legacy_s8 far*)file_load_resource(
		FILE_RESOURCE_SHAPE2D, aSdmsel);
	sprite_copy_wnd_to_1();
	shape = (struct SHAPE2D far*)locate_shape_fatal(resource, aScrn);
	sprite_shape_to_1_alt(shape);
	mmgr_free(resource);

	for (;;) {
		if (selected != previous) {
			previous = selected;
			sprite_copy_wnd_to_1();
			sprite_blit_to_video(render_window_sprite,
				LEGACY_S8_FROM_BITS(blit_mode));
			blit_mode = MAIN_MENU_REFRESH_BLIT_MODE;
			sprite_copy_2_to_1_2();
			sub_29772();
		}

		elapsed = (legacy_u16)mouse_timer_sprite_unk(selected,
			menu_buttons, word_407CE, word_407D0);
		key = (legacy_u16)input_checking(LEGACY_S16_FROM_BITS(elapsed));
		hit = (legacy_s16)mouse_multi_hittest(MAIN_MENU_BUTTON_COUNT,
			menu_buttons);
		if (hit != -1)
			selected = (legacy_u8)hit;

		menu_update_idle_counter(elapsed, MAIN_MENU_IDLE_LIMIT_TICKS);
		if (idle_expired != 0) {
			selected = MAIN_MENU_DRIVE;
			key = KEY_ENTER;
		}

		if (key == 0)
			continue;
		if (key == KEY_ENTER || key == KEY_SPACE)
			break;
		if (key == KEY_ESCAPE) {
			selected = MAIN_MENU_NO_SELECTION;
			break;
		}
		if (key == KEY_LEFT)
			selected = previous_selection[selected];
		else if (key == KEY_RIGHT)
			selected = next_selection[selected];
	}

	sprite_free_wnd(render_window_sprite);
	return LEGACY_S8_FROM_BITS(selected);
}
