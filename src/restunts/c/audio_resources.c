#include <stddef.h>
#include "audio.h"
#include "audio_internal.h"
#include "externs.h"
#include "fatal.h"
#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "platform.h"
#include "resource.h"

void audio_sequence_timer(void);
void timer_reg_callback(void (far* callback)(void));

static legacy_s16 audio_missing_file_fatal;
static legacy_s8 audio_driver_prefix[14];
static legacy_u8 audio_padded_id[5];
static legacy_s8 audio_filename_buffer[34];

static legacy_s16 legacy_toupper(legacy_s16 ch)
{
	if (ch >= 'a' && ch <= 'z') {
		ch -= ' ';
	}

	return ch;
}

legacy_s8* pad_id(const legacy_s8 far* source)
{
	legacy_u8* destination;
	legacy_u16 index;

	destination = audio_padded_id;
	for (index = 0; index < 4U; index++) {
		destination[index] = (legacy_u8)source[index];
		if (destination[index] == 0)
			destination[index] = ' ';
	}
	audio_padded_id[4] = 0;
	return (legacy_s8*)destination;
}

legacy_u32 audioresource_get_dword(const legacy_u8 far* source)
{
	return LEGACY_READ_U32_LE(source);
}

legacy_u16 audioresource_get_word(const legacy_u8 far* source)
{
	return LEGACY_READ_U16_LE(source);
}

void audioresource_copy_4_bytes(legacy_u8 far* destination,
	const legacy_u8 far* source)
{
	destination[0] = source[0];
	destination[1] = source[1];
	destination[2] = source[2];
	destination[3] = source[3];
}

static void audio_append_filename_part(legacy_s8* destination,
	const legacy_s8* source)
{
	while (*destination != 0)
		destination++;
	do {
		*destination++ = *source;
	} while (*source++ != 0);
}

static const legacy_s8* audio_find_last_backslash(const legacy_s8* text)
{
	const legacy_s8* match;

	match = 0;
	while (*text != 0) {
		if (*text == '\\')
			match = text;
		text++;
	}
	return match;
}

legacy_s8* audio_make_filename(const legacy_s8* filename, const legacy_s8* extension,
	const legacy_s8* inserted_path)
{
	const legacy_s8* basename;
	const legacy_s8* source;
	legacy_s8* separator;
	legacy_u16 length;

	separator = audio_filename_buffer;
	source = filename;
	do {
		*separator++ = *source;
	} while (*source++ != 0);
	separator = (legacy_s8*)audio_find_last_backslash(audio_filename_buffer);
	if (separator != 0)
		separator[1] = 0;
	else
		audio_filename_buffer[0] = 0;

	audio_append_filename_part(audio_filename_buffer, inserted_path);
	basename = audio_find_last_backslash(filename);
	if (basename != 0)
		basename++;
	else
		basename = filename;
	audio_append_filename_part(audio_filename_buffer, basename);

	length = 0;
	while (audio_filename_buffer[length] != 0)
		length++;
	if (length <= 4U || audio_filename_buffer[length - 4U] != '.') {
		audio_append_filename_part(audio_filename_buffer, ".");
		audio_append_filename_part(audio_filename_buffer, extension);
	}
	return audio_filename_buffer;
}

