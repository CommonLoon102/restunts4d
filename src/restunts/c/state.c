#include "externs.h"
#include "legacy.h"
#include "math.h"
#include "memmgr.h"
#include "residue.h"
#include "shape3d.h"

extern legacy_s16 penalty_time;
extern legacy_s16 grassDecelDivTab[];
extern struct TRACKOBJECT trkObjectList[215];
extern legacy_u8 oppnentSped[];
extern struct PLANE far* planptr;
extern struct PLANE far plan_memres;
extern legacy_s16 track_pieces_counter;
extern legacy_u8 byte_3E71E[];
extern legacy_u8 byte_3E724[];
extern legacy_u8 terrConnDataEtoW[];
extern legacy_u8 terrConnDataWtoE[];
extern legacy_u8 terrConnDataNtoS[];
extern legacy_u8 terrConnDataStoN[];
extern legacy_u8 byte_45635;
extern legacy_u8 byte_45D90;
extern legacy_u8 byte_45E16;
extern legacy_u8 byte_4616E;

#if defined(RESTUNTS_HEADLESS) || defined(RESTUNTS_FULL)
extern struct VECTOR* headless_track_vector_from_legacy_offset(
	legacy_u16 offset);
#endif

static struct VECTOR* track_vector_from_legacy_offset(legacy_u16 offset)
{
#if defined(RESTUNTS_HEADLESS) || defined(RESTUNTS_FULL)
	return headless_track_vector_from_legacy_offset(offset);
#else
	return (struct VECTOR*)offset;
#endif
}

legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 track);

static legacy_s16 penalty_route_next(legacy_s16 track_index)
{
	legacy_u8 far* route_bytes;
	legacy_u16 byte_offset;
	legacy_u16 value;

	/*
	 * The original instruction sequence doubles the 16-bit index and adds
	 * it to the far pointer's offset without normalizing the segment.  In
	 * particular, route index -1 reads offset FFFEh in the same segment.
	 * Express that wrap explicitly so DOS and flat-memory builds agree.
	 */
	route_bytes = (legacy_u8 far*)td01_track_file_cpy;
	byte_offset = LEGACY_U16_WRAP_MUL(track_index, 2U);
	value = (legacy_u16)route_bytes[byte_offset];
	value |= (legacy_u16)route_bytes[
		LEGACY_U16_WRAP_ADD(byte_offset, 1U)] << 8;
	return LEGACY_S16_FROM_BITS(value);
}

legacy_s16 detect_penalty(legacy_s16* current_track, legacy_s16* penalty_count)
{
	legacy_u8 visited[904];
	legacy_s16 pending_track[128];
	legacy_s16 pending_distance[128];
	legacy_u16 pending_count;
	legacy_s16 track_index;
	legacy_s16 next_track;
	legacy_s16 alternate_track;
	legacy_s16 best_track;
	legacy_s16 distance;
	legacy_s16 best_distance;
	legacy_s16 column;
	legacy_s16 row;
	legacy_u8 minimum_column;
	legacy_u8 maximum_column;
	legacy_u8 minimum_row;
	legacy_u8 maximum_row;
	legacy_u8 tile_element;
	legacy_u8 multi_tile_flags;
	legacy_u8 sentinel_visited;
	legacy_u16 index;

	column = LEGACY_S8_FROM_BITS(
		(legacy_u8)((legacy_u32)state.playerstate.car_posWorld1.lx >> 16));
	row = LEGACY_S8_FROM_BITS((legacy_u8)(0x1DU -
		(legacy_u8)((legacy_u32)state.playerstate.car_posWorld1.lz >> 16)));
	if ((column == state.game_startcol || column == state.game_startcol2) &&
		(row == state.game_startrow || row == state.game_startrow2)) {
		*penalty_count = 0;
		return 0;
	}
	if (column < 0 || column > 0x1D || row < 0 || row > 0x1D) {
		*penalty_count = -2;
		return 1;
	}

	best_distance = 0;
	best_track = 0;
	pending_count = 0;
	distance = 0;
	sentinel_visited = 0;
	for (index = 0; index < (legacy_u16)track_pieces_counter; index++)
		visited[index] = 0;
	track_index = (legacy_s16)*current_track;

	for (;;) {
		next_track = penalty_route_next(track_index);
		if (next_track == -1) {
			if (sentinel_visited != 0)
				goto backtrack;
			sentinel_visited = 1;
		} else if (next_track < 0 ||
			next_track >= track_pieces_counter ||
			visited[next_track] != 0) {
backtrack:
			if (pending_count != 0) {
				pending_count--;
				track_index = pending_track[pending_count];
				distance = pending_distance[pending_count];
				continue;
			}
			if (best_distance != 0) {
				*current_track = best_track;
				*penalty_count = best_distance;
				return 1;
			}
			state.game_startcol = column;
			state.game_startcol2 = column;
			state.game_startrow = row;
			state.game_startrow2 = row;
			*penalty_count = -2;
			return 1;
		} else {
			visited[next_track] = 1;
		}

		minimum_row = (legacy_u8)td22_row_from_path[next_track];
		tile_element = (legacy_u8)td17_trk_elem_ordered[next_track];
		multi_tile_flags = trkObjectList[tile_element].ss_multiTileFlag;
		maximum_row = minimum_row;
		if ((multi_tile_flags & 1U) != 0)
			maximum_row++;
		minimum_column = (legacy_u8)td21_col_from_path[next_track];
		maximum_column = minimum_column;
		if ((multi_tile_flags & 2U) != 0)
			maximum_column++;

		if (((legacy_u8)column == minimum_column ||
			(legacy_u8)column == maximum_column) &&
			((legacy_u8)row == minimum_row ||
			(legacy_u8)row == maximum_row)) {
			if (td02_penalty_related[track_index] != -1)
				next_track = track_index;
			state.game_startcol = LEGACY_S8_FROM_BITS(minimum_column);
			state.game_startcol2 = LEGACY_S8_FROM_BITS(maximum_column);
			state.game_startrow = LEGACY_S8_FROM_BITS(minimum_row);
			state.game_startrow2 = LEGACY_S8_FROM_BITS(maximum_row);
			if (distance <= 0) {
				*current_track = next_track;
				*penalty_count = distance;
				return 1;
			}
			if (best_distance == 0 || best_distance > distance) {
				best_track = next_track;
				best_distance = distance;
			}
		}

		alternate_track = td02_penalty_related[track_index];
		if (alternate_track != -1) {
			pending_distance[pending_count] = distance;
			pending_track[pending_count] = alternate_track;
			pending_count++;
		}
		if (next_track == 0) {
			distance = -1;
		} else if (distance != -1) {
			distance = LEGACY_S16_WRAP_ADD(distance, 1);
		}
		track_index = next_track;
	}
}

/*
 * The original player update reuses four words below player_op's stack frame.
 * update_car_speed and update_grip write that same physical window before the
 * player physics reads it. Keep the window as explicit 16-bit execution state
 * so its behavior does not depend on a compiler's frame layout or ABI.
 */
static void update_legacy_grip_stack_words(
	struct CARSTATE* carstate,
	struct SIMD* simd,
	legacy_u16 speed_before_grip,
	legacy_u16 speed2_before_grip
) {
	legacy_s16 combined_grip_operand;
	legacy_s16 sliding_sum;
	legacy_s16* sliding_values;
	legacy_u16 grip_speed;
	legacy_u16 speed_shr8;
	legacy_u32 speed_squared;
	legacy_s32 scaled_combined_grip;
	legacy_s16 grass_wheels;
	legacy_s16 i;

	/* The original player_op reaches update_grip with SI == 0x50. */
	legacy_execution_residue.grip_stack_words[3] = 0x50;
	if (carstate->car_sumSurfAllWheels == 0)
		return;

	/*
	 * Reproduce update_grip's first operands: twice the car's base grip and
	 * the sum of the four surface-specific sliding coefficients.
	 */
	combined_grip_operand = (legacy_s16)((legacy_u16)simd->grip << 1);
	sliding_sum = 0;
	sliding_values = &simd->sliding;
	for (i = 0; i < 4; i++) {
		sliding_sum = (legacy_s16)(
			(legacy_u16)sliding_sum +
			(legacy_u16)sliding_values[
				(legacy_u8)carstate->car_surfaceWhl[i]
			]
		);
	}

	/* Operand words left by update_grip's first signed long multiply. */
	legacy_execution_residue.grip_stack_words[0] = sliding_sum < 0 ? -1 : 0;
	legacy_execution_residue.grip_stack_words[1] = combined_grip_operand;
	legacy_execution_residue.grip_stack_words[2] =
		combined_grip_operand < 0 ? -1 : 0;

	if (carstate->car_demandedGrip <= carstate->car_surfacegrip_sum)
		return;

	/*
	 * Sliding grip uses the post-deceleration speed when any wheel is on
	 * grass, with the divisor selected by the number of grass wheels.
	 */
	grass_wheels = 0;
	for (i = 0; i < 4; i++) {
		if (carstate->car_surfaceWhl[i] == 4)
			grass_wheels++;
	}
	grip_speed = speed_before_grip;
	if (grass_wheels != 0) {
		speed2_before_grip = LEGACY_U16_WRAP_SUB(speed2_before_grip,
			LEGACY_U16_DIV_OR_ZERO(speed2_before_grip,
				grassDecelDivTab[grass_wheels]));
		grip_speed = speed2_before_grip;
	}

	/* Operand words left by the sliding-grip signed long division. */
	speed_shr8 = grip_speed >> 8;
	speed_squared = (legacy_u32)speed_shr8 * speed_shr8;
	scaled_combined_grip =
		(legacy_s32)carstate->car_surfacegrip_sum * 0x100L;
	legacy_execution_residue.grip_stack_words[0] =
		(legacy_s16)((legacy_u32)scaled_combined_grip >> 16);
	legacy_execution_residue.grip_stack_words[1] = (legacy_s16)speed_squared;
	legacy_execution_residue.grip_stack_words[2] =
		(legacy_s16)(speed_squared >> 16);
}

void update_car_speed(legacy_s8, legacy_s16, struct CARSTATE* carstate, struct SIMD* simd);
void update_player_state(struct CARSTATE* playerstate, struct SIMD* playersimd, struct CARSTATE* oppstate, struct SIMD* oppsimd, legacy_s16);

