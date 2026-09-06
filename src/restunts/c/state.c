#include "state_internal.h"
#include "game_input.h"

#define ROUTE_POINT_ADVANCE_DISTANCE 275
#define ROUTE_ALIGNMENT_WRAP_LIMIT \
	(ANGLE_FULL_TURN - ANGLE_EIGHTH_TURN)
#define ROUTE_GUIDANCE_DIRECTION_SHIFT 8U
#define ROUTE_DIRECTION_LEFT_SECTOR 1
#define ROUTE_DIRECTION_RIGHT_SECTOR 3
#define ROUTE_GEOMETRY_POINT_COUNT 4U
#define ROUTE_HEIGHT_REFERENCE_TRACK_LEVEL 0
#define ROUTE_HEIGHT_REFERENCE_CAR_LEVEL 1
#define PENALTY_ROUTE_DECISION_DISTANCE 3
#define PENALTY_ROUTE_CONFIRMATION_COUNT 3
#define PENALTY_SECONDS_PER_SKIPPED_ROUTE 3
#define PENALTY_DISPLAY_DURATION_SHIFT 2U

/* Vector from the player's car to its current route point. A y of -1 marks
   a route point with no height of its own: the route search still measures
   the drop to the car, the steering hint treats the point as level. */
static void route_point_delta(struct VECTOR* delta,
	legacy_s16 unspecified_height_reference)
{
	*delta = state.playerstate.car_vec_unk3;
	delta->x = LEGACY_S16_WRAP_SUB(delta->x,
		position_to_word(state.playerstate.car_posWorld1.lx));
	if (delta->y == ROUTE_POINT_HEIGHT_UNSPECIFIED) {
		delta->y = unspecified_height_reference ==
			ROUTE_HEIGHT_REFERENCE_CAR_LEVEL ? 0 : LEGACY_S16_WRAP_NEGATE(
				position_to_word(state.playerstate.car_posWorld1.ly));
	} else {
		delta->y = LEGACY_S16_WRAP_SUB(delta->y,
			position_to_word(state.playerstate.car_posWorld1.ly));
	}
	delta->z = LEGACY_S16_WRAP_SUB(delta->z,
		position_to_word(state.playerstate.car_posWorld1.lz));
}

