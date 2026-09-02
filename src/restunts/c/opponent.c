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

#include "state_internal.h"

/*
 * Track object zero has no info record.  In the original executable its null
 * near pointer aliases the fixed DGROUP prefix.  The camera pointer found
 * there reaches an unrelated 256-vector data window; the tail of that window
 * is zero-filled.  Preserve the exact legacy alias explicitly so changing the
 * C-only data layout cannot change route physics.
 */
static struct VECTOR legacy_null_track_vectors[256] = {
	{ 0, 0, 0 },
	{ -8960, -137, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ 255, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 13056, -13108 },
	{ 27955, -27978, 27977 },
	{ -27978, 27977, -27978 },
	{ 73, 0, -27648 },
	{ -27568, -27056, -26543 },
	{ 28754, 12643, 53 },
	{ 25715, 24941, 28265 },
	{ 8448, 24944, 108 },
	{ 28019, 30063, 29440 },
	{ 28004, 26977, 110 },
	{ 28019, 30063, 27904 },
	{ 28525, 117, 8192 },
	{ 768, 0, 17152 },
	{ 28783, 29305, 26473 },
	{ 29800, 10272, 10563 },
	{ 21792, 27758, 28009 },
	{ 29801, 25701, 21280 },
	{ 26223, 30580, 29281 },
	{ 8293, 28233, 11875 },
	{ 12576, 14393, 11321 },
	{ 14641, 12345, 8238 },
	{ 16672, 27756, 29216 },
	{ 26473, 29800, 8307 },
	{ 25970, 25971, 30322 },
	{ 25701, 46, 0 },
	{ 0, 19712, 18243 },
	{ 8257, 18775, 17486 },
	{ 22351, 30464, 28265 },
	{ 28516, 25719, 26213 },
	{ 11552, 20256, 21589 },
	{ 20256, 8262, 20306 },
	{ 8279, 16724, 19522 },
	{ 8261, 20563, 17217 },
	{ 3397, 11776, 22096 },
	{ 83, 22574, 21334 },
	{ 11776, 21334, 72 },
	{ 20526, 21317, 11776 },
	{ 21317, 72, 19456 },
	{ 20820, 22100, 23380 },
	{ 24660, 25940, 84 },
	{ 513, 1027, 1541 },
	{ 2055, 2569, 3083 },
	{ 3597, 11791, 22096 },
	{ 83, 20053, 19526 },
	{ 20553, 11776, 22104 },
	{ 83, 20526, 21317 },
	{ 21760, 17998, 18764 },
	{ 80, 17710, 18515 },
	{ 8448, 18253, 65 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, -26624 },
	{ 22789, 15367, 20028 },
	{ 21325, 15943, 62 },
	{ 20992, 12342, 12336 },
	{ 2573, 8237, 29811 },
	{ 25441, 8299, 30319 },
	{ 29285, 27750, 30575 },
	{ 2573, 768, 20992 },
	{ 12342, 13104, 2573 },
	{ 8237, 28265, 25972 },
	{ 25959, 8306, 26980 },
	{ 26998, 25956, 25120 },
	{ 8313, 3376, 10 },
	{ 9, 13906, 12336 },
	{ 3385, 11530, 28192 },
	{ 29807, 25888, 28526 },
	{ 26485, 8296, 28787 },
	{ 25441, 8293, 28518 },
	{ 8306, 28261, 26998 },
	{ 28530, 28014, 28261 },
	{ 3444, 10, 252 },
	{ 2573, -256, 29184 },
	{ 28277, 29741, 28009 },
	{ 8293, 29285, 28530 },
	{ 8306, 512, 20992 },
	{ 12342, 12848, 2573 },
	{ 8237, 27750, 24943 },
	{ 26996, 26478, 28704 },
	{ 26991, 29806, 28192 },
	{ 29807, 27680, 24943 },
	{ 25956, 3428, 10 },
	{ 1, 13906, 12336 },
	{ 3377, 11530, 28192 },
	{ 27765, 8300, 28528 },
	{ 28265, 25972, 8306 },
	{ 29537, 26995, 28263 },
	{ 25965, 29806, 2573 },
	{ 2560, 2560, 25153 },
	{ 28526, 28018, 27745 },
	{ 28704, 28530, 29287 },
	{ 28001, 29728, 29285 },
	{ 26989, 24942, 26996 },
	{ 28271, 10, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, -256 }
};

