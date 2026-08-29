#include <assert.h>
#include <stdarg.h>

#include "../../src/restunts/c/memmgr.h"

const legacy_s8 aLocateshape4_4sShapeNotF[] = "shape";
const legacy_s8 aLocatesound4_4sSoundNotF[] = "sound";

void fatal_error(const legacy_s8* format, ...)
{
	(void)format;
	assert(0);
}

int main(void)
{
	legacy_u8 resource[18] = {
		0, 0, 0, 0,
		1, 0,
		'a', 'b', 'c', 0,
		0, 0, 0, 0,
		0x12, 0x34, 0x56, 0x78
	};
	const legacy_s8 literal_name[] = "abc";
	legacy_s8 four_byte_name[4] = { 'a', 'b', 'c', 0 };
	legacy_s8 far* result;

	result = locate_resource((legacy_s8 far*)resource, literal_name, 0U);
	assert(result == (legacy_s8 far*)&resource[14]);
	assert(literal_name[3] == 0);

	result = locate_shape_nofatal((legacy_s8 far*)resource,
		four_byte_name);
	assert(result == (legacy_s8 far*)&resource[14]);
	assert(four_byte_name[3] == 0);

	return 0;
}
