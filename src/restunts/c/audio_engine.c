#include <stddef.h>
#include "audio.h"
#include "audio_internal.h"
#include "externs.h"
#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "platform.h"
#include "resource.h"

extern legacy_s16 audio_car_state_read_index;
extern legacy_s16 audio_car_state_write_index;
extern legacy_u8 far* audio_car_state_records;
extern legacy_u8 audio_previous_replay_mode;
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
static legacy_u8 audio_saved_channel_volumes[24];
static legacy_u8 audio_effect_channel_volumes[24];
legacy_u8 audio_effect_rate;
legacy_u8 audio_channel_reserved[24];
static legacy_u8 audio_channel_values[24];
static legacy_u8 audio_channel_notes[24];
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
	audio_driver_timer_rate = 0x16U;
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
	for (field = 0; field < 0x30U; field++) {
		source = (const legacy_u8 far*)dos_memory_make_pointer(
			source_segment, source_offset);
		((legacy_u8*)engine_definition)[field] = *source;
		source_offset++;
		if (source_offset == 0)
			source_segment = LEGACY_U16_WRAP_ADD(
				source_segment, 0x1000U);
	}

	if (engine_definition->initialized == 0) {
		resource = locate_shape_fatal((legacy_s8 far*)shape_resources,
			pad_id((const legacy_s8 far*)audio_read_far_pointer(
				(legacy_u8*)&engine_definition->resources[0])));
		audio_write_far_pointer(
			(legacy_u8*)&engine_definition->resources[0], resource);
		for (resource_index = 2U; resource_index < 10U;
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

	channel = sub_37470(-1, 0x7FU);
	timer->channel = channel;
	timer->engine_active = 0;
	timer->current_volume = 0;
	timer->current_pitch = 0;
	timer->target_volume = 0;
	definition = (const legacy_u8 far*)audio_read_far_pointer(
		(legacy_u8*)&engine_definition->resources[0]);
	divisor = definition[0x0EU];
	rate = LEGACY_U16_DIV_OR_ZERO(
		engine_definition->sample_count, divisor);
	rate = LEGACY_U16_WRAP_ADD(rate,
		(legacy_u16)((legacy_u16)definition[0x0FU] << 4));
	timer->target_pitch = rate;
	timer->last_volume = 0xFFU;
	timer->last_pitch = 0xFFFFU;
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
	divisor = definition[0x0EU];
	value = LEGACY_U16_DIV_OR_ZERO(sample_count, divisor);
	value = LEGACY_U16_WRAP_ADD(value,
		(legacy_u16)((legacy_u16)definition[0x0FU] << 4));
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
	if (LEGACY_S16_FROM_BITS(second_distance) > 0x1770) {
		timer->target_volume = 0;
		return;
	}

	first_distance = (legacy_u16)polarRadius2D(
		polarRadius2D(first_x, first_z), first_y);
	distance_delta = LEGACY_U16_WRAP_SUB(
		first_distance, second_distance);
	quotient = LEGACY_U16_DIV_OR_ZERO(100U, (legacy_u16)interval);
	scaled_delta = LEGACY_U16_WRAP_MUL(quotient, distance_delta);
	quotient = (legacy_u16)LEGACY_U32_DIV_OR_ZERO(
		LEGACY_U32_WRAP_MUL(0x7FUL, second_distance), 0x1770UL);
	volume = LEGACY_U16_WRAP_SUB(0x7FU, quotient);
	if (LEGACY_S16_FROM_BITS(scaled_delta) > 0)
		volume = LEGACY_U16_WRAP_SUB(volume, volume >> 4);

	engine_definition = &timer->definition;
	definition = (const legacy_u8 far*)audio_read_far_pointer(
		(legacy_u8*)&engine_definition->resources[0]);
	divisor = definition[0x0EU];
	base_rate = LEGACY_U16_DIV_OR_ZERO(
		(legacy_u16)base_value, divisor);
	base_rate = LEGACY_U16_WRAP_ADD(base_rate,
		(legacy_u16)((legacy_u16)definition[0x0FU] << 4));
	denominator = LEGACY_U16_WRAP_SUB(0x1770U, scaled_delta);
	if (denominator != 0) {
		base_rate = (legacy_u16)LEGACY_U32_DIV_OR_ZERO(
			LEGACY_U32_WRAP_MUL(0x1770UL, base_rate), denominator);
		timer->target_pitch = base_rate;
	}
	timer->target_volume = (legacy_u8)volume;
}

void sub_18D06(const legacy_u8 far* sample, legacy_s16 interval)
{
	audio_op_unk2(audio_player_engine_channel,
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x1EU)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 6U)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 8U)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x0AU)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x0CU)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x0EU)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x10U)),
		interval);
	if (gameconfig.game_opponenttype != 0) {
		audio_op_unk2(audio_opponent_engine_channel,
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x20U)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x12U)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x14U)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x16U)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x18U)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x1AU)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x1CU)),
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
		offset = (channel + 0x10U) * 0x4CU;
		if ((LEGACY_READ_U16_LE(audio_channels + offset) |
			LEGACY_READ_U16_LE(audio_channels + offset + 2)) != 0)
			return 0;
	}

	return 1;
}

