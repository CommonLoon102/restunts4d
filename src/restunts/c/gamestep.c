#include <dos.h>
#include "restunts.h"

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

	car_count = gameconfig.game_opponenttype == 0 ? 1U : 2U;
	for (car_index = 0; car_index < car_count; car_index++) {
		previous_position = car_index == 0 ?
			&state.game_vec3 : &state.game_vec4;
		*previous_position = state.game_vec1[car_index];
		carstate = car_index == 0 ?
			&state.playerstate : &state.opponentstate;
		car_x = (legacy_s16)LEGACY_S32_SAR(
			carstate->car_posWorld1.lx, 6U);
		car_y = (legacy_s16)LEGACY_S32_SAR(
			carstate->car_posWorld1.ly, 6U);
		car_z = (legacy_s16)LEGACY_S32_SAR(
			carstate->car_posWorld1.lz, 6U);
		target = carstate->car_vec_unk3;
		if ((car_index == 0 &&
			(state.field_45B != 0 || state.field_45C != 0)) ||
			carstate->field_B6 != 0 ||
			carstate->car_crashBmpFlag != 0 ||
			carstate->car_trackdata3_index == -1 ||
			(carstate->field_48 > 0x80 &&
			carstate->field_48 < 0x380)) {
			target.x = car_x;
			target.y = car_y;
			target.z = car_z;
		}

		target_y = LEGACY_S16_WRAP_ADD(car_y, 0x10E);
		delta_y = LEGACY_S16_WRAP_SUB(
			state.game_vec1[car_index].y, target_y);
		if (delta_y != 0) {
			if (delta_y > 0x1E)
				delta_y = 0x1E;
			else if (delta_y < -0x1E)
				delta_y = -0x1E;
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
		if (distance > 0x1C2) {
			adjustment = LEGACY_S16_WRAP_SUB(distance, 0x1C2);
			if (framespersec == 0x14) {
				if (adjustment > 0x78)
					adjustment = 0x78;
			} else if (adjustment > 0xF0) {
				adjustment = 0xF0;
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

		divisor = (legacy_u16)framespersec >> 1;
		if (divisor != 0 &&
			(legacy_u16)state.game_frame % divisor != 0)
			continue;
		nearest_distance = 0x2710;
		for (candidate = 0;
			LEGACY_S8_FROM_BITS(candidate) <
				LEGACY_S8_FROM_BITS(byte_4616E);
			candidate++) {
			delta_x = (legacy_s32)(legacy_s16)
				trackdata9[candidate * 3U] - (legacy_s32)car_x;
			delta_z = (legacy_s32)(legacy_s16)
				trackdata9[candidate * 3U + 2U] - (legacy_s32)car_z;
			absolute_x = delta_x < 0 ? -delta_x : delta_x;
			if (absolute_x >= nearest_distance)
				continue;
			absolute_z = delta_z < 0 ? -delta_z : delta_z;
			if (absolute_z >= nearest_distance)
				continue;
			distance = (legacy_s16)polarRadius2D(
				(legacy_s16)delta_x, (legacy_s16)delta_z);
			if (distance < nearest_distance) {
				state.field_3F7[car_index] = (legacy_s8)candidate;
				nearest_distance = distance;
			}
		}
	}
}

void update_gamestate(void)
{
	legacy_s8 car_input;
	legacy_u16 checkpoint_index;

	car_input = td16_rpl_buffer[state.game_frame];
	if (car_input != 0)
		state.game_inputmode = 1;

	if (word_45A00 == 0 ||
		((legacy_u16)state.game_frame % (legacy_u16)word_45A00) == 0) {
		get_kevinrandom_seed(state.kevinseed);
		checkpoint_index = LEGACY_U16_DIV_OR_ZERO(
			state.game_frame, word_45A00);
		fmemcpy(&cvxptr[checkpoint_index],
			MK_FP(FP_SEG(&state), FP_OFF(&state)),
			sizeof(struct GAMESTATE));
	}

	state.game_frame++;
	if (state.game_3F6autoLoadEvalFlag != 0 &&
		state.game_frame_in_sec < state.game_frames_per_sec) {
		state.game_frame_in_sec++;
		if (state.game_frame_in_sec == state.game_frames_per_sec &&
			byte_449DA == 0) {
			if (state.playerstate.car_crashBmpFlag == 1 &&
				state.playerstate.car_speed2 != 0) {
				state.game_frames_per_sec++;
			} else if (game_replay_mode == 0) {
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
		audio_carstate();
	} else if (game_replay_mode == 1) {
		audio_carstate();
		if (byte_4393C != 0) {
			if (word_44DCA < 0x1C2)
				word_44DCA += 8;
			if (byte_4393C == 1 && word_44DCA > 0x180)
				byte_4393C++;
			if (byte_4393C == 2) {
				if (multiply_and_scale(cos_fast(track_angle),
					trackcenterpos[startrow2] -
					(legacy_s16)LEGACY_S32_SAR(
						state.playerstate.car_posWorld1.lz, 6U)) +
					multiply_and_scale(sin_fast(track_angle),
					trackcenterpos2[startcol2] -
					(legacy_s16)LEGACY_S32_SAR(
						state.playerstate.car_posWorld1.lx, 6U)) <= 0xE4) {
					if (state.playerstate.car_speed != 0)
						player_op(2);
					else
						byte_4393C = 0;
				} else if (state.playerstate.car_speed < 0x500) {
					player_op(1);
				} else {
					player_op(0);
				}
			}
		}
	}
}
