#include "frame_internal.h"

#define TRACK_PREVIEW_HALF_SHIFT 1U
#define TRACK_PREVIEW_SCREEN_WIDTH 320
#define TRACK_PREVIEW_SCREEN_HEIGHT 200
#define TRACK_PREVIEW_SKYBOX_HEIGHT 100
#define TRACK_PREVIEW_SKYBOX_LEFT 2U
#define TRACK_PREVIEW_SKYBOX_RIGHT 3U
#define TRACK_PREVIEW_TRANSFORM_DISTANCE 1024
#define TRACK_PREVIEW_GRID_SIZE 30U
#define TRACK_PREVIEW_PLACEHOLDER_MINIMUM 253U
#define TRACK_PREVIEW_HILL_TERRAIN 6U
#define TRACK_PREVIEW_HILL_ROAD_TERRAIN_FIRST 7U
#define TRACK_PREVIEW_HILL_ROAD_TERRAIN_END 11U
#define TRACK_PREVIEW_BRIDGE_FIRST 105U
#define TRACK_PREVIEW_BRIDGE_LAST 108U
#define TRACK_PREVIEW_BRIDGE_QUADRANT_COUNT 4U
#define TRACK_PREVIEW_QUADRANT_COLUMN_BIT 1U
#define TRACK_PREVIEW_QUADRANT_ROW_BIT 2U
#define TRACK_PREVIEW_HILL_BASE_SINGLE_SHAPE 43U
#define TRACK_PREVIEW_HILL_BASE_VERTICAL_SHAPE 91U
#define TRACK_PREVIEW_HILL_BASE_HORIZONTAL_SHAPE 92U
#define TRACK_PREVIEW_HILL_BASE_QUAD_SHAPE 93U
#define TRACK_PREVIEW_TRANSFORM_FLAGS 5U
#define TRACK_PREVIEW_OBJECT_TRANSFORM_FLAG 4U
#define TRACK_PREVIEW_ROTATE_CLIP 1

