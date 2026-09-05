#include <assert.h>
#include <stdarg.h>

#include "../c/memmgr.h"

#define TEST_RESOURCE_COUNT              1U
#define TEST_RESOURCE_PAYLOAD_SIZE       4U
#define TEST_RESOURCE_DATA_OFFSET        \
	(RESOURCE_FILE_DIRECTORY_OFFSET + RESOURCE_FILE_IDENTIFIER_SIZE + \
	RESOURCE_FILE_OFFSET_SIZE)
#define TEST_RESOURCE_SIZE               \
	(TEST_RESOURCE_DATA_OFFSET + TEST_RESOURCE_PAYLOAD_SIZE)
#define TEST_NAME_TERMINATOR_OFFSET      3U
#define TEST_RESOURCE_MARKER_BYTE_0      18U
#define TEST_RESOURCE_MARKER_BYTE_1      52U
#define TEST_RESOURCE_MARKER_BYTE_2      86U
#define TEST_RESOURCE_MARKER_BYTE_3      120U

const legacy_s8 aLocateshape4_4sShapeNotF[] = "shape";
const legacy_s8 aLocatesound4_4sSoundNotF[] = "sound";

void fatal_error(const legacy_s8* format, ...)
{
	(void)format;
	assert(0);
}

int main(void)
{
	legacy_u8 resource[TEST_RESOURCE_SIZE] = {
		0, 0, 0, 0,
		TEST_RESOURCE_COUNT, 0,
		'a', 'b', 'c', 0,
		0, 0, 0, 0,
		TEST_RESOURCE_MARKER_BYTE_0, TEST_RESOURCE_MARKER_BYTE_1,
		TEST_RESOURCE_MARKER_BYTE_2, TEST_RESOURCE_MARKER_BYTE_3
	};
	const legacy_s8 literal_name[] = "abc";
	legacy_s8 four_byte_name[RESOURCE_FILE_IDENTIFIER_SIZE] = {
		'a', 'b', 'c', 0
	};
	legacy_s8 far* result;

	result = locate_resource((legacy_s8 far*)resource, literal_name, 0U);
	assert(result ==
		(legacy_s8 far*)&resource[TEST_RESOURCE_DATA_OFFSET]);
	assert(literal_name[TEST_NAME_TERMINATOR_OFFSET] == 0);

	result = locate_shape_nofatal((legacy_s8 far*)resource,
		four_byte_name);
	assert(result ==
		(legacy_s8 far*)&resource[TEST_RESOURCE_DATA_OFFSET]);
	assert(four_byte_name[TEST_NAME_TERMINATOR_OFFSET] == 0);

	return 0;
}
