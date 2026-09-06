#include "audio.h"
#include "audio_internal.h"
#include "externs.h"
#include "game_input.h"
#include "legacy.h"
#include "math.h"
#include "platform.h"
#include "camera.h"
#include "car_audio.h"

#define AUDIO_TRACK_CAMERA_VERTICAL_OFFSET 90
#define AUDIO_CAR_COUNT_WITH_OPPONENT 2

void audio_play_bump(legacy_s16 channel);
void audio_play_scrape(legacy_s16 channel);
void audio_start_engine(legacy_s16 channel);
void audio_stop_engine(legacy_s16 channel);
void audio_play_paved_skid(legacy_s16 channel);
void audio_play_offroad_skid(legacy_s16 channel);
void audio_stop_skid_sound(legacy_s16 channel);

static legacy_s16 audio_carstate_position(legacy_s32 position)
{
	return position_to_word(position);
}

/* Each car is logged as its offset from the camera, before and after. */
static void audio_carstate_offsets(struct VECTOR far* previous,
	struct VECTOR far* current, const struct VECTOR* camera_previous,
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

	desired = (legacy_u8)carstate->car_sound_flags;
	if ((desired & CAR_SOUND_ENGINE_ACTIVE_FLAG) != 0) {
		if ((flags & CAR_SOUND_ENGINE_ACTIVE_FLAG) == 0) {
			flags = (legacy_u8)(flags | CAR_SOUND_ENGINE_ACTIVE_FLAG);
			audio_start_engine(channel);
		}
	} else if ((flags & CAR_SOUND_ENGINE_ACTIVE_FLAG) != 0) {
		flags = (legacy_u8)(flags - CAR_SOUND_ENGINE_ACTIVE_FLAG);
		audio_stop_engine(channel);
	}

	if ((desired & CAR_SOUND_SKID_MASK) != 0) {
		if ((flags & CAR_SOUND_SKID_MASK) ==
			(desired & CAR_SOUND_SKID_MASK))
			return flags;
		if ((flags & CAR_SOUND_SKID_MASK) == 0) {
			if ((desired & CAR_SOUND_SKID_PAVED_FLAG) != 0) {
				audio_play_paved_skid(channel);
				return (legacy_u8)(flags + CAR_SOUND_SKID_PAVED_FLAG);
			}
			audio_play_offroad_skid(channel);
			return (legacy_u8)(flags + CAR_SOUND_SKID_OFFROAD_FLAG);
		}
	} else if ((flags & CAR_SOUND_SKID_MASK) == 0) {
		return flags;
	}

	if ((flags & CAR_SOUND_SKID_PAVED_FLAG) != 0)
		flags = (legacy_u8)(flags - CAR_SOUND_SKID_PAVED_FLAG);
	if ((flags & CAR_SOUND_SKID_OFFROAD_FLAG) != 0)
		flags = (legacy_u8)(flags - CAR_SOUND_SKID_OFFROAD_FLAG);
	audio_stop_skid_sound(channel);
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
			if (((legacy_u8)audio_player_car_flags &
				CAR_SOUND_SKID_MASK) != 0)
				audio_stop_skid_sound(audio_player_engine_channel);
			if (((legacy_u8)audio_player_car_flags &
				CAR_SOUND_ENGINE_ACTIVE_FLAG) != 0)
				audio_stop_engine(audio_player_engine_channel);
			if (gameconfig.game_opponenttype != 0) {
				if (((legacy_u8)audio_opponent_car_flags &
					CAR_SOUND_SKID_MASK) != 0)
					audio_stop_skid_sound(audio_opponent_engine_channel);
				if (((legacy_u8)audio_opponent_car_flags &
					CAR_SOUND_ENGINE_ACTIVE_FLAG) != 0)
					audio_stop_engine(audio_opponent_engine_channel);
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
		(legacy_s32)state.playerstate.car_previous_position.lx);
	player_previous.y = audio_carstate_position(
		(legacy_s32)state.playerstate.car_previous_position.ly);
	player_previous.z = audio_carstate_position(
		(legacy_s32)state.playerstate.car_previous_position.lz);
	player_current.x = audio_carstate_position(
		(legacy_s32)state.playerstate.car_position.lx);
	player_current.y = audio_carstate_position(
		(legacy_s32)state.playerstate.car_position.ly);
	player_current.z = audio_carstate_position(
		(legacy_s32)state.playerstate.car_position.lz);

	if (gameconfig.game_opponenttype != 0) {
		opponent_previous.x = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_previous_position.lx);
		opponent_previous.y = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_previous_position.ly);
		opponent_previous.z = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_previous_position.lz);
		opponent_current.x = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_position.lx);
		opponent_current.y = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_position.ly);
		opponent_current.z = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_position.lz);
	}

	if (cameramode == CAMERA_MODE_FOLLOW) {
		camera_current = state.game_follow_camera_position[(legacy_u8)followOpponentFlag];
		camera_previous = followOpponentFlag != 0 ?
			state.game_opponent_camera_previous : state.game_player_camera_previous;
	} else if (cameramode == CAMERA_MODE_TRACKSIDE) {
		track_index = LEGACY_S16_FROM_BITS((legacy_u16)(legacy_s8)
			state.game_trackside_camera_index[(legacy_u8)followOpponentFlag]);
		camera_current.x = trackside_camera_positions[track_index].x;
		camera_current.y = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(trackside_camera_positions[track_index].y,
				camera_track_height_offset),
			AUDIO_TRACK_CAMERA_VERTICAL_OFFSET);
		camera_current.z = trackside_camera_positions[track_index].z;
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
		car_count = AUDIO_CAR_COUNT_WITH_OPPONENT;
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
