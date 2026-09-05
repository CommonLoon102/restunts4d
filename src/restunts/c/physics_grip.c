#include "state_internal.h"

#define TRACK_GRID_LAST_COORDINATE 29
#define TRACK_WORLD_TILE_SHIFT 16U
#define PENALTY_ROUTE_VISITED_CAPACITY 904U
#define PENALTY_ROUTE_PENDING_CAPACITY 128U
#define PENALTY_ROUTE_SENTINEL (-1)
#define PENALTY_ROUTE_OUTSIDE_TRACK (-2)
#define PENALTY_ROUTE_START_TRACK_INDEX 900U
#define PENALTY_ROUTE_START_TILE_INDEX 11999U
#define PENALTY_ROUTE_START_COLUMN_INDEX 1963U
#define MULTI_TILE_ROW_FLAG 1U
#define MULTI_TILE_COLUMN_FLAG 2U
#define TRACK_START_FINISH_PIECE_INDEX 0
#define PENALTY_DISTANCE_FINISH_REACHED (-1)
#define PENALTY_NOT_DETECTED 0
#define PENALTY_DETECTED 1

static legacy_s16 penalty_route_next(legacy_s16 track_index)
{
	if (track_index == PENALTY_ROUTE_SENTINEL)
		return legacy_execution_residue.penalty_route_word;
	if (track_index < 0 || track_index >= track_pieces_counter)
		return PENALTY_ROUTE_SENTINEL;
	return td01_track_file_cpy[track_index];
}

static legacy_s16 penalty_route_alternate(legacy_s16 track_index)
{
	if (track_index == PENALTY_ROUTE_SENTINEL)
		return td01_track_file_cpy[PENALTY_ROUTE_START_TRACK_INDEX];
	return td02_penalty_related[track_index];
}

static legacy_s16 finish_penalty_route(legacy_s16* current_track,
	legacy_s16* penalty_count, legacy_s16 best_track,
	legacy_s16 best_distance, legacy_s16 column, legacy_s16 row)
{
	if (best_distance != 0) {
		*current_track = best_track;
		*penalty_count = best_distance;
	} else {
		state.game_startcol = column;
		state.game_startcol2 = column;
		state.game_startrow = row;
		state.game_startrow2 = row;
		*penalty_count = PENALTY_ROUTE_OUTSIDE_TRACK;
	}
	return PENALTY_DETECTED;
}

