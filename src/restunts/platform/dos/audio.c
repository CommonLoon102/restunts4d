#include <dos.h>
#include "../../c/platform.h"

typedef void (far* driver_set_volume_type)(legacy_s16 driver_channel,
	legacy_u8* context, legacy_u16 volume);
typedef void (far* driver_control_type)(legacy_s16 driver_channel,
	legacy_u8* context, legacy_u16 control, legacy_u16 value);
typedef void (far* driver_pitch_type)(legacy_u8* timer, legacy_s16 pitch,
	legacy_s16 driver_channel);
typedef void (far* driver_data_type)(legacy_u16 length, legacy_u8* data);
typedef void (far* driver_bind_context_type)(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u8* timer, void far* resource);
typedef void (far* driver_channel_operation_type)(legacy_s16 driver_channel);
typedef void (far* driver_context_operation_type)(legacy_s16 driver_channel,
	legacy_u8* driver_context);
typedef void (far* driver_operation_type)(void);
typedef void (far* driver_suspend_context_type)(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u16 value, void far* resource);
typedef void (far* driver_contexts_operation_type)(legacy_u8* contexts);
typedef void (far* driver_master_state_type)(legacy_s16 operation,
	void far* state);
typedef legacy_u8 (far* driver_initialize_type)(void);
typedef void (far* driver_load_bank_type)(void far* bank);
typedef void (far* driver_activate_context_type)(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u8* timer, legacy_s16 pitch,
	legacy_u16 parameter, void far* resource);

extern void audio_sequence_timer(void);
extern void timer_remove_callback(void (far* callback)(void));
extern void mmgr_release(void far* memory);

legacy_u8 audio_timers[25U * 0x4CU];
legacy_u8 audio_channels[24U * 0x4CU];
legacy_u8* audio_sfx_channels = audio_channels + 16U * 0x4CU;
legacy_u8 dos_audio_contexts[16U * 0x2EU];
legacy_u8 dos_audio_master_state[3] = { 16U, 0, 22U };
legacy_u8 dos_audio_driver_data[256];
void far* dos_audio_driver_binary;
legacy_s16 audio_update_lock = 1;
legacy_s8 audio_music_enabled = 1;
legacy_s8 audio_effects_enabled = 1;
legacy_u8 dos_audio_uses_direct_channels;
legacy_u8 dos_audio_special_mode;
legacy_u8 dos_audio_master_volume;
legacy_u8 dos_audio_context_count;

static void far* dos_audio_driver_entry(legacy_u16 offset)
{
	return MK_FP(FP_SEG(dos_audio_driver_binary),
		LEGACY_U16_WRAP_ADD(FP_OFF(dos_audio_driver_binary), offset));
}

legacy_u8 dos_audio_driver_initialize(void)
{
	driver_initialize_type initialize;

	initialize = (driver_initialize_type)dos_audio_driver_entry(0);
	return initialize();
}

void dos_audio_driver_load_bank(void far* bank)
{
	driver_load_bank_type load_bank;

	load_bank = (driver_load_bank_type)dos_audio_driver_entry(0x42U);
	load_bank(bank);
}

static void dos_audio_driver_set_volume(legacy_s16 driver_channel,
	legacy_u8* context, legacy_u16 volume)
{
	driver_set_volume_type set_volume;

	set_volume = (driver_set_volume_type)dos_audio_driver_entry(0x12U);
	set_volume(driver_channel, context, volume);
}

void dos_audio_driver_prepare_context(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u8* timer, void far* resource)
{
	driver_bind_context_type bind_context;

	bind_context =
		(driver_bind_context_type)dos_audio_driver_entry(0x21U);
	bind_context(driver_channel, driver_context, timer, resource);
}

void dos_audio_driver_set_context_value(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u16 value)
{
	driver_set_volume_type set_value;

	set_value = (driver_set_volume_type)dos_audio_driver_entry(0x24U);
	set_value(driver_channel, driver_context, value);
}

void dos_audio_driver_set_control(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u16 control, legacy_u16 value)
{
	driver_control_type set_control;

	set_control = (driver_control_type)dos_audio_driver_entry(0x15U);
	set_control(driver_channel, driver_context, control, value);
}

void dos_audio_driver_set_pitch(legacy_u8* timer, legacy_s16 pitch,
	legacy_s16 driver_channel)
{
	driver_pitch_type set_pitch;

	set_pitch = (driver_pitch_type)dos_audio_driver_entry(0x1BU);
	set_pitch(timer, pitch, driver_channel);
}

void dos_audio_driver_send_data(legacy_u16 length, legacy_u8* data)
{
	driver_data_type send_data;

	send_data = (driver_data_type)dos_audio_driver_entry(0x39U);
	send_data(length, data);
}

void dos_audio_driver_activate_context(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u8* timer, legacy_s16 pitch,
	legacy_u16 parameter, void far* resource)
{
	driver_activate_context_type activate_context;

	activate_context = (driver_activate_context_type)
		dos_audio_driver_entry(9U);
	activate_context(driver_channel, driver_context, timer, pitch,
		parameter, resource);
}

void dos_audio_driver_release_channel(legacy_s16 driver_channel)
{
	driver_channel_operation_type release_channel;

	release_channel = (driver_channel_operation_type)
		dos_audio_driver_entry(0x1EU);
	release_channel(driver_channel);
}

void dos_audio_driver_start_context(legacy_s16 driver_channel,
	legacy_u8* driver_context)
{
	driver_context_operation_type start_context;

	start_context = (driver_context_operation_type)
		dos_audio_driver_entry(0x0CU);
	start_context(driver_channel, driver_context);
}

