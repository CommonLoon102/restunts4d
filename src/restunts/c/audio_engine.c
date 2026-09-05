#include <stddef.h>
#include "audio.h"
#include "audio_internal.h"
#include "externs.h"
#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "platform.h"
#include "resource.h"

#define AUDIO_DRIVER_TIMER_RATE 22U
#define DOS_SEGMENT_WRAP_PARAGRAPHS 4096U
#define AUDIO_ENGINE_FIRST_SAMPLE_RESOURCE 2U
#define AUDIO_ENGINE_RATE_DIVISOR_OFFSET 14U
#define AUDIO_ENGINE_RATE_BASE_OFFSET 15U
#define AUDIO_ENGINE_RATE_BASE_SHIFT 4U
#define AUDIO_ENGINE_MAX_VOLUME 127U
#define AUDIO_ENGINE_UNSET_VOLUME 255U
#define AUDIO_ENGINE_UNSET_PITCH 65535U
#define AUDIO_MAX_AUDIBLE_DISTANCE 6000
#define AUDIO_SPATIAL_INTERVAL_PERCENT 100U
#define AUDIO_APPROACH_VOLUME_REDUCTION_SHIFT 4U
#define AUDIO_VECTOR_X_OFFSET 0U
#define AUDIO_VECTOR_Y_OFFSET 2U
#define AUDIO_VECTOR_Z_OFFSET 4U

extern legacy_s16 camera_track_height_offset;

void audio_sequence_timer(void);
void add_exit_handler(void (far* exit_handler)(void));
void timer_reg_callback(void (far* callback)(void));
void timer_remove_callback(void (far* callback)(void));
legacy_u32 timer_copy_counter(legacy_u32 ticks);
legacy_u32 timer_wait_for_dx(void);

legacy_u8 audio_music_channel_count;
legacy_u8 audio_suspended;
legacy_u8 audio_music_active;
legacy_u8 audio_music_rate;
legacy_s8 audio_car_state_ready;
legacy_s8 audio_player_car_flags;
legacy_s8 audio_opponent_car_flags;
static legacy_u8 audio_saved_channel_volumes[AUDIO_CHANNEL_COUNT];
static legacy_u8 audio_effect_channel_volumes[AUDIO_CHANNEL_COUNT];
legacy_u8 audio_effect_rate;
legacy_u8 audio_channel_reserved[AUDIO_CHANNEL_COUNT];
static legacy_u8 audio_channel_values[AUDIO_CHANNEL_COUNT];
static legacy_u8 audio_channel_notes[AUDIO_CHANNEL_COUNT];
void far* audio_bass_drum_resource;
void far* audio_snare_resource;
void far* audio_tom_resource;
void far* audio_ride_resource;
void far* audio_crash_resource;
void far* audio_closed_hihat_resource;
void far* audio_open_hihat_resource;
legacy_s16 audio_player_engine_channel;
legacy_s16 audio_opponent_engine_channel;
static legacy_u16 audio_driver_timer_rate;
static legacy_s16 audio_driver_timer_divider;
legacy_u16 audio_engine_value_44d48;
legacy_u16 audio_engine_value_454ba;

void far* audio_read_far_pointer(const legacy_u8 far* source)
{
	return dos_memory_make_pointer(LEGACY_READ_U16_LE(source + 2),
		LEGACY_READ_U16_LE(source));
}

void audio_write_far_pointer(legacy_u8 far* destination,
	const void far* value)
{
	LEGACY_WRITE_U16_LE(destination, dos_memory_pointer_offset(value));
	LEGACY_WRITE_U16_LE(destination + 2, dos_memory_pointer_segment(value));
}

void audio_add_driver_timer(void)
{
	legacy_u16 index;

	for (index = 0; index < AUDIO_TIMER_COUNT; index++)
		audio_timers[index].active = 0;
	audio_driver_timer_rate = AUDIO_DRIVER_TIMER_RATE;
	timer_reg_callback(&audio_driver_timer);
}

void audio_remove_driver_timer(void)
{
	legacy_u16 index;
	legacy_s16 channel;

	for (index = 0; index < AUDIO_TIMER_COUNT; index++) {
		if (audio_timers[index].active == 1) {
			channel = audio_timers[index].channel;
			sub_374DE(channel);
		}
		audio_timers[index].active = 0;
	}
	timer_remove_callback(&audio_driver_timer);
}

legacy_s16 audio_init_engine(legacy_s16 unused_type, void far* source_pointer,
	void far* shape_resources, void far* audio_resources)
{
	const legacy_u8 far* source;
	struct AUDIO_TIMER* timer;
	struct AUDIO_ENGINE_DEFINITION* engine_definition;
	const legacy_u8 far* definition;
	void far* resource;
	legacy_u16 source_offset;
	legacy_u16 source_segment;
	legacy_u16 rate;
	legacy_u16 divisor;
	legacy_u16 index;
	legacy_u16 field;
	legacy_u16 resource_index;
	legacy_s16 channel;

	(void)unused_type;
	for (index = 0; index < AUDIO_TIMER_COUNT; index++) {
		if (audio_timers[index].active == 0)
			break;
	}
	if (index == AUDIO_TIMER_COUNT) {
		fatal_error("InitEngine: All handles used.");
		return -1;
	}

	timer = &audio_timers[index];
	engine_definition = &timer->definition;
	source_offset = (legacy_u16)dos_memory_pointer_offset(source_pointer);
	source_segment = (legacy_u16)dos_memory_pointer_segment(source_pointer);
	for (field = 0; field < AUDIO_ENGINE_DEFINITION_SIZE; field++) {
		source = (const legacy_u8 far*)dos_memory_make_pointer(
			source_segment, source_offset);
		((legacy_u8*)engine_definition)[field] = *source;
		source_offset++;
		if (source_offset == 0)
			source_segment = LEGACY_U16_WRAP_ADD(source_segment,
				DOS_SEGMENT_WRAP_PARAGRAPHS);
	}

	if (engine_definition->initialized == 0) {
		resource = locate_shape_fatal((legacy_s8 far*)shape_resources,
			pad_id((const legacy_s8 far*)audio_read_far_pointer(
				(legacy_u8*)&engine_definition->resources[0])));
		audio_write_far_pointer(
			(legacy_u8*)&engine_definition->resources[0], resource);
		for (resource_index = AUDIO_ENGINE_FIRST_SAMPLE_RESOURCE;
			resource_index < AUDIO_ENGINE_RESOURCE_COUNT;
			resource_index++) {
			resource = init_audio_resources(audio_resources,
				shape_resources,
				pad_id((const legacy_s8 far*)audio_read_far_pointer(
					(legacy_u8*)&engine_definition->resources[
						resource_index])));
			audio_write_far_pointer(
				(legacy_u8*)&engine_definition->resources[resource_index],
				resource);
		}
		engine_definition->initialized = 1;
	}

	channel = sub_37470(-1, AUDIO_ENGINE_MAX_VOLUME);
	timer->channel = channel;
	timer->engine_active = 0;
	timer->current_volume = 0;
	timer->current_pitch = 0;
	timer->target_volume = 0;
	definition = (const legacy_u8 far*)audio_read_far_pointer(
		(legacy_u8*)&engine_definition->resources[0]);
	divisor = definition[AUDIO_ENGINE_RATE_DIVISOR_OFFSET];
	rate = LEGACY_U16_DIV_OR_ZERO(
		engine_definition->sample_count, divisor);
	rate = LEGACY_U16_WRAP_ADD(rate,
		(legacy_u16)((legacy_u16)definition[AUDIO_ENGINE_RATE_BASE_OFFSET] <<
			AUDIO_ENGINE_RATE_BASE_SHIFT));
	timer->target_pitch = rate;
	timer->last_volume = AUDIO_ENGINE_UNSET_VOLUME;
	timer->last_pitch = AUDIO_ENGINE_UNSET_PITCH;
	timer->engine_context = -1;
	timer->effect_channel = -1;
	timer->secondary_effect_channel = -1;
	timer->sample_count = engine_definition->sample_count;
	timer->parameters_changed = 0;
	timer->restart_engine = 0;
	timer->active = 1;
	return (legacy_s16)index;
}

