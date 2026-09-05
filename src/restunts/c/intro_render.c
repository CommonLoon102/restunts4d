#include "frame_internal.h"

#define TRACK_OBJECT_COUNT 215U

/*
 * In the original dseg, sceneshapes2 immediately follows trkObjectList.
 * Some track objects store overlay indices into that combined legacy table,
 * so indices beyond trkObjectList intentionally address sceneshapes2.
 */
static legacy_s16 intro_shift_position(legacy_s32 position,
	legacy_s16 camera)
{
	return LEGACY_S16_WRAP_SUB(position_to_word(position), camera);
}

static void intro_draw_transformed_shape(
	struct TRANSFORMEDSHAPE3D* transformed,
	struct RECTANGLE* shape_rect, legacy_s16 rotation_z)
{
	if (slow_video_mgmt_copy != 0) {
		transformed->rectptr = shape_rect;
		transformed->ts_flags = 0x0C;
	} else {
		transformed->ts_flags = 4;
	}
	transformed->rotvec.x = 0;
	transformed->rotvec.y = 0;
	transformed->rotvec.z = rotation_z;
	transformed->unk = 0x400;
	transformed->material = 0;
	transformed_shape_op(transformed);
}

static void intro_op_impl(legacy_s16 camera_x, legacy_s16 camera_y, legacy_s16 camera_z,
	legacy_s16 rotate_y,
	legacy_s16 rotate_x, legacy_s16 draw_car, legacy_s16 primary_logo, struct VECTOR* stars,
	struct POINT2D* previous_points, legacy_s16* previous_point_count,
	struct RECTANGLE* previous_rect, struct RECTANGLE* shape_rect,
	struct RECTANGLE* combined_rect)
{
	struct TRANSFORMEDSHAPE3D transformed;
	struct VECTOR translated;
	struct VECTOR projected;
	struct POINT2D point;
	struct RECTANGLE current_shape_rect;
	struct RECTANGLE point_rect;
	struct RECTANGLE redraw_rect;
	legacy_u16 old_point_count;
	legacy_u16 new_point_count;
	legacy_u16 i;

	current_shape_rect = cliprect_unk;
	select_cliprect_rotate(0, rotate_x, rotate_y, &intro_cliprect, 0);
	transformed.shapeptr = primary_logo != 0 ? &logoshape : &logo2shape;
	transformed.pos.x = LEGACY_S16_WRAP_SUB(0x400, camera_x);
	transformed.pos.y = LEGACY_S16_WRAP_NEGATE(camera_y);
	transformed.pos.z = LEGACY_S16_WRAP_SUB(0x400, camera_z);
	intro_draw_transformed_shape(&transformed, &current_shape_rect, 0);

	if (draw_car != 0) {
		transformed.pos.x = intro_shift_position(
			(legacy_s32)state.opponentstate.car_posWorld1.lx,
			(legacy_s16)camera_x);
		transformed.pos.y = intro_shift_position(
			(legacy_s32)state.opponentstate.car_posWorld1.ly,
			(legacy_s16)camera_y);
		transformed.pos.z = intro_shift_position(
			(legacy_s32)state.opponentstate.car_posWorld1.lz,
			(legacy_s16)camera_z);
		transformed.shapeptr = &bravshape;
		intro_draw_transformed_shape(&transformed, &current_shape_rect,
			LEGACY_S16_WRAP_NEGATE(state.opponentstate.car_rotate.x));
	}

	if (slow_video_mgmt_copy != 0) {
		old_point_count = (legacy_u16)*previous_point_count;
		for (i = 0; i < old_point_count; i++)
			putpixel_single_maybe(previous_points[i].px,
				previous_points[i].py, 0);
		rect_union(shape_rect, previous_rect, &redraw_rect);
		if (rect_intersect(&redraw_rect, &rect_unk3) == 0) {
			sprite_set_1_size(redraw_rect.left, redraw_rect.right,
				redraw_rect.top, redraw_rect.bottom);
			sprite_clear_1_color(0);
		}
		point_rect = current_shape_rect;
	} else {
		sprite_set_1_size(intro_cliprect.left, intro_cliprect.right,
			intro_cliprect.top, intro_cliprect.bottom);
		sprite_clear_1_color(0);
	}

