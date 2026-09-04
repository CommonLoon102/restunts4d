#ifndef RESTUNTS_AUDIO_INTERNAL_H
#define RESTUNTS_AUDIO_INTERNAL_H

#include "audio.h"
#include "legacy.h"

#define AUDIO_CAR_STATE_RECORD_COUNT 0x28U
#define AUDIO_CAR_STATE_RECORD_SIZE  0x22U

extern legacy_s8 audio_music_enabled;
extern legacy_s8 audio_effects_enabled;
extern legacy_s16 audio_update_lock;
extern legacy_u8 dos_audio_special_mode;
extern legacy_u8 dos_audio_context_count;
extern legacy_u8 audio_channels[];
extern legacy_u8* audio_sfx_channels;
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
extern legacy_u8 far* audio_car_state_records;
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