void audio_op_unk(legacy_s16 index)
{
	struct AUDIO_TIMER* timer;
	struct AUDIO_ENGINE_DEFINITION* engine_definition;
	const legacy_u8 far* definition;
	legacy_u16 sample_count;
	legacy_u16 value;
	legacy_u16 divisor;
	legacy_s16 handle;
	legacy_s16 channel;

	timer = &audio_timers[index];
	if (timer->active != 1 || timer->engine_active != 0)
		return;

	handle = timer->channel;
	engine_definition = &timer->definition;
	dos_audio_bind_channel_context(handle,
		audio_read_far_pointer(
			(legacy_u8*)&engine_definition->resources[0]));
	sample_count = engine_definition->sample_count;
	definition = (const legacy_u8 far*)audio_read_far_pointer(
		(legacy_u8*)&engine_definition->resources[0]);
	divisor = definition[AUDIO_ENGINE_RATE_DIVISOR_OFFSET];
	value = LEGACY_U16_DIV_OR_ZERO(sample_count, divisor);
	value = LEGACY_U16_WRAP_ADD(value,
		(legacy_u16)((legacy_u16)definition[AUDIO_ENGINE_RATE_BASE_OFFSET] <<
			AUDIO_ENGINE_RATE_BASE_SHIFT));
	timer->target_pitch = value;
	channel = audio_start_sample(value, handle);
	timer->engine_context = channel;
	timer->engine_active = 1;
	timer->parameters_changed = 1;
	dos_audio_set_channel_volume(handle, 0);
}

void audio_op_unk2(legacy_s16 index, legacy_s16 base_value,
	legacy_s16 first_x, legacy_s16 first_y, legacy_s16 first_z,
	legacy_s16 second_x, legacy_s16 second_y, legacy_s16 second_z,
	legacy_s16 interval)
{
	struct AUDIO_TIMER* timer;
	struct AUDIO_ENGINE_DEFINITION* engine_definition;
	const legacy_u8 far* definition;
	legacy_u16 first_distance;
	legacy_u16 second_distance;
	legacy_u16 distance_delta;
	legacy_u16 scaled_delta;
	legacy_u16 volume;
	legacy_u16 base_rate;
	legacy_u16 denominator;
	legacy_u16 divisor;
	legacy_u16 quotient;

	timer = &audio_timers[index];
	second_distance = (legacy_u16)polarRadius2D(
		polarRadius2D(second_x, second_z), second_y);
	if (LEGACY_S16_FROM_BITS(second_distance) > AUDIO_MAX_AUDIBLE_DISTANCE) {
		timer->target_volume = 0;
		return;
	}

	first_distance = (legacy_u16)polarRadius2D(
		polarRadius2D(first_x, first_z), first_y);
	distance_delta = LEGACY_U16_WRAP_SUB(
		first_distance, second_distance);
	quotient = LEGACY_U16_DIV_OR_ZERO(AUDIO_SPATIAL_INTERVAL_PERCENT,
		(legacy_u16)interval);
	scaled_delta = LEGACY_U16_WRAP_MUL(quotient, distance_delta);
	quotient = (legacy_u16)LEGACY_U32_DIV_OR_ZERO(
		LEGACY_U32_WRAP_MUL(AUDIO_ENGINE_MAX_VOLUME, second_distance),
		AUDIO_MAX_AUDIBLE_DISTANCE);
	volume = LEGACY_U16_WRAP_SUB(AUDIO_ENGINE_MAX_VOLUME, quotient);
	if (LEGACY_S16_FROM_BITS(scaled_delta) > 0)
		volume = LEGACY_U16_WRAP_SUB(volume,
			volume >> AUDIO_APPROACH_VOLUME_REDUCTION_SHIFT);

	engine_definition = &timer->definition;
	definition = (const legacy_u8 far*)audio_read_far_pointer(
		(legacy_u8*)&engine_definition->resources[0]);
	divisor = definition[AUDIO_ENGINE_RATE_DIVISOR_OFFSET];
	base_rate = LEGACY_U16_DIV_OR_ZERO(
		(legacy_u16)base_value, divisor);
	base_rate = LEGACY_U16_WRAP_ADD(base_rate,
		(legacy_u16)((legacy_u16)definition[AUDIO_ENGINE_RATE_BASE_OFFSET] <<
			AUDIO_ENGINE_RATE_BASE_SHIFT));
	denominator = LEGACY_U16_WRAP_SUB(AUDIO_MAX_AUDIBLE_DISTANCE,
		scaled_delta);
	if (denominator != 0) {
		base_rate = (legacy_u16)LEGACY_U32_DIV_OR_ZERO(
			LEGACY_U32_WRAP_MUL(AUDIO_MAX_AUDIBLE_DISTANCE, base_rate),
			denominator);
		timer->target_pitch = base_rate;
	}
	timer->target_volume = (legacy_u8)volume;
}

