#include <dos.h>
#include "../../c/audio.h"
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

#define DOS_AUDIO_DRIVER_INITIALIZE_OFFSET 0U
#define DOS_AUDIO_DRIVER_SHUTDOWN_OFFSET 3U
#define DOS_AUDIO_DRIVER_START_OFFSET 6U
#define DOS_AUDIO_DRIVER_ACTIVATE_CONTEXT_OFFSET 9U
#define DOS_AUDIO_DRIVER_START_CONTEXT_OFFSET 12U
#define DOS_AUDIO_DRIVER_END_CONTEXT_OFFSET 15U
#define DOS_AUDIO_DRIVER_SET_VOLUME_OFFSET 18U
#define DOS_AUDIO_DRIVER_SET_CONTROL_OFFSET 21U
#define DOS_AUDIO_DRIVER_RESET_OFFSET 24U
#define DOS_AUDIO_DRIVER_SET_PITCH_OFFSET 27U
#define DOS_AUDIO_DRIVER_RELEASE_CHANNEL_OFFSET 30U
#define DOS_AUDIO_DRIVER_PREPARE_CONTEXT_OFFSET 33U
#define DOS_AUDIO_DRIVER_CONTEXT_VALUE_OFFSET 36U
#define DOS_AUDIO_DRIVER_SUSPEND_CONTEXT_OFFSET 39U
#define DOS_AUDIO_DRIVER_SUSPEND_ALL_OFFSET 48U
#define DOS_AUDIO_DRIVER_SEND_DATA_OFFSET 57U
#define DOS_AUDIO_DRIVER_SET_MASTER_STATE_OFFSET 63U
#define DOS_AUDIO_DRIVER_LOAD_BANK_OFFSET 66U
#define DOS_AUDIO_MASTER_STATE_SIZE 3U
#define DOS_AUDIO_DRIVER_DATA_SIZE 256U
#define DOS_AUDIO_SHUTDOWN_MASTER_VOLUME 100U
#define DOS_AUDIO_SHUTDOWN_MASTER_OPERATION 4
#define DOS_AUDIO_RESOURCE_CHANNEL_OFFSET 67U
#define DOS_AUDIO_DIRECT_CHANNEL_LIMIT 16U
#define DOS_AUDIO_CHANNEL_INDEX_MASK 15U
#define DOS_AUDIO_DRIVER_CHANNEL_BASE 1U
#define DOS_AUDIO_MASTER_STATE_INITIALIZER { 16U, 0U, 22U }

struct AUDIO_TIMER audio_timers[AUDIO_TIMER_COUNT];
struct AUDIO_CHANNEL audio_channels[AUDIO_CHANNEL_COUNT];
struct AUDIO_CHANNEL* audio_sfx_channels =
	audio_channels + AUDIO_EFFECT_CHANNEL_FIRST;
struct AUDIO_CONTEXT dos_audio_contexts[AUDIO_CONTEXT_COUNT];
legacy_u8 dos_audio_master_state[DOS_AUDIO_MASTER_STATE_SIZE] =
	DOS_AUDIO_MASTER_STATE_INITIALIZER;
legacy_u8 dos_audio_driver_data[DOS_AUDIO_DRIVER_DATA_SIZE];
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

	initialize = (driver_initialize_type)dos_audio_driver_entry(
		DOS_AUDIO_DRIVER_INITIALIZE_OFFSET);
	return initialize();
}

void dos_audio_driver_load_bank(void far* bank)
{
	driver_load_bank_type load_bank;

	load_bank = (driver_load_bank_type)dos_audio_driver_entry(
		DOS_AUDIO_DRIVER_LOAD_BANK_OFFSET);
	load_bank(bank);
}

static void dos_audio_driver_set_volume(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* context, legacy_u16 volume)
{
	driver_set_volume_type set_volume;

	set_volume = (driver_set_volume_type)dos_audio_driver_entry(
		DOS_AUDIO_DRIVER_SET_VOLUME_OFFSET);
	set_volume(driver_channel, (legacy_u8*)context, volume);
}

