#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../c/externs.h"
#include "../c/car_model.h"
#include "../c/fileio.h"
#include "../c/memmgr.h"
#include "../c/scene_resources.h"

#define TEST_SHAPE_COUNT 7U
#define TEST_VERTEX_COUNT 32U
#define TEST_SHAPE_BYTES (SHAPE3D_HEADER_SIZE + TEST_VERTEX_COUNT * SHAPE3D_VERTEX_SIZE + 14U)
#define TEST_RESOURCE_BYTES (TEST_SHAPE_COUNT * TEST_SHAPE_BYTES)
#define TEST_FIRST_CAR_SHAPE 116U

legacy_s8 car_shape_resource_name[] = "sttest";
/* The empty-shape render path does not use these math lookup/residue bytes. */
legacy_u8 atantable[257];
legacy_u8 vector_saved_z_low;
legacy_u8 vector_saved_z_high;

static void *allocated[2];
static unsigned allocation_count;
static unsigned file_load_count;
static unsigned cached_release_count;
static unsigned discarded_release_count;

void fatal_error(const legacy_s8 *format, ...)
{
	(void)format;
	abort();
}

void far *mmgr_alloc_resbytes(const legacy_s8 *name, legacy_s32 size)
{
	void *resource;

	(void)name;
	assert(size == TEST_RESOURCE_BYTES);
	assert(allocation_count < 2U);
	resource = calloc(1U, TEST_RESOURCE_BYTES);
	assert(resource != 0);
	allocated[allocation_count++] = resource;
	return resource;
}

legacy_u32 mmgr_get_chunk_size_bytes(legacy_s8 far *resource)
{
	assert(resource == allocated[0]);
	return TEST_RESOURCE_BYTES;
}

void far *file_load_3dres(const legacy_s8 *name)
{
	legacy_u8 *resource;
	legacy_u8 *shape;
	unsigned shape_index;
	unsigned vertex_index;

	file_load_count++;
	resource = mmgr_alloc_resbytes(name, TEST_RESOURCE_BYTES);
	for (shape_index = 0; shape_index < TEST_SHAPE_COUNT; shape_index++) {
		shape = resource + shape_index * TEST_SHAPE_BYTES;
		shape[SHAPE3D_VERTEX_COUNT_OFFSET] = TEST_VERTEX_COUNT;
		shape[SHAPE3D_PRIMITIVE_COUNT_OFFSET] = 1U;
		shape[SHAPE3D_PAINT_COUNT_OFFSET] = 1U;
		for (vertex_index = 0; vertex_index < TEST_VERTEX_COUNT; vertex_index++) {
			LEGACY_WRITE_U16_LE(shape + SHAPE3D_HEADER_SIZE + vertex_index * SHAPE3D_VERTEX_SIZE,
								vertex_index + 100U * shape_index);
		}
	}
	return resource;
}

legacy_s8 far *locate_shape_fatal(legacy_s8 far *resource, const legacy_s8 *name)
{
	unsigned shape_index;

	if (memcmp(name, "car", 3U) == 0) {
		shape_index = (unsigned)(name[3] - '0');
	} else {
		assert(memcmp(name, "exp", 3U) == 0);
		shape_index = (unsigned)(name[3] - '0') + 3U;
	}
	assert(shape_index < TEST_SHAPE_COUNT);
	return resource + shape_index * TEST_SHAPE_BYTES;
}

static void release_resource(void *resource)
{
	assert(allocation_count != 0U);
	assert(resource == allocated[allocation_count - 1U]);
	memset(resource, 0xa5, TEST_RESOURCE_BYTES);
	free(resource);
	allocated[--allocation_count] = 0;
}

void mmgr_release(void far *resource)
{
	discarded_release_count++;
	release_resource(resource);
}

void far *mmgr_free(legacy_s8 far *resource)
{
	cached_release_count++;
	release_resource(resource);
	return 0;
}

static void check_released_shapes(void)
{
	struct TRANSFORMEDSHAPE3D instance = {0};
	unsigned index;

	instance.culling_distance = 1024U;
	polyinfo_reset();
	for (index = TEST_FIRST_CAR_SHAPE; index <= OPPONENT_CAR_HIGH_SHAPE; index++) {
		instance.shapeptr = &game3dshapes[index];
		assert(instance.shapeptr->shape3d_numverts == 0U);
		assert(instance.shapeptr->shape3d_numprimitives == 0U);
		assert(instance.shapeptr->shape3d_numpaints == 0U);
		assert(instance.shapeptr->shape3d_vertex_bytes == 0);
		assert(instance.shapeptr->shape3d_primitives == 0);
		assert(instance.shapeptr->shape3d_visibility_masks == 0);
		assert(instance.shapeptr->shape3d_front_facing_masks == 0);
		/* A scene may retain the descriptor after its resource is released. */
		assert(shape3d_transform_and_queue(&instance) == LEGACY_U16_MAX);
	}
	assert(carresptr == 0);
	assert(car2resptr == 0);
	assert(allocation_count == 0U);
}

static void check_cycle(legacy_s8 *opponent, unsigned expected_loads, unsigned expected_discards)
{
	legacy_s8 player[] = "TEST";
	struct VECTOR vertex;
	unsigned previous_loads = file_load_count;
	unsigned previous_discards = discarded_release_count;
	unsigned previous_cached = cached_release_count;

	shape3d_load_car_shapes(player, opponent);
	assert(file_load_count - previous_loads == expected_loads);
	assert(game3dshapes[PLAYER_CAR_HIGH_SHAPE].shape3d_numverts == TEST_VERTEX_COUNT);
	shape3d_vertex_read(&game3dshapes[PLAYER_CAR_HIGH_SHAPE], 9U, &vertex);
	assert(vertex.x == 209);
	if (opponent[0] != -1) {
		assert(carresptr != car2resptr);
		shape3d_vertex_read(&game3dshapes[OPPONENT_CAR_HIGH_SHAPE], 9U, &vertex);
		assert(vertex.x == 209);
	}
	shape3d_free_car_shapes();
	assert(cached_release_count - previous_cached == 1U);
	assert(discarded_release_count - previous_discards == expected_discards);
	check_released_shapes();
	assert(game3dshapes[TEST_FIRST_CAR_SHAPE - 1U].shape3d_numverts == 17U);
	shape3d_free_car_shapes();
	assert(cached_release_count - previous_cached == 1U);
	assert(discarded_release_count - previous_discards == expected_discards);
}

int main(void)
{
	legacy_s8 no_opponent[] = {-1, 0, 0, 0};
	legacy_s8 same_opponent[] = "TEST";
	legacy_s8 different_opponent[] = "DIFF";
	unsigned cycle;

	game3dshapes[TEST_FIRST_CAR_SHAPE - 1U].shape3d_numverts = 17U;
	for (cycle = 0; cycle < 20U; cycle++) {
		check_cycle(same_opponent, 1U, 1U);
		check_cycle(no_opponent, 1U, 0U);
		check_cycle(different_opponent, 2U, 1U);
	}
	return 0;
}
