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

void transformed_shape_add_for_sort(legacy_s16 z_adjust, legacy_s16 type)
{
	struct VECTOR transformed_position;
	legacy_s16 index;

	mat_mul_vector(&curtransshape_ptr->pos, &mat_temp,
		&transformed_position);
	index = LEGACY_S8_FROM_BITS((legacy_u8)transformedshape_counter);
	transformedshape_zarray[index] = LEGACY_S16_WRAP_ADD(
		transformed_position.z, z_adjust);
	transformedshape_arg2array[index] = (legacy_s8)(legacy_u8)type;
	transformedshape_indices[index] = index;
	transformedshape_counter = LEGACY_S8_WRAP_ADD(
		transformedshape_counter, 1);
	curtransshape_ptr++;
}

static legacy_s16 frame_position_word(legacy_s32 position)
{
	return LEGACY_S16_FROM_BITS(
		(legacy_u16)LEGACY_S32_SAR(position, 6U));
}

static legacy_s16 frame_relative_position(legacy_s32 position,
	legacy_s16 camera_position)
{
	return LEGACY_S16_WRAP_SUB(
		frame_position_word(position), camera_position);
}

static legacy_s16 frame_relative_position_sum(legacy_s32 first,
	legacy_s32 second, legacy_s16 camera_position)
{
	return frame_relative_position(
		LEGACY_S32_WRAP_ADD(first, second), camera_position);
}

static legacy_s16 frame_relative_track_position(legacy_s32 offset,
	legacy_s16 track_position, legacy_s16 camera_position)
{
	return LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_ADD(
		frame_position_word(offset), track_position), camera_position);
}

static legacy_s8 frame_tile_from_world(legacy_s32 position)
{
	return LEGACY_S8_FROM_BITS(
		(legacy_u8)((legacy_u32)position >> 16));
}

static legacy_s8 frame_south_tile_from_world(legacy_s32 position)
{
	return LEGACY_S8_WRAP_SUB(0x1D, frame_tile_from_world(position));
}

static legacy_s8 frame_tile_from_world_offset(legacy_s32 position,
	legacy_s16 offset)
{
	return frame_tile_from_world(
		LEGACY_S32_WRAP_ADD_S16(position, offset));
}

static legacy_s8 frame_south_tile_from_world_offset(legacy_s32 position,
	legacy_s16 offset)
{
	return LEGACY_S8_WRAP_SUB(0x1D,
		frame_tile_from_world_offset(position, offset));
}

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

struct RECTANGLE* do_sinking(legacy_s16 frame, legacy_s16 top, legacy_s16 height)
{
	legacy_s16 duration;
	legacy_s16 clipped_frame;
	legacy_s16 sink_height;
	legacy_s16 bottom;

	duration = LEGACY_S16_SHL(framespersec, 2U);
	clipped_frame = (legacy_s16)frame;
	if (clipped_frame > duration)
		clipped_frame = duration;
	sink_height = LEGACY_S16_FROM_BITS((legacy_u16)
		LEGACY_S32_DIV_OR_ZERO(LEGACY_S32_WRAP_MUL(
			(legacy_s32)height, (legacy_s32)clipped_frame),
			(legacy_s32)duration));
	bottom = LEGACY_S16_WRAP_ADD(top, height);
	rect_ingame_text.left = 0;
	rect_ingame_text.right = 0x140;
	rect_ingame_text.top = LEGACY_S16_WRAP_SUB(bottom, sink_height);
	rect_ingame_text.bottom = bottom;
	sprite_set_1_size(0, 0x140, rect_ingame_text.top,
		rect_ingame_text.bottom);
	sprite_clear_1_color((legacy_u8)skybox_wat_color);
	return &rect_ingame_text;
}

struct RECTANGLE* init_crak(legacy_s16 frame, legacy_s16 top, legacy_s16 height)
{
	legacy_u8 far* crack_lines;
	legacy_u8 far* crack_info;
	legacy_s16 frame_count;
	legacy_s16 frame_index;
	legacy_s16 line_count;
	legacy_s16 start_x;
	legacy_s16 start_y;
	legacy_s16 end_x;
	legacy_s16 end_y;
	legacy_s16 scaled_start_y;
	legacy_s16 scaled_end_y;
	legacy_s16 frame_divisor;
	legacy_s32 scaled_coordinate;
	struct POINT2D point;
	legacy_s16 i;

	crack_lines = (legacy_u8 far*)locate_shape_alt(gameresptr, "crak");
	crack_info = (legacy_u8 far*)locate_shape_alt(gameresptr, "cinf");
	frame_divisor = LEGACY_S16_FROM_BITS(
		LEGACY_U16_DIV_OR_ZERO(framespersec, 7U));
	frame_index = LEGACY_S16_DIV_OR_ZERO(frame, frame_divisor);
	frame_count = LEGACY_READ_S16_LE(crack_info);
	if (frame_index >= frame_count)
		frame_index = LEGACY_S16_WRAP_SUB(frame_count, 1);
	line_count = LEGACY_READ_S16_LE(crack_info +
		((legacy_u16)LEGACY_S16_WRAP_ADD(frame_index, 1) << 1));
	rect_ingame_text = cliprect_unk;

	for (i = 0; i < line_count; i++) {
		legacy_u16 line_offset = (legacy_u16)i << 3;

		start_x = LEGACY_READ_S16_LE(crack_lines + line_offset);
		start_y = LEGACY_READ_S16_LE(crack_lines + line_offset + 2U);
		end_x = LEGACY_READ_S16_LE(crack_lines + line_offset + 4U);
		end_y = LEGACY_READ_S16_LE(crack_lines + line_offset + 6U);
		scaled_coordinate = LEGACY_S32_WRAP_MUL(
			(legacy_s32)start_y, (legacy_s32)height);
		scaled_start_y = LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_S32_DIV_OR_ZERO(scaled_coordinate, 200L));
		scaled_coordinate = LEGACY_S32_WRAP_MUL(
			(legacy_s32)end_y, (legacy_s32)height);
		scaled_end_y = LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_S32_DIV_OR_ZERO(scaled_coordinate, 200L));

		preRender_line(start_x,
			LEGACY_S16_WRAP_SUB(
				LEGACY_S16_WRAP_ADD(scaled_start_y, top), 1),
			end_x,
			LEGACY_S16_WRAP_SUB(
				LEGACY_S16_WRAP_ADD(scaled_end_y, top), 1),
			0);
		preRender_line(start_x,
			LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(scaled_start_y, top), 1),
			end_x,
			LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(scaled_end_y, top), 1),
			0);
		preRender_line(start_x,
			LEGACY_S16_WRAP_ADD(scaled_start_y, top),
			end_x,
			LEGACY_S16_WRAP_ADD(scaled_end_y, top),
			dialog_fnt_colour);

		if (slow_video_mgmt_copy != 0) {
			point.px = start_x;
			point.py = LEGACY_S16_WRAP_SUB(
				LEGACY_S16_WRAP_ADD(scaled_start_y, top), 1);
			rect_adjust_from_point(&point, &rect_ingame_text);
			point.px = end_x;
			point.py = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(scaled_end_y, top), 1);
			rect_adjust_from_point(&point, &rect_ingame_text);
			point.px = start_x;
			point.py = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(scaled_start_y, top), 1);
			rect_adjust_from_point(&point, &rect_ingame_text);
			point.px = end_x;
			point.py = LEGACY_S16_WRAP_SUB(
				LEGACY_S16_WRAP_ADD(scaled_end_y, top), 1);
			rect_adjust_from_point(&point, &rect_ingame_text);
		}
	}

	return &rect_ingame_text;
}

static void draw_centered_ingame_resource(legacy_s8* resource_id, legacy_s16 y)
{
	copy_string(&resID_byte1, locate_text_res(gameresptr, resource_id));
	rect_union(&rect_ingame_text,
		intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1), y,
			dialog_fnt_colour, 0),
		&rect_ingame_text);
}

struct RECTANGLE* draw_ingame_text(void)
{
	legacy_u16 replay_frame;
	legacy_s16 replay_x;

	rect_ingame_text = cliprect_unk;
	if (idle_expired != 0) {
		draw_centered_ingame_resource("dm1", 0xAA);
		draw_centered_ingame_resource("dm2", 0xB6);
		return &rect_ingame_text;
	}

	if (game_replay_mode != 0) {
		if (game_replay_mode != 2)
			return &rect_ingame_text;
		replay_frame = (legacy_u16)state.game_frame % framespersec;
		if (replay_frame >= (legacy_u16)LEGACY_S16_SAR(framespersec, 1U))
			return &rect_ingame_text;
		copy_string(&resID_byte1, locate_text_res(gameresptr, "rpl"));
		replay_x = LEGACY_S16_WRAP_SUB(0x138,
			LEGACY_U16_WRAP_MUL(strlen(&resID_byte1), 8U));
		rect_union(&rect_ingame_text,
			intro_draw_text(&resID_byte1, replay_x, 0x0F,
				dialog_fnt_colour, 0),
			&rect_ingame_text);
		return &rect_ingame_text;
	}

	if (state.game_inputmode == 0) {
		draw_centered_ingame_resource("pre", 0x5A);
		return &rect_ingame_text;
	}
	if (passed_security == 0) {
		draw_centered_ingame_resource("se1", 0x5D);
		draw_centered_ingame_resource("se2", 0x69);
		return &rect_ingame_text;
	}
	if (followOpponentFlag != 0 || cameramode != 0 ||
		state.playerstate.car_crashBmpFlag != 0)
		return &rect_ingame_text;

	switch (state.field_45D) {
	case 1:
		sprite_putimage_transparent(sdgame2shapes[3], 0x94, 0x5D);
		rect_union(&rect_ingame_text, &rect_ingame_text2,
			&rect_ingame_text);
		break;
	case 2:
		sprite_putimage_transparent(sdgame2shapes[4], 0x94, 0x5D);
		rect_union(&rect_ingame_text, &rect_ingame_text2,
			&rect_ingame_text);
		break;
	case 3:
		draw_centered_ingame_resource("www", 0x5D);
		break;
	}