legacy_s16 audio_load_dos_driver(const legacy_s8* driver,
	legacy_s16 unused, legacy_s16 mode)
{
	static const legacy_s8 driver_extension[] = "drv";
	static const legacy_s8 empty_path[] = "";
	static const legacy_s8 mt32_bank_filename[] = "mt32.plb";
	static const legacy_s8 missing_driver_message[] =
		"Can't find driver!\n";
	void far* bank;
	legacy_u16 driver_length;
	legacy_u16 basename_offset;
	legacy_u16 scan_offset;
	legacy_u8 channel_count;

	(void)unused;
	if (mode == 0x473A)
		dos_audio_special_mode = 1;
	if (dos_audio_driver_binary != 0)
		dos_audio_shutdown();
	else
		add_exit_handler(dos_audio_shutdown);
	dos_audio_driver_binary = 0;

	driver_length = 0;
	while (driver[driver_length] != 0)
		driver_length++;
	basename_offset = 0;
	for (scan_offset = driver_length; scan_offset != 0; scan_offset--) {
		if (driver[scan_offset] == '\\' || driver[scan_offset] == ':') {
			basename_offset = LEGACY_U16_WRAP_ADD(scan_offset, 1U);
			break;
		}
	}
	audio_driver_prefix[0] = driver[basename_offset];
	audio_driver_prefix[1] = driver[basename_offset + 1U];
	audio_driver_prefix[2] = 0;

	dos_audio_driver_binary = file_load_binary_nofatal(audio_make_filename(
		driver, driver_extension, empty_path));
	audio_music_rate = 0x7FU;
	audio_effect_rate = 0x7FU;
	if (dos_audio_driver_binary == 0) {
		fatal_error(missing_driver_message);
		return 2;
	}

	channel_count = dos_audio_driver_initialize();
	dos_audio_context_count = channel_count;
	if (channel_count == 0 || channel_count == 0xFFU)
		return 2;
	if (channel_count > 0x7FU) {
		dos_audio_context_count = 0x10U;
		dos_audio_uses_direct_channels = 1;
		dos_audio_special_mode = 0;
	}

	audio_reset_channels();
	timer_reg_callback(audio_sequence_timer);
	if (dos_audio_uses_direct_channels != 0) {
		bank = file_load_binary_nofatal(mt32_bank_filename);
		if (bank != 0) {
			dos_audio_driver_load_bank(bank);
			mmgr_release((legacy_s8 far*)bank);
			dos_audio_master_volume = 0x64U;
			dos_audio_driver_set_master_state(
				4, (void far*)dos_audio_master_state);
		}
	}

	audio_suspended = 0;
	audio_music_enabled = 1;
	audio_music_active = 0;
	audio_effects_enabled = 1;
	return 0;
}

void far* load_sfx_ge(const legacy_s8* filename, const legacy_s8* extension,
	const legacy_s8* inserted_path)
{
	legacy_s8 compressed_extension[4];
	void far* result;

	result = file_load_binary_nofatal(audio_make_filename(
		filename, extension, inserted_path));
	if (result != 0)
		return result;

	compressed_extension[0] = 'P';
	compressed_extension[1] = extension[0];
	compressed_extension[2] = extension[1];
	compressed_extension[3] = 0;
	result = file_decomp_nofatal(audio_make_filename(
		filename, compressed_extension, inserted_path));
	if (result != 0)
		return result;

	result = file_load_binary_nofatal(audio_make_filename(
		filename, extension, "ge"));
	if (result != 0)
		return result;

	result = file_decomp_nofatal(audio_make_filename(
		filename, compressed_extension, "ge"));
	if (result != 0)
		return result;

	result = file_load_binary_nofatal(audio_make_filename(
		filename, extension, ""));
	if (result != 0)
		return result;

	result = file_decomp_nofatal(audio_make_filename(
		filename, compressed_extension, ""));
	if (result != 0)
		return result;

	return file_load_binary_nofatal(filename);
}

/* The special audio modes keep their samples under their own extension and
   fall back to the standard one before the load counts as a failure. */
static void far* load_audio_file(const legacy_s8* filename,
	const legacy_s8* special_extension, const legacy_s8* extension,
	const legacy_s8* error_message)
{
	void far* result;

	result = 0;
	if (dos_audio_special_mode != 0)
		result = load_sfx_ge(filename, special_extension,
			audio_driver_prefix);
	if (result == 0)
		result = load_sfx_ge(filename, extension, audio_driver_prefix);
	if (result == 0 && audio_missing_file_fatal != 0)
		fatal_error(error_message, filename);
	return result;
}

void far* load_sfx_file(const legacy_s8* filename)
{
	return load_audio_file(filename, "dsf", "sfx",
		"cannot load sfx file %s");
}

void far* load_song_file(const legacy_s8* filename)
{
	void far* result;

	result = load_sfx_ge(filename, "kms", audio_driver_prefix);
	if (result == 0 && audio_missing_file_fatal != 0)
		fatal_error("cannot load song file %s", filename);
	return result;
}

void far* load_voice_file(const legacy_s8* filename)
{
	return load_audio_file(filename, "dvc", "vce",
		"cannot load voice file %s");
}

