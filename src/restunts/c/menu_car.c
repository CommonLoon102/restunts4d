#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

#define GAME_RESOURCE_FILE_INDEX 2
#define CAR_MENU_PLAYER_MODE 0U
#define CAR_MENU_MAXIMUM_CARS 32U
#define CAR_ID_LENGTH 4U
#define CAR_ID_BUFFER_SIZE (CAR_ID_LENGTH + 1U)
#define CAR_RESOURCE_ID_OFFSET 3U
#define CAR_MENU_BUTTON_COUNT 5U
#define CAR_MENU_DONE_BUTTON 0U
#define CAR_MENU_NEXT_BUTTON 1U
#define CAR_MENU_PREVIOUS_BUTTON 2U
#define CAR_MENU_TRANSMISSION_BUTTON 3U
#define CAR_MENU_COLOR_BUTTON 4U
#define CAR_MENU_NO_SELECTION 255U
#define CAR_MENU_INITIAL_BLIT_MODE 255U
#define CAR_MENU_REFRESH_BLIT_MODE 254U
#define CAR_MENU_WAIT_TICKS 90
#define CAR_MENU_IDLE_LIMIT_TICKS 12000
#define CAR_MENU_BUTTON_WIDTH 86
#define CAR_MENU_BUTTON_HEIGHT 16
#define CAR_MENU_SCREEN_WIDTH 320U
#define CAR_MENU_SCREEN_HEIGHT 200U
#define CAR_MENU_OPPONENT_PANEL_X 240
#define CAR_MENU_OPPONENT_PANEL_RIGHT 240
#define CAR_MENU_TRANSPARENT_COLOR 15U
#define CAR_MENU_PROJECTION_X_SCALE 36
#define CAR_MENU_PROJECTION_Y_SCALE 17
#define CAR_MENU_PROJECTION_HEIGHT 100
#define CAR_MENU_TRANSFORM_DISTANCE 30000U
#define CAR_MENU_BACKLIGHT_PAINT 45
#define CAR_MENU_CAR_SHAPE_INDEX 124U
#define CAR_MENU_CLIPPED_TRANSFORM_FLAG 8U
#define CAR_MENU_BACKGROUND_Y 103
#define CAR_MENU_BACKGROUND_HEIGHT 97
#define CAR_MENU_LEFT_PANEL_X 5
#define CAR_MENU_RIGHT_PANEL_X 82
#define CAR_MENU_PANEL_Y 109
#define CAR_MENU_LEFT_PANEL_WIDTH 70
#define CAR_MENU_RIGHT_PANEL_WIDTH 140
#define CAR_MENU_PANEL_HEIGHT 85
#define CAR_MENU_GRAPH_LABEL_X 9
#define CAR_MENU_GRAPH_LABEL_150_Y 115
#define CAR_MENU_GRAPH_LABEL_100_Y 135
#define CAR_MENU_GRAPH_LABEL_50_Y 155
#define CAR_MENU_GRAPH_LABEL_0_Y 175
#define CAR_MENU_GRAPH_AXIS_X 26
#define CAR_MENU_GRAPH_AXIS_Y 185
#define CAR_MENU_GRAPH_FRAME_RATE 20
#define CAR_MENU_GRAPH_BASELINE_Y 181U
#define CAR_MENU_GRAPH_MINIMUM_Y 117U
#define CAR_MENU_GRAPH_SPEED_SCALE 64UL
#define CAR_MENU_GRAPH_SPEED_DIVISOR 150UL
#define CAR_MENU_GRAPH_WIDTH 38UL
#define CAR_MENU_GRAPH_STEPS 800U
#define CAR_MENU_GRAPH_FIRST_X 28U
#define CAR_MENU_DESCRIPTION_X 88
#define CAR_MENU_DESCRIPTION_FIRST_Y 116
#define CAR_MENU_PREVIEW_GAME_STATE (-2)
#define CAR_MENU_PREVIEW_TRANSMISSION 1
#define CAR_MENU_FULL_CLIP_BOTTOM 200
#define CAR_MENU_CAR_CLIP_BOTTOM 95
#define CAR_RENDER_IDLE_PHASE 0U
#define CAR_RENDER_DRAW_PHASE 1U
#define CAR_RENDER_START_PHASE 3U
#define TRANSMISSION_MODE_MASK 1U