	resID_byte1 = 0;
	switch (state.field_45E) {
	case 1:
		sprite_putimage_transparent(sdgame2shapes[3], 0x44, 0x71);
		rect_union(&rect_ingame_text, &rect_ingame_text3,
			&rect_ingame_text);
		copy_string(&resID_byte1,
			locate_text_res(gameresptr, "opp"));
		break;
	case 2:
		sprite_putimage_transparent(sdgame2shapes[4], 0xE4, 0x71);
		rect_union(&rect_ingame_text, &rect_ingame_text4,
			&rect_ingame_text);
		copy_string(&resID_byte1,
			locate_text_res(gameresptr, "opp"));
		break;
	}
	if (resID_byte1 != 0)
		rect_union(&rect_ingame_text,
			intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1),
				0x74, dialog_fnt_colour, 0),
			&rect_ingame_text);

	if (show_penalty_counter != 0) {
		copy_string(&resID_byte1, locate_text_res(gameresptr, "pen"));
		format_frame_as_string(&resID_byte1 + strlen(&resID_byte1),
			penalty_time, 0);
		rect_union(&rect_ingame_text,
			intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1),
				0x66, dialog_fnt_colour, 0),
			&rect_ingame_text);
	}

	return &rect_ingame_text;
}

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

static legacy_s16 track_preview_half(legacy_s16 value)
{
	legacy_u16 bits;

	bits = (legacy_u16)value;
	return LEGACY_S16_FROM_BITS((bits >> 1) | (bits & 0x8000U));
}

static void track_preview_draw_terrain(legacy_u8 terrain,
	legacy_u8 column, legacy_u8 row, legacy_s16 height,
	legacy_s16 camera_x, legacy_s16 camera_y, legacy_s16 camera_z,
	legacy_s16 use_high_detail, struct TRANSFORMEDSHAPE3D* transformed)
{
	struct TRACKOBJECT* terrain_object;

	if (terrain == 0)
		return;
	terrain_object = &sceneshapes2[terrain];
	transformed->shapeptr = use_high_detail != 0 ?
		terrain_object->ss_shapePtr : terrain_object->ss_loShapePtr;
	transformed->pos.x = track_preview_half(LEGACY_S16_WRAP_SUB(
		trackcenterpos2[column], camera_x));
	transformed->pos.y = track_preview_half(LEGACY_S16_WRAP_SUB(
		height, camera_y));
	transformed->pos.z = track_preview_half(LEGACY_S16_WRAP_SUB(
		trackcenterpos[row], camera_z));
	transformed->rotvec.z = terrain_object->ss_rotY;
	transformed->ts_flags = 5;
	transformed->material = 0;
	transformed_shape_op(transformed);
}

void draw_track_preview(void)
{
	struct TRANSFORMEDSHAPE3D transformed;
	struct TRACKOBJECT* track_object;
	struct TRACKOBJECT* overlay_object;
	struct VECTOR projected_vector;
	struct VECTOR track_position;
	struct POINT2D projected_point;
	struct MATRIX* rotation;
	legacy_s16 camera_x;
	legacy_s16 camera_y;
	legacy_s16 camera_z;
	legacy_s16 camera_angle;
	legacy_s16 camera_radius;
	legacy_s16 horizon;
	legacy_s16 terrain_height;
	legacy_u8 column;
	legacy_u8 row;
	legacy_u8 adjacent_column;
	legacy_u8 adjacent_row;
	legacy_u8 terrain;
	legacy_u8 track;
	legacy_u8 quadrant;

	camera_x = (legacy_s16)word_3C108;
	camera_y = (legacy_s16)word_3C10A;
	camera_z = (legacy_s16)word_3C10C;
	camera_radius = (legacy_s16)polarRadius2D(
		LEGACY_S16_WRAP_SUB(word_3C10E, camera_x),
		LEGACY_S16_WRAP_SUB(word_3C112, camera_z));
	camera_angle = (legacy_s16)polarAngle(
		LEGACY_S16_WRAP_SUB(word_3C110, camera_y), camera_radius);
	rotation = mat_rot_zxy(0, camera_angle, 0, 1);
	mat_mul_vector(&unk_3C114, rotation, &projected_vector);
	vector_to_point(&projected_vector, &projected_point);
	horizon = (legacy_s16)projected_point.py;
	if (horizon < 0)
		horizon = 0;

	sprite_set_1_size(0, 0x140, 0,
		LEGACY_S16_WRAP_SUB(horizon, skybox_current));
	sprite_clear_1_color((legacy_u8)skybox_sky_color);
	sprite_set_1_size(0, 0x140, 0, 0x64);
	sprite_putimage_and_alt(skyboxes[2], 0,
		LEGACY_S16_WRAP_SUB(horizon, skybox_ptr3));
	sprite_putimage_and_alt(skyboxes[3], 0x140,
		LEGACY_S16_WRAP_SUB(horizon, skybox_ptr4));
	sprite_set_1_size(0, 0x140, horizon, 0xC8);
	sprite_clear_1_color((legacy_u8)skybox_grd_color);
	sprite_set_1_size(0, 0x140, 0, 0xC8);
	select_cliprect_rotate(0, camera_angle, 0, &trackpreview_cliprect, 1);

	transformed.rotvec.x = 0;
	transformed.rotvec.y = 0;
	transformed.unk = 0x400;
	for (row = 0; row < 30U; row++) {
		for (column = 0; column < 30U; column++) {
			track = td14_elem_map_main[
				LEGACY_U16_WRAP_ADD(trackrows[row], column)];
			terrain = td15_terr_map_main[
				LEGACY_U16_WRAP_ADD(terrainrows[row], column)];
			if (track != 0 && terrain >= 7U && terrain < 11U) {
				track = subst_hillroad_track(terrain, track);
				terrain = 0;
			}
			if (track >= 0xFDU) {
				track = 0;
				terrain = 0;
			}

			terrain_height = 0;
			if (terrain == 6U) {
				terrain_height = hillHeightConsts[1];
				if (track != 0)
					terrain = 0;
			} else if (track >= 0x69U && track <= 0x6CU) {
				for (quadrant = 0; quadrant < 4U; quadrant++) {
					adjacent_column = (legacy_u8)(column +
						((quadrant & 1U) != 0 ? 1U : 0U));
					adjacent_row = (legacy_u8)(row +
						((quadrant & 2U) != 0 ? 1U : 0U));
					terrain = td15_terr_map_main[
						LEGACY_U16_WRAP_ADD(
							terrainrows[adjacent_row],
							adjacent_column)];
					track_preview_draw_terrain(terrain,
						adjacent_column, adjacent_row, 0,
						camera_x, camera_y, camera_z, 1,
						&transformed);
				}
				terrain = 0;
			}

			track_preview_draw_terrain(terrain, column, row,
				terrain_height, camera_x, camera_y, camera_z, 0,
				&transformed);
			if (track == 0) {
				get_a_poly_info();
				continue;
			}

			track_object = &trkObjectList[track];
			track_position.x = track_preview_half(
				LEGACY_S16_WRAP_SUB(
					(track_object->ss_multiTileFlag & 2U) != 0 ?
					trackpos2[column + 1U] : trackcenterpos2[column],
					camera_x));
			track_position.y = track_preview_half(
				LEGACY_S16_WRAP_SUB(terrain_height, camera_y));
			track_position.z = track_preview_half(
				LEGACY_S16_WRAP_SUB(
					(track_object->ss_multiTileFlag & 1U) != 0 ?
					trackpos[row] : trackcenterpos[row], camera_z));

			if (terrain_height != 0) {
				switch (track_object->ss_multiTileFlag) {
				case 1:
					transformed.shapeptr = &game3dshapes[91];
					break;
				case 2:
					transformed.shapeptr = &game3dshapes[92];
					break;
				case 3:
					transformed.shapeptr = &game3dshapes[93];
					break;
				default:
					transformed.shapeptr = &game3dshapes[43];
					break;
				}
				transformed.pos = track_position;
				transformed.rotvec.z = 0;
				transformed.ts_flags = 5;
				transformed.material = 0;
				transformed_shape_op(&transformed);
			}

			if (track_object->ss_ssOvelay != 0) {
				overlay_object = &trkObjectList[
					track_object->ss_ssOvelay];
				if (overlay_object->ss_loShapePtr != 0) {
					transformed.shapeptr =
						overlay_object->ss_loShapePtr;
					transformed.pos = track_position;
					transformed.rotvec.z = track_object->ss_rotY;
					transformed.ts_flags = 5;
					transformed.material =
						(legacy_s8)overlay_object->ss_surfaceType < 0 ?
						0 : overlay_object->ss_surfaceType;
					transformed_shape_op(&transformed);
				}
			}

			transformed.shapeptr = track_object->ss_loShapePtr;
			transformed.pos = track_position;
			transformed.rotvec.z = track_object->ss_rotY;
			transformed.ts_flags =
				(legacy_u8)(track_object->ss_ignoreZBias | 4U);
			transformed.material =
				(legacy_s8)track_object->ss_surfaceType < 0 ?
				0 : track_object->ss_surfaceType;
			transformed_shape_op(&transformed);
			get_a_poly_info();
		}
	}
}

void init_rect_arrays(void) {
	legacy_s16 i;

	if (slow_video_mgmt_copy == 0)
		return;

	rect_array_unk[0] = rect_unk5;
	rect_array_unk2[0] = rect_unk5;
	for (i = 1; i < 15; i++) {
		rect_array_unk[i] = cliprect_unk;
		rect_array_unk2[i] = cliprect_unk;
	}
}

void font_set_fontdef2(void far* data) {
	set_fontdefseg(data);
	fontdef_unk_0E = LEGACY_S16_FROM_BITS(
		LEGACY_READ_U16_LE((legacy_u8 far*)data + 14U));
}

void font_set_fontdef(void) {
	font_set_fontdef2(fontdefptr);
}

