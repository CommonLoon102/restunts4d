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