legacy_s16 detect_penalty(legacy_s16* current_track, legacy_s16* penalty_count)
{
	legacy_u8 visited[PENALTY_ROUTE_VISITED_CAPACITY];
	legacy_s16 pending_track[PENALTY_ROUTE_PENDING_CAPACITY];
	legacy_s16 pending_distance[PENALTY_ROUTE_PENDING_CAPACITY];
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
		(legacy_u8)((legacy_u32)state.playerstate.car_posWorld1.lx >>
			TRACK_WORLD_TILE_SHIFT));
	row = LEGACY_S8_FROM_BITS(LEGACY_U8_WRAP_SUB(
		TRACK_GRID_LAST_COORDINATE,
		(legacy_u8)((legacy_u32)state.playerstate.car_posWorld1.lz >>
			TRACK_WORLD_TILE_SHIFT)));
	if ((column == state.game_startcol || column == state.game_startcol2) &&
		(row == state.game_startrow || row == state.game_startrow2)) {
		*penalty_count = 0;
		return PENALTY_NOT_DETECTED;
	}
	if (column < 0 || column > TRACK_GRID_LAST_COORDINATE || row < 0 ||
		row > TRACK_GRID_LAST_COORDINATE) {
		*penalty_count = PENALTY_ROUTE_OUTSIDE_TRACK;
		return PENALTY_DETECTED;
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
		if (next_track == PENALTY_ROUTE_SENTINEL) {
			if (sentinel_visited == 0) {
				sentinel_visited = 1;
			} else if (pending_count != 0) {
				pending_count = LEGACY_U16_WRAP_SUB(pending_count, 1U);
				track_index = pending_track[pending_count];
				distance = pending_distance[pending_count];
				continue;
			} else {
				return finish_penalty_route(current_track, penalty_count,
					best_track, best_distance, column, row);
			}
		} else if (next_track < 0 ||
			next_track >= track_pieces_counter ||
			visited[next_track] != 0) {
			if (pending_count != 0) {
				pending_count = LEGACY_U16_WRAP_SUB(pending_count, 1U);
				track_index = pending_track[pending_count];
				distance = pending_distance[pending_count];
				continue;
			}
			return finish_penalty_route(current_track, penalty_count,
				best_track, best_distance, column, row);
		} else {
			visited[next_track] = 1;
		}

		if (next_track == PENALTY_ROUTE_SENTINEL) {
			minimum_row = (legacy_u8)td21_col_from_path[
				PENALTY_ROUTE_START_TRACK_INDEX];
			tile_element = (legacy_u8)td16_rpl_buffer[
				PENALTY_ROUTE_START_TILE_INDEX];
		} else {
			minimum_row = (legacy_u8)td22_row_from_path[next_track];
			tile_element = (legacy_u8)td17_trk_elem_ordered[next_track];
		}
		multi_tile_flags = trkObjectList[tile_element].ss_multiTileFlag;
		maximum_row = minimum_row;
		if ((multi_tile_flags & MULTI_TILE_ROW_FLAG) != 0)
			maximum_row = LEGACY_U8_WRAP_ADD(maximum_row, 1U);
		if (next_track == PENALTY_ROUTE_SENTINEL)
			minimum_column = (legacy_u8)td20_trk_file_appnd[
				PENALTY_ROUTE_START_COLUMN_INDEX];
		else
			minimum_column = (legacy_u8)td21_col_from_path[next_track];
		maximum_column = minimum_column;
		if ((multi_tile_flags & MULTI_TILE_COLUMN_FLAG) != 0)
			maximum_column = LEGACY_U8_WRAP_ADD(maximum_column, 1U);

		if (((legacy_u8)column == minimum_column ||
			(legacy_u8)column == maximum_column) &&
			((legacy_u8)row == minimum_row ||
			(legacy_u8)row == maximum_row)) {
			if (penalty_route_alternate(track_index) !=
				PENALTY_ROUTE_SENTINEL)
				next_track = track_index;
			state.game_startcol = LEGACY_S8_FROM_BITS(minimum_column);
			state.game_startcol2 = LEGACY_S8_FROM_BITS(maximum_column);
			state.game_startrow = LEGACY_S8_FROM_BITS(minimum_row);
			state.game_startrow2 = LEGACY_S8_FROM_BITS(maximum_row);
			if (distance <= 0) {
				*current_track = next_track;
				*penalty_count = distance;
				return PENALTY_DETECTED;
			}
			if (best_distance == 0 || best_distance > distance) {
				best_track = next_track;
				best_distance = distance;
			}
		}

		alternate_track = penalty_route_alternate(track_index);
		if (alternate_track != PENALTY_ROUTE_SENTINEL) {
			pending_distance[pending_count] = distance;
			pending_track[pending_count] = alternate_track;
			pending_count = LEGACY_U16_WRAP_ADD(pending_count, 1U);
		}
		if (next_track == TRACK_START_FINISH_PIECE_INDEX) {
			distance = PENALTY_DISTANCE_FINISH_REACHED;
		} else if (distance != PENALTY_DISTANCE_FINISH_REACHED) {
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
void update_legacy_grip_stack_words(
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
	combined_grip_operand = LEGACY_S16_SHL(simd->grip, 1U);
	sliding_sum = 0;
	sliding_values = &simd->sliding;
	for (i = 0; i < 4; i++) {
		sliding_sum = LEGACY_S16_WRAP_ADD(
			sliding_sum,
			sliding_values[(legacy_u8)carstate->car_surfaceWhl[i]]);
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
			grass_wheels = LEGACY_S16_WRAP_ADD(grass_wheels, 1);
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
	speed_squared = LEGACY_U32_WRAP_MUL(
		(legacy_u32)speed_shr8, (legacy_u32)speed_shr8);
	scaled_combined_grip = LEGACY_S32_WRAP_MUL(
		(legacy_s32)carstate->car_surfacegrip_sum, 0x100L);
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
			grass_wheels = LEGACY_U16_WRAP_ADD(grass_wheels, 1U);
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
	absolute_angle = absolute_word(adjusted_angle);
	angle_factor = LEGACY_S16_SAR(absolute_angle, 3U);
	square_low = LEGACY_U16_WRAP_MUL(speed_shr8, speed_shr8);
	square_low = (legacy_u16)(square_low >> 6);
	demanded_grip = LEGACY_U16_WRAP_MUL(square_low, angle_factor);

	combined_grip = LEGACY_S16_SHL(simd->grip, 1U);
	sliding_sum = 0;
	sliding_values = &simd->sliding;
	for (i = 0; i < 4U; i++) {
		sliding_sum = LEGACY_S16_WRAP_ADD(sliding_sum,
			sliding_values[(legacy_u8)carstate->car_surfaceWhl[i]]);
	}
	product = LEGACY_S32_WRAP_MUL(
		(legacy_s32)combined_grip, (legacy_s32)sliding_sum);
	combined_grip = LEGACY_S16_FROM_BITS(
		(legacy_u16)LEGACY_S32_SAR(product, 10U));
	carstate->car_demandedGrip = LEGACY_S16_FROM_BITS(demanded_grip);
	carstate->car_surfacegrip_sum = combined_grip;

	if (player_behavior == 0) {
		carstate->car_40MfrontWhlAngle = LEGACY_S16_SHL(
			carstate->car_steeringAngle, 2U);
		if (carstate->car_angle_z != 0) {
			carstate->car_angle_z = LEGACY_S16_SAR(
				LEGACY_S16_WRAP_MUL(carstate->car_angle_z, 15), 4U);
		}
	}

	if (player_behavior != 0) {
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
		numerator = LEGACY_S32_WRAP_MUL(
			(legacy_s32)combined_grip, 0x100L);
		denominator = LEGACY_S32_WRAP_MUL(
			(legacy_s32)speed_shr8, (legacy_s32)speed_shr8);
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
			tile_x = LEGACY_U8_WRAP_SUB(tile_x, 1U);
			tile_z = LEGACY_U8_WRAP_ADD(tile_z, 1U);
		} else if (track == 0xFEU) {
			tile_z = LEGACY_U8_WRAP_ADD(tile_z, 1U);
		} else if (track == 0xFFU) {
			tile_x = LEGACY_U8_WRAP_SUB(tile_x, 1U);
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
	}

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
		penalty = LEGACY_S16_SHL(absolute_angle, 1U);
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
