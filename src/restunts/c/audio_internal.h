#ifndef RESTUNTS_AUDIO_INTERNAL_H
#define RESTUNTS_AUDIO_INTERNAL_H

#include "audio.h"
#include "legacy.h"
#include "math.h"

#define AUDIO_CAR_STATE_RECORD_COUNT 40U
#define AUDIO_CAR_STATE_RECORD_SIZE 34U
#define AUDIO_CAR_STATE_UNKNOWN_PREFIX_SIZE 6U
#define AUDIO_CAR_STATE_PLAYER_PREVIOUS_OFFSET 6U
#define AUDIO_CAR_STATE_PLAYER_CURRENT_OFFSET 12U
#define AUDIO_CAR_STATE_OPPONENT_PREVIOUS_OFFSET 18U
#define AUDIO_CAR_STATE_OPPONENT_CURRENT_OFFSET 24U
#define AUDIO_CAR_STATE_PLAYER_RPM_OFFSET 30U
#define AUDIO_CAR_STATE_OPPONENT_RPM_OFFSET 32U

#pragma pack (push, 1)

/* One entry of the ring buffer the engine sound is driven from: each car's
   position relative to the camera before and after the frame, plus its rev
   counter. The asm mixer reads these records, so the layout is fixed. */
struct AUDIO_CAR_STATE {
	legacy_u8 unknown_00[AUDIO_CAR_STATE_UNKNOWN_PREFIX_SIZE];
	struct VECTOR player_previous;
	struct VECTOR player_current;
	struct VECTOR opponent_previous;
	struct VECTOR opponent_current;
	legacy_s16 player_rpm;
	legacy_s16 opponent_rpm;
};

#pragma pack (pop)

typedef char audio_car_state_must_be_34_bytes[
	(sizeof(struct AUDIO_CAR_STATE) == AUDIO_CAR_STATE_RECORD_SIZE) ? 1 : -1];
typedef char audio_car_state_player_previous_offset_must_match[
	(offsetof(struct AUDIO_CAR_STATE, player_previous) ==
	AUDIO_CAR_STATE_PLAYER_PREVIOUS_OFFSET) ? 1 : -1];
typedef char audio_car_state_player_current_offset_must_match[
	(offsetof(struct AUDIO_CAR_STATE, player_current) ==
	AUDIO_CAR_STATE_PLAYER_CURRENT_OFFSET) ? 1 : -1];
typedef char audio_car_state_opponent_previous_offset_must_match[
	(offsetof(struct AUDIO_CAR_STATE, opponent_previous) ==
	AUDIO_CAR_STATE_OPPONENT_PREVIOUS_OFFSET) ? 1 : -1];
typedef char audio_car_state_opponent_current_offset_must_match[
	(offsetof(struct AUDIO_CAR_STATE, opponent_current) ==
	AUDIO_CAR_STATE_OPPONENT_CURRENT_OFFSET) ? 1 : -1];
typedef char audio_car_state_player_rpm_offset_must_match[
	(offsetof(struct AUDIO_CAR_STATE, player_rpm) ==
	AUDIO_CAR_STATE_PLAYER_RPM_OFFSET) ? 1 : -1];
typedef char audio_car_state_opponent_rpm_offset_must_match[
	(offsetof(struct AUDIO_CAR_STATE, opponent_rpm) ==
	AUDIO_CAR_STATE_OPPONENT_RPM_OFFSET) ? 1 : -1];

extern legacy_s8 audio_music_enabled;
extern legacy_s8 audio_effects_enabled;
extern legacy_s16 audio_update_lock;
extern legacy_u8 dos_audio_special_mode;
extern legacy_u8 dos_audio_context_count;
extern legacy_u8 dos_audio_uses_direct_channels;
extern legacy_u8 dos_audio_master_state[];
extern legacy_u8 dos_audio_master_volume;
extern void far* dos_audio_driver_binary;
extern legacy_s8 audio_car_state_ready;
extern legacy_s8 audio_player_car_flags;
extern legacy_s8 audio_opponent_car_flags;
extern legacy_s16 audio_player_engine_channel;
extern legacy_s16 audio_opponent_engine_channel;
extern legacy_u8 audio_music_rate;
extern legacy_u8 audio_music_channel_count;
extern legacy_u8 audio_suspended;
extern legacy_u8 audio_music_active;
extern legacy_u8 audio_effect_rate;
extern legacy_u16 audio_engine_value_44d48;
extern legacy_u16 audio_engine_value_454ba;
extern void far* audio_bass_drum_resource;
extern void far* audio_snare_resource;
extern void far* audio_tom_resource;
extern void far* audio_ride_resource;
extern void far* audio_crash_resource;
extern void far* audio_closed_hihat_resource;
extern void far* audio_open_hihat_resource;
extern legacy_s16 audio_car_state_read_index;
extern legacy_s16 audio_car_state_write_index;
extern legacy_s16 audio_car_state_interval;
extern struct AUDIO_CAR_STATE far* audio_car_state_records;
extern legacy_u8 audio_previous_replay_mode;

void far* audio_read_far_pointer(const legacy_u8 far* source);
void audio_write_far_pointer(legacy_u8 far* destination,
	const void far* value);
legacy_u32 audioresource_get_dword(const legacy_u8 far* source);
legacy_u16 audioresource_get_word(const legacy_u8 far* source);
legacy_s8* pad_id(const legacy_s8 far* source);
void sub_18D06(const legacy_u8 far* sample, legacy_s16 interval);
legacy_s16 audio_init_engine(legacy_s16 timer_index, void far* first,
	void far* second, void far* third);
void audio_carstate(void);
void audio_suspend(void);
void audio_resume(void);
legacy_s16 audio_load_dos_driver(const legacy_s8* driver_name,
	legacy_s16 unused1, legacy_s16 unused2);
void audio_reset_channels(void);
void sub_3736A(void);
void audio_driver_timer(void);
void audio_release_channel_range(legacy_s16 first_channel,
	legacy_s16 last_channel);
void audio_update_driver_contexts(void);
void audio_init_chunk(legacy_s16 first_channel, legacy_s16 last_channel,
	void far* resource, legacy_u16 resource_data_offset,
	legacy_u16 rate, legacy_u8 priority);
void audio_init_chunk2(legacy_s16 channel);
legacy_s16 audio_sequence_command_has_byte_argument(
	legacy_u8 command_index);
legacy_s16 audio_start_sample(legacy_u16 value, legacy_s16 handle);
legacy_s16 sub_37470(legacy_s16 channel, legacy_u8 priority);
void sub_374DE(legacy_s16 channel);
legacy_s16 sub_3771E(legacy_s16 channel);
void sub_38156(legacy_s16 index);
void audio_fade_out(legacy_s16 delay_ticks);

#endif
