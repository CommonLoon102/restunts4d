#include "legacy.h"
#include "math.h"
#include "physics_internal.h"
#include "residue.h"

legacy_s16 scale_position_delta(legacy_s32 current,
	legacy_s32 previous, legacy_s16 factor, legacy_s16 divisor)
{
	legacy_s32 delta;
	legacy_s32 product;
	legacy_s32 quotient;

	delta = LEGACY_S32_WRAP_SUB(current, previous);
	product = LEGACY_S32_WRAP_MUL(delta, (legacy_s32)factor);
	quotient = LEGACY_S32_DIV_OR_ZERO(product, (legacy_s32)divisor);
	return LEGACY_S16_FROM_BITS((legacy_u16)quotient);
}

legacy_s16 scale_speed_to_travel(legacy_u16 speed,
	legacy_u16 divisor)
{
	legacy_u32 product;
	legacy_u32 quotient;

	product = LEGACY_U32_WRAP_MUL((legacy_u32)speed, 0x580UL);
	quotient = LEGACY_U32_DIV_OR_ZERO(product, divisor);
	return LEGACY_S16_FROM_BITS((legacy_u16)quotient);
}

legacy_s16 physics_position_word(legacy_s32 position)
{
	return LEGACY_S16_FROM_BITS(
		(legacy_u16)LEGACY_S32_SAR(position, 6U));
}

legacy_s16 physics_difference_word(legacy_s32 left, legacy_s32 right)
{
	return LEGACY_S16_FROM_BITS(
		(legacy_u16)LEGACY_S32_WRAP_SUB(left, right));
}

legacy_s16 wheel_pair_delta(legacy_s16 first, legacy_s16 second,
	legacy_s16 third, legacy_s16 fourth)
{
	return LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_ADD(first, second), third), fourth);
}

legacy_s16 bto_auxiliary1(legacy_s16 column_arg, legacy_s16 row_arg, struct VECTOR* output)
{
	const struct VECTOR* dependency_points;
	legacy_u16 column;
	legacy_u16 row;
	legacy_u16 previous_row_base;
	legacy_u16 center_x;
	legacy_u16 center_z;
	legacy_u16 terrain_height;
	legacy_u16 orientation;
	legacy_u16 count;
	legacy_u16 index;
	legacy_u8 tile_element;
	legacy_u8 multi_tile_flags;
	legacy_s8 physical_model;

	column = (legacy_u16)column_arg;
	row = (legacy_u16)row_arg;
	tile_element = td14_elem_map_main[trackrows[row] + column];
	if (tile_element == 0)
		return 0;

	center_x = (legacy_u16)trackcenterpos2[column];
	center_z = (legacy_u16)trackcenterpos[row];
	previous_row_base = row == 0 ? (legacy_u16)word_45D3E :
		(legacy_u16)trackrows[row - 1U];
	if (tile_element == 0xFDU) {
		tile_element = td14_elem_map_main[
			LEGACY_U16_WRAP_SUB(previous_row_base + column, 1U)];
		multi_tile_flags = trkObjectList[tile_element].ss_multiTileFlag;
		if ((multi_tile_flags & 1U) != 0)
			center_z = (legacy_u16)trackpos[row + 1U];
		if ((multi_tile_flags & 2U) != 0)
			center_x = (legacy_u16)trackpos2[column];
	} else if (tile_element == 0xFEU) {
		tile_element = td14_elem_map_main[previous_row_base + column];
		multi_tile_flags = trkObjectList[tile_element].ss_multiTileFlag;
		if ((multi_tile_flags & 1U) != 0)
			center_z = (legacy_u16)trackpos[row + 1U];
		if ((multi_tile_flags & 2U) != 0)
			center_x = (legacy_u16)trackpos2[column + 1U];
	} else if (tile_element == 0xFFU) {
		tile_element = td14_elem_map_main[
			LEGACY_U16_WRAP_SUB(trackrows[row] + column, 1U)];
		multi_tile_flags = trkObjectList[tile_element].ss_multiTileFlag;
		if ((multi_tile_flags & 1U) != 0)
			center_z = (legacy_u16)trackpos[row];
		if ((multi_tile_flags & 2U) != 0)
			center_x = (legacy_u16)trackpos2[column];
	} else {
		multi_tile_flags = trkObjectList[tile_element].ss_multiTileFlag;
		if ((multi_tile_flags & 1U) != 0)
			center_z = (legacy_u16)trackpos[row];
		if ((multi_tile_flags & 2U) != 0)
			center_x = (legacy_u16)trackpos2[column + 1U];
	}

	dependency_points = 0;
	count = 0;
	physical_model = (legacy_s8)trkObjectList[tile_element].ss_physicalModel;
	if (physical_model == 0x0B ||
		(physical_model >= 0x47 && physical_model <= 0x4A)) {
		dependency_points = unk_3E640;
		count = 1;
	} else if (physical_model == 0x12) {
		dependency_points = unk_3E646;
		count = 8;
	} else if (physical_model == 0x20) {
		dependency_points = unk_3E682;
		count = 2;
	} else if (physical_model == 0x21) {
		dependency_points = unk_3E68E;
		count = 2;
	} else if (physical_model == 0x22) {
		dependency_points = unk_3E69A;
		count = 4;
	} else if (physical_model == 0x23) {
		dependency_points = unk_3E676;
		count = 2;
	}
	if (count == 0)
		return 0;

	terrain_height = td15_terr_map_main[terrainrows[row] + column] == 6 ?
		(legacy_u16)hillHeightConsts[1] : 0;
	orientation = (legacy_u16)trkObjectList[tile_element].ss_rotY;
	for (index = 0; index < count; index++) {
		legacy_u16 source_x;
		legacy_u16 source_y;
		legacy_u16 source_z;
		legacy_u16 rotated_x;
		legacy_u16 rotated_z;

		source_x = (legacy_u16)dependency_points[index].x;
		source_y = (legacy_u16)dependency_points[index].y;
		source_z = (legacy_u16)dependency_points[index].z;
		if (orientation == 0) {
			rotated_x = source_x;
			rotated_z = source_z;
		} else if (orientation == 0x100U) {
			rotated_x = source_z;
			rotated_z = LEGACY_U16_WRAP_SUB(0, source_x);
		} else if (orientation == 0x200U) {
			rotated_x = LEGACY_U16_WRAP_SUB(0, source_x);
			rotated_z = LEGACY_U16_WRAP_SUB(0, source_z);
		} else if (orientation == 0x300U) {
			rotated_x = LEGACY_U16_WRAP_SUB(0, source_z);
			rotated_z = source_x;
		} else {
			continue;
		}
		output[index].x = LEGACY_S16_FROM_BITS(
			LEGACY_U16_WRAP_ADD(rotated_x, center_x));
		output[index].y = LEGACY_S16_FROM_BITS(
			LEGACY_U16_WRAP_ADD(source_y, terrain_height));
		output[index].z = LEGACY_S16_FROM_BITS(
			LEGACY_U16_WRAP_ADD(rotated_z, center_z));
	}
	return count;
}