void sub_18D06(const legacy_u8 far* sample, legacy_s16 interval)
{
	audio_op_unk2(audio_player_engine_channel,
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(
			sample + AUDIO_CAR_STATE_PLAYER_RPM_OFFSET)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample +
			AUDIO_CAR_STATE_PLAYER_PREVIOUS_OFFSET + AUDIO_VECTOR_X_OFFSET)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample +
			AUDIO_CAR_STATE_PLAYER_PREVIOUS_OFFSET + AUDIO_VECTOR_Y_OFFSET)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample +
			AUDIO_CAR_STATE_PLAYER_PREVIOUS_OFFSET + AUDIO_VECTOR_Z_OFFSET)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample +
			AUDIO_CAR_STATE_PLAYER_CURRENT_OFFSET + AUDIO_VECTOR_X_OFFSET)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample +
			AUDIO_CAR_STATE_PLAYER_CURRENT_OFFSET + AUDIO_VECTOR_Y_OFFSET)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample +
			AUDIO_CAR_STATE_PLAYER_CURRENT_OFFSET + AUDIO_VECTOR_Z_OFFSET)),
		interval);
	if (gameconfig.game_opponenttype != 0) {
		audio_op_unk2(audio_opponent_engine_channel,
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(
				sample + AUDIO_CAR_STATE_OPPONENT_RPM_OFFSET)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample +
				AUDIO_CAR_STATE_OPPONENT_PREVIOUS_OFFSET +
				AUDIO_VECTOR_X_OFFSET)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample +
				AUDIO_CAR_STATE_OPPONENT_PREVIOUS_OFFSET +
				AUDIO_VECTOR_Y_OFFSET)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample +
				AUDIO_CAR_STATE_OPPONENT_PREVIOUS_OFFSET +
				AUDIO_VECTOR_Z_OFFSET)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample +
				AUDIO_CAR_STATE_OPPONENT_CURRENT_OFFSET +
				AUDIO_VECTOR_X_OFFSET)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample +
				AUDIO_CAR_STATE_OPPONENT_CURRENT_OFFSET +
				AUDIO_VECTOR_Y_OFFSET)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample +
				AUDIO_CAR_STATE_OPPONENT_CURRENT_OFFSET +
				AUDIO_VECTOR_Z_OFFSET)),
			interval);
	}
}

void audio_driver_timer(void)
{
	struct AUDIO_TIMER* timer;
	legacy_u32 accumulator;
	legacy_u16 volume_accumulator;
	legacy_u16 pitch;
	legacy_u16 index;
	legacy_u8 volume;
	legacy_u8 secondary_volume;
	legacy_s16 channel;

	if (dos_data_stack_segments_match() == 0)
		return;

	audio_driver_timer_divider = LEGACY_S16_WRAP_ADD(audio_driver_timer_divider, 1);
	if (audio_driver_timer_divider < 2 && dos_audio_uses_direct_channels != 0)
		return;

	for (index = 0; index < AUDIO_TIMER_COUNT; index++) {
		timer = &audio_timers[index];
		if (timer->active == 0 || audio_effects_enabled == 0)
			continue;

		volume_accumulator = LEGACY_U16_WRAP_ADD(
			(legacy_u16)((legacy_u16)timer->target_volume << 4),
			LEGACY_U16_WRAP_MUL(
				timer->current_volume, 7U));
		volume_accumulator >>= 3;
		timer->current_volume = volume_accumulator;
		volume = (legacy_u8)(volume_accumulator >> 4);
		if (volume != timer->last_volume ||
			timer->parameters_changed != 0) {
			channel = timer->channel;
			dos_audio_set_channel_volume(channel, volume);
			secondary_volume = volume >= 10U ?
				(legacy_u8)(volume - 10U) : 0;
			channel = timer->effect_channel;
			if (channel != -1)
				dos_audio_set_channel_volume(channel, secondary_volume);
			channel = timer->secondary_effect_channel;
			if (channel != -1)
				dos_audio_set_channel_volume(channel, secondary_volume);
			timer->last_volume = volume;
		}

		accumulator = timer->current_pitch;
		accumulator = accumulator * 7UL +
			((legacy_u32)timer->target_pitch << 4);
		accumulator >>= 3;
		timer->current_pitch = accumulator;
		pitch = (legacy_u16)(accumulator >> 4);
		if (pitch != timer->last_pitch ||
			timer->parameters_changed != 0) {
			channel = timer->engine_context;
			if (channel != -1) {
				dos_audio_set_context_pitch(channel, pitch);
				timer->last_pitch = pitch;
			}
		}

		timer->parameters_changed = 0;
		if (timer->restart_engine != 0) {
			channel = timer->effect_channel;
			if (timer->engine_active != 0) {
				audio_init_chunk2(channel);
				timer->restart_engine = 0;
			} else if (sub_3771E(channel) != 0) {
				audio_op_unk(index);
				timer->restart_engine = 0;
			}
		}
	}

	if (audio_driver_timer_divider >= 2)
		audio_driver_timer_divider = 0;
}

void audio_unload(void)
{
	audio_fade_out(2);
	mmgr_free(songfileptr);
	mmgr_free(voicefileptr);
	is_audioloaded = 0;
}

void audio_enable_flag2(void)
{
	audio_music_enabled = 1;
}

void audio_disable_flag2(void)
{
	audio_music_enabled = 0;
	audio_update_lock = 1;
	if (audio_music_channel_count != 0)
		audio_release_channel_range(
			0, (legacy_u16)audio_music_channel_count - 1U);
	audio_update_driver_contexts();
	audio_update_lock = 0;
}

legacy_s16 audio_toggle_flag2(void)
{
	if (audio_music_enabled == 1) {
		audio_disable_flag2();
		return 0;
	}

	audio_enable_flag2();
	return 1;
}

legacy_s16 nopsub_373FE(void)
{
	legacy_u16 offset;
	legacy_u16 channel;

	if (audio_suspended == 1 || audio_music_enabled == 0)
		return 1;

	for (channel = 0; channel < (legacy_u16)audio_music_channel_count; channel++) {
		if ((audio_channels[channel + AUDIO_EFFECT_CHANNEL_FIRST].cursor.offset |
			audio_channels[channel + AUDIO_EFFECT_CHANNEL_FIRST].cursor.segment) !=
			0)
			return 0;
	}

	return 1;
}

