#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

legacy_s8 run_menu(void)
{
	static const legacy_u8 previous_selection[5] = { 1, 2, 4, 0, 3 };
	static const legacy_u8 next_selection[5] = { 3, 0, 1, 4, 2 };
	legacy_s8 far* resource;
	struct SHAPE2D far* shape;
	legacy_u8 selected;
	legacy_u8 previous;
	legacy_u8 blit_mode;
	legacy_u16 elapsed;
	legacy_u16 key;
	legacy_s16 hit;

	selected = 0;
	previous = 0xFFU;
	blit_mode = 0xFFU;
	show_waiting();
	waitflag = 0xB4;
	render_window_sprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);
	resource = (legacy_s8 far*)file_load_resource(2, aSdmsel);
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
			blit_mode = 0xFEU;
			sprite_copy_2_to_1_2();
			sub_29772();
		}

		elapsed = (legacy_u16)mouse_timer_sprite_unk(selected,
			menu_buttons, word_407CE, word_407D0);
		key = (legacy_u16)input_checking(LEGACY_S16_FROM_BITS(elapsed));
		hit = (legacy_s16)mouse_multi_hittest(5, menu_buttons);
		if (hit != -1)
			selected = (legacy_u8)hit;

		menu_update_idle_counter(elapsed, 0x1770);
		if (idle_expired != 0) {
			selected = 0;
			key = 0x0DU;
		}

		if (key == 0)
			continue;
		if (key == 0x0DU || key == 0x20U)
			break;
		if (key == 0x1BU) {
			selected = 0xFFU;
			break;
		}
		if (key == 0x4B00U)
			selected = previous_selection[selected];
		else if (key == 0x4D00U)
			selected = next_selection[selected];
	}

	sprite_free_wnd(render_window_sprite);
	return LEGACY_S8_FROM_BITS(selected);
}
