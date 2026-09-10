#include "resource.h"
#include "fatal.h"
#include "resource_bytes.h"
#include "platform.h"

legacy_u16 resource_read_u16le(const legacy_u8 far *source)
{
	return LEGACY_READ_U16_LE(source);
}

legacy_u32 resource_read_u32le(const legacy_u8 far *source)
{
	return LEGACY_READ_U32_LE(source);
}

enum RESOURCE_NOT_FOUND_BEHAVIOR {
	RESOURCE_NOT_FOUND_IGNORE = 0,
	RESOURCE_NOT_FOUND_SHAPE_FATAL = 1,
	RESOURCE_NOT_FOUND_SOUND_FATAL = 2
};

#define RESOURCE_IDENTIFIER_PADDING ' '
#define RESOURCE_PARAGRAPH_SHIFT 4U
#define RESOURCE_PARAGRAPH_OFFSET_MASK 15U

static const legacy_u8 far *resource_file_offset_bytes(const legacy_u8 far *resource,
													   legacy_u16 count, legacy_u16 index)
{
	return resource + RESOURCE_FILE_DIRECTORY_OFFSET +
		   LEGACY_U16_WRAP_MUL(count, RESOURCE_FILE_IDENTIFIER_SIZE) +
		   LEGACY_U16_WRAP_MUL(index, RESOURCE_FILE_OFFSET_SIZE);
}

void resource_file_set_size(legacy_u8 far *resource, legacy_u32 size)
{
	LEGACY_WRITE_U32_LE(resource + RESOURCE_FILE_SIZE_OFFSET, size);
}

legacy_u16 resource_file_count(const legacy_u8 far *resource)
{
	return LEGACY_READ_U16_LE(resource + RESOURCE_FILE_COUNT_OFFSET);
}

const legacy_u8 far *resource_file_identifier(const legacy_u8 far *resource, legacy_u16 index)
{
	return resource + RESOURCE_FILE_DIRECTORY_OFFSET +
		   LEGACY_U16_WRAP_MUL(index, RESOURCE_FILE_IDENTIFIER_SIZE);
}

void resource_file_set_offset(legacy_u8 far *resource, legacy_u16 count, legacy_u16 index,
							  legacy_u32 offset)
{
	LEGACY_WRITE_U32_LE((legacy_u8 far *)resource_file_offset_bytes(resource, count, index),
						offset);
}

legacy_u16 resource_file_data_start(legacy_u16 count)
{
	return LEGACY_U16_WRAP_ADD(
		RESOURCE_FILE_DIRECTORY_OFFSET,
		LEGACY_U16_WRAP_MUL(count, RESOURCE_FILE_IDENTIFIER_SIZE + RESOURCE_FILE_OFFSET_SIZE));
}

legacy_u8 far *resource_file_data(legacy_u8 far *resource, legacy_u16 index)
{
	legacy_u16 count;
	legacy_u8 huge *result;

	count = resource_file_count(resource);
	result = resource;
	result += (legacy_u32)resource_file_data_start(count) +
			  LEGACY_READ_U32_LE(resource_file_offset_bytes(resource, count, index));
#if defined(RESTUNTS_DOS16)
	/* A huge-pointer addition need not normalize its offset in Watcom. The
	 * original resource lookup returned paragraph-normalized pointers, and
	 * the 16-bit blitters deliberately wrap offsets within that segment.
	 * Preserve that representation so a payload near a 64 KiB boundary is
	 * read continuously rather than wrapping into an earlier resource. */
	return (legacy_u8 far *)dos_memory_make_pointer(
		dos_memory_pointer_segment(result) +
			(dos_memory_pointer_offset(result) >> RESOURCE_PARAGRAPH_SHIFT),
		dos_memory_pointer_offset(result) & RESOURCE_PARAGRAPH_OFFSET_MASK);
#else
	return (legacy_u8 far *)result;
#endif
}

legacy_s8 far *locate_resource(legacy_s8 far *data, const legacy_s8 *name, legacy_u16 fatal)
{
	legacy_u16 chunk_count;
	legacy_u16 index;
	legacy_u16 character;
	const legacy_u8 far *identifier;
	legacy_s8 padded_name[RESOURCE_FILE_IDENTIFIER_SIZE];
	legacy_u16 padding;

	chunk_count = resource_file_count((const legacy_u8 far *)data);
	/* Compare through a local padded key.  Several callers pass string
	 * literals, so the original in-place padding is not portable. */
	padding = 0;
	for (character = 0; character < RESOURCE_FILE_IDENTIFIER_SIZE; character++) {
		if (padding == 0 && name[character] != 0) {
			padded_name[character] = name[character];
		} else {
			padding = 1;
			padded_name[character] = RESOURCE_IDENTIFIER_PADDING;
		}
	}

	/* The original runs this compare chunks+1 times.  The extra slot is the
	 * first offset dword, which is zero for normal resources and cannot match
	 * any space-padded name used by callers. */
	for (index = 0; index < chunk_count; index++) {
		identifier = resource_file_identifier((const legacy_u8 far *)data, index);
		for (character = 0; character < RESOURCE_FILE_IDENTIFIER_SIZE; character++) {
			if (identifier[character] != (legacy_u8)padded_name[character]) {
				break;
			}
		}
		if (character == RESOURCE_FILE_IDENTIFIER_SIZE ||
			(identifier[character] == 0 && padded_name[character] == RESOURCE_IDENTIFIER_PADDING)) {
			return (legacy_s8 far *)resource_file_data((legacy_u8 far *)data, index);
		}
	}

	if (fatal >= RESOURCE_NOT_FOUND_SOUND_FATAL) {
		fatal_error(missing_sound_error_format, name);
	}
	if (fatal == RESOURCE_NOT_FOUND_SHAPE_FATAL) {
		fatal_error(missing_shape_error_format, name);
	}
	return 0;
}

legacy_s8 far *locate_shape_nofatal(legacy_s8 far *data, const legacy_s8 *name)
{
	return locate_resource(data, name, RESOURCE_NOT_FOUND_IGNORE);
}

legacy_s8 far *locate_shape_fatal(legacy_s8 far *data, const legacy_s8 *name)
{
	return locate_resource(data, name, RESOURCE_NOT_FOUND_SHAPE_FATAL);
}

legacy_s8 far *locate_shape_alt(legacy_s8 far *data, const legacy_s8 *name)
{
	return locate_shape_fatal(data, name);
}

legacy_s8 far *locate_sound_fatal(legacy_s8 far *data, const legacy_s8 *name)
{
	return locate_resource(data, name, RESOURCE_NOT_FOUND_SOUND_FATAL);
}