void sub_3736A(void)
{
	audio_update_lock = 1;
	audio_music_active = 0;
	audio_release_channel_range(AUDIO_MUSIC_CHANNEL_FIRST,
		AUDIO_MUSIC_CHANNEL_LAST);
	audio_init_chunk(AUDIO_MUSIC_CHANNEL_FIRST, AUDIO_MUSIC_CHANNEL_LAST,
		0, 0, audio_music_rate, 0);
	audio_music_channel_count = 0;
	audio_update_driver_contexts();
	audio_update_lock = 0;
}

void audio_enable_flag6(void)
{
	legacy_s16 channel;

	if (audio_effects_enabled == 1)
		return;

	for (channel = AUDIO_EFFECT_CHANNEL_FIRST;
		channel <= AUDIO_EFFECT_CHANNEL_LAST; channel++)
		dos_audio_set_channel_volume(channel, audio_effect_channel_volumes[channel]);
	audio_effects_enabled = 1;
}

void audio_disable_flag6(void)
{
	legacy_s16 channel;

	if (audio_effects_enabled == 0)
		return;

	for (channel = AUDIO_EFFECT_CHANNEL_FIRST;
		channel <= AUDIO_EFFECT_CHANNEL_LAST; channel++) {
		audio_effect_channel_volumes[channel] =
			audio_sfx_channels[channel - AUDIO_EFFECT_CHANNEL_FIRST].volume;
		dos_audio_set_channel_volume(channel, 0);
	}
	audio_effects_enabled = 0;
}

legacy_s16 audio_toggle_flag6(void)
{
	if (audio_effects_enabled == 1) {
		audio_disable_flag6();
		return 0;
	}

	audio_enable_flag6();
	return 1;
}

legacy_s16 sub_3771E(legacy_s16 channel)
{
	legacy_u16 offset;

	if (audio_effects_enabled == 0 ||
		channel < AUDIO_EFFECT_CHANNEL_FIRST ||
		channel > AUDIO_EFFECT_CHANNEL_LAST)
		return 1;

	return (audio_channels[channel].cursor.offset |
		audio_channels[channel].cursor.segment) == 0;
}

void nopsub_37750(legacy_s16 channel, void far* value)
{
	void far* *field;

	field = (void far* *)&audio_channels[channel].finish_callback;
	*field = value;
}

void audio_init_chunk(legacy_s16 first_channel, legacy_s16 last_channel,
	void far* resource, legacy_u16 resource_data_offset,
	legacy_u16 rate, legacy_u8 priority)
{
	const legacy_u8 far* resource_data;
	struct AUDIO_CHANNEL* chunk;
	legacy_u32 pointer_value;
	legacy_s16 channel;
	legacy_s16 last;
	legacy_u16 pointer_offset;
	legacy_u16 pointer_segment;
	legacy_u16 resource_offset;
	legacy_u16 resource_segment;

	channel = (legacy_s16)first_channel;
	last = (legacy_s16)last_channel;
	if (channel > last)
		return;

	resource_offset = (legacy_u16)dos_memory_pointer_offset(resource);
	resource_segment = (legacy_u16)dos_memory_pointer_segment(resource);
	do {
		chunk = &audio_channels[channel];
		chunk->finish_callback.offset = 0;
		chunk->finish_callback.segment = 0;
		chunk->note_velocity = 0x7F;
		chunk->channel = (legacy_u8)channel;
		chunk->note_limit = 0x0F;
		audio_channel_values[(legacy_u16)channel] = 0;
		audio_channel_notes[(legacy_u16)channel] = 0;
		chunk->stack_depth = 0;
		chunk->call_depth = 0;
		chunk->priority = priority;
		chunk->active_notes = 0;
		chunk->delay = 0;
		chunk->unknown_1C = 0;
		chunk->resource.segment = 0;
		chunk->resource.offset = 0;
		chunk->volume = (legacy_u8)rate;
		chunk->sustain = 0;
		chunk->pitch = 0;
		chunk->unknown_29[0] = 0;
		chunk->unknown_29[1] = 0;
		chunk->unknown_29[2] = 0;
		chunk->unknown_29[3] = 0;
		chunk->driver_channel = 0xFF;

		if (resource != 0) {
			resource_data = (const legacy_u8 far*)dos_memory_make_pointer(
				resource_segment,
				LEGACY_U16_WRAP_ADD(resource_offset,
					resource_data_offset));
			pointer_value = audioresource_get_dword(resource_data);
			pointer_offset = LEGACY_U16_WRAP_ADD(
				(legacy_u16)pointer_value, 4U);
			pointer_segment = (legacy_u16)(pointer_value >> 16);
			chunk->call_stack[0].offset = pointer_offset;
			chunk->call_stack[0].segment = pointer_segment;
			pointer_value = audioresource_get_dword(resource_data);
			pointer_offset = LEGACY_U16_WRAP_ADD(
				(legacy_u16)pointer_value, 4U);
			pointer_segment = (legacy_u16)(pointer_value >> 16);
			chunk->cursor.offset = pointer_offset;
			chunk->cursor.segment = pointer_segment;
			resource_data_offset = LEGACY_U16_WRAP_ADD(
				resource_data_offset, 5U);
			chunk->instruments.offset =
				LEGACY_U16_WRAP_ADD(resource_offset, 7U);
			chunk->instruments.segment = resource_segment;
		} else {
			chunk->cursor.offset = 0;
			chunk->cursor.segment = 0;
		}

		channel = LEGACY_S16_WRAP_ADD(channel, 1);
	} while (channel <= last);
}

static void audio_clear_driver_context(struct AUDIO_CONTEXT* context)
{
	context->channel = 0xFFU;
	context->state = 0;
	context->priority = 0;
	context->resource.offset = 0;
	context->resource.segment = 0;
}