void sub_19F14(struct RECTANGLE* cliprect) {
	struct RECTANGLE* dirty_rect;
	legacy_s16 i;

	if (video_flag5_is0 != 0)
		return;

	sprite_copy_2_to_1_2();
	if (byte_454A4 != 0) {
		mouse_draw_opaque_check();
		sprite_putimage(render_window_sprite->sprite_bitmapptr);
	} else if (slow_video_mgmt_copy == 0) {
		sprite_set_1_size(
			cliprect->left,
			cliprect->right,
			cliprect->top,
			cliprect->bottom);
		mouse_draw_opaque_check();
		sprite_putimage(render_window_sprite->sprite_bitmapptr);
	} else {
		for (i = 0; i < 15; i++)
			rect_array_unk_indices[i] = 3;
		if (detail_level == 4)
			word_449FE = word_463D6;
		if (word_449FE == word_463D6 &&
			rect_array_unk[5].left == rect_array_unk2[5].left &&
			rect_array_unk[5].right == rect_array_unk2[5].right &&
			rect_array_unk[5].top == rect_array_unk2[5].top &&
			rect_array_unk[5].bottom == rect_array_unk2[5].bottom) {
			rect_array_unk_indices[5] = 0;
		}

		rect_array_unk3_length = 0;
		rectlist_add_rects(
			15,
			rect_array_unk_indices,
			rect_array_unk,
			rect_array_unk2,
			cliprect,
			&rect_array_unk3_length,
			rect_array_unk3);
		if (rect_array_unk3_length != 0) {
			rect_array_sort_by_top(
				rect_array_unk3_length,
				rect_array_unk3,
				rect_array_unk3_indices);
			mouse_draw_opaque_check();
			for (i = 0; i < rect_array_unk3_length; i++) {
				dirty_rect = &rect_array_unk3[rect_array_unk3_indices[i]];
				sprite_set_1_size(
					dirty_rect->left,
					dirty_rect->right,
					dirty_rect->top,
					dirty_rect->bottom);
				sprite_putimage(render_window_sprite->sprite_bitmapptr);
			}
		} else {
			sprite_set_1_size(0, 0x140, cliprect->top, cliprect->bottom);
			mouse_draw_opaque_check();
			sprite_putimage(render_window_sprite->sprite_bitmapptr);
		}
	}

	mouse_draw_transparent_check();
	if (slow_video_mgmt_copy != 0) {
		word_449FE = word_463D6;
		for (i = 0; i < 15; i++)
			rect_array_unk2[i] = rect_array_unk[i];
	}
}

void update_frame(legacy_s8 arg_0, struct RECTANGLE* arg_cliprectptr) {
	legacy_s16 si;
	legacy_s8 var_122;
	legacy_s8 var_E4;
	legacy_s8 var_DC[2];
	struct RECTANGLE* var_rectptr;
	struct MATRIX var_mat, var_mat2;
	struct MATRIX* car_rot_matrix;
	struct VECTOR cam_pos, car_pos, offset_vector, car_to_cam_rotated, var_vec8;
	legacy_s16 car_rot_y, car_rot_x, car_rot_z;
	legacy_s16 car_rot_y_2, car_rot_x_2, car_rot_z_2;
	legacy_s16 var_38, car_rot_z_3;
	legacy_s16 var_transformresult;
	legacy_s16 heading;
	legacy_s8* lookahead_tiles;
	legacy_s16 skybox_parameter;
	legacy_s16 var_counter, var_counter2;
	legacy_s8 cam_tile_south, cam_tile_east;
	legacy_s8 tile_south, tile_east;
	legacy_s8 tile_to_draw_south_offset, tile_to_draw_east_offset;
	legacy_s8 car_tile_east, car_tile_south;
	legacy_u8 tiles_to_draw_terr_type_vec[24];
	legacy_s8 should_skip_tile[24];
	legacy_s8 tile_detail_level[24];
	legacy_s8 tiles_to_draw_south[24];
	legacy_s8 tiles_to_draw_east[24];
	legacy_u8 tiles_to_draw_elem_type_vec[24];
	legacy_s8 detail_threshold;
	legacy_s8 var_3C;
	legacy_s8 var_60;
	legacy_s8 var_6E;
	legacy_s8 var_4A;
	legacy_s8 var_4E;
	legacy_s16 var_6C;
	legacy_s16 var_A4;
	legacy_s16 var_hillheight;
	legacy_s16 idx;
	struct TRACKOBJECT* var_trkobjectptr;
	struct TRACKOBJECT* var_trkobject_ptr; // NOTE: beware of similar names!!
	legacy_s8 tile_det_level;
	legacy_s8* var_10E;
	legacy_s16 di;
	legacy_u16 vertex_index;
	legacy_s16 var_132;
	legacy_s16 var_5E;
	legacy_s16 var_3A;
	legacy_s16* var_DA;
	legacy_s16 var_12A;
	legacy_u8 var_4C;
	struct RECTANGLE var_rect, var_rect2;
	struct VECTOR var_108[4];
	struct CARSTATE* var_stateptr;
	legacy_u8 elem_map_value;
	legacy_u8 terr_map_value;

	var_DC[0] = 0;
	var_DC[1] = 0;
	if (video_flag5_is0 == 0 || arg_0 == 0) {
		rectptr_unk = rect_array_unk;
		rectptr_unk2 = rect_array_unk2;
	} else {
		rectptr_unk2 = rect_array_unk;
		rectptr_unk = rect_array_unk2;
	}

	if (slow_video_mgmt_copy != 0) {
		var_122 = 8;
		var_rectptr = rect_unk;
		for (si = 0; si < 15; si++) {
			*var_rectptr = cliprect_unk;
			var_rectptr++;
		}
	} else {
		var_122 = 0;
	}

	// Set car position (own or opponent's)
	if (followOpponentFlag == 0) {		
		car_pos.x = frame_position_word(
			state.playerstate.car_posWorld1.lx);
		car_pos.y = frame_position_word(
			state.playerstate.car_posWorld1.ly);
		car_pos.z = frame_position_word(
			state.playerstate.car_posWorld1.lz);
		car_rot_y = state.playerstate.car_rotate.y;
		car_rot_z = state.playerstate.car_rotate.z;
		car_rot_x = state.playerstate.car_rotate.x;
	} else {
		car_pos.x = frame_position_word(
			state.opponentstate.car_posWorld1.lx);
		car_pos.y = frame_position_word(
			state.opponentstate.car_posWorld1.ly);
		car_pos.z = frame_position_word(
			state.opponentstate.car_posWorld1.lz);
		car_rot_y = state.opponentstate.car_rotate.y;
		car_rot_z = state.opponentstate.car_rotate.z;
		car_rot_x = state.opponentstate.car_rotate.x;
	}

	car_rot_x_2 = -1;
	car_rot_z_2 = 0;
	
	// Set camera position, based on the car position and the camera mode
	if (cameramode == 0) {
		car_rot_x_2 = car_rot_x & 0x3ff;
		car_rot_y_2 = car_rot_y & 0x3ff;
		car_rot_z_2   = car_rot_z & 0x3ff;
		car_rot_matrix = mat_rot_zxy(
			LEGACY_S16_WRAP_NEGATE(car_rot_z),
			LEGACY_S16_WRAP_NEGATE(car_rot_y),
			LEGACY_S16_WRAP_NEGATE(car_rot_x), 0);
		offset_vector.x = 0;
		offset_vector.z = 0;
		offset_vector.y = LEGACY_S16_WRAP_SUB(simd_player.car_height, 6);

		mat_mul_vector(&offset_vector, car_rot_matrix, &car_to_cam_rotated);
		cam_pos.x = LEGACY_S16_WRAP_ADD(
			car_pos.x, car_to_cam_rotated.x);
		cam_pos.y = LEGACY_S16_WRAP_ADD(
			car_pos.y, car_to_cam_rotated.y);
		cam_pos.z = LEGACY_S16_WRAP_ADD(
			car_pos.z, car_to_cam_rotated.z);
	} else if (cameramode == 1) {
		cam_pos.x = state.game_vec1[followOpponentFlag].x;
		cam_pos.z = state.game_vec1[followOpponentFlag].z;
		cam_pos.y = state.game_vec1[followOpponentFlag].y;
	} else if (cameramode == 2) {
		offset_vector.x = 0;
		offset_vector.y = 0;
		offset_vector.z = 0x4000;
		car_rot_matrix = mat_rot_zxy(
			LEGACY_S16_WRAP_NEGATE(car_rot_z),
			LEGACY_S16_WRAP_NEGATE(car_rot_y),
			LEGACY_S16_WRAP_NEGATE(car_rot_x), 0);
		mat_mul_vector(&offset_vector, car_rot_matrix, &car_to_cam_rotated);

		offset_vector.x = 0;
		offset_vector.y = 0;
		offset_vector.z = custom_camera_distance;
		car_rot_matrix = mat_rot_zxy(0,
			LEGACY_S16_WRAP_NEGATE(custom_camera_elevation_angle),
			LEGACY_S16_WRAP_SUB(polarAngle(car_to_cam_rotated.x,
				car_to_cam_rotated.z), custom_camera_azimuth_angle), 0);

		mat_mul_vector(&offset_vector, car_rot_matrix, &car_to_cam_rotated);
		cam_pos.x = LEGACY_S16_WRAP_ADD(car_pos.x, car_to_cam_rotated.x);
		cam_pos.y = LEGACY_S16_WRAP_ADD(car_pos.y, car_to_cam_rotated.y);
		cam_pos.z = LEGACY_S16_WRAP_ADD(car_pos.z, car_to_cam_rotated.z);
	} else if (cameramode == 3) {
		cam_pos.x = trackdata9[state.field_3F7[followOpponentFlag] * 3 + 0];
		cam_pos.y = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
			trackdata9[state.field_3F7[followOpponentFlag] * 3 + 1],
			camera_track_height_offset), 0x5A);
		cam_pos.z = trackdata9[state.field_3F7[followOpponentFlag] * 3 + 2];
	}

	// Unknown part; seems to be performing some initialization
	if (car_rot_x_2 == -1) {
		build_track_object(&cam_pos, &cam_pos);
		if (cam_pos.y < terrainHeight) {
			cam_pos.y = terrainHeight;
		}

		if (byte_4392C != 0) {		
			si = plane_origin_op(planindex, cam_pos.x, cam_pos.y, cam_pos.z);
			if (si < 0xC) {			
				vec_unk2.x = 0;
				vec_unk2.y = LEGACY_S16_WRAP_SUB(0xC, si);
				vec_unk2.z = 0;
				planindex_copy = planindex;
				pState_f36Mminf40sar2 = 0;
				pState_minusRotate_x_2 = 0;
				pState_minusRotate_z_2 = 0;
				pState_minusRotate_y_2 = 0;
				plane_rotate_op();
				cam_pos.x = LEGACY_S16_WRAP_ADD(
					cam_pos.x, vec_planerotopresult.x);
				cam_pos.y = LEGACY_S16_WRAP_ADD(
					cam_pos.y, vec_planerotopresult.y);
				cam_pos.z = LEGACY_S16_WRAP_ADD(
					cam_pos.z, vec_planerotopresult.z);
			}
		}

		car_rot_x_2 = LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_S16_WRAP_NEGATE(polarAngle(
				LEGACY_S16_WRAP_SUB(car_pos.x, cam_pos.x),
				LEGACY_S16_WRAP_SUB(car_pos.z, cam_pos.z))) & 0x3FFU);
		var_38 = polarRadius2D(
			LEGACY_S16_WRAP_SUB(car_pos.x, cam_pos.x),
			LEGACY_S16_WRAP_SUB(car_pos.z, cam_pos.z));
		car_rot_y_2 = LEGACY_S16_FROM_BITS((legacy_u16)polarAngle(
			LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_SUB(car_pos.y, cam_pos.y), 0x32),
			var_38) & 0x3FFU);
	}

	if (car_rot_z_2 > 1 && car_rot_z_2 < 0x3FF) {
		car_rot_z_3 = car_rot_z_2;
	} else {
		car_rot_z_3 = 0;
	}

	if (state.game_frame == 0) {
		var_E4 = byte_3C0C6[frame_callback_count&0xF];
	} else {
		var_E4 = byte_3C0C6[state.game_frame&0xF];
	}

	// Select the vector specifying the 23 tiles to draw. The vector contains
	// 24 elements, each 3 bytes long, in format (east_offset, south_offset,
	// detail threshold). A tile is drawn only if its detail threshold is lower
	// enough (0 = draw always, 1 = only if graphic detail is MEDIUM or FULL,
	// 2 = only if graphic detail is FULL).
	// There are 8 possible vectors, but they are all rotations/reflections of a
	// basic schema. Which is chosen depends on the heading of the car. For a
	// car heading north ($), the schema is the following:
	//
	// OOOOO
	// OOOOO
	// OOOOO
	// OOOOO
	//  O$O
	//
	// Also, note that the tiles appear in the vector in drawing order
	// (farthest tiles first). If a car is heading north but slightly west, the
	// algo will draw the NW tile before the NE, and vice-versa

	heading = select_cliprect_rotate(car_rot_z_3, car_rot_y_2, car_rot_x_2, arg_cliprectptr, 0);
	lookahead_tiles = lookahead_tiles_tables[(heading & 0x3FF) >> 7];

	var_mat = *mat_rot_zxy(car_rot_z_3, car_rot_y_2, 0, 1);
	offset_vector.x = 0;
	offset_vector.y = 0;
	offset_vector.z = 0x3E8;
	mat_mul_vector(&offset_vector, &var_mat, &var_vec8);
	if (var_vec8.z > 0) {
		skybox_parameter = 1;
	} else {
		skybox_parameter = -1;
	}

	// Draw 8 shapes (still TBD what they are), but only if the detail
	// level is the max one
	if (detail_level == 0) {
		currenttransshape->rectptr = &rect_unk9;
		currenttransshape->ts_flags = var_122 | 7;
		currenttransshape->rotvec.x = 0;
		currenttransshape->rotvec.y = 0;
		currenttransshape->unk = 0x400;
		currenttransshape->material = 0;

		for (var_counter = 0; var_counter < 8; var_counter++) {
			si = LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(
					word_3BE34[var_counter], car_rot_x_2),
				run_game_random) & 0x3FFU);
			if (si < 0x87 || si > 0x379) {
				mat_rot_y(&var_mat2, si);
				offset_vector.x = 0;
				offset_vector.y = LEGACY_S16_WRAP_SUB(0xAE6, cam_pos.y);
				offset_vector.z = 0x3A98; //15000
				mat_mul_vector(&offset_vector, &var_mat2, &car_to_cam_rotated);
				car_to_cam_rotated.z = 0x3A98; //15000
				mat_mul_vector(&car_to_cam_rotated, &var_mat, &currenttransshape->pos);
				if (currenttransshape->pos.z > 0xC8) {
					currenttransshape->shapeptr = off_3BE44[var_counter];
					currenttransshape->rotvec.z =
						LEGACY_S16_WRAP_NEGATE(car_rot_x_2);
					var_transformresult = transformed_shape_op(&currenttransshape[0]);
					(void) var_transformresult; // we cannot be out of memory as we are just starting to process
				}
			}
		}
	}