void player_op(legacy_s8 arg_carInputByte) {
	struct VECTOR var_38;
	struct VECTOR var_32;
	struct VECTOR var_28;
	struct VECTOR var_1A[ROUTE_GEOMETRY_POINT_COUNT];
	struct VECTOR var_52[ROUTE_GEOMETRY_POINT_COUNT];
	struct MATRIX* var_matptr;
	legacy_s8 var_3A;
	legacy_s8 var_1C;
	legacy_s8 var_2A;
	legacy_s8 var_2C;
	legacy_s16 var_2;
	legacy_s16 var_1EpenaltyCounter;
	legacy_u16 var_speedBeforeGrip;
	legacy_u16 var_speed2BeforeGrip;
	legacy_u8 route_point;
	legacy_u8 commit_penalty;
	legacy_u8 route_selection_required;
	legacy_u8 route_advance_required;
	legacy_u8 guidance_required;
	legacy_s16 si;

	//return ported_player_op_(arg_carInputByte);

	if (show_penalty_counter != 0) {
		show_penalty_counter = LEGACY_S8_WRAP_SUB(
			show_penalty_counter, 1);
	}

	state.playerstate.field_CF = CAR_SOUND_ENGINE_ACTIVE_FLAG;
	if (state.playerstate.car_crashBmpFlag != CRASH_EVENT_NONE) {
		state.field_45D = ROUTE_INDICATOR_NONE;
		arg_carInputByte = INPUT_BRAKE_FLAG;

		if (state.playerstate.car_speed2 == 0) {
			state.playerstate.field_CF = 0;

			if (state.playerstate.car_speed == 0 && state.playerstate.car_rc1[0] == 0 && state.playerstate.car_rc1[1] == 0 && state.playerstate.car_rc1[2] == 0 && state.playerstate.car_rc1[3] == 0) {
				return ;
			}
		}
	}

	update_car_speed(arg_carInputByte, PLAYER_CAR_INDEX,
		&state.playerstate, &simd_player);
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_FIRST_WORD] =
		state.playerstate.car_lastrpm;
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_SECOND_WORD] =
		(legacy_s16)state.playerstate.car_speed;
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_THIRD_WORD] =
		(legacy_s16)state.playerstate.car_gearratio;
	upd_statef20_from_steer_input(
		LEGACY_S16_SAR((legacy_s16)arg_carInputByte,
			INPUT_STEERING_SHIFT) & INPUT_PEDAL_MASK);
	var_speedBeforeGrip = state.playerstate.car_speed;
	var_speed2BeforeGrip = state.playerstate.car_speed2;
	update_grip(&state.playerstate, &simd_player, GRIP_BEHAVIOR_PLAYER);
	update_legacy_grip_stack_words(
		&state.playerstate,
		&simd_player,
		var_speedBeforeGrip,
		var_speed2BeforeGrip
	);
	update_player_state(&state.playerstate, &simd_player,
		&state.opponentstate, &simd_opponent, PLAYER_CAR_INDEX);
	state.game_travDist = LEGACY_S32_WRAP_ADD(
		state.game_travDist,
		(legacy_s32)(legacy_u16)state.playerstate.car_speed2);
	var_1C = state.field_45B;
	var_2 = state.field_2F2;
	si = detect_penalty(&var_2, &var_1EpenaltyCounter);
	if (si != 0) {
		commit_penalty = 0;
		if (var_1EpenaltyCounter == PENALTY_ROUTE_OUTSIDE_TRACK) {
			state.field_45B = ROUTE_TRACKING_OUTSIDE_TRACK;
			state.field_45C = 0;
		} else {
			if (state.field_45B == ROUTE_TRACKING_OUTSIDE_TRACK) {
				state.field_45B = ROUTE_TRACKING_NORMAL;
				state.field_45C = 0;
			}
			if (state.field_45B == ROUTE_TRACKING_NORMAL) {
				if (var_2 == 0 && state.field_2F4 != 0) {
					state.playerstate.field_CD = LEGACY_S8_WRAP_ADD(
						state.playerstate.field_CD, 1);
					commit_penalty = 1;
				} else if (var_1EpenaltyCounter >= 0 &&
					var_1EpenaltyCounter < PENALTY_ROUTE_DECISION_DISTANCE) {
					state.field_45C = 0;
					state.field_2F2 = var_2;
				} else if (var_1EpenaltyCounter ==
					PENALTY_ROUTE_FINISH_REACHED ||
					var_1EpenaltyCounter > PENALTY_ROUTE_DECISION_DISTANCE) {
					if (td01_track_file_cpy[state.field_2F4] == var_2 ||
						td02_penalty_related[state.field_2F4] == var_2) {
						state.field_45C = LEGACY_S8_WRAP_ADD(
							state.field_45C, 1);
					} else {
						if (td01_track_file_cpy[var_2] == state.field_2F4 ||
							td02_penalty_related[var_2] == state.field_2F4) {
							state.field_45B = ROUTE_TRACKING_WRONG_WAY;
						}
						state.field_45C = 1;
					}
					if (state.field_45C >= PENALTY_ROUTE_CONFIRMATION_COUNT)
						commit_penalty = 1;
				}
			}
		}
		if (commit_penalty != 0) {
			state.field_2F2 = var_2;
			state.field_45C = 0;
			if (var_1EpenaltyCounter > 0) {
				penalty_time = LEGACY_S16_WRAP_MUL(
					LEGACY_S16_WRAP_MUL(
						var_1EpenaltyCounter, framespersec),
					PENALTY_SECONDS_PER_SKIPPED_ROUTE);
				show_penalty_counter = LEGACY_S8_FROM_BITS(
					(legacy_u8)LEGACY_U16_SHL(framespersec,
						PENALTY_DISPLAY_DURATION_SHIFT));
				state.game_penalty = LEGACY_S16_WRAP_ADD(
					state.game_penalty, penalty_time);
			}
		}
		state.field_2F4 = var_2;
	}
	state.field_45D = ROUTE_INDICATOR_NONE;
	if (state.field_45B != ROUTE_TRACKING_OUTSIDE_TRACK) {
		var_matptr = mat_rot_zxy(
			state.playerstate.car_rotate.z,
			state.playerstate.car_rotate.y,
			state.playerstate.car_rotate.x,
			MATRIX_ROTATION_ORDER_YXZ);
		route_selection_required = 0;
		route_advance_required = 0;
		guidance_required = 1;

		if (state.field_45B == ROUTE_TRACKING_WRONG_WAY) {
			if (state.playerstate.car_crashBmpFlag == CRASH_EVENT_NONE)
				state.field_45D = ROUTE_INDICATOR_WRONG_WAY;
			var_2 = state.field_2F4;
			route_selection_required = 1;
		} else {
			si = 0;
			if (state.playerstate.car_trackdata3_index != ROUTE_INDEX_NONE) {
				if ((var_1C == ROUTE_TRACKING_NORMAL ||
					state.field_45B != ROUTE_TRACKING_NORMAL) &&
					(state.playerstate.car_trackdata3_index ==
						state.field_2F2 ||
					td01_track_file_cpy[state.field_2F2] ==
						state.playerstate.car_trackdata3_index ||
					td02_penalty_related[state.field_2F2] ==
						state.playerstate.car_trackdata3_index)) {
					var_32.x = LEGACY_S16_WRAP_SUB(
						state.playerstate.car_vec_unk3.x,
						position_to_word(
							state.playerstate.car_posWorld1.lx));
					if (state.playerstate.car_vec_unk3.y ==
						ROUTE_POINT_HEIGHT_UNSPECIFIED) {
						var_32.y = 0;
					} else {
						var_32.y = LEGACY_S16_WRAP_SUB(
							state.playerstate.car_vec_unk3.y,
							position_to_word(
								state.playerstate.car_posWorld1.ly));
					}
					var_32.z = LEGACY_S16_WRAP_SUB(
						state.playerstate.car_vec_unk3.z,
						position_to_word(
							state.playerstate.car_posWorld1.lz));
					mat_mul_vector(&var_32, var_matptr, &var_38);
					si = var_38.z;
				} else {
					state.playerstate.car_trackdata3_index = ROUTE_INDEX_NONE;
				}
			}
			if (si < ROUTE_POINT_ADVANCE_DISTANCE) {
				if (state.playerstate.car_trackdata3_index == ROUTE_INDEX_NONE) {
					var_2 = state.field_2F2;
					route_selection_required = 1;
				} else {
					route_advance_required = 1;
				}
			}
		}

		if (route_selection_required != 0) {
			if (td02_penalty_related[var_2] != TRACK_ROUTE_LINK_NONE) {
				guidance_required = 0;
			} else {
				var_2A = 0;
				var_2C = ROUTE_POINT_FIRST;
				do {
					var_2A = LEGACY_S8_FROM_BITS((legacy_u8)sub_18D60(
						var_2, &state.playerstate.car_vec_unk3,
						(legacy_s16)(legacy_u8)var_2C, 0));
					route_point_delta(&var_28,
						ROUTE_HEIGHT_REFERENCE_TRACK_LEVEL);
					mat_mul_vector(&var_28, var_matptr, &var_38);
					if (var_2C == ROUTE_POINT_FIRST ||
						(var_38.z < var_32.z && var_38.z > 0)) {
						var_3A = var_2C;
						var_32.z = var_38.z;
					}
					var_2C = LEGACY_S8_WRAP_ADD(var_2C, ROUTE_POINT_STEP);
				} while (var_2A == 0);

				if (state.field_45B == ROUTE_TRACKING_WRONG_WAY) {
					if (var_3A == ROUTE_POINT_FIRST) {
						sub_18D60(var_2, var_52, ROUTE_POINT_FIRST, 0);
						sub_18D60(var_2, var_1A, ROUTE_POINT_SECOND, 0);
					} else {
						sub_18D60(var_2, var_52,
							(legacy_s16)LEGACY_S8_WRAP_SUB(
								var_3A, ROUTE_POINT_STEP), 0);
						sub_18D60(var_2, var_1A,
							(legacy_s16)(legacy_u8)var_3A, 0);
					}
					si = LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S16_WRAP_SUB(
						state.playerstate.car_rotate.x,
						polarAngle(
							LEGACY_S16_WRAP_SUB(
								var_52[0].x, var_1A[0].x),
							LEGACY_S16_WRAP_SUB(
								var_1A[0].z, var_52[0].z))) & ANGLE_MASK);
					if (si > ROUTE_ALIGNMENT_WRAP_LIMIT ||
						si < ANGLE_EIGHTH_TURN) {
						state.field_45B = ROUTE_TRACKING_NORMAL;
						state.field_45C = 1;
						state.playerstate.car_trackdata3_index = var_2;
						state.playerstate.field_CE = var_3A;
					}
				} else {
					state.playerstate.car_trackdata3_index =
						state.field_2F2;
					state.playerstate.field_CE = var_3A;
				}
				route_advance_required = 1;
			}
		}

		if (route_advance_required != 0) {
			route_point = (legacy_u8)state.playerstate.field_CE;
			state.playerstate.field_CE = LEGACY_S8_WRAP_ADD(
				route_point, ROUTE_POINT_STEP);
			if (sub_18D60(state.playerstate.car_trackdata3_index,
				&state.playerstate.car_vec_unk3,
				(legacy_s16)route_point, 0) != 0) {
				if (td02_penalty_related[state.field_2F2] !=
					TRACK_ROUTE_LINK_NONE) {
					state.playerstate.car_trackdata3_index = ROUTE_INDEX_NONE;
				} else {
					state.playerstate.car_trackdata3_index =
						td01_track_file_cpy[state.field_2F2];
				}
				state.playerstate.field_CE = ROUTE_POINT_FIRST;
			}
		}

		if (guidance_required != 0 &&
			state.playerstate.car_trackdata3_index != ROUTE_INDEX_NONE &&
			state.field_45B == ROUTE_TRACKING_NORMAL) {
			route_point_delta(&var_28, ROUTE_HEIGHT_REFERENCE_CAR_LEVEL);
			var_matptr = mat_rot_zxy(
				state.playerstate.car_rotate.z,
				state.playerstate.car_rotate.y,
				state.playerstate.car_rotate.x,
				MATRIX_ROTATION_ORDER_YXZ);
			mat_mul_vector(&var_28, var_matptr, &var_38);
			state.playerstate.field_48 = LEGACY_S16_FROM_BITS(
				(legacy_u16)polarAngle(
					LEGACY_S16_WRAP_NEGATE(var_38.x), var_38.z) & ANGLE_MASK);
			if (state.playerstate.car_crashBmpFlag == CRASH_EVENT_NONE) {
				si = LEGACY_U16_SAR(LEGACY_U16_WRAP_ADD(
					state.playerstate.field_48, ANGLE_EIGHTH_TURN) &
					ANGLE_MASK, ROUTE_GUIDANCE_DIRECTION_SHIFT);
				if (si == ROUTE_DIRECTION_LEFT_SECTOR) {
					state.field_45D = ROUTE_INDICATOR_LEFT;
				} else if (si == ROUTE_DIRECTION_RIGHT_SECTOR &&
					state.playerstate.field_B6 == 0) {
					state.field_45D = ROUTE_INDICATOR_RIGHT;
				} else {
					state.field_45D = ROUTE_INDICATOR_NONE;
				}
			}
		}

		if (state.playerstate.field_CD != 0) {
			si = multiply_and_scale(cos_fast(track_angle),
				LEGACY_S16_WRAP_SUB(trackcenterpos[startrow2],
					position_to_word(
						state.playerstate.car_posWorld1.lz)));
			si = LEGACY_S16_WRAP_ADD(si,
				multiply_and_scale(sin_fast(track_angle),
					LEGACY_S16_WRAP_SUB(trackcenterpos2[startcol2],
						position_to_word(
							state.playerstate.car_posWorld1.lx))));
			if (si < 0)
				update_crash_state(CRASH_EVENT_FINISH, PLAYER_CAR_INDEX);
		}
	}
}