void update_grip(struct CARSTATE* carstate, struct SIMD* simd,
	legacy_s16 player_behavior)
{
	legacy_s16 initial_angle;
	legacy_s16 adjusted_angle;
	legacy_s16 absolute_angle;
	legacy_s16 angle_factor;
	legacy_s16 combined_grip;
	legacy_s16 sliding_sum;
	legacy_s16 correction;
	legacy_s16 rotation_low;
	legacy_s16 penalty;
	legacy_s16 quotient;
	legacy_s16* sliding_values;
	legacy_u16 speed_shr8;
	legacy_u16 demanded_grip;
	legacy_u16 square_low;
	legacy_u16 grass_wheels;
	legacy_u16 i;
	legacy_u8 tile_x;
	legacy_u8 tile_z;
	legacy_u8 track;
	legacy_s32 product;
	legacy_s32 numerator;
	legacy_s32 denominator;

	if (carstate->car_sumSurfAllWheels == 0) {
		carstate->car_40MfrontWhlAngle = 0;
		carstate->car_slidingFlag = 0;
		return;
	}

	grass_wheels = 0;
	for (i = 0; i < 4U; i++) {
		if (carstate->car_surfaceWhl[i] == 4)
			grass_wheels++;
	}
	if (grass_wheels != 0) {
		carstate->car_speed2 = LEGACY_U16_WRAP_SUB(
			carstate->car_speed2,
			LEGACY_U16_DIV_OR_ZERO(carstate->car_speed2,
				grassDecelDivTab[grass_wheels]));
		carstate->car_speed = carstate->car_speed2;
	}

	initial_angle = LEGACY_S16_WRAP_ADD(carstate->car_steeringAngle,
		carstate->car_36MwhlAngle);
	adjusted_angle = initial_angle;
	speed_shr8 = (legacy_u16)(carstate->car_speed >> 8);
	absolute_angle = adjusted_angle;
	if (absolute_angle < 0)
		absolute_angle = LEGACY_S16_WRAP_NEGATE(absolute_angle);
	angle_factor = LEGACY_S16_SAR(absolute_angle, 3U);
	square_low = LEGACY_U16_WRAP_MUL(speed_shr8, speed_shr8);
	square_low = (legacy_u16)(square_low >> 6);
	demanded_grip = LEGACY_U16_WRAP_MUL(square_low, angle_factor);

	combined_grip = LEGACY_S16_FROM_BITS(
		(legacy_u16)simd->grip << 1);
	sliding_sum = 0;
	sliding_values = &simd->sliding;
	for (i = 0; i < 4U; i++) {
		sliding_sum = LEGACY_S16_WRAP_ADD(sliding_sum,
			sliding_values[(legacy_u8)carstate->car_surfaceWhl[i]]);
	}
	product = (legacy_s32)combined_grip * (legacy_s32)sliding_sum;
	combined_grip = LEGACY_S16_FROM_BITS(
		(legacy_u16)LEGACY_S32_SAR(product, 10U));
	carstate->car_demandedGrip = (legacy_s16)demanded_grip;
	carstate->car_surfacegrip_sum = combined_grip;

	if (player_behavior == 0) {
		carstate->car_40MfrontWhlAngle = LEGACY_S16_FROM_BITS(
			(legacy_u16)carstate->car_steeringAngle << 2);
		if (carstate->car_angle_z != 0) {
			carstate->car_angle_z = LEGACY_S16_SAR(
				LEGACY_S16_WRAP_MUL(carstate->car_angle_z, 15), 4U);
		}
		goto finish_angles;
	}

	if (carstate->car_steeringAngle == 0) {
		rotation_low = LEGACY_S8_FROM_BITS(
			(legacy_u8)carstate->car_rotate.x);
		if (rotation_low != 0) {
			absolute_angle = rotation_low;
			if (absolute_angle < 0)
				absolute_angle = LEGACY_S16_WRAP_NEGATE(absolute_angle);
			if (absolute_angle < 8) {
				if (rotation_low > 0) {
					carstate->car_rotate.x = LEGACY_S16_WRAP_SUB(
						carstate->car_rotate.x, 1);
				} else {
					carstate->car_rotate.x = LEGACY_S16_WRAP_ADD(
						carstate->car_rotate.x, 1);
				}
			}
		}
	}

	if (LEGACY_S16_FROM_BITS(demanded_grip) > combined_grip) {
		carstate->car_slidingFlag = 1;
		numerator = (legacy_s32)combined_grip * 0x100L;
		denominator = (legacy_s32)speed_shr8 * (legacy_s32)speed_shr8;
		adjusted_angle = LEGACY_S16_FROM_BITS(
			(legacy_u16)LEGACY_S32_DIV_OR_ZERO(numerator, denominator));
		if (initial_angle < 0)
			adjusted_angle = LEGACY_S16_WRAP_NEGATE(adjusted_angle);
		adjusted_angle = LEGACY_S16_SAR(LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_MUL(adjusted_angle, 3), initial_angle), 2U);
		carstate->field_42 = LEGACY_S16_WRAP_SUB(
			initial_angle, adjusted_angle);
	} else {
		carstate->car_slidingFlag = 0;
		if (carstate->field_42 != 0) {
			carstate->field_42 = LEGACY_S16_WRAP_SUB(
				carstate->field_42, LEGACY_S16_SAR(
					carstate->field_42, 4U));
			absolute_angle = carstate->field_42;
			if (absolute_angle < 0)
				absolute_angle = LEGACY_S16_WRAP_NEGATE(absolute_angle);
			if (absolute_angle < 0x10)
				carstate->field_42 = LEGACY_S16_SAR(
					carstate->field_42, 1U);
		}
	}

	if (carstate->car_angle_z == 0 && carstate->car_crashBmpFlag != 1)
		carstate->car_40MfrontWhlAngle = adjusted_angle;
	else
		carstate->car_40MfrontWhlAngle = 0;

	absolute_angle = carstate->car_rotate.z;
	if (absolute_angle < 0)
		absolute_angle = LEGACY_S16_WRAP_NEGATE(absolute_angle);
	if (absolute_angle > 4) {
		tile_x = (legacy_u8)((legacy_u32)
			carstate->car_posWorld1.lx >> 16);
		tile_z = (legacy_u8)((legacy_u32)
			carstate->car_posWorld1.lz >> 16);
		track = td14_elem_map_main[
			LEGACY_U16_WRAP_ADD(terrainrows[tile_z], tile_x)];
		if (track == 0xFDU) {
			tile_x = (legacy_u8)(tile_x - 1U);
			tile_z = (legacy_u8)(tile_z + 1U);
		} else if (track == 0xFEU) {
			tile_z = (legacy_u8)(tile_z + 1U);
		} else if (track == 0xFFU) {
			tile_x = (legacy_u8)(tile_x - 1U);
		}
		track = td14_elem_map_main[
			LEGACY_U16_WRAP_ADD(terrainrows[tile_z], tile_x)];
		if (track >= 0x34U && track <= 0x37U) {
			carstate->car_40MfrontWhlAngle = LEGACY_S16_WRAP_ADD(
				carstate->car_40MfrontWhlAngle,
				LEGACY_S16_DIV_OR_ZERO(carstate->car_rotate.z, 5));
		}
	}

	correction = LEGACY_S16_DIV_OR_ZERO(
		LEGACY_S16_WRAP_SUB(adjusted_angle, initial_angle), 0x0E);
	if (LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
		combined_grip, 0x3E8U)) < LEGACY_S16_FROM_BITS(demanded_grip)) {
		carstate->car_angle_z = LEGACY_S16_WRAP_ADD(
			carstate->car_angle_z, correction);
		carstate->car_angle_z = LEGACY_S16_DIV_OR_ZERO(
			carstate->car_angle_z, 2);
	} else if (carstate->car_angle_z != 0) {
		carstate->car_angle_z = LEGACY_S16_WRAP_ADD(
			carstate->car_angle_z, correction);
		carstate->car_angle_z = LEGACY_S16_DIV_OR_ZERO(
			carstate->car_angle_z, 2);
		if (carstate->car_angle_z == 0) {
			carstate->car_speed2 = (legacy_u16)multiply_and_scale(
				cos_fast(carstate->car_36MwhlAngle),
				carstate->car_speed2);
			if (cos_fast(carstate->car_36MwhlAngle) < 0)
				carstate->car_speed2 = 0;
			carstate->car_36MwhlAngle = 0;
		}
	}

finish_angles:
	if (carstate->car_36MwhlAngle != 0 && carstate->car_angle_z == 0) {
		carstate->car_36MwhlAngle = LEGACY_S16_SAR(
			LEGACY_S16_WRAP_MUL(carstate->car_36MwhlAngle, 15), 4U);
	}
	if (carstate->car_angle_z != 0) {
		carstate->car_36MwhlAngle = LEGACY_S16_WRAP_SUB(
			carstate->car_36MwhlAngle, carstate->car_angle_z);
	}

	if (carstate->car_slidingFlag != 0) {
		absolute_angle = carstate->field_42;
		if (absolute_angle < 0)
			absolute_angle = LEGACY_S16_WRAP_NEGATE(absolute_angle);
		penalty = LEGACY_S16_FROM_BITS((legacy_u16)absolute_angle << 1);
		if (carstate->car_speed <= (legacy_u16)penalty) {
			carstate->car_speed = 0;
			carstate->car_speed2 = 0;
		} else if (carstate->car_speed2 > (legacy_u16)penalty) {
			carstate->car_speed = LEGACY_U16_WRAP_SUB(
				carstate->car_speed, penalty);
			carstate->car_speed2 = LEGACY_U16_WRAP_SUB(
				carstate->car_speed2, penalty);
		} else {
			carstate->car_speed = 0;
			carstate->car_speed2 = 0;
		}

		if (carstate->car_crashBmpFlag == 0) {
			for (i = 0; i < 4U; i++) {
				if (carstate->car_surfaceWhl[i] == 1)
					break;
			}
			carstate->field_CF = (legacy_u8)carstate->field_CF |
				(i < 4U ? 2U : 4U);
		}
	}
	carstate->field_42 = 0;
}

