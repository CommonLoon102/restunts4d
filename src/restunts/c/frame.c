#include "frame_internal.h"

#define TRACK_OBJECT_COUNT 215U

/*
 * In the original dseg, sceneshapes2 immediately follows trkObjectList.
 * Some track objects store overlay indices into that combined legacy table,
 * so indices beyond trkObjectList intentionally address sceneshapes2.
 */
struct TRACKOBJECT* frame_track_object_from_legacy_index(
	legacy_u8 index)
{
	if (index < TRACK_OBJECT_COUNT)
		return &trkObjectList[index];
	return &sceneshapes2[(legacy_u16)index - TRACK_OBJECT_COUNT];
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
	transformedshape_arg2array[index] = (legacy_s8)(legacy_u8)type;
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

static legacy_s16 frame_car_z_adjust(const legacy_s8* wheel_surfaces,
	struct MATRIX* rotation)
{
	struct VECTOR offset_vector;
	struct VECTOR rotated_vector;

	if (wheel_surfaces[0] == 4 && wheel_surfaces[1] == 4 &&
		wheel_surfaces[2] == 4 && wheel_surfaces[3] == 4)
		return 0;
	offset_vector.x = 0;
	offset_vector.z = 0;
	offset_vector.y = 0x7530;
	mat_mul_vector(&offset_vector, rotation, &rotated_vector);
	mat_mul_vector(&rotated_vector, &mat_temp, &offset_vector);
	if (offset_vector.z <= 0)
		return -0x800;
	return 0x800;
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
		LEGACY_S16_WRAP_NEGATE(carstate->car_rotate.x), 0);
	best_tile_index = -1;
	matched_wheel = -1;
	for (wheel = 0; wheel < 4; wheel++) {
		offset_vector = simd->wheel_coords[wheel];
		mat_mul_vector(&offset_vector, rotation, &rotated_vector);
		tile_east = frame_tile_from_world_offset(
			carstate->car_posWorld1.lx, rotated_vector.x);
		tile_south = frame_south_tile_from_world_offset(
			carstate->car_posWorld1.lz, rotated_vector.z);
		for (tile_index = 0x16; tile_index > best_tile_index;
			tile_index--) {
			if (should_skip_tile[tile_index] != 2 &&
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
	curtransshape_ptr->rectptr = &rect_unk6;
	curtransshape_ptr->ts_flags = flags;
	curtransshape_ptr->rotvec.x = LEGACY_S16_WRAP_NEGATE(
		state.field_2FE[state_index]);
	curtransshape_ptr->rotvec.y = LEGACY_S16_WRAP_NEGATE(
		state.field_32E[state_index]);
	curtransshape_ptr->rotvec.z = LEGACY_S16_WRAP_NEGATE(
		state.field_35E[state_index]);
	curtransshape_ptr->unk = 0x400;
	curtransshape_ptr->material = material;
	transformed_shape_add_for_sort(z_adjust, 0);
}

static void frame_prepare_flat_track_shape(struct TRANSFORMEDSHAPE3D* shape,
	legacy_s8 tile_east, legacy_s8 tile_south,
	const struct VECTOR* camera_position, legacy_s16 flags,
	legacy_s16 rotation)
{
	shape->pos.x = LEGACY_S16_WRAP_SUB(
		trackcenterpos2[tile_east], camera_position->x);
	shape->pos.y = LEGACY_S16_WRAP_NEGATE(camera_position->y);
	shape->pos.z = LEGACY_S16_WRAP_SUB(
		trackcenterpos[tile_south], camera_position->z);
	shape->rectptr = &rect_unk2;
	shape->ts_flags = flags;
	shape->rotvec.x = 0;
	shape->rotvec.y = 0;
	shape->rotvec.z = rotation;
	shape->unk = 0x400;
	shape->material = 0;
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

	if (state.field_42A != 0) {
		for (index = 0; index < 0x18; index++) {
			if (state.field_38E[index] != 0 &&
				state.field_443[index] == debris_owner) {
				track_object = &sceneshapes3[state.field_42B[index]];
				curtransshape_ptr->pos.x = frame_relative_position_sum(
					state.game_longs1[index],
					carstate->car_posWorld1.lx, camera_position->x);
				curtransshape_ptr->pos.y = frame_relative_position_sum(
					state.game_longs2[index],
					carstate->car_posWorld1.ly, camera_position->y);
				curtransshape_ptr->pos.z = frame_relative_position_sum(
					state.game_longs3[index],
					carstate->car_posWorld1.lz, camera_position->z);
				frame_add_dynamic_shape(track_object, index,
					flags | 5, material, z_adjust);
			}
		}
	}

	track_object = &trkObjectList[car_object];
	curtransshape_ptr->pos.x = frame_relative_position(
		carstate->car_posWorld1.lx, camera_position->x);
	curtransshape_ptr->pos.y = frame_relative_position(
		carstate->car_posWorld1.ly, camera_position->y);
	curtransshape_ptr->pos.z = frame_relative_position(
		carstate->car_posWorld1.lz, camera_position->z);

	if (tile_detail != 0 || detail_level > 2) {
		curtransshape_ptr->shapeptr = track_object->ss_loShapePtr;
	} else {
		curtransshape_ptr->shapeptr = track_object->ss_shapePtr;
		sub_204AE(wheel_shape, 8U, carstate->car_steeringAngle,
			carstate->car_rc2, wheel_angles, wheel_vectors,
			wheel_vector);
	}

	if (slow_video_mgmt_copy != 0) {
		curtransshape_ptr->rectptr = slow_rect;
		curtransshape_ptr->ts_flags = 0xC;
	} else if (carstate->car_crashBmpFlag != 1) {
		curtransshape_ptr->ts_flags = 4;
	} else {
		*crash_rect = cliprect_unk;
		curtransshape_ptr->rectptr = crash_rect;
		curtransshape_ptr->ts_flags = 0xC;
	}

	curtransshape_ptr->rotvec.x = LEGACY_S16_WRAP_NEGATE(
		carstate->car_rotate.z);
	curtransshape_ptr->rotvec.y = LEGACY_S16_WRAP_NEGATE(
		carstate->car_rotate.y);
	curtransshape_ptr->rotvec.z = LEGACY_S16_WRAP_NEGATE(
		carstate->car_rotate.x);
	curtransshape_ptr->unk = 0x12C;
	curtransshape_ptr->material = material;
	/* The sort slot carries the same id as the track object: 2 for the
	   player, 3 for the opponent. */
	transformed_shape_add_for_sort(z_adjust, (legacy_s16)car_object);
}

/* Border fences: a tile sits on the low edge (0), the high edge (1) or in
   between (2) along each axis, and the pair picks the fence piece. -1 means
   the tile is not on the border at all. */
static const legacy_s8 fence_by_edge[3][3] = {
	{ 7, 5, 6 },
	{ 1, 3, 2 },
	{ 0, 4, -1 }
};

static legacy_s16 frame_border_index(legacy_s8 offset)
{
	if (offset == 0)
		return 0;
	if (offset == 0x1D)
		return 1;
	return 2;
}

/* The camera looks out of the car, so it uses the car's rotation inverted. */
static struct MATRIX* frame_car_rotation(legacy_s16 rot_x, legacy_s16 rot_y,
	legacy_s16 rot_z)
{
	return mat_rot_zxy(LEGACY_S16_WRAP_NEGATE(rot_z),
		LEGACY_S16_WRAP_NEGATE(rot_y),
		LEGACY_S16_WRAP_NEGATE(rot_x), 0);
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
	const struct FRAME_LOOKAHEAD_TILE* lookahead_tiles;
	legacy_s16 skybox_parameter;
	legacy_s16 var_counter;
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
		car_pos.x = position_to_word(
			state.playerstate.car_posWorld1.lx);
		car_pos.y = position_to_word(
			state.playerstate.car_posWorld1.ly);
		car_pos.z = position_to_word(
			state.playerstate.car_posWorld1.lz);
		car_rot_y = state.playerstate.car_rotate.y;
		car_rot_z = state.playerstate.car_rotate.z;
		car_rot_x = state.playerstate.car_rotate.x;
	} else {
		car_pos.x = position_to_word(
			state.opponentstate.car_posWorld1.lx);
		car_pos.y = position_to_word(
			state.opponentstate.car_posWorld1.ly);
		car_pos.z = position_to_word(
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
		car_rot_matrix = frame_car_rotation(car_rot_x, car_rot_y,
			car_rot_z);
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
		car_rot_matrix = frame_car_rotation(car_rot_x, car_rot_y,
			car_rot_z);
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
		cam_pos.x = trackdata9[state.field_3F7[followOpponentFlag]].x;
		cam_pos.y = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
			trackdata9[state.field_3F7[followOpponentFlag]].y,
			camera_track_height_offset), 0x5A);
		cam_pos.z = trackdata9[state.field_3F7[followOpponentFlag]].z;
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
	lookahead_tiles = (const struct FRAME_LOOKAHEAD_TILE*)
		lookahead_tiles_tables[(heading & 0x3FF) >> 7];

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
		if (lookahead_tiles[si].detail <= detail_threshold) {
			tile_east = LEGACY_S8_WRAP_ADD(
				lookahead_tiles[si].east, cam_tile_east);
			tile_south = LEGACY_S8_WRAP_ADD(
				lookahead_tiles[si].south, cam_tile_south);

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
				tile_detail_level[si] = lookahead_tiles[si].detail;

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
								if (lookahead_tiles[di].east == tile_to_draw_east_offset && (lookahead_tiles[di].south == tile_to_draw_south_offset || lookahead_tiles[di].south == tile_to_draw_south_offset + 1)) {
									should_skip_tile[di] = 1;
								}
							}
						} else if (idx == 2) {
							for (di = 0; di < si; di++) {
								if (lookahead_tiles[di].south == tile_to_draw_south_offset && (lookahead_tiles[di].east == tile_to_draw_east_offset || lookahead_tiles[di].east == tile_to_draw_east_offset + 1)) {
									should_skip_tile[di] = 1;
								}
							}
						} else if (idx == 3) {
							for (di = 0; di < si; di++) {
								if ((lookahead_tiles[di].east == tile_to_draw_east_offset || lookahead_tiles[di].east == tile_to_draw_east_offset + 1) &&
									(lookahead_tiles[di].south == tile_to_draw_south_offset || lookahead_tiles[di].south == tile_to_draw_south_offset + 1))
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

			var_6C = frame_find_car_wheel(&state.playerstate,
				&simd_player, should_skip_tile, lookahead_tiles,
				cam_tile_east, cam_tile_south, &var_3C, &var_60);
		}
	}

	// Draw opponent's wheels
	var_4A = -1;
	var_A4 = 0;
	if (gameconfig.game_opponenttype != 0) {

		if (cameramode != 0 || followOpponentFlag == 0) {
			if (state.opponentstate.car_crashBmpFlag != 2) {
				var_A4 = frame_find_car_wheel(&state.opponentstate,
					&simd_opponent, should_skip_tile, lookahead_tiles,
					cam_tile_east, cam_tile_south, &var_4A, &var_6E);
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
				di = fence_by_edge[frame_border_index(
					tile_to_draw_east_offset)][frame_border_index(
					tile_to_draw_south_offset)];

				if (di != -1) {
					var_trkobjectptr = frame_track_object_from_legacy_index(
						fence_TrkObjCodes[di]);
					if (tile_det_level == 0) {
						currenttransshape->shapeptr = var_trkobjectptr->ss_shapePtr;
					} else {
						currenttransshape->shapeptr = var_trkobjectptr->ss_loShapePtr;
					}

					frame_prepare_flat_track_shape(currenttransshape,
						tile_to_draw_east_offset,
						tile_to_draw_south_offset,
						&cam_pos, (legacy_s16)(var_122 | 5),
						word_3C0D6[di]);
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
						frame_prepare_flat_track_shape(currenttransshape,
							tile_to_draw_east_offset,
							tile_to_draw_south_offset,
							&cam_pos, (legacy_s16)(var_122 | 5),
							var_trkobject_ptr->ss_rotY);
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
				var_trkobjectptr = frame_track_object_from_legacy_index(
					var_trkobject_ptr->ss_ssOvelay);
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
						td10_track_check_rel[var_4C].x, cam_pos.x);
					curtransshape_ptr->pos.y = LEGACY_S16_WRAP_SUB(
						td10_track_check_rel[var_4C].y, cam_pos.y);
					curtransshape_ptr->pos.z = LEGACY_S16_WRAP_SUB(
						td10_track_check_rel[var_4C].z, cam_pos.z);
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
								td10_track_check_rel[var_4C].x, cam_pos.x);
							curtransshape_ptr->pos.y = frame_relative_track_position(
								state.game_longs2[di],
								td10_track_check_rel[var_4C].y, cam_pos.y);
							curtransshape_ptr->pos.z = frame_relative_track_position(
								state.game_longs3[di],
								td10_track_check_rel[var_4C].z, cam_pos.z);
							frame_add_dynamic_shape(var_trkobject_ptr, di,
								var_122 | 5, 0, 0);
						}
					}
				}
			}
		}

		if ((var_3C == tile_east || var_3C == tile_to_draw_east_offset) && (var_60 == tile_south || var_60 == tile_to_draw_south_offset)) {
			frame_add_car(&state.playerstate, 0, 2,
				&game3dshapes[0x0AD4 / sizeof(struct SHAPE3D)],
				word_443E8, carshapevecs, carshapevec,
				&rect_unk12, &var_rect, &cam_pos, tile_det_level,
				var_122, gameconfig.game_playermaterial,
				var_6C & var_12A);
		}

		if ((var_4A == tile_east) || (var_4A == tile_to_draw_east_offset)) {
			if ((var_6E == tile_south) || (var_6E == tile_to_draw_south_offset)) {
				frame_add_car(&state.opponentstate, 1, 3,
					&game3dshapes[0x0AEA / sizeof(struct SHAPE3D)],
					word_4448A, oppcarshapevecs, oppcarshapevec,
					&rect_unk15, &var_rect2, &cam_pos, tile_det_level,
					var_122, gameconfig.game_opponentmaterial,
					var_A4 & var_12A);
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
