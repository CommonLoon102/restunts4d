#include "frame_internal.h"
#include "game_input.h"
#include "trackdata_layout.h"

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

void update_frame(legacy_s8 buffer_index, struct RECTANGLE* cliprect) {
	legacy_s16 tile_angle_or_frame_index;
	legacy_s8 redraw_transform_flags;
	legacy_s8 animated_material;
	legacy_s8 visible_car_explosions[FRAME_EXPLOSION_CAR_COUNT];
	struct RECTANGLE* redraw_rect;
	struct MATRIX camera_pitch_roll_rotation, cloud_heading_rotation;
	struct MATRIX* car_rot_matrix;
	struct VECTOR cam_pos, car_pos, offset_vector, car_to_cam_rotated, shape_relative_position;
	legacy_s16 car_rot_y, car_rot_x, car_rot_z;
	legacy_s16 camera_pitch, camera_yaw, camera_roll;
	legacy_s16 camera_horizontal_distance, effective_camera_roll;
	legacy_s16 transform_result;
	legacy_s16 heading;
	const struct FRAME_LOOKAHEAD_TILE* lookahead_tiles;
	legacy_s16 skybox_parameter;
	legacy_s16 shape_count_or_extent;
	legacy_s8 cam_tile_south, cam_tile_east;
	legacy_s8 tile_south, tile_east;
	legacy_s8 tile_to_draw_south_offset, tile_to_draw_east_offset;
	legacy_s8 car_tile_east, car_tile_south;
	legacy_u8 tiles_to_draw_terr_type_vec[FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT];
	legacy_s8 should_skip_tile[FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT];
	legacy_s8 tile_detail_level[FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT];
	legacy_s8 tiles_to_draw_south[FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT];
	legacy_s8 tiles_to_draw_east[FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT];
	legacy_u8 tiles_to_draw_elem_type_vec[FRAME_LOOKAHEAD_TABLE_ENTRY_COUNT];
	legacy_s8 detail_threshold;
	legacy_s8 player_tile_east;
	legacy_s8 player_tile_south;
	legacy_s8 opponent_tile_south;
	legacy_s8 opponent_tile_east;
	legacy_s8 overlay_needs_depth_sort;
	legacy_s16 player_depth_adjustment;
	legacy_s16 opponent_depth_adjustment;
	legacy_s16 hill_height;
	legacy_s16 idx;
	struct TRACKOBJECT* overlay_track_object;
	struct TRACKOBJECT* track_object; // NOTE: beware of similar names!!
	legacy_s8 tile_det_level;
	legacy_s8* fence_tile_offsets;
	legacy_s16 shape_or_tile_index;
	legacy_u16 vertex_index;
	legacy_s16 skybox_requires_full_redraw;
	legacy_s16 track_object_world_z;
	legacy_s16 track_object_world_x;
	legacy_s16* hill_fill_offsets;
	legacy_s16 depth_adjustment_mask;
	legacy_u8 breakable_object_index;
	struct RECTANGLE player_crash_rect, opponent_crash_rect;
	struct VECTOR start_flag_vertices[FRAME_START_FLAG_VERTEX_COUNT];
	struct CARSTATE* viewed_carstate;
	legacy_u8 elem_map_value;
	legacy_u8 terr_map_value;

	visible_car_explosions[PLAYER_CAR_INDEX] = 0;
	visible_car_explosions[OPPONENT_CAR_INDEX] = 0;
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
		for (tile_angle_or_frame_index = 0; tile_angle_or_frame_index < FRAME_DIRTY_RECT_COUNT; tile_angle_or_frame_index++) {
			*redraw_rect = empty_rect;
			redraw_rect++;
		}
	} else {
		redraw_transform_flags = 0;
	}

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

	camera_yaw = -1;
	camera_roll = 0;

	// Set camera position, based on the car position and the camera mode
	if (cameramode == CAMERA_MODE_COCKPIT) {
		camera_yaw = car_rot_x & ANGLE_MASK;
		camera_pitch = car_rot_y & ANGLE_MASK;
		camera_roll = car_rot_z & ANGLE_MASK;
		car_rot_matrix = frame_car_rotation(car_rot_x, car_rot_y,
			car_rot_z);
		offset_vector.x = 0;
		offset_vector.z = 0;
		offset_vector.y = LEGACY_S16_WRAP_SUB(simd_player.car_height,
			FRAME_COCKPIT_HEIGHT_CLEARANCE);

		mat_mul_vector(&offset_vector, car_rot_matrix, &car_to_cam_rotated);
		cam_pos.x = LEGACY_S16_WRAP_ADD(
			car_pos.x, car_to_cam_rotated.x);
		cam_pos.y = LEGACY_S16_WRAP_ADD(
			car_pos.y, car_to_cam_rotated.y);
		cam_pos.z = LEGACY_S16_WRAP_ADD(
			car_pos.z, car_to_cam_rotated.z);
	} else if (cameramode == CAMERA_MODE_FOLLOW) {
		cam_pos.x = state.game_follow_camera_position[followOpponentFlag].x;
		cam_pos.z = state.game_follow_camera_position[followOpponentFlag].z;
		cam_pos.y = state.game_follow_camera_position[followOpponentFlag].y;
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
		cam_pos.x = LEGACY_S16_WRAP_ADD(car_pos.x, car_to_cam_rotated.x);
		cam_pos.y = LEGACY_S16_WRAP_ADD(car_pos.y, car_to_cam_rotated.y);
		cam_pos.z = LEGACY_S16_WRAP_ADD(car_pos.z, car_to_cam_rotated.z);
	} else if (cameramode == CAMERA_MODE_TRACKSIDE) {
		cam_pos.x = trackside_camera_positions[state.game_trackside_camera_index[followOpponentFlag]].x;
		cam_pos.y = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
			trackside_camera_positions[state.game_trackside_camera_index[followOpponentFlag]].y,
			camera_track_height_offset), FRAME_TRACK_CAMERA_HEIGHT_OFFSET);
		cam_pos.z = trackside_camera_positions[state.game_trackside_camera_index[followOpponentFlag]].z;
	}

	// Keep external cameras above the track and aim them at the followed car.
	if (camera_yaw == -1) {
		build_track_object(&cam_pos, &cam_pos);
		if (cam_pos.y < terrainHeight) {
			cam_pos.y = terrainHeight;
		}

		if (track_wall_collision_enabled != 0) {
			tile_angle_or_frame_index = plane_signed_distance(planindex, cam_pos.x, cam_pos.y, cam_pos.z);
			if (tile_angle_or_frame_index < FRAME_PLANE_CLEARANCE) {
				wheel_forward_travel.x = 0;
				wheel_forward_travel.y = LEGACY_S16_WRAP_SUB(FRAME_PLANE_CLEARANCE,
					tile_angle_or_frame_index);
				wheel_forward_travel.z = 0;
				planindex_copy = planindex;
				wheel_heading_offset = 0;
				car_initial_pitch = 0;
				car_initial_roll = 0;
				car_initial_yaw = 0;
				transform_wheel_travel_to_world();
				cam_pos.x = LEGACY_S16_WRAP_ADD(
					cam_pos.x, wheel_world_travel.x);
				cam_pos.y = LEGACY_S16_WRAP_ADD(
					cam_pos.y, wheel_world_travel.y);
				cam_pos.z = LEGACY_S16_WRAP_ADD(
					cam_pos.z, wheel_world_travel.z);
			}
		}

		camera_yaw = LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_S16_WRAP_NEGATE(polarAngle(
				LEGACY_S16_WRAP_SUB(car_pos.x, cam_pos.x),
				LEGACY_S16_WRAP_SUB(car_pos.z, cam_pos.z))) & ANGLE_MASK);
		camera_horizontal_distance = polarRadius2D(
			LEGACY_S16_WRAP_SUB(car_pos.x, cam_pos.x),
			LEGACY_S16_WRAP_SUB(car_pos.z, cam_pos.z));
		camera_pitch = LEGACY_S16_FROM_BITS((legacy_u16)polarAngle(
			LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_SUB(car_pos.y, cam_pos.y),
				FRAME_CAMERA_TARGET_HEIGHT_OFFSET),
			camera_horizontal_distance) & ANGLE_MASK);
	}

	if (camera_roll > 1 && camera_roll < ANGLE_MASK) {
		effective_camera_roll = camera_roll;
	} else {
		effective_camera_roll = 0;
	}

	if (state.game_frame == 0) {
		animated_material = track_material_animation[
			frame_callback_count & FRAME_ANIMATION_PHASE_MASK];
	} else {
		animated_material = track_material_animation[
			state.game_frame & FRAME_ANIMATION_PHASE_MASK];
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

	heading = select_cliprect_rotate(effective_camera_roll, camera_pitch, camera_yaw, cliprect, 0);
	lookahead_tiles = (const struct FRAME_LOOKAHEAD_TILE*)
		lookahead_tiles_tables[(heading & ANGLE_MASK) >>
			FRAME_LOOKAHEAD_HEADING_SHIFT];

	camera_pitch_roll_rotation = *mat_rot_zxy(effective_camera_roll, camera_pitch, 0,
		MATRIX_ROTATION_ORDER_YXZ);
	offset_vector.x = 0;
	offset_vector.y = 0;
	offset_vector.z = FRAME_SKYBOX_TEST_DISTANCE;
	mat_mul_vector(&offset_vector, &camera_pitch_roll_rotation, &shape_relative_position);
	if (shape_relative_position.z > 0) {
		skybox_parameter = 1;
	} else {
		skybox_parameter = -1;
	}

	// Draw the eight cloud shapes at full detail.
	if (detail_level == FRAME_DETAIL_FULL) {
		currenttransshape->rectptr = &frame_cloud_rect;
		currenttransshape->ts_flags = redraw_transform_flags | FRAME_DISTANT_SHAPE_FLAGS;
		currenttransshape->rotvec.x = 0;
		currenttransshape->rotvec.y = 0;
		currenttransshape->culling_distance = FRAME_DEFAULT_TRANSFORM_DISTANCE;
		currenttransshape->material = 0;

		for (shape_count_or_extent = 0; shape_count_or_extent < FRAME_DISTANT_SHAPE_COUNT;
			shape_count_or_extent++) {
			tile_angle_or_frame_index = LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(
					cloud_heading_offsets[shape_count_or_extent], camera_yaw),
				run_game_random) & ANGLE_MASK);
			if (tile_angle_or_frame_index < FRAME_DISTANT_SHAPE_MIN_ANGLE ||
				tile_angle_or_frame_index > FRAME_DISTANT_SHAPE_MAX_ANGLE) {
				mat_rot_y(&cloud_heading_rotation, tile_angle_or_frame_index);
				offset_vector.x = 0;
				offset_vector.y = LEGACY_S16_WRAP_SUB(
					FRAME_DISTANT_SHAPE_HEIGHT, cam_pos.y);
				offset_vector.z = FRAME_DISTANT_SHAPE_DISTANCE;
				mat_mul_vector(&offset_vector, &cloud_heading_rotation, &car_to_cam_rotated);
				car_to_cam_rotated.z = FRAME_DISTANT_SHAPE_DISTANCE;
				mat_mul_vector(&car_to_cam_rotated, &camera_pitch_roll_rotation, &currenttransshape->pos);
				if (currenttransshape->pos.z >
					FRAME_DISTANT_SHAPE_MIN_DEPTH) {
					currenttransshape->shapeptr = cloud_shapes[shape_count_or_extent];
					currenttransshape->rotvec.z =
						LEGACY_S16_WRAP_NEGATE(camera_yaw);
					transform_result = shape3d_transform_and_queue(&currenttransshape[0]);
					(void) transform_result; // we cannot be out of memory as we are just starting to process
				}
			}
		}
	}

