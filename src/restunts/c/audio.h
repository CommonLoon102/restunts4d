#ifndef RESTUNTS_AUDIO_H
#define RESTUNTS_AUDIO_H

#include <stddef.h>
#include "legacy.h"

#define AUDIO_TIMER_COUNT   25U
#define AUDIO_CHANNEL_COUNT 24U
#define AUDIO_CONTEXT_COUNT 16U

#pragma pack (push, 1)

/* A far pointer as stored in the original DOS data segment. */
struct AUDIO_FAR_POINTER {
	legacy_u16 offset;
	legacy_u16 segment;
};

struct AUDIO_ENGINE_DEFINITION {
	legacy_u16 sample_count;
	legacy_u8 unknown_02[4];
	legacy_u8 initialized;
	legacy_u8 unknown_07;
	struct AUDIO_FAR_POINTER resources[10];
};

struct AUDIO_TIMER {
	legacy_u8 active;
	legacy_u8 engine_active;
	legacy_s16 channel;
	legacy_u16 current_volume;
	legacy_u32 current_pitch;
	legacy_u8 target_volume;
	legacy_u8 unknown_0B;
	legacy_u16 target_pitch;
	legacy_u8 last_volume;
	legacy_u8 unknown_0F;
	legacy_u16 last_pitch;
	legacy_s16 engine_context;
	legacy_s16 effect_channel;
	legacy_s16 secondary_effect_channel;
	legacy_u16 sample_count;
	legacy_u8 parameters_changed;
	legacy_u8 restart_engine;
	struct AUDIO_ENGINE_DEFINITION definition;
};

struct AUDIO_CONTEXT {
	legacy_u8 channel;
	legacy_u8 state;
	legacy_u8 priority;
	legacy_u8 unknown_03[5];
	legacy_u32 age;
	legacy_u32 fade_out_flag;
	struct AUDIO_FAR_POINTER resource;
	legacy_s16 level;
	legacy_u8 envelope_state;
	legacy_u8 unknown_17;
	legacy_u16 modulation_delay;
	legacy_u16 modulation_count;
	legacy_s16 modulation;
	legacy_u16 sequence_delay;
	legacy_u16 sequence_count;
	legacy_u8 sequence_value;
	legacy_u8 unknown_23;
	legacy_u16 modulation_step;
	legacy_u8 modulation_direction;
	legacy_u8 modulation_tick;
	legacy_u8 sequence_tick;
	legacy_u8 sequence_index;
	legacy_u16 timer_offset;
	legacy_u8 driver_channel;
	legacy_u8 unknown_2D;
};

#pragma pack (pop)

typedef char audio_far_pointer_must_be_4_bytes[
	(sizeof(struct AUDIO_FAR_POINTER) == 4) ? 1 : -1];
typedef char audio_engine_definition_must_be_48_bytes[
	(sizeof(struct AUDIO_ENGINE_DEFINITION) == 0x30) ? 1 : -1];
typedef char audio_timer_must_be_76_bytes[
	(sizeof(struct AUDIO_TIMER) == 0x4C) ? 1 : -1];
typedef char audio_context_must_be_46_bytes[
	(sizeof(struct AUDIO_CONTEXT) == 0x2E) ? 1 : -1];
typedef char audio_timer_definition_must_be_at_1C[
	(offsetof(struct AUDIO_TIMER, definition) == 0x1C) ? 1 : -1];
typedef char audio_context_fade_out_flag_must_be_at_0C[
	(offsetof(struct AUDIO_CONTEXT, fade_out_flag) == 0x0C) ? 1 : -1];
typedef char audio_context_resource_must_be_at_10[
	(offsetof(struct AUDIO_CONTEXT, resource) == 0x10) ? 1 : -1];
typedef char audio_context_timer_offset_must_be_at_2A[
	(offsetof(struct AUDIO_CONTEXT, timer_offset) == 0x2A) ? 1 : -1];

extern struct AUDIO_TIMER audio_timers[AUDIO_TIMER_COUNT];
extern struct AUDIO_CONTEXT dos_audio_contexts[AUDIO_CONTEXT_COUNT];

#endif
