#include "externs.h"
#include "legacy.h"
#include "math.h"
#include "shape3d.h"

extern int penalty_time;
extern short legacy_grip_stack_words[4];
extern short grassDecelDivTab[];
extern struct TRACKOBJECT trkObjectList[215];
extern unsigned char oppnentSped[];
extern struct PLANE far* planptr;
extern struct PLANE far plan_memres;
extern int track_pieces_counter;

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

int detect_penalty(int* current_track, int* penalty_count)
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
	unsigned int speed_before_grip,
	unsigned int speed2_before_grip
) {
	short combined_grip_operand;
	short sliding_sum;
	short* sliding_values;
	unsigned int grip_speed;
	unsigned int speed_shr8;
	unsigned long speed_squared;
	long scaled_combined_grip;
	int grass_wheels;
	int i;

	/* The original player_op reaches update_grip with SI == 0x50. */
	legacy_grip_stack_words[3] = 0x50;
	if (carstate->car_sumSurfAllWheels == 0)
		return;

	/*
	 * Reproduce update_grip's first operands: twice the car's base grip and
	 * the sum of the four surface-specific sliding coefficients.
	 */
	combined_grip_operand = (short)((unsigned short)simd->grip << 1);
	sliding_sum = 0;
	sliding_values = &simd->sliding;
	for (i = 0; i < 4; i++) {
		sliding_sum = (short)(
			(unsigned short)sliding_sum +
			(unsigned short)sliding_values[
				(unsigned char)carstate->car_surfaceWhl[i]
			]
		);
	}

	/* Operand words left by update_grip's first signed long multiply. */
	legacy_grip_stack_words[0] = sliding_sum < 0 ? -1 : 0;
	legacy_grip_stack_words[1] = combined_grip_operand;
	legacy_grip_stack_words[2] = combined_grip_operand < 0 ? -1 : 0;

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
		speed2_before_grip -=
			speed2_before_grip / (unsigned short)grassDecelDivTab[grass_wheels];
		grip_speed = speed2_before_grip;
	}

	/* Operand words left by the sliding-grip signed long division. */
	speed_shr8 = grip_speed >> 8;
	speed_squared = (unsigned long)speed_shr8 * speed_shr8;
	scaled_combined_grip =
		(long)carstate->car_surfacegrip_sum * 0x100L;
	legacy_grip_stack_words[0] =
		(short)((unsigned long)scaled_combined_grip >> 16);
	legacy_grip_stack_words[1] = (short)speed_squared;
	legacy_grip_stack_words[2] = (short)(speed_squared >> 16);
}

void update_car_speed(char, int, struct CARSTATE* carstate, struct SIMD* simd);
void update_player_state(struct CARSTATE* playerstate, struct SIMD* playersimd, struct CARSTATE* oppstate, struct SIMD* oppsimd, int);

static legacy_s16 grip_sar(legacy_s16 value, legacy_u16 count)
{
	legacy_u16 bits;

	bits = (legacy_u16)value;
	while (count-- != 0)
		bits = (legacy_u16)((bits >> 1) | (bits & 0x8000U));
	return LEGACY_S16_FROM_BITS(bits);
}

static legacy_s32 grip_sar32(legacy_s32 value, legacy_u16 count)
{
	legacy_u32 bits;

	bits = (legacy_u32)value;
	while (count-- != 0)
		bits = (bits >> 1) | (bits & 0x80000000UL);
	return LEGACY_S32_FROM_BITS(bits);
}