static legacy_s16 opponent_position(legacy_s32 position)
{
	legacy_u32 bits;

	bits = (legacy_u32)position;
	bits = (bits >> 6) |
		((bits & 0x80000000UL) != 0 ? 0xFC000000UL : 0);
	return LEGACY_S16_FROM_BITS((legacy_u16)bits);
}

static legacy_s16 opponent_route_word(legacy_s16 index)
{
	legacy_u16 offset;
	legacy_u16 value;

	offset = LEGACY_U16_WRAP_MUL(index, 2U);
	value = (legacy_u8)trackdata3[offset];
	value |= (legacy_u16)(legacy_u8)trackdata3[
		LEGACY_U16_WRAP_ADD(offset, 1U)] << 8;
	return LEGACY_S16_FROM_BITS(value);
}

static void opponent_advance_route(void)
{
	legacy_u8 route_point;

	route_point = (legacy_u8)state.opponentstate.field_CE;
	state.opponentstate.field_CE = (legacy_u8)(route_point + 1U);
	if (sub_18D60(opponent_route_word(
		state.opponentstate.car_trackdata3_index),
		&state.opponentstate.car_vec_unk3, route_point,
		(legacy_s16*)&state.field_3F9) == 0) {
		return;
	}
	state.opponentstate.car_trackdata3_index = LEGACY_S16_WRAP_ADD(
		state.opponentstate.car_trackdata3_index, 1);
	if (opponent_route_word(
		state.opponentstate.car_trackdata3_index) == 0) {
		state.opponentstate.field_CD =
			(legacy_u8)(state.opponentstate.field_CD + 1U);
		state.opponentstate.car_trackdata3_index = 0;
	}
	state.opponentstate.field_CE = 0;
}

static legacy_s16 opponent_average(legacy_s16 first, legacy_s16 second)
{
	legacy_s32 sum;

	sum = (legacy_s32)first + (legacy_s32)second;
	return LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S32_SAR(sum, 1U));
}

void opponent_op(void)
{
	struct VECTOR route_target;
	struct VECTOR relative;
	struct VECTOR transformed;
	struct MATRIX* rotation;
	legacy_s16 opponent_x;
	legacy_s16 opponent_y;
	legacy_s16 opponent_z;
	legacy_s16 player_x;
	legacy_s16 player_y;
	legacy_s16 player_z;
	legacy_s16 steering_target;
	legacy_s16 steering_delta;
	legacy_s16 absolute_value;
	legacy_s16 steering_step;
	legacy_s16 speed_step;
	legacy_s16 route_distance;
	legacy_s16 player_forward;
	legacy_s16 finish_distance;
	legacy_u16 target_speed;
	legacy_u16 speed_threshold;
	legacy_u8 forced_route;
	legacy_u8 input;

	if (framespersec == 0x14U) {
		steering_step = 8;
		speed_step = 1;
	} else {
		steering_step = 0x10;
		speed_step = 2;
	}
	forced_route = state.opponentstate.car_36MwhlAngle != 0 ||
		state.game_inputmode == 2;
	opponent_x = opponent_position(
		(legacy_s32)state.opponentstate.car_posWorld1.lx);
	opponent_y = opponent_position(
		(legacy_s32)state.opponentstate.car_posWorld1.ly);
	opponent_z = opponent_position(
		(legacy_s32)state.opponentstate.car_posWorld1.lz);
	player_x = opponent_position(
		(legacy_s32)state.playerstate.car_posWorld1.lx);
	player_y = opponent_position(
		(legacy_s32)state.playerstate.car_posWorld1.ly);
	player_z = opponent_position(
		(legacy_s32)state.playerstate.car_posWorld1.lz);
	state.opponentstate.field_CF = 0;
	state.field_45E = 0;
	rotation = mat_rot_zxy(state.opponentstate.car_rotate.z,
		state.opponentstate.car_rotate.y,
		state.opponentstate.car_rotate.x, 1);
	state.opponentstate.field_CF = 1;
	if (state.opponentstate.car_crashBmpFlag != 0) {
		if (state.opponentstate.car_speed2 == 0)
			state.opponentstate.field_CF = 0;
		goto choose_input;
	}

	route_target = state.opponentstate.car_vec_unk3;
	if (route_target.y != -1) {
		relative.x = LEGACY_S16_WRAP_SUB(route_target.x, opponent_x);
		relative.y = LEGACY_S16_WRAP_SUB(route_target.y, opponent_y);
		relative.z = LEGACY_S16_WRAP_SUB(route_target.z, opponent_z);
		route_distance = (legacy_s16)polarRadius3D(&relative);
	} else {
		route_distance = (legacy_s16)polarRadius2D(
			LEGACY_S16_WRAP_SUB(route_target.x, opponent_x),
			LEGACY_S16_WRAP_SUB(route_target.z, opponent_z));
	}
	if (route_distance < 0xC8) {
		opponent_advance_route();
	}

select_route_target:
	route_target = state.opponentstate.car_vec_unk3;
	if (state.game_inputmode != 2) {
		relative.x = LEGACY_S16_WRAP_SUB(player_x, opponent_x);
		relative.y = LEGACY_S16_WRAP_SUB(player_y, opponent_y);
		relative.z = LEGACY_S16_WRAP_SUB(player_z, opponent_z);
		mat_mul_vector(&relative, rotation, &transformed);
		player_forward = transformed.z;
		absolute_value = transformed.x;
		if (absolute_value < 0)
			absolute_value = LEGACY_S16_WRAP_NEGATE(absolute_value);
		if (transformed.y <= 0x5A && absolute_value <= 0xB4 &&
			transformed.z <= 0x258 && transformed.z >= -0xB4) {
			relative.x = LEGACY_S16_WRAP_SUB(
				player_x, state.opponentstate.car_vec_unk3.x);
			relative.y = state.opponentstate.car_vec_unk3.y == -1 ? 0 :
				LEGACY_S16_WRAP_SUB(player_y,
					state.opponentstate.car_vec_unk3.y);
			relative.z = LEGACY_S16_WRAP_SUB(
				player_z, state.opponentstate.car_vec_unk3.z);
			mat_mul_vector(&relative, rotation, &transformed);
			if (transformed.x < 0) {
				route_target.x = opponent_average(
					state.opponentstate.car_vec_unk3.x,
					state.opponentstate.car_vec_unk5.x);
				route_target.y = state.opponentstate.car_vec_unk3.y == -1 ?
					-1 : opponent_average(
						state.opponentstate.car_vec_unk3.y,
						state.opponentstate.car_vec_unk5.y);
				route_target.z = opponent_average(
					state.opponentstate.car_vec_unk3.z,
					state.opponentstate.car_vec_unk5.z);
				if (player_forward > -0x4E &&
					state.playerstate.car_crashBmpFlag == 0) {
					state.field_45E = 2;
				}
			} else {
				route_target.x = opponent_average(
					state.opponentstate.car_vec_unk3.x,
					state.opponentstate.car_vec_unk4.x);
				route_target.y = state.opponentstate.car_vec_unk3.y == -1 ?
					-1 : opponent_average(
						state.opponentstate.car_vec_unk3.y,
						state.opponentstate.car_vec_unk4.y);
				route_target.z = opponent_average(
					state.opponentstate.car_vec_unk3.z,
					state.opponentstate.car_vec_unk4.z);
				if (player_forward > -0x4E &&
					state.playerstate.car_crashBmpFlag == 0) {
					state.field_45E = 1;
				}
			}
		}
	}

	relative.x = LEGACY_S16_WRAP_SUB(route_target.x, opponent_x);
	relative.y = route_target.y == -1 ? 0 :
		LEGACY_S16_WRAP_SUB(route_target.y, opponent_y);
	relative.z = LEGACY_S16_WRAP_SUB(route_target.z, opponent_z);
	mat_mul_vector(&relative, rotation, &transformed);
	steering_target = (legacy_s16)polarAngle(transformed.x, transformed.z);
	if (state.opponentstate.car_slidingFlag == 0) {
		absolute_value = steering_target;
		if (absolute_value < 0)
			absolute_value = LEGACY_S16_WRAP_NEGATE(absolute_value);
		if (absolute_value > 0x100) {
			opponent_advance_route();
		}
	}
	if (steering_target > 0x41) {
		if (forced_route == 0) {
			forced_route = 1;
			opponent_advance_route();
			goto select_route_target;
		}
		steering_target = 0x41;
	} else if (steering_target < -0x41) {
		if (forced_route == 0) {
			forced_route = 1;
			opponent_advance_route();
			goto select_route_target;
		}
		steering_target = -0x41;
	}
	if (state.opponentstate.car_sumSurfFrontWheels == 0)
		steering_target = 0;
	steering_delta = LEGACY_S16_WRAP_SUB(steering_target,
		state.opponentstate.car_steeringAngle);
	absolute_value = steering_delta;
	if (absolute_value < 0)
		absolute_value = LEGACY_S16_WRAP_NEGATE(absolute_value);
	if (absolute_value > steering_step) {
		if (steering_target < state.opponentstate.car_steeringAngle) {
			state.opponentstate.car_steeringAngle = LEGACY_S16_WRAP_SUB(
				state.opponentstate.car_steeringAngle, steering_step);
		} else {
			state.opponentstate.car_steeringAngle = LEGACY_S16_WRAP_ADD(
				state.opponentstate.car_steeringAngle, steering_step);
		}
	} else {
		state.opponentstate.car_steeringAngle = steering_target;
	}

choose_input:
	input = 0;
	if (state.opponentstate.car_sumSurfRearWheels != 0) {
		if (state.opponentstate.car_crashBmpFlag != 0) {
			input = 2;
		} else if (state.opponentstate.car_36MwhlAngle != 0) {
			speed_threshold = (legacy_u16)speed_step << 9;
			if (speed_threshold > state.opponentstate.car_speed2) {
				state.opponentstate.car_speed2 = 0;
				state.opponentstate.car_36MwhlAngle = 0;
			} else {
				state.opponentstate.car_speed2 = LEGACY_U16_WRAP_SUB(
					state.opponentstate.car_speed2, speed_threshold);
			}
		} else if (state.opponentstate.car_demandedGrip <=
			state.opponentstate.car_surfacegrip_sum) {
			target_speed = state.game_inputmode == 2 ? 0x4000U :
				(legacy_u16)(legacy_u8)state.field_3F9 << 8;
			if (LEGACY_U16_WRAP_SUB(target_speed, 0x100U) >
				state.opponentstate.car_speed) {
				input = 1;
			} else if (LEGACY_U16_WRAP_ADD(target_speed, 0x300U) <
				state.opponentstate.car_speed) {
				input = 2;
			}
		} else {
			input = 2;
		}
	}

	update_car_speed(input, 1, &state.opponentstate, &simd_opponent);
	update_grip(&state.opponentstate, &simd_opponent, 0);
	update_player_state(&state.opponentstate, &simd_opponent,
		&state.playerstate, &simd_player, 1);
	if (state.opponentstate.car_crashBmpFlag == 0) {
		relative.x = LEGACY_S16_WRAP_SUB(
			state.opponentstate.car_vec_unk3.x,
			opponent_position((legacy_s32)
				state.opponentstate.car_posWorld1.lx));
		relative.y = LEGACY_S16_WRAP_SUB(
			state.opponentstate.car_vec_unk3.y,
			opponent_position((legacy_s32)
				state.opponentstate.car_posWorld1.ly));
		relative.z = LEGACY_S16_WRAP_SUB(
			state.opponentstate.car_vec_unk3.z,
			opponent_position((legacy_s32)
				state.opponentstate.car_posWorld1.lz));
		rotation = mat_rot_zxy(state.opponentstate.car_rotate.z,
			state.opponentstate.car_rotate.y,
			state.opponentstate.car_rotate.x, 1);
		mat_mul_vector(&relative, rotation, &transformed);
		state.opponentstate.field_48 = LEGACY_S16_FROM_BITS(
			(legacy_u16)polarAngle(
				LEGACY_S16_WRAP_NEGATE(transformed.x),
				transformed.z) & 0x03FFU);
	}

	if (state.opponentstate.field_CD != 0) {
		finish_distance = multiply_and_scale(cos_fast(track_angle),
			LEGACY_S16_WRAP_SUB(trackcenterpos[startrow2],
				opponent_position((legacy_s32)
					state.opponentstate.car_posWorld1.lz)));
		finish_distance = LEGACY_S16_WRAP_ADD(finish_distance,
			multiply_and_scale(sin_fast(track_angle),
				LEGACY_S16_WRAP_SUB(trackcenterpos2[startcol2],
					opponent_position((legacy_s32)
						state.opponentstate.car_posWorld1.lx))));
		if (finish_distance < 0)
			update_crash_state(3, 1);
	}
}

