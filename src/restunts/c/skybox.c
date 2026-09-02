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

void skybox_op_helper2(struct RECTANGLE* rect, legacy_s16 angle, legacy_s16 horizon)
{
	legacy_u16 top;
	legacy_u16 bottom;
	legacy_u16 left;
	legacy_u16 right;
	legacy_u16 sky_lines;
	legacy_u16 rect_height;
	legacy_u16 image_x;
	legacy_u16 ground_top;
	legacy_u16 ground_lines;
	legacy_u16 horizon_bits;

	top = (legacy_u16)rect->top;
	bottom = (legacy_u16)rect->bottom;
	left = (legacy_u16)rect->left;
	right = (legacy_u16)rect->right;
	horizon_bits = (legacy_u16)horizon;
	sky_lines = LEGACY_U16_WRAP_SUB(horizon_bits, top);
	if (detail_level != 4)
		sky_lines = LEGACY_U16_WRAP_SUB(sky_lines, skybox_current);
	rect_height = LEGACY_U16_WRAP_SUB(bottom, top);
	if (LEGACY_S16_FROM_BITS(rect_height) <
		LEGACY_S16_FROM_BITS(sky_lines))
		sky_lines = rect_height;
	if (LEGACY_S16_FROM_BITS(sky_lines) > 0) {
		sprite_set_1_size(left, right, top,
			LEGACY_U16_WRAP_ADD(top, sky_lines));
		sprite_clear_1_color((legacy_u8)skybox_sky_color);
	}

	if (detail_level != 4 &&
		LEGACY_S16_FROM_BITS(top) < LEGACY_S16_FROM_BITS(horizon_bits) &&
		LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB(
			horizon_bits, word_454CE)) <= LEGACY_S16_FROM_BITS(bottom)) {
		sprite_set_1_size(left, right, top, bottom);
		image_x = LEGACY_U16_WRAP_SUB(
			LEGACY_U16_WRAP_ADD(angle, 0x200U) & 0x03FFU, 0x0400U);
		sprite_putimage_and_alt(skyboxes[0],
			LEGACY_S16_FROM_BITS(image_x), LEGACY_S16_WRAP_SUB(
				horizon_bits, skybox_ptr1));
		sprite_putimage_and_alt(skyboxes[1], LEGACY_S16_FROM_BITS(
			LEGACY_U16_WRAP_ADD(image_x, 0x0140U)),
			LEGACY_S16_WRAP_SUB(horizon_bits, skybox_ptr2));
		sprite_putimage_and_alt(skyboxes[2], LEGACY_S16_FROM_BITS(
			LEGACY_U16_WRAP_ADD(image_x, 0x0200U)),
			LEGACY_S16_WRAP_SUB(horizon_bits, skybox_ptr3));
		sprite_putimage_and_alt(skyboxes[3], LEGACY_S16_FROM_BITS(
			LEGACY_U16_WRAP_ADD(image_x, 0x0340U)),
			LEGACY_S16_WRAP_SUB(horizon_bits, skybox_ptr4));
		sprite_putimage_and_alt(skyboxes[0], LEGACY_S16_FROM_BITS(
			LEGACY_U16_WRAP_ADD(image_x, 0x0400U)),
			LEGACY_S16_WRAP_SUB(horizon_bits, skybox_ptr1));
	}

	ground_top = horizon_bits;
	if (LEGACY_S16_FROM_BITS(top) > LEGACY_S16_FROM_BITS(horizon_bits))
		ground_top = top;
	ground_lines = LEGACY_U16_WRAP_SUB(bottom, ground_top);
	if (LEGACY_S16_FROM_BITS(ground_lines) > 0) {
		sprite_set_1_size(left, right, ground_top,
			LEGACY_U16_WRAP_ADD(ground_top, ground_lines));
		sprite_clear_1_color((legacy_u8)skybox_grd_color);
	}
}

static legacy_s16 skybox_scaled_constant(legacy_u16 value,
	legacy_s16 scale)
{
	return LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_MUL(
		value, (legacy_u16)scale));
}