/*
; -----------------------------------------------------------------------------------------------
*/

	cam_tile_east = LEGACY_S8_FROM_BITS(
		(legacy_u8)LEGACY_S16_SAR(cam_pos.x, FRAME_CAMERA_TILE_SHIFT));
	cam_tile_south = LEGACY_S8_WRAP_SUB(TRACK_GRID_LAST_COORDINATE,
		LEGACY_S16_SAR(cam_pos.z, FRAME_CAMERA_TILE_SHIFT));
	if (detail_level != FRAME_DETAIL_FULL) {
		car_tile_east = frame_tile_from_world(
			state.playerstate.car_position.lx);
		car_tile_south = frame_south_tile_from_world(
			state.playerstate.car_position.lz);
	}

	for (tile_angle_or_frame_index = 0; tile_angle_or_frame_index < FRAME_LOOKAHEAD_TILE_COUNT; tile_angle_or_frame_index++) {
		should_skip_tile[tile_angle_or_frame_index] = FRAME_TILE_DRAW_MARKER;
	}

	// Select the detail level (FULL if 1st or 2nd option in the graphics menu
	// were chosen, MEDIUM if the 3rd, FASTEST if 4th or 5th)
	detail_threshold = detail_threshold_by_level[detail_level];

	// Cycle on the 23 tiles to draw, determine if they really need to be drawn
	for (tile_angle_or_frame_index = FRAME_LOOKAHEAD_LAST_TILE_INDEX; tile_angle_or_frame_index >= 0; tile_angle_or_frame_index--) {

		// Skip if a previous iteration determined this tile is not needed
		// (happens for multi-tile elements)
		if (should_skip_tile[tile_angle_or_frame_index] != FRAME_TILE_DRAW_MARKER)
			continue;

		// Skip if detail threshold not met (e.g. far tiles in FASTEST detail)
		if (lookahead_tiles[tile_angle_or_frame_index].detail <= detail_threshold) {
			tile_east = LEGACY_S8_WRAP_ADD(
				lookahead_tiles[tile_angle_or_frame_index].east, cam_tile_east);
			tile_south = LEGACY_S8_WRAP_ADD(
				lookahead_tiles[tile_angle_or_frame_index].south, cam_tile_south);

			// Skip if tile is out of bounds
			if (tile_east >= 0 &&
				tile_east <= TRACK_GRID_LAST_COORDINATE &&
				tile_south >= 0 &&
				tile_south <= TRACK_GRID_LAST_COORDINATE) {
				elem_map_value = td14_elem_map_main[tile_east + trackrows[tile_south]];
				terr_map_value = td15_terr_map_main[tile_east + terrainrows[tile_south]];

				if (elem_map_value != 0) {

					if (terr_map_value >= FRAME_HILL_ROAD_TERRAIN_FIRST &&
						terr_map_value < FRAME_HILL_ROAD_TERRAIN_END) {
						elem_map_value = subst_hillroad_track(terr_map_value, elem_map_value);
						terr_map_value = 0;
					}

					// Found a filler tile (non-main tile of a multitile component)
					// Process the main tile of the component instead (the NW one)
					if (elem_map_value ==
						TRACK_TILE_CONTINUATION_SOUTHEAST) {
						tile_east = LEGACY_S8_WRAP_SUB(tile_east, 1);
						tile_south = LEGACY_S8_WRAP_SUB(tile_south, 1);
						elem_map_value = td14_elem_map_main[tile_east + trackrows[tile_south]];
						terr_map_value = td15_terr_map_main[tile_east + terrainrows[tile_south]];
					} else if (elem_map_value ==
						TRACK_TILE_CONTINUATION_SOUTH) {
						tile_south = LEGACY_S8_WRAP_SUB(tile_south, 1);
						elem_map_value = td14_elem_map_main[tile_east + trackrows[tile_south]];
						terr_map_value = td15_terr_map_main[tile_east + terrainrows[tile_south]];
					} else if (elem_map_value ==
						TRACK_TILE_CONTINUATION_EAST) {
						tile_east = LEGACY_S8_WRAP_SUB(tile_east, 1);
						elem_map_value = td14_elem_map_main[tile_east + trackrows[tile_south]];
						terr_map_value = td15_terr_map_main[tile_east + terrainrows[tile_south]];
					}
				}

				tiles_to_draw_terr_type_vec[tile_angle_or_frame_index] = terr_map_value;
				tile_detail_level[tile_angle_or_frame_index] = lookahead_tiles[tile_angle_or_frame_index].detail;

				if (elem_map_value != 0 &&
					detail_level != FRAME_DETAIL_FULL &&
					trkObjectList[elem_map_value].ss_physicalModel >=
						FRAME_SCENERY_PHYSICAL_MODEL_FIRST &&
					(tile_east != car_tile_east || tile_south != car_tile_south))
				{
					elem_map_value = 0;
				}

				tiles_to_draw_east[tile_angle_or_frame_index] = tile_east;
				tiles_to_draw_south[tile_angle_or_frame_index] = tile_south;
				tiles_to_draw_elem_type_vec[tile_angle_or_frame_index] = elem_map_value;

				if (elem_map_value != 0) {
					idx = trkObjectList[elem_map_value].ss_multiTileFlag;
					if (idx != FRAME_MULTITILE_NONE) {
						// Look the future tiles to process (i.e. with lower index, since tile_angle_or_frame_index
						// counts backwards) and remove those which belong to the same
						// multi-tile component as this tile

						// Recalculate the offset (needed in case we hit a filler tile)
						tile_to_draw_east_offset = LEGACY_S8_WRAP_SUB(
							tile_east, cam_tile_east);
						tile_to_draw_south_offset = LEGACY_S8_WRAP_SUB(
							tile_south, cam_tile_south);
						if (idx == FRAME_MULTITILE_ROW) {
							for (shape_or_tile_index = 0; shape_or_tile_index < tile_angle_or_frame_index; shape_or_tile_index++) {
								if (lookahead_tiles[shape_or_tile_index].east == tile_to_draw_east_offset && (lookahead_tiles[shape_or_tile_index].south == tile_to_draw_south_offset || lookahead_tiles[shape_or_tile_index].south == tile_to_draw_south_offset + 1)) {
									should_skip_tile[shape_or_tile_index] =
										FRAME_TILE_MULTITILE_COVERED_MARKER;
								}
							}
						} else if (idx == FRAME_MULTITILE_COLUMN) {
							for (shape_or_tile_index = 0; shape_or_tile_index < tile_angle_or_frame_index; shape_or_tile_index++) {
								if (lookahead_tiles[shape_or_tile_index].south == tile_to_draw_south_offset && (lookahead_tiles[shape_or_tile_index].east == tile_to_draw_east_offset || lookahead_tiles[shape_or_tile_index].east == tile_to_draw_east_offset + 1)) {
									should_skip_tile[shape_or_tile_index] =
										FRAME_TILE_MULTITILE_COVERED_MARKER;
								}
							}
						} else if (idx == FRAME_MULTITILE_BOTH) {
							for (shape_or_tile_index = 0; shape_or_tile_index < tile_angle_or_frame_index; shape_or_tile_index++) {
								if ((lookahead_tiles[shape_or_tile_index].east == tile_to_draw_east_offset || lookahead_tiles[shape_or_tile_index].east == tile_to_draw_east_offset + 1) &&
									(lookahead_tiles[shape_or_tile_index].south == tile_to_draw_south_offset || lookahead_tiles[shape_or_tile_index].south == tile_to_draw_south_offset + 1))
								{
									should_skip_tile[shape_or_tile_index] =
										FRAME_TILE_MULTITILE_COVERED_MARKER;
								}
							}
						}
					}
				}

			} else {
				should_skip_tile[tile_angle_or_frame_index] = FRAME_TILE_UNAVAILABLE_MARKER;
			}
		} else {
			should_skip_tile[tile_angle_or_frame_index] = FRAME_TILE_UNAVAILABLE_MARKER;
		}
	}

