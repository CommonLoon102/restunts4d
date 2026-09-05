#include "frame_internal.h"

#define TRACK_OBJECT_COUNT 215U
#define INTRO_SCREEN_WIDTH 320U
#define INTRO_SCREEN_HEIGHT 200U
#define INTRO_SCREEN_MAX_X 320
#define INTRO_SCREEN_MAX_Y 200
#define INTRO_SCREEN_COLOR 15U
#define INTRO_STAR_COUNT 100U
#define INTRO_POINT_BUFFER_COUNT 2
#define INTRO_POINT_BUFFER_MASK 1U
#define INTRO_TITLE_SHAPE_COUNT 3

#define TRANSFORMED_SHAPE_BASE_FLAG 4U
#define TRANSFORMED_SHAPE_RECT_FLAG 8U
#define INTRO_TRANSFORMED_SHAPE_SCALE 1024
#define INTRO_LOGO_WORLD_CENTER 1024
#define INTRO_STAR_MIN_DEPTH 200

#define INTRO_STAR_RANDOM_SCALE 128U
#define INTRO_STAR_XZ_OFFSET 16384
#define INTRO_STAR_Y_OFFSET 5000
#define INTRO_PROJECTION_SCALE_X 40
#define INTRO_PROJECTION_SCALE_Y 40

#define INTRO_INITIAL_CAMERA_Y 300
#define INTRO_LOGO_CAMERA_Y 90
#define INTRO_CAMERA_CAR_HEIGHT 20
#define INTRO_CAMERA_RISE_STEP 20
#define INTRO_CAMERA_RETREAT_STEP 5
#define INTRO_CAMERA_CENTER_STEP 10
#define INTRO_CAMERA_CENTER_SNAP_DISTANCE 10

