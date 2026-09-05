#include "externs.h"
#include "legacy.h"
#include "math.h"

#define PARTICLE_SLOT_COUNT 24
#define CAR_CRASH_PARTICLE_KIND_COUNT 2
#define CAR_CRASH_PARTICLE_LIMIT 18
#define CAR_CRASH_PARTICLE_LIFETIME_SCALE 6
#define OBJECT_PARTICLE_ANGLE_OFFSET 96
#define OBJECT_PARTICLE_ANGULAR_RANGE 192
#define OBJECT_PARTICLE_LIMIT 8
#define PARTICLE_TYPE_VARIANT_COUNT 4
#define PARTICLE_TYPE_VARIANT_MASK 3U
#define PARTICLE_RANDOM_ROTATION_SCALE 4
#define PARTICLE_RANDOM_SPEED_SCALE 6
#define PARTICLE_FORWARD_SPEED_BIAS 384
#define PARTICLE_GRAVITY_STEP 19
#define PARTICLE_ROTATION_STEP 16
#define LOW_FRAME_RATE 10

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

#ifndef RESTUNTS_HEADLESS
static void stop_car_engine_audio(legacy_s16 player_flag) {
	if (is_in_replay != 0 || audio_car_state_ready == 0)
		return;

	if (player_flag == 0)
		audio_function2_wrap(audio_player_engine_channel);
	else
		audio_function2_wrap(audio_opponent_engine_channel);
}
#endif

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
	if (kind < CAR_CRASH_PARTICLE_KIND_COUNT) {
		angular_range = ANGLE_FULL_TURN;
		particle_limit = CAR_CRASH_PARTICLE_LIMIT;
		type_base = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_MUL(kind, PARTICLE_TYPE_VARIANT_COUNT),
			PARTICLE_TYPE_VARIANT_COUNT);
		lifetime_scale = CAR_CRASH_PARTICLE_LIFETIME_SCALE;
	} else {
		base_angle = LEGACY_S16_WRAP_SUB(
			base_angle, OBJECT_PARTICLE_ANGLE_OFFSET);
		angular_range = OBJECT_PARTICLE_ANGULAR_RANGE;
		particle_limit = OBJECT_PARTICLE_LIMIT;
		type_base = 0;
		lifetime_scale = 1;
	}

	state.field_42A = 1;
	free_count = 0;
	for (slot = 0; slot < PARTICLE_SLOT_COUNT; slot++) {
		if (state.field_38E[slot] == 0)
			free_count = LEGACY_S16_WRAP_ADD(free_count, 1);
	}
	if (free_count > particle_limit)
		free_count = particle_limit;

	emitted = 0;
	for (slot = 0; slot < PARTICLE_SLOT_COUNT && emitted < free_count; slot++) {
		if (state.field_38E[slot] != 0)
			continue;

		state.field_443[slot] = (legacy_u8)kind;
		state.field_42B[slot] = (legacy_u8)(
			((legacy_u8)emitted & PARTICLE_TYPE_VARIANT_MASK) +
			(legacy_u8)type_base);
		state.game_longs1[slot] = 0;
		state.game_longs2[slot] = 0;
		state.game_longs3[slot] = 0;

		random_value = (legacy_s16)get_kevinrandom();
		state.field_2FE[slot] = LEGACY_S16_WRAP_MUL(
			random_value, PARTICLE_RANDOM_ROTATION_SCALE);
		random_value = (legacy_s16)get_kevinrandom();
		state.field_32E[slot] = LEGACY_S16_WRAP_MUL(
			random_value, PARTICLE_RANDOM_ROTATION_SCALE);

		particle_angle = LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_S32_DIV_OR_ZERO(
				LEGACY_S32_WRAP_MUL(
					(legacy_s32)angular_range, (legacy_s32)emitted),
				(legacy_s32)free_count));
		particle_angle = LEGACY_S16_WRAP_ADD(particle_angle, base_angle);
		state.field_35E[slot] = LEGACY_S16_FROM_BITS(
			(legacy_u16)particle_angle & ANGLE_MASK);

		random_value = (legacy_s16)get_kevinrandom();
		particle_timer = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_SAR2(LEGACY_S16_WRAP_MUL(
				random_value, PARTICLE_RANDOM_SPEED_SCALE)),
			energy_offset);
		particle_timer = LEGACY_S16_WRAP_ADD(
			particle_timer, PARTICLE_FORWARD_SPEED_BIAS);
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
	for (slot = 0; slot < PARTICLE_SLOT_COUNT; slot++) {
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
		particle_velocity = LEGACY_S16_WRAP_SUB(
			particle_velocity, PARTICLE_GRAVITY_STEP);
		LEGACY_WRITE_U16_LE(
			&state.field_3BE[slot * 2], particle_velocity);
		state.game_longs2[slot] = LEGACY_S32_WRAP_ADD_S16(
			state.game_longs2[slot], particle_velocity);

		if (framespersec == LOW_FRAME_RATE) {
			particle_velocity = LEGACY_S16_WRAP_SUB(
				particle_velocity, PARTICLE_GRAVITY_STEP);
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
			state.field_2FE[slot], PARTICLE_ROTATION_STEP);
		state.field_32E[slot] = LEGACY_S16_WRAP_ADD(
			state.field_32E[slot], PARTICLE_ROTATION_STEP);
	}

	state.field_42A = any_active;
}