//; -----------------------------------------------------------------------------

	// Draw own wheels
	player_tile_east = -1;
	player_depth_adjustment = 0;
	if (cameramode != CAMERA_MODE_COCKPIT ||
		followOpponentFlag != 0) {

		if (state.playerstate.car_crashBmpFlag !=
			CRASH_EVENT_WATER) {

			player_depth_adjustment = frame_find_car_wheel(&state.playerstate,
				&simd_player, should_skip_tile, lookahead_tiles,
				cam_tile_east, cam_tile_south, &player_tile_east, &player_tile_south);
		}
	}

	// Draw opponent's wheels
	opponent_tile_east = -1;
	opponent_depth_adjustment = 0;
	if (gameconfig.game_opponenttype != 0) {

		if (cameramode != CAMERA_MODE_COCKPIT ||
			followOpponentFlag == 0) {
			if (state.opponentstate.car_crashBmpFlag !=
				CRASH_EVENT_WATER) {
				opponent_depth_adjustment = frame_find_car_wheel(&state.opponentstate,
					&simd_opponent, should_skip_tile, lookahead_tiles,
					cam_tile_east, cam_tile_south, &opponent_tile_east, &opponent_tile_south);
			}
		}
	}
//; -----------------------------------------------------------------------------


	overlay_needs_depth_sort = 0;
	tile_angle_or_frame_index = 0;

	// With the information collected by the previus tile-scan algorithm,
	// proceed to draw the shapes in each tile. Start from the farthest
	// (painter's algorithm)
	for (tile_angle_or_frame_index = 0; tile_angle_or_frame_index < FRAME_LOOKAHEAD_TILE_COUNT; tile_angle_or_frame_index++) {
		if (should_skip_tile[tile_angle_or_frame_index] != FRAME_TILE_DRAW_MARKER) {
			continue;
		}
		tile_east = tiles_to_draw_east[tile_angle_or_frame_index];
		tile_south = tiles_to_draw_south[tile_angle_or_frame_index];
		elem_map_value = tiles_to_draw_elem_type_vec[tile_angle_or_frame_index];
		terr_map_value = tiles_to_draw_terr_type_vec[tile_angle_or_frame_index];
		tile_det_level = tile_detail_level[tile_angle_or_frame_index];
		depth_adjustment_mask = 0;
		if (elem_map_value == 0) {
			shape_count_or_extent = 1;
			fence_tile_offsets = fence_tile_offsets_column;
		} else {
			track_object = &trkObjectList[elem_map_value];
			if (track_object->ss_multiTileFlag ==
				FRAME_MULTITILE_NONE) {
				shape_count_or_extent = FRAME_FENCE_POSITION_COUNT_SINGLE;
				fence_tile_offsets = fence_tile_offsets_single;
			} else if (track_object->ss_multiTileFlag ==
				FRAME_MULTITILE_ROW) {
				shape_count_or_extent = FRAME_FENCE_POSITION_COUNT_ROW;
				fence_tile_offsets = fence_tile_offsets_row;
			} else if (track_object->ss_multiTileFlag ==
				FRAME_MULTITILE_COLUMN) {
				shape_count_or_extent = FRAME_FENCE_POSITION_COUNT_COLUMN;
				fence_tile_offsets = fence_tile_offsets_column;
			} else if (track_object->ss_multiTileFlag ==
				FRAME_MULTITILE_BOTH) {
				shape_count_or_extent = FRAME_FENCE_POSITION_COUNT_BOTH;
				fence_tile_offsets = fence_tile_offsets_both;
			}
		}

		// Draw the fence
		for (idx = 0; idx < shape_count_or_extent; idx++) {
			tile_to_draw_east_offset = LEGACY_S8_WRAP_ADD(
				fence_tile_offsets[idx * FRAME_FENCE_POSITION_STRIDE], tile_east);
			tile_to_draw_south_offset = LEGACY_S8_WRAP_ADD(
				fence_tile_offsets[idx * FRAME_FENCE_POSITION_STRIDE +
					FRAME_FENCE_SECOND_COORDINATE], tile_south);

			if (detail_level == FRAME_DETAIL_FULL ||
				(tile_to_draw_east_offset == car_tile_east &&
					tile_to_draw_south_offset == car_tile_south)) {
				shape_or_tile_index = fence_by_edge[frame_border_index(
					tile_to_draw_east_offset)][frame_border_index(
					tile_to_draw_south_offset)];

				if (shape_or_tile_index != FRAME_FENCE_NONE) {
					overlay_track_object = frame_track_object_from_legacy_index(
						fence_TrkObjCodes[shape_or_tile_index]);
					if (tile_det_level == FRAME_TILE_DETAIL_FULL) {
						currenttransshape->shapeptr = overlay_track_object->ss_shapePtr;
					} else {
						currenttransshape->shapeptr = overlay_track_object->ss_loShapePtr;
					}

					frame_prepare_flat_track_shape(currenttransshape,
						tile_to_draw_east_offset,
						tile_to_draw_south_offset,
						&cam_pos, (legacy_s16)(redraw_transform_flags |
							FRAME_TRANSFORM_FLAGS_NO_DEPTH_SORT),
						fence_rotations[shape_or_tile_index]);
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

		// Elevated terrain is a flat piece of land at an elevated level.
		if (terr_map_value != TERRAIN_RAISED_TILE) {
			hill_height = 0;

			// Special treatment of elevated corners
			if (elem_map_value >= FRAME_ELEVATED_CORNER_FIRST &&
				elem_map_value <= FRAME_ELEVATED_CORNER_LAST) {
				for (idx = 0; idx < FRAME_ELEVATED_CORNER_COUNT; idx++) {
					if (idx == FRAME_CORNER_NORTHWEST) {
						tile_to_draw_east_offset = tile_east;
						tile_to_draw_south_offset = tile_south;
					} else if (idx == FRAME_CORNER_NORTHEAST) {
						tile_to_draw_east_offset = LEGACY_S8_WRAP_ADD(
							tile_east, 1);
						tile_to_draw_south_offset = tile_south;
					} else if (idx == FRAME_CORNER_SOUTHWEST) {
						tile_to_draw_east_offset = tile_east;
						tile_to_draw_south_offset = LEGACY_S8_WRAP_ADD(
							tile_south, 1);
					} else if (idx == FRAME_CORNER_SOUTHEAST) {
						tile_to_draw_east_offset = LEGACY_S8_WRAP_ADD(
							tile_east, 1);
						tile_to_draw_south_offset = LEGACY_S8_WRAP_ADD(
							tile_south, 1);
					}
					terr_map_value = td15_terr_map_main[tile_to_draw_east_offset + terrainrows[tile_to_draw_south_offset]];
					if (terr_map_value != 0) {
						track_object = &terrain_scene_objects[terr_map_value];
						currenttransshape->shapeptr = track_object->ss_shapePtr;
						frame_prepare_flat_track_shape(currenttransshape,
							tile_to_draw_east_offset,
							tile_to_draw_south_offset,
							&cam_pos, (legacy_s16)(redraw_transform_flags |
								FRAME_TRANSFORM_FLAGS_NO_DEPTH_SORT),
							track_object->ss_rotY);
						transform_result = shape3d_transform_and_queue(&currenttransshape[0]);
						if (transform_result > 0)
							break;
					}
				}

				terr_map_value = 0;
			}
		} else {
			hill_height =
				hillHeightConsts[TERRAIN_RAISED_HEIGHT_INDEX];
			if (elem_map_value != 0) {
				terr_map_value = 0;
			}
		}

		// The rest of the rendering loop still needs to be analyzed in detail.
		// Anyway, the gist is that every tile is associated with various shape,
		// each of which is rendered via a call to `shape3d_transform_and_queue`. The
		// result of such fn is checked each time, since a return value of 1
		// means we ran out of memory

		if (terr_map_value != 0) {
			track_object = &terrain_scene_objects[terr_map_value];
			currenttransshape->shapeptr = track_object->ss_shapePtr;
			currenttransshape->pos.x = LEGACY_S16_WRAP_SUB(
				track_column_centers[tile_east], cam_pos.x);
			currenttransshape->pos.y = LEGACY_S16_WRAP_SUB(
				hill_height, cam_pos.y);
			currenttransshape->pos.z = LEGACY_S16_WRAP_SUB(
				track_row_centers[tile_south], cam_pos.z);
			if (hill_height == 0) {
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
				break;
		}

		transformedshape_counter = 0;
		curtransshape_ptr = currenttransshape;
		if (elem_map_value == 0) {
			tile_to_draw_east_offset = tile_east;
			tile_to_draw_south_offset = tile_south;
		} else {
			track_object = &trkObjectList[elem_map_value];
			if ((track_object->ss_multiTileFlag & FRAME_MULTITILE_ROW) !=
				0) {
				track_object_world_z = track_row_positions[tile_south];
				tile_to_draw_south_offset = LEGACY_S8_WRAP_ADD(
					tile_south, 1);
			} else {
				track_object_world_z = track_row_centers[tile_south];
				tile_to_draw_south_offset = tile_south;
			}

			if ((track_object->ss_multiTileFlag &
				FRAME_MULTITILE_COLUMN) != 0) {
				track_object_world_x = track_column_positions[LEGACY_S8_WRAP_ADD(tile_east, 1)];
				tile_to_draw_east_offset = LEGACY_S8_WRAP_ADD(
					tile_east, 1);
			} else {
				track_object_world_x = track_column_centers[tile_east];
				tile_to_draw_east_offset = tile_east;
			}

			shape_relative_position.x = LEGACY_S16_WRAP_SUB(track_object_world_x, cam_pos.x);
			shape_relative_position.y = LEGACY_S16_WRAP_SUB(hill_height, cam_pos.y);
			shape_relative_position.z = LEGACY_S16_WRAP_SUB(track_object_world_z, cam_pos.z);
			if (hill_height != 0) {
				if (track_object->ss_multiTileFlag ==
					FRAME_MULTITILE_NONE) {
					shape_or_tile_index = FRAME_HILL_FILL_COUNT_SINGLE;
					hill_fill_offsets = hill_fill_offsets_single;
				} else if (track_object->ss_multiTileFlag ==
					FRAME_MULTITILE_ROW) {
					shape_or_tile_index = FRAME_HILL_FILL_COUNT_ROW;
					hill_fill_offsets = hill_fill_offsets_row;
				} else if (track_object->ss_multiTileFlag ==
					FRAME_MULTITILE_COLUMN) {
					shape_or_tile_index = FRAME_HILL_FILL_COUNT_COLUMN;
					hill_fill_offsets = hill_fill_offsets_column;
				} else if (track_object->ss_multiTileFlag ==
					FRAME_MULTITILE_BOTH) {
					shape_or_tile_index = FRAME_HILL_FILL_COUNT_BOTH;
					hill_fill_offsets = hill_fill_offsets_both;
				}

				for (idx = 0; idx < shape_or_tile_index; idx++) {
					currenttransshape->pos.x = LEGACY_S16_WRAP_ADD(
						*hill_fill_offsets, shape_relative_position.x);
					hill_fill_offsets++;
					currenttransshape->pos.y = shape_relative_position.y;
					currenttransshape->pos.z = LEGACY_S16_WRAP_ADD(
						*hill_fill_offsets, shape_relative_position.z);
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

			if (track_object->ss_ssOvelay != 0) {
				overlay_track_object = frame_track_object_from_legacy_index(
					track_object->ss_ssOvelay);
				if (tile_det_level != FRAME_TILE_DETAIL_FULL) {
					currenttransshape[1].shapeptr = overlay_track_object->ss_loShapePtr;
				} else {
					currenttransshape[1].shapeptr = overlay_track_object->ss_shapePtr;
				}

				if (currenttransshape[1].shapeptr != 0) {
					currenttransshape[1].pos = shape_relative_position;
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
							break;
					} else {
						currenttransshape[1].rectptr = &frame_sorted_shapes_rect;
						overlay_needs_depth_sort = 1;
					}
				}
			}

			if (tile_det_level != FRAME_TILE_DETAIL_FULL) {
				currenttransshape->shapeptr = track_object->ss_loShapePtr;
			} else {
				currenttransshape->shapeptr = track_object->ss_shapePtr;
			}

			currenttransshape->pos = shape_relative_position; // whatever
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
					break;
			} else {
				currenttransshape->rectptr = &frame_sorted_shapes_rect;
				transformed_shape_add_for_sort(0, 0);
				if (overlay_needs_depth_sort != 0) {
					overlay_needs_depth_sort = 0;
					transformed_shape_add_for_sort(
						-FRAME_SINGLE_TILE_TRANSFORM_DISTANCE, 0);
					if (player_depth_adjustment != 0) {
						player_depth_adjustment = -FRAME_WHEEL_SORT_ADJUSTMENT;
					}

					if (opponent_depth_adjustment != 0) {
						opponent_depth_adjustment = LEGACY_S16_WRAP_SUB(opponent_depth_adjustment,
							FRAME_WHEEL_SORT_ADJUSTMENT);
					}
				}

				if (tile_east == start_finish_column && tile_south == start_finish_row) {
					depth_adjustment_mask = 0;
				} else {
					depth_adjustment_mask = -1;
				}
			}

			breakable_object_index = roadside_sign_indices_by_tile[tile_east + trackrows[tile_south]];
			if (breakable_object_index != FRAME_CHECKPOINT_NONE) {
				if (state.game_object_destroyed[breakable_object_index] == 0) {
					track_object = &trkObjectList[
						FRAME_CHECKPOINT_TRACK_OBJECT_BASE +
						roadside_sign_shape_indices[breakable_object_index]];
					curtransshape_ptr->pos.x = LEGACY_S16_WRAP_SUB(
						roadside_sign_positions[breakable_object_index].x, cam_pos.x);
					curtransshape_ptr->pos.y = LEGACY_S16_WRAP_SUB(
						roadside_sign_positions[breakable_object_index].y, cam_pos.y);
					curtransshape_ptr->pos.z = LEGACY_S16_WRAP_SUB(
						roadside_sign_positions[breakable_object_index].z, cam_pos.z);
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
					for (shape_or_tile_index = 0; shape_or_tile_index < FRAME_DEBRIS_SLOT_COUNT; shape_or_tile_index++) {
						if (state.game_particle_forward_speed[shape_or_tile_index] != 0 &&
							breakable_object_index + FRAME_CHECKPOINT_OWNER_OFFSET ==
								state.game_particle_owner[shape_or_tile_index]) {
							track_object = &particle_scene_objects[state.game_particle_shape_index[shape_or_tile_index]];
							curtransshape_ptr->pos.x = frame_relative_track_position(
								state.game_particle_x[shape_or_tile_index],
								roadside_sign_positions[breakable_object_index].x, cam_pos.x);
							curtransshape_ptr->pos.y = frame_relative_track_position(
								state.game_particle_y[shape_or_tile_index],
								roadside_sign_positions[breakable_object_index].y, cam_pos.y);
							curtransshape_ptr->pos.z = frame_relative_track_position(
								state.game_particle_z[shape_or_tile_index],
								roadside_sign_positions[breakable_object_index].z, cam_pos.z);
							frame_add_dynamic_shape(track_object, shape_or_tile_index,
								redraw_transform_flags |
									FRAME_TRANSFORM_FLAGS_NO_DEPTH_SORT,
								0, 0);
						}
					}
				}
			}
		}

		if ((player_tile_east == tile_east || player_tile_east == tile_to_draw_east_offset) && (player_tile_south == tile_south || player_tile_south == tile_to_draw_south_offset)) {
			frame_add_car(&state.playerstate, PLAYER_CAR_INDEX,
				FRAME_PLAYER_SORT_ID,
				&game3dshapes[FRAME_PLAYER_SHAPE_RESOURCE_OFFSET /
					sizeof(struct SHAPE3D)],
				player_wheel_vertex_state, player_base_wheel_vertices, player_front_wheel_centers,
				&frame_player_car_rect, &player_crash_rect, &cam_pos, tile_det_level,
				redraw_transform_flags, gameconfig.game_playermaterial,
				player_depth_adjustment & depth_adjustment_mask);
		}

		if ((opponent_tile_east == tile_east) || (opponent_tile_east == tile_to_draw_east_offset)) {
			if ((opponent_tile_south == tile_south) || (opponent_tile_south == tile_to_draw_south_offset)) {
				frame_add_car(&state.opponentstate, OPPONENT_CAR_INDEX,
					FRAME_OPPONENT_SORT_ID,
					&game3dshapes[FRAME_OPPONENT_SHAPE_RESOURCE_OFFSET /
						sizeof(struct SHAPE3D)],
					opponent_wheel_vertex_state, opponent_base_wheel_vertices, opponent_front_wheel_centers,
					&frame_opponent_car_rect, &opponent_crash_rect, &cam_pos, tile_det_level,
					redraw_transform_flags, gameconfig.game_opponentmaterial,
					opponent_depth_adjustment & depth_adjustment_mask);
			}
		}

		if (state.game_inputmode == GAME_INPUT_MODE_WAITING) {
			if ((tile_east == start_finish_column || tile_to_draw_east_offset == start_finish_column) && (tile_south == start_finish_row || tile_to_draw_south_offset == start_finish_row)) {

				idx = multiply_and_scale(cos_fast(start_flag_animation),
					FRAME_START_FLAG_RADIUS);
				shape_count_or_extent = LEGACY_S16_WRAP_ADD(
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
					LEGACY_S16_WRAP_SUB(idx,
					FRAME_START_FLAG_RADIUS);
				start_flag_vertices[FRAME_START_FLAG_VERTEX_LEFT_FAR].x =
					LEGACY_S16_WRAP_SUB(idx,
					FRAME_START_FLAG_RADIUS);
				start_flag_vertices[FRAME_START_FLAG_VERTEX_RIGHT_NEAR].x =
					LEGACY_S16_WRAP_SUB(
					FRAME_START_FLAG_RADIUS, idx);
				start_flag_vertices[FRAME_START_FLAG_VERTEX_RIGHT_FAR].x =
					LEGACY_S16_WRAP_SUB(
					FRAME_START_FLAG_RADIUS, idx);

				start_flag_vertices[FRAME_START_FLAG_VERTEX_LEFT_NEAR].z = shape_count_or_extent;
				start_flag_vertices[FRAME_START_FLAG_VERTEX_LEFT_FAR].z = shape_count_or_extent;
				start_flag_vertices[FRAME_START_FLAG_VERTEX_RIGHT_NEAR].z = shape_count_or_extent;
				start_flag_vertices[FRAME_START_FLAG_VERTEX_RIGHT_FAR].z = shape_count_or_extent;
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
						track_column_centers[start_finish_column]), cam_pos.x);
				curtransshape_ptr->pos.y = LEGACY_S16_WRAP_SUB(
					hillHeightConsts[hillFlag], cam_pos.y);
				curtransshape_ptr->pos.z = LEGACY_S16_WRAP_SUB(
					LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
						multiply_and_scale(cos_fast(LEGACY_S16_WRAP_ADD(
							track_angle, ANGLE_QUARTER_TURN)),
							FRAME_START_FLAG_RADIUS),
						multiply_and_scale(cos_fast(LEGACY_S16_WRAP_ADD(
							track_angle, ANGLE_HALF_TURN)),
							FRAME_START_FLAG_FAR_OFFSET)),
						track_row_centers[start_finish_row]), cam_pos.z);

				curtransshape_ptr->shapeptr = &game3dshapes[
					FRAME_START_FLAG_RESOURCE_OFFSET / sizeof(struct SHAPE3D)];
				curtransshape_ptr->rectptr = &frame_sorted_shapes_rect;
				curtransshape_ptr->ts_flags = redraw_transform_flags |
					FRAME_TRANSFORM_FLAGS_DEFAULT;
				curtransshape_ptr->rotvec.x = 0;
				curtransshape_ptr->rotvec.y = 0;
				curtransshape_ptr->rotvec.z = track_angle;
				curtransshape_ptr->culling_distance = FRAME_DEFAULT_TRANSFORM_DISTANCE;
				idx = LEGACY_S16_SAR(start_flag_animation,
					FRAME_START_FLAG_ANIMATION_SHIFT);
				if (idx > FRAME_START_FLAG_MAX_MATERIAL) {
					idx = FRAME_START_FLAG_MAX_MATERIAL;
				}

				curtransshape_ptr->material = idx;
				transformed_shape_add_for_sort(depth_adjustment_mask &
					-FRAME_SINGLE_TILE_TRANSFORM_DISTANCE, 0);
			}
		}

		if (transformedshape_counter != 0) {
			if (transformedshape_counter >=
				FRAME_SORT_MINIMUM_SHAPE_COUNT) {
				heapsort_by_order(transformedshape_counter, transformedshape_zarray, transformedshape_indices);
			}

			// Draw red overlights on the brake lights on own and opponent's car
			for (idx = 0; idx < transformedshape_counter; idx++) {
				// shape_or_tile_index is used for index into currenttransshape elsewhere
				shape_or_tile_index = transformedshape_indices[idx];
				if (transformed_shape_sort_types[shape_or_tile_index] == FRAME_PLAYER_SORT_ID) {
					if (state.playerstate.car_is_braking != 0) {
						backlights_paint_override =
							BACKLIGHT_PAINT_BRAKING;
					} else {
						backlights_paint_override =
							BACKLIGHT_PAINT_NORMAL;
					}
				} else if (transformed_shape_sort_types[shape_or_tile_index] ==
					FRAME_OPPONENT_SORT_ID) {
					if (state.opponentstate.car_is_braking == 0) {
						backlights_paint_override =
							BACKLIGHT_PAINT_NORMAL;
					} else {
						backlights_paint_override =
							BACKLIGHT_PAINT_BRAKING;
					}
				}

				transform_result = shape3d_transform_and_queue(&currenttransshape[shape_or_tile_index]); // DI??
				if (transform_result > 0)
					break;

				if (transform_result == 0) {
					if (transformed_shape_sort_types[shape_or_tile_index] ==
						FRAME_PLAYER_SORT_ID) {
						if (state.playerstate.car_crashBmpFlag ==
							CRASH_EVENT_COLLISION) {
							visible_car_explosions[PLAYER_CAR_INDEX] = 1;
						}
					} else if (transformed_shape_sort_types[shape_or_tile_index] ==
						FRAME_OPPONENT_SORT_ID) {
						if (state.opponentstate.car_crashBmpFlag ==
							CRASH_EVENT_COLLISION) {
							visible_car_explosions[OPPONENT_CAR_INDEX] = 1;
						}
					}
				}
			}
		}
	}

	// Draw the skybox
	skybox_requires_full_redraw = skybox_render(buffer_index, cliprect, skybox_parameter, &camera_pitch_roll_rotation, effective_camera_roll, camera_yaw, cam_pos.y);
	sprite_set_target_clip_bounds(0, FRAME_SCREEN_WIDTH, cliprect->top,
		cliprect->bottom);
	shape3d_render_queued_primitives();

	// Draw the three explosion images in successive four-frame phases.
	for (tile_angle_or_frame_index = 0; tile_angle_or_frame_index < FRAME_EXPLOSION_CAR_COUNT; tile_angle_or_frame_index++) {
		if (visible_car_explosions[tile_angle_or_frame_index] == 0) {
			continue;
		}
		if (slow_video_mgmt_copy == 0) {
			if (tile_angle_or_frame_index == PLAYER_CAR_INDEX) {
				redraw_rect = &player_crash_rect;
			} else {
				redraw_rect = &opponent_crash_rect;
			}
		} else {
			if (tile_angle_or_frame_index == PLAYER_CAR_INDEX) {
				redraw_rect = &frame_player_car_rect;
			} else {
				redraw_rect = &frame_opponent_car_rect;
			}
		}

		if (rect_intersect(redraw_rect, cliprect) == 0) {
			sprite_set_target_clip_bounds(redraw_rect->left, redraw_rect->right, redraw_rect->top, redraw_rect->bottom);
			offset_vector.x = LEGACY_S16_SAR(LEGACY_S16_WRAP_ADD(
				redraw_rect->right, redraw_rect->left),
				FRAME_RECT_CENTER_SHIFT);
			offset_vector.y = LEGACY_S16_SAR(LEGACY_S16_WRAP_ADD(
				redraw_rect->top, redraw_rect->bottom),
				FRAME_RECT_CENTER_SHIFT);
			idx = LEGACY_S16_WRAP_SUB(
				redraw_rect->right, redraw_rect->left);
			shape_count_or_extent = LEGACY_S16_WRAP_SUB(
				redraw_rect->bottom, redraw_rect->top);
			if (shape_count_or_extent > idx) {
				idx = shape_count_or_extent;
			}

			shape_or_tile_index = LEGACY_S16_SAR(state.game_frame,
				FRAME_EXPLOSION_FRAME_SHIFT) % FRAME_EXPLOSION_VARIANT_COUNT;
			shape_count_or_extent = LEGACY_S16_FROM_BITS((legacy_u16)
				LEGACY_S32_DIV_OR_ZERO(
					LEGACY_S32_WRAP_MUL((legacy_s32)idx,
						FRAME_EXPLOSION_FIXED_SCALE),
					(legacy_s32)sdgame2_widths[shape_or_tile_index]));
			shape2d_draw_scaled_transparent_clipped(shape_count_or_extent, sdgame2shapes[shape_or_tile_index], offset_vector.x, offset_vector.y);
		}
	}

/*
; --------------------------------------------------------
*/

	// Depict windscreen cracking after a crash
	sprite_set_target_clip_bounds(0, FRAME_SCREEN_WIDTH, cliprect->top,
		cliprect->bottom);
	if (cameramode == CAMERA_MODE_COCKPIT) {

		if (followOpponentFlag != 0) {
			viewed_carstate = &state.opponentstate;
			tile_angle_or_frame_index = state.game_oEndFrame;
		} else {
			viewed_carstate = &state.playerstate;
			tile_angle_or_frame_index = state.game_pEndFrame;
		}

		if (viewed_carstate->car_crashBmpFlag ==
			CRASH_EVENT_COLLISION) {
			if (slow_video_mgmt_copy != 0) {
				rect_union(init_crak(state.game_frame - tile_angle_or_frame_index, cliprect->top, cliprect->bottom - cliprect->top), frame_layer_rects, frame_layer_rects);
			} else {
				init_crak(state.game_frame - tile_angle_or_frame_index, cliprect->top, cliprect->bottom - cliprect->top);
			}
		} else if (viewed_carstate->car_crashBmpFlag ==
			CRASH_EVENT_WATER) {
			if (slow_video_mgmt_copy != 0) {
				rect_union(do_sinking(state.game_frame - tile_angle_or_frame_index, cliprect->top, cliprect->bottom - cliprect->top), frame_layer_rects, frame_layer_rects);
			} else {
				do_sinking(state.game_frame - tile_angle_or_frame_index, cliprect->top, cliprect->bottom - cliprect->top);
			}
		}
	}

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

	if (slow_video_mgmt_copy != 0) {
		rect_union(draw_ingame_text(), frame_layer_rects, frame_layer_rects);
		if (skybox_requires_full_redraw != 0) {
			frame_layer_rects[0] = *cliprect;
			for (tile_angle_or_frame_index = 1; tile_angle_or_frame_index < FRAME_DIRTY_RECT_COUNT; tile_angle_or_frame_index++) {
				frame_layer_rects[tile_angle_or_frame_index] = empty_rect;
			}
		}

		for (tile_angle_or_frame_index = 0; tile_angle_or_frame_index < FRAME_DIRTY_RECT_COUNT; tile_angle_or_frame_index++) {
			active_frame_rects[tile_angle_or_frame_index] = frame_layer_rects[tile_angle_or_frame_index];
		}
		frame_buffer_camera_headings[buffer_index] = camera_yaw;
		last_rendered_camera_heading = camera_yaw;

	} else {
		draw_ingame_text();
	}

}
