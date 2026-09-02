#include "externs.h"
#include "fileio.h"
#include "legacy.h"
#include "math.h"
#include "memmgr.h"
#include "shape2d.h"
#include "shape3d.h"

extern struct RECTANGLE* rectptr_unk2;
extern struct RECTANGLE rect_array_unk[];
extern struct RECTANGLE rect_array_unk2[];
extern struct RECTANGLE rect_array_unk3[];
extern legacy_s8 rect_array_unk_indices[];
extern legacy_s16 rect_array_unk3_indices[];
extern legacy_s8 rect_array_unk3_length;
extern struct RECTANGLE rect_unk[];
/* These legacy labels were views into consecutive elements of rect_unk. */
#define rect_unk2  rect_unk[1]
#define rect_unk6  rect_unk[2]
#define rect_unk12 rect_unk[3]
#define rect_unk15 rect_unk[4]
#define rect_skybox rect_unk[5]
#define rect_unk11 rect_unk[6]
#define rect_unk9  rect_unk[7]
extern struct RECTANGLE rect_unk3;
extern struct RECTANGLE rect_unk5;
extern struct RECTANGLE cliprect_unk;
extern struct RECTANGLE rect_ingame_text2;
extern struct RECTANGLE rect_ingame_text3;
extern struct RECTANGLE rect_ingame_text4;
extern struct VECTOR vec_unk2;
extern struct VECTOR vec_planerotopresult;
extern struct MATRIX mat_temp;
extern legacy_s16 custom_camera_distance;
extern legacy_s16 custom_camera_elevation_angle;
extern legacy_s16 custom_camera_azimuth_angle;
extern legacy_s16 camera_track_height_offset;
extern legacy_s8 detail_threshold_by_level[];
extern legacy_s8 byte_3C0C6[];
extern legacy_u16 frame_callback_count;
extern legacy_s16 word_3BE34[];
extern legacy_s8* lookahead_tiles_tables[];
extern struct SHAPE3D* off_3BE44[];
extern legacy_s16 terrainHeight;
extern legacy_s16 planindex;
extern legacy_s16 planindex_copy;
extern legacy_s8 byte_4392C;
extern struct TRANSFORMEDSHAPE3D currenttransshape[29];
//extern struct TRANSFORMEDSHAPE3D transshapeunk;
extern struct TRANSFORMEDSHAPE3D* curtransshape_ptr;
extern struct TRACKOBJECT trkObjectList[215]; // 215 entries
extern legacy_u8 fence_TrkObjCodes[];
extern legacy_s16 pState_minusRotate_z_2, pState_minusRotate_x_2, pState_minusRotate_y_2, pState_f36Mminf40sar2;

extern legacy_s8 unk_3C0EE[];
extern legacy_s8 unk_3C0F0[];
extern legacy_s8 unk_3C0F8[];
extern legacy_s8 unk_3C0F4[];
extern legacy_s16 word_3C0D6[];
extern legacy_s16 unk_3C0A2[];
extern legacy_s16 unk_3C0A6[];
extern legacy_s16 unk_3C0AE[];
extern legacy_s16 unk_3C0B6[];
extern struct TRACKOBJECT sceneshapes2[];
extern struct TRACKOBJECT sceneshapes3[];
extern struct SHAPE3D game3dshapes[130];
extern struct VECTOR carshapevec[2];
extern struct VECTOR carshapevecs[24];
extern legacy_s16 word_443E8[];
extern struct VECTOR oppcarshapevec[2];
extern struct VECTOR oppcarshapevecs[24];
extern legacy_s16 word_4448A[];
extern legacy_s8 backlights_paint_override;
extern legacy_s16 word_449FC[];
extern legacy_s16 word_463D6;
extern legacy_s16 transformedshape_zarray[];
extern legacy_s16 transformedshape_indices[];
extern legacy_s8 transformedshape_arg2array[];
extern legacy_s16 sdgame2_widths[];
extern void far* sdgame2shapes[];
extern void far* fontledresptr;
extern legacy_s16 dialog_fnt_colour;
extern legacy_s8 transformedshape_counter;
extern legacy_s16 word_449FE;
extern struct SPRITE far* render_window_sprite;
extern legacy_s16 fontdef_unk_0E;
extern legacy_u16 skybox_current;
extern legacy_u16 word_454CE;
extern legacy_u16 skybox_ptr1;
extern legacy_u16 skybox_ptr2;
extern legacy_u16 skybox_ptr3;
extern legacy_u16 skybox_ptr4;
extern legacy_s16 skybox_sky_color;
extern legacy_s16 skybox_grd_color;
extern legacy_s16 skybox_wat_color;
extern struct RECTANGLE rect_ingame_text;
extern struct RECTANGLE intro_cliprect;
extern struct SHAPE2D far* skyboxes[];
extern legacy_s16 penalty_time;
extern legacy_s16 intro_colorvalue;
extern legacy_s16 word_407CC;
extern struct SHAPE3D logoshape;
extern struct SHAPE3D logo2shape;
extern struct SHAPE3D bravshape;
extern legacy_s16 word_44DCC;
extern legacy_s16 word_3C108;
extern legacy_s16 word_3C10A;
extern legacy_s16 word_3C10C;
extern legacy_s16 word_3C10E;
extern legacy_s16 word_3C110;
extern legacy_s16 word_3C112;
extern struct VECTOR unk_3C114;
extern struct RECTANGLE trackpreview_cliprect;