/*
; -----------------------------------------------------------------------------------------------
*/

	cam_tile_east = LEGACY_S8_FROM_BITS(
		(legacy_u8)LEGACY_S16_SAR(cam_pos.x, 10U));
	cam_tile_south = LEGACY_S8_WRAP_SUB(0x1D,
		LEGACY_S16_SAR(cam_pos.z, 10U));
	if (detail_level != 0) {
		car_tile_east = frame_tile_from_world(
			state.playerstate.car_posWorld1.lx);
		car_tile_south = frame_south_tile_from_world(
			state.playerstate.car_posWorld1.lz);
	}

	for (si = 0; si < 0x17; si++) {
		should_skip_tile[si] = 0;
	}

	// Select the detail level (FULL if 1st or 2nd option in the graphics menu
	// were chosen, MEDIUM if the 3rd, FASTEST if 4th or 5th)
	detail_threshold = detail_threshold_by_level[detail_level];
	
	// Cycle on the 23 tiles to draw, determine if they really need to be drawn
	for (si = 0x16; si >= 0; si--) {

		// Skip if a previous iteration determined this tile is not needed
		// (happens for multi-tile elements)
		if (should_skip_tile[si] != 0)
			continue;

		// Skip if detail threshold not met (e.g. far tiles in FASTEST detail)
		if (lookahead_tiles[si * 3 + 2] <= detail_threshold) {
			tile_east = LEGACY_S8_WRAP_ADD(
				lookahead_tiles[si * 3], cam_tile_east);
			tile_south = LEGACY_S8_WRAP_ADD(
				lookahead_tiles[si * 3 + 1], cam_tile_south);

			// Skip if tile is out of bounds
			if (tile_east >= 0 && tile_east <= 0x1D && tile_south >= 0 && tile_south <= 0x1D) {
				elem_map_value = td14_elem_map_main[tile_east + trackrows[tile_south]];
				terr_map_value = td15_terr_map_main[tile_east + terrainrows[tile_south]];
				
				if (elem_map_value != 0) {

					if (terr_map_value >= 7 && terr_map_value < 0xB) {
						elem_map_value = subst_hillroad_track(terr_map_value, elem_map_value);
						terr_map_value = 0;
					}

					// Found a filler tile (non-main tile of a multitile component)
					// Process the main tile of the component instead (the NW one)
					if (elem_map_value == 0xFD) {
						tile_east = LEGACY_S8_WRAP_SUB(tile_east, 1);
						tile_south = LEGACY_S8_WRAP_SUB(tile_south, 1);
						elem_map_value = td14_elem_map_main[tile_east + trackrows[tile_south]];
						terr_map_value = td15_terr_map_main[tile_east + terrainrows[tile_south]];
					} else if (elem_map_value == 0xFE) {
						tile_south = LEGACY_S8_WRAP_SUB(tile_south, 1);
						elem_map_value = td14_elem_map_main[tile_east + trackrows[tile_south]];
						terr_map_value = td15_terr_map_main[tile_east + terrainrows[tile_south]];
					} else if (elem_map_value == 0xFF) {
						tile_east = LEGACY_S8_WRAP_SUB(tile_east, 1);
						elem_map_value = td14_elem_map_main[tile_east + trackrows[tile_south]];
						terr_map_value = td15_terr_map_main[tile_east + terrainrows[tile_south]];
					}
				}

				tiles_to_draw_terr_type_vec[si] = terr_map_value;
				tile_detail_level[si] = lookahead_tiles[si * 3 + 2];

				if (elem_map_value != 0 && detail_level != 0 &&
					trkObjectList[elem_map_value].ss_physicalModel >= 0x40 &&
					(tile_east != car_tile_east || tile_south != car_tile_south))
				{
					elem_map_value = 0;
				}

				tiles_to_draw_east[si] = tile_east;
				tiles_to_draw_south[si] = tile_south;
				tiles_to_draw_elem_type_vec[si] = elem_map_value;

				if (elem_map_value != 0) {
					idx = trkObjectList[elem_map_value].ss_multiTileFlag;
					if (idx != 0) {
						// Look the future tiles to process (i.e. with lower index, since si
						// counts backwards) and remove those which belong to the same
						// multi-tile component as this tile

						// Recalculate the offset (needed in case we hit a filler tile)
						tile_to_draw_east_offset = LEGACY_S8_WRAP_SUB(
							tile_east, cam_tile_east);
						tile_to_draw_south_offset = LEGACY_S8_WRAP_SUB(
							tile_south, cam_tile_south);
						if (idx == 1) {
							for (di = 0; di < si; di++) {
								if (lookahead_tiles[di * 3] == tile_to_draw_east_offset && (lookahead_tiles[di * 3 + 1] == tile_to_draw_south_offset || lookahead_tiles[di * 3 + 1] == tile_to_draw_south_offset + 1)) {
									should_skip_tile[di] = 1;
								}
							}
						} else if (idx == 2) {
							for (di = 0; di < si; di++) {
								if (lookahead_tiles[si * 3 + 1] == tile_to_draw_south_offset && (lookahead_tiles[si * 3] == tile_to_draw_east_offset || lookahead_tiles[si * 3] != tile_to_draw_east_offset + 1)) {
									should_skip_tile[di] = 1;
								}
							}
						} else if (idx == 3) {
							for (di = 0; di < si; di++) {
								if ((lookahead_tiles[di * 3] == tile_to_draw_east_offset || lookahead_tiles[di * 3] == tile_to_draw_east_offset + 1) &&
									(lookahead_tiles[di * 3 + 1] == tile_to_draw_south_offset || lookahead_tiles[di * 3 + 1] == tile_to_draw_south_offset + 1))
								{
									should_skip_tile[di] = 1;
								}
							}
						}
					}
				}
				
			} else {
				should_skip_tile[si] = 2;
			}
		} else {
			should_skip_tile[si] = 2;
		}
	}
	
