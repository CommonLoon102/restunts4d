#include "audio_internal.h"
#include "platform.h"

extern legacy_s8 audio_music_enabled;
extern legacy_s16 audio_update_lock;
extern legacy_u8 audio_channels[];
extern legacy_u8 dos_audio_context_count;
extern legacy_u8 dos_audio_uses_direct_channels;
extern void far* dos_audio_driver_binary;

extern legacy_u8 audio_music_channel_count;
extern legacy_u8 audio_suspended;
extern legacy_u8 audio_music_active;
extern legacy_u8 audio_effect_rate;
extern legacy_u8 audio_channel_reserved[];
extern legacy_u16 audio_engine_value_44d48;
extern legacy_u16 audio_engine_value_454ba;
extern legacy_u8 dos_audio_driver_data[];

extern void far* audio_read_far_pointer(const legacy_u8 far* source);
extern void audio_write_far_pointer(legacy_u8 far* destination,
	const void far* value);
extern legacy_s16 audio_start_note(legacy_u8* timer, legacy_u16 value,
	legacy_u32 duration, legacy_u8 note, legacy_u16 parameter,
	legacy_s16 handle);
extern void audio_advance_driver_context(struct AUDIO_CONTEXT* context);
extern void audio_update_driver_contexts(void);
extern void audio_release_channel_range(legacy_s16 first_channel,
	legacy_s16 last_channel);
extern void audio_init_chunk(legacy_s16 first_channel,
	legacy_s16 last_channel, void far* resource,
	legacy_u16 resource_data_offset, legacy_u16 rate,
	legacy_u8 priority);

static legacy_u8 audio_sequence_timer_active;

struct audio_sequence_event {
	legacy_u32 delay;
	legacy_u8 command;
	legacy_u8 argument;
	legacy_u32 value;
	legacy_u8 size;
};

typedef void (far* audio_channel_callback_type)(legacy_s16 channel);

legacy_s16 audio_sequence_command_has_byte_argument(
	legacy_u8 command_index)
{
	switch (command_index) {
	case 3:
	case 4:
	case 5:
	case 7:
	case 8:
	case 9:
	case 11:
	case 16:
	case 17:
		return 1;
	default:
		return 0;
	}
}

static legacy_u32 audio_parse_variable_length(
	const legacy_u8 far* source, legacy_u16* size)
{
	legacy_u32 value;
	legacy_u8 byte_value;

	value = 0;
	do {
		byte_value = source[*size];
		*size = LEGACY_U16_WRAP_ADD(*size, 1U);
		value = LEGACY_U32_WRAP_ADD(value << 7,
			(legacy_u32)(byte_value & 0x7FU));
	} while ((byte_value & 0x80U) != 0);
	return value;
}

static void audio_parse_sequence_event(const legacy_u8 far* source,
	struct audio_sequence_event* event)
{
	legacy_u16 payload_size;
	legacy_u16 index;
	legacy_u16 size;
	legacy_u8 command_index;

	size = 0;
	event->delay = audio_parse_variable_length(source, &size);
	event->command = source[size++];
	event->argument = 0;
	event->value = 0;

	if (event->command >= 0xD9U && event->command <= 0xEAU) {
		command_index = (legacy_u8)(event->command - 0xD9U);
		if (audio_sequence_command_has_byte_argument(command_index)) {
			event->argument = source[size++];
		} else {
			switch (command_index) {
			case 6:
				event->argument = source[size++];
				event->value = source[size++];
				break;
			case 12:
				event->value = LEGACY_READ_U16_LE(source + size);
				size = LEGACY_U16_WRAP_ADD(size, 2U);
				break;
			case 13:
				event->argument = source[size++];
				event->value = LEGACY_READ_U32_LE(source + size);
				size = LEGACY_U16_WRAP_ADD(size, 4U);
				break;
			case 14:
				payload_size = source[size];
				size = LEGACY_U16_WRAP_ADD(size,
					LEGACY_U16_WRAP_ADD(payload_size, 1U));
				break;
			case 15:
				payload_size = source[size++];
				for (index = 0; index < payload_size; index++)
					dos_audio_driver_data[index] = source[size + index];
				size = LEGACY_U16_WRAP_ADD(size, payload_size);
				break;
			default:
				break;
			}
		}
	} else {
		if (event->command > 0x80U)
			event->argument = source[size++];
		event->value = audio_parse_variable_length(source, &size);
	}
	event->size = (legacy_u8)size;
}

static void far* audio_sequence_instrument(legacy_u8* chunk,
	legacy_u8 instrument)
{
	const legacy_u8 far* instruments;

	instruments = (const legacy_u8 far*)audio_read_far_pointer(chunk + 0x2EU);
	return audio_read_far_pointer(instruments +
		(legacy_u16)instrument * 4U);
}