void sub_3736A(void)
{
	audio_update_lock = 1;
	audio_music_active = 0;
	audio_release_channel_range(0, 0x0F);
	audio_init_chunk(0, 0x0F, 0, 0, audio_music_rate, 0);
	audio_music_channel_count = 0;
	audio_update_driver_contexts();
	audio_update_lock = 0;
}

void audio_enable_flag6(void)
{
	legacy_s16 channel;

	if (audio_effects_enabled == 1)
		return;

	for (channel = 0x10; channel < 0x18; channel++)
		dos_audio_set_channel_volume(channel, audio_effect_channel_volumes[channel]);
	audio_effects_enabled = 1;
}

void audio_disable_flag6(void)
{
	legacy_s16 channel;

	if (audio_effects_enabled == 0)
		return;

	for (channel = 0x10; channel < 0x18; channel++) {
		audio_effect_channel_volumes[channel] =
			audio_sfx_channels[(channel - 0x10) * 0x4C + 0x28];
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

	if (audio_effects_enabled == 0 || channel < 0x10 || channel > 0x17)
		return 1;

	offset = (legacy_u16)channel * 0x4CU;
	return (LEGACY_READ_U16_LE(audio_channels + offset) |
		LEGACY_READ_U16_LE(audio_channels + offset + 2)) == 0;
}

void nopsub_37750(legacy_s16 channel, void far* value)
{
	void far* *field;

	field = (void far* *)(audio_channels +
		(legacy_u16)channel * 0x4CU + 0x48U);
	*field = value;
}

void audio_init_chunk(legacy_s16 first_channel, legacy_s16 last_channel,
	void far* resource, legacy_u16 resource_data_offset,
	legacy_u16 rate, legacy_u8 priority)
{
	const legacy_u8 far* resource_data;
	legacy_u8* chunk;
	legacy_u32 pointer_value;
	legacy_s16 channel;
	legacy_s16 last;
	legacy_u16 chunk_offset;
	legacy_u16 pointer_offset;
	legacy_u16 pointer_segment;
	legacy_u16 resource_offset;
	legacy_u16 resource_segment;

	channel = (legacy_s16)first_channel;
	last = (legacy_s16)last_channel;
	if (channel > last)
		return;

	chunk_offset = LEGACY_U16_WRAP_MUL(channel, 0x4CU);
	resource_offset = (legacy_u16)dos_memory_pointer_offset(resource);
	resource_segment = (legacy_u16)dos_memory_pointer_segment(resource);
	do {
		chunk = audio_channels + chunk_offset;
		LEGACY_WRITE_U16_LE(chunk + 0x48U, 0);
		LEGACY_WRITE_U16_LE(chunk + 0x4AU, 0);
		chunk[0x22] = 0x7F;
		chunk[0x23] = (legacy_u8)channel;
		chunk[0x16] = 0x0F;
		audio_channel_values[(legacy_u16)channel] = 0;
		audio_channel_notes[(legacy_u16)channel] = 0;
		chunk[0x32] = 0;
		chunk[4] = 0;
		chunk[0x24] = priority;
		chunk[0x15] = 0;
		LEGACY_WRITE_U16_LE(chunk + 0x1AU, 0);
		LEGACY_WRITE_U16_LE(chunk + 0x18U, 0);
		chunk[0x1C] = 0;
		LEGACY_WRITE_U16_LE(chunk + 0x20U, 0);
		LEGACY_WRITE_U16_LE(chunk + 0x1EU, 0);
		chunk[0x28] = (legacy_u8)rate;
		chunk[0x25] = 0;
		LEGACY_WRITE_U16_LE(chunk + 0x26U, 0);
		chunk[0x29] = 0;
		chunk[0x2A] = 0;
		chunk[0x2B] = 0;
		chunk[0x2C] = 0;
		chunk[0x47] = 0xFF;

		if (resource != 0) {
			resource_data = (const legacy_u8 far*)dos_memory_make_pointer(
				resource_segment,
				LEGACY_U16_WRAP_ADD(resource_offset,
					resource_data_offset));
			pointer_value = audioresource_get_dword(resource_data);
			pointer_offset = LEGACY_U16_WRAP_ADD(
				(legacy_u16)pointer_value, 4U);
			pointer_segment = (legacy_u16)(pointer_value >> 16);
			LEGACY_WRITE_U16_LE(chunk + 5U, pointer_offset);
			LEGACY_WRITE_U16_LE(chunk + 7U, pointer_segment);
			pointer_value = audioresource_get_dword(resource_data);
			pointer_offset = LEGACY_U16_WRAP_ADD(
				(legacy_u16)pointer_value, 4U);
			pointer_segment = (legacy_u16)(pointer_value >> 16);
			LEGACY_WRITE_U16_LE(chunk, pointer_offset);
			LEGACY_WRITE_U16_LE(chunk + 2U, pointer_segment);
			resource_data_offset = LEGACY_U16_WRAP_ADD(
				resource_data_offset, 5U);
			LEGACY_WRITE_U16_LE(chunk + 0x2EU,
				LEGACY_U16_WRAP_ADD(resource_offset, 7U));
			LEGACY_WRITE_U16_LE(chunk + 0x30U, resource_segment);
		} else {
			LEGACY_WRITE_U16_LE(chunk, 0);
			LEGACY_WRITE_U16_LE(chunk + 2U, 0);
		}

		chunk_offset = LEGACY_U16_WRAP_ADD(chunk_offset, 0x4CU);
		channel = LEGACY_S16_WRAP_ADD(channel, 1);
	} while (channel <= last);
}

void audio_reset_channels(void)
{
	struct AUDIO_CONTEXT* context;
	legacy_u16 context_index;

	audio_update_lock = 1;
	audio_init_chunk(0, 0x17, 0, 0, 0x7FU, 0);
	context = dos_audio_contexts;
	for (context_index = 0; context_index < dos_audio_context_count;
		context_index++) {
		dos_audio_driver_release_channel((legacy_s16)context_index);
		context->channel = 0xFFU;
		context->state = 0;
		context->priority = 0;
		context->resource.offset = 0;
		context->resource.segment = 0;
		context->driver_channel = 0xFFU;
		context++;
	}
	dos_audio_driver_reset();
	dos_audio_driver_start();
	audio_update_lock = 0;
}

static void audio_clear_driver_context(struct AUDIO_CONTEXT* context)
{
	context->channel = 0xFFU;
	context->state = 0;
	context->priority = 0;
	context->resource.offset = 0;
	context->resource.segment = 0;
}

void audio_release_channel_range(legacy_s16 first_channel,
	legacy_s16 last_channel)
{
	legacy_u8* chunk;
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
			chunk = audio_channels + LEGACY_U16_WRAP_MUL(
				(legacy_u16)channel, 0x4CU);
			if (chunk[0x47U] < 0x10U)
				dos_audio_driver_release_channel(chunk[0x47U]);

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
		chunk = audio_channels + LEGACY_U16_WRAP_MUL(
			(legacy_u16)channel, 0x4CU);
		chunk[0x15U] = 0;
		channel = LEGACY_S16_WRAP_ADD(channel, 1);
	}
}

