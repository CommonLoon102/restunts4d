#include "frame_internal.h"
#include "game_input.h"
#include "trackdata_layout.h"
#include "ui_text.h"
#include "shape3d.h"
#include "track_types.h"
#include "track_objects.h"
#include "track_collision.h"
#include "wheel_transform.h"
#include "camera.h"
#include "video_frame.h"
#include "shape2d.h"
#include "car_model.h"
#include "scene_resources.h"
#include "projection.h"
#include "skybox.h"
#include "race_graphics.h"
#include "math.h"
#include "externs.h"
#include "crash_state.h"

#define TRACK_OBJECT_COUNT 215U
#define TRACK_GRID_LAST_COORDINATE 29
#define TRACK_WORLD_TILE_SHIFT 16U
#define FRAME_CAR_WHEEL_COUNT 4
#define FRAME_LOOKAHEAD_TILE_COUNT 23
#define FRAME_LOOKAHEAD_LAST_TILE_INDEX 22
#define FRAME_CAR_UP_VECTOR_LENGTH 30000
#define FRAME_CAR_NEAR_SORT_ADJUSTMENT 2048
#define FRAME_DEFAULT_TRANSFORM_DISTANCE 1024
#define FRAME_CAR_TRANSFORM_DISTANCE 300
#define FRAME_SCREEN_WIDTH 320
#define FRAME_DIRTY_RECT_COUNT 15
#define FRAME_FONT_HEIGHT_OFFSET 14U
#define FRAME_DEBRIS_SLOT_COUNT 24
#define FRAME_TRANSFORM_FLAGS_DEFAULT 4
#define FRAME_TRANSFORM_FLAGS_NO_DEPTH_SORT 5
#define FRAME_TRANSFORM_FLAGS_CLIPPED 12
#define FRAME_COCKPIT_HEIGHT_CLEARANCE 6
#define FRAME_CAMERA_DIRECTION_LENGTH 16384
#define FRAME_TRACK_CAMERA_HEIGHT_OFFSET 90
#define FRAME_PLANE_CLEARANCE 12
#define FRAME_CAMERA_TARGET_HEIGHT_OFFSET 50
#define FRAME_ANIMATION_PHASE_MASK 15U
#define FRAME_LOOKAHEAD_HEADING_SHIFT 7U
#define FRAME_SKYBOX_TEST_DISTANCE 1000
#define FRAME_DISTANT_SHAPE_FLAGS 7
#define FRAME_DISTANT_SHAPE_COUNT 8
#define FRAME_DISTANT_SHAPE_MIN_ANGLE 135
#define FRAME_DISTANT_SHAPE_MAX_ANGLE 889
#define FRAME_DISTANT_SHAPE_HEIGHT 2790
#define FRAME_DISTANT_SHAPE_DISTANCE 15000
#define FRAME_DISTANT_SHAPE_MIN_DEPTH 200
#define FRAME_HILL_ROAD_TERRAIN_FIRST 7U
#define FRAME_HILL_ROAD_TERRAIN_END 11U
#define FRAME_SCENERY_PHYSICAL_MODEL_FIRST 64
#define FRAME_MULTITILE_ROW 1
#define FRAME_MULTITILE_COLUMN 2
#define FRAME_MULTITILE_BOTH 3
#define FRAME_MULTITILE_NONE 0
#define FRAME_ELEVATED_CORNER_FIRST 105U
#define FRAME_ELEVATED_CORNER_LAST 108U
#define FRAME_ELEVATED_CORNER_COUNT 4
#define FRAME_HILL_FILL_COUNT_SINGLE 1
#define FRAME_HILL_FILL_COUNT_ROW 2
#define FRAME_HILL_FILL_COUNT_COLUMN 2
#define FRAME_HILL_FILL_COUNT_BOTH 4
#define FRAME_HILL_FILL_SHAPE_RESOURCE_OFFSET 946U
#define FRAME_SINGLE_TILE_TRANSFORM_DISTANCE 2048
#define FRAME_NO_DEPTH_SORT_FLAG 1U
#define FRAME_WHEEL_SORT_ADJUSTMENT 1024
#define FRAME_CHECKPOINT_NONE 255U
#define FRAME_CHECKPOINT_TRACK_OBJECT_BASE 212U
#define FRAME_CHECKPOINT_OWNER_OFFSET 2
#define FRAME_CHECKPOINT_TRANSFORM_DISTANCE 100
#define FRAME_PLAYER_SHAPE_RESOURCE_OFFSET 2772U
#define FRAME_OPPONENT_SHAPE_RESOURCE_OFFSET 2794U
#define FRAME_START_FLAG_RESOURCE_OFFSET 2442U
#define FRAME_START_FLAG_RADIUS 36
#define FRAME_START_FLAG_CENTER_Z 56
#define FRAME_START_FLAG_FAR_OFFSET 438
#define FRAME_START_FLAG_VERTEX_COUNT 4U
#define FRAME_START_FLAG_FIRST_VERTEX 8U
#define FRAME_START_FLAG_ANIMATION_SHIFT 6U
#define FRAME_START_FLAG_MAX_MATERIAL 3
#define FRAME_EXPLOSION_CAR_COUNT 2
#define FRAME_EXPLOSION_FRAME_SHIFT 2U
#define FRAME_EXPLOSION_VARIANT_COUNT 3
#define FRAME_EXPLOSION_FIXED_SCALE 256L
#define FRAME_ELAPSED_TIME_X 140
#define FRAME_ELAPSED_TIME_Y_OFFSET 2
#define FRAME_DIRTY_RECT_CHANGED 3
#define FRAME_SKYBOX_RECT_INDEX 5
#define FRAME_DETAIL_FULL 0
#define FRAME_DETAIL_FASTEST 4
#define FRAME_CAR_LOW_DETAIL_FIRST 3
#define FRAME_TILE_DETAIL_FULL 0
#define FRAME_STEERED_WHEEL_FIRST_VERTEX 8U
#define FRAME_FENCE_EDGE_CLASS_COUNT 3
#define FRAME_FENCE_NONE (-1)
#define FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT 24
#define FRAME_SLOW_VIDEO_TRANSFORM_FLAG 8
#define FRAME_CAMERA_TILE_SHIFT 10U
#define FRAME_FENCE_POSITION_STRIDE 2
#define FRAME_FENCE_SECOND_COORDINATE 1
#define FRAME_FENCE_POSITION_COUNT_SINGLE 1
#define FRAME_FENCE_POSITION_COUNT_ROW 2
#define FRAME_FENCE_POSITION_COUNT_COLUMN 3
#define FRAME_FENCE_POSITION_COUNT_BOTH 4
#define FRAME_SORT_MINIMUM_SHAPE_COUNT 2
#define FRAME_RECT_CENTER_SHIFT 1U

enum FRAME_TILE_MARKER {
	FRAME_TILE_DRAW_MARKER = 0,
	FRAME_TILE_MULTITILE_COVERED_MARKER = 1,
	FRAME_TILE_UNAVAILABLE_MARKER = 2
};

enum FRAME_SORT_ID {
	FRAME_PLAYER_SORT_ID = 2,
	FRAME_OPPONENT_SORT_ID = 3
};

enum FRAME_FENCE_EDGE_CLASS {
	FRAME_FENCE_EDGE_LOW = 0,
	FRAME_FENCE_EDGE_HIGH = 1,
	FRAME_FENCE_EDGE_INTERIOR = 2
};

enum FRAME_CORNER {
	FRAME_CORNER_NORTHWEST = 0,
	FRAME_CORNER_NORTHEAST = 1,
	FRAME_CORNER_SOUTHWEST = 2,
	FRAME_CORNER_SOUTHEAST = 3
};

enum FRAME_START_FLAG_VERTEX_INDEX {
	FRAME_START_FLAG_VERTEX_LEFT_NEAR = 0,
	FRAME_START_FLAG_VERTEX_LEFT_FAR = 1,
	FRAME_START_FLAG_VERTEX_RIGHT_NEAR = 2,
	FRAME_START_FLAG_VERTEX_RIGHT_FAR = 3
};

/*
 * In the original dseg, terrain_scene_objects immediately follows trkObjectList.
 * Some track objects store overlay indices into that combined legacy table,
 * so indices beyond trkObjectList intentionally address terrain_scene_objects.
 */
struct TRACKOBJECT* frame_track_object_from_legacy_index(
	legacy_u8 index)
{
	if (index < TRACK_OBJECT_COUNT)
		return &trkObjectList[index];
	return &terrain_scene_objects[(legacy_u16)index - TRACK_OBJECT_COUNT];
}

void transformed_shape_add_for_sort(legacy_s16 z_adjust, legacy_s16 type)
{
	struct VECTOR transformed_position;
	legacy_s16 index;

	mat_mul_vector(&curtransshape_ptr->pos, &mat_temp,
		&transformed_position);
	index = LEGACY_S8_FROM_BITS((legacy_u8)transformedshape_counter);
	transformedshape_zarray[index] = LEGACY_S16_WRAP_ADD(
		transformed_position.z, z_adjust);
	transformed_shape_sort_types[index] = (legacy_s8)(legacy_u8)type;
	transformedshape_indices[index] = index;
	transformedshape_counter = LEGACY_S8_WRAP_ADD(
		transformedshape_counter, 1);
	curtransshape_ptr++;
}

/* Each lookahead table is a run of three-byte records: the tile offset from
   the camera tile, and the detail level to draw that tile at. */
struct FRAME_LOOKAHEAD_TILE {
	legacy_s8 east;
	legacy_s8 south;
	legacy_s8 detail;
};

static legacy_s16 frame_relative_position(legacy_s32 position,
	legacy_s16 camera_position)
{
	return LEGACY_S16_WRAP_SUB(
		position_to_word(position), camera_position);
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
		position_to_word(offset), track_position), camera_position);
}

static legacy_s8 frame_tile_from_world(legacy_s32 position)
{
	return LEGACY_S8_FROM_BITS(
		(legacy_u8)((legacy_u32)position >> TRACK_WORLD_TILE_SHIFT));
}

static legacy_s8 frame_south_tile_from_world(legacy_s32 position)
{
	return LEGACY_S8_WRAP_SUB(TRACK_GRID_LAST_COORDINATE,
		frame_tile_from_world(position));
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
	return LEGACY_S8_WRAP_SUB(TRACK_GRID_LAST_COORDINATE,
		frame_tile_from_world_offset(position, offset));
}

static legacy_s16 frame_car_z_adjust(const legacy_s8* wheel_surfaces,
	struct MATRIX* rotation)
{
	struct VECTOR offset_vector;
	struct VECTOR rotated_vector;

	if (wheel_surfaces[0] == CAR_SURFACE_GRASS &&
		wheel_surfaces[1] == CAR_SURFACE_GRASS &&
		wheel_surfaces[2] == CAR_SURFACE_GRASS &&
		wheel_surfaces[3] == CAR_SURFACE_GRASS)
		return 0;
	offset_vector.x = 0;
	offset_vector.z = 0;
	offset_vector.y = FRAME_CAR_UP_VECTOR_LENGTH;
	mat_mul_vector(&offset_vector, rotation, &rotated_vector);
	mat_mul_vector(&rotated_vector, &mat_temp, &offset_vector);
	if (offset_vector.z <= 0)
		return -FRAME_CAR_NEAR_SORT_ADJUSTMENT;
	return FRAME_CAR_NEAR_SORT_ADJUSTMENT;
}

