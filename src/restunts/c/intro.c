#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "resource.h"
#include "shape2d.h"

static void far* ui_temp_resource;

legacy_s16 input_repeat_check(legacy_s16 duration);

legacy_s16 run_intro(void)
{
	struct SHAPE2D far* shape;
	legacy_s16 result;

	mouse_draw_opaque_check();
	sprite_copy_2_to_1_clear();
	mouse_draw_transparent_check();
	sprite_copy_wnd_to_1_clear();

	shape = (struct SHAPE2D far*)locate_shape_fatal(
		(legacy_s8 far*)ui_temp_resource, "prod");
	waitflag = shape2d_get_pos_y(shape) != 0 ? 0xA0 : 0xB4;

	shape = (struct SHAPE2D far*)locate_shape_fatal(
		(legacy_s8 far*)ui_temp_resource, "prod");
	sprite_shape_to_1_alt(shape);
	result = sprite_blit_to_video(render_window_sprite, -1);
	if (result == 0)
		result = input_repeat_check(0x190);

	if (result == 0) {
		sprite_copy_wnd_to_1_clear();
		waitflag = 0xB4;
		shape = (struct SHAPE2D far*)locate_shape_fatal(
			(legacy_s8 far*)ui_temp_resource, "titl");
		sprite_shape_to_1_alt(shape);
		result = sprite_blit_to_video(render_window_sprite, -1);
		if (result == 0)
			result = input_repeat_check(0x190);
	}

	return result;
}

legacy_s16 run_intro_looped(void)
{
	legacy_s16 result;

	file_load_audiores("skidtitl", "skidms", "TITL");
	ui_temp_resource = file_load_resource(2, "sdtitl");
	render_window_sprite = sprite_make_wnd(0x140, 0xC8, 0x0F);
	result = run_intro();
	sprite_free_wnd(render_window_sprite);
	mmgr_free((legacy_s8 far*)ui_temp_resource);

	if (result == 0) {
		result = setup_intro();
		if (result == 0) {
			ui_temp_resource = file_load_resource(2, "sdcred");
			render_window_sprite = sprite_make_wnd(0x140, 0xC8, 0x0F);
			sprite_copy_wnd_to_1_clear();
			sprite_blit_to_video(render_window_sprite, 0);
			result = load_intro_resources();
			sprite_free_wnd(render_window_sprite);
			mmgr_free((legacy_s8 far*)ui_temp_resource);
		}
	}

	audio_unload();
	return result;
}

static void intro_draw_resource_line(
	legacy_s8 far* resource,
	legacy_s8* resource_id,
	legacy_s16 is_text,
	legacy_s16 x,
	legacy_s16 y,
	legacy_s16 color,
	legacy_s16 shadow_color
) {
	legacy_s8 far* text;

	if (is_text != 0)
		text = locate_text_res(resource, resource_id);
	else
		text = locate_shape_alt(resource, resource_id);
	copy_string(&resID_byte1, text);
	intro_draw_text(&resID_byte1, x, y, color, shadow_color);
}

