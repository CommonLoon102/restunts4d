#ifndef RESTUNTS_RESOURCE_H
#define RESTUNTS_RESOURCE_H

#include "legacy.h"

#define RESOURCE_FILE_SIZE_OFFSET       0U
#define RESOURCE_FILE_COUNT_OFFSET      4U
#define RESOURCE_FILE_DIRECTORY_OFFSET  6U
#define RESOURCE_FILE_COUNT_SIZE        2U
#define RESOURCE_FILE_IDENTIFIER_SIZE   4U
#define RESOURCE_FILE_OFFSET_SIZE       4U

static legacy_u16 resource_file_count(const legacy_u8 far* resource)
{
	return LEGACY_READ_U16_LE(resource + RESOURCE_FILE_COUNT_OFFSET);
}

static const legacy_u8 far* resource_file_identifier(
	const legacy_u8 far* resource, legacy_u16 index)
{
	return resource + RESOURCE_FILE_DIRECTORY_OFFSET +
		LEGACY_U16_WRAP_MUL(index, RESOURCE_FILE_IDENTIFIER_SIZE);
}

static const legacy_u8 far* resource_file_offset_bytes(
	const legacy_u8 far* resource, legacy_u16 count, legacy_u16 index)
{
	return resource + RESOURCE_FILE_DIRECTORY_OFFSET +
		LEGACY_U16_WRAP_MUL(count, RESOURCE_FILE_IDENTIFIER_SIZE) +
		LEGACY_U16_WRAP_MUL(index, RESOURCE_FILE_OFFSET_SIZE);
}

static legacy_u8 far* resource_file_data(
	legacy_u8 far* resource, legacy_u16 index)
{
	legacy_u16 count;
	legacy_u16 directory_size;
	legacy_u8 huge* result;

	count = resource_file_count(resource);
	directory_size = LEGACY_U16_WRAP_ADD(
		RESOURCE_FILE_DIRECTORY_OFFSET,
		LEGACY_U16_WRAP_MUL(count,
			RESOURCE_FILE_IDENTIFIER_SIZE + RESOURCE_FILE_OFFSET_SIZE));
	result = resource;
	result += (legacy_u32)directory_size +
		LEGACY_READ_U32_LE(resource_file_offset_bytes(
			resource, count, index));
	return (legacy_u8 far*)result;
}

#endif
