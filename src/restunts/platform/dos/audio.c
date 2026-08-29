#include <dos.h>
#include "../../c/legacy.h"

typedef void (far* driver_set_volume_type)(legacy_s16 driver_channel,
	legacy_u8* context, legacy_u16 volume);
typedef void (far* driver_bind_context_type)(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u8* timer, void far* resource);
typedef void (far* driver_channel_operation_type)(legacy_s16 driver_channel);
typedef void (far* driver_operation_type)(void);
typedef void (far* driver_suspend_context_type)(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u16 value, void far* resource);
typedef void (far* driver_contexts_operation_type)(legacy_u8* contexts);
typedef void (far* driver_master_state_type)(legacy_s16 operation,
	void far* state);
typedef legacy_u8 (far* driver_initialize_type)(void);
typedef void (far* driver_load_bank_type)(void far* bank);

extern legacy_u8 audiotimers[];
extern legacy_u8 audiochunks_unk[];
extern legacy_s8 audioflag2;
extern legacy_s8 audioflag6;
extern legacy_u8 byte_40634;
extern legacy_u8 byte_40635;
extern legacy_u8 byte_40639;
extern legacy_u8 byte_459D2;
extern legacy_u8 unk_45A26[];
extern void far* audiodriverbinary;
extern legacy_s16 word_4063A;
extern void audio_driver_timer(void);
extern void timer_remove_callback(void (far* callback)(void));
extern void mmgr_release(void far* memory);

static void far* dos_audio_driver_entry(legacy_u16 offset)
{
	return MK_FP(FP_SEG(audiodriverbinary),
		LEGACY_U16_WRAP_ADD(FP_OFF(audiodriverbinary), offset));
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

static void dos_audio_driver_bind_context(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u8* timer, void far* resource)
{
	driver_bind_context_type bind_context;

	bind_context =
		(driver_bind_context_type)dos_audio_driver_entry(0x21U);
	bind_context(driver_channel, driver_context, timer, resource);
}

void dos_audio_driver_release_channel(legacy_s16 driver_channel)
{
	driver_channel_operation_type release_channel;

	release_channel = (driver_channel_operation_type)
		dos_audio_driver_entry(0x1EU);
	release_channel(driver_channel);
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
	word_4063A = 1;
	if (audiodriverbinary != 0) {
		timer_remove_callback(audio_driver_timer);
		audioflag2 = 0;
		audioflag6 = 0;
		if (byte_40634 != 0) {
			byte_40639 = 0x64U;
			dos_audio_driver_set_master_state(
				4, (void far*)&word_4063A);
		}
		dos_audio_driver_start();
		dos_audio_driver_shutdown();
		mmgr_release(audiodriverbinary);
		audiodriverbinary = 0;
		byte_40634 = 0;
		byte_40635 = 0;
	}
	word_4063A = 0;
}

void dos_audio_bind_channel_context(legacy_s16 channel, void far* resource)
{
	legacy_u8* timer;
	legacy_u8* driver_context;
	legacy_u16 context_index;
	legacy_u16 timer_offset;
	legacy_u8 driver_channel;

	timer_offset = LEGACY_U16_WRAP_MUL((legacy_u16)channel, 0x4CU);
	timer = audiotimers + timer_offset;
	LEGACY_WRITE_U16_LE(timer + 0x1EU, FP_OFF(resource));
	LEGACY_WRITE_U16_LE(timer + 0x20U, FP_SEG(resource));
	if (((legacy_u8 far*)resource)[0x43U] < 0x10U)
		driver_channel = ((legacy_u8 far*)resource)[0x43U];
	else
		driver_channel = (legacy_u8)(((legacy_u16)channel & 0x0FU) + 1U);
	timer[0x47U] = driver_channel;

	if (byte_40634 != 0) {
		dos_audio_driver_bind_context(
			driver_channel, 0, timer, resource);
		return;
	}

	driver_context = unk_45A26;
	for (context_index = 0; context_index < byte_459D2;
		context_index++) {
		if ((legacy_u16)driver_context[0] == (legacy_u16)channel)
			dos_audio_driver_bind_context((legacy_s16)context_index,
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
	chunk = audiochunks_unk + chunk_offset;
	volume_bits = (legacy_u8)volume;
	chunk[0x28U] = (legacy_u8)volume_bits;

	if (byte_40634 != 0) {
		dos_audio_driver_set_volume(chunk[0x47U], 0, volume_bits);
		return;
	}

	context = unk_45A26;
	for (context_index = 0; context_index < byte_459D2;
		context_index++) {
		if ((legacy_u16)context[0] == (legacy_u16)channel)
			dos_audio_driver_set_volume(
				(legacy_s16)context_index, context, volume_bits);
		context += 0x2EU;
	}
}