static legacy_s16 frame_find_car_wheel(const struct CARSTATE* carstate,
	const struct SIMD* simd, const legacy_s8* should_skip_tile,
	const struct FRAME_LOOKAHEAD_TILE* lookahead_tiles,
	legacy_s8 camera_tile_east,
	legacy_s8 camera_tile_south, legacy_s8* result_tile_east,
	legacy_s8* result_tile_south)
{
	struct MATRIX* rotation;
	struct VECTOR offset_vector;
	struct VECTOR rotated_vector;
	legacy_s16 wheel;
	legacy_s16 tile_index;
	legacy_s16 best_tile_index;
	legacy_s16 matched_wheel;
	legacy_s8 tile_east;
	legacy_s8 tile_south;

	rotation = mat_rot_zxy(
		LEGACY_S16_WRAP_NEGATE(carstate->car_rotate.z),
		LEGACY_S16_WRAP_NEGATE(carstate->car_rotate.y),
		LEGACY_S16_WRAP_NEGATE(carstate->car_rotate.x),
		MATRIX_ROTATION_ORDER_ZXY);
	best_tile_index = -1;
	matched_wheel = -1;
	for (wheel = 0; wheel < FRAME_CAR_WHEEL_COUNT; wheel++) {
		offset_vector = simd->wheel_coords[wheel];
		mat_mul_vector(&offset_vector, rotation, &rotated_vector);
		tile_east = frame_tile_from_world_offset(
			carstate->car_position.lx, rotated_vector.x);
		tile_south = frame_south_tile_from_world_offset(
			carstate->car_position.lz, rotated_vector.z);
		for (tile_index = FRAME_LOOKAHEAD_LAST_TILE_INDEX;
			tile_index > best_tile_index;
			tile_index--) {
			if (should_skip_tile[tile_index] !=
				FRAME_TILE_UNAVAILABLE_MARKER &&
				lookahead_tiles[tile_index].east + camera_tile_east ==
					tile_east &&
				lookahead_tiles[tile_index].south + camera_tile_south ==
					tile_south) {
				*result_tile_east = tile_east;
				*result_tile_south = tile_south;
				best_tile_index = tile_index;
				matched_wheel = wheel;
			}
		}
	}
	if (matched_wheel != -1)
		return frame_car_z_adjust(carstate->car_surfaceWhl, rotation);
	return 0;
}

static void frame_add_dynamic_shape(struct TRACKOBJECT* track_object,
	legacy_s16 state_index, legacy_s16 flags, legacy_s16 material,
	legacy_s16 z_adjust)
{
	curtransshape_ptr->shapeptr = track_object->ss_shapePtr;
	curtransshape_ptr->rectptr = &frame_sorted_shapes_rect;
	curtransshape_ptr->ts_flags = flags;
	curtransshape_ptr->rotvec.x = LEGACY_S16_WRAP_NEGATE(
		state.game_particle_rotation_x[state_index]);
	curtransshape_ptr->rotvec.y = LEGACY_S16_WRAP_NEGATE(
		state.game_particle_rotation_y[state_index]);
	curtransshape_ptr->rotvec.z = LEGACY_S16_WRAP_NEGATE(
		state.game_particle_heading[state_index]);
	curtransshape_ptr->culling_distance = FRAME_DEFAULT_TRANSFORM_DISTANCE;
	curtransshape_ptr->material = material;
	transformed_shape_add_for_sort(z_adjust, 0);
}

static void frame_prepare_flat_track_shape(struct TRANSFORMEDSHAPE3D* shape,
	legacy_s8 tile_east, legacy_s8 tile_south,
	const struct VECTOR* camera_position, legacy_s16 flags,
	legacy_s16 rotation)
{
	shape->pos.x = LEGACY_S16_WRAP_SUB(
		track_column_centers[tile_east], camera_position->x);
	shape->pos.y = LEGACY_S16_WRAP_NEGATE(camera_position->y);
	shape->pos.z = LEGACY_S16_WRAP_SUB(
		track_row_centers[tile_south], camera_position->z);
	shape->rectptr = &frame_unsorted_shapes_rect;
	shape->ts_flags = flags;
	shape->rotvec.x = 0;
	shape->rotvec.y = 0;
	shape->rotvec.z = rotation;
	shape->culling_distance = FRAME_DEFAULT_TRANSFORM_DISTANCE;
	shape->material = 0;
}

void init_rect_arrays(void) {
	legacy_s16 i;

	if (slow_video_mgmt_copy == 0)
		return;

	frame_rects_page0[0] = full_screen_rect;
	frame_rects_page1[0] = full_screen_rect;
	for (i = 1; i < FRAME_DIRTY_RECT_COUNT; i++) {
		frame_rects_page0[i] = empty_rect;
		frame_rects_page1[i] = empty_rect;
	}
}

void font_set_fontdef2(void far* data) {
	set_fontdefseg(data);
	font_glyph_height = LEGACY_S16_FROM_BITS(
		LEGACY_READ_U16_LE((legacy_u8 far*)data +
			FRAME_FONT_HEIGHT_OFFSET));
}

void font_set_fontdef(void) {
	font_set_fontdef2(fontdefptr);
}

void frame_present(struct RECTANGLE* cliprect) {
	struct RECTANGLE* dirty_rect;
	legacy_s16 i;

	if (video_uses_page_flipping != 0)
		return;

	sprite_select_screen_compat();
	if (full_redraw_frames_remaining != 0) {
		mouse_draw_opaque_check();
		sprite_putimage(render_window_sprite->sprite_bitmapptr);
	} else if (slow_video_mgmt_copy == 0) {
		sprite_set_target_clip_bounds(
			cliprect->left,
			cliprect->right,
			cliprect->top,
			cliprect->bottom);
		mouse_draw_opaque_check();
		sprite_putimage(render_window_sprite->sprite_bitmapptr);
	} else {
		for (i = 0; i < FRAME_DIRTY_RECT_COUNT; i++)
			frame_rect_change_flags[i] = FRAME_DIRTY_RECT_CHANGED;
		if (detail_level == FRAME_DETAIL_FASTEST)
			frame_buffer_camera_headings[1] = last_rendered_camera_heading;
		if (frame_buffer_camera_headings[1] == last_rendered_camera_heading &&
			frame_rects_page0[FRAME_SKYBOX_RECT_INDEX].left ==
				frame_rects_page1[FRAME_SKYBOX_RECT_INDEX].left &&
			frame_rects_page0[FRAME_SKYBOX_RECT_INDEX].right ==
				frame_rects_page1[FRAME_SKYBOX_RECT_INDEX].right &&
			frame_rects_page0[FRAME_SKYBOX_RECT_INDEX].top ==
				frame_rects_page1[FRAME_SKYBOX_RECT_INDEX].top &&
			frame_rects_page0[FRAME_SKYBOX_RECT_INDEX].bottom ==
				frame_rects_page1[FRAME_SKYBOX_RECT_INDEX].bottom) {
			frame_rect_change_flags[FRAME_SKYBOX_RECT_INDEX] = 0;
		}

		redraw_rect_count = 0;
		rectlist_add_rects(
			FRAME_DIRTY_RECT_COUNT,
			frame_rect_change_flags,
			frame_rects_page0,
			frame_rects_page1,
			cliprect,
			&redraw_rect_count,
			merged_redraw_rects);
		if (redraw_rect_count != 0) {
			rect_array_sort_by_top(
				redraw_rect_count,
				merged_redraw_rects,
				redraw_rect_sort_indices);
			mouse_draw_opaque_check();
			for (i = 0; i < redraw_rect_count; i++) {
				dirty_rect = &merged_redraw_rects[redraw_rect_sort_indices[i]];
				sprite_set_target_clip_bounds(
					dirty_rect->left,
					dirty_rect->right,
					dirty_rect->top,
					dirty_rect->bottom);
				sprite_putimage(render_window_sprite->sprite_bitmapptr);
			}
		} else {
			sprite_set_target_clip_bounds(0, FRAME_SCREEN_WIDTH,
				cliprect->top, cliprect->bottom);
			mouse_draw_opaque_check();
			sprite_putimage(render_window_sprite->sprite_bitmapptr);
		}
	}

	mouse_draw_transparent_check();
	if (slow_video_mgmt_copy != 0) {
		frame_buffer_camera_headings[1] = last_rendered_camera_heading;
		for (i = 0; i < FRAME_DIRTY_RECT_COUNT; i++)
			frame_rects_page1[i] = frame_rects_page0[i];
	}
}

/* The player and the opponent are drawn identically: first the debris
   attached to that car, then the car body itself with its wheels, clip
   rectangle and rotation. Only the shapes, buffers and material differ. */
static void frame_add_car(struct CARSTATE* carstate, legacy_s8 debris_owner,
	legacy_u16 car_object, struct SHAPE3D* wheel_shape,
	legacy_s16* wheel_angles, struct VECTOR* wheel_vectors,
	struct VECTOR* wheel_vector, struct RECTANGLE* slow_rect,
	struct RECTANGLE* crash_rect, const struct VECTOR* camera_position,
	legacy_s8 tile_detail, legacy_s8 flags, legacy_s16 material,
	legacy_s16 z_adjust)
{
	struct TRACKOBJECT* track_object;
	legacy_s16 index;

	if (state.game_particles_active != 0) {
		for (index = 0; index < FRAME_DEBRIS_SLOT_COUNT; index++) {
			if (state.game_particle_forward_speed[index] != 0 &&
				state.game_particle_owner[index] == debris_owner) {
				track_object = &particle_scene_objects[state.game_particle_shape_index[index]];
				curtransshape_ptr->pos.x = frame_relative_position_sum(
					state.game_particle_x[index],
					carstate->car_position.lx, camera_position->x);
				curtransshape_ptr->pos.y = frame_relative_position_sum(
					state.game_particle_y[index],
					carstate->car_position.ly, camera_position->y);
				curtransshape_ptr->pos.z = frame_relative_position_sum(
					state.game_particle_z[index],
					carstate->car_position.lz, camera_position->z);
				frame_add_dynamic_shape(track_object, index,
					flags | FRAME_TRANSFORM_FLAGS_NO_DEPTH_SORT,
					material, z_adjust);
			}
		}
	}

	track_object = &trkObjectList[car_object];
	curtransshape_ptr->pos.x = frame_relative_position(
		carstate->car_position.lx, camera_position->x);
	curtransshape_ptr->pos.y = frame_relative_position(
		carstate->car_position.ly, camera_position->y);
	curtransshape_ptr->pos.z = frame_relative_position(
		carstate->car_position.lz, camera_position->z);

	if (tile_detail != FRAME_TILE_DETAIL_FULL ||
		detail_level >= FRAME_CAR_LOW_DETAIL_FIRST) {
		curtransshape_ptr->shapeptr = track_object->ss_loShapePtr;
	} else {
		curtransshape_ptr->shapeptr = track_object->ss_shapePtr;
		shape3d_update_car_wheel_vertices(wheel_shape, FRAME_STEERED_WHEEL_FIRST_VERTEX,
			carstate->car_steeringAngle,
			carstate->car_suspension_deflection, wheel_angles, wheel_vectors,
			wheel_vector);
	}

	if (slow_video_mgmt_copy != 0) {
		curtransshape_ptr->rectptr = slow_rect;
		curtransshape_ptr->ts_flags = FRAME_TRANSFORM_FLAGS_CLIPPED;
	} else if (carstate->car_crashBmpFlag != CRASH_EVENT_COLLISION) {
		curtransshape_ptr->ts_flags = FRAME_TRANSFORM_FLAGS_DEFAULT;
	} else {
		*crash_rect = empty_rect;
		curtransshape_ptr->rectptr = crash_rect;
		curtransshape_ptr->ts_flags = FRAME_TRANSFORM_FLAGS_CLIPPED;
	}

	curtransshape_ptr->rotvec.x = LEGACY_S16_WRAP_NEGATE(
		carstate->car_rotate.z);
	curtransshape_ptr->rotvec.y = LEGACY_S16_WRAP_NEGATE(
		carstate->car_rotate.y);
	curtransshape_ptr->rotvec.z = LEGACY_S16_WRAP_NEGATE(
		carstate->car_rotate.x);
	curtransshape_ptr->culling_distance = FRAME_CAR_TRANSFORM_DISTANCE;
	curtransshape_ptr->material = material;
	/* The sort slot carries the same id as the track object: 2 for the
	   player, 3 for the opponent. */
	transformed_shape_add_for_sort(z_adjust, (legacy_s16)car_object);
}