#define INTRO_CAR_PHASE_SECONDS 6
#define INTRO_LOGO_PHASE_SECONDS 11
#define INTRO_TOTAL_SECONDS 23

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
		transformed->ts_flags =
			TRANSFORMED_SHAPE_BASE_FLAG | TRANSFORMED_SHAPE_RECT_FLAG;
	} else {
		transformed->ts_flags = TRANSFORMED_SHAPE_BASE_FLAG;
	}
	transformed->rotvec.x = 0;
	transformed->rotvec.y = 0;
	transformed->rotvec.z = rotation_z;
	transformed->unk = INTRO_TRANSFORMED_SHAPE_SCALE;
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
	transformed.pos.x = LEGACY_S16_WRAP_SUB(
		INTRO_LOGO_WORLD_CENTER, camera_x);
	transformed.pos.y = LEGACY_S16_WRAP_NEGATE(camera_y);
	transformed.pos.z = LEGACY_S16_WRAP_SUB(
		INTRO_LOGO_WORLD_CENTER, camera_z);
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
	for (i = 0; i < INTRO_STAR_COUNT; i++) {
		translated.x = LEGACY_S16_WRAP_SUB(stars[i].x, camera_x);
		translated.y = LEGACY_S16_WRAP_SUB(stars[i].y, camera_y);
		translated.z = LEGACY_S16_WRAP_SUB(stars[i].z, camera_z);
		mat_mul_vector(&translated, &mat_temp, &projected);
		if (projected.z <= INTRO_STAR_MIN_DEPTH)
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
	legacy_s8 far* title_shapes[INTRO_TITLE_SHAPE_COUNT];
	void far* opponent_resource;
	struct VECTOR stars[INTRO_STAR_COUNT];
	struct POINT2D point_buffers
		[INTRO_POINT_BUFFER_COUNT][INTRO_STAR_COUNT];
	legacy_s16 point_counts[INTRO_POINT_BUFFER_COUNT];
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
		render_window_sprite = sprite_make_wnd(
			INTRO_SCREEN_WIDTH, INTRO_SCREEN_HEIGHT, INTRO_SCREEN_COLOR);

	for (i = 0; i < INTRO_STAR_COUNT; i++) {
		stars[i].x = LEGACY_S16_WRAP_SUB(
			LEGACY_U16_WRAP_MUL(get_kevinrandom(),
				INTRO_STAR_RANDOM_SCALE), INTRO_STAR_XZ_OFFSET);
		stars[i].y = LEGACY_S16_WRAP_NEGATE(LEGACY_S16_WRAP_SUB(
			LEGACY_U16_WRAP_MUL(get_kevinrandom(),
				INTRO_STAR_RANDOM_SCALE), INTRO_STAR_Y_OFFSET));
		stars[i].z = LEGACY_S16_WRAP_SUB(
			LEGACY_U16_WRAP_MUL(get_kevinrandom(),
				INTRO_STAR_RANDOM_SCALE), INTRO_STAR_XZ_OFFSET);
	}

	set_projection(INTRO_PROJECTION_SCALE_X, INTRO_PROJECTION_SCALE_Y,
		INTRO_SCREEN_MAX_X, INTRO_SCREEN_MAX_Y);
	camera_x = INTRO_LOGO_WORLD_CENTER;
	camera_y = INTRO_INITIAL_CAMERA_Y;
	camera_z = INTRO_LOGO_WORLD_CENTER;
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
	rect_unk[0].right = INTRO_SCREEN_MAX_X;
	rect_unk[0].top = 0;
	rect_unk[0].bottom = INTRO_SCREEN_MAX_Y;
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
			elapsed_limit = LEGACY_S16_WRAP_MUL(
				framespersec, INTRO_LOGO_PHASE_SECONDS);
			if (frame_count > elapsed_limit) {
				logo_changed = 1;
				camera_y = LEGACY_S16_WRAP_ADD(
					camera_y, INTRO_CAMERA_RISE_STEP);
				camera_z = LEGACY_S16_WRAP_SUB(
					camera_z, INTRO_CAMERA_RETREAT_STEP);
				difference = LEGACY_S16_WRAP_SUB(
					camera_x, INTRO_LOGO_WORLD_CENTER);
				absolute_difference = absolute_word(difference);
				if (absolute_difference < INTRO_CAMERA_CENTER_SNAP_DISTANCE)
					camera_x = INTRO_LOGO_WORLD_CENTER;
				else if (difference > 0)
					camera_x = LEGACY_S16_WRAP_SUB(
						camera_x, INTRO_CAMERA_CENTER_STEP);
				else if (difference < 0)
					camera_x = LEGACY_S16_WRAP_ADD(
						camera_x, INTRO_CAMERA_CENTER_STEP);

				target_x = intro_step_towards(
					target_x, INTRO_LOGO_WORLD_CENTER);
				target_z = intro_step_towards(
					target_z, INTRO_LOGO_WORLD_CENTER);
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

			elapsed_limit = LEGACY_S16_WRAP_MUL(
				framespersec, INTRO_CAR_PHASE_SECONDS);
			if (frame_count < elapsed_limit) {
				draw_car = 0;
				horizontal_angle = LEGACY_S16_FROM_BITS(
					(legacy_u16)state.opponentstate.car_rotate.x & ANGLE_MASK);
				vertical_angle = 0;
				camera_x = opponent_x;
				camera_y = LEGACY_S16_WRAP_ADD(
					opponent_y, INTRO_CAMERA_CAR_HEIGHT);
				camera_z = opponent_z;
			} else {
				elapsed_limit = LEGACY_S16_WRAP_MUL(
					framespersec, INTRO_LOGO_PHASE_SECONDS);
				if (frame_count < elapsed_limit) {
					camera_x = INTRO_LOGO_WORLD_CENTER;
					camera_y = INTRO_LOGO_CAMERA_Y;
					camera_z = INTRO_LOGO_WORLD_CENTER;
					target_x = opponent_x;
					target_y = opponent_y;
					target_z = opponent_z;
				}
			}

			if (horizontal_angle == -1) {
				horizontal_angle = LEGACY_S16_FROM_BITS(
					(legacy_u16)LEGACY_S16_WRAP_NEGATE(polarAngle(
						LEGACY_S16_WRAP_SUB(target_x, camera_x),
						LEGACY_S16_WRAP_SUB(target_z, camera_z))) & ANGLE_MASK);
				target_distance = (legacy_s16)polarRadius2D(
					LEGACY_S16_WRAP_SUB(target_x, camera_x),
					LEGACY_S16_WRAP_SUB(target_z, camera_z));
				vertical_angle = LEGACY_S16_FROM_BITS((legacy_u16)polarAngle(
					LEGACY_S16_WRAP_SUB(target_y, camera_y),
					target_distance) & ANGLE_MASK);
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
				rect_index ^= INTRO_POINT_BUFFER_MASK;
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
		elapsed_limit = LEGACY_S16_WRAP_MUL(
			INTRO_TOTAL_SECONDS, framespersec);
		if (frame_count >= elapsed_limit)
			break;
	}

	if (video_flag5_is0 != 0) {
		if (get_0() != 0) {
			setup_mcgawnd2();
			sub_35C4E(0, 0, INTRO_SCREEN_MAX_X, INTRO_SCREEN_MAX_Y, 0);
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