void build_track_object(struct VECTOR* a, struct VECTOR* b);
void transformed_shape_add_for_sort(legacy_s16 z_adjust, legacy_s16 type);
void skybox_op_helper2(struct RECTANGLE* rect, legacy_s16 angle, legacy_s16 horizon);
legacy_u8 subst_hillroad_track(legacy_u8 a, legacy_u8 b);
legacy_s16 skybox_op(legacy_s16 a, struct RECTANGLE* rectptr, legacy_s16 c, struct MATRIX* matptr, legacy_s16 e, legacy_s16 f, legacy_s16 g);
void sprite_putimage_transparent(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);
void copy_string(legacy_s8* destination, legacy_s8 far* source);
legacy_s16 font_op2_alt(const legacy_s8* text);
struct RECTANGLE* draw_ingame_text(void);
struct RECTANGLE* init_crak(legacy_s16 frame, legacy_s16 top, legacy_s16 height);
struct RECTANGLE* do_sinking(legacy_s16 frame, legacy_s16 top, legacy_s16 height);
struct RECTANGLE* intro_draw_text(legacy_s8* str, legacy_s16 a, legacy_s16 b, legacy_s16 c, legacy_s16 d);
void intro_op(legacy_s16 camera_x, legacy_s16 camera_y, legacy_s16 camera_z, legacy_s16 rotate_y,
	legacy_s16 rotate_x, legacy_s16 draw_car, legacy_s16 primary_logo, struct VECTOR* stars,
	struct POINT2D* previous_points, legacy_s16* previous_point_count,
	struct RECTANGLE previous_rect, struct RECTANGLE* shape_rect,
	struct RECTANGLE* combined_rect);
void init_plantrak(void);
void do_opponent_op(void);
void setup_aero_trackdata(void far* carresptr, legacy_s16 is_opponent);
legacy_u32 timer_get_delta(void);
legacy_s16 get_0(void);
void sub_35C4E(legacy_s16 source_x, legacy_s16 source_y, legacy_s16 width, legacy_s16 height,
	legacy_s16 destination_shift);
void font_set_fontdef2(void far* data);
void set_fontdefseg(void far* data);
void format_frame_as_string(legacy_s8* s, legacy_s16 time, legacy_s16 c);
void heapsort_by_order(legacy_s16 n, legacy_s16* heap, legacy_s16* data);

#define TRACK_OBJECT_COUNT 215U

/*
 * In the original dseg, sceneshapes2 immediately follows trkObjectList.
 * Some track objects store overlay indices into that combined legacy table,
 * so indices beyond trkObjectList intentionally address sceneshapes2.
 */
static legacy_s16 intro_shift_position(legacy_s32 position,
	legacy_s16 camera)
{
	legacy_u32 bits;

	bits = (legacy_u32)position;
	bits = (bits >> 6) |
		((bits & 0x80000000UL) != 0 ? 0xFC000000UL : 0);
	return LEGACY_S16_WRAP_SUB((legacy_u16)bits, camera);
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
	if (slow_video_mgmt_copy != 0) {
		transformed.rectptr = &current_shape_rect;
		transformed.ts_flags = 0x0C;
	} else {
		transformed.ts_flags = 4;
	}
	transformed.rotvec.x = 0;
	transformed.rotvec.y = 0;
	transformed.rotvec.z = 0;
	transformed.unk = 0x400;
	transformed.material = 0;
	transformed_shape_op(&transformed);

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
		if (slow_video_mgmt_copy != 0) {
			transformed.rectptr = &current_shape_rect;
			transformed.ts_flags = 0x0C;
		} else {
			transformed.ts_flags = 4;
		}
		transformed.rotvec.x = 0;
		transformed.rotvec.y = 0;
		transformed.rotvec.z = LEGACY_S16_WRAP_NEGATE(
			state.opponentstate.car_rotate.x);
		transformed.unk = 0x400;
		transformed.material = 0;
		transformed_shape_op(&transformed);
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
				absolute_difference = difference;
				if (absolute_difference < 0)
					absolute_difference = LEGACY_S16_WRAP_NEGATE(
						absolute_difference);
				if (absolute_difference < 10)
					camera_x = 0x400;
				else if (difference > 0)
					camera_x = LEGACY_S16_WRAP_SUB(camera_x, 10);
				else if (difference < 0)
					camera_x = LEGACY_S16_WRAP_ADD(camera_x, 10);

				if (target_x > 0x400)
					target_x = LEGACY_S16_WRAP_SUB(target_x, 1);
				else if (target_x < 0x400)
					target_x = LEGACY_S16_WRAP_ADD(target_x, 1);
				if (target_z > 0x400)
					target_z = LEGACY_S16_WRAP_SUB(target_z, 1);
				else if (target_z < 0x400)
					target_z = LEGACY_S16_WRAP_ADD(target_z, 1);
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