struct LEGACY_EXECUTION_RESIDUE legacy_execution_residue;

legacy_s16 carState_rc_op(
	struct CARSTATE* carstate,
	legacy_s16 contact_delta_arg,
	legacy_s16 wheel_index
) {
	legacy_s16 previous_rc2;
	legacy_s16 contact_delta;
	legacy_s16 adjustment;
	legacy_s16 scaled_delta;
	legacy_s16 target;

	previous_rc2 = (legacy_s16)carstate->car_rc2[wheel_index];
	contact_delta = (legacy_s16)contact_delta_arg;
	adjustment = 0;

	/* Decay the per-wheel target by four toward zero each frame. */
	target = (legacy_s16)carstate->car_rc5[wheel_index];
	if (target < 0) {
		target = LEGACY_S16_WRAP_ADD(target, 4);
		if (target > 0)
			target = 0;
	} else if (target > 0) {
		target = LEGACY_S16_WRAP_SUB(target, 4);
		if (target < 0)
			target = 0;
	}
	carstate->car_rc5[wheel_index] = target;

	if (contact_delta < 0 &&
		(legacy_s16)carstate->car_rc2[wheel_index] >
		LEGACY_S16_WRAP_NEGATE(contact_delta)) {
		contact_delta = 0;
	}

	if (contact_delta == 0) {
		if ((legacy_s16)carstate->car_rc2[wheel_index] > target) {
			carstate->car_rc2[wheel_index] = LEGACY_S16_WRAP_SUB(
				carstate->car_rc2[wheel_index], 0x80);
			if ((legacy_s16)carstate->car_rc2[wheel_index] < target)
				carstate->car_rc2[wheel_index] = target;
			adjustment = LEGACY_S16_WRAP_SUB(
				previous_rc2, carstate->car_rc2[wheel_index]);
		} else if ((legacy_s16)carstate->car_rc2[wheel_index] < target) {
			carstate->car_rc2[wheel_index] = LEGACY_S16_WRAP_ADD(
				carstate->car_rc2[wheel_index], 0x80);
			if ((legacy_s16)carstate->car_rc2[wheel_index] > target)
				carstate->car_rc2[wheel_index] = target;
		}
	} else if (contact_delta > 0) {
		if (contact_delta > 0xC0)
			contact_delta = 0xC0;
		carstate->car_rc2[wheel_index] = LEGACY_S16_WRAP_ADD(
			carstate->car_rc2[wheel_index], contact_delta);
		if ((legacy_s16)carstate->car_rc2[wheel_index] > 0x180)
			carstate->car_rc2[wheel_index] = 0x180;
		carstate->car_rc4[wheel_index] = 0;
	} else {
		if (LEGACY_S16_WRAP_ADD(
			contact_delta, carstate->car_rc2[wheel_index]) > -0x120) {
			carstate->car_rc2[wheel_index] = LEGACY_S16_WRAP_ADD(
				carstate->car_rc2[wheel_index], contact_delta);
		} else {
			scaled_delta = LEGACY_S16_SAR2(
				LEGACY_S16_WRAP_MUL(contact_delta, 3));
			carstate->car_rc2[wheel_index] = LEGACY_S16_WRAP_ADD(
				carstate->car_rc2[wheel_index], scaled_delta);
			if ((legacy_s16)carstate->car_rc2[wheel_index] < -0x180)
				carstate->car_rc2[wheel_index] = -0x180;
		}
		adjustment = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_SUB(
				previous_rc2, carstate->car_rc2[wheel_index]),
			contact_delta);
	}

	return LEGACY_S16_WRAP_ADD(previous_rc2, adjustment);
}