//; -----------------------------------------------------------------------------
	
	// Draw own wheels
	var_3C = -1;
	var_6C = 0;
	if (cameramode != 0 || followOpponentFlag != 0) {

		if (state.playerstate.car_crashBmpFlag != 2) {

			car_rot_matrix = mat_rot_zxy(
				LEGACY_S16_WRAP_NEGATE(state.playerstate.car_rotate.z),
				LEGACY_S16_WRAP_NEGATE(state.playerstate.car_rotate.y),
				LEGACY_S16_WRAP_NEGATE(state.playerstate.car_rotate.x), 0);
			idx = -1;
			di = -1;
			for (var_counter2 = 0; var_counter2 < 4; var_counter2++) {
				offset_vector = simd_player.wheel_coords[var_counter2];
				mat_mul_vector(&offset_vector, car_rot_matrix, &var_vec8); //; rotating car wheels, maybe?
				// Tile where the wheel is standing
				tile_east = frame_tile_from_world_offset(
					state.playerstate.car_posWorld1.lx, var_vec8.x);
				tile_south = frame_south_tile_from_world_offset(
					state.playerstate.car_posWorld1.lz, var_vec8.z);

				for (si = 0x16; si > idx; si--) {
					if (should_skip_tile[si] != 2 && lookahead_tiles[si * 3] + cam_tile_east == tile_east && lookahead_tiles[si * 3 + 1] + cam_tile_south == tile_south) {
						var_3C = tile_east;
						var_60 = tile_south;
						idx = si;
						di = var_counter2;
					}
				}
			}

			if (di != -1) {
				if (state.playerstate.car_surfaceWhl[0] != 4 || state.playerstate.car_surfaceWhl[1] != 4 || state.playerstate.car_surfaceWhl[2] != 4 || state.playerstate.car_surfaceWhl[3] != 4) {
					offset_vector.x = 0;
					offset_vector.z = 0;
					offset_vector.y = 0x7530;
					mat_mul_vector(&offset_vector, car_rot_matrix, &var_vec8);
					mat_mul_vector(&var_vec8, &mat_temp, &offset_vector);
					if (offset_vector.z <= 0) {
						var_6C = -0x800 ;
					} else {
						var_6C = 0x800;
					}
				}
			}
		}
	}

	// Draw opponent's wheels
	var_4A = -1;
	var_A4 = 0;
	if (gameconfig.game_opponenttype != 0) {

		if (cameramode != 0 || followOpponentFlag == 0) {
			if (state.opponentstate.car_crashBmpFlag != 2) {
				car_rot_matrix = mat_rot_zxy(
					LEGACY_S16_WRAP_NEGATE(
						state.opponentstate.car_rotate.z),
					LEGACY_S16_WRAP_NEGATE(
						state.opponentstate.car_rotate.y),
					LEGACY_S16_WRAP_NEGATE(
						state.opponentstate.car_rotate.x), 0);
				idx = -1;
				di = -1;

				for (var_counter2 = 0; var_counter2 < 4; var_counter2++) {
					offset_vector = simd_opponent.wheel_coords[var_counter2];
					mat_mul_vector(&offset_vector, car_rot_matrix, &var_vec8); //; rotating car wheels, maybe?
					tile_east = frame_tile_from_world_offset(
						state.opponentstate.car_posWorld1.lx, var_vec8.x);
					tile_south = frame_south_tile_from_world_offset(
						state.opponentstate.car_posWorld1.lz, var_vec8.z);

					for (si = 0x16; si > idx; si--) {
						if (should_skip_tile[si] != 2 && lookahead_tiles[si * 3] + cam_tile_east == tile_east && lookahead_tiles[si * 3 + 1] + cam_tile_south == tile_south) {
							var_4A = tile_east;
							var_6E = tile_south;
							idx = si;
							di = var_counter2;
						}
					}
				}

				if (di != -1) {
						
					if (state.opponentstate.car_surfaceWhl[0] != 4 || state.opponentstate.car_surfaceWhl[1] != 4 || state.opponentstate.car_surfaceWhl[2] != 4 || state.opponentstate.car_surfaceWhl[3] != 4) {
						offset_vector.x = 0;
						offset_vector.z = 0;
						offset_vector.y = 0x7530;
						mat_mul_vector(&offset_vector, car_rot_matrix, &var_vec8);
						mat_mul_vector(&var_vec8, &mat_temp, &offset_vector);
						if (offset_vector.z <= 0) {
							var_A4 = -0x800; //0xF800; // signed number!
						} else {
							var_A4 = 0x800;
						}
					}
				}
			}
		}
	}