static legacy_u16 audio_far_read_u16(const legacy_u8 far* source)
{
	return LEGACY_READ_U16_LE(source);
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
	legacy_u8* timer)
{
	struct AUDIO_CONTEXT* context;
	legacy_u8* old_timer;
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

	resource_mask = audio_far_read_u16(resource + 0x0CU);
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
		restrict_to_timer = timer[0x15U] >= timer[0x16U];
	}

	context = dos_audio_contexts;
	for (context_index = 0; context_index < context_count;
		context_index++) {
		if (dos_audio_uses_direct_channels == 0) {
			context_mask = context_index < 16U ?
				(legacy_u16)(1U << context_index) : 0;
			if ((resource_mask & context_mask) == 0 ||
				(restrict_to_timer != 0 &&
					timer[0x23U] != context->channel)) {
				context++;
				continue;
			}
		}

		if (context->state == 0) {
			if (dos_audio_uses_direct_channels == 0)
				timer[0x15U]++;
			return (legacy_s16)context_index;
		}
		if (dos_audio_uses_direct_channels == 0 &&
			timer[0x24U] < context->priority) {
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
		old_timer = (legacy_u8*)audio_timers + LEGACY_U16_WRAP_SUB(
			context->timer_offset,
			dos_memory_pointer_offset(audio_timers));
		if (old_timer != timer) {
			old_timer[0x15U]--;
			timer[0x15U]++;
		}
	}
	dos_audio_driver_start_context(context->driver_channel, context);
	dos_audio_driver_end_context(context->driver_channel, context);
	return selected;
}

