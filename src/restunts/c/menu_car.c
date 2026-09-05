#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

static void car_menu_draw_standard_button(legacy_s8 far* text,
	legacy_u16 button_index)
{
	draw_button(text,
		LEGACY_S16_WRAP_ADD(carmenu_buttons[0].x1, 1),
		LEGACY_S16_WRAP_ADD(carmenu_buttons[button_index].y1, 1),
		0x56, 0x10, word_407F4, word_407F6, word_407F8, 0);
}

void run_car_menu(legacy_s8* car_id, legacy_s8* material, legacy_s8* transmission,
	legacy_u16 opponent_type)
{
	legacy_s8 car_ids[32][5];
	legacy_s8 swap_id[5];
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
	transformed.shapeptr = &game3dshapes[124];
	transformed.rotvec.x = 0;
	transformed.rotvec.y = 0;
	transformed.unk = 0x7530U;
	slow_video_mgmt_copy = slow_video_mgmt;
	if (slow_video_mgmt_copy != 0) {
		transformed.rectptr = &current_rect;
		transformed.ts_flags = 8;
	} else {
		transformed.rectptr = 0;
		transformed.ts_flags = 0;
	}

	ensure_file_exists(2);
	found_path = file_combine_and_find(0, aCar, a_res_0);
	if (found_path == 0)
		return;
	car_count = 0;
	do {
		for (i = 0; i < 4U; i++)
			car_ids[car_count][i] = found_path[i + 3U];
		car_ids[car_count][4] = 0;
		car_count++;
		if (car_count >= 32U)
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
		for (j = 0; j < 4U; j++) {
			if (car_ids[i][j] != car_id[j])
				break;
		}
		if (j == 4U)
			car_index = (legacy_u8)i;
	}

	waitflag = 0x5AU;
	blit_mode = 0xFFU;
	backlights_paint_override = 0x2D;
	selector_resource = file_load_shape2d_fatal(aSdcsel);
	opponent_sprite = 0;
	if (opponent_type == 0)
		miscptr = file_load_resfile(aMisc_0);

	if (opponent_type != 0) {
		rect_unk16.right = 0xF0;
		if (video_flag5_is0 != 0) {
			opponent_shape = (struct SHAPE2D far*)
				oppresources[(legacy_u16)opponent_type];
			opponent_sprite = sprite_make_wnd(
				shape2d_get_width(opponent_shape),
				shape2d_get_height(opponent_shape), 0x0FU);
			setup_mcgawnd2();
			sprite_clear_1_color(0);
			sprite_putimage_transparent(opponent_shape, 0, 0);
			sprite_clear_shape_alt(opponent_sprite->sprite_bitmapptr,
				0, 0);
		}
	} else {
		rect_unk16.right = 0x140;
	}

	previous_car_index = 0xFFU;
	rotation = 0;
	selected = 0;
	sub_29772();
	rotation_delta = 0;
	previous_selected = 0xFFU;
	set_projection(0x24, 0x11, 0x140, 0x64);
	(void)timer_get_delta_alt();
	render_window_sprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);

	for (;;) {
	render_deferred = 0;
	if (previous_car_index != car_index) {
		if (previous_car_index != 0xFFU) {
			unload_resource(car_resource);
			shape3d_free_car_shapes();
		}

		shape3d_load_car_shapes(car_ids[car_index],
			gameconfig.game_opponentcarid);
		for (i = 0; i < 4U; i++)
			aCarcoun[i + 3U] = car_ids[car_index][i];
		car_resource = (legacy_s8 far*)file_load_resfile(aCarcoun);
		setup_aero_trackdata(car_resource, 0);

		sprite_copy_wnd_to_1_clear();
		draw_button(0, 0, 0x67, 0x140, 0x61,
			word_407F4, word_407F6, word_407F8, 0);
		draw_button(0, 5, 0x6D, 0x46, 0x55,
			word_407F4, word_407F6, word_407F8, 0);
		draw_button(0, 0x52, 0x6D, 0x8C, 0x55,
			word_407F4, word_407F6, word_407F8, 0);
		shape = (struct SHAPE2D far*)locate_shape_fatal(
			selector_resource, aGrap);
		sprite_shape_to_1_alt(shape);

		font_set_fontdef2(fontnptr);
		font_set_unk(0, dialog_fnt_colour);
		font_draw_text(a150, 9, 0x73);
		font_draw_text(a100, 9, 0x87);
		font_draw_text(a50, 9, 0x9B);
		font_draw_text(a0, 9, 0xAF);
		font_draw_text(a02040, 0x1A, 0xB9);
		font_set_fontdef();

		car_menu_draw_standard_button(
			locate_text_res(miscptr, aBdo_0), 0);
		car_menu_draw_standard_button(
			locate_text_res(miscptr, aBnx_0), 1);
		car_menu_draw_standard_button(
			locate_text_res(miscptr, aBla_0), 2);
		transmission_text = locate_text_res(miscptr,
			*transmission != 0 ? aBau : aBma);
		car_menu_draw_standard_button(transmission_text, 3);
		car_menu_draw_standard_button(
			locate_text_res(miscptr, aBco), 4);

		old_frame_rate = (legacy_u16)framespersec;
		framespersec = 0x14;
		init_game_state(-2);
		state.playerstate.car_transmission = 1;
		graph_step = 0;
		for (;;) {
			update_car_speed(1, 0, &state.playerstate, &simd_player);
			speed = (legacy_u16)state.playerstate.car_speed >> 8;
			graph_y = LEGACY_U16_WRAP_SUB(0xB5U,
				(legacy_u16)LEGACY_U32_DIV_OR_ZERO(
					LEGACY_U32_WRAP_MUL(speed, 64UL), 0x96UL));
			if (graph_y < 0x75U)
				break;
			graph_x = LEGACY_U16_WRAP_ADD(
				(legacy_u16)LEGACY_U32_DIV_OR_ZERO(
					LEGACY_U32_WRAP_MUL(0x26UL, graph_step),
					0x320UL),
				0x1CU);
			putpixel_single_maybe(graph_x, graph_y,
				performGraphColor);
			graph_step++;
			if (graph_step >= 0x320U)
				break;
		}
		framespersec = (legacy_s16)old_frame_rate;

		font_set_fontdef2(fontnptr);
		description = locate_text_res(car_resource, aDes_1);
		line_length = 0;
		text_y = 0x74;
		do {
			character = (legacy_u8)*description++;
			if (character == ']') {
				if (line_length != 0) {
					(&resID_byte1)[line_length] = 0;
					font_draw_text(&resID_byte1, 0x58, text_y);
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
		previous_selected = 0xFFU;
		previous_rect.left = 0;
		previous_rect.right = 0x140;
		previous_rect.top = 0;
		previous_rect.bottom = 0xC8;
		car_ready = 0;
		render_phase = 3;
	}

	rotation = LEGACY_S16_WRAP_ADD(rotation, rotation_delta);
	if (render_phase == 0 || render_phase == 3) {
		car_position_angle = (legacy_s16)polarAngle(
			carmenu_carpos.y, carmenu_carpos.z);
		current_rect = slow_video_mgmt_copy != 0 ?
			cliprect_unk : carmenu_cliprect;
		select_cliprect_rotate(0, car_position_angle, 0,
			&carmenu_cliprect, 0);
		if ((legacy_s8)(legacy_u8)*material >=
			(legacy_s8)(legacy_u8)game3dshapes[124].shape3d_numpaints)
			*material = 0;
		transformed.rotvec.z = rotation;
		transformed.material = (legacy_u8)*material;
		transformed_shape_op(&transformed);
		rect_unk16.bottom = previous_car_index == car_index ?
			0x5F : 0xC8;
		(void)rect_intersect(&current_rect, &rect_unk16);
		rect_union(&current_rect, &previous_rect, &union_rect);
		if (render_phase != 3) {
			render_phase = 1;
			render_deferred = 1;
		}
	}

	if (render_deferred == 0 &&
		(render_phase == 1 || render_phase == 3)) {
		render_phase = 0;
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

		if (opponent_type != 0 && previous_car_index != car_index) {
			sprite_copy_wnd_to_1();
			if (video_flag5_is0 == 0) {
				sprite_putimage_transparent(
					(struct SHAPE2D far*)oppresources[
						(legacy_u16)opponent_type], 0xF0, 0);
			} else {
				sprite_putimage_and_alt(
					opponent_sprite->sprite_bitmapptr, 0xF0, 0);
			}
		}

		sprite_copy_2_to_1_2();
		sprite_set_1_size(union_rect.left, union_rect.right,
			union_rect.top, union_rect.bottom);
		mouse_draw_opaque_check();
		if (blit_mode != 0xFEU) {
			(void)sprite_blit_to_video(render_window_sprite,
				LEGACY_S8_FROM_BITS(blit_mode));
			blit_mode = 0xFEU;
		} else {
			sprite_putimage(render_window_sprite->sprite_bitmapptr);
		}
		mouse_draw_transparent_check();
		previous_car_index = car_index;
	}

	if (previous_selected != selected) {
		if (previous_selected != 0xFFU) {
			sprite_copy_2_to_1_2();
			sprite_set_1_size(carmenu_buttons[0].x1,
				LEGACY_S16_FROM_BITS((legacy_u16)(
					(LEGACY_U16_WRAP_ADD(carmenu_buttons[0].x2,
						video_flag2_is1)) &
					(legacy_u16)video_flag3_isFFFF)),
				carmenu_buttons[0].y1,
				LEGACY_S16_WRAP_ADD(carmenu_buttons[4].y2, 1));
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
	menu_update_idle_counter((legacy_u16)rotation_delta, 0x2EE0);
	input = (legacy_u16)input_checking(rotation_delta);
	mouse_hit = (legacy_s16)mouse_multi_hittest(5, carmenu_buttons);
	if (mouse_hit != -1)
		selected = (legacy_u8)mouse_hit;
	if (idle_expired != 0) {
		selected = 0;
		input = 0x0DU;
	}

	if (input == 0)
		continue;
	if (input == 0x4800U) {
		selected = selected == 0 ? 4U : (legacy_u8)(selected - 1U);
		continue;
	}
	if (input == 0x5000U) {
		selected = selected >= 4U ? 0U : (legacy_u8)(selected + 1U);
		continue;
	}
	if (input != 0x0DU && input != 0x1BU && input != 0x20U)
		continue;

	if (selected == 0) {
		if (car_ready == 0)
			continue;
		break;
	} else if (selected == 1) {
		car_index++;
		if (car_index == car_count)
			car_index = 0;
		continue;
	} else if (selected == 2) {
		car_index = car_index == 0 ?
			(legacy_u8)(car_count - 1U) : (legacy_u8)(car_index - 1U);
		continue;
	} else if (selected == 3) {
		*transmission = (legacy_s8)((legacy_u8)*transmission ^ 1U);
		sprite_copy_wnd_to_1();
		transmission_text = locate_text_res(miscptr,
			*transmission != 0 ? aBau_0 : aBma_0);
		car_menu_draw_standard_button(transmission_text, 3);
		sprite_copy_2_to_1_2();
		mouse_draw_opaque_check();
		car_menu_draw_standard_button(transmission_text, 3);
		mouse_draw_transparent_check();
		continue;
	} else if (selected == 4) {
		*material = (legacy_s8)((legacy_u8)*material + 1U);
		render_phase = 3;
		continue;
	} else {
		continue;
	}
	}

	sprite_free_wnd(render_window_sprite);
	unload_resource(car_resource);
	shape3d_free_car_shapes();
	if (opponent_type != 0 && video_flag5_is0 != 0)
		sprite_free_wnd(opponent_sprite);
	if (opponent_type == 0)
		unload_resource(miscptr);
	mmgr_free((legacy_s8 far*)selector_resource);
	mouse_draw_opaque_check();
	for (i = 0; i < 4U; i++)
		car_id[i] = car_ids[car_index][i];
	idle_expired = 0;
}
