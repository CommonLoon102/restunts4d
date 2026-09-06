#include "state_internal.h"

#define TRACK_GRID_LAST_COORDINATE 29
#define TRACK_WORLD_TILE_SHIFT 16U
#define PENALTY_ROUTE_VISITED_CAPACITY 904U
#define PENALTY_ROUTE_PENDING_CAPACITY 128U
#define PENALTY_ROUTE_SENTINEL (-1)
#define PENALTY_ROUTE_START_TRACK_INDEX 900U
#define PENALTY_ROUTE_START_TILE_INDEX 11999U
#define PENALTY_ROUTE_START_COLUMN_INDEX 1963U
#define PENALTY_ROUTE_DISTANCE_NONE 0
#define PENALTY_ROUTE_DISTANCE_STEP 1
#define PENALTY_ROUTE_PENDING_NONE 0U
#define PENALTY_ROUTE_PENDING_STEP 1U
#define PENALTY_ROUTE_SENTINEL_UNVISITED 0U
#define PENALTY_ROUTE_SENTINEL_VISITED 1U
#define PENALTY_ROUTE_TRACK_UNVISITED 0U
#define PENALTY_ROUTE_TRACK_VISITED 1U
#define PENALTY_ROUTE_INDEX_FIRST 0U
#define MULTI_TILE_ROW_FLAG 1U
#define MULTI_TILE_COLUMN_FLAG 2U
#define TRACK_START_FINISH_PIECE_INDEX 0
#define PENALTY_NOT_DETECTED 0
#define PENALTY_DETECTED 1
#define LEGACY_GRIP_STACK_SI_VALUE 80
#define CAR_WHEEL_COUNT 4U
#define CAR_WHEEL_INDEX_FIRST 0
#define GRASS_WHEEL_COUNT_NONE 0
#define GRASS_WHEEL_COUNT_STEP 1
#define SURFACE_PAVED 1
#define SURFACE_GRASS 4
#define CAR_SPEED_INTEGER_SHIFT 8U
#define GRIP_FIXED_SCALE 256L
#define LEGACY_LONG_HIGH_WORD_SHIFT 16U
#define DEMANDED_GRIP_ANGLE_SHIFT 3U
#define DEMANDED_GRIP_SPEED_SQUARE_SHIFT 6U
#define COMBINED_GRIP_PRODUCT_SHIFT 10U
#define FRONT_WHEEL_ANGLE_SHIFT 2U
#define ROTATION_DAMPING_NUMERATOR 15
#define ROTATION_DAMPING_SHIFT 4U
#define ROTATION_RECENTER_THRESHOLD 8
#define SLIDE_ANGLE_WEIGHT 3
#define SLIDE_ANGLE_BLEND_SHIFT 2U
#define SLIDE_ANGLE_DECAY_SHIFT 4U
#define SLIDE_ANGLE_DECAY_THRESHOLD 16
#define CAR_CRASHED_FLAG 1
#define BANK_EFFECT_ROTATION_THRESHOLD 4
#define TRACK_CONTINUATION_NORTHWEST 253U
#define TRACK_CONTINUATION_NORTH 254U
#define TRACK_CONTINUATION_WEST 255U
#define BANKED_TRACK_FIRST 52U
#define BANKED_TRACK_LAST 55U
#define BANKED_TRACK_TILT_DIVISOR 5
#define SLIDE_CORRECTION_DIVISOR 14
#define SLIDE_GRIP_TOLERANCE 1000U
#define SLIDE_YAW_DAMPING_DIVISOR 2
#define SLIDE_SPEED_PENALTY_SHIFT 1U
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
	if (best_distance != PENALTY_ROUTE_DISTANCE_NONE) {
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
		*penalty_count = PENALTY_ROUTE_DISTANCE_NONE;
		return PENALTY_NOT_DETECTED;
	}
	if (column < 0 || column > TRACK_GRID_LAST_COORDINATE || row < 0 ||
		row > TRACK_GRID_LAST_COORDINATE) {
		*penalty_count = PENALTY_ROUTE_OUTSIDE_TRACK;
		return PENALTY_DETECTED;
	}

	best_distance = PENALTY_ROUTE_DISTANCE_NONE;
	best_track = TRACK_START_FINISH_PIECE_INDEX;
	pending_count = PENALTY_ROUTE_PENDING_NONE;
	distance = PENALTY_ROUTE_DISTANCE_NONE;
	sentinel_visited = PENALTY_ROUTE_SENTINEL_UNVISITED;
	for (index = PENALTY_ROUTE_INDEX_FIRST;
		index < (legacy_u16)track_pieces_counter; index++)
		visited[index] = PENALTY_ROUTE_TRACK_UNVISITED;
	track_index = (legacy_s16)*current_track;

	for (;;) {
		next_track = penalty_route_next(track_index);
		if (next_track == PENALTY_ROUTE_SENTINEL) {
			if (sentinel_visited == PENALTY_ROUTE_SENTINEL_UNVISITED) {
				sentinel_visited = PENALTY_ROUTE_SENTINEL_VISITED;
			} else if (pending_count != PENALTY_ROUTE_PENDING_NONE) {
				pending_count = LEGACY_U16_WRAP_SUB(
					pending_count, PENALTY_ROUTE_PENDING_STEP);
				track_index = pending_track[pending_count];
				distance = pending_distance[pending_count];
				continue;
			} else {
				return finish_penalty_route(current_track, penalty_count,
					best_track, best_distance, column, row);
			}
		} else if (next_track < 0 ||
			next_track >= track_pieces_counter ||
			visited[next_track] != PENALTY_ROUTE_TRACK_UNVISITED) {
			if (pending_count != PENALTY_ROUTE_PENDING_NONE) {
				pending_count = LEGACY_U16_WRAP_SUB(
					pending_count, PENALTY_ROUTE_PENDING_STEP);
				track_index = pending_track[pending_count];
				distance = pending_distance[pending_count];
				continue;
			}
			return finish_penalty_route(current_track, penalty_count,
				best_track, best_distance, column, row);
		} else {
			visited[next_track] = PENALTY_ROUTE_TRACK_VISITED;
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
			if (distance <= PENALTY_ROUTE_DISTANCE_NONE) {
				*current_track = next_track;
				*penalty_count = distance;
				return PENALTY_DETECTED;
			}
			if (best_distance == PENALTY_ROUTE_DISTANCE_NONE ||
				best_distance > distance) {
				best_track = next_track;
				best_distance = distance;
			}
		}

		alternate_track = penalty_route_alternate(track_index);
		if (alternate_track != PENALTY_ROUTE_SENTINEL) {
			pending_distance[pending_count] = distance;
			pending_track[pending_count] = alternate_track;
			pending_count = LEGACY_U16_WRAP_ADD(
				pending_count, PENALTY_ROUTE_PENDING_STEP);
		}
		if (next_track == TRACK_START_FINISH_PIECE_INDEX) {
			distance = PENALTY_ROUTE_FINISH_REACHED;
		} else if (distance != PENALTY_ROUTE_FINISH_REACHED) {
			distance = LEGACY_S16_WRAP_ADD(
				distance, PENALTY_ROUTE_DISTANCE_STEP);
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

	/* The original player_op reaches update_grip with SI == 80. */
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_FOURTH_WORD] =
		LEGACY_GRIP_STACK_SI_VALUE;
	if (carstate->car_sumSurfAllWheels == CAR_WHEEL_CONTACT_NONE)
		return;

	/*
	 * Reproduce update_grip's first operands: twice the car's base grip and
	 * the sum of the four surface-specific sliding coefficients.
	 */
	combined_grip_operand = LEGACY_S16_SHL(simd->grip, 1U);
	sliding_sum = 0;
	sliding_values = &simd->sliding;
	for (i = CAR_WHEEL_INDEX_FIRST; i < CAR_WHEEL_COUNT; i++) {
		sliding_sum = LEGACY_S16_WRAP_ADD(
			sliding_sum,
			sliding_values[(legacy_u8)carstate->car_surfaceWhl[i]]);
	}

	/* Operand words left by update_grip's first signed long multiply. */
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_FIRST_WORD] =
		sliding_sum < 0 ? -1 : 0;
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_SECOND_WORD] =
		combined_grip_operand;
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_THIRD_WORD] =
		combined_grip_operand < 0 ? -1 : 0;

	if (carstate->car_demandedGrip <= carstate->car_surfacegrip_sum)
		return;

	/*
	 * Sliding grip uses the post-deceleration speed when any wheel is on
	 * grass, with the divisor selected by the number of grass wheels.
	 */
	grass_wheels = GRASS_WHEEL_COUNT_NONE;
	for (i = CAR_WHEEL_INDEX_FIRST; i < CAR_WHEEL_COUNT; i++) {
		if (carstate->car_surfaceWhl[i] == SURFACE_GRASS)
			grass_wheels = LEGACY_S16_WRAP_ADD(
				grass_wheels, GRASS_WHEEL_COUNT_STEP);
	}
	grip_speed = speed_before_grip;
	if (grass_wheels != GRASS_WHEEL_COUNT_NONE) {
		speed2_before_grip = LEGACY_U16_WRAP_SUB(speed2_before_grip,
			LEGACY_U16_DIV_OR_ZERO(speed2_before_grip,
				grassDecelDivTab[grass_wheels]));
		grip_speed = speed2_before_grip;
	}

	/* Operand words left by the sliding-grip signed long division. */
	speed_shr8 = grip_speed >> CAR_SPEED_INTEGER_SHIFT;
	speed_squared = LEGACY_U32_WRAP_MUL(
		(legacy_u32)speed_shr8, (legacy_u32)speed_shr8);
	scaled_combined_grip = LEGACY_S32_WRAP_MUL(
		(legacy_s32)carstate->car_surfacegrip_sum, GRIP_FIXED_SCALE);
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_FIRST_WORD] =
		(legacy_s16)((legacy_u32)scaled_combined_grip >>
			LEGACY_LONG_HIGH_WORD_SHIFT);
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_SECOND_WORD] =
		(legacy_s16)speed_squared;
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_THIRD_WORD] =
		(legacy_s16)(speed_squared >> LEGACY_LONG_HIGH_WORD_SHIFT);
}

