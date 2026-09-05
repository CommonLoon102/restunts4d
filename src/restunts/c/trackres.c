#include "externs.h"
#include "fileio.h"
#include "resource.h"

#if !defined(__BORLANDC__)
static struct PLANE decoded_planes[TRACK_PLAN_RESOURCE_COUNT];
static struct TRACK_WALL decoded_walls[TRACK_WALL_RESOURCE_COUNT];
#endif

extern struct PLANE far* planptr;
extern struct TRACK_WALL far* wallptr;

void load_track_collision_resources(void)
{
	const legacy_u8 far* plane_resource;
	const legacy_u8 far* wall_resource;

	gameresptr = file_load_resfile("game");
	plane_resource = (const legacy_u8 far*)locate_shape_alt(
		gameresptr, "plan");
	wall_resource = (const legacy_u8 far*)locate_shape_alt(
		gameresptr, "wall");
	track_collision_resources_decode(plane_resource, wall_resource);
}

#if !defined(__BORLANDC__)
struct TRACK_RESOURCE_READER {
	const legacy_u8 far* source;
	legacy_u16 offset;
};

/* Both records are flat little-endian word streams, so every field is read
   the same way and the cursor always advances by one word. */
static legacy_s16 track_resource_next_s16(struct TRACK_RESOURCE_READER* reader)
{
	legacy_s16 value;

	value = LEGACY_READ_S16_LE(reader->source + reader->offset);
	reader->offset = LEGACY_U16_WRAP_ADD(reader->offset,
		LEGACY_WORD_BYTES);
	return value;
}

static void track_resource_next_vector(struct TRACK_RESOURCE_READER* reader,
	struct VECTOR* vector)
{
	vector->x = track_resource_next_s16(reader);
	vector->y = track_resource_next_s16(reader);
	vector->z = track_resource_next_s16(reader);
}
#endif

void track_collision_resources_decode(const legacy_u8 far* plane_source,
	const legacy_u8 far* wall_source)
{
#if defined(__BORLANDC__)
	/* DOS is little-endian and these packed records have their original ABI
	 * sizes. Keep the resource-backed tables instead of duplicating nearly
	 * 19 KiB in scarce conventional memory. */
	planptr = (struct PLANE far*)plane_source;
	wallptr = (struct TRACK_WALL far*)wall_source;
#else
	struct TRACK_RESOURCE_READER reader;
	legacy_u16 index;
	legacy_u16 component;
	struct PLANE* plane;
	struct TRACK_WALL* wall;

	reader.source = plane_source;
	reader.offset = 0U;
	for (index = 0U; index < TRACK_PLAN_RESOURCE_COUNT; index++) {
		plane = &decoded_planes[index];
		plane->plane_yz = track_resource_next_s16(&reader);
		plane->plane_xy = track_resource_next_s16(&reader);
		track_resource_next_vector(&reader, &plane->plane_origin);
		track_resource_next_vector(&reader, &plane->plane_normal);
		for (component = 0U; component < MATRIX_ELEMENT_COUNT; component++) {
			plane->plane_rotation.vals[component] =
				track_resource_next_s16(&reader);
		}
	}

	reader.source = wall_source;
	reader.offset = 0U;
	for (index = 0U; index < TRACK_WALL_RESOURCE_COUNT; index++) {
		wall = &decoded_walls[index];
		wall->orientation = track_resource_next_s16(&reader);
		wall->x = track_resource_next_s16(&reader);
		wall->z = track_resource_next_s16(&reader);
	}

	planptr = decoded_planes;
	wallptr = decoded_walls;
#endif
}