void upd_statef20_from_steer_input(legacy_s8 steering_input) {
	legacy_s8* response_table;
	legacy_s16 steering_angle;
	legacy_s16 response;
	legacy_s16 centering_limit;
	legacy_s16 response_index;
	legacy_u8 speed_index;

	response_table = (legacy_s8*)steerWhlRespTable_ptr;
	steering_angle = state.playerstate.car_steeringAngle;
	speed_index = (legacy_u8)((state.playerstate.car_speed2 >> 10) & 0xFC);
	response_index = (legacy_s16)(speed_index + (legacy_s8)steering_input);
	response = response_table[response_index];

	/* Turning farther from center gets the original fourfold response. */
	if ((response > 0 && steering_angle < -1) ||
		(response < 0 && steering_angle > 1)) {
		response = (legacy_s16)(response * 4);
	}

	/* With no steering input, bring a moving car back toward center. */
	if (response == 0 && state.playerstate.car_speed2 != 0 &&
		steering_angle != 0) {
		centering_limit = (legacy_s16)(response_table[speed_index + 1] * 2);
		if (steering_angle < 0) {
			if ((legacy_s16)-steering_angle > centering_limit)
				response = centering_limit;
			else
				response = (legacy_s16)-steering_angle;
		} else {
			if (steering_angle > centering_limit)
				response = (legacy_s16)-centering_limit;
			else
				response = (legacy_s16)-steering_angle;
		}
	}

	if (framespersec == 10) {
		if (response > 0xA0)
			response = 0xA0;
		if (response < -0xA0)
			response = -0xA0;
	} else {
		if (response > 0x50)
			response = 0x50;
		if (response < -0x50)
			response = -0x50;
	}

	steering_angle = (legacy_s16)(steering_angle + response);
	if (steering_angle > 0xF0)
		steering_angle = 0xF0;
	if (steering_angle < -0xF0)
		steering_angle = -0xF0;

	if (response_table[response_index] == 0 &&
		steering_angle > -8 && steering_angle < 8) {
		steering_angle = 0;
	}

	state.playerstate.car_steeringAngle = steering_angle;
}

static legacy_s16 route_average(legacy_s16 first, legacy_s16 second) {
	legacy_s32 sum;

	sum = LEGACY_S32_WRAP_ADD(first, second);
	return LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S32_SAR(sum, 1U));
}

legacy_s16 sub_18D60(
	legacy_s16 track_index_arg,
	struct VECTOR* output,
	legacy_s16 route_index_arg,
	legacy_s16* optional_speed
) {
	struct TRACKOBJECT* track_object;
	struct TRKOBJINFO* track_info;
	struct VECTOR* route_vectors;
	struct VECTOR first_point;
	struct VECTOR second_point;
	legacy_s16 track_index;
	legacy_u16 packed_opponent_offset;
	legacy_u16 speed_index;
	legacy_u16 route_index_word;
	legacy_u8 tile_element;
	legacy_u8 track_subtype;
	legacy_u8 connection_status;
	legacy_u8 arrow_type;
	legacy_u8 route_index;
	legacy_u8 vector_index;
	legacy_u8 column;
	legacy_u8 row;
	legacy_u8 has_opponent_path;
	legacy_s16 base_position;
	legacy_s16 orientation;

	track_index = (legacy_s16)track_index_arg;
	tile_element = (legacy_u8)td17_trk_elem_ordered[track_index];
	track_subtype = (legacy_u8)trackdata18[track_index] & 0x0FU;
	connection_status = (legacy_u8)trackdata18[track_index] & 0x10U;
	track_object = &trkObjectList[tile_element];
	track_info = &track_object->ss_trkObjInfoPtr[track_subtype];
	arrow_type = (legacy_u8)track_info->si_arrowType;
	route_index = (legacy_u8)route_index_arg;

	if (connection_status == 0) {
		vector_index = (legacy_u8)(route_index * 2U);
	} else {
		vector_index = (legacy_u8)(arrow_type - route_index);
		vector_index = (legacy_u8)(vector_index * 2U);
		vector_index = (legacy_u8)(vector_index - 2U);
	}

	if (optional_speed != 0) {
		speed_index = (legacy_u8)track_info->si_oppSpedCode;
		speed_index = LEGACY_U16_WRAP_ADD(
			speed_index, (legacy_u8)track_object->ss_surfaceType);
		((legacy_u8*)optional_speed)[0] = oppnentSped[speed_index];
	}

	packed_opponent_offset = (legacy_u16)(
		(legacy_u8)track_info->si_opp1 |
		((legacy_u16)(legacy_u8)track_info->si_opp2 << 8));
	has_opponent_path = packed_opponent_offset != 0;
	if (connection_status != 0 && has_opponent_path != 0) {
		route_vectors = track_vector_from_legacy_offset(
			packed_opponent_offset);
	} else {
		route_vectors = (struct VECTOR*)track_info->si_cameraDataOffset;
	}

	if (connection_status != 0 && has_opponent_path == 0) {
		first_point = route_vectors[vector_index + 1];
		second_point = route_vectors[vector_index];
	} else {
		first_point = route_vectors[vector_index];
		second_point = route_vectors[vector_index + 1];
	}

	orientation = (legacy_s16)track_info->si_arrowOrient;
	if (orientation == 0x100) {
		base_position = first_point.x;
		first_point.x = first_point.z;
		first_point.z = LEGACY_S16_WRAP_NEGATE(base_position);
		base_position = second_point.x;
		second_point.x = second_point.z;
		second_point.z = LEGACY_S16_WRAP_NEGATE(base_position);
	} else if (orientation == 0x200) {
		first_point.x = LEGACY_S16_WRAP_NEGATE(first_point.x);
		first_point.z = LEGACY_S16_WRAP_NEGATE(first_point.z);
		second_point.x = LEGACY_S16_WRAP_NEGATE(second_point.x);
		second_point.z = LEGACY_S16_WRAP_NEGATE(second_point.z);
	} else if (orientation == 0x300) {
		base_position = first_point.x;
		first_point.x = LEGACY_S16_WRAP_NEGATE(first_point.z);
		first_point.z = base_position;
		base_position = second_point.x;
		second_point.x = LEGACY_S16_WRAP_NEGATE(second_point.z);
		second_point.z = base_position;
	}

	column = (legacy_u8)td21_col_from_path[track_index];
	row = (legacy_u8)td22_row_from_path[track_index];
	if (first_point.y != -1 &&
		td15_terr_map_main[terrainrows[row] + column] == 6) {
		first_point.y = LEGACY_S16_WRAP_ADD(
			first_point.y, hillHeightConsts[1]);
		second_point.y = LEGACY_S16_WRAP_ADD(
			second_point.y, hillHeightConsts[1]);
	}

	if (((legacy_u8)track_object->ss_multiTileFlag & 1U) != 0)
		base_position = (legacy_s16)trackpos[row];
	else
		base_position = (legacy_s16)trackcenterpos[row];
	first_point.z = LEGACY_S16_WRAP_ADD(first_point.z, base_position);
	second_point.z = LEGACY_S16_WRAP_ADD(second_point.z, base_position);

	if (((legacy_u8)track_object->ss_multiTileFlag & 2U) != 0)
		base_position = (legacy_s16)trackpos2[column + 1];
	else
		base_position = (legacy_s16)trackcenterpos2[column];
	first_point.x = LEGACY_S16_WRAP_ADD(first_point.x, base_position);
	second_point.x = LEGACY_S16_WRAP_ADD(second_point.x, base_position);

	output[0].x = route_average(first_point.x, second_point.x);
	if (first_point.y == -1)
		output[0].y = -1;
	else
		output[0].y = route_average(first_point.y, second_point.y);
	output[0].z = route_average(first_point.z, second_point.z);
	output[1] = first_point;
	output[2] = second_point;
	LEGACY_WRITE_U16_LE((legacy_u8*)output + 18, has_opponent_path);

	route_index_word = route_index;
	if ((route_index & 0x80U) != 0)
		route_index_word |= 0xFF00U;
	return LEGACY_U16_WRAP_SUB(arrow_type, 1U) == route_index_word;
}

