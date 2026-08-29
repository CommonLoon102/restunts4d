#ifndef RESTUNTS_RESOURCE_H
#define RESTUNTS_RESOURCE_H

#include "legacy.h"

#define RESOURCE_FILE_SIZE_OFFSET       0U
#define RESOURCE_FILE_COUNT_OFFSET      4U
#define RESOURCE_FILE_DIRECTORY_OFFSET  6U
#define RESOURCE_FILE_COUNT_SIZE        2U
#define RESOURCE_FILE_IDENTIFIER_SIZE   4U
#define RESOURCE_FILE_OFFSET_SIZE       4U

void resource_file_set_size(legacy_u8 far* resource, legacy_u32 size);
legacy_u16 resource_file_count(const legacy_u8 far* resource);
const legacy_u8 far* resource_file_identifier(
	const legacy_u8 far* resource, legacy_u16 index);
void resource_file_set_offset(
	legacy_u8 far* resource, legacy_u16 count, legacy_u16 index,
	legacy_u32 offset);
legacy_u16 resource_file_data_start(legacy_u16 count);
legacy_u8 far* resource_file_data(
	legacy_u8 far* resource, legacy_u16 index);

#endif