static void audio_sequence_bind_instrument(legacy_s16 channel,
	legacy_u8* chunk, legacy_u8 instrument)
{
	void far* resource;
	legacy_u8 driver_channel;

	resource = audio_sequence_instrument(chunk, instrument);
	audio_write_far_pointer(chunk + 0x1EU, resource);
	if (dos_audio_uses_direct_channels == 0)
		return;

	if (((legacy_u8 far*)resource)[0x43U] < 0x10U)
		driver_channel = ((legacy_u8 far*)resource)[0x43U];
	else
		driver_channel = (legacy_u8)(((legacy_u16)channel & 0x0FU) + 1U);
	chunk[0x47U] = driver_channel;
	dos_audio_set_channel_volume(channel, chunk[0x28U]);
	dos_audio_driver_prepare_context(driver_channel, 0, chunk, resource);
}

static void audio_sequence_set_control(legacy_u8* chunk,
	legacy_u8 control, legacy_u16 value)
{
	struct AUDIO_CONTEXT* context;
	legacy_u16 context_index;

	if (control == 0x40U)
		chunk[0x25U] = (legacy_u8)value;

	if (dos_audio_uses_direct_channels != 0) {
		dos_audio_driver_set_control(chunk[0x47U], 0, control, value);
	}

	context = dos_audio_contexts;
	for (context_index = 0; context_index < dos_audio_context_count;
		context_index++) {
		if (context->channel == chunk[0x23U]) {
			if (dos_audio_uses_direct_channels == 0)
				dos_audio_driver_set_control((legacy_s16)context_index,
					context, control, value);
			if (control == 0x40U && value == 0 && context->state == 2U)
				context->envelope_state = 4U;
		}
		context++;
	}
}

static void audio_sequence_set_pitch(legacy_u8* chunk, legacy_u16 value)
{
	legacy_s16 low_value;
	legacy_s16 pitch;

	if ((value & 0x100U) != 0)
		value = LEGACY_U16_REPLACE_LOW_BYTE(value,
			LEGACY_U16_LOW_BYTE(value) | 0x80U);
	low_value = LEGACY_S8_FROM_BITS(LEGACY_U16_LOW_BYTE(value));
	pitch = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_FROM_BITS((value & 0xFF00U) >> 1), low_value);
	pitch = LEGACY_S16_WRAP_SUB(pitch, 0x2000);
	LEGACY_WRITE_U16_LE(chunk + 0x26U, pitch);
	dos_audio_driver_set_pitch(chunk, pitch, chunk[0x47U]);
}

static void audio_sequence_finish_channel(legacy_s16 channel,
	legacy_u8* chunk, legacy_s16 reset_channel)
{
	audio_channel_callback_type callback;

	callback = (audio_channel_callback_type)audio_read_far_pointer(
		chunk + 0x48U);
	LEGACY_WRITE_U16_LE(chunk, 0);
	LEGACY_WRITE_U16_LE(chunk + 2U, 0);
	audio_release_channel_range(channel, channel);
	if (reset_channel != 0)
		audio_init_chunk(channel, channel, 0, 0, audio_effect_rate, 0);
	if (callback != 0)
		callback(channel);
}