void audio_reset_channels(void)
{
	struct AUDIO_CONTEXT* context;
	legacy_u16 context_index;

	audio_update_lock = 1;
	audio_init_chunk(AUDIO_MUSIC_CHANNEL_FIRST, AUDIO_EFFECT_CHANNEL_LAST,
		0, 0, 0x7FU, 0);
	context = dos_audio_contexts;
	for (context_index = 0; context_index < dos_audio_context_count;
		context_index++) {
		dos_audio_driver_release_channel((legacy_s16)context_index);
		audio_clear_driver_context(context);
		context->driver_channel = 0xFFU;
		context++;
	}
	dos_audio_driver_reset();
	dos_audio_driver_start();
	audio_update_lock = 0;
}
void audio_release_channel_range(legacy_s16 first_channel,
	legacy_s16 last_channel)
{
	struct AUDIO_CHANNEL* chunk;
	struct AUDIO_CONTEXT* context;
	legacy_u16 context_index;
	legacy_u16 channel_bits;
	legacy_s16 channel;

	if (dos_audio_uses_direct_channels == 0) {
		context = dos_audio_contexts;
		for (context_index = 0; context_index < dos_audio_context_count;
			context_index++) {
			channel_bits = context->channel;
			if (channel_bits >= (legacy_u16)first_channel &&
				channel_bits <= (legacy_u16)last_channel) {
				dos_audio_driver_release_channel(
					(legacy_s16)context_index);
				audio_clear_driver_context(context);
			}
			context++;
		}
	} else {
		channel = first_channel;
		while (channel <= last_channel) {
			chunk = &audio_channels[channel];
			if (chunk->driver_channel < 0x10U)
				dos_audio_driver_release_channel(chunk->driver_channel);

			context = dos_audio_contexts;
			for (context_index = 0; context_index < AUDIO_CONTEXT_COUNT;
				context_index++) {
				if ((legacy_u16)context->channel ==
					(legacy_u16)channel)
					audio_clear_driver_context(context);
				context++;
			}
			channel = LEGACY_S16_WRAP_ADD(channel, 1);
		}
	}

	channel = first_channel;
	while (channel <= last_channel) {
		chunk = &audio_channels[channel];
		chunk->active_notes = 0;
		channel = LEGACY_S16_WRAP_ADD(channel, 1);
	}
}

static void far* audio_select_sample_resource(void far* original_resource,
	legacy_u8 note)
{
	static const legacy_u8 percussion_resource_indices[16] = {
		0U, 2U, 1U, 2U, 2U, 2U, 5U, 2U,
		6U, 2U, 6U, 2U, 2U, 3U, 2U, 4U
	};
	void far* resources[7];
	legacy_u16 note_index;

	if (((legacy_u8 far*)original_resource)[5] != 5U)
		return original_resource;
	resources[0] = audio_bass_drum_resource;
	resources[1] = audio_snare_resource;
	resources[2] = audio_tom_resource;
	resources[3] = audio_ride_resource;
	resources[4] = audio_crash_resource;
	resources[5] = audio_closed_hihat_resource;
	resources[6] = audio_open_hihat_resource;
	note_index = LEGACY_U16_WRAP_SUB(note, 0x18U);
	if (note_index >= 16U)
		return audio_tom_resource;
	return resources[percussion_resource_indices[note_index]];
}

static legacy_s16 audio_find_driver_context(legacy_u8 far* resource,
	struct AUDIO_CHANNEL* timer)
{
	struct AUDIO_CONTEXT* context;
	struct AUDIO_CHANNEL* old_timer;
	legacy_u32 oldest_state1_age;
	legacy_u32 oldest_state2_age;
	legacy_u32 age;
	legacy_u16 resource_mask;
	legacy_u16 context_mask;
	legacy_u16 context_index;
	legacy_u16 context_count;
	legacy_s16 oldest_state1;
	legacy_s16 oldest_state2;
	legacy_s16 selected;
	legacy_s16 restrict_to_timer;

	resource_mask = audioresource_get_word(resource + 0x0CU);
	if (resource_mask == 0)
		return -1;
	oldest_state1 = -1;
	oldest_state2 = -1;
	oldest_state1_age = 0;
	oldest_state2_age = 0;

	if (dos_audio_uses_direct_channels != 0) {
		context_count = 0x10U;
		restrict_to_timer = 0;
	} else {
		context_count = dos_audio_context_count;
		restrict_to_timer = timer->active_notes >= timer->note_limit;
	}

	context = dos_audio_contexts;
	for (context_index = 0; context_index < context_count;
		context_index++) {
		if (dos_audio_uses_direct_channels == 0) {
			context_mask = context_index < 16U ?
				(legacy_u16)(1U << context_index) : 0;
			if ((resource_mask & context_mask) == 0 ||
				(restrict_to_timer != 0 &&
					timer->channel != context->channel)) {
				context++;
				continue;
			}
		}

		if (context->state == 0) {
			if (dos_audio_uses_direct_channels == 0)
				timer->active_notes++;
			return (legacy_s16)context_index;
		}
		if (dos_audio_uses_direct_channels == 0 &&
			timer->priority < context->priority) {
			context++;
			continue;
		}

		age = context->age;
		if (context->state == 1U && age > oldest_state1_age) {
			oldest_state1_age = age;
			oldest_state1 = (legacy_s16)context_index;
		}
		if (context->state == 2U && age > oldest_state2_age) {
			oldest_state2_age = age;
			oldest_state2 = (legacy_s16)context_index;
		}
		context++;
	}

	selected = oldest_state2 != -1 ? oldest_state2 : oldest_state1;
	if (selected == -1)
		return -1;
	context = &dos_audio_contexts[selected];
	if (dos_audio_uses_direct_channels == 0 && restrict_to_timer == 0) {
		/* The stored offset is a plain data-segment offset, so the old
		   record is recovered relative to any object in that segment. */
		old_timer = (struct AUDIO_CHANNEL*)((legacy_u8*)audio_timers +
			LEGACY_U16_WRAP_SUB(context->timer_offset,
				dos_memory_pointer_offset(audio_timers)));
		if (old_timer != timer) {
			old_timer->active_notes--;
			timer->active_notes++;
		}
	}
	dos_audio_driver_start_context(context->driver_channel, context);
	dos_audio_driver_end_context(context->driver_channel, context);
	return selected;
}