/* Border fences: a tile sits on the low edge (0), the high edge (1) or in
   between (2) along each axis, and the pair picks the fence piece. -1 means
   the tile is not on the border at all. */
static const legacy_s8 fence_by_edge[FRAME_FENCE_EDGE_CLASS_COUNT]
	[FRAME_FENCE_EDGE_CLASS_COUNT] = {
	{ 7, 5, 6 },
	{ 1, 3, 2 },
	{ 0, 4, FRAME_FENCE_NONE }
};

static legacy_s16 frame_border_index(legacy_s8 offset)
{
	if (offset == 0)
		return FRAME_FENCE_EDGE_LOW;
	if (offset == TRACK_GRID_LAST_COORDINATE)
		return FRAME_FENCE_EDGE_HIGH;
	return FRAME_FENCE_EDGE_INTERIOR;
}

/* The camera looks out of the car, so it uses the car's rotation inverted. */
static struct MATRIX* frame_car_rotation(legacy_s16 rot_x, legacy_s16 rot_y,
	legacy_s16 rot_z)
{
	return mat_rot_zxy(LEGACY_S16_WRAP_NEGATE(rot_z),
		LEGACY_S16_WRAP_NEGATE(rot_y),
		LEGACY_S16_WRAP_NEGATE(rot_x), MATRIX_ROTATION_ORDER_ZXY);
}

/* Per-frame scratch data stays on the stack; none of it survives a frame. */
struct FRAME_CAMERA {
	struct VECTOR position;
	struct MATRIX pitch_roll_rotation;
	legacy_s16 pitch;
	legacy_s16 yaw;
	legacy_s16 roll;
	legacy_s16 skybox_parameter;
};

struct FRAME_TILE_SELECTION {
	const struct FRAME_LOOKAHEAD_TILE* lookahead;
	legacy_s8 camera_east, camera_south;
	legacy_s8 player_east, player_south;
	legacy_s8 markers[FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT];
	legacy_s8 east[FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT];
	legacy_s8 south[FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT];
	legacy_s8 detail[FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT];
	legacy_u8 elements[FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT];
	legacy_u8 terrain[FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT];
};

struct FRAME_CAR_RENDER {
	legacy_s8 east, south;
	legacy_s16 depth_adjustment;
	struct RECTANGLE crash_rect;
	legacy_s8 explosion_visible;
};

struct FRAME_TILE {
	legacy_s8 east, south;
	legacy_s8 last_east, last_south;
	legacy_s8 detail;
	legacy_u8 element, terrain;
	legacy_s16 height;
	legacy_s16 depth_mask;
	struct VECTOR position;
};

static legacy_s8 frame_begin(legacy_s8 buffer_index)
{
	legacy_s16 rect_index;
	legacy_s8 redraw_transform_flags;
	struct RECTANGLE* redraw_rect;

	if (video_uses_page_flipping == 0 || buffer_index == 0) {
		active_frame_rects = frame_rects_page0;
		alternate_frame_rects = frame_rects_page1;
	} else {
		alternate_frame_rects = frame_rects_page0;
		active_frame_rects = frame_rects_page1;
	}

	if (slow_video_mgmt_copy != 0) {
		redraw_transform_flags = FRAME_SLOW_VIDEO_TRANSFORM_FLAG;
		redraw_rect = frame_layer_rects;
		for (rect_index = 0; rect_index < FRAME_DIRTY_RECT_COUNT; rect_index++) {
			*redraw_rect = empty_rect;
			redraw_rect++;
		}
	} else {
		redraw_transform_flags = 0;
	}

	return redraw_transform_flags;
}

static void frame_setup_camera(struct FRAME_CAMERA* camera)
{
	legacy_s16 plane_distance;
	struct MATRIX* car_rot_matrix;
	struct VECTOR car_pos;
	struct VECTOR offset_vector;
	struct VECTOR car_to_cam_rotated;
	legacy_s16 car_rot_y;
	legacy_s16 car_rot_x;
	legacy_s16 car_rot_z;
	legacy_s16 camera_roll;
	legacy_s16 camera_horizontal_distance;

	// Set car position (own or opponent's)
	if (followOpponentFlag == 0) {
		car_pos.x = position_to_word(
			state.playerstate.car_position.lx);
		car_pos.y = position_to_word(
			state.playerstate.car_position.ly);
		car_pos.z = position_to_word(
			state.playerstate.car_position.lz);
		car_rot_y = state.playerstate.car_rotate.y;
		car_rot_z = state.playerstate.car_rotate.z;
		car_rot_x = state.playerstate.car_rotate.x;
	} else {
		car_pos.x = position_to_word(
			state.opponentstate.car_position.lx);
		car_pos.y = position_to_word(
			state.opponentstate.car_position.ly);
		car_pos.z = position_to_word(
			state.opponentstate.car_position.lz);
		car_rot_y = state.opponentstate.car_rotate.y;
		car_rot_z = state.opponentstate.car_rotate.z;
		car_rot_x = state.opponentstate.car_rotate.x;
	}

	camera->yaw = -1;
	camera_roll = 0;

	// Set camera position, based on the car position and the camera mode
	if (cameramode == CAMERA_MODE_COCKPIT) {
		camera->yaw = car_rot_x & ANGLE_MASK;
		camera->pitch = car_rot_y & ANGLE_MASK;
		camera_roll = car_rot_z & ANGLE_MASK;
		car_rot_matrix = frame_car_rotation(car_rot_x, car_rot_y,
			car_rot_z);
		offset_vector.x = 0;
		offset_vector.z = 0;
		offset_vector.y = LEGACY_S16_WRAP_SUB(simd_player.car_height,
			FRAME_COCKPIT_HEIGHT_CLEARANCE);

		mat_mul_vector(&offset_vector, car_rot_matrix, &car_to_cam_rotated);
		camera->position.x = LEGACY_S16_WRAP_ADD(
			car_pos.x, car_to_cam_rotated.x);
		camera->position.y = LEGACY_S16_WRAP_ADD(
			car_pos.y, car_to_cam_rotated.y);
		camera->position.z = LEGACY_S16_WRAP_ADD(
			car_pos.z, car_to_cam_rotated.z);
	} else if (cameramode == CAMERA_MODE_FOLLOW) {
		camera->position.x = state.game_follow_camera_position[followOpponentFlag].x;
		camera->position.z = state.game_follow_camera_position[followOpponentFlag].z;
		camera->position.y = state.game_follow_camera_position[followOpponentFlag].y;
	} else if (cameramode == CAMERA_MODE_CUSTOM) {
		offset_vector.x = 0;
		offset_vector.y = 0;
		offset_vector.z = FRAME_CAMERA_DIRECTION_LENGTH;
		car_rot_matrix = frame_car_rotation(car_rot_x, car_rot_y,
			car_rot_z);
		mat_mul_vector(&offset_vector, car_rot_matrix, &car_to_cam_rotated);

		offset_vector.x = 0;
		offset_vector.y = 0;
		offset_vector.z = custom_camera.distance;
		car_rot_matrix = mat_rot_zxy(0,
			LEGACY_S16_WRAP_NEGATE(custom_camera.elevation_angle),
			LEGACY_S16_WRAP_SUB(polarAngle(car_to_cam_rotated.x,
				car_to_cam_rotated.z), custom_camera.azimuth_angle),
			MATRIX_ROTATION_ORDER_ZXY);

		mat_mul_vector(&offset_vector, car_rot_matrix, &car_to_cam_rotated);
		camera->position.x = LEGACY_S16_WRAP_ADD(car_pos.x, car_to_cam_rotated.x);
		camera->position.y = LEGACY_S16_WRAP_ADD(car_pos.y, car_to_cam_rotated.y);
		camera->position.z = LEGACY_S16_WRAP_ADD(car_pos.z, car_to_cam_rotated.z);
	} else if (cameramode == CAMERA_MODE_TRACKSIDE) {
		camera->position.x = trackside_camera_positions[state.game_trackside_camera_index[followOpponentFlag]].x;
		camera->position.y = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
			trackside_camera_positions[state.game_trackside_camera_index[followOpponentFlag]].y,
			camera_track_height_offset), FRAME_TRACK_CAMERA_HEIGHT_OFFSET);
		camera->position.z = trackside_camera_positions[state.game_trackside_camera_index[followOpponentFlag]].z;
	}

	// Keep external cameras above the track and aim them at the followed car.
	if (camera->yaw == -1) {
		build_track_object(&camera->position, &camera->position);
		if (camera->position.y < terrainHeight) {
			camera->position.y = terrainHeight;
		}

		if (track_wall_collision_enabled != 0) {
			plane_distance = plane_signed_distance(planindex, camera->position.x,
				camera->position.y, camera->position.z);
			if (plane_distance < FRAME_PLANE_CLEARANCE) {
				wheel_forward_travel.x = 0;
				wheel_forward_travel.y = LEGACY_S16_WRAP_SUB(FRAME_PLANE_CLEARANCE,
					plane_distance);
				wheel_forward_travel.z = 0;
				planindex_copy = planindex;
				wheel_heading_offset = 0;
				car_initial_pitch = 0;
				car_initial_roll = 0;
				car_initial_yaw = 0;
				transform_wheel_travel_to_world();
				camera->position.x = LEGACY_S16_WRAP_ADD(
					camera->position.x, wheel_world_travel.x);
				camera->position.y = LEGACY_S16_WRAP_ADD(
					camera->position.y, wheel_world_travel.y);
				camera->position.z = LEGACY_S16_WRAP_ADD(
					camera->position.z, wheel_world_travel.z);
			}
		}

		camera->yaw = LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_S16_WRAP_NEGATE(polarAngle(
				LEGACY_S16_WRAP_SUB(car_pos.x, camera->position.x),
				LEGACY_S16_WRAP_SUB(car_pos.z, camera->position.z))) & ANGLE_MASK);
		camera_horizontal_distance = polarRadius2D(
			LEGACY_S16_WRAP_SUB(car_pos.x, camera->position.x),
			LEGACY_S16_WRAP_SUB(car_pos.z, camera->position.z));
		camera->pitch = LEGACY_S16_FROM_BITS((legacy_u16)polarAngle(
			LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_SUB(car_pos.y, camera->position.y),
				FRAME_CAMERA_TARGET_HEIGHT_OFFSET),
			camera_horizontal_distance) & ANGLE_MASK);
	}

	if (camera_roll > 1 && camera_roll < ANGLE_MASK) {
		camera->roll = camera_roll;
	} else {
		camera->roll = 0;
	}
}