legacy_s16 car_car_speed_adjust_maybe(
	struct CARSTATE* first_state,
	struct CARSTATE* second_state
) {
	legacy_s16 first_angle;
	legacy_s16 second_angle;
	legacy_s16 first_sin_speed;
	legacy_s16 second_sin_speed;
	legacy_s16 first_cos_speed;
	legacy_s16 second_cos_speed;
	legacy_s16 relative_speed;
	legacy_s16 slowdown;
	legacy_s16 angle_delta;

	first_state->field_C8 = 1;
	second_state->field_C8 = 1;
	first_angle = (legacy_s16)first_state->car_rotate.x;
	second_angle = (legacy_s16)second_state->car_rotate.x;

	first_sin_speed = multiply_and_scale(
		(legacy_s16)(first_state->car_speed2 >> 8),
		sin_fast((legacy_u16)first_angle));
	second_sin_speed = multiply_and_scale(
		(legacy_s16)(second_state->car_speed2 >> 8),
		sin_fast((legacy_u16)second_angle));
	first_cos_speed = multiply_and_scale(
		(legacy_s16)(first_state->car_speed2 >> 8),
		cos_fast((legacy_u16)first_angle));
	second_cos_speed = multiply_and_scale(
		(legacy_s16)(second_state->car_speed2 >> 8),
		cos_fast((legacy_u16)second_angle));

	relative_speed = (legacy_s16)polarRadius2D(
		LEGACY_S16_WRAP_SUB(second_sin_speed, first_sin_speed),
		LEGACY_S16_WRAP_SUB(second_cos_speed, first_cos_speed));
	if (relative_speed < 10)
		relative_speed = 10;

	/* The original keeps only the low product word before shifting it. */
	slowdown = LEGACY_S16_SAR2(
		LEGACY_S16_WRAP_MUL(0x300, relative_speed));
	if ((legacy_u16)first_state->car_speed2 < (legacy_u16)slowdown) {
		first_state->car_speed2 = 0;
	} else {
		first_state->car_speed2 = LEGACY_U16_WRAP_SUB(
			first_state->car_speed2, slowdown);
	}

	angle_delta = LEGACY_S16_WRAP_SUB(second_angle, first_angle);
	if (angle_delta >= 0x200)
		angle_delta = LEGACY_S16_WRAP_SUB(angle_delta, 0x400);
	if (angle_delta <= -0x200)
		angle_delta = LEGACY_S16_WRAP_ADD(angle_delta, 0x400);
	first_state->car_36MwhlAngle = angle_delta;

	angle_delta = LEGACY_S16_WRAP_SUB(first_angle, second_angle);
	if (angle_delta >= 0x200)
		angle_delta = LEGACY_S16_WRAP_SUB(angle_delta, 0x400);
	if (angle_delta <= -0x200)
		angle_delta = LEGACY_S16_WRAP_ADD(angle_delta, 0x400);
	second_state->car_36MwhlAngle = angle_delta;

	first_state->car_speed = first_state->car_speed2;
	second_state->car_speed = second_state->car_speed2;
	return relative_speed > 0x1E;
}

static legacy_s16 collision_axis_distance(legacy_s16 first, legacy_s16 second) {
	if (first < second)
		return LEGACY_S16_WRAP_SUB(second, first);
	return LEGACY_S16_WRAP_SUB(first, second);
}

