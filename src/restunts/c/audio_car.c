#include "audio.h"
#include "audio_internal.h"
#include "externs.h"
#include "legacy.h"
#include "math.h"
#include "platform.h"

extern legacy_s16 camera_track_height_offset;

void audio_op_unk3(legacy_s16 channel);
void audio_op_unk4(legacy_s16 channel);
void audio_op_unk(legacy_s16 channel);
void audio_function2(legacy_s16 channel);
void audio_op_unk5(legacy_s16 channel);
void audio_op_unk6(legacy_s16 channel);
void audio_op_unk7(legacy_s16 channel);
void audio_unk3(legacy_u8 flags, legacy_s16 channel);

static legacy_s16 audio_carstate_position(legacy_s32 position)
{
	legacy_u32 bits;

	bits = (legacy_u32)position;
	bits = (bits >> 6) |
		((bits & 0x80000000UL) != 0 ? 0xFC000000UL : 0);
	return LEGACY_S16_FROM_BITS((legacy_u16)bits);
}

/* Each car is logged as its offset from the camera, before and after. */
static void audio_carstate_offsets(struct VECTOR* previous,
	struct VECTOR* current, const struct VECTOR* camera_previous,
	const struct VECTOR* camera_current, const struct VECTOR* car_previous,
	const struct VECTOR* car_current)
{
	previous->x = LEGACY_S16_WRAP_SUB(camera_previous->x, car_previous->x);
	previous->y = LEGACY_S16_WRAP_SUB(camera_previous->y, car_previous->y);
	previous->z = LEGACY_S16_WRAP_SUB(camera_previous->z, car_previous->z);
	current->x = LEGACY_S16_WRAP_SUB(camera_current->x, car_current->x);
	current->y = LEGACY_S16_WRAP_SUB(camera_current->y, car_current->y);
	current->z = LEGACY_S16_WRAP_SUB(camera_current->z, car_current->z);
}

static legacy_u8 audio_carstate_update_flags(struct CARSTATE* carstate,
	legacy_s16 channel, legacy_u8 flags)
{
	legacy_u8 desired;

	desired = (legacy_u8)carstate->field_CF;
	if ((desired & 1U) != 0) {
		if ((flags & 1U) == 0) {
			flags = (legacy_u8)(flags | 1U);
			audio_op_unk(channel);
		}
	} else if ((flags & 1U) != 0) {
		flags = (legacy_u8)(flags - 1U);
		audio_function2(channel);
	}

	if ((desired & 6U) != 0) {
		if ((flags & 6U) == (desired & 6U))
			return flags;
		if ((flags & 6U) == 0) {
			if ((desired & 2U) != 0) {
				audio_op_unk5(channel);
				return (legacy_u8)(flags + 2U);
			}
			audio_op_unk6(channel);
			return (legacy_u8)(flags + 4U);
		}
	} else if ((flags & 6U) == 0) {
		return flags;
	}

	if ((flags & 2U) != 0)
		flags = (legacy_u8)(flags - 2U);
	if ((flags & 4U) != 0)
		flags = (legacy_u8)(flags - 4U);
	audio_op_unk7(channel);
	return flags;
}