#define TRACK_SETUP_TILE_COUNT 0x385U
#define TRACK_SETUP_BRANCH_COUNT 0x40U

enum TRACK_SETUP_ERROR {
	TRACK_SETUP_OK = 0,
	TRACK_SETUP_NO_START_FINISH = 1,
	TRACK_SETUP_INTERNAL_ERROR = 2,
	TRACK_SETUP_MANY_START_FINISH = 3,
	TRACK_SETUP_ELEMENT_MISMATCH = 4,
	TRACK_SETUP_WRONG_WAY = 5,
	TRACK_SETUP_MANY_ELEMENTS = 6,
	TRACK_SETUP_NO_PATH = 7,
	TRACK_SETUP_MANY_PATHS = 8,
	TRACK_SETUP_NO_RUNWAY = 9,
	TRACK_SETUP_LONG_JUMP = 10,
	TRACK_SETUP_TERRAIN_MISMATCH = 11
};

#pragma pack(push, 1)
struct TRACK_SETUP_BRANCH {
	legacy_s8 column;
	legacy_s8 row;
	legacy_u8 tile_element;
	legacy_u8 subtype;
	legacy_s8 connection_status;
	legacy_u8 runway_length;
	legacy_s8 previous_column;
	legacy_s8 previous_row;
	legacy_u8 previous_tile_element;
	legacy_u8 previous_subtype;
	legacy_s8 previous_connection_status;
	legacy_u8 previous_connection_code;
	legacy_s16 previous_piece;
};
#pragma pack(pop)

typedef legacy_s8 track_setup_branch_must_be_14_bytes[
	(sizeof(struct TRACK_SETUP_BRANCH) == 14) ? 1 : -1];

static legacy_s8 track_setup_add_s8(legacy_s8 value, legacy_s16 amount)
{
	return LEGACY_S8_FROM_BITS((legacy_u8)(
		(legacy_u8)value + (legacy_u8)amount));
}

static void track_setup_rotate_vector(
	struct VECTOR* vector,
	legacy_s16 orientation
) {
	legacy_s16 temporary;

	if (orientation == 0x100) {
		temporary = vector->x;
		vector->x = vector->z;
		vector->z = LEGACY_S16_WRAP_NEGATE(temporary);
	} else if (orientation == 0x200) {
		vector->x = LEGACY_S16_WRAP_NEGATE(vector->x);
		vector->z = LEGACY_S16_WRAP_NEGATE(vector->z);
	} else if (orientation == 0x300) {
		temporary = vector->x;
		vector->x = LEGACY_S16_WRAP_NEGATE(vector->z);
		vector->z = temporary;
	}
}

static void track_setup_link_piece(
	legacy_s16 source_piece,
	legacy_s16 destination_piece
) {
	if (td01_track_file_cpy[source_piece] == -1)
		td01_track_file_cpy[source_piece] = destination_piece;
	else
		td02_penalty_related[source_piece] = destination_piece;
}