legacy_s8 load_intro_resources(void)
{
	legacy_s8 far* credit_resource;
	struct SHAPE2D far* credit_shapes[11];
	struct SHAPE2D far* arrow_shape;
	legacy_s16 target_x;
	legacy_s16 arrow_x;
	legacy_s16 arrow_y;
	legacy_s16 arrow_width;
	legacy_s16 arrow_height;
	legacy_s16 frame_elapsed;
	legacy_s16 animation_elapsed;
	legacy_s16 animation_target;
	legacy_s16 input;
	legacy_u16 animation_index;

	credit_resource = (legacy_s8 far*)file_load_resfile(aCred);
	locate_many_resources((legacy_s8 far*)ui_temp_resource,
		aArowarrwarw1ar, (legacy_s8 far**)credit_shapes);
	waitflag = 0x96;
	sprite_copy_wnd_to_1_clear();
	arrow_shape = credit_shapes[1];
	target_x = (legacy_s16)shape2d_get_pos_x(arrow_shape);
	arrow_y = (legacy_s16)shape2d_get_pos_y(arrow_shape);
	arrow_width = LEGACY_S16_WRAP_MUL(
		(legacy_s16)shape2d_get_width(arrow_shape), video_flag1_is1);
	arrow_height = (legacy_s16)shape2d_get_height(arrow_shape);

	intro_draw_resource_line(credit_resource, aCre, 1,
		0x78, 0, word_407D8, word_407DA);
	intro_draw_resource_line(credit_resource, aGds0, 0,
		0x3C, 0x0C, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGds1, 0,
		0x68, 0x14, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aDes, 1,
		0x14, 0x20, word_407DC, word_407DE);
	intro_draw_resource_line(credit_resource, aGdon, 0,
		0x14, 0x2C, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGkev, 0,
		0x14, 0x34, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGbra, 0,
		0x14, 0x3C, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGrob, 0,
		0x14, 0x44, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGsta, 0,
		0x14, 0x4C, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aMus, 1,
		0x14, 0x5C, word_407E8, word_407EA);
	intro_draw_resource_line(credit_resource, aGmsy, 0,
		0x14, 0x68, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGkri, 0,
		0x14, 0x70, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGbri, 0,
		0x14, 0x78, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aPro, 1,
		0xAC, 0x20, word_407E0, word_407E2);
	intro_draw_resource_line(credit_resource, aGkev_0, 0,
		0xAC, 0x2C, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aOpr, 1,
		0xAC, 0x38, word_407E0, word_407E2);
	intro_draw_resource_line(credit_resource, aGbra_0, 0,
		0xAC, 0x40, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGric, 0,
		0xAC, 0x48, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aArt, 1,
		0xAC, 0x54, word_407E4, word_407E6);
	intro_draw_resource_line(credit_resource, aGmsm, 0,
		0xAC, 0x60, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGdav, 0,
		0xAC, 0x68, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGnic, 0,
		0xAC, 0x70, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGkev_1, 0,
		0xAC, 0x78, word_407D4, word_407D6);
	unload_resource(credit_resource);

	(void)sprite_blit_to_video(render_window_sprite, -1);
	sprite_copy_2_to_1_2();
	(void)timer_get_delta_alt();
	arrow_x = 0x14A;
	input = 0;
	for (;;) {
		frame_elapsed = (legacy_s16)timer_get_delta_alt();
		arrow_x = LEGACY_S16_WRAP_SUB(arrow_x,
			LEGACY_S16_WRAP_MUL(frame_elapsed, 2));
		if (target_x > arrow_x)
			break;
		mouse_draw_opaque_check();
		sprite_putimage_and_alt(arrow_shape, arrow_x, arrow_y);
		sprite_1_unk2(LEGACY_S16_WRAP_ADD(arrow_width, arrow_x),
			arrow_y, 0x20, arrow_height, 0);
		mouse_draw_transparent_check();
		input = (legacy_s16)input_do_checking(frame_elapsed);
		if (input != 0)
			break;
	}

	arrow_y = (legacy_s16)shape2d_get_pos_y(credit_shapes[0]);
	animation_target = 0;
	animation_elapsed = 0;
	for (animation_index = 2;
		animation_index < 10U && input == 0;
		animation_index++) {
		sprite_copy_wnd_to_1();
		sprite_set_1_size(0, 0x140, arrow_y, 0xC8);
		sprite_clear_1_color(0);
		sprite_shape_to_1_alt(credit_shapes[animation_index]);
		sprite_copy_2_to_1_2();
		sprite_set_1_size(0, 0x140, arrow_y, 0xC8);
		mouse_draw_opaque_check();
		sprite_putimage(render_window_sprite->sprite_bitmapptr);
		mouse_draw_transparent_check();
		animation_target = LEGACY_S16_WRAP_ADD(animation_target, 5);
		while (animation_target > animation_elapsed) {
			frame_elapsed = (legacy_s16)timer_get_delta_alt();
			input = (legacy_s16)input_do_checking(frame_elapsed);
			animation_elapsed = LEGACY_S16_WRAP_ADD(
				animation_elapsed, frame_elapsed);
		}
	}

	sprite_set_1_size(0, 0x140, 0, 0xC8);
	mouse_draw_opaque_check();
	sprite_clear_shape(render_window_sprite->sprite_bitmapptr);
	sprite_copy_wnd_to_1();
	sprite_set_1_size(0, 0x140, arrow_y, 0xC8);
	sprite_clear_1_color(0);
	sprite_shape_to_1_alt(credit_shapes[0]);
	sprite_shape_to_1_alt(credit_shapes[10]);
	if (sprite_blit_to_video(render_window_sprite, 0) != 0)
		return 1;
	return input_repeat_check(0x1F4) != 0;
}