static void car_menu_draw_standard_button(legacy_s8 far* text,
	legacy_u16 button_index)
{
	draw_button(text,
		LEGACY_S16_WRAP_ADD(carmenu_buttons[0].x1, 1),
		LEGACY_S16_WRAP_ADD(carmenu_buttons[button_index].y1, 1),
		CAR_MENU_BUTTON_WIDTH, CAR_MENU_BUTTON_HEIGHT,
		word_407F4, word_407F6, word_407F8, 0);
}

void run_car_menu(legacy_s8* car_id, legacy_s8* material, legacy_s8* transmission,
	legacy_u16 opponent_type)
{
	legacy_s8 car_ids[CAR_MENU_MAXIMUM_CARS][CAR_ID_BUFFER_SIZE];
	legacy_s8 swap_id[CAR_ID_BUFFER_SIZE];
	const legacy_s8* found_path;
	legacy_s8 far* car_resource;
	legacy_s8 far* description;
	legacy_s8 far* transmission_text;
	void far* selector_resource;
	struct SHAPE2D far* opponent_shape;
	struct SHAPE2D far* shape;
	struct SPRITE far* opponent_sprite;
	struct TRANSFORMEDSHAPE3D transformed;
	struct RECTANGLE current_rect;
	struct RECTANGLE previous_rect;
	struct RECTANGLE union_rect;
	legacy_u8 car_count;
	legacy_u8 car_index;
	legacy_u8 previous_car_index;
	legacy_u8 selected;
	legacy_u8 previous_selected;
	legacy_u8 blit_mode;
	legacy_u8 render_phase;
	legacy_u8 car_ready;
	legacy_u8 render_deferred;
	legacy_u8 character;
	legacy_u16 i;
	legacy_u16 j;
	legacy_u16 line_length;
	legacy_u16 old_frame_rate;
	legacy_u16 input;
	legacy_u16 speed;
	legacy_u16 graph_x;
	legacy_u16 graph_y;
	legacy_u16 graph_step;
	legacy_s16 car_position_angle;
	legacy_s16 rotation;
	legacy_s16 rotation_delta;
	legacy_s16 text_y;
	legacy_s16 mouse_hit;

	transformed.pos = carmenu_carpos;
	transformed.shapeptr = &game3dshapes[CAR_MENU_CAR_SHAPE_INDEX];
	transformed.rotvec.x = 0;
	transformed.rotvec.y = 0;
	transformed.unk = CAR_MENU_TRANSFORM_DISTANCE;
	slow_video_mgmt_copy = slow_video_mgmt;
	if (slow_video_mgmt_copy != 0) {
		transformed.rectptr = &current_rect;
		transformed.ts_flags = CAR_MENU_CLIPPED_TRANSFORM_FLAG;
	} else {
		transformed.rectptr = 0;
		transformed.ts_flags = 0;
	}

	ensure_file_exists(GAME_RESOURCE_FILE_INDEX);
	found_path = file_combine_and_find(0, aCar, a_res_0);
	if (found_path == 0)
		return;
	car_count = 0;
	do {
		for (i = 0; i < CAR_ID_LENGTH; i++)
			car_ids[car_count][i] = found_path[i + CAR_RESOURCE_ID_OFFSET];
		car_ids[car_count][CAR_ID_LENGTH] = 0;
		car_count++;
		if (car_count >= CAR_MENU_MAXIMUM_CARS)
			break;
		found_path = file_find_next_alt();
	} while (found_path != 0);

	for (i = 0; i + 1U < car_count; i++) {
		for (j = i + 1U; j < car_count; j++) {
			if (strcmp(car_ids[i], car_ids[j]) > 0) {
				strcpy(swap_id, car_ids[i]);
				strcpy(car_ids[i], car_ids[j]);
				strcpy(car_ids[j], swap_id);
			}
		}
	}

	car_index = 0;
	for (i = 0; i < car_count; i++) {
		for (j = 0; j < CAR_ID_LENGTH; j++) {
			if (car_ids[i][j] != car_id[j])
				break;
		}
		if (j == CAR_ID_LENGTH)
			car_index = (legacy_u8)i;
	}

	waitflag = CAR_MENU_WAIT_TICKS;
	blit_mode = CAR_MENU_INITIAL_BLIT_MODE;
	backlights_paint_override = CAR_MENU_BACKLIGHT_PAINT;
	selector_resource = file_load_shape2d_fatal(aSdcsel);
	opponent_sprite = 0;
	if (opponent_type == CAR_MENU_PLAYER_MODE)
		miscptr = file_load_resfile(aMisc_0);

	if (opponent_type != CAR_MENU_PLAYER_MODE) {
		rect_unk16.right = CAR_MENU_OPPONENT_PANEL_RIGHT;
		if (video_flag5_is0 != 0) {
			opponent_shape = (struct SHAPE2D far*)
				oppresources[(legacy_u16)opponent_type];
			opponent_sprite = sprite_make_wnd(
				shape2d_get_width(opponent_shape),
				shape2d_get_height(opponent_shape),
				CAR_MENU_TRANSPARENT_COLOR);
			setup_mcgawnd2();
			sprite_clear_1_color(0);
			sprite_putimage_transparent(opponent_shape, 0, 0);
			sprite_clear_shape_alt(opponent_sprite->sprite_bitmapptr,
				0, 0);
		}
	} else {
		rect_unk16.right = CAR_MENU_SCREEN_WIDTH;
	}

	previous_car_index = CAR_MENU_NO_SELECTION;
	rotation = 0;
	selected = CAR_MENU_DONE_BUTTON;
	sub_29772();
	rotation_delta = 0;
	previous_selected = CAR_MENU_NO_SELECTION;
	set_projection(CAR_MENU_PROJECTION_X_SCALE,
		CAR_MENU_PROJECTION_Y_SCALE, CAR_MENU_SCREEN_WIDTH,
		CAR_MENU_PROJECTION_HEIGHT);
	(void)timer_get_delta_alt();
	render_window_sprite = sprite_make_wnd(CAR_MENU_SCREEN_WIDTH,
		CAR_MENU_SCREEN_HEIGHT, CAR_MENU_TRANSPARENT_COLOR);

	for (;;) {
	render_deferred = 0;
	if (previous_car_index != car_index) {
		if (previous_car_index != CAR_MENU_NO_SELECTION) {
			unload_resource(car_resource);
			shape3d_free_car_shapes();
		}

		shape3d_load_car_shapes(car_ids[car_index],
			gameconfig.game_opponentcarid);
		for (i = 0; i < CAR_ID_LENGTH; i++)
			aCarcoun[i + CAR_RESOURCE_ID_OFFSET] = car_ids[car_index][i];
		car_resource = (legacy_s8 far*)file_load_resfile(aCarcoun);
		setup_aero_trackdata(car_resource, 0);

		sprite_copy_wnd_to_1_clear();
		draw_button(0, 0, CAR_MENU_BACKGROUND_Y, CAR_MENU_SCREEN_WIDTH,
			CAR_MENU_BACKGROUND_HEIGHT,
			word_407F4, word_407F6, word_407F8, 0);
		draw_button(0, CAR_MENU_LEFT_PANEL_X, CAR_MENU_PANEL_Y,
			CAR_MENU_LEFT_PANEL_WIDTH, CAR_MENU_PANEL_HEIGHT,
			word_407F4, word_407F6, word_407F8, 0);
		draw_button(0, CAR_MENU_RIGHT_PANEL_X, CAR_MENU_PANEL_Y,
			CAR_MENU_RIGHT_PANEL_WIDTH, CAR_MENU_PANEL_HEIGHT,
			word_407F4, word_407F6, word_407F8, 0);
		shape = (struct SHAPE2D far*)locate_shape_fatal(
			selector_resource, aGrap);
		sprite_shape_to_1_alt(shape);

		font_set_fontdef2(fontnptr);
		font_set_unk(0, dialog_fnt_colour);
		font_draw_text(a150, CAR_MENU_GRAPH_LABEL_X,
			CAR_MENU_GRAPH_LABEL_150_Y);
		font_draw_text(a100, CAR_MENU_GRAPH_LABEL_X,
			CAR_MENU_GRAPH_LABEL_100_Y);
		font_draw_text(a50, CAR_MENU_GRAPH_LABEL_X,
			CAR_MENU_GRAPH_LABEL_50_Y);
		font_draw_text(a0, CAR_MENU_GRAPH_LABEL_X,
			CAR_MENU_GRAPH_LABEL_0_Y);
		font_draw_text(a02040, CAR_MENU_GRAPH_AXIS_X,
			CAR_MENU_GRAPH_AXIS_Y);
		font_set_fontdef();

		car_menu_draw_standard_button(
			locate_text_res(miscptr, aBdo_0), CAR_MENU_DONE_BUTTON);
		car_menu_draw_standard_button(
			locate_text_res(miscptr, aBnx_0), CAR_MENU_NEXT_BUTTON);
		car_menu_draw_standard_button(
			locate_text_res(miscptr, aBla_0), CAR_MENU_PREVIOUS_BUTTON);
		transmission_text = locate_text_res(miscptr,
			*transmission != 0 ? aBau : aBma);
		car_menu_draw_standard_button(transmission_text,
			CAR_MENU_TRANSMISSION_BUTTON);
		car_menu_draw_standard_button(
			locate_text_res(miscptr, aBco), CAR_MENU_COLOR_BUTTON);

		old_frame_rate = (legacy_u16)framespersec;
		framespersec = CAR_MENU_GRAPH_FRAME_RATE;
		init_game_state(CAR_MENU_PREVIEW_GAME_STATE);
		state.playerstate.car_transmission = CAR_MENU_PREVIEW_TRANSMISSION;
		graph_step = 0;
		for (;;) {
			update_car_speed(INPUT_ACCELERATE_FLAG, 0,
				&state.playerstate, &simd_player);
			speed = (legacy_u16)state.playerstate.car_speed >> 8;
			graph_y = LEGACY_U16_WRAP_SUB(CAR_MENU_GRAPH_BASELINE_Y,
				(legacy_u16)LEGACY_U32_DIV_OR_ZERO(
					LEGACY_U32_WRAP_MUL(speed,
						CAR_MENU_GRAPH_SPEED_SCALE),
					CAR_MENU_GRAPH_SPEED_DIVISOR));
			if (graph_y < CAR_MENU_GRAPH_MINIMUM_Y)
				break;
			graph_x = LEGACY_U16_WRAP_ADD(
				(legacy_u16)LEGACY_U32_DIV_OR_ZERO(
					LEGACY_U32_WRAP_MUL(CAR_MENU_GRAPH_WIDTH,
						graph_step),
					CAR_MENU_GRAPH_STEPS),
				CAR_MENU_GRAPH_FIRST_X);
			putpixel_single_maybe(graph_x, graph_y,
				performGraphColor);
			graph_step++;
			if (graph_step >= CAR_MENU_GRAPH_STEPS)
				break;
		}
		framespersec = (legacy_s16)old_frame_rate;

		font_set_fontdef2(fontnptr);
		description = locate_text_res(car_resource, aDes_1);
		line_length = 0;
		text_y = CAR_MENU_DESCRIPTION_FIRST_Y;
		do {
			character = (legacy_u8)*description++;
			if (character == ']') {
				if (line_length != 0) {
					(&resID_byte1)[line_length] = 0;
					font_draw_text(&resID_byte1,
						CAR_MENU_DESCRIPTION_X, text_y);
				}
				line_length = 0;
				text_y = LEGACY_S16_WRAP_ADD(text_y,
					fontdef_unk_0E);
			} else {
				(&resID_byte1)[line_length++] = (legacy_s8)character;
			}
		} while (*description != 0);
		font_set_fontdef();
		(void)timer_get_delta_alt();
		previous_selected = CAR_MENU_NO_SELECTION;
		previous_rect.left = 0;
		previous_rect.right = CAR_MENU_SCREEN_WIDTH;
		previous_rect.top = 0;
		previous_rect.bottom = CAR_MENU_SCREEN_HEIGHT;
		car_ready = 0;
		render_phase = CAR_RENDER_START_PHASE;
	}

	rotation = LEGACY_S16_WRAP_ADD(rotation, rotation_delta);
	if (render_phase == CAR_RENDER_IDLE_PHASE ||
		render_phase == CAR_RENDER_START_PHASE) {
		car_position_angle = (legacy_s16)polarAngle(
			carmenu_carpos.y, carmenu_carpos.z);
		current_rect = slow_video_mgmt_copy != 0 ?
			cliprect_unk : carmenu_cliprect;
		select_cliprect_rotate(0, car_position_angle, 0,
			&carmenu_cliprect, 0);
		if ((legacy_s8)(legacy_u8)*material >=
			(legacy_s8)(legacy_u8)
				game3dshapes[CAR_MENU_CAR_SHAPE_INDEX].shape3d_numpaints)
			*material = 0;
		transformed.rotvec.z = rotation;
		transformed.material = (legacy_u8)*material;
		transformed_shape_op(&transformed);
		rect_unk16.bottom = previous_car_index == car_index ?
			CAR_MENU_CAR_CLIP_BOTTOM : CAR_MENU_FULL_CLIP_BOTTOM;
		(void)rect_intersect(&current_rect, &rect_unk16);
		rect_union(&current_rect, &previous_rect, &union_rect);
		if (render_phase != CAR_RENDER_START_PHASE) {
			render_phase = CAR_RENDER_DRAW_PHASE;
			render_deferred = 1;
		}
	}

	if (render_deferred == 0 &&
		(render_phase == CAR_RENDER_DRAW_PHASE ||
			render_phase == CAR_RENDER_START_PHASE)) {
		render_phase = CAR_RENDER_IDLE_PHASE;
		car_ready = 1;
		sprite_copy_wnd_to_1();
		sprite_set_1_size(union_rect.left, union_rect.right,
			union_rect.top, union_rect.bottom);
		sprite_putimage((struct SHAPE2D far*)locate_shape_fatal(
			selector_resource, aStop_1));
		get_a_poly_info();
		sprite_copy_wnd_to_1();
		sprite_set_1_size(union_rect.left, union_rect.right,
			union_rect.top, union_rect.bottom);
		previous_rect = current_rect;

		if (opponent_type != CAR_MENU_PLAYER_MODE &&
			previous_car_index != car_index) {
			sprite_copy_wnd_to_1();
			if (video_flag5_is0 == 0) {
				sprite_putimage_transparent(
					(struct SHAPE2D far*)oppresources[
						(legacy_u16)opponent_type],
					CAR_MENU_OPPONENT_PANEL_X, 0);
			} else {
				sprite_putimage_and_alt(
					opponent_sprite->sprite_bitmapptr,
					CAR_MENU_OPPONENT_PANEL_X, 0);
			}
		}

		sprite_copy_2_to_1_2();
		sprite_set_1_size(union_rect.left, union_rect.right,
			union_rect.top, union_rect.bottom);
		mouse_draw_opaque_check();
		if (blit_mode != CAR_MENU_REFRESH_BLIT_MODE) {
			(void)sprite_blit_to_video(render_window_sprite,
				LEGACY_S8_FROM_BITS(blit_mode));
			blit_mode = CAR_MENU_REFRESH_BLIT_MODE;
		} else {
			sprite_putimage(render_window_sprite->sprite_bitmapptr);
		}
		mouse_draw_transparent_check();
		previous_car_index = car_index;
	}

	if (previous_selected != selected) {
		if (previous_selected != CAR_MENU_NO_SELECTION) {
			sprite_copy_2_to_1_2();
			sprite_set_1_size(carmenu_buttons[0].x1,
				LEGACY_S16_FROM_BITS((legacy_u16)(
					(LEGACY_U16_WRAP_ADD(carmenu_buttons[0].x2,
						video_flag2_is1)) &
					(legacy_u16)video_flag3_isFFFF)),
				carmenu_buttons[0].y1,
				LEGACY_S16_WRAP_ADD(
					carmenu_buttons[CAR_MENU_COLOR_BUTTON].y2, 1));
			mouse_draw_opaque_check();
			sprite_putimage(render_window_sprite->sprite_bitmapptr);
			mouse_draw_transparent_check();
			sprite_copy_2_to_1_2();
		}
		sub_29772();
		previous_selected = selected;
	}

	sprite_copy_2_to_1_2();
	rotation_delta = (legacy_s16)mouse_timer_sprite_unk(selected,
		carmenu_buttons, word_407CE, word_407D0);
	menu_update_idle_counter((legacy_u16)rotation_delta,
		CAR_MENU_IDLE_LIMIT_TICKS);
	input = (legacy_u16)input_checking(rotation_delta);
	mouse_hit = (legacy_s16)mouse_multi_hittest(CAR_MENU_BUTTON_COUNT,
		carmenu_buttons);
	if (mouse_hit != -1)
		selected = (legacy_u8)mouse_hit;
	if (idle_expired != 0) {
		selected = CAR_MENU_DONE_BUTTON;
		input = KEY_ENTER;
	}

	if (input == 0)
		continue;
	if (input == KEY_UP) {
		selected = selected == CAR_MENU_DONE_BUTTON ?
			CAR_MENU_COLOR_BUTTON : (legacy_u8)(selected - 1U);
		continue;
	}
	if (input == KEY_DOWN) {
		selected = selected >= CAR_MENU_COLOR_BUTTON ?
			CAR_MENU_DONE_BUTTON : (legacy_u8)(selected + 1U);
		continue;
	}
	if (input != KEY_ENTER && input != KEY_ESCAPE && input != KEY_SPACE)
		continue;

	if (selected == CAR_MENU_DONE_BUTTON) {
		if (car_ready == 0)
			continue;
		break;
	} else if (selected == CAR_MENU_NEXT_BUTTON) {
		car_index++;
		if (car_index == car_count)
			car_index = 0;
		continue;
	} else if (selected == CAR_MENU_PREVIOUS_BUTTON) {
		car_index = car_index == 0 ?
			(legacy_u8)(car_count - 1U) : (legacy_u8)(car_index - 1U);
		continue;
	} else if (selected == CAR_MENU_TRANSMISSION_BUTTON) {
		*transmission = (legacy_s8)((legacy_u8)*transmission ^
			TRANSMISSION_MODE_MASK);
		sprite_copy_wnd_to_1();
		transmission_text = locate_text_res(miscptr,
			*transmission != 0 ? aBau_0 : aBma_0);
		car_menu_draw_standard_button(transmission_text,
			CAR_MENU_TRANSMISSION_BUTTON);
		sprite_copy_2_to_1_2();
		mouse_draw_opaque_check();
		car_menu_draw_standard_button(transmission_text,
			CAR_MENU_TRANSMISSION_BUTTON);
		mouse_draw_transparent_check();
		continue;
	} else if (selected == CAR_MENU_COLOR_BUTTON) {
		*material = (legacy_s8)((legacy_u8)*material + 1U);
		render_phase = CAR_RENDER_START_PHASE;
		continue;
	} else {
		continue;
	}
	}

	sprite_free_wnd(render_window_sprite);
	unload_resource(car_resource);
	shape3d_free_car_shapes();
	if (opponent_type != CAR_MENU_PLAYER_MODE && video_flag5_is0 != 0)
		sprite_free_wnd(opponent_sprite);
	if (opponent_type == CAR_MENU_PLAYER_MODE)
		unload_resource(miscptr);
	mmgr_free((legacy_s8 far*)selector_resource);
	mouse_draw_opaque_check();
	for (i = 0; i < CAR_ID_LENGTH; i++)
		car_id[i] = car_ids[car_index][i];
	idle_expired = 0;
}
