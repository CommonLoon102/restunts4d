#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../c/audio_car.c"
#undef printf

legacy_s8 audio_car_state_ready, audio_player_car_flags, audio_opponent_car_flags;
legacy_s16 audio_player_engine_channel, audio_opponent_engine_channel;
legacy_s16 audio_car_state_read_index, audio_car_state_write_index;
legacy_s16 camera_track_height_offset;
legacy_u8 audio_previous_replay_mode;
struct AUDIO_CAR_STATE *audio_car_state_records;
static uint64_t trace_hash = UINT64_C(1469598103934665603);
static struct AUDIO_CAR_STATE records[AUDIO_CAR_STATE_RECORD_COUNT];
static struct VECTOR cameras[2];
static void trace_word(legacy_u16 value)
{
	trace_hash = (trace_hash ^ (value & 255U)) * UINT64_C(1099511628211);
	trace_hash = (trace_hash ^ (value >> 8)) * UINT64_C(1099511628211);
}
void audio_start_engine(legacy_s16 channel)
{
	trace_word(1);
	trace_word(channel);
}
void audio_stop_engine(legacy_s16 channel)
{
	trace_word(2);
	trace_word(channel);
}
void audio_play_paved_skid(legacy_s16 channel)
{
	trace_word(3);
	trace_word(channel);
}
void audio_play_offroad_skid(legacy_s16 channel)
{
	trace_word(4);
	trace_word(channel);
}
void audio_stop_skid_sound(legacy_s16 channel)
{
	trace_word(5);
	trace_word(channel);
}
void audio_reset_channels(void)
{
	trace_word(6);
}
static void record_result(void)
{
	unsigned i;
	const legacy_u8 *bytes = (const legacy_u8 *)records;
	trace_word(audio_car_state_ready);
	trace_word(audio_player_car_flags);
	trace_word(audio_opponent_car_flags);
	trace_word(audio_car_state_read_index);
	trace_word(audio_car_state_write_index);
	trace_word(audio_previous_replay_mode);
	for (i = 0; i < sizeof(records); i++) {
		trace_word(bytes[i]);
	}
}
static void reset_audio_car(unsigned index)
{
	memset(&state, 0, sizeof(state));
	memset(records, 0x5a, sizeof(records));
	audio_car_state_records = records;
	audio_car_state_read_index = 7;
	audio_car_state_write_index = 39;
	audio_player_engine_channel = 17;
	audio_opponent_engine_channel = 21;
	audio_car_state_ready = index % 2U;
	audio_previous_replay_mode = index % 3U;
	state.playerstate.car_previous_position.lx = 0x7ffffff0L;
	state.playerstate.car_previous_position.ly = -0x12345L;
	state.playerstate.car_previous_position.lz = 0x87654L;
	state.playerstate.car_position.lx = -0x7fffffffL;
	state.playerstate.car_position.ly = 0x10002L;
	state.playerstate.car_position.lz = -1;
	state.opponentstate.car_previous_position.lx = -1;
	state.opponentstate.car_previous_position.ly = -33;
	state.opponentstate.car_previous_position.lz = 32;
	state.opponentstate.car_position.lx = 12345;
	state.opponentstate.car_position.ly = 23456;
	state.opponentstate.car_position.lz = -34567;
	state.playerstate.car_currpm = 32767;
	state.opponentstate.car_currpm = -32768;
	cameras[0].x = 32760;
	cameras[0].y = -32760;
	cameras[0].z = 13;
	cameras[1].x = -27;
	cameras[1].y = 32760;
	cameras[1].z = -40;
	trackside_camera_positions = cameras;
	camera_track_height_offset = 32760;
	state.game_follow_camera_position[0] = cameras[0];
	state.game_follow_camera_position[1] = cameras[1];
	state.game_player_camera_previous = cameras[1];
	state.game_opponent_camera_previous = cameras[0];
	state.game_trackside_camera_index[0] = 0;
	state.game_trackside_camera_index[1] = 1;
	trace_word(index);
}
static void test_recording_modes(void)
{
	unsigned mode, opponent, follow, flags, index = 0;
	for (mode = 0; mode < 6U; mode++) {
		for (opponent = 0; opponent < 2U; opponent++) {
			for (follow = 0; follow <= opponent; follow++) {
				for (flags = 0; flags < 16U; flags++) {
					reset_audio_car(index++);
					gameconfig.game_opponenttype = opponent;
					followOpponentFlag = follow;
					cameramode = mode;
					is_in_replay = 0;
					audio_player_car_flags = flags;
					audio_opponent_car_flags = flags ^ 7U;
					state.playerstate.car_sound_flags = flags ^ 15U;
					state.opponentstate.car_sound_flags = flags;
					audio_carstate();
					record_result();
					audio_carstate();
					record_result();
				}
			}
		}
	}
}
static void test_replay_shutdown(void)
{
	unsigned replay, opponent, flags, ready, index = 400;
	for (replay = 1; replay <= 2U; replay++) {
		for (opponent = 0; opponent < 2U; opponent++) {
			for (flags = 0; flags < 16U; flags++) {
				for (ready = 0; ready < 2U; ready++) {
					reset_audio_car(index++);
					gameconfig.game_opponenttype = opponent;
					is_in_replay = replay;
					audio_player_car_flags = flags;
					audio_opponent_car_flags = flags ^ 7U;
					audio_car_state_ready = ready;
					audio_carstate();
					record_result();
					audio_carstate();
					record_result();
				}
			}
		}
	}
}
int main(void)
{
	test_recording_modes();
	test_replay_shutdown();
	assert(trace_hash == UINT64_C(0xafadd350106660b7));
	printf("test-audio-car: passed\n");
	return 0;
}