static legacy_s8 frame_animated_material(void)
{
	legacy_s8 animated_material;

	if (state.game_frame == 0) {
		animated_material = track_material_animation[
			frame_callback_count & FRAME_ANIMATION_PHASE_MASK];
	} else {
		animated_material = track_material_animation[
			state.game_frame & FRAME_ANIMATION_PHASE_MASK];
	}

	return animated_material;
}

static const struct FRAME_LOOKAHEAD_TILE* frame_setup_projection(
	struct FRAME_CAMERA* camera, struct RECTANGLE* cliprect)
{
	struct VECTOR offset_vector;
	struct VECTOR shape_relative_position;
	legacy_s16 heading;
	const struct FRAME_LOOKAHEAD_TILE* lookahead_tiles;

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

	heading = select_cliprect_rotate(camera->roll, camera->pitch, camera->yaw, cliprect, 0);
	lookahead_tiles = (const struct FRAME_LOOKAHEAD_TILE*)
		lookahead_tiles_tables[(heading & ANGLE_MASK) >>
			FRAME_LOOKAHEAD_HEADING_SHIFT];

	camera->pitch_roll_rotation = *mat_rot_zxy(camera->roll, camera->pitch, 0,
		MATRIX_ROTATION_ORDER_YXZ);
	offset_vector.x = 0;
	offset_vector.y = 0;
	offset_vector.z = FRAME_SKYBOX_TEST_DISTANCE;
	mat_mul_vector(&offset_vector, &camera->pitch_roll_rotation, &shape_relative_position);
	if (shape_relative_position.z > 0) {
		camera->skybox_parameter = 1;
	} else {
		camera->skybox_parameter = -1;
	}

	return lookahead_tiles;
}

static void frame_draw_clouds(struct FRAME_CAMERA* camera,
	legacy_s8 redraw_transform_flags)
{
	legacy_s16 cloud_angle;
	struct MATRIX cloud_heading_rotation;
	struct VECTOR offset_vector;
	struct VECTOR rotated_position;
	legacy_s16 transform_result;
	legacy_s16 cloud_index;

	// Draw the eight cloud shapes at full detail.
	if (detail_level == FRAME_DETAIL_FULL) {
		currenttransshape->rectptr = &frame_cloud_rect;
		currenttransshape->ts_flags = redraw_transform_flags | FRAME_DISTANT_SHAPE_FLAGS;
		currenttransshape->rotvec.x = 0;
		currenttransshape->rotvec.y = 0;
		currenttransshape->culling_distance = FRAME_DEFAULT_TRANSFORM_DISTANCE;
		currenttransshape->material = 0;

		for (cloud_index = 0; cloud_index < FRAME_DISTANT_SHAPE_COUNT;
			cloud_index++) {
			cloud_angle = LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(
					cloud_heading_offsets[cloud_index], camera->yaw),
				run_game_random) & ANGLE_MASK);
			if (cloud_angle < FRAME_DISTANT_SHAPE_MIN_ANGLE ||
				cloud_angle > FRAME_DISTANT_SHAPE_MAX_ANGLE) {
				mat_rot_y(&cloud_heading_rotation, cloud_angle);
				offset_vector.x = 0;
				offset_vector.y = LEGACY_S16_WRAP_SUB(
					FRAME_DISTANT_SHAPE_HEIGHT, camera->position.y);
				offset_vector.z = FRAME_DISTANT_SHAPE_DISTANCE;
				mat_mul_vector(&offset_vector, &cloud_heading_rotation, &rotated_position);
				rotated_position.z = FRAME_DISTANT_SHAPE_DISTANCE;
				mat_mul_vector(&rotated_position, &camera->pitch_roll_rotation,
					&currenttransshape->pos);
				if (currenttransshape->pos.z >
					FRAME_DISTANT_SHAPE_MIN_DEPTH) {
					currenttransshape->shapeptr = cloud_shapes[cloud_index];
					currenttransshape->rotvec.z =
						LEGACY_S16_WRAP_NEGATE(camera->yaw);
					transform_result = shape3d_transform_and_queue(&currenttransshape[0]);
					(void) transform_result; // we cannot be out of memory as we are just starting to process
				}
			}
		}
	}
}

static void frame_resolve_track_tile(struct FRAME_TILE* tile)
{
	if (tile->element != 0) {

		if (tile->terrain >= FRAME_HILL_ROAD_TERRAIN_FIRST &&
			tile->terrain < FRAME_HILL_ROAD_TERRAIN_END) {
			tile->element = subst_hillroad_track(tile->terrain, tile->element);
			tile->terrain = 0;
		}

		// Found a filler tile (non-main tile of a multitile component)
		// Process the main tile of the component instead (the NW one)
		if (tile->element ==
			TRACK_TILE_CONTINUATION_SOUTHEAST) {
			tile->east = LEGACY_S8_WRAP_SUB(tile->east, 1);
			tile->south = LEGACY_S8_WRAP_SUB(tile->south, 1);
			tile->element = track_element_map[tile->east + trackrows[tile->south]];
			tile->terrain = track_terrain_map[tile->east + terrainrows[tile->south]];
		} else if (tile->element ==
			TRACK_TILE_CONTINUATION_SOUTH) {
			tile->south = LEGACY_S8_WRAP_SUB(tile->south, 1);
			tile->element = track_element_map[tile->east + trackrows[tile->south]];
			tile->terrain = track_terrain_map[tile->east + terrainrows[tile->south]];
		} else if (tile->element ==
			TRACK_TILE_CONTINUATION_EAST) {
			tile->east = LEGACY_S8_WRAP_SUB(tile->east, 1);
			tile->element = track_element_map[tile->east + trackrows[tile->south]];
			tile->terrain = track_terrain_map[tile->east + terrainrows[tile->south]];
		}
	}
}

static void frame_mark_covered_tiles(struct FRAME_TILE_SELECTION* tiles,
	const struct FRAME_TILE* tile, legacy_s16 tile_index)
{
	legacy_s8 tile_to_draw_south_offset;
	legacy_s8 tile_to_draw_east_offset;
	legacy_s16 multitile_flag;
	legacy_s16 covered_index;

	if (tile->element != 0) {
		multitile_flag = trkObjectList[tile->element].ss_multiTileFlag;
		if (multitile_flag != FRAME_MULTITILE_NONE) {
			// Look at future tiles to process (with lower index, since tile_index
			// counts backwards) and remove those which belong to the same
			// multi-tile component as this tile

			// Recalculate the offset (needed in case we hit a filler tile)
			tile_to_draw_east_offset = LEGACY_S8_WRAP_SUB(
				tile->east, tiles->camera_east);
			tile_to_draw_south_offset = LEGACY_S8_WRAP_SUB(
				tile->south, tiles->camera_south);
			if (multitile_flag == FRAME_MULTITILE_ROW) {
				for (covered_index = 0; covered_index < tile_index; covered_index++) {
					if (tiles->lookahead[covered_index].east == tile_to_draw_east_offset &&
						(tiles->lookahead[covered_index].south == tile_to_draw_south_offset ||
						tiles->lookahead[covered_index].south == tile_to_draw_south_offset + 1)) {
						tiles->markers[covered_index] =
							FRAME_TILE_MULTITILE_COVERED_MARKER;
					}
				}
			} else if (multitile_flag == FRAME_MULTITILE_COLUMN) {
				for (covered_index = 0; covered_index < tile_index; covered_index++) {
					if (tiles->lookahead[covered_index].south == tile_to_draw_south_offset &&
						(tiles->lookahead[covered_index].east == tile_to_draw_east_offset ||
						tiles->lookahead[covered_index].east == tile_to_draw_east_offset + 1)) {
						tiles->markers[covered_index] =
							FRAME_TILE_MULTITILE_COVERED_MARKER;
					}
				}
			} else if (multitile_flag == FRAME_MULTITILE_BOTH) {
				for (covered_index = 0; covered_index < tile_index; covered_index++) {
					if ((tiles->lookahead[covered_index].east == tile_to_draw_east_offset ||
						tiles->lookahead[covered_index].east == tile_to_draw_east_offset + 1) &&
						(tiles->lookahead[covered_index].south == tile_to_draw_south_offset ||
							tiles->lookahead[covered_index].south == tile_to_draw_south_offset + 1))
					{
						tiles->markers[covered_index] =
							FRAME_TILE_MULTITILE_COVERED_MARKER;
					}
				}
			}
		}
	}
}