legacy_s16 audioresource_compare_chunknames(legacy_s16 case_sensitive,
	const legacy_s8 far* first_name, const legacy_s8 far* second_name, legacy_s16 count)
{
	legacy_u16 first_offset;
	legacy_u16 first_segment;
	legacy_u16 second_offset;
	legacy_u16 second_segment;
	legacy_u16 remaining;
	legacy_u8 first;
	legacy_u8 second;

	remaining = (legacy_u16)count;
	if (remaining == 0)
		return 1;
	first_offset = (legacy_u16)dos_memory_pointer_offset(first_name);
	first_segment = (legacy_u16)dos_memory_pointer_segment(first_name);
	second_offset = (legacy_u16)dos_memory_pointer_offset(second_name);
	second_segment = (legacy_u16)dos_memory_pointer_segment(second_name);
	do {
		first = *(const legacy_u8 far*)dos_memory_make_pointer(
			first_segment, first_offset);
		second = *(const legacy_u8 far*)dos_memory_make_pointer(
			second_segment, second_offset);
		if (first == 0 || second == 0)
			return 1;
		if (case_sensitive != 0) {
			if (first != second)
				return 0;
		} else if (legacy_toupper(second) != legacy_toupper(first)) {
			return 0;
		}
		first_offset = LEGACY_U16_WRAP_ADD(first_offset, 1U);
		second_offset = LEGACY_U16_WRAP_ADD(second_offset, 1U);
		remaining--;
	} while (remaining != 0);
	return 1;
}

legacy_s16 audioresource_get_chunk_index(legacy_s16 extra_name_stride, legacy_s16 chunk_count,
	const legacy_s8* requested_name, const legacy_u8 far* chunk_names)
{
	const legacy_s8 far* requested_name_far;
	const legacy_u8 far* candidate;
	legacy_u16 names_offset;
	legacy_u16 names_segment;
	legacy_s16 count;
	legacy_s16 index;

	count = LEGACY_S16_FROM_BITS(chunk_count);
	if (count <= 0)
		return -1;
	requested_name_far = (const legacy_s8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(requested_name), dos_memory_pointer_offset(requested_name));
	names_offset = (legacy_u16)dos_memory_pointer_offset(chunk_names);
	names_segment = (legacy_u16)dos_memory_pointer_segment(chunk_names);
	for (index = 0; index < count;
		index = LEGACY_S16_WRAP_ADD(index, 1)) {
		candidate = (const legacy_u8 far*)dos_memory_make_pointer(
			names_segment, names_offset);
		if (audioresource_compare_chunknames(0,
			(const legacy_s8 far*)candidate, requested_name_far, 4))
			return index;
		names_offset = LEGACY_U16_WRAP_ADD(names_offset,
			LEGACY_U16_WRAP_ADD(4U, extra_name_stride));
	}
	return -1;
}

static void far* audio_far_pointer_add_normalized(void far* pointer,
	legacy_u16 increment)
{
	legacy_u16 old_offset;
	legacy_u16 new_offset;
	legacy_u16 segment;

	old_offset = (legacy_u16)dos_memory_pointer_offset(pointer);
	segment = (legacy_u16)dos_memory_pointer_segment(pointer);
	new_offset = LEGACY_U16_WRAP_ADD(old_offset, increment);
	if (new_offset < old_offset)
		segment = LEGACY_U16_WRAP_ADD(segment, 0x1000U);
	return dos_memory_make_pointer(segment, new_offset);
}

void far* audioresource_find(void far* resource, const legacy_s8* chunk_name)
{
	legacy_u8 far* bytes;
	legacy_u8 far* offset_entry;
	legacy_u16 resource_offset;
	legacy_u16 resource_segment;
	legacy_u16 chunk_count;
	legacy_u16 table_offset;
	legacy_u16 relative_offset;
	legacy_u16 result_offset;
	legacy_s16 chunk_index;

	bytes = (legacy_u8 far*)resource;
	resource_offset = (legacy_u16)dos_memory_pointer_offset(resource);
	resource_segment = (legacy_u16)dos_memory_pointer_segment(resource);
	chunk_count = audioresource_get_word(
		(const legacy_u8 far*)audio_far_pointer_add_normalized(bytes, 4U));
	chunk_index = audioresource_get_chunk_index(0, chunk_count, chunk_name,
		(const legacy_u8 far*)audio_far_pointer_add_normalized(bytes, 6U));
	if (chunk_index < 0)
		return 0;

	table_offset = LEGACY_U16_WRAP_ADD(resource_offset,
		LEGACY_U16_WRAP_MUL(chunk_count, 4U));
	table_offset = LEGACY_U16_WRAP_ADD(table_offset,
		LEGACY_U16_WRAP_MUL(chunk_index, 4U));
	table_offset = LEGACY_U16_WRAP_ADD(table_offset, 6U);
	offset_entry = (legacy_u8 far*)dos_memory_make_pointer(resource_segment, table_offset);
	relative_offset = (legacy_u16)audioresource_get_dword(offset_entry);
	result_offset = LEGACY_U16_WRAP_ADD(resource_offset,
		LEGACY_U16_WRAP_MUL(chunk_count, 8U));
	result_offset = LEGACY_U16_WRAP_ADD(result_offset, relative_offset);
	result_offset = LEGACY_U16_WRAP_ADD(result_offset, 6U);
	return dos_memory_make_pointer(resource_segment, result_offset);
}