legacy_s16 audio_start_note(struct AUDIO_CHANNEL* timer, legacy_u16 value,
	legacy_u32 duration, legacy_u8 note, legacy_u16 parameter,
	legacy_s16 handle)
{
	struct AUDIO_CONTEXT* context;
	legacy_u8 far* resource;
	legacy_s16 driver_channel;
	legacy_s16 pitch;
	legacy_s16 context_index;

	resource = (legacy_u8 far*)audio_read_far_pointer(
		(legacy_u8*)&timer->resource);
	resource = (legacy_u8 far*)audio_select_sample_resource(resource, note);
	if (resource == 0)
		return -1;
	context_index = audio_find_driver_context(resource, timer);
	if (context_index == -1)
		return -1;
	context = &dos_audio_contexts[context_index];
	if (context->resource.offset != dos_memory_pointer_offset(resource) ||
		context->resource.segment != dos_memory_pointer_segment(resource)) {
		audio_write_far_pointer((legacy_u8*)&context->resource, resource);
		if (dos_audio_uses_direct_channels == 0)
			dos_audio_driver_prepare_context(context_index,
				context, (legacy_u8*)timer, resource);
	}

	context->channel = (legacy_u8)handle;
	context->state = 1U;
	context->priority = timer->priority;
	context->age = 0;
	context->fade_out_flag = LEGACY_U32_WRAP_SUB(duration, 1UL);
	context->level = LEGACY_S16_FROM_BITS(
		audioresource_get_word(resource + 0x1CU));
	context->envelope_state = 1U;
	context->modulation_delay = audioresource_get_word(resource + 0x2AU);
	context->modulation_count = audioresource_get_word(resource + 0x2CU);
	context->modulation = 0;
	context->sequence_delay = audioresource_get_word(resource + 0x36U);
	context->sequence_count = audioresource_get_word(resource + 0x38U);
	context->sequence_value = 0;
	context->modulation_step = audioresource_get_word(resource + 0x30U);
	context->modulation_direction = resource[0x34U];
	context->modulation_tick = 0;
	context->sequence_tick = 0;
	context->sequence_index = 0;
	context->timer_offset = dos_memory_pointer_offset(timer);
	driver_channel = dos_audio_uses_direct_channels == 0 ?
		context_index : timer->driver_channel;
	context->driver_channel = (legacy_u8)driver_channel;

	if (note == 0xFFU) {
		dos_audio_driver_set_context_value(driver_channel, context, value);
		if (dos_audio_uses_direct_channels != 0)
			note = 0x3CU;
	}
	pitch = LEGACY_S16_WRAP_ADD(
		(legacy_s8)resource[0x10U], (legacy_s8)note);
	dos_audio_driver_activate_context(driver_channel, context,
		(legacy_u8*)timer, pitch, parameter, resource);
	audio_channel_notes[(legacy_u16)handle] = note;
	return context_index;
}

legacy_s16 audio_start_sample(legacy_u16 value, legacy_s16 handle)
{
	struct AUDIO_CHANNEL* timer;

	timer = &audio_channels[handle];
	return audio_start_note(timer, value, 0xFFFFFFE0UL, 0xFFU, 0, handle);
}

void audio_advance_driver_context(struct AUDIO_CONTEXT* context)
{
	struct AUDIO_CHANNEL* chunk;
	legacy_u32 value;

	context->age = LEGACY_U32_WRAP_ADD(context->age, 1UL);
	value = context->fade_out_flag;
	if (value != 0) {
		context->fade_out_flag = LEGACY_U32_WRAP_SUB(value, 1UL);
		return;
	}

	dos_audio_driver_start_context(context->driver_channel, context);
	context->state = 2U;
	chunk = &audio_channels[context->channel];
	context->envelope_state = chunk->sustain != 0 ? 3U : 4U;
}

static legacy_u16 audio_absolute_word(legacy_s16 value)
{
	if (value < 0)
		return (legacy_u16)LEGACY_S16_WRAP_NEGATE(value);
	return (legacy_u16)value;
}

void audio_update_driver_contexts(void)
{
	struct AUDIO_CONTEXT* context;
	struct AUDIO_CHANNEL* chunk;
	legacy_u8 far* resource;
	legacy_s16 level;
	legacy_s16 modulation;
	legacy_u16 magnitude;
	legacy_u16 threshold;
	legacy_u16 value;
	legacy_u16 context_index;
	legacy_u8 sequence_index;

	context = dos_audio_contexts;
	for (context_index = 0; context_index < dos_audio_context_count;
		context_index++) {
		if (context->state == 0) {
			context++;
			continue;
		}
		if (context->channel > 0x0FU)
			audio_advance_driver_context(context);

		resource = (legacy_u8 far*)
			audio_read_far_pointer((legacy_u8*)&context->resource);
		if (context->envelope_state == 1U) {
			level = LEGACY_S16_WRAP_ADD(
				context->level,
				LEGACY_S16_FROM_BITS(audioresource_get_word(
					resource + 0x20U)));
			context->level = level;
			value = audioresource_get_word(resource + 0x1EU);
			if (level >= LEGACY_S16_FROM_BITS(value)) {
				context->level = LEGACY_S16_FROM_BITS(value);
				context->envelope_state =
					LEGACY_S16_FROM_BITS(audioresource_get_word(
						resource + 0x24U)) >=
					LEGACY_S16_FROM_BITS(value) ? 3U : 2U;
			}
		}
		if (context->envelope_state == 2U) {
			level = LEGACY_S16_WRAP_SUB(
				context->level,
				LEGACY_S16_FROM_BITS(audioresource_get_word(
					resource + 0x22U)));
			context->level = level;
			value = audioresource_get_word(resource + 0x24U);
			if (level <= LEGACY_S16_FROM_BITS(value)) {
				context->envelope_state = 3U;
				context->level = LEGACY_S16_FROM_BITS(value);
			}
		}
		if (context->envelope_state == 3U &&
			audioresource_get_word(resource + 0x24U) == 0)
			context->envelope_state = 4U;
		if (context->envelope_state == 4U) {
			level = LEGACY_S16_WRAP_SUB(
				context->level,
				LEGACY_S16_FROM_BITS(audioresource_get_word(
					resource + 0x26U)));
			context->level = level;
			if (level <= 0) {
				context->level = 0;
				context->envelope_state = 0;
				context->state = 0;
				chunk = &audio_channels[context->channel];
				chunk->active_notes--;
				dos_audio_driver_end_context(
					context->driver_channel, context);
				audio_channel_notes[context->channel] = 0;
			}
		}

		if (resource[0x28U] != 0) {
			value = context->modulation_delay;
			if (value != 0) {
				context->modulation_delay =
					LEGACY_U16_WRAP_SUB(value, 1U);
			} else {
				value = context->modulation_count;
				if (value != 0) {
					if (value != 0x7FFFU)
						context->modulation_count =
							LEGACY_U16_WRAP_SUB(value, 1U);
					if (context->modulation_tick != 0) {
						context->modulation_tick--;
					} else {
						context->modulation_tick = resource[0x29U];
						modulation = context->modulation;
						if (context->modulation_direction == 2U)
							modulation = LEGACY_S16_WRAP_SUB(modulation,
								context->modulation_step);
						else
							modulation = LEGACY_S16_WRAP_ADD(modulation,
								context->modulation_step);
						context->modulation = modulation;
						magnitude = audio_absolute_word(modulation);
						threshold = audioresource_get_word(resource + 0x2EU);
						if (magnitude >= threshold) {
							if (context->modulation_direction == 2U &&
								(resource[0x34U] & 1U) != 0)
								context->modulation_direction = 1U;
							else if (context->modulation_direction != 2U &&
								(resource[0x34U] & 2U) != 0)
								context->modulation_direction = 2U;
							else
								context->modulation = 0;
						}
					}
				}
			}
		}

		if (resource[0x35U] != 0) {
			value = context->sequence_delay;
			if (value != 0) {
				context->sequence_delay = LEGACY_U16_WRAP_SUB(value, 1U);
			} else {
				value = context->sequence_count;
				if (value != 0) {
					context->sequence_count =
						LEGACY_U16_WRAP_SUB(value, 1U);
					if (context->sequence_tick != 0) {
						context->sequence_tick--;
					} else {
						context->sequence_tick = resource[0x3AU];
						sequence_index = context->sequence_index++;
						context->sequence_value =
							resource[0x3BU + (sequence_index & 7U)];
					}
				}
			}
		}

		dos_audio_driver_suspend_context(context->driver_channel, context,
			context->timer_offset, resource);
		context++;
	}
	dos_audio_driver_suspend_all(dos_audio_contexts);
}