void update_grip(struct CARSTATE* carstate, struct SIMD* simd,
	int player_behavior)
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
			(legacy_u16)(carstate->car_speed2 /
				(legacy_u16)grassDecelDivTab[grass_wheels]));
		carstate->car_speed = carstate->car_speed2;
	}

	initial_angle = LEGACY_S16_WRAP_ADD(carstate->car_steeringAngle,
		carstate->car_36MwhlAngle);
	adjusted_angle = initial_angle;
	speed_shr8 = (legacy_u16)(carstate->car_speed >> 8);
	absolute_angle = adjusted_angle;
	if (absolute_angle < 0)
		absolute_angle = LEGACY_S16_WRAP_NEGATE(absolute_angle);
	angle_factor = grip_sar(absolute_angle, 3U);
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
		(legacy_u16)grip_sar32(product, 10U));
	carstate->car_demandedGrip = (legacy_s16)demanded_grip;
	carstate->car_surfacegrip_sum = combined_grip;

	if (player_behavior == 0) {
		carstate->car_40MfrontWhlAngle = LEGACY_S16_FROM_BITS(
			(legacy_u16)carstate->car_steeringAngle << 2);
		if (carstate->car_angle_z != 0) {
			carstate->car_angle_z = grip_sar(
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
			(legacy_u16)(numerator / denominator));
		if (initial_angle < 0)
			adjusted_angle = LEGACY_S16_WRAP_NEGATE(adjusted_angle);
		adjusted_angle = grip_sar(LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_MUL(adjusted_angle, 3), initial_angle), 2U);
		carstate->field_42 = LEGACY_S16_WRAP_SUB(
			initial_angle, adjusted_angle);
	} else {
		carstate->car_slidingFlag = 0;
		if (carstate->field_42 != 0) {
			carstate->field_42 = LEGACY_S16_WRAP_SUB(
				carstate->field_42, grip_sar(carstate->field_42, 4U));
			absolute_angle = carstate->field_42;
			if (absolute_angle < 0)
				absolute_angle = LEGACY_S16_WRAP_NEGATE(absolute_angle);
			if (absolute_angle < 0x10)
				carstate->field_42 = grip_sar(carstate->field_42, 1U);
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
				(legacy_s16)(carstate->car_rotate.z / 5));
		}
	}

	correction = (legacy_s16)(LEGACY_S16_WRAP_SUB(
		adjusted_angle, initial_angle) / 0x0E);
	if (LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
		combined_grip, 0x3E8U)) < LEGACY_S16_FROM_BITS(demanded_grip)) {
		carstate->car_angle_z = LEGACY_S16_WRAP_ADD(
			carstate->car_angle_z, correction);
		carstate->car_angle_z = (legacy_s16)(carstate->car_angle_z / 2);
	} else if (carstate->car_angle_z != 0) {
		carstate->car_angle_z = LEGACY_S16_WRAP_ADD(
			carstate->car_angle_z, correction);
		carstate->car_angle_z = (legacy_s16)(carstate->car_angle_z / 2);
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
		carstate->car_36MwhlAngle = grip_sar(
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

void upd_statef20_from_steer_input(char steering_input) {
	signed char* response_table;
	short steering_angle;
	short response;
	short centering_limit;
	short response_index;
	unsigned char speed_index;

	response_table = (signed char*)steerWhlRespTable_ptr;
	steering_angle = state.playerstate.car_steeringAngle;
	speed_index = (unsigned char)((state.playerstate.car_speed2 >> 10) & 0xFC);
	response_index = (short)(speed_index + (signed char)steering_input);
	response = response_table[response_index];

	/* Turning farther from center gets the original fourfold response. */
	if ((response > 0 && steering_angle < -1) ||
		(response < 0 && steering_angle > 1)) {
		response = (short)(response * 4);
	}

	/* With no steering input, bring a moving car back toward center. */
	if (response == 0 && state.playerstate.car_speed2 != 0 &&
		steering_angle != 0) {
		centering_limit = (short)(response_table[speed_index + 1] * 2);
		if (steering_angle < 0) {
			if ((short)-steering_angle > centering_limit)
				response = centering_limit;
			else
				response = (short)-steering_angle;
		} else {
			if (steering_angle > centering_limit)
				response = (short)-centering_limit;
			else
				response = (short)-steering_angle;
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

	steering_angle = (short)(steering_angle + response);
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

	sum = (legacy_s32)first + (legacy_s32)second;
	if (sum < 0)
		return (legacy_s16)(-(((-sum) + 1) / 2));
	return (legacy_s16)(sum / 2);
}

short sub_18D60(
	short track_index_arg,
	struct VECTOR* output,
	short route_index_arg,
	short* optional_speed
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
		route_vectors = (struct VECTOR*)packed_opponent_offset;
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
		(long)0x00017700L,
		0L,
		(long)((legacy_s32)path_z * (legacy_s32)64),
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
		(short)route_index,
		(short*)&state.field_3F9);
}

void player_op(char arg_carInputByte) {
	struct VECTOR var_38;
	struct VECTOR var_32;
	struct VECTOR var_28;
	struct VECTOR var_1A[4];
	struct VECTOR var_52[4];
	struct MATRIX* var_matptr;
	char var_3A;
	char var_1C;
	char var_2A;
	char var_2C;
	int var_2;
	int var_1EpenaltyCounter;
	unsigned int var_speedBeforeGrip;
	unsigned int var_speed2BeforeGrip;
	int si;

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
	legacy_grip_stack_words[0] = state.playerstate.car_lastrpm;
	legacy_grip_stack_words[1] = (short)state.playerstate.car_speed;
	legacy_grip_stack_words[2] = (short)state.playerstate.car_gearratio;
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