// previously set_AV_event_triggers
void update_crash_state(legacy_s16 arg_someFlag, legacy_s16 arg_MplayerFlag) {
	legacy_s8 var_2;
	struct CARSTATE* var_cState;
	if (arg_MplayerFlag == 0)
		var_cState = &state.playerstate;
	else if (arg_MplayerFlag == 1)
		var_cState = &state.opponentstate;
	if (var_cState->car_crashBmpFlag != 0)
		return;

	var_2 = 0;
	switch (arg_someFlag) {
	case 5:
		arg_someFlag = 1;
		var_2 = 1;
		/* fall through */
	case 1:
		var_cState->car_crashBmpFlag = 1;
		state_op_unk(arg_MplayerFlag, var_cState->car_rotate.x, 0);
		if (arg_MplayerFlag == 0) {
			state.game_impactSpeed = var_cState->car_speed2;
			state.game_frames_per_sec = LEGACY_S16_FROM_BITS(
				LEGACY_U16_SHL(framespersec, 2U));
		}
#ifndef RESTUNTS_HEADLESS
		stop_car_engine_audio(arg_MplayerFlag);
#endif
		break;

	case 2:
#ifndef RESTUNTS_HEADLESS
		stop_car_engine_audio(arg_MplayerFlag);
#endif
		var_cState->car_crashBmpFlag = 2;
		var_2 = 1;
		if (arg_MplayerFlag == 0) {
			state.game_impactSpeed = var_cState->car_speed2;
			state.game_frames_per_sec = LEGACY_S16_FROM_BITS(
				LEGACY_U16_SHL(framespersec, 2U));
		}
		break;

	case 3:
		var_cState->car_crashBmpFlag = 3;
		if (arg_MplayerFlag == 0) {
			state.game_total_finish = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(
					state.game_frame, state.game_penalty),
				elapsed_time1);
			state.game_frames_per_sec = framespersec;
		} else {
			state.field_144 = LEGACY_S16_WRAP_ADD(
				state.game_frame, elapsed_time1);
		}
		break;

	case 4:
		state.game_frame_in_sec = 1;
		state.game_frames_per_sec = 1;
		break;
	}

	if (var_2 != 0) {
		var_cState->car_speed2 = 0;
		var_cState->car_speed = 0;
	}
	if (arg_MplayerFlag == 0)
		state.game_pEndFrame = state.game_frame;
	else
		state.game_oEndFrame = state.game_frame;
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
