#include "restunts.h"
#include "game_input.h"

#define ACTIVE_CAR_COUNT_WITHOUT_OPPONENT 1U
#define ACTIVE_CAR_COUNT_WITH_OPPONENT 2U
#define PLAYER_CAR_INDEX 0U
#define CAR_WORLD_POSITION_SHIFT 6U
#define CAMERA_TARGET_OVERRIDE_FIELD_MIN 128
#define CAMERA_TARGET_OVERRIDE_FIELD_END 896
#define CAMERA_TARGET_HEIGHT 270
#define CAMERA_VERTICAL_STEP_LIMIT 30
#define CAMERA_FOLLOW_DISTANCE 450
#define FULL_GAME_FRAMES_PER_SECOND 20U
#define CAMERA_FULL_RATE_STEP_LIMIT 120
#define CAMERA_REDUCED_RATE_STEP_LIMIT 240
#define TRACK_POINT_UPDATE_DIVISOR_SHIFT 1U
#define TRACK_POINT_INITIAL_DISTANCE 10000
#define REPLAY_MODE_LIVE 0U
#define REPLAY_MODE_PAUSED 1U
#define CRASH_ROLL_DISTANCE_LIMIT 450
#define CRASH_ROLL_STAGE_THRESHOLD 384
#define CRASH_ROLL_ADVANCE_STEP 8
#define CRASH_ROLL_APPROACH_DISTANCE 228
#define CRASH_ROLL_SPEED_LIMIT 1280

extern legacy_u8 byte_4616E;

void sub_2298C(void)
{
	struct VECTOR* previous_position;
	struct CARSTATE* carstate;
	struct VECTOR target;
	legacy_s16 car_x;
	legacy_s16 car_y;
	legacy_s16 car_z;
	legacy_s16 target_y;
	legacy_s16 delta_y;
	legacy_s16 angle;
	legacy_s16 distance;
	legacy_s16 adjustment;
	legacy_s16 nearest_distance;
	legacy_s32 delta_x;
	legacy_s32 delta_z;
	legacy_s32 absolute_x;
	legacy_s32 absolute_z;
	legacy_u16 car_index;
	legacy_u16 car_count;
	legacy_u16 divisor;
	legacy_u8 candidate;

	car_count = gameconfig.game_opponenttype == 0 ?
		ACTIVE_CAR_COUNT_WITHOUT_OPPONENT : ACTIVE_CAR_COUNT_WITH_OPPONENT;
	for (car_index = 0; car_index < car_count; car_index++) {
		previous_position = car_index == PLAYER_CAR_INDEX ?
			&state.game_vec3 : &state.game_vec4;
		*previous_position = state.game_vec1[car_index];
		carstate = car_index == PLAYER_CAR_INDEX ?
			&state.playerstate : &state.opponentstate;
		car_x = LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S32_SAR(
			carstate->car_posWorld1.lx, CAR_WORLD_POSITION_SHIFT));
		car_y = LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S32_SAR(
			carstate->car_posWorld1.ly, CAR_WORLD_POSITION_SHIFT));
		car_z = LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S32_SAR(
			carstate->car_posWorld1.lz, CAR_WORLD_POSITION_SHIFT));
		target = carstate->car_vec_unk3;
		if ((car_index == PLAYER_CAR_INDEX &&
			(state.field_45B != 0 || state.field_45C != 0)) ||
			carstate->field_B6 != 0 ||
			carstate->car_crashBmpFlag != 0 ||
			carstate->car_trackdata3_index == -1 ||
			(carstate->field_48 > CAMERA_TARGET_OVERRIDE_FIELD_MIN &&
			carstate->field_48 < CAMERA_TARGET_OVERRIDE_FIELD_END)) {
			target.x = car_x;
			target.y = car_y;
			target.z = car_z;
		}

		target_y = LEGACY_S16_WRAP_ADD(car_y, CAMERA_TARGET_HEIGHT);
		delta_y = LEGACY_S16_WRAP_SUB(
			state.game_vec1[car_index].y, target_y);
		if (delta_y != 0) {
			if (delta_y > CAMERA_VERTICAL_STEP_LIMIT)
				delta_y = CAMERA_VERTICAL_STEP_LIMIT;
			else if (delta_y < -CAMERA_VERTICAL_STEP_LIMIT)
				delta_y = -CAMERA_VERTICAL_STEP_LIMIT;
			state.game_vec1[car_index].y = LEGACY_S16_WRAP_SUB(
				state.game_vec1[car_index].y, delta_y);
		}

		angle = (legacy_s16)polarAngle(
			LEGACY_S16_WRAP_SUB(
				target.x, state.game_vec1[car_index].x),
			LEGACY_S16_WRAP_SUB(
				target.z, state.game_vec1[car_index].z));
		distance = (legacy_s16)polarRadius2D(
			LEGACY_S16_WRAP_SUB(
				car_x, state.game_vec1[car_index].x),
			LEGACY_S16_WRAP_SUB(
				car_z, state.game_vec1[car_index].z));
		if (distance > CAMERA_FOLLOW_DISTANCE) {
			adjustment = LEGACY_S16_WRAP_SUB(distance,
				CAMERA_FOLLOW_DISTANCE);
			if (framespersec == FULL_GAME_FRAMES_PER_SECOND) {
				if (adjustment > CAMERA_FULL_RATE_STEP_LIMIT)
					adjustment = CAMERA_FULL_RATE_STEP_LIMIT;
			} else if (adjustment > CAMERA_REDUCED_RATE_STEP_LIMIT) {
				adjustment = CAMERA_REDUCED_RATE_STEP_LIMIT;
			}
			state.game_vec1[car_index].x = LEGACY_S16_WRAP_ADD(
				state.game_vec1[car_index].x,
				multiply_and_scale(adjustment,
					sin_fast((legacy_u16)angle)));
			state.game_vec1[car_index].z = LEGACY_S16_WRAP_ADD(
				state.game_vec1[car_index].z,
				multiply_and_scale(adjustment,
					cos_fast((legacy_u16)angle)));
		}

		divisor = LEGACY_U16_SAR(framespersec,
			TRACK_POINT_UPDATE_DIVISOR_SHIFT);
		if (divisor != 0 &&
			(legacy_u16)state.game_frame % divisor != 0)
			continue;
		nearest_distance = TRACK_POINT_INITIAL_DISTANCE;
		for (candidate = 0;
			LEGACY_S8_FROM_BITS(candidate) <
				LEGACY_S8_FROM_BITS(byte_4616E);
			candidate++) {
			delta_x = LEGACY_S32_WRAP_SUB(
				(legacy_s32)trackdata9[candidate].x,
				(legacy_s32)car_x);
			delta_z = LEGACY_S32_WRAP_SUB(
				(legacy_s32)trackdata9[candidate].z,
				(legacy_s32)car_z);
			absolute_x = delta_x < 0 ?
				LEGACY_S32_WRAP_NEGATE(delta_x) : delta_x;
			if (absolute_x >= nearest_distance)
				continue;
			absolute_z = delta_z < 0 ?
				LEGACY_S32_WRAP_NEGATE(delta_z) : delta_z;
			if (absolute_z >= nearest_distance)
				continue;
			distance = polarRadius2D(
				LEGACY_S16_FROM_BITS((legacy_u16)delta_x),
				LEGACY_S16_FROM_BITS((legacy_u16)delta_z));
			if (distance < nearest_distance) {
				state.field_3F7[car_index] =
					LEGACY_S8_FROM_BITS(candidate);
				nearest_distance = distance;
			}
		}
	}
}

