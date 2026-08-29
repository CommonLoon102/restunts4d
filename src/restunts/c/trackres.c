#include "externs.h"

static struct PLANE decoded_planes[TRACK_PLAN_RESOURCE_COUNT];
static struct TRACK_WALL decoded_walls[TRACK_WALL_RESOURCE_COUNT];

extern struct PLANE far* planptr;
extern struct TRACK_WALL far* wallptr;

static legacy_s16 track_resource_read_s16(const legacy_u8 far* source,
	legacy_u16 offset)
{
	return LEGACY_READ_S16_LE(source + offset);
}

void track_collision_resources_decode(const legacy_u8 far* plane_source,
	const legacy_u8 far* wall_source)
{
	legacy_u16 index;
	legacy_u16 component;
	legacy_u16 offset;
	struct PLANE* plane;
	struct TRACK_WALL* wall;

	offset = 0U;
	for (index = 0U; index < TRACK_PLAN_RESOURCE_COUNT; index++) {
		plane = &decoded_planes[index];
		plane->plane_yz = track_resource_read_s16(plane_source, offset);
		offset = LEGACY_U16_WRAP_ADD(offset, 2U);
		plane->plane_xy = track_resource_read_s16(plane_source, offset);
		offset = LEGACY_U16_WRAP_ADD(offset, 2U);
		plane->plane_origin.x = track_resource_read_s16(plane_source, offset);
		offset = LEGACY_U16_WRAP_ADD(offset, 2U);
		plane->plane_origin.y = track_resource_read_s16(plane_source, offset);
		offset = LEGACY_U16_WRAP_ADD(offset, 2U);
		plane->plane_origin.z = track_resource_read_s16(plane_source, offset);
		offset = LEGACY_U16_WRAP_ADD(offset, 2U);
		plane->plane_normal.x = track_resource_read_s16(plane_source, offset);
		offset = LEGACY_U16_WRAP_ADD(offset, 2U);
		plane->plane_normal.y = track_resource_read_s16(plane_source, offset);
		offset = LEGACY_U16_WRAP_ADD(offset, 2U);
		plane->plane_normal.z = track_resource_read_s16(plane_source, offset);
		offset = LEGACY_U16_WRAP_ADD(offset, 2U);
		for (component = 0U; component < 9U; component++) {
			plane->plane_rotation.vals[component] =
				track_resource_read_s16(plane_source, offset);
			offset = LEGACY_U16_WRAP_ADD(offset, 2U);
		}
	}

	offset = 0U;
	for (index = 0U; index < TRACK_WALL_RESOURCE_COUNT; index++) {
		wall = &decoded_walls[index];
		wall->orientation = track_resource_read_s16(wall_source, offset);
		offset = LEGACY_U16_WRAP_ADD(offset, 2U);
		wall->x = track_resource_read_s16(wall_source, offset);
		offset = LEGACY_U16_WRAP_ADD(offset, 2U);
		wall->z = track_resource_read_s16(wall_source, offset);
		offset = LEGACY_U16_WRAP_ADD(offset, 2U);
	}

	planptr = decoded_planes;
	wallptr = decoded_walls;
}
