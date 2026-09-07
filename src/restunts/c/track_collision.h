#ifndef RESTUNTS_TRACK_COLLISION_H
#define RESTUNTS_TRACK_COLLISION_H

#include "track_types.h"

/* Collision planes and walls for the currently selected track object. */

extern legacy_s16 planindex;
extern legacy_s16 planindex_copy;
extern legacy_s8 current_surf_type;
extern legacy_s16 wallindex;
extern legacy_s16 elRdWallRelated;
extern legacy_s16 wallHeight;
extern legacy_s16 wallStartX;
extern legacy_s16 wallStartZ;
extern legacy_s16 wallOrientation;
extern struct PLANE far *planptr;
extern struct PLANE far *current_planptr;
extern legacy_s16 elem_xCenter;
extern legacy_s16 elem_zCenter;
extern legacy_s16 terrainHeight;
extern legacy_s8 track_wall_collision_enabled;
extern struct TRACK_WALL far *wallptr;

void build_track_object(struct VECTOR *, struct VECTOR *);

#define TRACK_PLAN_RESOURCE_COUNT 536U
#define TRACK_WALL_RESOURCE_COUNT 191U

void load_track_collision_resources(void);

void track_collision_resources_decode(const legacy_u8 far *plane_source,
									  const legacy_u8 far *wall_source);

legacy_s16 plane_signed_distance(legacy_s16 plane_index, legacy_s16 x, legacy_s16 y, legacy_s16 z);

#endif