static void audio_service_sequence_channel(legacy_s16 channel)
{
	struct audio_sequence_event event;
	struct audio_sequence_event next_event;
	legacy_u8* chunk;
	legacy_u32 delay;
	legacy_u16 offset;
	legacy_u8 depth;
	legacy_u8 count;
	void far* pointer;

	chunk = audio_channels +
		LEGACY_U16_WRAP_MUL((legacy_u16)channel, 0x4CU);
	for (;;) {
		delay = LEGACY_READ_U32_LE(chunk + 0x18U);
		if (delay != 0) {
			LEGACY_WRITE_U32_LE(chunk + 0x18U,
				LEGACY_U32_WRAP_SUB(delay, 1UL));
			return;
		}
		pointer = audio_read_far_pointer(chunk);
		if (pointer == 0)
			return;

		audio_parse_sequence_event((const legacy_u8 far*)pointer, &event);
		LEGACY_WRITE_U16_LE(chunk, LEGACY_U16_WRAP_ADD(
			LEGACY_READ_U16_LE(chunk), event.size));
		if (event.command < 0xD9U) {
			if (event.command < 0x80U)
				event.argument = chunk[0x22U];
			event.command &= 0x7FU;
			audio_start_note(chunk, 0, event.value, event.command,
				event.argument, channel);
		} else {
			switch ((legacy_u8)(event.command - 0xD9U)) {
			case 0:
				depth = chunk[4];
				if (depth != 0) {
					pointer = audio_read_far_pointer(chunk + 5U +
						(legacy_u16)depth * 4U);
					audio_write_far_pointer(chunk, pointer);
					chunk[4]--;
				} else {
					audio_sequence_finish_channel(channel, chunk, 0);
				}
				break;
			case 1:
				audio_sequence_finish_channel(channel, chunk, 1);
				break;
			case 2:
				chunk[4] = 0;
				chunk[0x32U] = 0;
				pointer = audio_read_far_pointer(chunk + 5U);
				audio_write_far_pointer(chunk, pointer);
				break;
			case 3:
				audio_sequence_bind_instrument(channel, chunk,
					event.argument);
				break;
			case 4:
				if (channel < 0x10)
					audio_engine_value_454ba = LEGACY_U16_DIV_OR_ZERO(
						0x7D00U, event.argument);
				break;
			case 5:
				dos_audio_set_channel_volume(channel, event.argument);
				break;
			case 6:
				audio_sequence_set_control(chunk,
					event.argument, (legacy_u16)event.value);
				break;
			case 7:
				chunk[0x16U] = event.argument;
				break;
			case 8:
				chunk[0x24U] = event.argument;
				break;
			case 9:
				depth = chunk[0x32U];
				audio_write_far_pointer(chunk + 0x33U +
					(legacy_u16)depth * 4U, audio_read_far_pointer(chunk));
				chunk[0x43U + depth] = (legacy_u8)(event.argument - 1U);
				chunk[0x32U]++;
				break;
			case 10:
				depth = chunk[0x32U];
				if (depth != 0) {
					pointer = audio_read_far_pointer(chunk + 0x2FU +
						(legacy_u16)depth * 4U);
					audio_write_far_pointer(chunk, pointer);
					count = chunk[0x42U + depth];
					chunk[0x42U + depth]--;
					if (count == 0)
						chunk[0x32U]--;
				}
				break;
			case 11:
				chunk[0x22U] = event.argument;
				break;
			case 12:
				audio_sequence_set_pitch(chunk, (legacy_u16)event.value);
				break;
			case 13:
				chunk[4]++;
				depth = chunk[4];
				audio_write_far_pointer(chunk + 5U +
					(legacy_u16)depth * 4U, audio_read_far_pointer(chunk));
				pointer = dos_memory_make_pointer(
					(legacy_u16)(event.value >> 16),
					LEGACY_U16_WRAP_ADD((legacy_u16)event.value, 4U));
				audio_write_far_pointer(chunk, pointer);
				break;
			case 15:
				offset = LEGACY_U16_WRAP_SUB(event.size, 4U);
				dos_audio_driver_send_data(offset, dos_audio_driver_data);
				break;
			case 16:
				chunk[0x47U] = event.argument;
				break;
			case 17:
				audio_channel_reserved[(legacy_u16)channel] = event.argument;
				break;
			default:
				break;
			}
		}

		pointer = audio_read_far_pointer(chunk);
		if (pointer != 0) {
			audio_parse_sequence_event((const legacy_u8 far*)pointer,
				&next_event);
			LEGACY_WRITE_U32_LE(chunk + 0x18U, next_event.delay);
		}
	}
}

static void audio_advance_music_contexts(void)
{
	struct AUDIO_CONTEXT* context;
	legacy_u16 context_index;

	context = dos_audio_contexts;
	for (context_index = 0; context_index < dos_audio_context_count;
		context_index++) {
		if (context->state != 0 && context->channel < 0x10U)
			audio_advance_driver_context(context);
		context++;
	}
}

void audio_sequence_timer(void)
{
	legacy_u16 channel;

	if (dos_data_stack_segments_match() == 0 ||
		dos_audio_driver_binary == 0 || audio_update_lock != 0 ||
		audio_sequence_timer_active != 0)
		return;

	audio_sequence_timer_active = 1;
	audio_update_driver_contexts();
	if (audio_music_active == 1 && audio_music_enabled == 1 &&
		audio_suspended == 0) {
		audio_engine_value_44d48 = LEGACY_U16_WRAP_ADD(
			audio_engine_value_44d48, 0x80U);
		while (audio_engine_value_44d48 >= audio_engine_value_454ba) {
			audio_advance_music_contexts();
			audio_engine_value_44d48 = LEGACY_U16_WRAP_SUB(
				audio_engine_value_44d48, audio_engine_value_454ba);
			for (channel = 0; channel < audio_music_channel_count; channel++)
				audio_service_sequence_channel((legacy_s16)channel);
		}
	} else {
		audio_advance_music_contexts();
	}
	for (channel = 0x10U; channel < 0x17U; channel++)
		audio_service_sequence_channel((legacy_s16)channel);
	audio_sequence_timer_active--;
}