/* Bytes 8-13 of the legacy DGROUP prefix read as "MS Run". */
static struct TRKOBJINFO legacy_null_track_info = {
	0, 0, 0, 0, 0, 0, 0,
	legacy_null_track_vectors,
	0x20, 0x52, 0x75, 0x6E
};

extern struct VECTOR* headless_track_vector_from_legacy_offset(
	legacy_u16 offset);

struct VECTOR* track_vector_from_legacy_offset(legacy_u16 offset)
{
	return headless_track_vector_from_legacy_offset(offset);
}

legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 track);

legacy_s16 world_position_word(legacy_s32 position)
{
	return LEGACY_S16_FROM_BITS(
		(legacy_u16)LEGACY_S32_SAR(position, 6U));
}

static legacy_s16 opponent_route_word(legacy_s16 index)
{
	legacy_u16 offset;

	offset = LEGACY_U16_WRAP_MUL(index, 2U);
	return LEGACY_READ_S16_LE(
		(const legacy_u8 far*)trackdata3 + offset);
}

static void opponent_advance_route(void)
{
	legacy_u8 route_point;

	route_point = (legacy_u8)state.opponentstate.field_CE;
	state.opponentstate.field_CE = LEGACY_S8_WRAP_ADD(route_point, 1);
	if (sub_18D60(opponent_route_word(
		state.opponentstate.car_trackdata3_index),
		&state.opponentstate.car_vec_unk3, route_point,
		&state.field_3F9) == 0) {
		return;
	}
	state.opponentstate.car_trackdata3_index = LEGACY_S16_WRAP_ADD(
		state.opponentstate.car_trackdata3_index, 1);
	if (opponent_route_word(
		state.opponentstate.car_trackdata3_index) == 0) {
		state.opponentstate.field_CD = LEGACY_S8_WRAP_ADD(
			state.opponentstate.field_CD, 1);
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
	opponent_x = world_position_word(
		(legacy_s32)state.opponentstate.car_posWorld1.lx);
	opponent_y = world_position_word(
		(legacy_s32)state.opponentstate.car_posWorld1.ly);
	opponent_z = world_position_word(
		(legacy_s32)state.opponentstate.car_posWorld1.lz);
	player_x = world_position_word(
		(legacy_s32)state.playerstate.car_posWorld1.lx);
	player_y = world_position_word(
		(legacy_s32)state.playerstate.car_posWorld1.ly);
	player_z = world_position_word(
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
	} else {
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

	for (;;) {
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
			continue;
		}
		steering_target = 0x41;
	} else if (steering_target < -0x41) {
		if (forced_route == 0) {
			forced_route = 1;
			opponent_advance_route();
			continue;
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
	break;
	}
	}

	input = 0;
	if (state.opponentstate.car_sumSurfRearWheels != 0) {
		if (state.opponentstate.car_crashBmpFlag != 0) {
			input = 2;
		} else if (state.opponentstate.car_36MwhlAngle != 0) {
			speed_threshold = LEGACY_U16_SHL(speed_step, 9U);
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
				LEGACY_U16_SHL((legacy_u8)state.field_3F9, 8U);
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
			world_position_word((legacy_s32)
				state.opponentstate.car_posWorld1.lx));
		relative.y = LEGACY_S16_WRAP_SUB(
			state.opponentstate.car_vec_unk3.y,
			world_position_word((legacy_s32)
				state.opponentstate.car_posWorld1.ly));
		relative.z = LEGACY_S16_WRAP_SUB(
			state.opponentstate.car_vec_unk3.z,
			world_position_word((legacy_s32)
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
				world_position_word((legacy_s32)
					state.opponentstate.car_posWorld1.lz)));
		finish_distance = LEGACY_S16_WRAP_ADD(finish_distance,
			multiply_and_scale(sin_fast(track_angle),
				LEGACY_S16_WRAP_SUB(trackcenterpos2[startcol2],
					world_position_word((legacy_s32)
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

	response_table = steerWhlRespTable_ptr;
	steering_angle = state.playerstate.car_steeringAngle;
	speed_index = (legacy_u8)((state.playerstate.car_speed2 >> 10) & 0xFC);
	response_index = LEGACY_S16_WRAP_ADD(
		(legacy_s16)speed_index, (legacy_s16)steering_input);
	response = response_table[response_index];

	/* Turning farther from center gets the original fourfold response. */
	if ((response > 0 && steering_angle < -1) ||
		(response < 0 && steering_angle > 1)) {
		response = LEGACY_S16_SHL(response, 2U);
	}

	/* With no steering input, bring a moving car back toward center. */
	if (response == 0 && state.playerstate.car_speed2 != 0 &&
		steering_angle != 0) {
		centering_limit = LEGACY_S16_SHL(
			(legacy_s16)response_table[speed_index + 1U], 1U);
		if (steering_angle < 0) {
			if (LEGACY_S16_WRAP_NEGATE(steering_angle) > centering_limit)
				response = centering_limit;
			else
				response = LEGACY_S16_WRAP_NEGATE(steering_angle);
		} else {
			if (steering_angle > centering_limit)
				response = LEGACY_S16_WRAP_NEGATE(centering_limit);
			else
				response = LEGACY_S16_WRAP_NEGATE(steering_angle);
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

	steering_angle = LEGACY_S16_WRAP_ADD(steering_angle, response);
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

static legacy_u8 opponent_speed_at(legacy_u16 index)
{
	if (index < OPPONENT_SPEED_COUNT)
		return oppnentSped[index];

	/*
	 * The assembly adds two zero-extended bytes as a 16-bit address; it does
	 * not wrap the sum to a byte.  The built-in surface -1 track objects use
	 * offset 268, in the original zero-filled rectangle workspace after the
	 * speed table.  Null track-info aliases similarly reach an unused zero
	 * byte in the path workspace.  Keep those reads deterministic instead of
	 * indexing beyond the C array.
	 */
	return 0;
}

legacy_s16 sub_18D60(
	legacy_s16 track_index_arg,
	struct VECTOR* output,
	legacy_s16 route_index_arg,
	legacy_s8* optional_speed
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
	if (track_object->ss_trkObjInfoPtr == 0)
		track_info = &legacy_null_track_info;
	else
		track_info = &track_object->ss_trkObjInfoPtr[track_subtype];
	arrow_type = (legacy_u8)track_info->si_arrowType;
	route_index = (legacy_u8)route_index_arg;

	if (connection_status == 0) {
		vector_index = LEGACY_U8_WRAP_MUL(route_index, 2U);
	} else {
		vector_index = LEGACY_U8_WRAP_SUB(arrow_type, route_index);
		vector_index = LEGACY_U8_WRAP_MUL(vector_index, 2U);
		vector_index = LEGACY_U8_WRAP_SUB(vector_index, 2U);
	}

	if (optional_speed != 0) {
		speed_index = (legacy_u8)track_info->si_oppSpedCode;
		speed_index = LEGACY_U16_WRAP_ADD(
			speed_index, (legacy_u8)track_object->ss_surfaceType);
		*optional_speed = LEGACY_S8_FROM_BITS(
			opponent_speed_at(speed_index));
	}

	packed_opponent_offset = (legacy_u16)(
		(legacy_u8)track_info->si_opp1 |
		LEGACY_U16_SHL((legacy_u8)track_info->si_opp2, 8U));
	has_opponent_path = packed_opponent_offset != 0;
	if (connection_status != 0 && has_opponent_path != 0) {
		route_vectors = track_vector_from_legacy_offset(
			packed_opponent_offset);
	} else {
		route_vectors = track_info->si_cameraDataOffset;
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