//; -----------------------------------------------------------------------------


	var_4E = 0;
	si = 0;
	
	// With the information collected by the previus tile-scan algorithm,
	// proceed to draw the shapes in each tile. Start from the farthest
	// (painter's algorithm)
	for (si = 0; si < 0x17; si++) {
		if (should_skip_tile[si] != 0) {
			continue;
		}
		tile_east = tiles_to_draw_east[si];
		tile_south = tiles_to_draw_south[si];
		elem_map_value = tiles_to_draw_elem_type_vec[si];
		terr_map_value = tiles_to_draw_terr_type_vec[si];
		tile_det_level = tile_detail_level[si];
		var_12A = 0;
		if (elem_map_value == 0) {
			var_counter = 1;
			var_10E = unk_3C0F4;
		} else {
			var_trkobject_ptr = &trkObjectList[elem_map_value];
			if (var_trkobject_ptr->ss_multiTileFlag == 0) {
				var_counter = 1;
				var_10E = unk_3C0EE;
			} else if (var_trkobject_ptr->ss_multiTileFlag == 1) {
				var_counter = 2;
				var_10E = unk_3C0F0;
			} else if (var_trkobject_ptr->ss_multiTileFlag == 2) {
				var_counter = 3;
				var_10E = unk_3C0F4;
			} else if (var_trkobject_ptr->ss_multiTileFlag == 3) {
				var_counter = 4;
				var_10E = unk_3C0F8;
			}
		}

		// Draw the fence
		for (idx = 0; idx < var_counter; idx++) {
			tile_to_draw_east_offset = LEGACY_S8_WRAP_ADD(
				var_10E[idx * 2], tile_east);
			tile_to_draw_south_offset = LEGACY_S8_WRAP_ADD(
				var_10E[idx * 2 + 1], tile_south);
			
			if (detail_level == 0 || (tile_to_draw_east_offset == car_tile_east && tile_to_draw_south_offset == car_tile_south)) {
				if (tile_to_draw_east_offset == 0) {
					if (tile_to_draw_south_offset == 0) {
						di = 7;
					} else if (tile_to_draw_south_offset == 0x1D) {
						di = 5;
					} else {
						di = 6;
					}
				} else if (tile_to_draw_east_offset == 0x1D) {
					if (tile_to_draw_south_offset == 0) {
						di = 1;
					} else
					if (tile_to_draw_south_offset == 0x1D) {
						di = 3;
					} else {
						di = 2;
					}
				} else {
					if (tile_to_draw_south_offset == 0) {
						di = 0;
					} else if (tile_to_draw_south_offset == 0x1D) {
						di = 4;
					} else {
						di = -1;
					}
				}

				if (di != -1) {
					var_trkobjectptr = &trkObjectList[fence_TrkObjCodes[di]];
					if (tile_det_level == 0) {
						currenttransshape->shapeptr = var_trkobjectptr->ss_shapePtr;
					} else {
						currenttransshape->shapeptr = var_trkobjectptr->ss_loShapePtr;
					}

					currenttransshape->pos.x = LEGACY_S16_WRAP_SUB(
						trackcenterpos2[tile_to_draw_east_offset], cam_pos.x);
					currenttransshape->pos.y = LEGACY_S16_WRAP_NEGATE(cam_pos.y);
					currenttransshape->pos.z = LEGACY_S16_WRAP_SUB(
						trackcenterpos[tile_to_draw_south_offset], cam_pos.z);
					currenttransshape->rectptr = &rect_unk2;
					currenttransshape->ts_flags = var_122 | 5;
					currenttransshape->rotvec.x = 0;
					currenttransshape->rotvec.y = 0;
					currenttransshape->rotvec.z = word_3C0D6[di];
					currenttransshape->unk = 0x400;
					currenttransshape->material = 0;
					var_transformresult = transformed_shape_op(&currenttransshape[0]);
					if (var_transformresult > 0) {
						// if the return value is > 0, we are out of memory
						// for the polygons, so the rendering is interrupted.
						// Note that (since we start from afar) this means that
						// if the scene is too complex only the far objects
						// will be drawn, while our car and its immediate
						// surroundings will be invisible. Luckily, it does not
						// happen often
						break;
					}
				}
			}
		}

		// terrain type 0x06: a flat piece of land at an elevated level  
		if (terr_map_value != 6) {
			var_hillheight = 0;

			// Special treatment of elevated corners
			if (elem_map_value >= 0x69 && elem_map_value <= 0x6C) {
				for (idx = 0; idx < 4; idx++) {
					if (idx == 0) {
						tile_to_draw_east_offset = tile_east;
						tile_to_draw_south_offset = tile_south;
					} else if (idx == 1) {
						tile_to_draw_east_offset = LEGACY_S8_WRAP_ADD(
							tile_east, 1);
						tile_to_draw_south_offset = tile_south;
					} else if (idx == 2) {
						tile_to_draw_east_offset = tile_east;
						tile_to_draw_south_offset = LEGACY_S8_WRAP_ADD(
							tile_south, 1);
					} else if (idx == 3) {
						tile_to_draw_east_offset = LEGACY_S8_WRAP_ADD(
							tile_east, 1);
						tile_to_draw_south_offset = LEGACY_S8_WRAP_ADD(
							tile_south, 1);
					}
					terr_map_value = td15_terr_map_main[tile_to_draw_east_offset + terrainrows[tile_to_draw_south_offset]];
					if (terr_map_value != 0) {
						var_trkobject_ptr = &sceneshapes2[terr_map_value];
						currenttransshape->shapeptr = var_trkobject_ptr->ss_shapePtr;
						currenttransshape->pos.x = LEGACY_S16_WRAP_SUB(
							trackcenterpos2[tile_to_draw_east_offset], cam_pos.x);
						currenttransshape->pos.y = LEGACY_S16_WRAP_NEGATE(cam_pos.y);
						currenttransshape->pos.z = LEGACY_S16_WRAP_SUB(
							trackcenterpos[tile_to_draw_south_offset], cam_pos.z);
						currenttransshape->rectptr = &rect_unk2;
						currenttransshape->ts_flags = var_122 | 5;
						currenttransshape->rotvec.x = 0;
						currenttransshape->rotvec.y = 0;
						currenttransshape->rotvec.z = var_trkobject_ptr->ss_rotY;
						currenttransshape->unk = 0x400;
						currenttransshape->material = 0;
						var_transformresult = transformed_shape_op(&currenttransshape[0]);
						if (var_transformresult > 0)
							break;
					}
				}
				
				terr_map_value = 0;
			}
		} else {
			var_hillheight = hillHeightConsts[1];
			if (elem_map_value != 0) {
				terr_map_value = 0;
			}
		}

		// The rest of the rendering loop still needs to be analyzed in detail.
		// Anyway, the gist is that every tile is associated with various shape,
		// each of which is rendered via a call to `transformed_shape_op`. The
		// result of such fn is checked each time, since a return value of 1
		// means we ran out of memory

		if (terr_map_value != 0) {
			var_trkobject_ptr = &sceneshapes2[terr_map_value];
			currenttransshape->shapeptr = var_trkobject_ptr->ss_shapePtr;
			currenttransshape->pos.x = LEGACY_S16_WRAP_SUB(
				trackcenterpos2[tile_east], cam_pos.x);
			currenttransshape->pos.y = LEGACY_S16_WRAP_SUB(
				var_hillheight, cam_pos.y);
			currenttransshape->pos.z = LEGACY_S16_WRAP_SUB(
				trackcenterpos[tile_south], cam_pos.z);
			if (var_hillheight == 0) {
				currenttransshape->rectptr = &rect_unk2;
			} else {
				currenttransshape->rectptr = &rect_unk6;
			}

			currenttransshape->ts_flags = var_122 | 5;
			currenttransshape->rotvec.x = 0;
			currenttransshape->rotvec.y = 0;
			currenttransshape->rotvec.z = var_trkobject_ptr->ss_rotY;
			currenttransshape->unk = 0x400;
			currenttransshape->material = 0;
			var_transformresult = transformed_shape_op(&currenttransshape[0]);
			if (var_transformresult > 0)
				break;
		}

		transformedshape_counter = 0;
		curtransshape_ptr = currenttransshape;
		if (elem_map_value == 0) {
			tile_to_draw_east_offset = tile_east;
			tile_to_draw_south_offset = tile_south;
		} else {
			var_trkobject_ptr = &trkObjectList[elem_map_value];
			if ((var_trkobject_ptr->ss_multiTileFlag & 1) != 0) {
				var_5E = trackpos[tile_south];
				tile_to_draw_south_offset = LEGACY_S8_WRAP_ADD(
					tile_south, 1);
			} else {
				var_5E = trackcenterpos[tile_south];
				tile_to_draw_south_offset = tile_south;
			}

			if ((var_trkobject_ptr->ss_multiTileFlag & 2) != 0) {
				var_3A = trackpos2[LEGACY_S8_WRAP_ADD(tile_east, 1)];
				tile_to_draw_east_offset = LEGACY_S8_WRAP_ADD(
					tile_east, 1);
			} else {
				var_3A = trackcenterpos2[tile_east];
				tile_to_draw_east_offset = tile_east;
			}

			var_vec8.x = LEGACY_S16_WRAP_SUB(var_3A, cam_pos.x);
			var_vec8.y = LEGACY_S16_WRAP_SUB(var_hillheight, cam_pos.y);
			var_vec8.z = LEGACY_S16_WRAP_SUB(var_5E, cam_pos.z);
			if (var_hillheight != 0) {
				if (var_trkobject_ptr->ss_multiTileFlag == 0) {
					di = 1;
					var_DA = unk_3C0A2;
				} else if (var_trkobject_ptr->ss_multiTileFlag == 1) {
					di = 2;
					var_DA = unk_3C0A6;
				} else if (var_trkobject_ptr->ss_multiTileFlag == 2) {
					di = 2;
					var_DA = unk_3C0AE;
				} else if (var_trkobject_ptr->ss_multiTileFlag == 3) {
					di = 4;
					var_DA = unk_3C0B6;
				}

				for (idx = 0; idx < di; idx++) {
					currenttransshape->pos.x = LEGACY_S16_WRAP_ADD(
						*var_DA, var_vec8.x);
					var_DA++;
					currenttransshape->pos.y = var_vec8.y;
					currenttransshape->pos.z = LEGACY_S16_WRAP_ADD(
						*var_DA, var_vec8.z);
					var_DA++;
					currenttransshape->shapeptr = &game3dshapes[0x3B2 / sizeof(struct SHAPE3D)];
					currenttransshape->rectptr = &rect_unk6;
					currenttransshape->ts_flags = var_122 | 5;
					currenttransshape->rotvec.x = 0;
					currenttransshape->rotvec.y = 0;
					currenttransshape->rotvec.z = 0;
					currenttransshape->unk = 0x800;
					currenttransshape->material = 0;
					var_transformresult = transformed_shape_op(&currenttransshape[0]);
					if (var_transformresult > 0)
						break;
				}
			}

			if (var_trkobject_ptr->ss_ssOvelay != 0) {
				var_trkobjectptr = &trkObjectList[var_trkobject_ptr->ss_ssOvelay];
				if (tile_det_level != 0) {
					currenttransshape[1].shapeptr = var_trkobjectptr->ss_loShapePtr;
				} else {
					currenttransshape[1].shapeptr = var_trkobjectptr->ss_shapePtr;
				}

				if (currenttransshape[1].shapeptr != 0) {
					currenttransshape[1].pos = var_vec8;
					currenttransshape[1].rotvec.x = 0;
					currenttransshape[1].rotvec.y = 0;
					currenttransshape[1].rotvec.z = var_trkobjectptr->ss_rotY;
					if (var_trkobjectptr->ss_multiTileFlag != 0) {
						currenttransshape[1].unk = 0x400;
					} else {
						currenttransshape[1].unk = 0x800;
					}

					if (var_trkobjectptr->ss_surfaceType >= 0) {
						currenttransshape[1].material = var_trkobjectptr->ss_surfaceType;
					} else {
						currenttransshape[1].material = var_E4;
					}

					currenttransshape[1].ts_flags = var_trkobjectptr->ss_ignoreZBias | var_122 | 4;
					if ((currenttransshape[1].ts_flags & 1) != 0) {
						currenttransshape[1].rectptr = &rect_unk2;
						var_transformresult = transformed_shape_op(&currenttransshape[1]);
						if (var_transformresult > 0)
							break;
					} else {
						currenttransshape[1].rectptr = &rect_unk6;
						var_4E = 1;
					}
				}
			}

			if (tile_det_level != 0) {
				currenttransshape->shapeptr = var_trkobject_ptr->ss_loShapePtr;
			} else {
				currenttransshape->shapeptr = var_trkobject_ptr->ss_shapePtr;
			}

			currenttransshape->pos = var_vec8; // whatever
			currenttransshape->rotvec.x = 0;
			currenttransshape->rotvec.y = 0;
			currenttransshape->rotvec.z = var_trkobject_ptr->ss_rotY;
			if (var_trkobject_ptr->ss_multiTileFlag != 0) {
				currenttransshape->unk = 0x400;
			} else {
				currenttransshape->unk = 0x800;
			}

			currenttransshape->ts_flags = var_trkobject_ptr->ss_ignoreZBias | var_122 | 4;
			if (var_trkobject_ptr->ss_surfaceType >= 0) {
				currenttransshape->material = var_trkobject_ptr->ss_surfaceType;
			} else {
				currenttransshape->material = var_E4;
			}

			if ((var_trkobject_ptr->ss_ignoreZBias & 1) != 0) {
				currenttransshape->rectptr = &rect_unk2;
				var_transformresult = transformed_shape_op(&currenttransshape[0]);
				if (var_transformresult > 0)
					break;
			} else {
				currenttransshape->rectptr = &rect_unk6;
				transformed_shape_add_for_sort(0, 0);
				if (var_4E != 0) {
					var_4E = 0;
					transformed_shape_add_for_sort(-0x800 /*0xF800*/, 0);
					if (var_6C != 0) {
						var_6C = -0x400;//0xFC00;
					}

					if (var_A4 != 0) {
					var_A4 = LEGACY_S16_WRAP_SUB(var_A4, 0x400);
					}
				}

				if (tile_east == startcol2 && tile_south == startrow2) {
					var_12A = 0;
				} else {
					var_12A = -1;
				}
			}

			var_4C = trackdata19[tile_east + trackrows[tile_south]];
			if (var_4C != 0xFF) {
				if (state.field_3FA[var_4C] == 0) {
					var_trkobject_ptr = &trkObjectList[212 + trackdata23[var_4C]];
					curtransshape_ptr->pos.x = LEGACY_S16_WRAP_SUB(
						td10_track_check_rel[var_4C * 3], cam_pos.x);
					curtransshape_ptr->pos.y = LEGACY_S16_WRAP_SUB(
						td10_track_check_rel[var_4C * 3 + 1], cam_pos.y);
					curtransshape_ptr->pos.z = LEGACY_S16_WRAP_SUB(
						td10_track_check_rel[var_4C * 3 + 2], cam_pos.z);
					curtransshape_ptr->shapeptr = var_trkobject_ptr->ss_shapePtr;
					curtransshape_ptr->rectptr = &rect_unk6;
					curtransshape_ptr->ts_flags = var_122 | 4;
					curtransshape_ptr->rotvec.x = 0;
					curtransshape_ptr->rotvec.y = 0;
					curtransshape_ptr->rotvec.z = td08_direction_related[var_4C];
					curtransshape_ptr->unk = 0x64;
					curtransshape_ptr->material = 0;
					transformed_shape_add_for_sort(0, 0);
				} else if (state.field_42A != 0) {
					for (di = 0; di < 0x18; di++) {
						if (state.field_38E[di] != 0 && var_4C + 2 == state.field_443[di]) {
							var_trkobject_ptr = &sceneshapes3[state.field_42B[di]];
							curtransshape_ptr->pos.x = frame_relative_track_position(
								state.game_longs1[di],
								td10_track_check_rel[var_4C * 3], cam_pos.x);
							curtransshape_ptr->pos.y = frame_relative_track_position(
								state.game_longs2[di],
								td10_track_check_rel[var_4C * 3 + 1], cam_pos.y);
							curtransshape_ptr->pos.z = frame_relative_track_position(
								state.game_longs3[di],
								td10_track_check_rel[var_4C * 3 + 2], cam_pos.z);
							curtransshape_ptr->shapeptr = var_trkobject_ptr->ss_shapePtr;
							curtransshape_ptr->rectptr = &rect_unk6;
							curtransshape_ptr->ts_flags = var_122 | 5;
							curtransshape_ptr->rotvec.x = LEGACY_S16_WRAP_NEGATE(
								state.field_2FE[di]);
							curtransshape_ptr->rotvec.y = LEGACY_S16_WRAP_NEGATE(
								state.field_32E[di]);
							curtransshape_ptr->rotvec.z = LEGACY_S16_WRAP_NEGATE(
								state.field_35E[di]);
							curtransshape_ptr->unk = 0x400;
							curtransshape_ptr->material = 0;
							transformed_shape_add_for_sort(0, 0);
						}
					}
				}
			}
		}

		if ((var_3C == tile_east || var_3C == tile_to_draw_east_offset) && (var_60 == tile_south || var_60 == tile_to_draw_south_offset)) {
			if (state.field_42A != 0) {
				for (di = 0; di < 0x18; di++) {
					if (state.field_38E[di] != 0 && state.field_443[di] == 0) {
						var_trkobject_ptr = &sceneshapes3[state.field_42B[di]];
						curtransshape_ptr->pos.x = frame_relative_position_sum(
							state.game_longs1[di],
							state.playerstate.car_posWorld1.lx, cam_pos.x);
						curtransshape_ptr->pos.y = frame_relative_position_sum(
							state.game_longs2[di],
							state.playerstate.car_posWorld1.ly, cam_pos.y);
						curtransshape_ptr->pos.z = frame_relative_position_sum(
							state.game_longs3[di],
							state.playerstate.car_posWorld1.lz, cam_pos.z);
						curtransshape_ptr->shapeptr = var_trkobject_ptr->ss_shapePtr;
						curtransshape_ptr->rectptr = &rect_unk6;
						curtransshape_ptr->ts_flags = var_122 | 5;
						curtransshape_ptr->rotvec.x = LEGACY_S16_WRAP_NEGATE(
							state.field_2FE[di]);
						curtransshape_ptr->rotvec.y = LEGACY_S16_WRAP_NEGATE(
							state.field_32E[di]);
						curtransshape_ptr->rotvec.z = LEGACY_S16_WRAP_NEGATE(
							state.field_35E[di]);
						curtransshape_ptr->unk = 0x400;
						curtransshape_ptr->material = gameconfig.game_playermaterial;
						transformed_shape_add_for_sort(var_6C & var_12A, 0);
					}
				}
			}

			var_trkobject_ptr = &trkObjectList[2];//0x1C / sizeof(struct TRACKOBJECT)];
			curtransshape_ptr->pos.x = frame_relative_position(
				state.playerstate.car_posWorld1.lx, cam_pos.x);
			curtransshape_ptr->pos.y = frame_relative_position(
				state.playerstate.car_posWorld1.ly, cam_pos.y);
			curtransshape_ptr->pos.z = frame_relative_position(
				state.playerstate.car_posWorld1.lz, cam_pos.z);
			
			if (tile_det_level != 0 || detail_level > 2) {
				curtransshape_ptr->shapeptr = var_trkobject_ptr->ss_loShapePtr;
			} else {
				curtransshape_ptr->shapeptr = var_trkobject_ptr->ss_shapePtr;
				sub_204AE(&game3dshapes[0x0AD4 / sizeof(struct SHAPE3D)],
					8U, state.playerstate.car_steeringAngle,
					state.playerstate.car_rc2, word_443E8,
					carshapevecs, carshapevec);
			}

			if (slow_video_mgmt_copy != 0) {
				curtransshape_ptr->rectptr = &rect_unk12;
				curtransshape_ptr->ts_flags = 0xC;
			} else if (state.playerstate.car_crashBmpFlag != 1) {
				curtransshape_ptr->ts_flags = 4;
			} else {
				var_rect = cliprect_unk;
				curtransshape_ptr->rectptr = &var_rect;
				curtransshape_ptr->ts_flags = 0xC;
			}

			curtransshape_ptr->rotvec.x = LEGACY_S16_WRAP_NEGATE(
				state.playerstate.car_rotate.z);
			curtransshape_ptr->rotvec.y = LEGACY_S16_WRAP_NEGATE(
				state.playerstate.car_rotate.y);
			curtransshape_ptr->rotvec.z = LEGACY_S16_WRAP_NEGATE(
				state.playerstate.car_rotate.x);
			curtransshape_ptr->unk = 0x12C;
			curtransshape_ptr->material = gameconfig.game_playermaterial;
			transformed_shape_add_for_sort(var_6C & var_12A, 2);
		}
		
		if ((var_4A == tile_east) || (var_4A == tile_to_draw_east_offset)) {
			if ((var_6E == tile_south) || (var_6E == tile_to_draw_south_offset)) {
				if (state.field_42A != 0) {
					for (di = 0; di < 0x18; di++) {
						if (state.field_38E[di] != 0) {
							if (state.field_443[di] == 1) {
								var_trkobject_ptr = &sceneshapes3[state.field_42B[di]];
								curtransshape_ptr->pos.x = frame_relative_position_sum(
									state.game_longs1[di],
									state.opponentstate.car_posWorld1.lx, cam_pos.x);
								curtransshape_ptr->pos.y = frame_relative_position_sum(
									state.game_longs2[di],
									state.opponentstate.car_posWorld1.ly, cam_pos.y);
								curtransshape_ptr->pos.z = frame_relative_position_sum(
									state.game_longs3[di],
									state.opponentstate.car_posWorld1.lz, cam_pos.z);
								curtransshape_ptr->shapeptr = var_trkobject_ptr->ss_shapePtr;
								curtransshape_ptr->rectptr = &rect_unk6;
								curtransshape_ptr->ts_flags = var_122 | 5;
								curtransshape_ptr->rotvec.x = LEGACY_S16_WRAP_NEGATE(
									state.field_2FE[di]);
								curtransshape_ptr->rotvec.y = LEGACY_S16_WRAP_NEGATE(
									state.field_32E[di]);
								curtransshape_ptr->rotvec.z = LEGACY_S16_WRAP_NEGATE(
									state.field_35E[di]);
								curtransshape_ptr->unk = 0x400;
								curtransshape_ptr->material = gameconfig.game_opponentmaterial;
								transformed_shape_add_for_sort(var_A4 & var_12A, 0);
							}
						}
					}
				}
				var_trkobject_ptr = &trkObjectList[3];//0x2A / sizeof(struct TRACKOBJECT)];
				curtransshape_ptr->pos.x = frame_relative_position(
					state.opponentstate.car_posWorld1.lx, cam_pos.x);
				curtransshape_ptr->pos.y = frame_relative_position(
					state.opponentstate.car_posWorld1.ly, cam_pos.y);
				curtransshape_ptr->pos.z = frame_relative_position(
					state.opponentstate.car_posWorld1.lz, cam_pos.z);

				if (tile_det_level != 0 || detail_level > 2) {
					curtransshape_ptr->shapeptr = var_trkobject_ptr->ss_loShapePtr;
				} else {
					curtransshape_ptr->shapeptr = var_trkobject_ptr->ss_shapePtr;
					sub_204AE(&game3dshapes[0x0AEA / sizeof(struct SHAPE3D)],
						8U, state.opponentstate.car_steeringAngle,
						state.opponentstate.car_rc2, word_4448A,
						oppcarshapevecs, oppcarshapevec);
				}

				if (slow_video_mgmt_copy != 0) {
					curtransshape_ptr->rectptr = &rect_unk15;
					curtransshape_ptr->ts_flags = 0xC;
				} else {
					if (state.opponentstate.car_crashBmpFlag != 1) {
						curtransshape_ptr->ts_flags = 4;
					} else {
						var_rect2 = cliprect_unk;
						curtransshape_ptr->rectptr = &var_rect2;
						curtransshape_ptr->ts_flags = 0xC;
					}
				}

				curtransshape_ptr->rotvec.x = LEGACY_S16_WRAP_NEGATE(
					state.opponentstate.car_rotate.z);
				curtransshape_ptr->rotvec.y = LEGACY_S16_WRAP_NEGATE(
					state.opponentstate.car_rotate.y);
				curtransshape_ptr->rotvec.z = LEGACY_S16_WRAP_NEGATE(
					state.opponentstate.car_rotate.x);
				curtransshape_ptr->unk = 0x12C;
				curtransshape_ptr->material = gameconfig.game_opponentmaterial;
				transformed_shape_add_for_sort(var_A4 & var_12A, 3);
			}
		}

		if (state.game_inputmode == 0) {
			if ((tile_east == startcol2 || tile_to_draw_east_offset == startcol2) && (tile_south == startrow2 || tile_to_draw_south_offset == startrow2)) {

				idx = multiply_and_scale(cos_fast(word_44DCA), 0x24);
				var_counter = LEGACY_S16_WRAP_ADD(
					multiply_and_scale(sin_fast(word_44DCA), 0x24), 0x38);

				for (vertex_index = 0; vertex_index < 4U; vertex_index++)
					shape3d_vertex_read(
						&game3dshapes[0x98A / sizeof(struct SHAPE3D)],
						LEGACY_U16_WRAP_ADD(8U, vertex_index),
						&var_108[vertex_index]);
				var_108[0].x = LEGACY_S16_WRAP_SUB(idx, 0x24);
				var_108[1].x = LEGACY_S16_WRAP_SUB(idx, 0x24);
				var_108[2].x = LEGACY_S16_WRAP_SUB(0x24, idx);
				var_108[3].x = LEGACY_S16_WRAP_SUB(0x24, idx);

				var_108[0].z = var_counter;
				var_108[1].z = var_counter;
				var_108[2].z = var_counter;
				var_108[3].z = var_counter;
				for (vertex_index = 0; vertex_index < 4U; vertex_index++)
					shape3d_vertex_write(
						&game3dshapes[0x98A / sizeof(struct SHAPE3D)],
						LEGACY_U16_WRAP_ADD(8U, vertex_index),
						&var_108[vertex_index]);
				 
				curtransshape_ptr->pos.x = LEGACY_S16_WRAP_SUB(
					LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
						multiply_and_scale(sin_fast(LEGACY_S16_WRAP_ADD(
							track_angle, 0x100)), 0x24),
						multiply_and_scale(sin_fast(LEGACY_S16_WRAP_ADD(
							track_angle, 0x200)), 0x1B6)),
						trackcenterpos2[startcol2]), cam_pos.x);
				curtransshape_ptr->pos.y = LEGACY_S16_WRAP_SUB(
					hillHeightConsts[hillFlag], cam_pos.y);
				curtransshape_ptr->pos.z = LEGACY_S16_WRAP_SUB(
					LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
						multiply_and_scale(cos_fast(LEGACY_S16_WRAP_ADD(
							track_angle, 0x100)), 0x24),
						multiply_and_scale(cos_fast(LEGACY_S16_WRAP_ADD(
							track_angle, 0x200)), 0x1B6)),
						trackcenterpos[startrow2]), cam_pos.z);

				curtransshape_ptr->shapeptr = &game3dshapes[0x98A / sizeof(struct SHAPE3D)];
				curtransshape_ptr->rectptr = &rect_unk6;
				curtransshape_ptr->ts_flags = var_122 | 4;
				curtransshape_ptr->rotvec.x = 0;
				curtransshape_ptr->rotvec.y = 0;
				curtransshape_ptr->rotvec.z = track_angle;
				curtransshape_ptr->unk = 0x400;
				idx = LEGACY_S16_SAR(word_44DCA, 6U);
				if (idx > 3) {
					idx = 3;
				}

				curtransshape_ptr->material = idx;
				transformed_shape_add_for_sort(var_12A & -0x800 /*0xF800*/, 0);
			}
		}

		if (transformedshape_counter != 0) {
			if (transformedshape_counter > 1) {
				heapsort_by_order(transformedshape_counter, transformedshape_zarray, transformedshape_indices);
			}

			// Draw red overlights on the brake lights on own and opponent's car
			for (idx = 0; idx < transformedshape_counter; idx++) {
				// di is used for index into currenttransshape elsewhere
				di = transformedshape_indices[idx];
				if (transformedshape_arg2array[di] == 2) {
					if (state.playerstate.car_is_braking != 0) {
						backlights_paint_override = 0x2F;
					} else {
						backlights_paint_override = 0x2E;
					}
				} else if (transformedshape_arg2array[di] == 3) {
					if (state.opponentstate.car_is_braking == 0) {
						backlights_paint_override = 0x2E;
					} else {
						backlights_paint_override = 0x2F;
					}
				}

				var_transformresult = transformed_shape_op(&currenttransshape[di]); // DI??
				if (var_transformresult > 0)
					break;

				if (var_transformresult == 0) {
					if (transformedshape_arg2array[di] == 2) {
						if (state.playerstate.car_crashBmpFlag == 1) {
							var_DC[0] = 1;
						}
					} else if (transformedshape_arg2array[di] == 3) {
						if (state.opponentstate.car_crashBmpFlag == 1) {
							var_DC[1] = 1;
						}
					}
				}
			}
		}
	}

	// Draw the skybox
	var_132 = skybox_op(arg_0, arg_cliprectptr, skybox_parameter, &var_mat, car_rot_z_3, car_rot_x_2, cam_pos.y);
	sprite_set_1_size(0, 0x140, arg_cliprectptr->top, arg_cliprectptr->bottom);
	get_a_poly_info();

	// This supposedly draws the explosion. The fact that it cycles three
	// different patterns, each 4 frames long, seems to corroborate the
	// hypothesis
	for (si = 0; si < 2; si++) {
		if (var_DC[si] == 0) {
			continue;
		}
		if (slow_video_mgmt_copy == 0) {
			if (si == 0) {
				var_rectptr = &var_rect;
			} else {
				var_rectptr = &var_rect2;
			}
		} else {
			if (si == 0) {
				var_rectptr = &rect_unk12;
			} else {
				var_rectptr = &rect_unk15;
			}
		}

		if (rect_intersect(var_rectptr, arg_cliprectptr) == 0) {
			sprite_set_1_size(var_rectptr->left, var_rectptr->right, var_rectptr->top, var_rectptr->bottom);
			offset_vector.x = LEGACY_S16_SAR(LEGACY_S16_WRAP_ADD(
				var_rectptr->right, var_rectptr->left), 1U);
			offset_vector.y = LEGACY_S16_SAR(LEGACY_S16_WRAP_ADD(
				var_rectptr->top, var_rectptr->bottom), 1U);
			idx = LEGACY_S16_WRAP_SUB(
				var_rectptr->right, var_rectptr->left);
			var_counter = LEGACY_S16_WRAP_SUB(
				var_rectptr->bottom, var_rectptr->top);
			if (var_counter > idx) {
				idx = var_counter;
			}

			di = LEGACY_S16_SAR(state.game_frame, 2U) % 3;
			var_counter = LEGACY_S16_FROM_BITS((legacy_u16)
				LEGACY_S32_DIV_OR_ZERO(
					LEGACY_S32_WRAP_MUL((legacy_s32)idx, 0x100L),
					(legacy_s32)sdgame2_widths[di]));
			shape_op_explosion(var_counter, sdgame2shapes[di], offset_vector.x, offset_vector.y);
		}
	}