static void build_collision_corners(
	struct POINT2D* collision_points,
	struct VECTOR* world_coordinates,
	struct VECTOR corners[4]
) {
	struct MATRIX* rotation;
	struct VECTOR local_corner;
	legacy_s16 corner;

	rotation = mat_rot_zxy(
		LEGACY_S16_WRAP_NEGATE(world_coordinates[1].x),
		LEGACY_S16_WRAP_NEGATE(world_coordinates[1].y),
		LEGACY_S16_WRAP_NEGATE(world_coordinates[1].z),
		0);
	for (corner = 0; corner < 4; corner++) {
		if (corner == 0 || corner == 3) {
			local_corner.x = LEGACY_S16_WRAP_NEGATE(
				collision_points[0].px);
		} else {
			local_corner.x = (legacy_s16)collision_points[0].px;
		}
		local_corner.y = 0;
		if (corner >= 2) {
			local_corner.z = LEGACY_S16_WRAP_NEGATE(
				collision_points[1].px);
		} else {
			local_corner.z = (legacy_s16)collision_points[1].px;
		}

		mat_mul_vector(&local_corner, rotation, &corners[corner]);
		corners[corner].x = LEGACY_S16_WRAP_ADD(
			corners[corner].x, world_coordinates[0].x);
		corners[corner].y = LEGACY_S16_WRAP_ADD(
			corners[corner].y, world_coordinates[0].y);
		corners[corner].z = LEGACY_S16_WRAP_ADD(
			corners[corner].z, world_coordinates[0].z);
	}
}

static legacy_s16 collision_corners_inside(
	struct VECTOR corners[4],
	struct POINT2D* collision_points,
	struct VECTOR* world_coordinates
) {
	struct MATRIX* rotation;
	struct VECTOR relative_corner;
	struct VECTOR local_corner;
	legacy_s16 negative_extent;
	legacy_s16 corner;

	rotation = mat_rot_zxy(
		world_coordinates[1].x,
		world_coordinates[1].y,
		world_coordinates[1].z,
		1);
	for (corner = 0; corner < 4; corner++) {
		relative_corner.x = LEGACY_S16_WRAP_SUB(
			world_coordinates[0].x, corners[corner].x);
		relative_corner.y = LEGACY_S16_WRAP_SUB(
			world_coordinates[0].y, corners[corner].y);
		relative_corner.z = LEGACY_S16_WRAP_SUB(
			world_coordinates[0].z, corners[corner].z);
		mat_mul_vector(&relative_corner, rotation, &local_corner);

		if (local_corner.y < 0 ||
			local_corner.y > (legacy_s16)collision_points[0].py) {
			continue;
		}
		negative_extent = LEGACY_S16_WRAP_NEGATE(collision_points[0].px);
		if (local_corner.x < negative_extent ||
			local_corner.x > (legacy_s16)collision_points[0].px) {
			continue;
		}
		negative_extent = LEGACY_S16_WRAP_NEGATE(collision_points[1].px);
		if (local_corner.z < negative_extent ||
			local_corner.z > (legacy_s16)collision_points[1].px) {
			continue;
		}
		return 1;
	}

	return 0;
}

legacy_s16 car_car_coll_detect_maybe(
	struct POINT2D* first_collision_points,
	struct VECTOR* first_world_coordinates,
	struct POINT2D* second_collision_points,
	struct VECTOR* second_world_coordinates
) {
	struct VECTOR position_delta;
	struct VECTOR corners[4];
	legacy_s16 combined_radius;

	combined_radius = LEGACY_S16_WRAP_ADD(
		first_collision_points[1].py,
		second_collision_points[1].py);
	if (collision_axis_distance(
		first_world_coordinates[0].x,
		second_world_coordinates[0].x) > combined_radius) {
		return 0;
	}
	if (collision_axis_distance(
		first_world_coordinates[0].z,
		second_world_coordinates[0].z) > combined_radius) {
		return 0;
	}
	if (collision_axis_distance(
		first_world_coordinates[0].y,
		second_world_coordinates[0].y) > combined_radius) {
		return 0;
	}

	position_delta.x = LEGACY_S16_WRAP_SUB(
		first_world_coordinates[0].x, second_world_coordinates[0].x);
	position_delta.y = LEGACY_S16_WRAP_SUB(
		first_world_coordinates[0].y, second_world_coordinates[0].y);
	position_delta.z = LEGACY_S16_WRAP_SUB(
		first_world_coordinates[0].z, second_world_coordinates[0].z);
	if ((legacy_u16)polarRadius3D(&position_delta) >
		(legacy_u16)combined_radius) {
		return 0;
	}

	build_collision_corners(
		first_collision_points, first_world_coordinates, corners);
	if (collision_corners_inside(
		corners, second_collision_points, second_world_coordinates)) {
		return 1;
	}

	build_collision_corners(
		second_collision_points, second_world_coordinates, corners);
	return collision_corners_inside(
		corners, first_collision_points, first_world_coordinates);
}