	sprite_set_1_size(intro_cliprect.left, intro_cliprect.right,
		intro_cliprect.top, intro_cliprect.bottom);
	new_point_count = 0;
	for (i = 0; i < 100U; i++) {
		translated.x = LEGACY_S16_WRAP_SUB(stars[i].x, camera_x);
		translated.y = LEGACY_S16_WRAP_SUB(stars[i].y, camera_y);
		translated.z = LEGACY_S16_WRAP_SUB(stars[i].z, camera_z);
		mat_mul_vector(&translated, &mat_temp, &projected);
		if (projected.z <= 0xC8)
			continue;
		vector_to_point(&projected, &point);
		putpixel_single_maybe(point.px, point.py, intro_colorvalue);
		if (slow_video_mgmt_copy != 0) {
			previous_points[new_point_count] = point;
			new_point_count++;
			rect_adjust_from_point(&point, &point_rect);
		}
		intro_colorvalue = LEGACY_S16_WRAP_ADD(intro_colorvalue, 1);
		if (intro_colorvalue == word_407CC)
			intro_colorvalue = 1;
	}
	if (slow_video_mgmt_copy != 0)
		*previous_point_count = new_point_count;

	get_a_poly_info();
	if (slow_video_mgmt_copy != 0) {
		*shape_rect = current_shape_rect;
		*combined_rect = point_rect;
	}
}

void intro_op(legacy_s16 camera_x, legacy_s16 camera_y, legacy_s16 camera_z, legacy_s16 rotate_y,
	legacy_s16 rotate_x, legacy_s16 draw_car, legacy_s16 primary_logo, struct VECTOR* stars,
	struct POINT2D* previous_points, legacy_s16* previous_point_count,
	struct RECTANGLE previous_rect, struct RECTANGLE* shape_rect,
	struct RECTANGLE* combined_rect)
{
	intro_op_impl(camera_x, camera_y, camera_z, rotate_y, rotate_x,
		draw_car, primary_logo, stars, previous_points,
		previous_point_count, &previous_rect, shape_rect, combined_rect);
}

/* Creep one unit towards the wanted value, and stop once it is reached. */
static legacy_s16 intro_step_towards(legacy_s16 value, legacy_s16 target)
{
	if (value > target)
		return LEGACY_S16_WRAP_SUB(value, 1);
	if (value < target)
		return LEGACY_S16_WRAP_ADD(value, 1);
	return value;
}