void audio_map_song_instruments(void far* song, void far* instruments)
{
	legacy_u8 far* header;
	void far* instrument;
	legacy_s8 name[4];
	legacy_u16 pointer_offset;
	legacy_u16 pointer_segment;
	legacy_u16 count;
	legacy_u16 index;
	legacy_u16 name_offset;

	header = (legacy_u8 far*)audioresource_find(song, "hdr1");
	if (header == 0)
		return;

	count = header[6];
	for (index = 0; index < count; ++index) {
		name_offset = 7U + index * 4U;
		name[0] = header[name_offset];
		name[1] = header[name_offset + 1U];
		name[2] = header[name_offset + 2U];
		name[3] = header[name_offset + 3U];
		instrument = audioresource_find(instruments, name);
		pointer_offset = (legacy_u16)dos_memory_pointer_offset(instrument);
		pointer_segment = (legacy_u16)dos_memory_pointer_segment(instrument);
		header[name_offset] = (legacy_u8)pointer_offset;
		header[name_offset + 1U] = (legacy_u8)(pointer_offset >> 8);
		header[name_offset + 2U] = (legacy_u8)pointer_segment;
		header[name_offset + 3U] = (legacy_u8)(pointer_segment >> 8);
	}

	audio_bass_drum_resource = audioresource_find(instruments, "BASD");
	audio_snare_resource = audioresource_find(instruments, "SNAR");
	audio_tom_resource = audioresource_find(instruments, "TOMM");
	audio_ride_resource = audioresource_find(instruments, "RIDE");
	audio_crash_resource = audioresource_find(instruments, "CRSH");
	audio_closed_hihat_resource = audioresource_find(instruments, "CHHT");
	audio_open_hihat_resource = audioresource_find(instruments, "OHHT");
}

static void audio_write_far_pointer_to_resource(legacy_u8 far* destination,
	legacy_u16 offset, legacy_u16 segment)
{
	destination[0] = (legacy_u8)offset;
	destination[1] = (legacy_u8)(offset >> 8);
	destination[2] = (legacy_u8)segment;
	destination[3] = (legacy_u8)(segment >> 8);
}

static void audio_patch_song_reference(legacy_u8 far* destination,
	legacy_u16 name_table_offset, legacy_u16 offset_table_offset,
	legacy_u16 first_data_offset, legacy_u16 resource_segment,
	legacy_u16 chunk_count)
{
	const legacy_u8 far* names;
	const legacy_u8 far* offset_entry;
	legacy_s8 name[4];
	legacy_u16 relative_offset;
	legacy_s16 chunk_index;

	name[0] = destination[0];
	name[1] = destination[1];
	name[2] = destination[2];
	name[3] = destination[3];
	names = (const legacy_u8 far*)dos_memory_make_pointer(resource_segment,
		name_table_offset);
	chunk_index = audioresource_get_chunk_index(0, chunk_count, name,
		names);
	if (chunk_index < 0)
		return;

	offset_entry = (const legacy_u8 far*)dos_memory_make_pointer(resource_segment,
		LEGACY_U16_WRAP_ADD(offset_table_offset,
			LEGACY_U16_WRAP_MUL((legacy_u16)chunk_index, 4U)));
	relative_offset = (legacy_u16)audioresource_get_dword(offset_entry);
	audio_write_far_pointer_to_resource(destination,
		LEGACY_U16_WRAP_ADD(first_data_offset, relative_offset),
		resource_segment);
}