void dos_audio_driver_end_context(legacy_s16 driver_channel,
	legacy_u8* driver_context)
{
	driver_context_operation_type end_context;

	end_context = (driver_context_operation_type)
		dos_audio_driver_entry(0x0FU);
	end_context(driver_channel, driver_context);
}

void dos_audio_driver_reset(void)
{
	driver_operation_type reset;

	reset = (driver_operation_type)dos_audio_driver_entry(0x18U);
	reset();
}

void dos_audio_driver_start(void)
{
	driver_operation_type start;

	start = (driver_operation_type)dos_audio_driver_entry(6U);
	start();
}

static void dos_audio_driver_shutdown(void)
{
	driver_operation_type shutdown;

	shutdown = (driver_operation_type)dos_audio_driver_entry(3U);
	shutdown();
}

void dos_audio_driver_suspend_context(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u16 value, void far* resource)
{
	driver_suspend_context_type suspend_context;

	suspend_context = (driver_suspend_context_type)
		dos_audio_driver_entry(0x27U);
	suspend_context(driver_channel, driver_context, value, resource);
}

void dos_audio_driver_suspend_all(legacy_u8* contexts)
{
	driver_contexts_operation_type suspend_all;

	suspend_all = (driver_contexts_operation_type)
		dos_audio_driver_entry(0x30U);
	suspend_all(contexts);
}

void dos_audio_driver_set_master_state(legacy_s16 operation,
	void far* state)
{
	driver_master_state_type set_master_state;

	set_master_state = (driver_master_state_type)
		dos_audio_driver_entry(0x3FU);
	set_master_state(operation, state);
}

void dos_audio_shutdown(void)
{
	audio_update_lock = 1;
	if (dos_audio_driver_binary != 0) {
		timer_remove_callback(audio_sequence_timer);
		audio_music_enabled = 0;
		audio_effects_enabled = 0;
		if (dos_audio_uses_direct_channels != 0) {
			dos_audio_master_volume = 0x64U;
			dos_audio_driver_set_master_state(
				4, (void far*)dos_audio_master_state);
		}
		dos_audio_driver_start();
		dos_audio_driver_shutdown();
		mmgr_release(dos_audio_driver_binary);
		dos_audio_driver_binary = 0;
		dos_audio_uses_direct_channels = 0;
		dos_audio_special_mode = 0;
	}
	audio_update_lock = 0;
}

void dos_audio_bind_channel_context(legacy_s16 channel, void far* resource)
{
	legacy_u8* timer;
	legacy_u8* driver_context;
	legacy_u16 context_index;
	legacy_u16 timer_offset;
	legacy_u8 driver_channel;

	timer_offset = LEGACY_U16_WRAP_MUL((legacy_u16)channel, 0x4CU);
	timer = audio_timers + timer_offset;
	LEGACY_WRITE_U16_LE(timer + 0x1EU, FP_OFF(resource));
	LEGACY_WRITE_U16_LE(timer + 0x20U, FP_SEG(resource));
	if (((legacy_u8 far*)resource)[0x43U] < 0x10U)
		driver_channel = ((legacy_u8 far*)resource)[0x43U];
	else
		driver_channel = (legacy_u8)(((legacy_u16)channel & 0x0FU) + 1U);
	timer[0x47U] = driver_channel;

	if (dos_audio_uses_direct_channels != 0) {
		dos_audio_driver_prepare_context(
			driver_channel, 0, timer, resource);
		return;
	}

	driver_context = dos_audio_contexts;
	for (context_index = 0; context_index < dos_audio_context_count;
		context_index++) {
		if ((legacy_u16)driver_context[0] == (legacy_u16)channel)
			dos_audio_driver_prepare_context((legacy_s16)context_index,
				driver_context, timer, resource);
		driver_context += 0x2EU;
	}
}

void dos_audio_set_channel_volume(legacy_s16 channel, legacy_s16 volume)
{
	legacy_u8* chunk;
	legacy_u8* context;
	legacy_u16 context_index;
	legacy_u16 chunk_offset;
	legacy_u16 volume_bits;

	chunk_offset = LEGACY_U16_WRAP_MUL((legacy_u16)channel, 0x4CU);
	chunk = audio_channels + chunk_offset;
	volume_bits = (legacy_u8)volume;
	chunk[0x28U] = (legacy_u8)volume_bits;

	if (dos_audio_uses_direct_channels != 0) {
		dos_audio_driver_set_volume(chunk[0x47U], 0, volume_bits);
		return;
	}

	context = dos_audio_contexts;
	for (context_index = 0; context_index < dos_audio_context_count;
		context_index++) {
		if ((legacy_u16)context[0] == (legacy_u16)channel)
			dos_audio_driver_set_volume(
				(legacy_s16)context_index, context, volume_bits);
		context += 0x2EU;
	}
}

void dos_audio_set_context_pitch(legacy_s16 context_index, legacy_s16 pitch)
{
	driver_set_volume_type set_pitch;
	legacy_u8* context;
	legacy_u16 context_offset;

	context_offset = LEGACY_U16_WRAP_MUL(
		(legacy_u16)context_index, 0x2EU);
	context = dos_audio_contexts + context_offset;
	set_pitch = (driver_set_volume_type)dos_audio_driver_entry(0x24U);
	set_pitch(context[0x2CU], context, (legacy_u16)pitch);
}