void dos_audio_driver_prepare_context(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context, legacy_u8* timer,
	void far* resource)
{
	driver_bind_context_type bind_context;

	bind_context =
		(driver_bind_context_type)dos_audio_driver_entry(
			DOS_AUDIO_DRIVER_PREPARE_CONTEXT_OFFSET);
	bind_context(driver_channel, (legacy_u8*)driver_context, timer, resource);
}

void dos_audio_driver_set_context_value(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context, legacy_u16 value)
{
	driver_set_volume_type set_value;

	set_value = (driver_set_volume_type)dos_audio_driver_entry(
		DOS_AUDIO_DRIVER_CONTEXT_VALUE_OFFSET);
	set_value(driver_channel, (legacy_u8*)driver_context, value);
}

void dos_audio_driver_set_control(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context, legacy_u16 control,
	legacy_u16 value)
{
	driver_control_type set_control;

	set_control = (driver_control_type)dos_audio_driver_entry(
		DOS_AUDIO_DRIVER_SET_CONTROL_OFFSET);
	set_control(driver_channel, (legacy_u8*)driver_context, control, value);
}

void dos_audio_driver_set_pitch(legacy_u8* timer, legacy_s16 pitch,
	legacy_s16 driver_channel)
{
	driver_pitch_type set_pitch;

	set_pitch = (driver_pitch_type)dos_audio_driver_entry(
		DOS_AUDIO_DRIVER_SET_PITCH_OFFSET);
	set_pitch(timer, pitch, driver_channel);
}

void dos_audio_driver_send_data(legacy_u16 length, legacy_u8* data)
{
	driver_data_type send_data;

	send_data = (driver_data_type)dos_audio_driver_entry(
		DOS_AUDIO_DRIVER_SEND_DATA_OFFSET);
	send_data(length, data);
}

void dos_audio_driver_activate_context(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context, legacy_u8* timer, legacy_s16 pitch,
	legacy_u16 parameter, void far* resource)
{
	driver_activate_context_type activate_context;

	activate_context = (driver_activate_context_type)
		dos_audio_driver_entry(DOS_AUDIO_DRIVER_ACTIVATE_CONTEXT_OFFSET);
	activate_context(driver_channel, (legacy_u8*)driver_context, timer, pitch,
		parameter, resource);
}

void dos_audio_driver_release_channel(legacy_s16 driver_channel)
{
	driver_channel_operation_type release_channel;

	release_channel = (driver_channel_operation_type)
		dos_audio_driver_entry(DOS_AUDIO_DRIVER_RELEASE_CHANNEL_OFFSET);
	release_channel(driver_channel);
}

void dos_audio_driver_start_context(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context)
{
	driver_context_operation_type start_context;

	start_context = (driver_context_operation_type)
		dos_audio_driver_entry(DOS_AUDIO_DRIVER_START_CONTEXT_OFFSET);
	start_context(driver_channel, (legacy_u8*)driver_context);
}

void dos_audio_driver_end_context(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context)
{
	driver_context_operation_type end_context;

	end_context = (driver_context_operation_type)
		dos_audio_driver_entry(DOS_AUDIO_DRIVER_END_CONTEXT_OFFSET);
	end_context(driver_channel, (legacy_u8*)driver_context);
}

void dos_audio_driver_reset(void)
{
	driver_operation_type reset;

	reset = (driver_operation_type)dos_audio_driver_entry(
		DOS_AUDIO_DRIVER_RESET_OFFSET);
	reset();
}

void dos_audio_driver_start(void)
{
	driver_operation_type start;

	start = (driver_operation_type)dos_audio_driver_entry(
		DOS_AUDIO_DRIVER_START_OFFSET);
	start();
}

static void dos_audio_driver_shutdown(void)
{
	driver_operation_type shutdown;

	shutdown = (driver_operation_type)dos_audio_driver_entry(
		DOS_AUDIO_DRIVER_SHUTDOWN_OFFSET);
	shutdown();
}