static legacy_u16 audio_skip_variable_length_field(
	legacy_u16 resource_segment, legacy_u16 cursor_offset)
{
	legacy_u8 far* cursor;

	cursor = (legacy_u8 far*)dos_memory_make_pointer(resource_segment,
		cursor_offset);
	while ((cursor[0] & 0x80U) != 0) {
		cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
		cursor = (legacy_u8 far*)dos_memory_make_pointer(resource_segment,
			cursor_offset);
	}
	return LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
}

void audio_map_song_tracks(void far* song)
{
	legacy_u8 far* bytes;
	legacy_u8 far* cursor;
	legacy_u16 resource_offset;
	legacy_u16 resource_segment;
	legacy_u16 chunk_count;
	legacy_u16 name_table_offset;
	legacy_u16 offset_table_offset;
	legacy_u16 first_data_offset;
	legacy_u16 chunk_offset;
	legacy_u16 chunk_end_offset;
	legacy_u16 cursor_offset;
	legacy_u16 relative_offset;
	legacy_u16 header_index;
	legacy_u16 index;
	legacy_u16 reference_count;
	legacy_u16 event;

	bytes = (legacy_u8 far*)song;
	resource_offset = (legacy_u16)dos_memory_pointer_offset(song);
	resource_segment = (legacy_u16)dos_memory_pointer_segment(song);
	chunk_count = audioresource_get_word(
		(const legacy_u8 far*)dos_memory_make_pointer(resource_segment,
			LEGACY_U16_WRAP_ADD(resource_offset, 4U)));
	name_table_offset = LEGACY_U16_WRAP_ADD(resource_offset, 6U);
	offset_table_offset = LEGACY_U16_WRAP_ADD(name_table_offset,
		LEGACY_U16_WRAP_MUL(chunk_count, 4U));
	first_data_offset = LEGACY_U16_WRAP_ADD(resource_offset,
		LEGACY_U16_WRAP_ADD(6U,
			LEGACY_U16_WRAP_MUL(chunk_count, 8U)));
	header_index = (legacy_u16)audioresource_get_chunk_index(0,
		chunk_count, "hdr1", (const legacy_u8 far*)dos_memory_make_pointer(
			resource_segment, name_table_offset));

	for (index = 0; index < chunk_count; ++index) {
		relative_offset = (legacy_u16)audioresource_get_dword(
			(const legacy_u8 far*)dos_memory_make_pointer(resource_segment,
				LEGACY_U16_WRAP_ADD(offset_table_offset,
					LEGACY_U16_WRAP_MUL(index, 4U))));
		chunk_offset = LEGACY_U16_WRAP_ADD(first_data_offset,
			relative_offset);
		bytes = (legacy_u8 far*)dos_memory_make_pointer(resource_segment, chunk_offset);
		chunk_end_offset = LEGACY_U16_WRAP_ADD(chunk_offset,
			(legacy_u16)audioresource_get_dword(bytes));
		cursor_offset = LEGACY_U16_WRAP_ADD(chunk_offset, 4U);

		if (index == header_index) {
			cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 2U);
			cursor = (legacy_u8 far*)dos_memory_make_pointer(resource_segment,
				cursor_offset);
			cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset,
				LEGACY_U16_WRAP_ADD(
					LEGACY_U16_WRAP_MUL(cursor[0], 4U), 1U));
			cursor = (legacy_u8 far*)dos_memory_make_pointer(resource_segment,
				cursor_offset);
			reference_count = cursor[0];
			cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
			while (reference_count != 0) {
				cursor = (legacy_u8 far*)dos_memory_make_pointer(resource_segment,
					cursor_offset);
				audio_patch_song_reference(cursor, name_table_offset,
					offset_table_offset, first_data_offset,
					resource_segment, chunk_count);
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 5U);
				reference_count--;
			}
			continue;
		}

		while (cursor_offset < chunk_end_offset) {
			cursor_offset = audio_skip_variable_length_field(
				resource_segment, cursor_offset);
			cursor = (legacy_u8 far*)dos_memory_make_pointer(resource_segment,
				cursor_offset);
			event = cursor[0];

			if (event < 0xD9U || event > 0xEAU) {
				if (event >= 0x80U)
					cursor_offset = LEGACY_U16_WRAP_ADD(
						cursor_offset, 1U);
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
				cursor_offset = audio_skip_variable_length_field(
					resource_segment, cursor_offset);
				continue;
			}
			if (audio_sequence_command_has_byte_argument(
				(legacy_u8)(event - 0xD9U))) {
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 2U);
				continue;
			}

			switch (event - 0xD9U) {
			case 0:
			case 1:
			case 2:
			case 10:
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
				break;

			case 6:
			case 12:
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 3U);
				break;

			case 13:
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 2U);
				cursor = (legacy_u8 far*)dos_memory_make_pointer(resource_segment,
					cursor_offset);
				audio_patch_song_reference(cursor, name_table_offset,
					offset_table_offset, first_data_offset,
					resource_segment, chunk_count);
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 4U);
				break;

			case 14:
			case 15:
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
				cursor = (legacy_u8 far*)dos_memory_make_pointer(resource_segment,
					cursor_offset);
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset,
					LEGACY_U16_WRAP_ADD(cursor[0], 1U));
				break;
			}
		}
	}
}