static legacy_s16 track_preview_half(legacy_s16 value)
{
	legacy_u16 bits;

	bits = (legacy_u16)value;
	return LEGACY_S16_FROM_BITS(LEGACY_U16_SAR(bits,
		TRACK_PREVIEW_HALF_SHIFT));
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
	transformed->ts_flags = TRACK_PREVIEW_TRANSFORM_FLAGS;
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

	sprite_set_1_size(0, TRACK_PREVIEW_SCREEN_WIDTH, 0,
		LEGACY_S16_WRAP_SUB(horizon, skybox.minimum_height));
	sprite_clear_1_color((legacy_u8)skybox.sky_color);
	sprite_set_1_size(0, TRACK_PREVIEW_SCREEN_WIDTH, 0,
		TRACK_PREVIEW_SKYBOX_HEIGHT);
	sprite_putimage_and_alt(skyboxes[TRACK_PREVIEW_SKYBOX_LEFT], 0,
		LEGACY_S16_WRAP_SUB(horizon,
			skybox.heights[TRACK_PREVIEW_SKYBOX_LEFT]));
	sprite_putimage_and_alt(skyboxes[TRACK_PREVIEW_SKYBOX_RIGHT],
		TRACK_PREVIEW_SCREEN_WIDTH,
		LEGACY_S16_WRAP_SUB(horizon,
			skybox.heights[TRACK_PREVIEW_SKYBOX_RIGHT]));
	sprite_set_1_size(0, TRACK_PREVIEW_SCREEN_WIDTH, horizon,
		TRACK_PREVIEW_SCREEN_HEIGHT);
	sprite_clear_1_color((legacy_u8)skybox.ground_color);
	sprite_set_1_size(0, TRACK_PREVIEW_SCREEN_WIDTH, 0,
		TRACK_PREVIEW_SCREEN_HEIGHT);
	select_cliprect_rotate(0, camera_angle, 0, &trackpreview_cliprect,
		TRACK_PREVIEW_ROTATE_CLIP);

	transformed.rotvec.x = 0;
	transformed.rotvec.y = 0;
	transformed.unk = TRACK_PREVIEW_TRANSFORM_DISTANCE;
	for (row = 0; row < TRACK_PREVIEW_GRID_SIZE; row++) {
		for (column = 0; column < TRACK_PREVIEW_GRID_SIZE; column++) {
			track = td14_elem_map_main[
				LEGACY_U16_WRAP_ADD(trackrows[row], column)];
			terrain = td15_terr_map_main[
				LEGACY_U16_WRAP_ADD(terrainrows[row], column)];
			if (track != 0 && terrain >=
				TRACK_PREVIEW_HILL_ROAD_TERRAIN_FIRST && terrain <
				TRACK_PREVIEW_HILL_ROAD_TERRAIN_END) {
				track = subst_hillroad_track(terrain, track);
				terrain = 0;
			}
			if (track >= TRACK_PREVIEW_PLACEHOLDER_MINIMUM) {
				track = 0;
				terrain = 0;
			}

			terrain_height = 0;
			if (terrain == TRACK_PREVIEW_HILL_TERRAIN) {
				terrain_height = hillHeightConsts[1];
				if (track != 0)
					terrain = 0;
			} else if (track >= TRACK_PREVIEW_BRIDGE_FIRST &&
				track <= TRACK_PREVIEW_BRIDGE_LAST) {
				for (quadrant = 0;
					quadrant < TRACK_PREVIEW_BRIDGE_QUADRANT_COUNT;
					quadrant++) {
					adjacent_column = (legacy_u8)(column +
						((quadrant & TRACK_PREVIEW_QUADRANT_COLUMN_BIT) != 0 ?
							1U : 0U));
					adjacent_row = (legacy_u8)(row +
						((quadrant & TRACK_PREVIEW_QUADRANT_ROW_BIT) != 0 ?
							1U : 0U));
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
					track_object_base_x(track_object, column),
					camera_x));
			track_position.y = track_preview_half(
				LEGACY_S16_WRAP_SUB(terrain_height, camera_y));
			track_position.z = track_preview_half(
				LEGACY_S16_WRAP_SUB(
					track_object_base_z(track_object, row), camera_z));

			if (terrain_height != 0) {
				switch (track_object->ss_multiTileFlag) {
				case TRACK_PREVIEW_QUADRANT_COLUMN_BIT:
					transformed.shapeptr = &game3dshapes[
						TRACK_PREVIEW_HILL_BASE_VERTICAL_SHAPE];
					break;
				case TRACK_PREVIEW_QUADRANT_ROW_BIT:
					transformed.shapeptr = &game3dshapes[
						TRACK_PREVIEW_HILL_BASE_HORIZONTAL_SHAPE];
					break;
				case TRACK_PREVIEW_QUADRANT_COLUMN_BIT |
					TRACK_PREVIEW_QUADRANT_ROW_BIT:
					transformed.shapeptr = &game3dshapes[
						TRACK_PREVIEW_HILL_BASE_QUAD_SHAPE];
					break;
				default:
					transformed.shapeptr = &game3dshapes[
						TRACK_PREVIEW_HILL_BASE_SINGLE_SHAPE];
					break;
				}
				transformed.pos = track_position;
				transformed.rotvec.z = 0;
				transformed.ts_flags = TRACK_PREVIEW_TRANSFORM_FLAGS;
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
					transformed.ts_flags = TRACK_PREVIEW_TRANSFORM_FLAGS;
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
				(legacy_u8)(track_object->ss_ignoreZBias |
					TRACK_PREVIEW_OBJECT_TRANSFORM_FLAG);
			transformed.material =
				(legacy_s8)track_object->ss_surfaceType < 0 ?
				0 : track_object->ss_surfaceType;
			transformed_shape_op(&transformed);
			get_a_poly_info();
		}
	}
}
