#include "resource.h"

static const legacy_u8 far* resource_file_offset_bytes(
	const legacy_u8 far* resource, legacy_u16 count, legacy_u16 index)
{
	return resource + RESOURCE_FILE_DIRECTORY_OFFSET +
		LEGACY_U16_WRAP_MUL(count, RESOURCE_FILE_IDENTIFIER_SIZE) +
		LEGACY_U16_WRAP_MUL(index, RESOURCE_FILE_OFFSET_SIZE);
}

void resource_file_set_size(legacy_u8 far* resource, legacy_u32 size)
{
	LEGACY_WRITE_U32_LE(resource + RESOURCE_FILE_SIZE_OFFSET, size);
}

legacy_u16 resource_file_count(const legacy_u8 far* resource)
{
	return LEGACY_READ_U16_LE(resource + RESOURCE_FILE_COUNT_OFFSET);
}

const legacy_u8 far* resource_file_identifier(
	const legacy_u8 far* resource, legacy_u16 index)
{
	return resource + RESOURCE_FILE_DIRECTORY_OFFSET +
		LEGACY_U16_WRAP_MUL(index, RESOURCE_FILE_IDENTIFIER_SIZE);
}

void resource_file_set_offset(
	legacy_u8 far* resource, legacy_u16 count, legacy_u16 index,
	legacy_u32 offset)
{
	LEGACY_WRITE_U32_LE((legacy_u8 far*)resource_file_offset_bytes(
		resource, count, index), offset);
}

legacy_u16 resource_file_data_start(legacy_u16 count)
{
	return LEGACY_U16_WRAP_ADD(
		RESOURCE_FILE_DIRECTORY_OFFSET,
		LEGACY_U16_WRAP_MUL(count,
			RESOURCE_FILE_IDENTIFIER_SIZE + RESOURCE_FILE_OFFSET_SIZE));
}

legacy_u8 far* resource_file_data(
	legacy_u8 far* resource, legacy_u16 index)
{
	legacy_u16 count;
	legacy_u8 huge* result;

	count = resource_file_count(resource);
	result = resource;
	result += (legacy_u32)resource_file_data_start(count) +
		LEGACY_READ_U32_LE(resource_file_offset_bytes(
			resource, count, index));
	return (legacy_u8 far*)result;
}