static void frame_select_tiles(struct FRAME_TILE_SELECTION* tiles,
	const struct FRAME_CAMERA* camera)
{
	legacy_s16 tile_index;
	legacy_s8 detail_threshold;
	struct FRAME_TILE tile;

	tiles->camera_east = LEGACY_S8_FROM_BITS(
		(legacy_u8)LEGACY_S16_SAR(camera->position.x, FRAME_CAMERA_TILE_SHIFT));
	tiles->camera_south = LEGACY_S8_WRAP_SUB(TRACK_GRID_LAST_COORDINATE,
		LEGACY_S16_SAR(camera->position.z, FRAME_CAMERA_TILE_SHIFT));
	if (detail_level != FRAME_DETAIL_FULL) {
		tiles->player_east = frame_tile_from_world(
			state.playerstate.car_position.lx);
		tiles->player_south = frame_south_tile_from_world(
			state.playerstate.car_position.lz);
	}

	for (tile_index = 0; tile_index < FRAME_LOOKAHEAD_TILE_COUNT; tile_index++) {
		tiles->markers[tile_index] = FRAME_TILE_DRAW_MARKER;
	}

	// Select the detail level (FULL if 1st or 2nd option in the graphics menu
	// were chosen, MEDIUM if the 3rd, FASTEST if 4th or 5th)
	detail_threshold = detail_threshold_by_level[detail_level];

	// Cycle on the 23 tiles to draw, determine if they really need to be drawn
	for (tile_index = FRAME_LOOKAHEAD_LAST_TILE_INDEX; tile_index >= 0; tile_index--) {

		// Skip if a previous iteration determined this tile is not needed
		// (happens for multi-tile elements)
		if (tiles->markers[tile_index] != FRAME_TILE_DRAW_MARKER)
			continue;

		// Skip if detail threshold not met (e.g. far tiles in FASTEST detail)
		if (tiles->lookahead[tile_index].detail <= detail_threshold) {
			tile.east = LEGACY_S8_WRAP_ADD(
				tiles->lookahead[tile_index].east, tiles->camera_east);
			tile.south = LEGACY_S8_WRAP_ADD(
				tiles->lookahead[tile_index].south, tiles->camera_south);

			// Skip if tile is out of bounds
			if (tile.east >= 0 &&
				tile.east <= TRACK_GRID_LAST_COORDINATE &&
				tile.south >= 0 &&
				tile.south <= TRACK_GRID_LAST_COORDINATE) {
				tile.element = track_element_map[tile.east + trackrows[tile.south]];
				tile.terrain = track_terrain_map[tile.east + terrainrows[tile.south]];

				frame_resolve_track_tile(&tile);
				tiles->terrain[tile_index] = tile.terrain;
				tiles->detail[tile_index] = tiles->lookahead[tile_index].detail;

				if (tile.element != 0 &&
					detail_level != FRAME_DETAIL_FULL &&
					trkObjectList[tile.element].ss_physicalModel >=
						FRAME_SCENERY_PHYSICAL_MODEL_FIRST &&
					(tile.east != tiles->player_east || tile.south != tiles->player_south))
				{
					tile.element = 0;
				}

				tiles->east[tile_index] = tile.east;
				tiles->south[tile_index] = tile.south;
				tiles->elements[tile_index] = tile.element;

				frame_mark_covered_tiles(tiles, &tile, tile_index);

			} else {
				tiles->markers[tile_index] = FRAME_TILE_UNAVAILABLE_MARKER;
			}
		} else {
			tiles->markers[tile_index] = FRAME_TILE_UNAVAILABLE_MARKER;
		}
	}
}

static void frame_place_cars(const struct FRAME_TILE_SELECTION* tiles,
	struct FRAME_CAR_RENDER* cars)
{
	// Locate the player in the visible tile list using its wheels.
	cars[PLAYER_CAR_INDEX].east = -1;
	cars[PLAYER_CAR_INDEX].depth_adjustment = 0;
	if (cameramode != CAMERA_MODE_COCKPIT ||
		followOpponentFlag != 0) {

		if (state.playerstate.car_crashBmpFlag !=
			CRASH_EVENT_WATER) {

			cars[PLAYER_CAR_INDEX].depth_adjustment = frame_find_car_wheel(&state.playerstate,
				&simd_player, tiles->markers, tiles->lookahead,
				tiles->camera_east, tiles->camera_south, &cars[PLAYER_CAR_INDEX].east,
					&cars[PLAYER_CAR_INDEX].south);
		}
	}

	// Locate the opponent in the same draw order.
	cars[OPPONENT_CAR_INDEX].east = -1;
	cars[OPPONENT_CAR_INDEX].depth_adjustment = 0;
	if (gameconfig.game_opponenttype != 0) {

		if (cameramode != CAMERA_MODE_COCKPIT ||
			followOpponentFlag == 0) {
			if (state.opponentstate.car_crashBmpFlag !=
				CRASH_EVENT_WATER) {
				cars[OPPONENT_CAR_INDEX].depth_adjustment = frame_find_car_wheel(&state.opponentstate,
					&simd_opponent, tiles->markers, tiles->lookahead,
					tiles->camera_east, tiles->camera_south, &cars[OPPONENT_CAR_INDEX].east,
						&cars[OPPONENT_CAR_INDEX].south);
			}
		}
	}
}

