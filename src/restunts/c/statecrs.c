#include "externs.h"
#include "legacy.h"
#include "math.h"

#ifndef RESTUNTS_HEADLESS
extern legacy_s32 gState_travDist;
extern legacy_s16 gState_total_finish_time;
extern legacy_s16 gState_144;
extern legacy_s16 gState_pEndFrame;
extern legacy_s16 gState_oEndFrame;
extern legacy_s16 gState_penalty;
extern legacy_s16 gState_impactSpeed;
extern legacy_s16 gState_topSpeed;
extern legacy_s16 gState_jumpCount;
#endif

extern legacy_s16 audio_player_engine_channel;
extern legacy_s8 audio_car_state_ready;
extern legacy_s16 audio_opponent_engine_channel;

void state_op_unk(legacy_s16 kind_arg, legacy_s16 base_angle_arg, legacy_s16 energy_offset_arg) {
	legacy_s16 kind;
	legacy_s16 base_angle;
	legacy_s16 energy_offset;
	legacy_s16 angular_range;
	legacy_s16 type_base;
	legacy_s16 lifetime_scale;
	legacy_s16 particle_limit;
	legacy_s16 free_count;
	legacy_s16 emitted;
	legacy_s16 particle_angle;
	legacy_s16 particle_timer;
	legacy_s16 particle_lifetime;
	legacy_s16 random_value;
	legacy_s16 slot;

	kind = (legacy_s16)kind_arg;
	base_angle = (legacy_s16)base_angle_arg;
	energy_offset = (legacy_s16)energy_offset_arg;
	if (kind < 2) {
		angular_range = 0x400;
		particle_limit = 0x12;
		type_base = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_MUL(kind, 4), 4);
		lifetime_scale = 6;
	} else {
		base_angle = LEGACY_S16_WRAP_SUB(base_angle, 0x60);
		angular_range = 0xC0;
		particle_limit = 8;
		type_base = 0;
		lifetime_scale = 1;
	}

	state.field_42A = 1;
	free_count = 0;
	for (slot = 0; slot < 24; slot++) {
		if (state.field_38E[slot] == 0)
			free_count = LEGACY_S16_WRAP_ADD(free_count, 1);
	}
	if (free_count > particle_limit)
		free_count = particle_limit;

	emitted = 0;
	for (slot = 0; slot < 24 && emitted < free_count; slot++) {
		if (state.field_38E[slot] != 0)
			continue;

		state.field_443[slot] = (legacy_u8)kind;
		state.field_42B[slot] = (legacy_u8)(
			((legacy_u8)emitted & 3U) + (legacy_u8)type_base);
		state.game_longs1[slot] = 0;
		state.game_longs2[slot] = 0;
		state.game_longs3[slot] = 0;

		random_value = (legacy_s16)get_kevinrandom();
		state.field_2FE[slot] = LEGACY_S16_WRAP_MUL(random_value, 4);
		random_value = (legacy_s16)get_kevinrandom();
		state.field_32E[slot] = LEGACY_S16_WRAP_MUL(random_value, 4);

		particle_angle = LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_S32_DIV_OR_ZERO(
				LEGACY_S32_WRAP_MUL(
					(legacy_s32)angular_range, (legacy_s32)emitted),
				(legacy_s32)free_count));
		particle_angle = LEGACY_S16_WRAP_ADD(particle_angle, base_angle);
		state.field_35E[slot] = LEGACY_S16_FROM_BITS(
			(legacy_u16)particle_angle & 0x03FFU);

		random_value = (legacy_s16)get_kevinrandom();
		particle_timer = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_SAR2(LEGACY_S16_WRAP_MUL(random_value, 6)),
			energy_offset);
		particle_timer = LEGACY_S16_WRAP_ADD(particle_timer, 0x180);
		state.field_38E[slot] = particle_timer;

		particle_lifetime = LEGACY_S16_SAR2(
			LEGACY_S16_WRAP_MUL(lifetime_scale, particle_timer));
		LEGACY_WRITE_U16_LE(
			&state.field_3BE[slot * 2], particle_lifetime);
		emitted = LEGACY_S16_WRAP_ADD(emitted, 1);
	}
}

void sub_19BA0(void) {
	struct VECTOR direction;
	struct VECTOR movement;
	struct MATRIX* rotation;
	legacy_s16 particle_velocity;
	legacy_s32 ground_position;
	legacy_u8 any_active;
	legacy_s16 slot;

	any_active = 0;
	for (slot = 0; slot < 24; slot++) {
		if (state.field_38E[slot] == 0)
			continue;

		direction.x = 0;
		direction.y = 0;
		direction.z = state.field_38E[slot];
		rotation = mat_rot_zxy(0, 0, state.field_35E[slot], 1);
		mat_mul_vector(&direction, rotation, &movement);
		state.game_longs1[slot] = LEGACY_S32_WRAP_ADD_S16(
			state.game_longs1[slot], movement.x);
		state.game_longs3[slot] = LEGACY_S32_WRAP_ADD_S16(
			state.game_longs3[slot], movement.z);

		particle_velocity = LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(
			&state.field_3BE[slot * 2]));
		particle_velocity = LEGACY_S16_WRAP_SUB(particle_velocity, 0x13);
		LEGACY_WRITE_U16_LE(
			&state.field_3BE[slot * 2], particle_velocity);
		state.game_longs2[slot] = LEGACY_S32_WRAP_ADD_S16(
			state.game_longs2[slot], particle_velocity);

		if (framespersec == 10) {
			particle_velocity = LEGACY_S16_WRAP_SUB(
				particle_velocity, 0x13);
			LEGACY_WRITE_U16_LE(
				&state.field_3BE[slot * 2], particle_velocity);
			state.game_longs2[slot] = LEGACY_S32_WRAP_ADD_S16(
				state.game_longs2[slot], particle_velocity);
		}

		ground_position = LEGACY_S32_WRAP_ADD(
			(legacy_s32)state.game_longs2[slot],
			(legacy_s32)state.playerstate.car_posWorld1.ly);
		if (ground_position < 0) {
			state.field_38E[slot] = 0;
			continue;
		}

		any_active = 1;
		state.field_2FE[slot] = LEGACY_S16_WRAP_ADD(
			state.field_2FE[slot], 0x10);
		state.field_32E[slot] = LEGACY_S16_WRAP_ADD(
			state.field_32E[slot], 0x10);
	}

	state.field_42A = any_active;
}