static void skybox_clear_rect(const struct RECTANGLE* rect, legacy_s16 color)
{
	sprite_set_1_size(rect->left, rect->right, rect->top, rect->bottom);
	sprite_clear_1_color((legacy_u8)color);
}

static void skybox_collect_changed_rects(struct RECTANGLE* clip)
{
	rect_array_unk3_length = 0;
	rectlist_add_rects(15, rect_array_unk_indices, rectptr_unk,
		rect_unk, clip, &rect_array_unk3_length, rect_array_unk3);
}

legacy_s16 skybox_op(legacy_s16 view_index, struct RECTANGLE* clip, legacy_s16 direction,
	struct MATRIX* rotation, legacy_s16 roll, legacy_s16 angle, legacy_s16 camera_y)
{
	static const legacy_u16 corner_angles[4] = {
		0x0080U, 0x0180U, 0x0280U, 0x0380U
	};
	struct VECTOR source;
	struct VECTOR vectors[2];
	struct POINT2D points[6];
	struct POINT2D point_swap;
	struct RECTANGLE work_rect;
	legacy_u16 line_data[14];
	legacy_s16 base_horizon;
	legacy_s16 horizon_delta;
	legacy_s16 horizon;
	legacy_s16 absolute_delta;
	legacy_s16 strip_count;
	legacy_s16 strip;
	legacy_s16 previous_x;
	legacy_s16 angle_offset;
	legacy_s16 point_index;
	legacy_s16 base_index;
	legacy_s16 fill_height;
	legacy_s16 track_view_index;
	legacy_s16 i;
	legacy_u8 has_linear_horizon;
	legacy_s16 fill_color;

	rect_array_unk3_length = 0;
	sprite_set_1_size(0, 0x140, clip->top, clip->bottom);