legacy_s16 track_setup(void)
{
	struct TRACK_SETUP_BRANCH far* branches;
	struct TRACK_SETUP_BRANCH far* branch;
	struct TRACKOBJECT* track_object;
	struct TRACKOBJECT* previous_track_object;
	struct TRKOBJINFO* track_info;
	struct TRKOBJINFO* current_info;
	struct TRKOBJINFO* previous_info;
	struct VECTOR* camera_vectors;
	struct VECTOR camera_vector;
	legacy_s16 far* camera_height;
	legacy_s16 far* camera_unknown;
	legacy_u8 visited_tiles[904];
	legacy_u8 subtype_by_piece[902];
	legacy_s8 connection_by_piece[902];
	legacy_u16 branch_count;
	legacy_u16 block_index;
	legacy_u16 index;
	legacy_u16 sample_index;
	legacy_u16 camera_count;
	legacy_s16 previous_piece;
	legacy_s16 existing_piece;
	legacy_s16 sampled_piece;
	legacy_s16 tile_index;
	legacy_s16 camera_index;
	legacy_s16 orientation;
	legacy_s16 base_position;
	legacy_u16 opponent_path_offset;
	legacy_u8 tile_terrain;
	legacy_u8 tile_element;
	legacy_u8 tile_entry_point;
	legacy_u8 previous_connection_code;
	legacy_u8 subtype;
	legacy_s8 connection_status;
	legacy_s8 selected_connection_status;
	legacy_u8 previous_subtype;
	legacy_s8 previous_connection_status;
	legacy_u8 previous_tile_element;
	legacy_s8 previous_column;
	legacy_s8 previous_row;
	legacy_s8 column;
	legacy_s8 row;
	legacy_u8 start_finish_count;
	legacy_u8 runway_length;
	legacy_u8 jump_length;
	legacy_u8 path_closed;
	legacy_u8 match_count;
	legacy_u8 arrow_code;
	legacy_u8 error_code;

	branches = (struct TRACK_SETUP_BRANCH far*)
		mmgr_alloc_resbytes("tcomp", 0x380L);
	if (branches == 0)
		return 2;

	camera_height = (legacy_s16 far*)trackdata7;
	camera_unknown = (legacy_s16 far*)trackdata6;
	start_finish_count = 0;
	jump_length = 0;
	track_pieces_counter = 0;
	for (index = 0; index < TRACK_SETUP_TILE_COUNT; index++)
		trackdata19[index] = 0xFFU;

	for (row = 0; row < 0x1E; row++) {
		previous_connection_code = 0x63U;
		for (column = 0; column < 0x1E; column++) {
			tile_terrain = td15_terr_map_main[
				terrainrows[row] + column];
			if (terrConnDataEtoW[tile_terrain] !=
				previous_connection_code &&
				previous_connection_code != 0x63U) {
				error_code = TRACK_SETUP_TERRAIN_MISMATCH;
				goto track_error;
			}
			previous_connection_code = terrConnDataWtoE[tile_terrain];
		}
	}

	for (column = 0; column < 0x1E; column++) {
		previous_connection_code = 0x63U;
		for (row = 0; row < 0x1E; row++) {
			tile_terrain = td15_terr_map_main[
				terrainrows[row] + column];
			if (terrConnDataNtoS[tile_terrain] !=
				previous_connection_code &&
				previous_connection_code != 0x63U) {
				error_code = TRACK_SETUP_TERRAIN_MISMATCH;
				goto track_error;
			}
			previous_connection_code = terrConnDataStoN[tile_terrain];
		}
	}

	for (row = 0; row < 0x1E; row++) {
		for (column = 0; column < 0x1E; column++) {
			tile_index = trackrows[row] + column;
			tile_element = td14_elem_map_main[tile_index];
			if (tile_element >= 0xFDU)
				tile_element = 0;
			if (tile_element >= 0xB6U) {
				tile_element = 4;
				td14_elem_map_main[tile_index] = 4;
			}

			orientation = -1;
			if (tile_element == 1 || tile_element == 0x86U ||
				tile_element == 0x93U)
				orientation = 0;
			else if (tile_element == 0x87U ||
				tile_element == 0x94U || tile_element == 0xB3U)
				orientation = 0x200;
			else if (tile_element == 0x88U ||
				tile_element == 0x95U || tile_element == 0xB4U)
				orientation = 0x100;
			else if (tile_element == 0x89U ||
				tile_element == 0x96U || tile_element == 0xB5U)
				orientation = 0x300;

			if (orientation != -1) {
				track_angle = orientation;
				if (start_finish_count != 0) {
					error_code = TRACK_SETUP_MANY_START_FINISH;
					goto track_error;
				}
				startcol2 = column;
				startrow2 = row;
				tile_terrain = td15_terr_map_main[
					terrainrows[row] + column];
				hillFlag = tile_terrain == 6;
				start_finish_count++;
			}
		}
	}

	if (start_finish_count == 0) {
		error_code = TRACK_SETUP_NO_START_FINISH;
		goto track_error;
	}

	track_pieces_counter = 0;
	branch_count = 0;
	byte_45635 = 0;
	byte_4616E = 0;
	runway_length = 0;
	path_closed = 0;
	for (index = 0; index < TRACK_SETUP_TILE_COUNT; index++) {
		visited_tiles[index] = 0;
		td01_track_file_cpy[index] = -1;
		td02_penalty_related[index] = -1;
	}

	column = LEGACY_S8_FROM_BITS((legacy_u8)startcol2);
	row = LEGACY_S8_FROM_BITS((legacy_u8)startrow2);
	orientation = (legacy_s16)track_angle;
	previous_connection_code = 0;
	previous_piece = -1;

track_next_tile:
	match_count = 0;
	if (column < 0 || row < 0 || column > 0x1D || row > 0x1D)
		goto track_backtrack;

	tile_element = td14_elem_map_main[trackrows[row] + column];
	tile_terrain = td15_terr_map_main[terrainrows[row] + column];
	if (tile_element != 0 && tile_terrain >= 7U && tile_terrain < 0x0BU)
		tile_element = subst_hillroad_track(tile_terrain, tile_element);

	if (tile_element == 0xFDU) {
		column = track_setup_add_s8(column, -1);
		row = track_setup_add_s8(row, -1);
		if (orientation == 0)
			tile_entry_point = 0x0CU;
		else if (orientation == 0x300)
			tile_entry_point = 9;
		else
			tile_entry_point = 0;
		tile_element = td14_elem_map_main[trackrows[row] + column];
	} else if (tile_element == 0xFEU) {
		row = track_setup_add_s8(row, -1);
		if (orientation == 0)
			tile_entry_point = 0x0BU;
		else if (orientation == 0x100)
			tile_entry_point = 6;
		else if (orientation == 0x300)
			tile_entry_point = 7;
		else
			tile_entry_point = 0;
		tile_element = td14_elem_map_main[trackrows[row] + column];
	} else if (tile_element == 0xFFU) {
		column = track_setup_add_s8(column, -1);
		if (orientation == 0)
			tile_entry_point = 0x0AU;
		else if (orientation == 0x200)
			tile_entry_point = 5;
		else if (orientation == 0x300)
			tile_entry_point = 8;
		else
			tile_entry_point = 0;
		tile_element = td14_elem_map_main[trackrows[row] + column];
	} else {
		if (orientation == 0)
			tile_entry_point = 2;
		else if (orientation == 0x100)
			tile_entry_point = 4;
		else if (orientation == 0x200)
			tile_entry_point = 1;
		else if (orientation == 0x300)
			tile_entry_point = 3;
		else
			tile_entry_point = 0;
	}

	if (jump_length == 0 && tile_entry_point == 0) {
		error_code = TRACK_SETUP_INTERNAL_ERROR;
		goto track_error;
	}

	track_object = &trkObjectList[tile_element];
	track_info = track_object->ss_trkObjInfoPtr;
	if (track_info != 0) {
		for (block_index = 0;
			block_index < (legacy_u8)track_info->si_noOfBlocks;
			block_index++) {
			current_info = &track_info[block_index];
			connection_status = -1;
			if ((legacy_u8)current_info->si_entryPoint ==
				tile_entry_point) {
				if ((legacy_u8)current_info->si_entryType !=
					previous_connection_code) {
					error_code = TRACK_SETUP_ELEMENT_MISMATCH;
					goto track_error;
				}
				connection_status = 0;
			} else if ((legacy_u8)current_info->si_exitPoint ==
				tile_entry_point) {
				if ((legacy_u8)current_info->si_exitType !=
					previous_connection_code) {
					error_code = TRACK_SETUP_ELEMENT_MISMATCH;
					goto track_error;
				}
				connection_status = 1;
			}

			if (connection_status >= 0 &&
				visited_tiles[trackrows[row] + column] != 0) {
				for (existing_piece = 0;
					existing_piece < track_pieces_counter;
					existing_piece++) {
					if ((legacy_u8)td21_col_from_path[existing_piece] ==
						(legacy_u8)column &&
						(legacy_u8)td22_row_from_path[existing_piece] ==
						(legacy_u8)row &&
						subtype_by_piece[existing_piece] ==
						(legacy_u8)block_index &&
						connection_by_piece[existing_piece] ==
						connection_status) {
						connection_status = -1;
						track_setup_link_piece(
							previous_piece, existing_piece);
						if (existing_piece == 0)
							path_closed = 1;
						break;
					}
				}
			}

			if (connection_status >= 0) {
				if (match_count == 0) {
					subtype = (legacy_u8)block_index;
					selected_connection_status = connection_status;
				} else {
					if (branch_count == TRACK_SETUP_BRANCH_COUNT) {
						error_code = TRACK_SETUP_MANY_PATHS;
						goto track_error;
					}
					branch = &branches[branch_count];
					branch->column = column;
					branch->row = row;
					branch->tile_element = tile_element;
					branch->subtype = (legacy_u8)block_index;
					branch->connection_status = connection_status;
					branch->previous_connection_code =
						previous_connection_code;
					branch->previous_piece = previous_piece;
					branch->runway_length = runway_length;
					branch->previous_column = previous_column;
					branch->previous_row = previous_row;
					branch->previous_tile_element = previous_tile_element;
					branch->previous_subtype = previous_subtype;
					branch->previous_connection_status =
						previous_connection_status;
					branch_count++;
				}
				match_count++;
			}
		}
	}

	if (match_count != 0) {
		connection_status = selected_connection_status;
		goto track_record_piece;
	}

	if (previous_connection_code != 1)
		goto track_backtrack;
	if (jump_length >= 2)
		goto track_backtrack;
	if (runway_length < 2) {
		error_code = TRACK_SETUP_NO_RUNWAY;
		goto track_error;
	}
	runway_length++;
	jump_length++;
	if (orientation == 0) {
		column = previous_column;
		row = track_setup_add_s8(previous_row,
			-(legacy_s16)jump_length - 1);
	} else if (orientation == 0x100) {
		row = previous_row;
		column = track_setup_add_s8(previous_column,
			(legacy_s16)jump_length + 1);
	} else if (orientation == 0x200) {
		column = previous_column;
		row = track_setup_add_s8(previous_row,
			(legacy_s16)jump_length + 1);
	} else if (orientation == 0x300) {
		row = previous_row;
		column = track_setup_add_s8(previous_column,
			-(legacy_s16)jump_length - 1);
	}
	goto track_next_tile;

track_backtrack:
	if (branch_count == 0) {
		if (path_closed == 0) {
			error_code = TRACK_SETUP_NO_PATH;
			goto track_error;
		}
		goto track_build_cameras;
	}
	branch_count--;
	branch = &branches[branch_count];
	column = branch->column;
	row = branch->row;
	tile_element = branch->tile_element;
	subtype = branch->subtype;
	connection_status = branch->connection_status;
	previous_connection_code = branch->previous_connection_code;
	previous_piece = branch->previous_piece;
	runway_length = branch->runway_length;
	previous_column = branch->previous_column;
	previous_row = branch->previous_row;
	previous_tile_element = branch->previous_tile_element;
	previous_subtype = branch->previous_subtype;
	previous_connection_status = branch->previous_connection_status;
	if (jump_length > 1) {
		error_code = TRACK_SETUP_LONG_JUMP;
		goto track_error;
	}

track_record_piece:
	jump_length = 0;
	visited_tiles[trackrows[row] + column] = 1;
	subtype_by_piece[track_pieces_counter] = subtype;
	connection_by_piece[track_pieces_counter] = connection_status;
	if (previous_piece != -1)
		track_setup_link_piece(previous_piece, track_pieces_counter);
	previous_piece = (legacy_s16)track_pieces_counter;
	td21_col_from_path[track_pieces_counter] = column;
	td22_row_from_path[track_pieces_counter] = row;
	trackdata18[track_pieces_counter] = (legacy_u8)(
		((legacy_u8)connection_status << 4) + subtype);
	td17_trk_elem_ordered[track_pieces_counter] = tile_element;

	track_info = trkObjectList[tile_element].ss_trkObjInfoPtr;
	current_info = &track_info[subtype];
	arrow_code = (legacy_u8)current_info->si_opp3;
	if (arrow_code == 0) {
		runway_length++;
	} else {
		if (arrow_code != 0xFFU && runway_length > 3 &&
			byte_45635 != 0x30U) {
			previous_track_object = &trkObjectList[previous_tile_element];
			previous_info = &previous_track_object->
				ss_trkObjInfoPtr[previous_subtype];
			opponent_path_offset = (legacy_u16)(
				(legacy_u8)previous_info->si_opp1 |
				((legacy_u16)(legacy_u8)previous_info->si_opp2 << 8));
			if (previous_connection_status != 0 &&
				opponent_path_offset != 0)
				camera_vectors = track_vector_from_legacy_offset(
					opponent_path_offset);
			else
				camera_vectors = (struct VECTOR*)
					previous_info->si_cameraDataOffset;
			index = (legacy_u8)previous_info->si_arrowType * 2U;
			if (previous_connection_status != 0)
				index += 2U;
			else
				index += 1U;
			camera_vector = camera_vectors[index];
			if (connection_status != 0)
				arrow_code = byte_3E724[
					LEGACY_S8_FROM_BITS(arrow_code)];
			else
				arrow_code = byte_3E71E[
					LEGACY_S8_FROM_BITS(arrow_code)];
			orientation = (legacy_s16)previous_info->si_arrowOrient;
			track_setup_rotate_vector(&camera_vector, orientation);
			td08_direction_related[byte_45635] =
				previous_connection_status != 0 ?
				(orientation ^ 0x200) : orientation;
			trackdata23[byte_45635] = arrow_code;
			if (td15_terr_map_main[terrainrows[previous_row] +
				previous_column] == 6)
				camera_vector.y = LEGACY_S16_WRAP_ADD(
					camera_vector.y, 0x1C2);
			camera_index = (legacy_s16)byte_45635;
			td10_track_check_rel[camera_index * 3 + 1] =
				camera_vector.y;
			if (((legacy_u8)previous_track_object->ss_multiTileFlag &
				1U) != 0)
				base_position = (legacy_s16)trackpos[previous_row];
			else
				base_position = (legacy_s16)trackcenterpos[previous_row];
			td10_track_check_rel[camera_index * 3 + 2] =
				LEGACY_S16_WRAP_ADD(camera_vector.z, base_position);
			if (((legacy_u8)previous_track_object->ss_multiTileFlag &
				2U) != 0)
				base_position = (legacy_s16)
					trackpos2[(legacy_u8)previous_column + 1U];
			else
				base_position = (legacy_s16)
					trackcenterpos2[previous_column];
			td10_track_check_rel[camera_index * 3] =
				LEGACY_S16_WRAP_ADD(camera_vector.x, base_position);
			trackdata19[trackrows[previous_row] + previous_column] =
				byte_45635;
			byte_45635++;
		}
		runway_length = 0;
	}

	track_pieces_counter++;
	if (track_pieces_counter == TRACK_SETUP_TILE_COUNT) {
		error_code = TRACK_SETUP_MANY_ELEMENTS;
		goto track_error;
	}
	current_info = &track_info[subtype];
	if (connection_status != 0) {
		tile_entry_point = (legacy_u8)current_info->si_entryPoint;
		previous_connection_code = (legacy_u8)current_info->si_entryType;
	} else {
		tile_entry_point = (legacy_u8)current_info->si_exitPoint;
		previous_connection_code = (legacy_u8)current_info->si_exitType;
	}
	previous_column = column;
	previous_row = row;
	previous_connection_status = connection_status;
	previous_subtype = subtype;
	previous_tile_element = tile_element;

	switch (tile_entry_point) {
	case 1:
		row = track_setup_add_s8(row, -1);
		orientation = 0;
		break;
	case 2:
		row = track_setup_add_s8(row, 1);
		orientation = 0x200;
		break;
	case 3:
		column = track_setup_add_s8(column, 1);
		orientation = 0x100;
		break;
	case 4:
		column = track_setup_add_s8(column, -1);
		orientation = 0x300;
		break;
	case 5:
		row = track_setup_add_s8(row, -1);
		column = track_setup_add_s8(column, 1);
		orientation = 0;
		break;
	case 6:
		row = track_setup_add_s8(row, 1);
		column = track_setup_add_s8(column, -1);
		orientation = 0x300;
		break;
	case 7:
		column = track_setup_add_s8(column, 1);
		row = track_setup_add_s8(row, 1);
		orientation = 0x100;
		break;
	case 8:
		column = track_setup_add_s8(column, 2);
		orientation = 0x100;
		break;
	case 9:
		column = track_setup_add_s8(column, 2);
		row = track_setup_add_s8(row, 1);
		orientation = 0x100;
		break;
	case 10:
		column = track_setup_add_s8(column, 1);
		row = track_setup_add_s8(row, 1);
		orientation = 0x200;
		break;
	case 11:
		row = track_setup_add_s8(row, 2);
		orientation = 0x200;
		break;
	case 12:
		column = track_setup_add_s8(column, 1);
		row = track_setup_add_s8(row, 2);
		orientation = 0x200;
		break;
	}
	goto track_next_tile;

track_build_cameras:
	byte_45D90 = (legacy_u8)startcol2;
	byte_45E16 = (legacy_u8)startrow2;
	camera_count = (legacy_u16)LEGACY_S16_DIV_OR_ZERO(
		track_pieces_counter, 3);
	if (camera_count > 0x40U)
		camera_count = 0x40U;
	byte_4616E = (legacy_u8)camera_count;
	for (index = 0; index < TRACK_SETUP_TILE_COUNT; index++)
		subtype_by_piece[index] = 0;
	camera_index = 0;
	for (sample_index = 0; sample_index < byte_4616E; sample_index++) {
		sampled_piece = LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_S32_DIV_OR_ZERO(
				LEGACY_S32_WRAP_MUL(
					(legacy_s32)track_pieces_counter,
					(legacy_s32)sample_index),
				(legacy_s32)(legacy_u16)byte_4616E));
		column = LEGACY_S8_FROM_BITS(
			(legacy_u8)td21_col_from_path[sampled_piece]);
		row = LEGACY_S8_FROM_BITS(
			(legacy_u8)td22_row_from_path[sampled_piece]);
		tile_index = terrainrows[row] + column;
		if (subtype_by_piece[tile_index] != 0)
			continue;
		subtype_by_piece[tile_index] = 1;
		tile_element = (legacy_u8)td17_trk_elem_ordered[sampled_piece];
		subtype = (legacy_u8)trackdata18[sampled_piece] & 0x0FU;
		connection_status = ((legacy_u8)trackdata18[sampled_piece] &
			0x10U) != 0;
		track_object = &trkObjectList[tile_element];
		current_info = &track_object->ss_trkObjInfoPtr[subtype];
		opponent_path_offset = (legacy_u16)(
			(legacy_u8)current_info->si_opp1 |
			((legacy_u16)(legacy_u8)current_info->si_opp2 << 8));
		if (connection_status != 0 && opponent_path_offset != 0)
			camera_vectors = track_vector_from_legacy_offset(
				opponent_path_offset);
		else
			camera_vectors = (struct VECTOR*)
				current_info->si_cameraDataOffset;
		index = (legacy_u8)current_info->si_arrowType * 2U;
		camera_vector = camera_vectors[index];
		orientation = (legacy_s16)current_info->si_arrowOrient;
		track_setup_rotate_vector(&camera_vector, orientation);
		if (td15_terr_map_main[terrainrows[row] + column] == 6)
			camera_height[camera_index] = 0x1C2;
		else
			camera_height[camera_index] = 0;
		camera_unknown[camera_index] = 0;
		trackdata9[camera_index * 3 + 1] = LEGACY_S16_WRAP_ADD(
			camera_height[camera_index], camera_vector.y);
		if (((legacy_u8)track_object->ss_multiTileFlag & 1U) != 0)
			base_position = (legacy_s16)trackpos[row];
		else
			base_position = (legacy_s16)trackcenterpos[row];
		trackdata9[camera_index * 3 + 2] = LEGACY_S16_WRAP_ADD(
			base_position, camera_vector.z);
		if (((legacy_u8)track_object->ss_multiTileFlag & 2U) != 0)
			base_position = (legacy_s16)trackpos2[(legacy_u8)column + 1U];
		else
			base_position = (legacy_s16)trackcenterpos2[column];
		trackdata9[camera_index * 3] = LEGACY_S16_WRAP_ADD(
			base_position, camera_vector.x);
		camera_index++;
	}
	byte_4616E = (legacy_u8)camera_index;
	error_code = TRACK_SETUP_OK;
	goto track_done;

