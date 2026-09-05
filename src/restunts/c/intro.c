#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "resource.h"
#include "shape2d.h"

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
#define CREDITS_LINE_SHAPE 0
#define CREDITS_LINE_TEXT 1
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

static void far* ui_temp_resource;


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
	waitflag = shape2d_get_pos_y(shape) != 0 ?
		INTRO_PRODUCTION_RAISED_WAIT : INTRO_DEFAULT_PAGE_WAIT;

	shape = (struct SHAPE2D far*)locate_shape_fatal(
		(legacy_s8 far*)ui_temp_resource, "prod");
	sprite_shape_to_1_alt(shape);
	result = sprite_blit_to_video(render_window_sprite, -1);
	if (result == 0)
		result = input_repeat_check(INTRO_PAGE_INPUT_DELAY);

	if (result == 0) {
		sprite_copy_wnd_to_1_clear();
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

	credit_resource = (legacy_s8 far*)file_load_resfile(aCred);
	locate_many_resources((legacy_s8 far*)ui_temp_resource,
		aArowarrwarw1ar, (legacy_s8 far**)credit_shapes);
	waitflag = CREDITS_INITIAL_WAIT;
	sprite_copy_wnd_to_1_clear();
	arrow_shape = credit_shapes[CREDITS_ARROW_INDEX];
	target_x = (legacy_s16)shape2d_get_pos_x(arrow_shape);
	arrow_y = (legacy_s16)shape2d_get_pos_y(arrow_shape);
	arrow_width = LEGACY_S16_WRAP_MUL(
		(legacy_s16)shape2d_get_width(arrow_shape), video_flag1_is1);
	arrow_height = (legacy_s16)shape2d_get_height(arrow_shape);

	intro_draw_resource_line(credit_resource, aCre, CREDITS_LINE_TEXT,
		CREDITS_TITLE_X, CREDITS_TITLE_Y, word_407D8, word_407DA);
	intro_draw_resource_line(credit_resource, aGds0, CREDITS_LINE_SHAPE,
		CREDITS_FIRST_LOGO_X, CREDITS_FIRST_LOGO_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGds1, CREDITS_LINE_SHAPE,
		CREDITS_SECOND_LOGO_X, CREDITS_SECOND_LOGO_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aDes, CREDITS_LINE_TEXT,
		CREDITS_LEFT_COLUMN_X, CREDITS_DESIGN_HEADING_Y, word_407DC, word_407DE);
	intro_draw_resource_line(credit_resource, aGdon, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_FIRST_DESIGNER_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGkev, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_SECOND_DESIGNER_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGbra, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_THIRD_DESIGNER_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGrob, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_FOURTH_DESIGNER_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGsta, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_FIFTH_DESIGNER_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aMus, CREDITS_LINE_TEXT,
		CREDITS_LEFT_COLUMN_X, CREDITS_MUSIC_HEADING_Y, word_407E8, word_407EA);
	intro_draw_resource_line(credit_resource, aGmsy, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_FIRST_MUSICIAN_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGkri, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_SECOND_MUSICIAN_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGbri, CREDITS_LINE_SHAPE,
		CREDITS_LEFT_COLUMN_X, CREDITS_THIRD_MUSICIAN_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aPro, CREDITS_LINE_TEXT,
		CREDITS_RIGHT_COLUMN_X, CREDITS_PRODUCTION_HEADING_Y, word_407E0, word_407E2);
	intro_draw_resource_line(credit_resource, aGkev_0, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_PRODUCER_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aOpr, CREDITS_LINE_TEXT,
		CREDITS_RIGHT_COLUMN_X, CREDITS_OPPONENT_HEADING_Y, word_407E0, word_407E2);
	intro_draw_resource_line(credit_resource, aGbra_0, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_FIRST_OPPONENT_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGric, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_SECOND_OPPONENT_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aArt, CREDITS_LINE_TEXT,
		CREDITS_RIGHT_COLUMN_X, CREDITS_ART_HEADING_Y, word_407E4, word_407E6);
	intro_draw_resource_line(credit_resource, aGmsm, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_FIRST_ARTIST_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGdav, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_SECOND_ARTIST_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGnic, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_THIRD_ARTIST_Y, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGkev_1, CREDITS_LINE_SHAPE,
		CREDITS_RIGHT_COLUMN_X, CREDITS_FOURTH_ARTIST_Y, word_407D4, word_407D6);
	unload_resource(credit_resource);

	(void)sprite_blit_to_video(render_window_sprite, -1);
	sprite_copy_2_to_1_2();
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
		sprite_putimage_and_alt(arrow_shape, arrow_x, arrow_y);
		sprite_1_unk2(LEGACY_S16_WRAP_ADD(arrow_width, arrow_x),
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
		sprite_copy_wnd_to_1();
		sprite_set_1_size(0, INTRO_SCREEN_WIDTH, arrow_y,
		INTRO_SCREEN_HEIGHT);
		sprite_clear_1_color(0);
		sprite_shape_to_1_alt(credit_shapes[animation_index]);
		sprite_copy_2_to_1_2();
		sprite_set_1_size(0, INTRO_SCREEN_WIDTH, arrow_y,
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

	sprite_set_1_size(0, INTRO_SCREEN_WIDTH, 0, INTRO_SCREEN_HEIGHT);
	mouse_draw_opaque_check();
	sprite_clear_shape(render_window_sprite->sprite_bitmapptr);
	sprite_copy_wnd_to_1();
	sprite_set_1_size(0, INTRO_SCREEN_WIDTH, arrow_y,
		INTRO_SCREEN_HEIGHT);
	sprite_clear_1_color(0);
	sprite_shape_to_1_alt(credit_shapes[CREDITS_BACKGROUND_INDEX]);
	sprite_shape_to_1_alt(credit_shapes[CREDITS_CLOSING_INDEX]);
	if (sprite_blit_to_video(render_window_sprite, 0) != 0)
		return 1;
	return input_repeat_check(CREDITS_END_INPUT_DELAY) != 0;
}
