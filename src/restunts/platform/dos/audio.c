#include <dos.h>
#include "../../c/legacy.h"

typedef void (far* driver_set_volume_type)(legacy_s16 driver_channel,
	legacy_u8* context, legacy_u16 volume);
typedef void (far* driver_bind_context_type)(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u8* timer, void far* resource);

extern legacy_u8 audiotimers[];
extern legacy_u8 audiochunks_unk[];
extern legacy_u8 byte_40634;
extern legacy_u8 byte_459D2;
extern legacy_u8 unk_45A26[];
extern void far* audiodriverbinary;

static void dos_audio_driver_set_volume(legacy_s16 driver_channel,
	legacy_u8* context, legacy_u16 volume)
{
	driver_set_volume_type set_volume;

	set_volume = (driver_set_volume_type)MK_FP(FP_SEG(audiodriverbinary),
		LEGACY_U16_WRAP_ADD(FP_OFF(audiodriverbinary), 0x12U));
	set_volume(driver_channel, context, volume);
}

static void dos_audio_driver_bind_context(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u8* timer, void far* resource)
{
	driver_bind_context_type bind_context;

	bind_context = (driver_bind_context_type)MK_FP(
		FP_SEG(audiodriverbinary), LEGACY_U16_WRAP_ADD(
			FP_OFF(audiodriverbinary), 0x21U));
	bind_context(driver_channel, driver_context, timer, resource);
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