/* Exhaustion stops only the fence pass, as in the original tile loop. */
static void frame_draw_fences(const struct FRAME_TILE* tile,
	const struct FRAME_TILE_SELECTION* tiles, const struct FRAME_CAMERA* camera,
	legacy_s8 redraw_transform_flags)
{
	legacy_s16 transform_result;
	legacy_s16 fence_position_count;
	legacy_s8 tile_to_draw_south_offset;
	legacy_s8 tile_to_draw_east_offset;
	legacy_s16 position_index;
	struct TRACKOBJECT* fence_object;
	struct TRACKOBJECT* track_object;
	legacy_s8* fence_tile_offsets;
	legacy_s16 fence_index;

	if (tile->element == 0) {
		fence_position_count = 1;
		fence_tile_offsets = fence_tile_offsets_column;
	} else {
		track_object = &trkObjectList[tile->element];
		if (track_object->ss_multiTileFlag ==
			FRAME_MULTITILE_NONE) {
			fence_position_count = FRAME_FENCE_POSITION_COUNT_SINGLE;
			fence_tile_offsets = fence_tile_offsets_single;
		} else if (track_object->ss_multiTileFlag ==
			FRAME_MULTITILE_ROW) {
			fence_position_count = FRAME_FENCE_POSITION_COUNT_ROW;
			fence_tile_offsets = fence_tile_offsets_row;
		} else if (track_object->ss_multiTileFlag ==
			FRAME_MULTITILE_COLUMN) {
			fence_position_count = FRAME_FENCE_POSITION_COUNT_COLUMN;
			fence_tile_offsets = fence_tile_offsets_column;
		} else if (track_object->ss_multiTileFlag ==
			FRAME_MULTITILE_BOTH) {
			fence_position_count = FRAME_FENCE_POSITION_COUNT_BOTH;
			fence_tile_offsets = fence_tile_offsets_both;
		}
	}

	// Draw the fence
	for (position_index = 0; position_index < fence_position_count; position_index++) {
		tile_to_draw_east_offset = LEGACY_S8_WRAP_ADD(
			fence_tile_offsets[position_index * FRAME_FENCE_POSITION_STRIDE], tile->east);
		tile_to_draw_south_offset = LEGACY_S8_WRAP_ADD(
			fence_tile_offsets[position_index * FRAME_FENCE_POSITION_STRIDE +
				FRAME_FENCE_SECOND_COORDINATE], tile->south);

		if (detail_level == FRAME_DETAIL_FULL ||
			(tile_to_draw_east_offset == tiles->player_east &&
				tile_to_draw_south_offset == tiles->player_south)) {
			fence_index = fence_by_edge[frame_border_index(
				tile_to_draw_east_offset)][frame_border_index(
				tile_to_draw_south_offset)];

			if (fence_index != FRAME_FENCE_NONE) {
				fence_object = frame_track_object_from_legacy_index(
					fence_TrkObjCodes[fence_index]);
				if (tile->detail == FRAME_TILE_DETAIL_FULL) {
					currenttransshape->shapeptr = fence_object->ss_shapePtr;
				} else {
					currenttransshape->shapeptr = fence_object->ss_loShapePtr;
				}

				frame_prepare_flat_track_shape(currenttransshape,
					tile_to_draw_east_offset,
					tile_to_draw_south_offset,
					&camera->position, (legacy_s16)(redraw_transform_flags |
						FRAME_TRANSFORM_FLAGS_NO_DEPTH_SORT),
					fence_rotations[fence_index]);
				transform_result = shape3d_transform_and_queue(&currenttransshape[0]);
				if (transform_result > 0) {
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
}

/* Return nonzero only when the ordinary terrain pass stops tile rendering.
   Elevated corners stop their own pass on exhaustion. */
static legacy_s16 frame_draw_terrain(struct FRAME_TILE* tile,
	const struct FRAME_CAMERA* camera, legacy_s8 redraw_transform_flags)
{
	legacy_s16 transform_result;
	legacy_s16 corner_index;
	struct TRACKOBJECT* track_object;

	// Elevated terrain is a flat piece of land at an elevated level.
	if (tile->terrain != TERRAIN_RAISED_TILE) {
		tile->height = 0;

		// Special treatment of elevated corners
		if (tile->element >= FRAME_ELEVATED_CORNER_FIRST &&
			tile->element <= FRAME_ELEVATED_CORNER_LAST) {
			for (corner_index = 0; corner_index < FRAME_ELEVATED_CORNER_COUNT; corner_index++) {
				if (corner_index == FRAME_CORNER_NORTHWEST) {
					tile->last_east = tile->east;
					tile->last_south = tile->south;
				} else if (corner_index == FRAME_CORNER_NORTHEAST) {
					tile->last_east = LEGACY_S8_WRAP_ADD(
						tile->east, 1);
					tile->last_south = tile->south;
				} else if (corner_index == FRAME_CORNER_SOUTHWEST) {
					tile->last_east = tile->east;
					tile->last_south = LEGACY_S8_WRAP_ADD(
						tile->south, 1);
				} else if (corner_index == FRAME_CORNER_SOUTHEAST) {
					tile->last_east = LEGACY_S8_WRAP_ADD(
						tile->east, 1);
					tile->last_south = LEGACY_S8_WRAP_ADD(
						tile->south, 1);
				}
				tile->terrain = track_terrain_map[tile->last_east + terrainrows[tile->last_south]];
				if (tile->terrain != 0) {
					track_object = &terrain_scene_objects[tile->terrain];
					currenttransshape->shapeptr = track_object->ss_shapePtr;
					frame_prepare_flat_track_shape(currenttransshape,
						tile->last_east,
						tile->last_south,
						&camera->position, (legacy_s16)(redraw_transform_flags |
							FRAME_TRANSFORM_FLAGS_NO_DEPTH_SORT),
						track_object->ss_rotY);
					transform_result = shape3d_transform_and_queue(&currenttransshape[0]);
					if (transform_result > 0)
						break;
				}
			}

			tile->terrain = 0;
		}
	} else {
		tile->height =
			hillHeightConsts[TERRAIN_RAISED_HEIGHT_INDEX];
		if (tile->element != 0) {
			tile->terrain = 0;
		}
	}

	// The rest of the rendering loop still needs to be analyzed in detail.
	// Anyway, the gist is that every tile is associated with various shape,
	// each of which is rendered via a call to `shape3d_transform_and_queue`. The
	// result of such fn is checked each time, since a return value of 1
	// means we ran out of memory

	if (tile->terrain != 0) {
		track_object = &terrain_scene_objects[tile->terrain];
		currenttransshape->shapeptr = track_object->ss_shapePtr;
		currenttransshape->pos.x = LEGACY_S16_WRAP_SUB(
			track_column_centers[tile->east], camera->position.x);
		currenttransshape->pos.y = LEGACY_S16_WRAP_SUB(
			tile->height, camera->position.y);
		currenttransshape->pos.z = LEGACY_S16_WRAP_SUB(
			track_row_centers[tile->south], camera->position.z);
		if (tile->height == 0) {
			currenttransshape->rectptr = &frame_unsorted_shapes_rect;
		} else {
			currenttransshape->rectptr = &frame_sorted_shapes_rect;
		}

		currenttransshape->ts_flags = redraw_transform_flags |
			FRAME_TRANSFORM_FLAGS_NO_DEPTH_SORT;
		currenttransshape->rotvec.x = 0;
		currenttransshape->rotvec.y = 0;
		currenttransshape->rotvec.z = track_object->ss_rotY;
		currenttransshape->culling_distance = FRAME_DEFAULT_TRANSFORM_DISTANCE;
		currenttransshape->material = 0;
		transform_result = shape3d_transform_and_queue(&currenttransshape[0]);
		if (transform_result > 0)
			return 1;
	}

	return 0;
}

/* Hill-fill exhaustion stops this pass; the track element still follows. */
static void frame_draw_hill_fill(const struct FRAME_TILE* tile,
	const struct TRACKOBJECT* track_object, legacy_s8 redraw_transform_flags)
{
	legacy_s16 transform_result;
	legacy_s16 fill_index;
	legacy_s16 fill_count;
	legacy_s16* hill_fill_offsets;

	if (tile->height != 0) {
		if (track_object->ss_multiTileFlag ==
			FRAME_MULTITILE_NONE) {
			fill_count = FRAME_HILL_FILL_COUNT_SINGLE;
			hill_fill_offsets = hill_fill_offsets_single;
		} else if (track_object->ss_multiTileFlag ==
			FRAME_MULTITILE_ROW) {
			fill_count = FRAME_HILL_FILL_COUNT_ROW;
			hill_fill_offsets = hill_fill_offsets_row;
		} else if (track_object->ss_multiTileFlag ==
			FRAME_MULTITILE_COLUMN) {
			fill_count = FRAME_HILL_FILL_COUNT_COLUMN;
			hill_fill_offsets = hill_fill_offsets_column;
		} else if (track_object->ss_multiTileFlag ==
			FRAME_MULTITILE_BOTH) {
			fill_count = FRAME_HILL_FILL_COUNT_BOTH;
			hill_fill_offsets = hill_fill_offsets_both;
		}

		for (fill_index = 0; fill_index < fill_count; fill_index++) {
			currenttransshape->pos.x = LEGACY_S16_WRAP_ADD(
				*hill_fill_offsets, tile->position.x);
			hill_fill_offsets++;
			currenttransshape->pos.y = tile->position.y;
			currenttransshape->pos.z = LEGACY_S16_WRAP_ADD(
				*hill_fill_offsets, tile->position.z);
			hill_fill_offsets++;
			currenttransshape->shapeptr = &game3dshapes[
				FRAME_HILL_FILL_SHAPE_RESOURCE_OFFSET /
				sizeof(struct SHAPE3D)];
			currenttransshape->rectptr = &frame_sorted_shapes_rect;
			currenttransshape->ts_flags = redraw_transform_flags |
				FRAME_TRANSFORM_FLAGS_NO_DEPTH_SORT;
			currenttransshape->rotvec.x = 0;
			currenttransshape->rotvec.y = 0;
			currenttransshape->rotvec.z = 0;
			currenttransshape->culling_distance =
				FRAME_SINGLE_TILE_TRANSFORM_DISTANCE;
			currenttransshape->material = 0;
			transform_result = shape3d_transform_and_queue(&currenttransshape[0]);
			if (transform_result > 0)
				break;
		}
	}
}

static legacy_s16 frame_prepare_overlay(const struct FRAME_TILE* tile,
	const struct TRACKOBJECT* track_object, legacy_s8 redraw_transform_flags,
	legacy_s8 animated_material, legacy_s8* overlay_needs_depth_sort)
{
	legacy_s16 transform_result;
	struct TRACKOBJECT* overlay_track_object;

	if (track_object->ss_ssOvelay != 0) {
		overlay_track_object = frame_track_object_from_legacy_index(
			track_object->ss_ssOvelay);
		if (tile->detail != FRAME_TILE_DETAIL_FULL) {
			currenttransshape[1].shapeptr = overlay_track_object->ss_loShapePtr;
		} else {
			currenttransshape[1].shapeptr = overlay_track_object->ss_shapePtr;
		}

		if (currenttransshape[1].shapeptr != 0) {
			currenttransshape[1].pos = tile->position;
			currenttransshape[1].rotvec.x = 0;
			currenttransshape[1].rotvec.y = 0;
			currenttransshape[1].rotvec.z = overlay_track_object->ss_rotY;
			if (overlay_track_object->ss_multiTileFlag !=
				FRAME_MULTITILE_NONE) {
				currenttransshape[1].culling_distance =
					FRAME_DEFAULT_TRANSFORM_DISTANCE;
			} else {
				currenttransshape[1].culling_distance =
					FRAME_SINGLE_TILE_TRANSFORM_DISTANCE;
			}

			if (overlay_track_object->ss_surfaceType >= 0) {
				currenttransshape[1].material = overlay_track_object->ss_surfaceType;
			} else {
				currenttransshape[1].material = animated_material;
			}

			currenttransshape[1].ts_flags =
				overlay_track_object->ss_ignoreZBias | redraw_transform_flags |
				FRAME_TRANSFORM_FLAGS_DEFAULT;
			if ((currenttransshape[1].ts_flags &
				FRAME_NO_DEPTH_SORT_FLAG) != 0) {
				currenttransshape[1].rectptr = &frame_unsorted_shapes_rect;
				transform_result = shape3d_transform_and_queue(&currenttransshape[1]);
				if (transform_result > 0)
					return 1;
			} else {
				currenttransshape[1].rectptr = &frame_sorted_shapes_rect;
				*overlay_needs_depth_sort = 1;
			}
		}
	}

	return 0;
}

static void frame_add_roadside_sign(const struct FRAME_TILE* tile,
	const struct FRAME_CAMERA* camera, legacy_s8 redraw_transform_flags)
{
	struct TRACKOBJECT* track_object;
	legacy_s16 particle_index;
	legacy_u8 breakable_object_index;

	breakable_object_index = roadside_sign_indices_by_tile[tile->east + trackrows[tile->south]];
	if (breakable_object_index != FRAME_CHECKPOINT_NONE) {
		if (state.game_object_destroyed[breakable_object_index] == 0) {
			track_object = &trkObjectList[
				FRAME_CHECKPOINT_TRACK_OBJECT_BASE +
				roadside_sign_shape_indices[breakable_object_index]];
			curtransshape_ptr->pos.x = LEGACY_S16_WRAP_SUB(
				roadside_sign_positions[breakable_object_index].x, camera->position.x);
			curtransshape_ptr->pos.y = LEGACY_S16_WRAP_SUB(
				roadside_sign_positions[breakable_object_index].y, camera->position.y);
			curtransshape_ptr->pos.z = LEGACY_S16_WRAP_SUB(
				roadside_sign_positions[breakable_object_index].z, camera->position.z);
			curtransshape_ptr->shapeptr = track_object->ss_shapePtr;
			curtransshape_ptr->rectptr = &frame_sorted_shapes_rect;
			curtransshape_ptr->ts_flags = redraw_transform_flags |
				FRAME_TRANSFORM_FLAGS_DEFAULT;
			curtransshape_ptr->rotvec.x = 0;
			curtransshape_ptr->rotvec.y = 0;
			curtransshape_ptr->rotvec.z = roadside_sign_headings[breakable_object_index];
			curtransshape_ptr->culling_distance =
				FRAME_CHECKPOINT_TRANSFORM_DISTANCE;
			curtransshape_ptr->material = 0;
			transformed_shape_add_for_sort(0, 0);
		} else if (state.game_particles_active != 0) {
			for (particle_index = 0; particle_index < FRAME_DEBRIS_SLOT_COUNT; particle_index++) {
				if (state.game_particle_forward_speed[particle_index] != 0 &&
					breakable_object_index + FRAME_CHECKPOINT_OWNER_OFFSET ==
						state.game_particle_owner[particle_index]) {
					track_object = &particle_scene_objects[state.game_particle_shape_index[particle_index]];
					curtransshape_ptr->pos.x = frame_relative_track_position(
						state.game_particle_x[particle_index],
						roadside_sign_positions[breakable_object_index].x, camera->position.x);
					curtransshape_ptr->pos.y = frame_relative_track_position(
						state.game_particle_y[particle_index],
						roadside_sign_positions[breakable_object_index].y, camera->position.y);
					curtransshape_ptr->pos.z = frame_relative_track_position(
						state.game_particle_z[particle_index],
						roadside_sign_positions[breakable_object_index].z, camera->position.z);
					frame_add_dynamic_shape(track_object, particle_index,
						redraw_transform_flags |
							FRAME_TRANSFORM_FLAGS_NO_DEPTH_SORT,
						0, 0);
				}
			}
		}
	}
}

static void frame_add_start_flag(const struct FRAME_TILE* tile,
	const struct FRAME_CAMERA* camera, legacy_s8 redraw_transform_flags)
{
	legacy_s16 flag_z;
	legacy_s16 flag_x_or_material;
	legacy_u16 vertex_index;
	struct VECTOR start_flag_vertices[FRAME_START_FLAG_VERTEX_COUNT];

	if (state.game_inputmode == GAME_INPUT_MODE_WAITING) {
		if ((tile->east == start_finish_column || tile->last_east == start_finish_column) &&
			(tile->south == start_finish_row || tile->last_south == start_finish_row)) {

			flag_x_or_material = multiply_and_scale(cos_fast(start_flag_animation),
				FRAME_START_FLAG_RADIUS);
			flag_z = LEGACY_S16_WRAP_ADD(
				multiply_and_scale(sin_fast(start_flag_animation),
					FRAME_START_FLAG_RADIUS),
				FRAME_START_FLAG_CENTER_Z);

			for (vertex_index = 0;
				vertex_index < FRAME_START_FLAG_VERTEX_COUNT;
				vertex_index++)
				shape3d_vertex_read(
					&game3dshapes[FRAME_START_FLAG_RESOURCE_OFFSET /
						sizeof(struct SHAPE3D)],
					LEGACY_U16_WRAP_ADD(FRAME_START_FLAG_FIRST_VERTEX,
						vertex_index),
					&start_flag_vertices[vertex_index]);
			start_flag_vertices[FRAME_START_FLAG_VERTEX_LEFT_NEAR].x =
				LEGACY_S16_WRAP_SUB(flag_x_or_material,
				FRAME_START_FLAG_RADIUS);
			start_flag_vertices[FRAME_START_FLAG_VERTEX_LEFT_FAR].x =
				LEGACY_S16_WRAP_SUB(flag_x_or_material,
				FRAME_START_FLAG_RADIUS);
			start_flag_vertices[FRAME_START_FLAG_VERTEX_RIGHT_NEAR].x =
				LEGACY_S16_WRAP_SUB(
				FRAME_START_FLAG_RADIUS, flag_x_or_material);
			start_flag_vertices[FRAME_START_FLAG_VERTEX_RIGHT_FAR].x =
				LEGACY_S16_WRAP_SUB(
				FRAME_START_FLAG_RADIUS, flag_x_or_material);

			start_flag_vertices[FRAME_START_FLAG_VERTEX_LEFT_NEAR].z = flag_z;
			start_flag_vertices[FRAME_START_FLAG_VERTEX_LEFT_FAR].z = flag_z;
			start_flag_vertices[FRAME_START_FLAG_VERTEX_RIGHT_NEAR].z = flag_z;
			start_flag_vertices[FRAME_START_FLAG_VERTEX_RIGHT_FAR].z = flag_z;
			for (vertex_index = 0;
				vertex_index < FRAME_START_FLAG_VERTEX_COUNT;
				vertex_index++)
				shape3d_vertex_write(
					&game3dshapes[FRAME_START_FLAG_RESOURCE_OFFSET /
						sizeof(struct SHAPE3D)],
					LEGACY_U16_WRAP_ADD(FRAME_START_FLAG_FIRST_VERTEX,
						vertex_index),
					&start_flag_vertices[vertex_index]);

			curtransshape_ptr->pos.x = LEGACY_S16_WRAP_SUB(
				LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
					multiply_and_scale(sin_fast(LEGACY_S16_WRAP_ADD(
						track_angle, ANGLE_QUARTER_TURN)),
						FRAME_START_FLAG_RADIUS),
					multiply_and_scale(sin_fast(LEGACY_S16_WRAP_ADD(
						track_angle, ANGLE_HALF_TURN)),
						FRAME_START_FLAG_FAR_OFFSET)),
					track_column_centers[start_finish_column]), camera->position.x);
			curtransshape_ptr->pos.y = LEGACY_S16_WRAP_SUB(
				hillHeightConsts[hillFlag], camera->position.y);
			curtransshape_ptr->pos.z = LEGACY_S16_WRAP_SUB(
				LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
					multiply_and_scale(cos_fast(LEGACY_S16_WRAP_ADD(
						track_angle, ANGLE_QUARTER_TURN)),
						FRAME_START_FLAG_RADIUS),
					multiply_and_scale(cos_fast(LEGACY_S16_WRAP_ADD(
						track_angle, ANGLE_HALF_TURN)),
						FRAME_START_FLAG_FAR_OFFSET)),
					track_row_centers[start_finish_row]), camera->position.z);

			curtransshape_ptr->shapeptr = &game3dshapes[
				FRAME_START_FLAG_RESOURCE_OFFSET / sizeof(struct SHAPE3D)];
			curtransshape_ptr->rectptr = &frame_sorted_shapes_rect;
			curtransshape_ptr->ts_flags = redraw_transform_flags |
				FRAME_TRANSFORM_FLAGS_DEFAULT;
			curtransshape_ptr->rotvec.x = 0;
			curtransshape_ptr->rotvec.y = 0;
			curtransshape_ptr->rotvec.z = track_angle;
			curtransshape_ptr->culling_distance = FRAME_DEFAULT_TRANSFORM_DISTANCE;
			flag_x_or_material = LEGACY_S16_SAR(start_flag_animation,
				FRAME_START_FLAG_ANIMATION_SHIFT);
			if (flag_x_or_material > FRAME_START_FLAG_MAX_MATERIAL) {
				flag_x_or_material = FRAME_START_FLAG_MAX_MATERIAL;
			}

			curtransshape_ptr->material = flag_x_or_material;
			transformed_shape_add_for_sort(tile->depth_mask &
				-FRAME_SINGLE_TILE_TRANSFORM_DISTANCE, 0);
		}
	}
}

