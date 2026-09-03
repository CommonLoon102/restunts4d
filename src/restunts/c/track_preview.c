#include "frame_internal.h"

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