void audio_carstate(void)
{
	struct VECTOR player_previous;
	struct VECTOR player_current;
	struct VECTOR opponent_previous;
	struct VECTOR opponent_current;
	struct VECTOR camera_previous;
	struct VECTOR camera_current;
	struct CARSTATE* carstate;
	struct AUDIO_CAR_STATE far* record;
	legacy_s16 track_index;
	legacy_s16 car_count;
	legacy_s16 car_index;
	legacy_u8 flags;
	legacy_s16 channel;

	if (is_in_replay != 0) {
		if (audio_car_state_ready != 0) {
			audio_car_state_read_index = audio_car_state_write_index;
			if (((legacy_u8)audio_player_car_flags & 6U) != 0)
				audio_op_unk7(audio_player_engine_channel);
			if (((legacy_u8)audio_player_car_flags & 1U) != 0)
				audio_function2(audio_player_engine_channel);
			if (gameconfig.game_opponenttype != 0) {
				if (((legacy_u8)audio_opponent_car_flags & 6U) != 0)
					audio_op_unk7(audio_opponent_engine_channel);
				if (((legacy_u8)audio_opponent_car_flags & 1U) != 0)
					audio_function2(audio_opponent_engine_channel);
			}
			audio_car_state_ready = 0;
			audio_player_car_flags = 0;
			audio_opponent_car_flags = 0;
		}
		if ((legacy_u8)audio_previous_replay_mode != (legacy_u8)is_in_replay)
			audio_reset_channels();
		audio_previous_replay_mode = (legacy_u8)is_in_replay;
		return;
	}

	player_previous.x = audio_carstate_position(
		(legacy_s32)state.playerstate.car_posWorld2.lx);
	player_previous.y = audio_carstate_position(
		(legacy_s32)state.playerstate.car_posWorld2.ly);
	player_previous.z = audio_carstate_position(
		(legacy_s32)state.playerstate.car_posWorld2.lz);
	player_current.x = audio_carstate_position(
		(legacy_s32)state.playerstate.car_posWorld1.lx);
	player_current.y = audio_carstate_position(
		(legacy_s32)state.playerstate.car_posWorld1.ly);
	player_current.z = audio_carstate_position(
		(legacy_s32)state.playerstate.car_posWorld1.lz);

	if (gameconfig.game_opponenttype != 0) {
		opponent_previous.x = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_posWorld2.lx);
		opponent_previous.y = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_posWorld2.ly);
		opponent_previous.z = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_posWorld2.lz);
		opponent_current.x = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_posWorld1.lx);
		opponent_current.y = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_posWorld1.ly);
		opponent_current.z = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_posWorld1.lz);
	}

	if (cameramode == 1) {
		camera_current = state.game_vec1[(legacy_u8)followOpponentFlag];
		camera_previous = followOpponentFlag != 0 ?
			state.game_vec4 : state.game_vec3;
	} else if (cameramode == 3) {
		track_index = LEGACY_S16_FROM_BITS((legacy_u16)(legacy_s8)
			state.field_3F7[(legacy_u8)followOpponentFlag]);
		camera_current.x = trackdata9[track_index * 3];
		camera_current.y = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(trackdata9[track_index * 3 + 1],
				camera_track_height_offset), 0x5A);
		camera_current.z = trackdata9[track_index * 3 + 2];
		camera_previous = camera_current;
	} else if (followOpponentFlag != 0) {
		camera_current = opponent_current;
		camera_previous = opponent_previous;
	} else {
		camera_current = player_current;
		camera_previous = player_previous;
	}

	record = &audio_car_state_records[audio_car_state_write_index];
	audio_carstate_offsets(&record->player_previous,
		&record->player_current, &camera_previous, &camera_current,
		&player_previous, &player_current);
	record->player_rpm = state.playerstate.car_currpm;

	car_count = 1;
	if (gameconfig.game_opponenttype != 0) {
		audio_carstate_offsets(&record->opponent_previous,
			&record->opponent_current, &camera_previous,
			&camera_current, &opponent_previous, &opponent_current);
		record->opponent_rpm = state.opponentstate.car_currpm;
		car_count = 2;
	}

	for (car_index = 0; car_index < car_count; car_index++) {
		if (car_index == 0) {
			carstate = &state.playerstate;
			channel = audio_player_engine_channel;
			flags = (legacy_u8)audio_player_car_flags;
		} else {
			carstate = &state.opponentstate;
			channel = audio_opponent_engine_channel;
			flags = (legacy_u8)audio_opponent_car_flags;
		}
		flags = audio_carstate_update_flags(carstate, channel, flags);
		if (car_index == 0)
			audio_player_car_flags = (legacy_s8)flags;
		else
			audio_opponent_car_flags = (legacy_s8)flags;
	}

	audio_car_state_ready = 1;
	audio_car_state_write_index = LEGACY_S16_WRAP_ADD(audio_car_state_write_index, 1);
	if (audio_car_state_write_index == AUDIO_CAR_STATE_RECORD_COUNT)
		audio_car_state_write_index = 0;
	audio_previous_replay_mode = (legacy_u8)is_in_replay;
}
