#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "resource.h"
#include "shape2d.h"
#include "timing.h"
#include "ui_text.h"
#include "game_input.h"
#include "ui_input.h"
#include "audio_control.h"
#include "externs.h"

#define INTRO_SCREEN_WIDTH 320
#define INTRO_SCREEN_HEIGHT 200
#define INTRO_SCREEN_COLOR 15
#define INTRO_PRODUCTION_RAISED_WAIT 160
#define INTRO_DEFAULT_PAGE_WAIT 180
#define INTRO_PAGE_INPUT_DELAY 400

#define CREDITS_RESOURCE_COUNT 11
#define CREDITS_BACKGROUND_INDEX 0
#define CREDITS_ARROW_INDEX 1
#define CREDITS_FIRST_ANIMATION_INDEX 2U
#define CREDITS_ANIMATION_END_INDEX 10U
#define CREDITS_CLOSING_INDEX 10
#define CREDITS_INITIAL_WAIT 150
#define CREDITS_LEFT_COLUMN_X 20
#define CREDITS_RIGHT_COLUMN_X 172
#define CREDITS_TITLE_X 120
#define CREDITS_FIRST_LOGO_X 60
#define CREDITS_SECOND_LOGO_X 104
#define CREDITS_TITLE_Y 0
#define CREDITS_FIRST_LOGO_Y 12
#define CREDITS_SECOND_LOGO_Y 20
#define CREDITS_DESIGN_HEADING_Y 32
#define CREDITS_FIRST_DESIGNER_Y 44
#define CREDITS_SECOND_DESIGNER_Y 52
#define CREDITS_THIRD_DESIGNER_Y 60
#define CREDITS_FOURTH_DESIGNER_Y 68
#define CREDITS_FIFTH_DESIGNER_Y 76
#define CREDITS_MUSIC_HEADING_Y 92
#define CREDITS_FIRST_MUSICIAN_Y 104
#define CREDITS_SECOND_MUSICIAN_Y 112
#define CREDITS_THIRD_MUSICIAN_Y 120
#define CREDITS_PRODUCTION_HEADING_Y 32
#define CREDITS_PRODUCER_Y 44
#define CREDITS_OPPONENT_HEADING_Y 56
#define CREDITS_FIRST_OPPONENT_Y 64
#define CREDITS_SECOND_OPPONENT_Y 72
#define CREDITS_ART_HEADING_Y 84
#define CREDITS_FIRST_ARTIST_Y 96
#define CREDITS_SECOND_ARTIST_Y 104
#define CREDITS_THIRD_ARTIST_Y 112
#define CREDITS_FOURTH_ARTIST_Y 120
#define CREDITS_ARROW_START_X 330
#define CREDITS_ARROW_SPEED 2
#define CREDITS_ARROW_ERASE_WIDTH 32
#define CREDITS_ANIMATION_INTERVAL 5
#define CREDITS_END_INPUT_DELAY 500

enum CREDITS_LINE_TYPE {
	CREDITS_LINE_SHAPE = 0,
	CREDITS_LINE_TEXT = 1
};

static void far* ui_temp_resource;

legacy_s16 run_intro(void)
{
	struct SHAPE2D far* shape;
	legacy_s16 result;

	mouse_draw_opaque_check();
	sprite_select_screen_and_clear();
	mouse_draw_transparent_check();
	sprite_select_render_window_and_clear();

	shape = (struct SHAPE2D far*)locate_shape_fatal(
		(legacy_s8 far*)ui_temp_resource, "prod");
	waitflag = shape2d_get_pos_y(shape) != 0 ?
		INTRO_PRODUCTION_RAISED_WAIT : INTRO_DEFAULT_PAGE_WAIT;

	shape = (struct SHAPE2D far*)locate_shape_fatal(
		(legacy_s8 far*)ui_temp_resource, "prod");
	sprite_shape_to_1_alt(shape);
	result = sprite_blit_to_video(render_window_sprite, -1);
	if (result == 0)
		result = input_repeat_check(INTRO_PAGE_INPUT_DELAY);

	if (result == 0) {
		sprite_select_render_window_and_clear();
		waitflag = INTRO_DEFAULT_PAGE_WAIT;
		shape = (struct SHAPE2D far*)locate_shape_fatal(
			(legacy_s8 far*)ui_temp_resource, "titl");
		sprite_shape_to_1_alt(shape);
		result = sprite_blit_to_video(render_window_sprite, -1);
		if (result == 0)
			result = input_repeat_check(INTRO_PAGE_INPUT_DELAY);
	}

	return result;
}