void far* init_audio_resources(void far* song, void far* instruments,
	const legacy_s8* name)
{
	legacy_u8 far* song_chunk;
	legacy_u8 far* header;
	legacy_u16 data_offset;
	void far* data;

	song_chunk = (legacy_u8 far*)audioresource_find(song, name);
	if (song_chunk == 0)
		return 0;
	header = (legacy_u8 far*)audioresource_find(song_chunk, "hdr1");
	if (header == 0)
		return 0;

	if (header[5] != 1) {
		audio_map_song_instruments(song_chunk, instruments);
		audio_map_song_tracks(song_chunk);
		header[5] = 1;
		data_offset = LEGACY_U16_WRAP_ADD(
			(legacy_u16)dos_memory_pointer_offset(song_chunk),
			(legacy_u16)((legacy_u16)song_chunk[4] << 3));
		data_offset = LEGACY_U16_WRAP_ADD(data_offset, 1U);
		data = dos_memory_make_pointer(dos_memory_pointer_segment(song_chunk), data_offset);
		audio_write_far_pointer(header, data);
	}

	return header;
}

void load_audio_finalize(void far* audio_resource)
{
	legacy_u8 far* resource;
	legacy_u16 data_offset;

	audio_update_lock = 1;
	sub_3736A();
	resource = (legacy_u8 far*)audio_resource;
	if (resource == 0 || resource[4] != 0 || resource[5] != 1)
		return;

	dos_audio_driver_reset();
	audio_engine_value_44d48 = 0;
	audio_engine_value_454ba = 0x80U;
	data_offset = LEGACY_U16_WRAP_ADD(
		(legacy_u16)((legacy_u16)resource[6] << 2), 7U);
	audio_music_channel_count = resource[data_offset++];
	audio_init_chunk(0,
		LEGACY_S16_FROM_BITS((legacy_u16)(audio_music_channel_count - 1U)),
		resource, data_offset, audio_music_rate, 0x20U);
	audio_music_active = 1;
	audio_update_lock = 0;
}

void audioresource_copy_n_bytes(const legacy_u8 far* source,
	legacy_u8 far* destination, legacy_s16 size)
{
	legacy_u16 source_offset;
	legacy_u16 source_segment;
	legacy_u16 destination_offset;
	legacy_u16 destination_segment;
	legacy_s16 remaining;

	remaining = LEGACY_S16_FROM_BITS(size);
	if (remaining <= 0)
		return;
	source_offset = (legacy_u16)dos_memory_pointer_offset(source);
	source_segment = (legacy_u16)dos_memory_pointer_segment(source);
	destination_offset = (legacy_u16)dos_memory_pointer_offset(destination);
	destination_segment = (legacy_u16)dos_memory_pointer_segment(destination);
	do {
		*(legacy_u8 far*)dos_memory_make_pointer(destination_segment,
			destination_offset) = *(const legacy_u8 far*)dos_memory_make_pointer(
			source_segment, source_offset);
		source_offset = LEGACY_U16_WRAP_ADD(source_offset, 1U);
		destination_offset = LEGACY_U16_WRAP_ADD(
			destination_offset, 1U);
		remaining = LEGACY_S16_WRAP_SUB(remaining, 1);
	} while (remaining != 0);
}

void sub_37C38(legacy_s16 value)
{
	audio_missing_file_fatal = value;
}