void update_gamestate(void)
{
	legacy_s8 car_input;
	legacy_u16 checkpoint_index;

	car_input = td16_rpl_buffer[(legacy_u16)state.game_frame];
	if (car_input != 0)
		state.game_inputmode = 1;

	if (word_45A00 == 0 ||
		((legacy_u16)state.game_frame % (legacy_u16)word_45A00) == 0) {
		get_kevinrandom_seed(state.kevinseed);
		checkpoint_index = LEGACY_U16_DIV_OR_ZERO(
			state.game_frame, word_45A00);
		fmemcpy(&cvxptr[checkpoint_index],
			&state,
			sizeof(struct GAMESTATE));
	}

	state.game_frame = LEGACY_S16_WRAP_ADD(state.game_frame, 1);
	if (state.game_3F6autoLoadEvalFlag != 0 &&
		state.game_frame_in_sec < state.game_frames_per_sec) {
		state.game_frame_in_sec = LEGACY_S16_WRAP_ADD(
			state.game_frame_in_sec, 1);
		if (state.game_frame_in_sec == state.game_frames_per_sec &&
			byte_449DA == 0) {
			if (state.playerstate.car_crashBmpFlag == 1 &&
				state.playerstate.car_speed2 != 0) {
				state.game_frames_per_sec = LEGACY_S16_WRAP_ADD(
					state.game_frames_per_sec, 1);
			} else if (game_replay_mode == REPLAY_MODE_LIVE) {
				byte_449DA = 1;
			}
		}
	}

	if (state.game_inputmode != 0) {
		player_op(car_input);
		if (gameconfig.game_opponenttype != 0)
			opponent_op();
		sub_2298C();
		if (state.field_42A != 0)
			sub_19BA0();
#ifndef RESTUNTS_HEADLESS
		audio_carstate();
#endif
	} else if (game_replay_mode == REPLAY_MODE_PAUSED) {
#ifndef RESTUNTS_HEADLESS
		audio_carstate();
#endif
		if (byte_4393C != 0) {
			if (word_44DCA < CRASH_ROLL_DISTANCE_LIMIT)
				word_44DCA = LEGACY_S16_WRAP_ADD(word_44DCA,
					CRASH_ROLL_ADVANCE_STEP);
			if (byte_4393C == 1 &&
				word_44DCA > CRASH_ROLL_STAGE_THRESHOLD)
				byte_4393C = LEGACY_S8_WRAP_ADD(byte_4393C, 1);
			if (byte_4393C == 2) {
				if (LEGACY_S16_WRAP_ADD(
					multiply_and_scale(cos_fast(track_angle),
						LEGACY_S16_WRAP_SUB(trackcenterpos[startrow2],
							LEGACY_S16_FROM_BITS((legacy_u16)
								LEGACY_S32_SAR(
									state.playerstate.car_posWorld1.lz,
									CAR_WORLD_POSITION_SHIFT)))),
					multiply_and_scale(sin_fast(track_angle),
						LEGACY_S16_WRAP_SUB(trackcenterpos2[startcol2],
							LEGACY_S16_FROM_BITS((legacy_u16)
								LEGACY_S32_SAR(
									state.playerstate.car_posWorld1.lx,
									CAR_WORLD_POSITION_SHIFT))))) <=
					CRASH_ROLL_APPROACH_DISTANCE) {
					if (state.playerstate.car_speed != 0)
						player_op(INPUT_BRAKE_FLAG);
					else
						byte_4393C = 0;
				} else if (state.playerstate.car_speed <
					CRASH_ROLL_SPEED_LIMIT) {
					player_op(INPUT_ACCELERATE_FLAG);
				} else {
					player_op(0);
				}
			}
		}
	}
}