track_error:
	if (column == -1)
		column = 0;
	else if (column == 0x1E)
		column = 0x1D;
	if (row == -1)
		row = 0;
	else if (row == 0x1E)
		row = 0x1D;
	byte_45D90 = (legacy_u8)column;
	byte_45E16 = (legacy_u8)row;

track_done:
	mmgr_release((legacy_s8 far*)branches);
	return error_code;
}

void init_plantrak(void) {
	legacy_s16 path_z;
	legacy_s16 route_track_index;
	legacy_u16 route_table_offset;
	legacy_u8 route_index;

	init_game_state(-3);
	state.game_inputmode = 2;
	planptr = &plan_memres;
	startcol2 = 1;
	startrow2 = 0x1C;

	td17_trk_elem_ordered[0] = 7;
	td17_trk_elem_ordered[1] = 6;
	td17_trk_elem_ordered[2] = 8;
	td17_trk_elem_ordered[3] = 9;
	td17_trk_elem_ordered[4] = 7;

	td21_col_from_path[0] = 1;
	td21_col_from_path[1] = 0;
	td21_col_from_path[2] = 0;
	td21_col_from_path[3] = 1;
	td21_col_from_path[4] = 1;

	td22_row_from_path[0] = startrow2;
	td22_row_from_path[1] = startrow2;
	td22_row_from_path[2] = (legacy_u8)startrow2 + 1U;
	td22_row_from_path[3] = (legacy_u8)startrow2 + 1U;
	td22_row_from_path[4] = startrow2;

	trackdata18[0] = 0;
	trackdata18[1] = 0;
	trackdata18[2] = 0;
	trackdata18[3] = 0;
	trackdata18[4] = 0;

	trackdata3[0x00] = 0; trackdata3[0x01] = 0;
	trackdata3[0x02] = 1; trackdata3[0x03] = 0;
	trackdata3[0x04] = 2; trackdata3[0x05] = 0;
	trackdata3[0x06] = 3; trackdata3[0x07] = 0;
	trackdata3[0x08] = 4; trackdata3[0x09] = 0;
	trackdata3[0x0A] = 1; trackdata3[0x0B] = 0;
	trackdata3[0x0C] = 2; trackdata3[0x0D] = 0;
	trackdata3[0x0E] = 3; trackdata3[0x0F] = 0;
	trackdata3[0x10] = 4; trackdata3[0x11] = 0;
	trackdata3[0x12] = 1; trackdata3[0x13] = 0;
	trackdata3[0x14] = 2; trackdata3[0x15] = 0;
	trackdata3[0x16] = 3; trackdata3[0x17] = 0;
	trackdata3[0x18] = 4; trackdata3[0x19] = 0;
	trackdata3[0x1A] = 0; trackdata3[0x1B] = 0;
	trackdata3[0x1C] = 1; trackdata3[0x1D] = 0;
	trackdata3[0x1E] = 2; trackdata3[0x1F] = 0;
	trackdata3[0x20] = 3; trackdata3[0x21] = 0;
	trackdata3[0x22] = 0; trackdata3[0x23] = 0;

	oppnentSped[0] = 0xC8;
	path_z = LEGACY_S16_WRAP_ADD(trackpos[0x1C], 0x012E);
	init_carstate_from_simd(
		&state.opponentstate,
		&simd_opponent,
		1,
		(legacy_s32)0x00017700L,
		0L,
		(legacy_s32)((legacy_s32)path_z * (legacy_s32)64),
		0);

	route_index = (legacy_u8)state.opponentstate.field_CE;
	state.opponentstate.field_CE = (legacy_u8)(route_index + 1U);
	route_table_offset = LEGACY_U16_WRAP_MUL(
		state.opponentstate.car_trackdata3_index, 2U);
	route_track_index = LEGACY_S16_FROM_BITS(
		(legacy_u16)(legacy_u8)trackdata3[route_table_offset] |
		((legacy_u16)(legacy_u8)trackdata3[
			LEGACY_U16_WRAP_ADD(route_table_offset, 1U)] << 8));
	sub_18D60(
		route_track_index,
		&state.opponentstate.car_vec_unk3,
		(legacy_s16)route_index,
		(legacy_s16*)&state.field_3F9);
}