	if (roll != 0) {
		source.x = skybox_scaled_constant(0x4650U,
			(legacy_s16)direction);
		source.y = LEGACY_S16_WRAP_NEGATE((legacy_s16)camera_y);
		source.z = skybox_scaled_constant(0x3A98U,
			(legacy_s16)direction);
		mat_mul_vector(&source, rotation, &vectors[0]);
		source.x = skybox_scaled_constant(0xB9B0U,
			(legacy_s16)direction);
		mat_mul_vector(&source, rotation, &vectors[1]);

		if (vectors[0].z < 0 || vectors[1].z < 0) {
			sprite_clear_1_color((legacy_u8)skybox_sky_color);
			return 0;
		}
		vector_to_point(&vectors[0], &points[0]);
		vector_to_point(&vectors[1], &points[1]);

		if (points[0].px > 0x140 && points[1].px > 0x140) {
			fill_color = points[0].py < points[1].py ?
				skybox_sky_color : skybox_grd_color;
			sprite_clear_1_color((legacy_u8)fill_color);
			return 0;
		}
		if (points[0].px < 0 && points[1].px < 0) {
			fill_color = points[0].py <= points[1].py ?
				skybox_grd_color : skybox_sky_color;
			sprite_clear_1_color((legacy_u8)fill_color);
			return 0;
		}
		if (clip->bottom < points[0].py &&
			clip->bottom < points[1].py) {
			fill_color = points[0].px <= points[1].px ?
				skybox_grd_color : skybox_sky_color;
			sprite_clear_1_color((legacy_u8)fill_color);
			return 0;
		}
		if (clip->top > points[0].py && clip->top > points[1].py) {
			fill_color = points[0].px >= points[1].px ?
				skybox_grd_color : skybox_sky_color;
			sprite_clear_1_color((legacy_u8)fill_color);
			return 0;
		}

		base_horizon = 0;
		horizon_delta = 0;
		has_linear_horizon = 0;
		if (detail_level != 4 && points[1].px < 0 &&
			points[0].px > 0x140 &&
			draw_line_related(points[1].px, points[1].py,
				points[0].px, points[0].py, line_data) == 0) {
			absolute_delta = LEGACY_S16_WRAP_SUB(
				line_data[3], line_data[5]);
			if (absolute_delta < 0)
				absolute_delta = LEGACY_S16_WRAP_NEGATE(absolute_delta);
			if (absolute_delta < 0x60) {
				if (line_data[1] == 0) {
					base_horizon = LEGACY_S16_FROM_BITS(line_data[3]);
					horizon_delta = LEGACY_S16_WRAP_SUB(
						line_data[5], base_horizon);
					has_linear_horizon = 1;
				} else if (line_data[1] == 0x13F) {
					base_horizon = LEGACY_S16_FROM_BITS(line_data[5]);
					horizon_delta = LEGACY_S16_WRAP_SUB(
						line_data[3], base_horizon);
					has_linear_horizon = 1;
				}
			}
		}

		if (has_linear_horizon != 0) {
			if (slow_video_mgmt_copy != 0) {
				work_rect.left = 0;
				work_rect.right = 0x140;
				rect_skybox.left = 0;
				rect_skybox.right = 0x140;
				if (byte_454A4 != 0) {
					rect_skybox.top = clip->top;
					rect_skybox.bottom = clip->bottom;
				} else {
					horizon = LEGACY_S16_WRAP_ADD(
						base_horizon, horizon_delta);
					rect_skybox.top = horizon < base_horizon ?
						horizon : base_horizon;
					rect_skybox.top = LEGACY_S16_WRAP_SUB(
						rect_skybox.top, word_454CE);
					if (clip->top > rect_skybox.top)
						rect_skybox.top = clip->top;
					rect_skybox.bottom = horizon > base_horizon ?
						horizon : base_horizon;
					for (i = 0; i < 15; i++)
						rect_array_unk_indices[i] = 1;
					rect_array_unk_indices[5] = 3;

					work_rect.top = 0;
					work_rect.bottom = rect_skybox.top;
					if (rect_intersect(&work_rect, clip) == 0) {
						skybox_collect_changed_rects(&work_rect);
						for (i = 0;
							i < (legacy_s8)rect_array_unk3_length;
							i++)
							skybox_clear_rect(&rect_array_unk3[i],
								skybox_sky_color);
					}

					work_rect.top = rect_skybox.bottom;
					work_rect.bottom = 0xC8;
					if (rect_intersect(&work_rect, clip) == 0) {
						skybox_collect_changed_rects(&work_rect);
						for (i = 0;
							i < (legacy_s8)rect_array_unk3_length;
							i++)
							skybox_clear_rect(&rect_array_unk3[i],
								skybox_grd_color);
					}
				}
				work_rect.top = rect_skybox.top;
				work_rect.bottom = rect_skybox.bottom;
			} else {
				work_rect.left = 0;
				work_rect.right = 0x140;
				work_rect.top = clip->top;
				work_rect.bottom = clip->bottom;
			}

			if (rect_intersect(&work_rect, clip) != 0)
				return 0;
			absolute_delta = horizon_delta;
			if (absolute_delta < 0)
				absolute_delta = LEGACY_S16_WRAP_NEGATE(absolute_delta);
			strip_count = LEGACY_S16_WRAP_ADD(absolute_delta, 1);
			if (strip_count > 0x20)
				strip_count = 0x20;
			previous_x = 0;
			for (strip = 0; strip < strip_count; strip++) {
				work_rect.left = previous_x;
				work_rect.right = (legacy_s16)((
					(legacy_s32)0x140 * strip + 0x140) /
					strip_count) & video_flag3_isFFFF;
				if (work_rect.left == work_rect.right)
					continue;
				horizon = LEGACY_S16_WRAP_ADD(base_horizon,
					(legacy_s16)((legacy_s32)horizon_delta * strip /
						strip_count));
				skybox_op_helper2(&work_rect, angle, horizon);
				previous_x = work_rect.right;
			}
			return 0;
		}

		angle_offset = (legacy_s16)polarAngle(
			LEGACY_S16_WRAP_SUB(points[0].px, points[1].px),
			LEGACY_S16_WRAP_SUB(points[0].py, points[1].py)) & 0x03FF;
		for (point_index = 0; point_index < 4; point_index++) {
			base_index = point_index < 2 ? 0 : 1;
			points[point_index + 2].px = LEGACY_S16_WRAP_ADD(
				points[base_index].px, multiply_and_scale(
					sin_fast(LEGACY_S16_WRAP_ADD(
						corner_angles[point_index], angle_offset)),
					0x3E80));
			points[point_index + 2].py = LEGACY_S16_WRAP_ADD(
				points[base_index].py, multiply_and_scale(
					cos_fast(LEGACY_S16_WRAP_ADD(
						corner_angles[point_index], angle_offset)),
					0x3E80));
		}
		/* The original inline stack arguments order these vertices 0,1,3,2. */
		point_swap = points[2];
		points[2] = points[3];
		points[3] = point_swap;
		skybox_op_helper(skybox_sky_color, 4, points);
		points[2] = points[4];
		points[3] = points[5];
		skybox_op_helper(skybox_grd_color, 4, points);
		return 1;
	}