void update_car_speed(legacy_s8, legacy_s16, struct CARSTATE* carstate, struct SIMD* simd);
void update_player_state(struct CARSTATE* playerstate, struct SIMD* playersimd, struct CARSTATE* oppstate, struct SIMD* oppsimd, legacy_s16);

void update_grip(struct CARSTATE* carstate, struct SIMD* simd,
	legacy_s16 grip_behavior)
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

	if (carstate->car_sumSurfAllWheels == CAR_WHEEL_CONTACT_NONE) {
		carstate->car_40MfrontWhlAngle = CAR_FRONT_WHEEL_ANGLE_STRAIGHT;
		carstate->car_slidingFlag = CAR_SLIDING_INACTIVE;
		return;
	}

	grass_wheels = GRASS_WHEEL_COUNT_NONE;
	for (i = CAR_WHEEL_INDEX_FIRST; i < CAR_WHEEL_COUNT; i++) {
		if (carstate->car_surfaceWhl[i] == SURFACE_GRASS)
			grass_wheels = LEGACY_U16_WRAP_ADD(
				grass_wheels, GRASS_WHEEL_COUNT_STEP);
	}
	if (grass_wheels != GRASS_WHEEL_COUNT_NONE) {
		carstate->car_speed2 = LEGACY_U16_WRAP_SUB(
			carstate->car_speed2,
			LEGACY_U16_DIV_OR_ZERO(carstate->car_speed2,
				grassDecelDivTab[grass_wheels]));
		carstate->car_speed = carstate->car_speed2;
	}

	initial_angle = LEGACY_S16_WRAP_ADD(carstate->car_steeringAngle,
		carstate->car_36MwhlAngle);
	adjusted_angle = initial_angle;
	speed_shr8 = (legacy_u16)(carstate->car_speed >>
		CAR_SPEED_INTEGER_SHIFT);
	absolute_angle = absolute_word(adjusted_angle);
	angle_factor = LEGACY_S16_SAR(absolute_angle,
		DEMANDED_GRIP_ANGLE_SHIFT);
	square_low = LEGACY_U16_WRAP_MUL(speed_shr8, speed_shr8);
	square_low = (legacy_u16)(square_low >>
		DEMANDED_GRIP_SPEED_SQUARE_SHIFT);
	demanded_grip = LEGACY_U16_WRAP_MUL(square_low, angle_factor);

	combined_grip = LEGACY_S16_SHL(simd->grip, 1U);
	sliding_sum = 0;
	sliding_values = &simd->sliding;
	for (i = CAR_WHEEL_INDEX_FIRST; i < CAR_WHEEL_COUNT; i++) {
		sliding_sum = LEGACY_S16_WRAP_ADD(sliding_sum,
			sliding_values[(legacy_u8)carstate->car_surfaceWhl[i]]);
	}
	product = LEGACY_S32_WRAP_MUL(
		(legacy_s32)combined_grip, (legacy_s32)sliding_sum);
	combined_grip = LEGACY_S16_FROM_BITS(
		(legacy_u16)LEGACY_S32_SAR(product,
			COMBINED_GRIP_PRODUCT_SHIFT));
	carstate->car_demandedGrip = LEGACY_S16_FROM_BITS(demanded_grip);
	carstate->car_surfacegrip_sum = combined_grip;

	if (grip_behavior == GRIP_BEHAVIOR_OPPONENT) {
		carstate->car_40MfrontWhlAngle = LEGACY_S16_SHL(
			carstate->car_steeringAngle, FRONT_WHEEL_ANGLE_SHIFT);
		if (carstate->car_angle_z != CAR_ROTATION_DELTA_NONE) {
			carstate->car_angle_z = LEGACY_S16_SAR(
				LEGACY_S16_WRAP_MUL(carstate->car_angle_z,
					ROTATION_DAMPING_NUMERATOR), ROTATION_DAMPING_SHIFT);
		}
	}

	if (grip_behavior == GRIP_BEHAVIOR_PLAYER) {
	if (carstate->car_steeringAngle == CAR_STEERING_CENTERED) {
		rotation_low = LEGACY_S8_FROM_BITS(
			(legacy_u8)carstate->car_rotate.x);
		if (rotation_low != 0) {
			absolute_angle = rotation_low;
			if (absolute_angle < 0)
				absolute_angle = LEGACY_S16_WRAP_NEGATE(absolute_angle);
			if (absolute_angle < ROTATION_RECENTER_THRESHOLD) {
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
		carstate->car_slidingFlag = CAR_SLIDING_ACTIVE;
		numerator = LEGACY_S32_WRAP_MUL(
			(legacy_s32)combined_grip, GRIP_FIXED_SCALE);
		denominator = LEGACY_S32_WRAP_MUL(
			(legacy_s32)speed_shr8, (legacy_s32)speed_shr8);
		adjusted_angle = LEGACY_S16_FROM_BITS(
			(legacy_u16)LEGACY_S32_DIV_OR_ZERO(numerator, denominator));
		if (initial_angle < 0)
			adjusted_angle = LEGACY_S16_WRAP_NEGATE(adjusted_angle);
		adjusted_angle = LEGACY_S16_SAR(LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_MUL(adjusted_angle, SLIDE_ANGLE_WEIGHT),
			initial_angle), SLIDE_ANGLE_BLEND_SHIFT);
		carstate->field_42 = LEGACY_S16_WRAP_SUB(
			initial_angle, adjusted_angle);
	} else {
		carstate->car_slidingFlag = CAR_SLIDING_INACTIVE;
		if (carstate->field_42 != CAR_SLIDE_ANGLE_NONE) {
			carstate->field_42 = LEGACY_S16_WRAP_SUB(
				carstate->field_42, LEGACY_S16_SAR(
					carstate->field_42, SLIDE_ANGLE_DECAY_SHIFT));
			absolute_angle = carstate->field_42;
			if (absolute_angle < 0)
				absolute_angle = LEGACY_S16_WRAP_NEGATE(absolute_angle);
			if (absolute_angle < SLIDE_ANGLE_DECAY_THRESHOLD)
				carstate->field_42 = LEGACY_S16_SAR(
					carstate->field_42, 1U);
		}
	}

	if (carstate->car_angle_z == CAR_ROTATION_DELTA_NONE &&
		carstate->car_crashBmpFlag != CAR_CRASHED_FLAG)
		carstate->car_40MfrontWhlAngle = adjusted_angle;
	else
		carstate->car_40MfrontWhlAngle = CAR_FRONT_WHEEL_ANGLE_STRAIGHT;

	absolute_angle = carstate->car_rotate.z;
	if (absolute_angle < 0)
		absolute_angle = LEGACY_S16_WRAP_NEGATE(absolute_angle);
	if (absolute_angle > BANK_EFFECT_ROTATION_THRESHOLD) {
		tile_x = (legacy_u8)((legacy_u32)
			carstate->car_posWorld1.lx >> TRACK_WORLD_TILE_SHIFT);
		tile_z = (legacy_u8)((legacy_u32)
			carstate->car_posWorld1.lz >> TRACK_WORLD_TILE_SHIFT);
		track = td14_elem_map_main[
			LEGACY_U16_WRAP_ADD(terrainrows[tile_z], tile_x)];
		if (track == TRACK_CONTINUATION_NORTHWEST) {
			tile_x = LEGACY_U8_WRAP_SUB(tile_x, 1U);
			tile_z = LEGACY_U8_WRAP_ADD(tile_z, 1U);
		} else if (track == TRACK_CONTINUATION_NORTH) {
			tile_z = LEGACY_U8_WRAP_ADD(tile_z, 1U);
		} else if (track == TRACK_CONTINUATION_WEST) {
			tile_x = LEGACY_U8_WRAP_SUB(tile_x, 1U);
		}
		track = td14_elem_map_main[
			LEGACY_U16_WRAP_ADD(terrainrows[tile_z], tile_x)];
		if (track >= BANKED_TRACK_FIRST && track <= BANKED_TRACK_LAST) {
			carstate->car_40MfrontWhlAngle = LEGACY_S16_WRAP_ADD(
				carstate->car_40MfrontWhlAngle,
				LEGACY_S16_DIV_OR_ZERO(carstate->car_rotate.z,
					BANKED_TRACK_TILT_DIVISOR));
		}
	}

	correction = LEGACY_S16_DIV_OR_ZERO(
		LEGACY_S16_WRAP_SUB(adjusted_angle, initial_angle),
		SLIDE_CORRECTION_DIVISOR);
	if (LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
		combined_grip, SLIDE_GRIP_TOLERANCE)) <
		LEGACY_S16_FROM_BITS(demanded_grip)) {
		carstate->car_angle_z = LEGACY_S16_WRAP_ADD(
			carstate->car_angle_z, correction);
		carstate->car_angle_z = LEGACY_S16_DIV_OR_ZERO(
			carstate->car_angle_z, SLIDE_YAW_DAMPING_DIVISOR);
	} else if (carstate->car_angle_z != CAR_ROTATION_DELTA_NONE) {
		carstate->car_angle_z = LEGACY_S16_WRAP_ADD(
			carstate->car_angle_z, correction);
		carstate->car_angle_z = LEGACY_S16_DIV_OR_ZERO(
			carstate->car_angle_z, SLIDE_YAW_DAMPING_DIVISOR);
		if (carstate->car_angle_z == CAR_ROTATION_DELTA_NONE) {
			carstate->car_speed2 = (legacy_u16)multiply_and_scale(
				cos_fast(carstate->car_36MwhlAngle),
				carstate->car_speed2);
			if (cos_fast(carstate->car_36MwhlAngle) < 0)
				carstate->car_speed2 = CAR_SPEED_STOPPED;
			carstate->car_36MwhlAngle = CAR_WHEEL_HEADING_STRAIGHT;
		}
	}
	}

	if (carstate->car_36MwhlAngle != CAR_WHEEL_HEADING_STRAIGHT &&
		carstate->car_angle_z == CAR_ROTATION_DELTA_NONE) {
		carstate->car_36MwhlAngle = LEGACY_S16_SAR(
			LEGACY_S16_WRAP_MUL(carstate->car_36MwhlAngle,
				ROTATION_DAMPING_NUMERATOR), ROTATION_DAMPING_SHIFT);
	}
	if (carstate->car_angle_z != CAR_ROTATION_DELTA_NONE) {
		carstate->car_36MwhlAngle = LEGACY_S16_WRAP_SUB(
			carstate->car_36MwhlAngle, carstate->car_angle_z);
	}

	if (carstate->car_slidingFlag != CAR_SLIDING_INACTIVE) {
		absolute_angle = carstate->field_42;
		if (absolute_angle < 0)
			absolute_angle = LEGACY_S16_WRAP_NEGATE(absolute_angle);
		penalty = LEGACY_S16_SHL(absolute_angle,
			SLIDE_SPEED_PENALTY_SHIFT);
		if (carstate->car_speed <= (legacy_u16)penalty) {
			carstate->car_speed = CAR_SPEED_STOPPED;
			carstate->car_speed2 = CAR_SPEED_STOPPED;
		} else if (carstate->car_speed2 > (legacy_u16)penalty) {
			carstate->car_speed = LEGACY_U16_WRAP_SUB(
				carstate->car_speed, penalty);
			carstate->car_speed2 = LEGACY_U16_WRAP_SUB(
				carstate->car_speed2, penalty);
		} else {
			carstate->car_speed = CAR_SPEED_STOPPED;
			carstate->car_speed2 = CAR_SPEED_STOPPED;
		}

		if (carstate->car_crashBmpFlag == CRASH_EVENT_NONE) {
			for (i = CAR_WHEEL_INDEX_FIRST; i < CAR_WHEEL_COUNT; i++) {
				if (carstate->car_surfaceWhl[i] == SURFACE_PAVED)
					break;
			}
			carstate->field_CF = (legacy_u8)carstate->field_CF |
				(i < CAR_WHEEL_COUNT ? CAR_SOUND_SKID_PAVED_FLAG :
					CAR_SOUND_SKID_OFFROAD_FLAG);
		}
	}
	carstate->field_42 = CAR_SLIDE_ANGLE_NONE;
}
