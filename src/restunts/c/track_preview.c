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
static struct TRACKOBJECT* frame_track_object_from_legacy_index(
	legacy_u8 index)
{
	if (index < TRACK_OBJECT_COUNT)
		return &trkObjectList[index];
	return &sceneshapes2[(legacy_u16)index - TRACK_OBJECT_COUNT];
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
				overlay_object = frame_track_object_from_legacy_index(
					track_object->ss_ssOvelay);
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