	source.x = 0;
	source.y = LEGACY_S16_WRAP_NEGATE((legacy_s16)camera_y);
	source.z = skybox_scaled_constant(0x3A98U,
		(legacy_s16)direction);
	mat_mul_vector(&source, rotation, &vectors[0]);
	if (vectors[0].z < 0) {
		sprite_clear_1_color((legacy_u8)skybox_sky_color);
		if (slow_video_mgmt_copy == 0)
			return 0;
		rect_skybox.left = 0;
		rect_skybox.right = 0x140;
		rect_skybox.top = clip->top;
		rect_skybox.bottom = clip->bottom;
		return 1;
	}

	vector_to_point(&vectors[0], &points[0]);
	horizon = (legacy_s16)points[0].py;
	if (clip->top > horizon)
		horizon = clip->top;
	if ((legacy_s16)direction == 1) {
		if (slow_video_mgmt_copy != 0) {
			rect_skybox.top = detail_level == 4 ?
				LEGACY_S16_WRAP_SUB(horizon, 1) :
				LEGACY_S16_WRAP_SUB(horizon, word_454CE);
			rect_skybox.left = 0;
			rect_skybox.right = 0x140;
			rect_skybox.bottom = horizon;
			if (byte_454A4 == 0) {
				for (i = 0; i < 15; i++)
					rect_array_unk_indices[i] = 1;
				track_view_index = (legacy_s16)view_index;
				if (detail_level == 4)
					word_449FC[track_view_index] = word_463D6;
				if (word_449FC[track_view_index] == angle &&
					rectptr_unk[5].left == rect_skybox.left &&
					rectptr_unk[5].right == rect_skybox.right &&
					rectptr_unk[5].top == rect_skybox.top &&
					rectptr_unk[5].bottom == rect_skybox.bottom) {
					rect_array_unk_indices[5] = 0;
				} else {
					rect_array_unk_indices[5] = 3;
				}
				skybox_collect_changed_rects(clip);
				for (i = 0; i < (legacy_s8)rect_array_unk3_length;
					i++)
					skybox_op_helper2(&rect_array_unk3[i],
						angle, horizon);
				return 0;
			}
		}

		work_rect.left = 0;
		work_rect.right = 0x140;
		work_rect.top = clip->top;
		work_rect.bottom = clip->bottom;
		skybox_op_helper2(&work_rect, angle, horizon);
		return 0;
	}

	fill_height = LEGACY_S16_WRAP_SUB(horizon, clip->top);
	if (LEGACY_S16_WRAP_SUB(clip->bottom, clip->top) < fill_height)
		fill_height = LEGACY_S16_WRAP_SUB(clip->bottom, clip->top);
	if (fill_height > 0) {
		work_rect.left = 0;
		work_rect.right = 0x140;
		work_rect.top = clip->top;
		work_rect.bottom = LEGACY_S16_WRAP_ADD(clip->top, fill_height);
		skybox_clear_rect(&work_rect, skybox_grd_color);
	}
	fill_height = LEGACY_S16_WRAP_SUB(clip->bottom, horizon);
	if (fill_height > 0) {
		work_rect.left = 0;
		work_rect.right = 0x140;
		work_rect.top = horizon;
		work_rect.bottom = LEGACY_S16_WRAP_ADD(horizon, fill_height);
		skybox_clear_rect(&work_rect, skybox_sky_color);
	}
	return 1;
}