legacy_s16 run_intro_looped(void)
{
	legacy_s16 result;

	file_load_audiores("skidtitl", "skidms", "TITL");
	ui_temp_resource = file_load_resource(FILE_RESOURCE_SHAPE2D, "sdtitl");
	render_window_sprite = sprite_make_wnd(INTRO_SCREEN_WIDTH,
		INTRO_SCREEN_HEIGHT, INTRO_SCREEN_COLOR);
	result = run_intro();
	sprite_free_wnd(render_window_sprite);
	mmgr_free((legacy_s8 far*)ui_temp_resource);

	if (result == 0) {
		result = setup_intro();
		if (result == 0) {
			ui_temp_resource = file_load_resource(
				FILE_RESOURCE_SHAPE2D, "sdcred");
			render_window_sprite = sprite_make_wnd(INTRO_SCREEN_WIDTH,
				INTRO_SCREEN_HEIGHT, INTRO_SCREEN_COLOR);
			sprite_select_render_window_and_clear();
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
	struct SHAPE2D far* credit_shapes[CREDITS_RESOURCE_COUNT];
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

	credit_resource = (legacy_s8 far*)file_load_resfile(credits_resource_name);
	locate_many_resources((legacy_s8 far*)ui_temp_resource,
		credits_shape_ids, (legacy_s8 far**)credit_shapes);
	waitflag = CREDITS_INITIAL_WAIT;
	sprite_select_render_window_and_clear();
	arrow_shape = credit_shapes[CREDITS_ARROW_INDEX];
	target_x = (legacy_s16)shape2d_get_pos_x(arrow_shape);
	arrow_y = (legacy_s16)shape2d_get_pos_y(arrow_shape);
	arrow_width = LEGACY_S16_WRAP_MUL(
		(legacy_s16)shape2d_get_width(arrow_shape), video_shape_width_scale);
	arrow_height = (legacy_s16)shape2d_get_height(arrow_shape);

	intro_draw_resource_line(credit_resource, credits_title_id, CREDITS_LINE_TEXT,
		CREDITS_TITLE_X, CREDITS_TITLE_Y, credits_title_color, credits_title_shadow_color);
	intro_draw_resource_line(credit_resource, credits_first_logo_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_FIRST_LOGO_X, CREDITS_FIRST_LOGO_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_second_logo_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_SECOND_LOGO_X, CREDITS_SECOND_LOGO_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_design_heading_id, CREDITS_LINE_TEXT,
		CREDITS_LEFT_COLUMN_X, CREDITS_DESIGN_HEADING_Y, credits_design_heading_color, credits_design_heading_shadow_color);
	intro_draw_resource_line(credit_resource, credits_first_designer_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_FIRST_DESIGNER_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_second_designer_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_SECOND_DESIGNER_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_third_designer_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_THIRD_DESIGNER_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_fourth_designer_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_FOURTH_DESIGNER_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_fifth_designer_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_FIFTH_DESIGNER_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_music_heading_id, CREDITS_LINE_TEXT,
		CREDITS_LEFT_COLUMN_X, CREDITS_MUSIC_HEADING_Y, credits_music_heading_color, credits_music_heading_shadow_color);
	intro_draw_resource_line(credit_resource, credits_first_musician_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_FIRST_MUSICIAN_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_second_musician_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_SECOND_MUSICIAN_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_third_musician_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_THIRD_MUSICIAN_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_production_heading_id, CREDITS_LINE_TEXT,
		CREDITS_RIGHT_COLUMN_X, CREDITS_PRODUCTION_HEADING_Y, credits_production_heading_color, credits_production_heading_shadow_color);
	intro_draw_resource_line(credit_resource, credits_producer_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_PRODUCER_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_opponent_heading_id, CREDITS_LINE_TEXT,
		CREDITS_RIGHT_COLUMN_X, CREDITS_OPPONENT_HEADING_Y, credits_production_heading_color, credits_production_heading_shadow_color);
	intro_draw_resource_line(credit_resource, credits_first_opponent_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_FIRST_OPPONENT_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_second_opponent_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_SECOND_OPPONENT_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_art_heading_id, CREDITS_LINE_TEXT,
		CREDITS_RIGHT_COLUMN_X, CREDITS_ART_HEADING_Y, credits_art_heading_color, credits_art_heading_shadow_color);
	intro_draw_resource_line(credit_resource, credits_first_artist_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_FIRST_ARTIST_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_second_artist_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_SECOND_ARTIST_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_third_artist_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_THIRD_ARTIST_Y, credits_text_color, credits_text_shadow_color);
	intro_draw_resource_line(credit_resource, credits_fourth_artist_shape_id, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_FOURTH_ARTIST_Y, credits_text_color, credits_text_shadow_color);
	unload_resource(credit_resource);

	(void)sprite_blit_to_video(render_window_sprite, -1);
	sprite_select_screen_compat();
	(void)timer_get_delta_alt();
	arrow_x = CREDITS_ARROW_START_X;
	input = 0;
	for (;;) {
		frame_elapsed = (legacy_s16)timer_get_delta_alt();
		arrow_x = LEGACY_S16_WRAP_SUB(arrow_x,
			LEGACY_S16_WRAP_MUL(frame_elapsed, CREDITS_ARROW_SPEED));
		if (target_x > arrow_x)
			break;
		mouse_draw_opaque_check();
		sprite_copy_image_at(arrow_shape, arrow_x, arrow_y);
		sprite_fill_rect_clipped(LEGACY_S16_WRAP_ADD(arrow_width, arrow_x),
			arrow_y, CREDITS_ARROW_ERASE_WIDTH, arrow_height, 0);
		mouse_draw_transparent_check();
		input = (legacy_s16)input_do_checking(frame_elapsed);
		if (input != 0)
			break;
	}

	arrow_y = (legacy_s16)shape2d_get_pos_y(
		credit_shapes[CREDITS_BACKGROUND_INDEX]);
	animation_target = 0;
	animation_elapsed = 0;
	for (animation_index = CREDITS_FIRST_ANIMATION_INDEX;
		animation_index < CREDITS_ANIMATION_END_INDEX && input == 0;
		animation_index++) {
		sprite_select_render_window();
		sprite_set_target_clip_bounds(0, INTRO_SCREEN_WIDTH, arrow_y,
		INTRO_SCREEN_HEIGHT);
		sprite_clear_target(0);
		sprite_shape_to_1_alt(credit_shapes[animation_index]);
		sprite_select_screen_compat();
		sprite_set_target_clip_bounds(0, INTRO_SCREEN_WIDTH, arrow_y,
		INTRO_SCREEN_HEIGHT);
		mouse_draw_opaque_check();
		sprite_putimage(render_window_sprite->sprite_bitmapptr);
		mouse_draw_transparent_check();
		animation_target = LEGACY_S16_WRAP_ADD(animation_target,
			CREDITS_ANIMATION_INTERVAL);
		while (animation_target > animation_elapsed) {
			frame_elapsed = (legacy_s16)timer_get_delta_alt();
			input = (legacy_s16)input_do_checking(frame_elapsed);
			animation_elapsed = LEGACY_S16_WRAP_ADD(
				animation_elapsed, frame_elapsed);
		}
	}

	sprite_set_target_clip_bounds(0, INTRO_SCREEN_WIDTH, 0, INTRO_SCREEN_HEIGHT);
	mouse_draw_opaque_check();
	sprite_clear_shape(render_window_sprite->sprite_bitmapptr);
	sprite_select_render_window();
	sprite_set_target_clip_bounds(0, INTRO_SCREEN_WIDTH, arrow_y,
		INTRO_SCREEN_HEIGHT);
	sprite_clear_target(0);
	sprite_shape_to_1_alt(credit_shapes[CREDITS_BACKGROUND_INDEX]);
	sprite_shape_to_1_alt(credit_shapes[CREDITS_CLOSING_INDEX]);
	if (sprite_blit_to_video(render_window_sprite, 0) != 0)
		return 1;
	return input_repeat_check(CREDITS_END_INPUT_DELAY) != 0;
}