/* Exhaustion stops the current tile's sorted shapes, then advances the tile. */
static void frame_draw_sorted_shapes(struct FRAME_CAR_RENDER* cars)
{
	legacy_s16 transform_result;
	legacy_s16 sort_index;
	legacy_s16 shape_index;

	if (transformedshape_counter != 0) {
		if (transformedshape_counter >=
			FRAME_SORT_MINIMUM_SHAPE_COUNT) {
			heapsort_by_order(transformedshape_counter, transformedshape_zarray,
				transformedshape_indices);
		}

		// Draw red overlights on the brake lights on own and opponent's car
		for (sort_index = 0; sort_index < transformedshape_counter; sort_index++) {
			shape_index = transformedshape_indices[sort_index];
			if (transformed_shape_sort_types[shape_index] == FRAME_PLAYER_SORT_ID) {
				if (state.playerstate.car_is_braking != 0) {
					backlights_paint_override =
						BACKLIGHT_PAINT_BRAKING;
				} else {
					backlights_paint_override =
						BACKLIGHT_PAINT_NORMAL;
				}
			} else if (transformed_shape_sort_types[shape_index] ==
				FRAME_OPPONENT_SORT_ID) {
				if (state.opponentstate.car_is_braking == 0) {
					backlights_paint_override =
						BACKLIGHT_PAINT_NORMAL;
				} else {
					backlights_paint_override =
						BACKLIGHT_PAINT_BRAKING;
				}
			}

			transform_result = shape3d_transform_and_queue(&currenttransshape[shape_index]);
			if (transform_result > 0)
				break;

			if (transform_result == 0) {
				if (transformed_shape_sort_types[shape_index] ==
					FRAME_PLAYER_SORT_ID) {
					if (state.playerstate.car_crashBmpFlag ==
						CRASH_EVENT_COLLISION) {
						cars[PLAYER_CAR_INDEX].explosion_visible = 1;
					}
				} else if (transformed_shape_sort_types[shape_index] ==
					FRAME_OPPONENT_SORT_ID) {
					if (state.opponentstate.car_crashBmpFlag ==
						CRASH_EVENT_COLLISION) {
						cars[OPPONENT_CAR_INDEX].explosion_visible = 1;
					}
				}
			}
		}
	}
}

static legacy_s16 frame_add_track_element(struct FRAME_TILE* tile,
	const struct FRAME_CAMERA* camera, struct FRAME_CAR_RENDER* cars,
	legacy_s8 redraw_transform_flags, legacy_s8 animated_material,
	legacy_s8* overlay_needs_depth_sort)
{
	legacy_s16 transform_result;
	struct TRACKOBJECT* track_object;
	legacy_s16 track_object_world_z;
	legacy_s16 track_object_world_x;

	if (tile->element == 0) {
		tile->last_east = tile->east;
		tile->last_south = tile->south;
	} else {
		track_object = &trkObjectList[tile->element];
		if ((track_object->ss_multiTileFlag & FRAME_MULTITILE_ROW) !=
			0) {
			track_object_world_z = track_row_positions[tile->south];
			tile->last_south = LEGACY_S8_WRAP_ADD(
				tile->south, 1);
		} else {
			track_object_world_z = track_row_centers[tile->south];
			tile->last_south = tile->south;
		}

		if ((track_object->ss_multiTileFlag &
			FRAME_MULTITILE_COLUMN) != 0) {
			track_object_world_x = track_column_positions[LEGACY_S8_WRAP_ADD(tile->east, 1)];
			tile->last_east = LEGACY_S8_WRAP_ADD(
				tile->east, 1);
		} else {
			track_object_world_x = track_column_centers[tile->east];
			tile->last_east = tile->east;
		}

		tile->position.x = LEGACY_S16_WRAP_SUB(track_object_world_x, camera->position.x);
		tile->position.y = LEGACY_S16_WRAP_SUB(tile->height, camera->position.y);
		tile->position.z = LEGACY_S16_WRAP_SUB(track_object_world_z, camera->position.z);
		frame_draw_hill_fill(tile, track_object, redraw_transform_flags);

		if (frame_prepare_overlay(tile, track_object, redraw_transform_flags,
			animated_material, overlay_needs_depth_sort) != 0)
			return 1;

		if (tile->detail != FRAME_TILE_DETAIL_FULL) {
			currenttransshape->shapeptr = track_object->ss_loShapePtr;
		} else {
			currenttransshape->shapeptr = track_object->ss_shapePtr;
		}

		currenttransshape->pos = tile->position;
		currenttransshape->rotvec.x = 0;
		currenttransshape->rotvec.y = 0;
		currenttransshape->rotvec.z = track_object->ss_rotY;
		if (track_object->ss_multiTileFlag !=
			FRAME_MULTITILE_NONE) {
			currenttransshape->culling_distance = FRAME_DEFAULT_TRANSFORM_DISTANCE;
		} else {
			currenttransshape->culling_distance =
				FRAME_SINGLE_TILE_TRANSFORM_DISTANCE;
		}

		currenttransshape->ts_flags =
			track_object->ss_ignoreZBias | redraw_transform_flags |
			FRAME_TRANSFORM_FLAGS_DEFAULT;
		if (track_object->ss_surfaceType >= 0) {
			currenttransshape->material = track_object->ss_surfaceType;
		} else {
			currenttransshape->material = animated_material;
		}

		if ((track_object->ss_ignoreZBias &
			FRAME_NO_DEPTH_SORT_FLAG) != 0) {
			currenttransshape->rectptr = &frame_unsorted_shapes_rect;
			transform_result = shape3d_transform_and_queue(&currenttransshape[0]);
			if (transform_result > 0)
				return 1;
		} else {
			currenttransshape->rectptr = &frame_sorted_shapes_rect;
			transformed_shape_add_for_sort(0, 0);
			if (*overlay_needs_depth_sort != 0) {
				*overlay_needs_depth_sort = 0;
				transformed_shape_add_for_sort(
					-FRAME_SINGLE_TILE_TRANSFORM_DISTANCE, 0);
				if (cars[PLAYER_CAR_INDEX].depth_adjustment != 0) {
					cars[PLAYER_CAR_INDEX].depth_adjustment = -FRAME_WHEEL_SORT_ADJUSTMENT;
				}

				if (cars[OPPONENT_CAR_INDEX].depth_adjustment != 0) {
					cars[OPPONENT_CAR_INDEX].depth_adjustment = LEGACY_S16_WRAP_SUB(cars[OPPONENT_CAR_INDEX].depth_adjustment,
						FRAME_WHEEL_SORT_ADJUSTMENT);
				}
			}

			if (tile->east == start_finish_column && tile->south == start_finish_row) {
				tile->depth_mask = 0;
			} else {
				tile->depth_mask = -1;
			}
		}

		frame_add_roadside_sign(tile, camera, redraw_transform_flags);
	}

	return 0;
}