void audio_suspend(void)
{
	struct AUDIO_CHANNEL* chunk;
	struct AUDIO_CONTEXT* context;
	legacy_u16 channel;
	legacy_u16 context_index;

	audio_suspended = 1;
	audio_update_lock = 1;
	if (dos_audio_uses_direct_channels != 0) {
		dos_audio_master_volume = 0;
		dos_audio_driver_set_master_state(4, (void far*)dos_audio_master_state);
		audio_update_lock = 0;
		return;
	}

	chunk = audio_channels;
	for (channel = 0; channel < AUDIO_CHANNEL_COUNT; channel++) {
		if (audio_effects_enabled == 1 ||
			channel < AUDIO_EFFECT_CHANNEL_FIRST) {
			audio_saved_channel_volumes[channel] = chunk->volume;
			dos_audio_set_channel_volume((legacy_s16)channel, 0);
		}
		chunk++;
	}

	context = dos_audio_contexts;
	for (context_index = 0; context_index < AUDIO_CONTEXT_COUNT;
		context_index++) {
		dos_audio_driver_suspend_context(context->driver_channel, context,
			context->timer_offset, audio_read_far_pointer(
				(legacy_u8*)&context->resource));
		context++;
	}
	dos_audio_driver_suspend_all(dos_audio_contexts);
	audio_update_lock = 0;
}

void audio_resume(void)
{
	legacy_u16 channel;

	audio_suspended = 1;
	audio_update_lock = 1;
	if (dos_audio_uses_direct_channels != 0) {
		dos_audio_master_volume = 0x64U;
		dos_audio_driver_set_master_state(4, (void far*)dos_audio_master_state);
	} else {
		for (channel = 0; channel < AUDIO_CHANNEL_COUNT; channel++) {
			if (audio_effects_enabled == 1 ||
				channel < AUDIO_EFFECT_CHANNEL_FIRST)
				dos_audio_set_channel_volume((legacy_s16)channel,
					audio_saved_channel_volumes[channel]);
		}
	}
	audio_update_lock = 0;
	audio_suspended = 0;
}

static legacy_s16 audio_find_free_sfx_channel(void)
{
	legacy_u16 offset;
	legacy_s16 candidate;

	for (candidate = AUDIO_EFFECT_CHANNEL_FIRST;
		candidate <= AUDIO_EFFECT_CHANNEL_LAST; candidate++) {
		if ((audio_sfx_channels[candidate - AUDIO_EFFECT_CHANNEL_FIRST].cursor.offset |
			audio_sfx_channels[candidate - AUDIO_EFFECT_CHANNEL_FIRST].cursor.segment) ==
			0 &&
			audio_channel_reserved[candidate] == 0)
			return candidate;
	}
	return -1;
}

legacy_s16 audio_check_flag(void far* resource, legacy_s16 channel,
	legacy_u8 priority, legacy_u16 rate)
{
	const legacy_u8 far* bytes;
	legacy_u16 scaled_rate;
	legacy_u16 resource_data_offset;
	legacy_u8 lowest_priority;
	legacy_s16 candidate;
	legacy_s16 replacement;

	bytes = (const legacy_u8 far*)resource;
	if (audio_effects_enabled == 0 || resource == 0 || bytes[5] != 1)
		return -1;

	if (audio_effect_rate != 0) {
		scaled_rate = (legacy_u16)((legacy_u32)(legacy_u16)rate *
			0x80UL);
		rate = LEGACY_U16_WRAP_SUB(LEGACY_U16_DIV_OR_ZERO(
			scaled_rate, (legacy_u16)audio_effect_rate), 1U);
	} else {
		rate = 0;
	}

	if (channel == -1)
		channel = audio_find_free_sfx_channel();
	if (channel == -1) {
		lowest_priority = 0xFFU;
		replacement = -1;
		for (candidate = AUDIO_EFFECT_CHANNEL_FIRST;
			candidate <= AUDIO_EFFECT_CHANNEL_LAST; candidate++) {
			if (audio_channel_reserved[candidate] == 0 &&
				audio_channels[candidate].priority <= lowest_priority) {
				lowest_priority = audio_channels[candidate].priority;
				replacement = candidate;
			}
		}
		if (replacement != -1 && lowest_priority <= priority) {
			audio_init_chunk2(replacement);
			channel = replacement;
		}
	}

	if (channel == -1)
		return -1;

	resource_data_offset = (legacy_u16)bytes[6] * 4U + 8U;
	audio_init_chunk(channel, channel, resource, resource_data_offset,
		rate, priority);
	return channel;
}

legacy_s16 audio_check_flag2(void far* resource, legacy_s16 channel,
	legacy_u8 priority)
{
	return audio_check_flag(resource, channel, priority,
		(legacy_u16)audio_effect_rate);
}

legacy_s16 nopsub_37456(void far* resource)
{
	return audio_check_flag2(resource, -1, 0x40U);
}