void player_op(legacy_s8 arg_carInputByte) {
	struct VECTOR var_38;
	struct VECTOR var_32;
	struct VECTOR var_28;
	struct VECTOR var_1A[4];
	struct VECTOR var_52[4];
	struct MATRIX* var_matptr;
	legacy_s8 var_3A;
	legacy_s8 var_1C;
	legacy_s8 var_2A;
	legacy_s8 var_2C;
	legacy_s16 var_2;
	legacy_s16 var_1EpenaltyCounter;
	legacy_u16 var_speedBeforeGrip;
	legacy_u16 var_speed2BeforeGrip;
	legacy_s16 si;

	//return ported_player_op_(arg_carInputByte);

	if (show_penalty_counter != 0) {
		show_penalty_counter--;
	}

	state.playerstate.field_CF = 1;
	if (state.playerstate.car_crashBmpFlag != 0) {
		state.field_45D = 0;
		arg_carInputByte = 2;
		
		if (state.playerstate.car_speed2 == 0) {
			state.playerstate.field_CF = 0;
			
			if (state.playerstate.car_speed == 0 && state.playerstate.car_rc1[0] == 0 && state.playerstate.car_rc1[1] == 0 && state.playerstate.car_rc1[2] == 0 && state.playerstate.car_rc1[3] == 0) {
				return ;
			}
		}
	}

	update_car_speed(arg_carInputByte, 0, &state.playerstate, &simd_player);
	legacy_execution_residue.grip_stack_words[0] =
		state.playerstate.car_lastrpm;
	legacy_execution_residue.grip_stack_words[1] =
		(legacy_s16)state.playerstate.car_speed;
	legacy_execution_residue.grip_stack_words[2] =
		(legacy_s16)state.playerstate.car_gearratio;
	upd_statef20_from_steer_input((arg_carInputByte >> 2) & 3);
	var_speedBeforeGrip = state.playerstate.car_speed;
	var_speed2BeforeGrip = state.playerstate.car_speed2;
	update_grip(&state.playerstate, &simd_player, 1);
	update_legacy_grip_stack_words(
		&state.playerstate,
		&simd_player,
		var_speedBeforeGrip,
		var_speed2BeforeGrip
	);
	update_player_state(&state.playerstate, &simd_player, &state.opponentstate, &simd_opponent, 0);
	state.game_travDist += state.playerstate.car_speed2;
	var_1C = state.field_45B;
	var_2 = state.field_2F2;
	si = detect_penalty(&var_2, &var_1EpenaltyCounter);
	if (si != 0)
		goto loc_172CB;
	goto loc_173B3;
loc_172CB:
	if (var_1EpenaltyCounter != -2)
		goto loc_172D8;
	state.field_45B = 1;
	goto loc_172E4;
loc_172D8:
	if (state.field_45B != 1)
		goto loc_172E9;
	state.field_45B = 0;
loc_172E4:
	state.field_45C = 0;
loc_172E9:
	if (state.field_45B == 0)
		goto loc_172F3;
	goto loc_173AD;
loc_172F3:
	if (var_2 != 0)
		goto loc_17308;
	if (state.field_2F4 == 0)
		goto loc_17308;
	state.playerstate.field_CD++;
	goto loc_1737B;
loc_17308:
	if (var_1EpenaltyCounter < 0)
		goto loc_17322;
	if (var_1EpenaltyCounter >= 3)
		goto loc_17322;
	state.field_45C = 0;
	state.field_2F2 = var_2;
	goto loc_173AD;
loc_17322:
	if (var_1EpenaltyCounter == -1)//0xFFFF)
		goto loc_1732E;
	if (var_1EpenaltyCounter <= 3)
		goto loc_173AD;
	
loc_1732E:
	if (td01_track_file_cpy[state.field_2F4] == var_2)
		goto loc_17349;
	if (td02_penalty_related[state.field_2F4] != var_2)
		goto loc_17350;
loc_17349:
	state.field_45C++;
	goto loc_17374;
loc_17350:
	if (td01_track_file_cpy[var_2] == state.field_2F4)
		goto loc_1736A;
	if (td02_penalty_related[var_2] != state.field_2F4)
		goto loc_1736F;
loc_1736A:
    state.field_45B = 2;
loc_1736F:
    state.field_45C = 1;
loc_17374:
	if (state.field_45C < 3)
		goto loc_173AD;
loc_1737B:
	state.field_2F2 = var_2;
	state.field_45C = 0;
	if (var_1EpenaltyCounter <= 0)
		goto loc_173AD;
		
	penalty_time = var_1EpenaltyCounter * framespersec * 3;
	show_penalty_counter = framespersec << 2;
	state.game_penalty += penalty_time;
	
loc_173AD:
	state.field_2F4 = var_2;
loc_173B3:
	state.field_45D = 0;
	if (state.field_45B != 1)
		goto loc_173C2;
	goto loc_17810;
loc_173C2:
	var_matptr = mat_rot_zxy(state.playerstate.car_rotate.z, state.playerstate.car_rotate.y, state.playerstate.car_rotate.x, 1);
	if (state.field_45B != 2)
		goto loc_173F6;
	if (state.playerstate.car_crashBmpFlag != 0)
		goto loc_173F0;
	state.field_45D = 3;
loc_173F0:
	var_2 = state.field_2F4;
	goto loc_174C9;
loc_173F6:
	if (state.playerstate.car_trackdata3_index != -1)
		goto loc_17402;
loc_173FD:
	si = 0;
	goto loc_174B3;
loc_17402:
	if (var_1C == 0)
		goto loc_1740F;
	if (state.field_45B == 0)
		goto loc_17431;
loc_1740F:
	if (state.playerstate.car_trackdata3_index == state.field_2F2)
		goto loc_1743A;
	if (td01_track_file_cpy[state.field_2F2] == state.playerstate.car_trackdata3_index)
		goto loc_1743A;
	if (td02_penalty_related[state.field_2F2] == state.playerstate.car_trackdata3_index)
		goto loc_1743A;
loc_17431:
	state.playerstate.car_trackdata3_index = -1;
	goto loc_173FD;
loc_1743A:
	var_32.x = state.playerstate.car_vec_unk3.x - (state.playerstate.car_posWorld1.lx >> 6);
	if (state.playerstate.car_vec_unk3.y == -1)
		goto loc_1747C;
	var_32.y = state.playerstate.car_vec_unk3.y - (state.playerstate.car_posWorld1.ly >> 6);
	goto loc_17481;
loc_1747C:
    var_32.y = 0;
loc_17481:
	var_32.z = state.playerstate.car_vec_unk3.z - (state.playerstate.car_posWorld1.lz >> 6);

	mat_mul_vector(&var_32, var_matptr, &var_38);
	si = var_38.z;
loc_174B3:
	if (si < 0x113)
		goto loc_174BC;
	goto loc_17699;
loc_174BC:
	if (state.playerstate.car_trackdata3_index == -1)
		goto loc_174C6;
	goto loc_1764C;
loc_174C6:
	var_2 = state.field_2F2;
loc_174C9:
	if (td02_penalty_related[var_2] == -1)
		goto loc_174DD;
	goto loc_17771;
loc_174DD:
    var_2A = 0;
    var_2C = 0;
loc_174E5:
	var_2A = sub_18D60(var_2, &state.playerstate.car_vec_unk3, var_2C, 0);
	var_28 = state.playerstate.car_vec_unk3;
	var_28.x -= state.playerstate.car_posWorld1.lx >> 6;
	if (var_28.y != -1)
		goto loc_1753E;
	var_28.y = -(state.playerstate.car_posWorld1.ly >> 6);
	goto loc_17552;
loc_1753E:
	var_28.y -= state.playerstate.car_posWorld1.ly >> 6;
loc_17552:
	var_28.z -= state.playerstate.car_posWorld1.lz >> 6;
	mat_mul_vector(&var_28, var_matptr, &var_38);
	if (var_2C == 0)
		goto loc_1758D;
	if (var_38.z >= var_32.z)
		goto loc_17599;
	if (var_38.z <= 0)
		goto loc_17599;
loc_1758D:
	var_3A = var_2C;
	var_32.z = var_38.z;
loc_17599:
	var_2C++;
	if (var_2A != 0)
		goto loc_175A5;
	goto loc_174E5;
loc_175A5:
	if (state.field_45B == 2)
		goto loc_175AF;
	goto loc_17640;
loc_175AF:
	if (var_3A != 0)
		goto loc_175D0;
	sub_18D60(var_2, &var_52, 0, 0);
	
	sub_18D60(var_2, &var_1A, 1, 0);
	goto loc_175F0;
loc_175D0:
	sub_18D60(var_2, &var_52, var_3A - 1, 0);

	sub_18D60(var_2, &var_1A, var_3A, 0);
loc_175F0:

	si = (state.playerstate.car_rotate.x - polarAngle(var_52[0].x - var_1A[0].x, var_1A[0].z - var_52[0].z) & 0x3FF) & 0x3FF;
	if (si > 0x380)
		goto loc_17631;
	if (si >= 0x80)
		goto loc_1764C;
loc_17631:
	state.field_45B = 0;
	state.field_45C = 1;
	state.playerstate.car_trackdata3_index = var_2;
	goto loc_17643;
loc_17640:
	state.playerstate.car_trackdata3_index = state.field_2F2;
loc_17643:
	state.playerstate.field_CE = var_3A;
loc_1764C:
	// NOTE: note the ++
	if (sub_18D60(state.playerstate.car_trackdata3_index, &state.playerstate.car_vec_unk3, state.playerstate.field_CE++, 0) == 0)
		goto loc_17699;
	if (td02_penalty_related[state.field_2F2] == -1)
		goto loc_17684;
	state.playerstate.car_trackdata3_index = -1;
	goto loc_17694;
loc_17684:
	state.playerstate.car_trackdata3_index = td01_track_file_cpy[state.field_2F2];
loc_17694:
	state.playerstate.field_CE = 0;
loc_17699:
	var_28 = state.playerstate.car_vec_unk3;
	if (state.playerstate.car_trackdata3_index != -1)
		goto loc_176B0;
	goto loc_17771;
loc_176B0:
	if (state.field_45B == 0)
		goto loc_176BA;
	goto loc_17771;
loc_176BA:
	var_28.x -= (state.playerstate.car_posWorld1.lx >> 6);
	if (var_28.y != -1)
		goto loc_176DC;
	var_28.y = 0;
	goto loc_176F0;
loc_176DC:
	var_28.y -= state.playerstate.car_posWorld1.ly >> 6;
loc_176F0:
	var_28.z -= state.playerstate.car_posWorld1.lz >> 6;
	var_matptr = mat_rot_zxy(state.playerstate.car_rotate.z, state.playerstate.car_rotate.y, state.playerstate.car_rotate.x, 1);
	mat_mul_vector(&var_28, var_matptr, &var_38);
	state.playerstate.field_48 = polarAngle(-var_38.x, var_38.z) & 0x3FF;
	if (state.playerstate.car_crashBmpFlag != 0)
		goto loc_17771;

	if (((state.playerstate.field_48 + 0x80) & 0x3FF) >> 8 == 1)
		goto loc_1776C;
	if (((state.playerstate.field_48 + 0x80) & 0x3FF) >> 8 == 3)
		goto loc_1779E;

loc_17764:
	state.field_45D = 0;
	goto loc_17771;
loc_1776C:
	state.field_45D = 1;
loc_17771:
	if (state.playerstate.field_CD != 0)
		goto loc_1777B;
	goto loc_17810;
loc_1777B:
	si = multiply_and_scale(cos_fast(track_angle), trackcenterpos[startrow2] - (state.playerstate.car_posWorld1.lz >> 6));
	goto loc_177AC;

loc_1779E:
	if (state.playerstate.field_B6 != 0)
		goto loc_17764;
	state.field_45D = 2;
	goto loc_17771;
loc_177AC:
	si += multiply_and_scale(sin_fast(track_angle), trackcenterpos2[startcol2] - (state.playerstate.car_posWorld1.lx >> 6));
	
	if (si >= 0)
		goto loc_17810;
	update_crash_state(3, 0);
loc_17810:
}