static void frame_add_tile_cars(const struct FRAME_TILE* tile,
	const struct FRAME_CAMERA* camera, struct FRAME_CAR_RENDER* cars,
	legacy_s8 redraw_transform_flags)
{
	if ((cars[PLAYER_CAR_INDEX].east == tile->east ||
		cars[PLAYER_CAR_INDEX].east == tile->last_east) &&
		(cars[PLAYER_CAR_INDEX].south == tile->south ||
		cars[PLAYER_CAR_INDEX].south == tile->last_south)) {
		frame_add_car(&state.playerstate, PLAYER_CAR_INDEX,
			FRAME_PLAYER_SORT_ID,
			&game3dshapes[FRAME_PLAYER_SHAPE_RESOURCE_OFFSET /
				sizeof(struct SHAPE3D)],
			player_wheel_vertex_state, player_base_wheel_vertices, player_front_wheel_centers,
			&frame_player_car_rect, &cars[PLAYER_CAR_INDEX].crash_rect, &camera->position,
				tile->detail,
			redraw_transform_flags, gameconfig.game_playermaterial,
			cars[PLAYER_CAR_INDEX].depth_adjustment & tile->depth_mask);
	}

	if ((cars[OPPONENT_CAR_INDEX].east == tile->east) ||
		(cars[OPPONENT_CAR_INDEX].east == tile->last_east)) {
		if ((cars[OPPONENT_CAR_INDEX].south == tile->south) ||
			(cars[OPPONENT_CAR_INDEX].south == tile->last_south)) {
			frame_add_car(&state.opponentstate, OPPONENT_CAR_INDEX,
				FRAME_OPPONENT_SORT_ID,
				&game3dshapes[FRAME_OPPONENT_SHAPE_RESOURCE_OFFSET /
					sizeof(struct SHAPE3D)],
				opponent_wheel_vertex_state, opponent_base_wheel_vertices,
					opponent_front_wheel_centers,
				&frame_opponent_car_rect, &cars[OPPONENT_CAR_INDEX].crash_rect,
					&camera->position, tile->detail,
				redraw_transform_flags, gameconfig.game_opponentmaterial,
				cars[OPPONENT_CAR_INDEX].depth_adjustment & tile->depth_mask);
		}
	}
}

static void frame_draw_tiles(const struct FRAME_TILE_SELECTION* tiles,
	const struct FRAME_CAMERA* camera, struct FRAME_CAR_RENDER* cars,
	legacy_s8 redraw_transform_flags, legacy_s8 animated_material)
{
	legacy_s16 tile_index;
	legacy_s8 overlay_needs_depth_sort;
	struct FRAME_TILE tile;

	/* A deferred overlay can carry over until a depth-sorted track shape. */
	overlay_needs_depth_sort = 0;

	// With the information collected by the tile-selection pass,
	// proceed to draw the shapes in each tile. Start from the farthest
	// (painter's algorithm)
	for (tile_index = 0; tile_index < FRAME_LOOKAHEAD_TILE_COUNT; tile_index++) {
		if (tiles->markers[tile_index] != FRAME_TILE_DRAW_MARKER) {
			continue;
		}
		tile.east = tiles->east[tile_index];
		tile.south = tiles->south[tile_index];
		tile.element = tiles->elements[tile_index];
		tile.terrain = tiles->terrain[tile_index];
		tile.detail = tiles->detail[tile_index];
		tile.depth_mask = 0;
		frame_draw_fences(&tile, tiles, camera, redraw_transform_flags);

		if (frame_draw_terrain(&tile, camera, redraw_transform_flags) != 0)
			break;

		transformedshape_counter = 0;
		curtransshape_ptr = currenttransshape;
		if (frame_add_track_element(&tile, camera, cars, redraw_transform_flags,
			animated_material, &overlay_needs_depth_sort) != 0)
			break;

		frame_add_tile_cars(&tile, camera, cars, redraw_transform_flags);

		frame_add_start_flag(&tile, camera, redraw_transform_flags);

		frame_draw_sorted_shapes(cars);
	}
}

static void frame_draw_explosions(struct FRAME_CAR_RENDER* cars,
	struct RECTANGLE* cliprect)
{
	legacy_s16 car_index;
	struct RECTANGLE* redraw_rect;
	struct VECTOR offset_vector;
	legacy_s16 height_or_scale;
	legacy_s16 extent;
	legacy_s16 explosion_index;

	// Draw the three explosion images in successive four-frame phases.
	for (car_index = 0; car_index < FRAME_EXPLOSION_CAR_COUNT; car_index++) {
		if (cars[car_index].explosion_visible == 0) {
			continue;
		}
		if (slow_video_mgmt_copy == 0) {
			if (car_index == PLAYER_CAR_INDEX) {
				redraw_rect = &cars[PLAYER_CAR_INDEX].crash_rect;
			} else {
				redraw_rect = &cars[OPPONENT_CAR_INDEX].crash_rect;
			}
		} else {
			if (car_index == PLAYER_CAR_INDEX) {
				redraw_rect = &frame_player_car_rect;
			} else {
				redraw_rect = &frame_opponent_car_rect;
			}
		}

		if (rect_intersect(redraw_rect, cliprect) == 0) {
			sprite_set_target_clip_bounds(redraw_rect->left, redraw_rect->right,
				redraw_rect->top, redraw_rect->bottom);
			offset_vector.x = LEGACY_S16_SAR(LEGACY_S16_WRAP_ADD(
				redraw_rect->right, redraw_rect->left),
				FRAME_RECT_CENTER_SHIFT);
			offset_vector.y = LEGACY_S16_SAR(LEGACY_S16_WRAP_ADD(
				redraw_rect->top, redraw_rect->bottom),
				FRAME_RECT_CENTER_SHIFT);
			extent = LEGACY_S16_WRAP_SUB(
				redraw_rect->right, redraw_rect->left);
			height_or_scale = LEGACY_S16_WRAP_SUB(
				redraw_rect->bottom, redraw_rect->top);
			if (height_or_scale > extent) {
				extent = height_or_scale;
			}

			explosion_index = LEGACY_S16_SAR(state.game_frame,
				FRAME_EXPLOSION_FRAME_SHIFT) % FRAME_EXPLOSION_VARIANT_COUNT;
			height_or_scale = LEGACY_S16_FROM_BITS((legacy_u16)
				LEGACY_S32_DIV_OR_ZERO(
					LEGACY_S32_WRAP_MUL((legacy_s32)extent,
						FRAME_EXPLOSION_FIXED_SCALE),
					(legacy_s32)sdgame2_widths[explosion_index]));
			shape2d_draw_scaled_transparent_clipped(height_or_scale,
				sdgame2shapes[explosion_index], offset_vector.x, offset_vector.y);
		}
	}
}

static void frame_draw_cockpit_effects(struct RECTANGLE* cliprect)
{
	legacy_s16 crash_frame;
	struct CARSTATE* viewed_carstate;

	// Depict windscreen cracking after a crash
	sprite_set_target_clip_bounds(0, FRAME_SCREEN_WIDTH, cliprect->top,
		cliprect->bottom);
	if (cameramode == CAMERA_MODE_COCKPIT) {

		if (followOpponentFlag != 0) {
			viewed_carstate = &state.opponentstate;
			crash_frame = state.game_oEndFrame;
		} else {
			viewed_carstate = &state.playerstate;
			crash_frame = state.game_pEndFrame;
		}

		if (viewed_carstate->car_crashBmpFlag ==
			CRASH_EVENT_COLLISION) {
			if (slow_video_mgmt_copy != 0) {
				rect_union(init_crak(state.game_frame - crash_frame, cliprect->top,
					cliprect->bottom - cliprect->top), frame_layer_rects, frame_layer_rects);
			} else {
				init_crak(state.game_frame - crash_frame, cliprect->top,
					cliprect->bottom - cliprect->top);
			}
		} else if (viewed_carstate->car_crashBmpFlag ==
			CRASH_EVENT_WATER) {
			if (slow_video_mgmt_copy != 0) {
				rect_union(do_sinking(state.game_frame - crash_frame, cliprect->top,
					cliprect->bottom - cliprect->top), frame_layer_rects, frame_layer_rects);
			} else {
				do_sinking(state.game_frame - crash_frame, cliprect->top,
					cliprect->bottom - cliprect->top);
			}
		}
	}
}

static void frame_draw_elapsed_time(void)
{
	// Show elapsed time
	if (game_replay_mode == REPLAY_MODE_LIVE) {
		if (state.game_inputmode != GAME_INPUT_MODE_WAITING) {
			format_frame_as_string(&resID_byte1, elapsed_time1 + elapsed_time2, 0);
			font_set_fontdef2(fontledresptr);
			if (slow_video_mgmt_copy != 0) {
				rect_union(intro_draw_text(&resID_byte1,
					FRAME_ELAPSED_TIME_X,
					roofbmpheight + FRAME_ELAPSED_TIME_Y_OFFSET,
					dialog_fnt_colour, 0), &frame_elapsed_time_rect, &frame_elapsed_time_rect);
			} else {
				intro_draw_text(&resID_byte1, FRAME_ELAPSED_TIME_X,
					roofbmpheight + FRAME_ELAPSED_TIME_Y_OFFSET,
					dialog_fnt_colour, 0);
			}

			font_set_fontdef();
		}
	}
}

static void frame_finish(legacy_s8 buffer_index, struct RECTANGLE* cliprect,
	legacy_s16 skybox_requires_full_redraw, legacy_s16 camera_yaw)
{
	legacy_s16 rect_index;

	if (slow_video_mgmt_copy != 0) {
		rect_union(draw_ingame_text(), frame_layer_rects, frame_layer_rects);
		if (skybox_requires_full_redraw != 0) {
			frame_layer_rects[0] = *cliprect;
			for (rect_index = 1; rect_index < FRAME_DIRTY_RECT_COUNT; rect_index++) {
				frame_layer_rects[rect_index] = empty_rect;
			}
		}

		for (rect_index = 0; rect_index < FRAME_DIRTY_RECT_COUNT; rect_index++) {
			active_frame_rects[rect_index] = frame_layer_rects[rect_index];
		}
		frame_buffer_camera_headings[buffer_index] = camera_yaw;
		last_rendered_camera_heading = camera_yaw;

	} else {
		draw_ingame_text();
	}
}

void update_frame(legacy_s8 buffer_index, struct RECTANGLE* cliprect) {
	struct FRAME_CAMERA camera;
	struct FRAME_TILE_SELECTION tiles;
	struct FRAME_CAR_RENDER cars[FRAME_EXPLOSION_CAR_COUNT];
	legacy_s8 redraw_transform_flags;
	legacy_s8 animated_material;
	legacy_s16 skybox_requires_full_redraw;

	cars[PLAYER_CAR_INDEX].explosion_visible = 0;
	cars[OPPONENT_CAR_INDEX].explosion_visible = 0;
	redraw_transform_flags = frame_begin(buffer_index);
	frame_setup_camera(&camera);
	animated_material = frame_animated_material();
	tiles.lookahead = frame_setup_projection(&camera, cliprect);
	frame_draw_clouds(&camera, redraw_transform_flags);
	frame_select_tiles(&tiles, &camera);
	frame_place_cars(&tiles, cars);
	frame_draw_tiles(&tiles, &camera, cars, redraw_transform_flags,
		animated_material);

	skybox_requires_full_redraw = skybox_render(buffer_index, cliprect,
		camera.skybox_parameter, &camera.pitch_roll_rotation, camera.roll,
		camera.yaw, camera.position.y);
	sprite_set_target_clip_bounds(0, FRAME_SCREEN_WIDTH, cliprect->top,
		cliprect->bottom);
	shape3d_render_queued_primitives();
	frame_draw_explosions(cars, cliprect);
	frame_draw_cockpit_effects(cliprect);
	frame_draw_elapsed_time();
	frame_finish(buffer_index, cliprect, skybox_requires_full_redraw,
		camera.yaw);
}