legacy_s16 audio_start_note(legacy_u8* timer, legacy_u16 value,
	legacy_u32 duration, legacy_u8 note, legacy_u16 parameter,
	legacy_s16 handle)
{
	struct AUDIO_CONTEXT* context;
	legacy_u8 far* resource;
	legacy_s16 driver_channel;
	legacy_s16 pitch;
	legacy_s16 context_index;

	resource = (legacy_u8 far*)audio_read_far_pointer(timer + 0x1EU);
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
				context, timer, resource);
	}

	context->channel = (legacy_u8)handle;
	context->state = 1U;
	context->priority = timer[0x24U];
	context->age = 0;
	context->fade_out_flag = LEGACY_U32_WRAP_SUB(duration, 1UL);
	context->level = LEGACY_S16_FROM_BITS(
		audio_far_read_u16(resource + 0x1CU));
	context->envelope_state = 1U;
	context->modulation_delay = audio_far_read_u16(resource + 0x2AU);
	context->modulation_count = audio_far_read_u16(resource + 0x2CU);
	context->modulation = 0;
	context->sequence_delay = audio_far_read_u16(resource + 0x36U);
	context->sequence_count = audio_far_read_u16(resource + 0x38U);
	context->sequence_value = 0;
	context->modulation_step = audio_far_read_u16(resource + 0x30U);
	context->modulation_direction = resource[0x34U];
	context->modulation_tick = 0;
	context->sequence_tick = 0;
	context->sequence_index = 0;
	context->timer_offset = dos_memory_pointer_offset(timer);
	driver_channel = dos_audio_uses_direct_channels == 0 ?
		context_index : timer[0x47U];
	context->driver_channel = (legacy_u8)driver_channel;

	if (note == 0xFFU) {
		dos_audio_driver_set_context_value(driver_channel, context, value);
		if (dos_audio_uses_direct_channels != 0)
			note = 0x3CU;
	}
	pitch = LEGACY_S16_WRAP_ADD(
		(legacy_s8)resource[0x10U], (legacy_s8)note);
	dos_audio_driver_activate_context(driver_channel, context, timer,
		pitch, parameter, resource);
	audio_channel_notes[(legacy_u16)handle] = note;
	return context_index;
}