/*
; --------------------------------------------------------
*/

	// Depict windscreen cracking after a crash
	sprite_set_1_size(0, 0x140, arg_cliprectptr->top, arg_cliprectptr->bottom);
	if (cameramode == 0) {

		if (followOpponentFlag != 0) {
			var_stateptr = &state.opponentstate;
			si = state.game_oEndFrame;
		} else {
			var_stateptr = &state.playerstate;
			si = state.game_pEndFrame;
		}

		if (var_stateptr->car_crashBmpFlag == 1) {
			if (slow_video_mgmt_copy != 0) {
				rect_union(init_crak(state.game_frame - si, arg_cliprectptr->top, arg_cliprectptr->bottom - arg_cliprectptr->top), rect_unk, rect_unk);
			} else {
				init_crak(state.game_frame - si, arg_cliprectptr->top, arg_cliprectptr->bottom - arg_cliprectptr->top);
			}
		} else if (var_stateptr->car_crashBmpFlag == 2) {
			if (slow_video_mgmt_copy != 0) {
				rect_union(do_sinking(state.game_frame - si, arg_cliprectptr->top, arg_cliprectptr->bottom - arg_cliprectptr->top), rect_unk, rect_unk);
			} else {
				do_sinking(state.game_frame - si, arg_cliprectptr->top, arg_cliprectptr->bottom - arg_cliprectptr->top);
			}
		}
	}

	// Show elapsed time
	if (game_replay_mode == 0) {
		if (state.game_inputmode != 0) {
			format_frame_as_string(&resID_byte1, elapsed_time1 + elapsed_time2, 0);
			font_set_fontdef2(fontledresptr);
			if (slow_video_mgmt_copy != 0) {
				rect_union(intro_draw_text(&resID_byte1, 0x8C, roofbmpheight + 2, dialog_fnt_colour, 0), &rect_unk11, &rect_unk11);
			} else {
				intro_draw_text(&resID_byte1, 0x8C, roofbmpheight + 2, dialog_fnt_colour, 0);
			}

			font_set_fontdef();
		}
	}

	if (slow_video_mgmt_copy != 0) {
		rect_union(draw_ingame_text(), rect_unk, rect_unk);
		if (var_132 != 0) {
			rect_unk[0] = *arg_cliprectptr;
			for (si = 1; si < 15; si++) {
				rect_unk[si] = cliprect_unk;
			}
		}

		for (si = 0; si < 15; si++) {
			rectptr_unk[si] = rect_unk[si];
		}
		word_449FC[arg_0] = car_rot_x_2;
		word_463D6 = car_rot_x_2;

	} else {
		draw_ingame_text();
	}

}