void dos_audio_driver_suspend_context(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context, legacy_u16 value,
	void far* resource)
{
	driver_suspend_context_type suspend_context;

	suspend_context = (driver_suspend_context_type)
		dos_audio_driver_entry(DOS_AUDIO_DRIVER_SUSPEND_CONTEXT_OFFSET);
	suspend_context(driver_channel, (legacy_u8*)driver_context,
		value, resource);
}

void dos_audio_driver_suspend_all(struct AUDIO_CONTEXT* contexts)
{
	driver_contexts_operation_type suspend_all;

	suspend_all = (driver_contexts_operation_type)
		dos_audio_driver_entry(DOS_AUDIO_DRIVER_SUSPEND_ALL_OFFSET);
	suspend_all((legacy_u8*)contexts);
}

void dos_audio_driver_set_master_state(legacy_s16 operation,
	void far* state)
{
	driver_master_state_type set_master_state;

	set_master_state = (driver_master_state_type)
		dos_audio_driver_entry(DOS_AUDIO_DRIVER_SET_MASTER_STATE_OFFSET);
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
			dos_audio_master_volume = DOS_AUDIO_SHUTDOWN_MASTER_VOLUME;
			dos_audio_driver_set_master_state(
				DOS_AUDIO_SHUTDOWN_MASTER_OPERATION,
				(void far*)dos_audio_master_state);
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
	struct AUDIO_CHANNEL* channel_state;
	struct AUDIO_CONTEXT* driver_context;
	legacy_u16 context_index;
	legacy_u8 driver_channel;

	channel_state = &audio_channels[channel];
	channel_state->resource.offset = FP_OFF(resource);
	channel_state->resource.segment = FP_SEG(resource);
	if (((legacy_u8 far*)resource)[DOS_AUDIO_RESOURCE_CHANNEL_OFFSET] <
		DOS_AUDIO_DIRECT_CHANNEL_LIMIT)
		driver_channel = ((legacy_u8 far*)resource)[
			DOS_AUDIO_RESOURCE_CHANNEL_OFFSET];
	else
		driver_channel = (legacy_u8)(((legacy_u16)channel &
			DOS_AUDIO_CHANNEL_INDEX_MASK) + DOS_AUDIO_DRIVER_CHANNEL_BASE);
	channel_state->driver_channel = driver_channel;

	if (dos_audio_uses_direct_channels != 0) {
		dos_audio_driver_prepare_context(
			driver_channel, 0, (legacy_u8*)channel_state, resource);
		return;
	}

	driver_context = dos_audio_contexts;
	for (context_index = 0; context_index < dos_audio_context_count;
		context_index++) {
		if ((legacy_u16)driver_context->channel == (legacy_u16)channel)
			dos_audio_driver_prepare_context((legacy_s16)context_index,
				driver_context, (legacy_u8*)channel_state, resource);
		driver_context++;
	}
}

void dos_audio_set_channel_volume(legacy_s16 channel, legacy_s16 volume)
{
	struct AUDIO_CHANNEL* chunk;
	struct AUDIO_CONTEXT* context;
	legacy_u16 context_index;
	legacy_u16 volume_bits;

	chunk = &audio_channels[channel];
	volume_bits = (legacy_u8)volume;
	chunk->volume = (legacy_u8)volume_bits;

	if (dos_audio_uses_direct_channels != 0) {
		dos_audio_driver_set_volume(chunk->driver_channel, 0, volume_bits);
		return;
	}

	context = dos_audio_contexts;
	for (context_index = 0; context_index < dos_audio_context_count;
		context_index++) {
		if ((legacy_u16)context->channel == (legacy_u16)channel)
			dos_audio_driver_set_volume(
				(legacy_s16)context_index, context, volume_bits);
		context++;
	}
}

void dos_audio_set_context_pitch(legacy_s16 context_index, legacy_s16 pitch)
{
	driver_set_volume_type set_pitch;
	struct AUDIO_CONTEXT* context;

	context = &dos_audio_contexts[context_index];
	set_pitch = (driver_set_volume_type)dos_audio_driver_entry(
		DOS_AUDIO_DRIVER_CONTEXT_VALUE_OFFSET);
	set_pitch(context->driver_channel, (legacy_u8*)context,
		(legacy_u16)pitch);
}
