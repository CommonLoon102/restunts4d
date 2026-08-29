#include <assert.h>

#include "../../src/restunts/c/externs.h"
#include "../../src/restunts/c/shape3d.h"

int main(void)
{
	legacy_u8 bytes[13];
	struct SHAPE3D shape;
	struct VECTOR source;
	struct VECTOR destination;

	shape.shape3d_vertex_bytes = bytes + 1;
	source.x = LEGACY_S16_FROM_BITS(0x8000U);
	source.y = 0x7FFF;
	source.z = -1;
	shape3d_vertex_write(&shape, 1U, &source);

	assert(bytes[7] == 0x00U);
	assert(bytes[8] == 0x80U);
	assert(bytes[9] == 0xFFU);
	assert(bytes[10] == 0x7FU);
	assert(bytes[11] == 0xFFU);
	assert(bytes[12] == 0xFFU);

	destination.x = 0;
	destination.y = 0;
	destination.z = 0;
	shape3d_vertex_read(&shape, 1U, &destination);
	assert(destination.x == source.x);
	assert(destination.y == source.y);
	assert(destination.z == source.z);

	return 0;
}