// previously set_AV_event_triggers
void update_crash_state(legacy_s16 arg_someFlag, legacy_s16 arg_MplayerFlag) {
	legacy_s8 var_2;
	struct CARSTATE* var_cState;
//	ported_update_crash_state_(arg_someFlag, arg_MplayerFlag);
//	return;
	if (arg_MplayerFlag == 0)
		goto loc_195FC;
	if (arg_MplayerFlag == 1)
		goto loc_19612;
	goto loc_19601;
loc_195FC:
    var_cState = &state.playerstate;
loc_19601:
	if (var_cState->car_crashBmpFlag == 0)
		goto loc_1961A;
	return ;
loc_19612:
	var_cState = &state.opponentstate;
    goto loc_19601;
loc_1961A:
	var_2 = 0;
	if (arg_someFlag == 1)
		goto loc_1967F;
	if (arg_someFlag != 2)
		goto loc_1962E;
	goto loc_196DE;
loc_1962E:
	if (arg_someFlag != 3)
		goto loc_19636;
	goto loc_19730;
loc_19636:
	if (arg_someFlag == 4)
		goto loc_19642;
	if (arg_someFlag == 5)
		goto loc_19676;
	goto loc_1964E;
loc_19642:
    state.game_frame_in_sec = 1;
    state.game_frames_per_sec = 1;
loc_1964E:
	if (var_2 == 0)
		goto loc_19664;
	var_cState->car_speed2 = 0;
	var_cState->car_speed = 0;
loc_19664:
	if (arg_MplayerFlag != 0)
		goto loc_1966D;
	goto loc_19760;
loc_1966D:
	state.game_oEndFrame = state.game_frame;
	goto loc_19766;
loc_19676:
    arg_someFlag = 1;
    var_2 = 1;
loc_1967F:
	var_cState->car_crashBmpFlag = 1;
	state_op_unk(arg_MplayerFlag, var_cState->car_rotate.x, 0);
	if (arg_MplayerFlag != 0)
		goto loc_196B3;
	state.game_impactSpeed = var_cState->car_speed2;
	state.game_frames_per_sec = LEGACY_S16_FROM_BITS(
		LEGACY_U16_SHL(framespersec, 2U));
loc_196B3:
#ifndef RESTUNTS_HEADLESS
	if (is_in_replay != 0)
		goto loc_1964E;
	if (audio_car_state_ready == 0)
		goto loc_1964E;
	if (arg_MplayerFlag != 0)
		goto loc_196CE;
	audio_function2_wrap(audio_player_engine_channel);
	goto loc_196D2;
loc_196CE:
	audio_function2_wrap(audio_opponent_engine_channel);
loc_196D2:
#endif
	goto loc_1964E;
loc_196DE:
#ifndef RESTUNTS_HEADLESS
	if (is_in_replay != 0)
		goto loc_19704;
	if (audio_car_state_ready == 0)
		goto loc_19704;
	if (arg_MplayerFlag != 0)
		goto loc_196F8;
	audio_function2_wrap(audio_player_engine_channel);
	goto loc_196FC;
loc_196F8:
	audio_function2_wrap(audio_opponent_engine_channel);
loc_196FC:
#endif
loc_19704:
	var_cState->car_crashBmpFlag = 2;
	var_2 = 1;
	if (arg_MplayerFlag == 0)
		goto loc_19719;
	goto loc_1964E;
loc_19719:
	state.game_impactSpeed = var_cState->car_speed2;
	state.game_frames_per_sec = LEGACY_S16_FROM_BITS(
		LEGACY_U16_SHL(framespersec, 2U));
loc_19729:
	goto loc_1964E;
loc_19730:
	var_cState->car_crashBmpFlag = 3;
	if (arg_MplayerFlag != 0)
		goto loc_19752;
	state.game_total_finish = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_ADD(state.game_frame, state.game_penalty),
		elapsed_time1);
	state.game_frames_per_sec = framespersec;
	goto loc_19729;
loc_19752:
	state.field_144 = LEGACY_S16_WRAP_ADD(
		state.game_frame, elapsed_time1);
	goto loc_1964E;
loc_19760:
	state.game_pEndFrame = state.game_frame;
loc_19766:
	if (state.game_3F6autoLoadEvalFlag == 0 && arg_MplayerFlag == 0)
		state.game_3F6autoLoadEvalFlag = arg_someFlag;
#ifndef RESTUNTS_HEADLESS
	if ((byte_43966 & 4) == 0) {
		// These copied values are used by the evaluation screen.
		gState_travDist = state.game_travDist;
		gState_frame = state.game_frame;
		gState_total_finish_time = state.game_total_finish;
		gState_144 = state.field_144;
		gState_pEndFrame = state.game_pEndFrame;
		gState_oEndFrame = state.game_oEndFrame;
		gState_penalty = state.game_penalty;
		gState_impactSpeed = state.game_impactSpeed;
		gState_topSpeed = state.game_topSpeed;
		gState_jumpCount = state.game_jumpCount;
	}
#endif
	return;
}