legacy_s16 audio_start_sample(legacy_u16 value, legacy_s16 handle)
{
	legacy_u8* timer;

	timer = audio_channels +
		LEGACY_U16_WRAP_MUL((legacy_u16)handle, 0x4CU);
	return audio_start_note(timer, value, 0xFFFFFFE0UL, 0xFFU, 0, handle);
}

void audio_advance_driver_context(struct AUDIO_CONTEXT* context)
{
	legacy_u8* chunk;
	legacy_u32 value;

	context->age = LEGACY_U32_WRAP_ADD(context->age, 1UL);
	value = context->fade_out_flag;
	if (value != 0) {
		context->fade_out_flag = LEGACY_U32_WRAP_SUB(value, 1UL);
		return;
	}

	dos_audio_driver_start_context(context->driver_channel, context);
	context->state = 2U;
	chunk = audio_channels +
		LEGACY_U16_WRAP_MUL(context->channel, 0x4CU);
	context->envelope_state = chunk[0x25U] != 0 ? 3U : 4U;
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
	legacy_u8* chunk;
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
				LEGACY_S16_FROM_BITS(audio_far_read_u16(
					resource + 0x20U)));
			context->level = level;
			value = audio_far_read_u16(resource + 0x1EU);
			if (level >= LEGACY_S16_FROM_BITS(value)) {
				context->level = LEGACY_S16_FROM_BITS(value);
				context->envelope_state =
					LEGACY_S16_FROM_BITS(audio_far_read_u16(
						resource + 0x24U)) >=
					LEGACY_S16_FROM_BITS(value) ? 3U : 2U;
			}
		}
		if (context->envelope_state == 2U) {
			level = LEGACY_S16_WRAP_SUB(
				context->level,
				LEGACY_S16_FROM_BITS(audio_far_read_u16(
					resource + 0x22U)));
			context->level = level;
			value = audio_far_read_u16(resource + 0x24U);
			if (level <= LEGACY_S16_FROM_BITS(value)) {
				context->envelope_state = 3U;
				context->level = LEGACY_S16_FROM_BITS(value);
			}
		}
		if (context->envelope_state == 3U &&
			audio_far_read_u16(resource + 0x24U) == 0)
			context->envelope_state = 4U;
		if (context->envelope_state == 4U) {
			level = LEGACY_S16_WRAP_SUB(
				context->level,
				LEGACY_S16_FROM_BITS(audio_far_read_u16(
					resource + 0x26U)));
			context->level = level;
			if (level <= 0) {
				context->level = 0;
				context->envelope_state = 0;
				context->state = 0;
				chunk = audio_channels + LEGACY_U16_WRAP_MUL(
					context->channel, 0x4CU);
				chunk[0x15U]--;
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
						threshold = audio_far_read_u16(resource + 0x2EU);
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
	legacy_u8* chunk;
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
	for (channel = 0; channel < 0x18U; channel++) {
		if (audio_effects_enabled == 1 || channel < 0x10U) {
			audio_saved_channel_volumes[channel] = chunk[0x28U];
			dos_audio_set_channel_volume((legacy_s16)channel, 0);
		}
		chunk += 0x4CU;
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
		for (channel = 0; channel < 0x18U; channel++) {
			if (audio_effects_enabled == 1 || channel < 0x10U)
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

	for (candidate = 0x10; candidate <= 0x17; candidate++) {
		offset = (legacy_u16)(candidate - 0x10) * 0x4CU;
		if ((LEGACY_READ_U16_LE(audio_sfx_channels + offset) |
			LEGACY_READ_U16_LE(audio_sfx_channels + offset + 2)) == 0 &&
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
	legacy_u16 offset;
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
		for (candidate = 0x10; candidate <= 0x17; candidate++) {
			offset = (legacy_u16)candidate * 0x4CU;
			if (audio_channel_reserved[candidate] == 0 &&
				audio_channels[offset + 0x24U] <= lowest_priority) {
				lowest_priority = audio_channels[offset + 0x24U];
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
	legacy_u16 offset;

	if (channel == -1)
		channel = audio_find_free_sfx_channel();

	if (channel != -1) {
		audio_channel_reserved[channel] = 1;
		offset = (legacy_u16)channel * 0x4CU;
		audio_channels[offset + 0x24U] = priority;
	}

	return channel;
}

void audio_init_chunk2(legacy_s16 channel)
{
	legacy_u16 offset;

	if (channel < 0x10 || channel > 0x17)
		return;

	offset = (legacy_u16)channel * 0x4CU;
	LEGACY_WRITE_U16_LE(audio_channels + offset, 0);
	LEGACY_WRITE_U16_LE(audio_channels + offset + 2, 0);
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

static legacy_s16 audio_start_indexed_event(legacy_s16 index,
	legacy_u16 resource_index, legacy_u8 priority)
{
	struct AUDIO_TIMER* timer;
	legacy_u16 rate;
	legacy_s16 channel;
	void far* resource;

	timer = &audio_timers[index];
	rate = timer->current_volume >> 4;
	resource = audio_read_far_pointer(
		(legacy_u8*)&timer->definition.resources[resource_index]);
	channel = audio_check_flag(resource, -1, priority, rate);
	timer->effect_channel = channel;
	timer->parameters_changed = 1;
	return channel;
}

void nopsub_27220(legacy_s16 index)
{
	struct AUDIO_TIMER* timer;
	legacy_u16 rate;
	legacy_s16 channel;
	void far* resource;

	timer = &audio_timers[index];
	rate = timer->current_volume >> 4;
	resource = audio_read_far_pointer(
		(legacy_u8*)&timer->definition.resources[2]);
	channel = audio_check_flag(resource, -1, 0x40U, rate);
	timer->effect_channel = channel;
	timer->parameters_changed = 1;
	timer->restart_engine = 1;
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
	legacy_u16 rate;
	legacy_s16 channel;
	void far* resource;

	timer = &audio_timers[index];
	channel = timer->secondary_effect_channel;
	if (channel != -1)
		audio_init_chunk2(channel);

	rate = timer->current_volume >> 4;
	resource = audio_read_far_pointer(
		(legacy_u8*)&timer->definition.resources[resource_index]);
	channel = audio_check_flag(resource, -1, 0x40U, rate);
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
	legacy_s16 volume;
	legacy_u32 delay;

	delay = (legacy_u32)(legacy_s32)delay_ticks;
	if (dos_audio_uses_direct_channels != 0) {
		volume = 0x64;
		do {
			audio_update_lock = 1;
			dos_audio_master_volume = (legacy_u8)volume;
			dos_audio_driver_set_master_state(
				4, (void far*)dos_audio_master_state);
			audio_update_lock = 0;
			timer_copy_counter(delay);
			timer_wait_for_dx();
			volume = LEGACY_S16_WRAP_SUB(volume, 2);
		} while (volume > 0);
	} else {
		volume = audio_music_rate;
		while (volume > 0) {
			audio_update_lock = 1;
			sub_37868(volume);
			audio_update_lock = 0;
			timer_copy_counter(delay);
			timer_wait_for_dx();
			volume = LEGACY_S16_WRAP_SUB(volume, 2);
		}
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