legacy_s16 sub_37470(legacy_s16 channel, legacy_u8 priority)
{

	if (channel == -1)
		channel = audio_find_free_sfx_channel();

	if (channel != -1) {
		audio_channel_reserved[channel] = 1;
		audio_channels[channel].priority = priority;
	}

	return channel;
}

void audio_init_chunk2(legacy_s16 channel)
{
	legacy_u16 offset;

	if (channel < AUDIO_EFFECT_CHANNEL_FIRST ||
		channel > AUDIO_EFFECT_CHANNEL_LAST)
		return;

	audio_channels[channel].cursor.offset = 0;
	audio_channels[channel].cursor.segment = 0;
	audio_release_channel_range(channel, channel);
	audio_init_chunk(channel, channel, 0, 0, audio_effect_rate, 0);
}

void audio_op_unk7(legacy_s16 index)
{
	legacy_s16 channel;

	channel = audio_timers[index].secondary_effect_channel;
	audio_init_chunk2(channel);
	audio_timers[index].secondary_effect_channel = -1;
}

legacy_s16 nopsub_27489(legacy_s16 index)
{
	legacy_s16 channel;

	channel = audio_timers[index].effect_channel;
	if (channel < 0)
		return 1;

	return sub_3771E(channel);
}

void audio_function2(legacy_s16 index)
{
	legacy_s16 channel;

	if (audio_timers[index].active != 1 ||
		audio_timers[index].engine_active != 1)
		return;

	channel = audio_timers[index].engine_context;
	sub_38156(channel);
	audio_timers[index].engine_context = -1;
	audio_timers[index].engine_active = 0;
	audio_timers[index].parameters_changed = 1;
}

static legacy_s16 audio_start_timer_resource(struct AUDIO_TIMER* timer,
	legacy_u16 resource_index, legacy_u8 priority)
{
	legacy_u16 rate;
	void far* resource;

	rate = timer->current_volume >> 4;
	resource = audio_read_far_pointer(
		(legacy_u8*)&timer->definition.resources[resource_index]);
	return audio_check_flag(resource, -1, priority, rate);
}

static legacy_s16 audio_start_indexed_event(legacy_s16 index,
	legacy_u16 resource_index, legacy_u8 priority)
{
	struct AUDIO_TIMER* timer;
	legacy_s16 channel;

	timer = &audio_timers[index];
	channel = audio_start_timer_resource(timer, resource_index, priority);
	timer->effect_channel = channel;
	timer->parameters_changed = 1;
	return channel;
}

void nopsub_27220(legacy_s16 index)
{
	audio_start_indexed_event(index, 2U, 0x40U);
	audio_timers[index].restart_engine = 1;
}

void audio_op_unk3(legacy_s16 index)
{
	audio_start_indexed_event(index, 8U, 0x40U);
}

void audio_op_unk4(legacy_s16 index)
{
	audio_start_indexed_event(index, 9U, 0x40U);
}

void audio_function2_wrap(legacy_s16 index)
{
	audio_start_indexed_event(index, 5U, 0x64U);
	audio_function2(index);
}

void nopsub_2726C(legacy_s16 index)
{
	audio_start_indexed_event(index, 3U, 0x40U);
	audio_function2(index);
}

void nopsub_272B0(legacy_s16 index)
{
	audio_start_indexed_event(index, 4U, 0x40U);
	audio_function2(index);
}

static void audio_start_secondary_event(legacy_s16 index,
	legacy_u16 resource_index)
{
	struct AUDIO_TIMER* timer;
	legacy_s16 channel;

	timer = &audio_timers[index];
	channel = timer->secondary_effect_channel;
	if (channel != -1)
		audio_init_chunk2(channel);

	channel = audio_start_timer_resource(timer, resource_index, 0x40U);
	timer->secondary_effect_channel = channel;
	timer->parameters_changed = 1;
}

void audio_op_unk5(legacy_s16 index)
{
	audio_start_secondary_event(index, 6U);
}

void audio_op_unk6(legacy_s16 index)
{
	audio_start_secondary_event(index, 7U);
}

void sub_374DE(legacy_s16 channel)
{
	if (channel > -1) {
		audio_channel_reserved[channel] = 0;
		audio_init_chunk2(channel);
	}
}

void sub_38156(legacy_s16 index)
{
	dos_audio_contexts[index].fade_out_flag = 1;
}

legacy_s16 sub_37868(legacy_s16 value)
{
	legacy_s16 channel;

	for (channel = 0; channel < (legacy_u16)audio_music_channel_count; channel++)
		dos_audio_set_channel_volume(channel, value);

	return channel;
}

void audio_fade_out(legacy_s16 delay_ticks)
{
	legacy_u8 uses_direct_channels;
	legacy_s16 volume;
	legacy_u32 delay;

	delay = (legacy_u32)(legacy_s32)delay_ticks;
	uses_direct_channels = dos_audio_uses_direct_channels;
	volume = uses_direct_channels != 0 ? 0x64 : audio_music_rate;
	while (volume > 0) {
		audio_update_lock = 1;
		if (uses_direct_channels != 0) {
			dos_audio_master_volume = (legacy_u8)volume;
			dos_audio_driver_set_master_state(
				4, (void far*)dos_audio_master_state);
		} else {
			sub_37868(volume);
		}
		audio_update_lock = 0;
		timer_copy_counter(delay);
		timer_wait_for_dx();
		volume = LEGACY_S16_WRAP_SUB(volume, 2);
	}

	sub_3736A();
	if (dos_audio_uses_direct_channels != 0) {
		timer_copy_counter(0x32UL);
		timer_wait_for_dx();
		dos_audio_master_volume = 0x64U;
		dos_audio_driver_set_master_state(4, (void far*)dos_audio_master_state);
	}
}

legacy_s16 nopsub_37898(legacy_s16 value)
{
	audio_music_rate = (legacy_u8)value;
	return sub_37868(value);
}

legacy_u16 nopsub_378AE(legacy_s16 channel)
{
	return (legacy_u16)audio_channel_values[(legacy_u16)channel];
}

legacy_u16 nopsub_378BC(legacy_s16 channel)
{
	return (legacy_u16)audio_channel_notes[(legacy_u16)channel];
}

void audio_unk3(legacy_u8 flags, legacy_s16 channel)
{
	if (audio_car_state_ready == 0)
		return;

	if ((flags & 0x10U) != 0)
		audio_op_unk4(channel);
	if ((flags & 0x20U) != 0)
		audio_op_unk3(channel);
}