legacy_s8 setup_intro(void)
{
	legacy_s8 far* title_resource;
	legacy_s8 far* title_shapes[3];
	void far* opponent_resource;
	struct VECTOR stars[100];
	struct POINT2D point_buffers[2][100];
	legacy_s16 point_counts[2];
	struct RECTANGLE shape_rect;
	struct RECTANGLE combined_rect;
	struct RECTANGLE redraw_rect;
	legacy_s16 camera_x;
	legacy_s16 camera_y;
	legacy_s16 camera_z;
	legacy_s16 target_x;
	legacy_s16 target_y;
	legacy_s16 target_z;
	legacy_s16 opponent_x;
	legacy_s16 opponent_y;
	legacy_s16 opponent_z;
	legacy_s16 horizontal_angle;
	legacy_s16 vertical_angle;
	legacy_s16 target_distance;
	legacy_s16 frame_count;
	legacy_s16 delta;
	legacy_s16 elapsed_limit;
	legacy_s16 logo_changed;
	legacy_s16 draw_car;
	legacy_s16 needs_render;
	legacy_u16 rect_index;
	legacy_u16 i;
	legacy_s16 difference;
	legacy_s16 absolute_difference;
	legacy_s16* active_point_count;
	struct POINT2D* active_points;
	legacy_s8 interrupted;

	interrupted = 0;
	title_resource = (legacy_s8 far*)file_load_3dres("title");
	locate_many_resources(title_resource, "logolog2brav", title_shapes);
	shape3d_init_shape(title_shapes[0], &logoshape);
	shape3d_init_shape(title_shapes[1], &logo2shape);
	shape3d_init_shape(title_shapes[2], &bravshape);
	if (video_flag5_is0 == 0)
		render_window_sprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);

	for (i = 0; i < 100U; i++) {
		stars[i].x = LEGACY_S16_WRAP_SUB(
			LEGACY_U16_WRAP_MUL(get_kevinrandom(), 0x80U), 0x4000);
		stars[i].y = LEGACY_S16_WRAP_NEGATE(LEGACY_S16_WRAP_SUB(
			LEGACY_U16_WRAP_MUL(get_kevinrandom(), 0x80U), 0x1388));
		stars[i].z = LEGACY_S16_WRAP_SUB(
			LEGACY_U16_WRAP_MUL(get_kevinrandom(), 0x80U), 0x4000);
	}

	set_projection(0x28, 0x28, 0x140, 0xC8);
	camera_x = 0x400;
	camera_y = 0x12C;
	camera_z = 0x400;
	logo_changed = 0;
	frame_count = 0;
	opponent_resource = file_load_resfile("carcoun");
	setup_aero_trackdata(opponent_resource, 1);
	unload_resource(opponent_resource);
	init_plantrak();
	(void)timer_get_delta();
	point_counts[0] = 0;
	point_counts[1] = 0;
	slow_video_mgmt_copy = slow_video_mgmt;
	rect_unk[0].left = 0;
	rect_unk[0].right = 0x140;
	rect_unk[0].top = 0;
	rect_unk[0].bottom = 0xC8;
	rect_unk2 = rect_unk[0];
	rect_unk3 = rect_unk[0];
	rect_index = 0;
	needs_render = 1;

	for (;;) {
		delta = LEGACY_S16_FROM_BITS((legacy_u16)timer_get_delta());
		word_44DCC = LEGACY_S16_WRAP_ADD(word_44DCC, delta);

		while ((legacy_s16)word_44DCC > (legacy_s16)word_4499C) {
			word_44DCC = LEGACY_S16_WRAP_SUB(word_44DCC, word_4499C);
			do_opponent_op();
			needs_render = 1;
			frame_count = LEGACY_S16_WRAP_ADD(frame_count, 1);
			elapsed_limit = LEGACY_S16_WRAP_MUL(framespersec, 11);
			if (frame_count > elapsed_limit) {
				logo_changed = 1;
				camera_y = LEGACY_S16_WRAP_ADD(camera_y, 0x14);
				camera_z = LEGACY_S16_WRAP_SUB(camera_z, 5);
				difference = LEGACY_S16_WRAP_SUB(camera_x, 0x400);
				absolute_difference = absolute_word(difference);
				if (absolute_difference < 10)
					camera_x = 0x400;
				else if (difference > 0)
					camera_x = LEGACY_S16_WRAP_SUB(camera_x, 10);
				else if (difference < 0)
					camera_x = LEGACY_S16_WRAP_ADD(camera_x, 10);

				target_x = intro_step_towards(target_x, 0x400);
				target_z = intro_step_towards(target_z, 0x400);
			}
		}

		if (needs_render != 0) {
			needs_render = 0;
			if (video_flag5_is0 != 0)
				setup_mcgawnd2();
			else
				sprite_copy_wnd_to_1();
			draw_car = 1;
			horizontal_angle = -1;
			opponent_x = intro_shift_position(
				(legacy_s32)state.opponentstate.car_posWorld1.lx, 0);
			opponent_y = intro_shift_position(
				(legacy_s32)state.opponentstate.car_posWorld1.ly, 0);
			opponent_z = intro_shift_position(
				(legacy_s32)state.opponentstate.car_posWorld1.lz, 0);

			elapsed_limit = LEGACY_S16_WRAP_MUL(framespersec, 6);
			if (frame_count < elapsed_limit) {
				draw_car = 0;
				horizontal_angle = LEGACY_S16_FROM_BITS(
					(legacy_u16)state.opponentstate.car_rotate.x & 0x03FFU);
				vertical_angle = 0;
				camera_x = opponent_x;
				camera_y = LEGACY_S16_WRAP_ADD(opponent_y, 0x14);
				camera_z = opponent_z;
			} else {
				elapsed_limit = LEGACY_S16_WRAP_MUL(framespersec, 11);
				if (frame_count < elapsed_limit) {
					camera_x = 0x400;
					camera_y = 0x5A;
					camera_z = 0x400;
					target_x = opponent_x;
					target_y = opponent_y;
					target_z = opponent_z;
				}
			}

			if (horizontal_angle == -1) {
				horizontal_angle = LEGACY_S16_FROM_BITS(
					(legacy_u16)LEGACY_S16_WRAP_NEGATE(polarAngle(
						LEGACY_S16_WRAP_SUB(target_x, camera_x),
						LEGACY_S16_WRAP_SUB(target_z, camera_z))) & 0x03FFU);
				target_distance = (legacy_s16)polarRadius2D(
					LEGACY_S16_WRAP_SUB(target_x, camera_x),
					LEGACY_S16_WRAP_SUB(target_z, camera_z));
				vertical_angle = LEGACY_S16_FROM_BITS((legacy_u16)polarAngle(
					LEGACY_S16_WRAP_SUB(target_y, camera_y),
					target_distance) & 0x03FFU);
			}

			active_points = point_buffers[rect_index];
			active_point_count = &point_counts[rect_index];
			intro_op_impl(camera_x, camera_y, camera_z, horizontal_angle,
				vertical_angle, draw_car, logo_changed, stars,
				active_points, active_point_count, &rect_unk[rect_index],
				&shape_rect, &combined_rect);

			if (video_flag5_is0 != 0) {
				mouse_draw_opaque_check();
				setup_mcgawnd1();
				mouse_draw_transparent_check();
				if (slow_video_mgmt_copy != 0)
					rect_unk[rect_index] = shape_rect;
				rect_index ^= 1U;
			} else {
				sprite_copy_2_to_1_2();
				if (slow_video_mgmt_copy != 0) {
					rect_union(&combined_rect, &rect_unk6, &redraw_rect);
					if (rect_intersect(&redraw_rect, &rect_unk3) == 0) {
						sprite_set_1_size(redraw_rect.left, redraw_rect.right,
							redraw_rect.top, redraw_rect.bottom);
						mouse_draw_opaque_check();
						sprite_putimage(render_window_sprite->sprite_bitmapptr);
						mouse_draw_transparent_check();
						rect_unk[0] = shape_rect;
						rect_unk6 = combined_rect;
					}
				} else {
					mouse_draw_opaque_check();
					sprite_putimage(render_window_sprite->sprite_bitmapptr);
					mouse_draw_transparent_check();
				}
			}
		}

		if (input_do_checking(delta) != 0) {
			interrupted = 1;
			break;
		}
		elapsed_limit = LEGACY_S16_WRAP_MUL(0x17, framespersec);
		if (frame_count >= elapsed_limit)
			break;
	}

	if (video_flag5_is0 != 0) {
		if (get_0() != 0) {
			setup_mcgawnd2();
			sub_35C4E(0, 0, 0x140, 0xC8, 0);
			mouse_draw_opaque_check();
			setup_mcgawnd1();
			mouse_draw_transparent_check();
		}
	} else {
		sprite_free_wnd(render_window_sprite);
	}
	mmgr_free(title_resource);
	return interrupted;
}
